#include "playback.hpp"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <limits>

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

std::string mimeFor(const Track& track) {
    if (audioType(track.mime)) return track.mime;
    if (track.suffix == "mp3") return "audio/mpeg";
    if (track.suffix == "flac") return "audio/flac";
    if (track.suffix == "m4a" || track.suffix == "mp4") return "audio/mp4";
    if (track.suffix == "ogg" || track.suffix == "oga" || track.suffix == "opus") return "audio/ogg";
    if (track.suffix == "wav") return "audio/wav";
    if (track.suffix == "aac") return "audio/aac";
    return "application/octet-stream";
}

class CacheWrite {
public:
    CacheWrite(Store& store, Id key, std::int64_t expected)
        : store_(store), key_(key), expected_(expected), reserved_(store.reserve(key, expected)),
          file_(reserved_ ? CreateFileW(store.partialPath(key).c_str(), GENERIC_WRITE, 0, nullptr,
                                       CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr) : INVALID_HANDLE_VALUE) {}
    ~CacheWrite() {
        file_.close();
        if (reserved_) store_.release(key_);
    }
    explicit operator bool() const { return static_cast<bool>(file_); }
    bool reserved() const { return reserved_; }
    void write(const char* data, DWORD size) {
        if (!file_) return;
        DWORD written = 0;
        if ((expected_ >= 0 && size > expected_ - bytes_) ||
            !WriteFile(file_.get(), data, size, &written, nullptr) || written != size) {
            file_.close();
            return;
        }
        bytes_ += written;
    }
    void finish(const std::string& mime) {
        if (!file_ || bytes_ == 0 || (expected_ >= 0 && expected_ != bytes_) ||
            !FlushFileBuffers(file_.get())) return;
        file_.close();
        store_.publish(key_, bytes_, mime);
    }

private:
    Store& store_;
    Id key_;
    std::int64_t expected_, bytes_ = 0;
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

void downloadPinned(Store& store, const Track& track, ServerApi& api, const std::atomic_bool& stopping) {
    auto response = api.audio(track, {}, false);
    auto mime = response->header(WINHTTP_QUERY_CONTENT_TYPE);
    if (response->status == 408 || response->status >= 500)
        throw ConnectionError("The server returned HTTP " + std::to_string(response->status) + ".");
    if (response->status != 200 || (!mime.empty() && !audioType(mime)))
        throw Error("The server did not provide a complete audio file.");
    CacheWrite cache(store, track.key, number(response->header(WINHTTP_QUERY_CONTENT_LENGTH)));
    if (!cache) {
        if (cache.reserved()) throw Error("The cache file could not be opened for writing.");
        return;
    }
    char buffer[32 * 1024];
    while (!stopping) {
        auto count = response->read(buffer, sizeof(buffer));
        if (count == 0) {
            cache.finish(mime.empty() ? mimeFor(track) : mime);
            if (store.track(track.key).bytes == 0) throw Error("The audio file could not be cached.");
            return;
        }
        cache.write(buffer, count);
        if (!cache) throw Error("The audio file could not be written to the cache.");
    }
}

} // namespace tangerine
