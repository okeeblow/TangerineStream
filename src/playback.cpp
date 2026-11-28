#include "playback.hpp"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <limits>
#include <optional>
#include <netlistmgr.h>

namespace tangerine {
namespace {

class File {
public:
    explicit File(HANDLE handle = INVALID_HANDLE_VALUE) : handle_(handle) {}
    ~File() { close(); }
    File(const File&) = delete;
    File& operator=(const File&) = delete;
    explicit operator bool() const { return handle_ != INVALID_HANDLE_VALUE; }
    HANDLE get() const { return handle_; }
    void close() {
        if (*this) CloseHandle(handle_);
        handle_ = INVALID_HANDLE_VALUE;
    }

private:
    HANDLE handle_;
};

std::int64_t number(std::string_view value) {
    if (value.empty()) return -1;
    std::int64_t result = -1;
    auto parsed = std::from_chars(value.data(), value.data() + value.size(), result);
    return parsed.ec == std::errc{} && parsed.ptr == value.data() + value.size() && result >= 0 ? result : -1;
}

bool audioType(std::string_view mime) {
    if (mime.find_first_of("\r\n") != std::string_view::npos ||
        mime.find('\0') != std::string_view::npos) return false;
    return mime.substr(0, 6) == "audio/" || mime.substr(0, 9) == "video/mp4" ||
        mime == "application/ogg" || mime == "application/x-ogg" ||
        mime == "application/octet-stream" || mime == "application/flac" || mime == "application/x-flac";
}

void checkAudioResponse(Response& response, AudioStream& stream, const InputExtensions& inputs) {
    if (response.status == 408 || response.status == 429 || response.status >= 500)
        throw ConnectionError("The server returned HTTP " + std::to_string(response.status) + ".");
    auto mime = response.header(WINHTTP_QUERY_CONTENT_TYPE);
    mime = mime.substr(0, mime.find(';'));
    if ((response.status != 200 && response.status != 206) || (!mime.empty() && !audioType(mime)))
        throw Error("The server did not return an audio file.");
    if (stream.format.empty()) return;
    // Some legacy servers send AAC in MP4 even when asked for a bare AAC stream.
    if (mime == "audio/mp4" || mime == "video/mp4" || mime == "audio/x-m4a")
        stream.format = inputs.count("m4a") ? "m4a" : "mp4";
    else if (mime == "audio/aac") stream.format = "aac";
    else if (mime == "audio/ogg" && stream.format != "opus") stream.format = "ogg";
    else if (mime == "audio/mpeg") stream.format = "mp3";
    else if (mime == "audio/flac" || mime == "audio/x-flac") stream.format = "flac";
}

class PlaybackDownload {
public:
    PlaybackDownload(Store& store, Id key) : store_(store), key_(key),
        path_(store.partialPath(key).parent_path() / (L"play-" + wide(randomHex()) + L".part")),
        file_(CreateFileW(path_.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr)) {
        if (!file_) throw Error("Could not create a playback file.");
    }
    ~PlaybackDownload() {
        file_.close();
        if (!path_.empty()) DeleteFileW(path_.c_str());
        if (reserved_) store_.release(key_);
    }
    void start(std::int64_t length, bool retain) {
        LARGE_INTEGER zero{};
        if (!SetFilePointerEx(file_.get(), zero, nullptr, FILE_BEGIN) || !SetEndOfFile(file_.get()))
            throw Error("Could not restart the playback download.");
        bytes = 0;
        if (retain && !reserved_) reserved_ = store_.reserve(key_, length, false);
        reservedBytes_ = std::max<std::int64_t>(0, length);
        if (reserved_ && !store_.growReservation(key_, reservedBytes_)) discardReservation();
    }
    void write(const char* data, DWORD size) {
        if (reserved_ && bytes + size > reservedBytes_) {
            reservedBytes_ = ((bytes + size + MiB - 1) / MiB) * MiB;
            if (!store_.growReservation(key_, reservedBytes_)) discardReservation();
        }
        DWORD written = 0;
        if (!WriteFile(file_.get(), data, size, &written, nullptr) || written != size)
            throw Error("Could not write the playback file. Check the free disk space.");
        bytes += written;
    }
    HANDLE finish(const Track& track, const std::string& format, std::filesystem::path& result, bool& temporary) {
        file_.close();
        HANDLE lease = INVALID_HANDLE_VALUE;
        result = path_;
        if (reserved_ && store_.publish(key_, bytes, format, result, &lease)) {
            path_.clear();
            temporary = false;
        } else {
            const auto& suffix = format.empty() ? track.suffix : format;
            if (suffix.empty() || suffix.size() > 12 ||
                suffix.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789") != suffix.npos)
                throw Error("Invalid audio file extension.");
            result.replace_extension(wide(suffix));
            if (!MoveFileW(path_.c_str(), result.c_str())) throw Error("Could not name the playback file.");
            path_ = result;
            lease = CreateFileW(result.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
            temporary = true;
            if (lease != INVALID_HANDLE_VALUE) path_.clear();
        }
        if (lease == INVALID_HANDLE_VALUE) throw Error("Could not open the downloaded track for playback.");
        return lease;
    }
    std::int64_t bytes = 0;
private:
    void discardReservation() { store_.release(key_); reserved_ = false; }
    Store& store_;
    Id key_;
    std::filesystem::path path_;
    File file_;
    std::int64_t reservedBytes_ = 0;
    bool reserved_ = false;
};

std::string validator(Response& response) {
    auto tag = response.header(WINHTTP_QUERY_ETAG);
    if (tag.size() >= 2 && tag.front() == '"' && tag.back() == '"') return tag;
    return response.header(WINHTTP_QUERY_LAST_MODIFIED);
}

class CacheWrite {
public:
    CacheWrite(Store& store, const Track& track, std::int64_t expected, bool upgrade)
        : store_(store), key_(track.key), expected_(expected),
          limit_(track.pinned || track.favorite ? INT64_MAX : store.rollingMiB() * MiB),
          reserved_(store.reserve(key_, expected, upgrade)),
          file_(reserved_ ? CreateFileW(store.partialPath(key_).c_str(), GENERIC_WRITE, 0, nullptr,
                                       CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr) : INVALID_HANDLE_VALUE) {}
    ~CacheWrite() {
        file_.close();
        if (reserved_) store_.release(key_);
    }
    explicit operator bool() const { return static_cast<bool>(file_); }
    bool reserved() const { return reserved_; }
    void write(const char* data, DWORD size) {
        if (!file_) return;
        // Transcoded HTTP streams often omit Content-Length. Reserve another block
        // before writing it so a partial file also counts against the rolling budget.
        if (expected_ < 0 && bytes_ + size > reservedBytes_) {
            reservedBytes_ = std::min(limit_, ((bytes_ + size + MiB - 1) / MiB) * MiB);
            if (!store_.growReservation(key_, reservedBytes_)) { file_.close(); return; }
        }
        DWORD written = 0;
        if (size > limit_ - bytes_ || (expected_ >= 0 && size > expected_ - bytes_) ||
            !WriteFile(file_.get(), data, size, &written, nullptr) || written != size) {
            file_.close();
            return;
        }
        bytes_ += written;
    }
    bool finish(const std::string& format) {
        if (!file_ || bytes_ == 0 || (expected_ >= 0 && expected_ != bytes_) ||
            !FlushFileBuffers(file_.get())) return false;
        file_.close();
        auto path = store_.partialPath(key_);
        return store_.publish(key_, bytes_, format, path);
    }

private:
    Store& store_;
    Id key_;
    std::int64_t expected_, limit_, bytes_ = 0, reservedBytes_ = 0;
    bool reserved_;
    File file_;
};

} // namespace

Playback::Playback(Store& store, const std::filesystem::path& profile)
    : items_(profile / L"items") {
    items_.make_preferred();
    const auto port = store.playbackPort();
    const auto secret = store.playbackSecret();
    if (port && !secret.empty())
        legacy_ = L"http://127.0.0.1:" + std::to_wstring(port) + L"/" + wide(secret) + L"/";
}

std::filesystem::path Playback::reference(const Track& track) const {
    auto label = wide(playlistTitle(track));
    for (auto& ch : label)
        if (ch < 32 || std::wstring_view(L"<>:\"/\\|?*").find(ch) != std::wstring_view::npos) ch = L'_';
    if (label.size() > 96) {
        label.resize(96);
        if (label.back() >= 0xd800 && label.back() <= 0xdbff) label.pop_back();
    }
    // A prefix also keeps Windows device names such as CON out of the filename.
    return items_ / std::to_wstring(track.key) / (L"Song - " + label + L".tstream");
}

std::wstring Playback::playlistFile(const Track& track) const {
    auto file = reference(track);
    std::filesystem::create_directories(file.parent_path());
    File marker(CreateFileW(file.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                            nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr));
    if (!marker) throw Error("Could not create the track reference for WACUP.");
    return file.wstring();
}

Id Playback::key(std::wstring_view name) const {
    std::wstring_view digits;
    std::wstring local;
    if (!legacy_.empty() && name.substr(0, legacy_.size()) == legacy_) {
        auto rest = name.substr(legacy_.size());
        auto slash = rest.find(L'/');
        if (slash == rest.npos || slash + 1 == rest.size()) return 0;
        digits = rest.substr(0, slash);
    } else {
        if (name.size() < 8 || _wcsnicmp(name.data() + name.size() - 8, L".tstream", 8)) return 0;
        if (name.find(L'/') != name.npos) {
            local.assign(name);
            std::replace(local.begin(), local.end(), L'/', L'\\');
            name = local;
        }
        const auto& prefix = items_.native();
        if (name.size() <= prefix.size()+1 || _wcsnicmp(name.data(), prefix.c_str(), prefix.size()) ||
            name[prefix.size()] != L'\\') return 0;
        auto rest = name.substr(prefix.size() + 1);
        auto slash = rest.find(L'\\');
        if (slash == rest.npos) return 0;
        auto leaf = rest.substr(slash + 1);
        if (leaf.size() <= 8 || leaf.find_first_of(L"/\\:") != leaf.npos) return 0;
        digits = rest.substr(0, slash);
    }
    if (digits.empty() || digits.size() > 19 || digits.find_first_not_of(L"0123456789") != digits.npos) return 0;
    auto id = number(utf8(digits));
    return id > 0 ? id : 0;
}

bool meteredInternet() {
    const HRESULT init = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    INetworkCostManager* manager = nullptr;
    DWORD cost = NLM_CONNECTION_COST_UNKNOWN;
    if (SUCCEEDED(CoCreateInstance(CLSID_NetworkListManager, nullptr, CLSCTX_ALL,
                                   IID_PPV_ARGS(&manager)))) {
        if (FAILED(manager->GetCost(&cost, nullptr))) cost = NLM_CONNECTION_COST_UNKNOWN;
        manager->Release();
    }
    if (SUCCEEDED(init)) CoUninitialize();
    return (cost & 0xffff) != NLM_CONNECTION_COST_UNRESTRICTED ||
        (cost & (NLM_CONNECTION_COST_ROAMING | NLM_CONNECTION_COST_OVERDATALIMIT |
                 NLM_CONNECTION_COST_APPROACHINGDATALIMIT)) != 0;
}

HANDLE prepareAudio(Store& store, HttpClient& http, const Track& track, const Server& profile,
                    const InputExtensions& inputs, unsigned& excluded, const std::function<bool()>& cancelled,
                    std::filesystem::path& path, bool& temporary) {
    auto api = http.connect(profile);
    for (;;) {
        if (cancelled()) throw Error("Playback cancelled.");
        if (!store.networkAvailable(track.server)) throw Error("The server is offline.");
        const bool metered = meteredInternet();
        const auto& quality = metered ? profile.metered : profile.unmetered;
        const bool retain = !metered || track.favorite || track.pinned;
        const unsigned previousChoices = excluded;
        bool costChanged = false;
        auto nextCheck = std::chrono::steady_clock::now() + std::chrono::seconds(1);
        auto continueTransfer = [&] {
            if (cancelled()) throw Error("Playback cancelled.");
            auto now = std::chrono::steady_clock::now();
            if (now >= nextCheck) {
                if (!store.networkAvailable(track.server)) throw Error("The server is offline.");
                costChanged = meteredInternet() != metered;
                nextCheck = now + std::chrono::seconds(1);
            }
            return !costChanged;
        };
        auto stream = api->stream(track, quality, inputs, excluded);
        excluded |= stream.choice;
        PlaybackDownload file(store, track.key);
        std::int64_t total = -1;
        std::string version;
        for (int attempt = 0; continueTransfer(); ++attempt) {
            const bool resume = file.bytes > 0 && total > file.bytes && !version.empty();
            const auto headers = resume ? L"Range: bytes=" + std::to_wstring(file.bytes) +
                L"-\r\nIf-Range: " + wide(version) + L"\r\n" : L"";
            try {
                Response response(http, stream.url, L"GET", headers);
                if (resume && response.status == 416) { version.clear(); throw ConnectionError("The audio file changed."); }
                checkAudioResponse(response, stream, inputs);
                if (!stream.format.empty() && !acceptsExtension(inputs, stream.format)) break;
                const auto length = number(response.header(WINHTTP_QUERY_CONTENT_LENGTH));
                if (response.status == 206) {
                    const auto range = "bytes " + std::to_string(file.bytes) + "-" + std::to_string(total-1) + "/" + std::to_string(total);
                    auto current = validator(response);
                    if (!resume || response.header(WINHTTP_QUERY_CONTENT_RANGE) != range ||
                        (!current.empty() && current != version) || (length >= 0 && length != total - file.bytes)) {
                        version.clear();
                        throw ConnectionError("The server returned an inconsistent audio range.");
                    }
                } else {
                    total = length;
                    version = stream.choice == originalSource ? validator(response) : std::string{};
                    file.start(total, retain);
                }
                char buffer[64 * 1024];
                while (auto count = response.read(buffer, sizeof(buffer))) {
                    if (!continueTransfer()) break;
                    if (total >= 0 && count > total - file.bytes) throw Error("The server exceeded the audio file length.");
                    file.write(buffer, count);
                }
                if (!continueTransfer()) break;
                if (!file.bytes || (total >= 0 && file.bytes != total))
                    throw ConnectionError("The audio download ended early.");
                return file.finish(track, stream.format, path, temporary);
            } catch (const ConnectionError&) {
                if (attempt >= 2) throw;
            }
        }
        if (costChanged) excluded = previousChoices;
    }
}

void downloadAudio(Store& store, HttpClient& http, const Track& track, const Server& profile, ServerApi& api,
                   bool metered, const std::atomic_bool& stopping, const InputExtensions& inputs,
                   unsigned excluded, bool (*currentCost)()) {
    const bool upgrade = !metered && profile.upgradeFavorites && track.favorite &&
        !(excluded & originalSource) && acceptsExtension(inputs, track.suffix);
    if (track.bytes && !(upgrade && !track.cachedFormat.empty())) return;
    if (metered && !track.favorite && !track.pinned) return;
    const auto quality = upgrade ? StreamQuality{"raw", 0} : (metered ? profile.metered : profile.unmetered);
    AudioStream stream;
    std::optional<Response> response;
    for (;;) {
        stream = api.stream(track, quality, inputs, excluded);
        if (track.bytes && stream.choice != originalSource) {
            // A shared container can hold a codec the available decoder cannot play.
            // Keep the existing favorite when the server also recommends a transcode.
            store.postponePin(track.key);
            return;
        }
        if (stopping || !store.networkAvailable(track.server) || currentCost() != metered) return;
        response.emplace(http, stream.url, L"GET", L"");
        checkAudioResponse(*response, stream, inputs);
        if (response->status != 200)
            throw Error("The server did not provide a complete audio file.");
        if (stream.format.empty() || acceptsExtension(inputs, stream.format)) break;
        // A legacy encoder can return a different container from the one requested.
        excluded |= stream.choice;
    }
    CacheWrite cache(store, track, number(response->header(WINHTTP_QUERY_CONTENT_LENGTH)), upgrade);
    if (!cache) {
        if (cache.reserved()) throw Error("The cache file could not be opened for writing.");
        return;
    }
    char buffer[32 * 1024];
    auto nextCheck = std::chrono::steady_clock::now();
    while (!stopping) {
        if (std::chrono::steady_clock::now() >= nextCheck) {
            if (!store.networkAvailable(track.server) || currentCost() != metered) return;
            nextCheck = std::chrono::steady_clock::now() + std::chrono::seconds(1);
        }
        auto count = response->read(buffer, sizeof(buffer));
        if (count == 0) {
            if (!cache.finish(stream.format)) throw Error("The audio file could not be cached.");
            return;
        }
        cache.write(buffer, count);
        if (!cache) throw Error("The audio file could not be written to the cache.");
    }
}

} // namespace tangerine
