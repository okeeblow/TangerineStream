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

std::string mediaFilename(const Track& track) {
    auto title = wide(playlistTitle(track));
    if (title.size() > 120) {
        title.resize(120);
        if (title.back() >= 0xd800 && title.back() <= 0xdbff) title.pop_back();
    }
    constexpr char hex[] = "0123456789ABCDEF";
    std::string name;
    for (unsigned char ch : utf8(title)) {
        if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
            (ch >= '0' && ch <= '9') || ch == '-' || ch == '_') {
            name += static_cast<char>(ch);
        } else {
            name += '%';
            name += hex[ch >> 4];
            name += hex[ch & 15];
        }
    }
    return (name.empty() ? "track" : name) + "." + track.suffix;
}

bool sendAll(SOCKET client, std::string_view data) {
    while (!data.empty()) {
        int sent = send(client, data.data(), static_cast<int>(data.size()), 0);
        if (sent <= 0) return false;
        data.remove_prefix(sent);
    }
    return true;
}

bool sendHeaders(SOCKET client, const std::string& status, std::string headers) {
    return sendAll(client, "HTTP/1.1 " + status + "\r\nConnection: close\r\n" +
        headers + "\r\n");
}

void fail(SOCKET client, const char* status) {
    sendHeaders(client, status, "Content-Length: 0\r\n");
}

struct Request {
    bool head = false;
    std::string path;
    std::string range;
    bool metadata = false;
};

Request readRequest(SOCKET client, const std::atomic_bool& stopping) {
    std::string input;
    char buffer[1024];
    while (input.find("\r\n\r\n") == std::string::npos) {
        if (stopping || input.size() >= 8192) return {};
        int count = recv(client, buffer, sizeof(buffer), 0);
        if (count <= 0) return {};
        input.append(buffer, count);
    }
    auto lineEnd = input.find("\r\n");
    std::string_view first(input.data(), lineEnd);
    auto space = first.find(' ');
    if (space == std::string_view::npos) return {};
    const bool head = first.substr(0, space) == "HEAD";
    if (!head && first.substr(0, space) != "GET") return {};
    first.remove_prefix(space + 1);
    space = first.find(' ');
    if (space == std::string_view::npos || first.substr(space + 1, 7) != "HTTP/1.") return {};
    Request result{head, std::string(first.substr(0, space)), {}};
    for (auto start = lineEnd + 2; start < input.size(); start = lineEnd + 2) {
        lineEnd = input.find("\r\n", start);
        if (lineEnd == std::string::npos || lineEnd == start) break;
        std::string_view line(input.data() + start, lineEnd - start);
        const auto colon = line.find(':');
        if (colon == std::string_view::npos) continue;
        std::string name(line.substr(0, colon));
        for (auto& ch : name) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        if (name != "range" && name != "icy-metadata") continue;
        line.remove_prefix(colon + 1);
        const auto initial = line.find_first_not_of(" \t");
        if (initial != std::string_view::npos) {
            line.remove_prefix(initial);
            line = line.substr(0, line.find_last_not_of(" \t") + 1);
        }
        if (name == "icy-metadata") {
            result.metadata = line == "1";
            continue;
        }
        if (line.size() > 60 || line.substr(0, 6) != "bytes=" ||
            line.substr(6).find_first_not_of("0123456789-") != std::string_view::npos) return {};
        result.range = line;
    }
    if (result.metadata && result.range == "bytes=0-") result.range.clear();
    return result;
}

// in_url asks for ICY metadata and uses StreamTitle when it refreshes the playlist.
// Insert metadata only on the downstream connection. Cache files contain original audio.
class StreamBody {
public:
    StreamBody(SOCKET client, const Request& request, const Track& track)
        : client_(client), framed_(request.metadata && request.range.empty()) {
        auto title = playlistTitle(track);
        for (char& ch : title)
            if (static_cast<unsigned char>(ch) < 32 || ch == 127) ch = ' ';
        if (title.size() > 2048) {
            size_t end = 2048;
            while ((static_cast<unsigned char>(title[end]) & 0xc0) == 0x80) --end;
            title.resize(end);
        }
        headers = "icy-name: " + title + "\r\n";
        if (!framed_) return;
        headers += "icy-metaint: " + std::to_string(interval) + "\r\n";
        auto field = "StreamTitle='" + title + "';";
        const size_t blocks = (field.size() + 15) / 16;
        metadata_.assign(1, static_cast<char>(blocks));
        metadata_ += field;
        metadata_.resize(1 + blocks * 16, '\0');
    }

    std::string lengthHeader(std::int64_t size) const {
        if (size < 0) return {};
        if (framed_ && size >= static_cast<std::int64_t>(interval)) {
            // Each completed interval has a length byte. Only the first has a title.
            const auto overhead = size / static_cast<std::int64_t>(interval) +
                                  static_cast<std::int64_t>(metadata_.size() - 1);
            if (size > std::numeric_limits<std::int64_t>::max() - overhead) return {};
            size += overhead;
        }
        return "Content-Length: " + std::to_string(size) + "\r\n";
    }

    bool send(std::string_view bytes) {
        if (!framed_) return sendAll(client_, bytes);
        while (!bytes.empty()) {
            auto n = std::min(bytes.size(), remaining_);
            if (!sendAll(client_, bytes.substr(0, n))) return false;
            bytes.remove_prefix(n);
            remaining_ -= n;
            if (!remaining_) {
                if (!sendAll(client_, metadata_)) return false;
                metadata_.assign(1, '\0');
                remaining_ = interval;
            }
        }
        return true;
    }
    std::string headers;
private:
    static constexpr size_t interval = 8192;
    SOCKET client_;
    bool framed_;
    size_t remaining_ = interval;
    std::string metadata_;
};

struct ByteRange {
    std::int64_t start;
    std::int64_t end;
};

ByteRange resolveRange(std::string_view range, std::int64_t size) {
    if (size <= 0) return {-1, -1};
    if (range.empty()) return {0, size - 1};
    range.remove_prefix(6);
    auto dash = range.find('-');
    if (dash == std::string_view::npos) return {-1, -1};
    auto tail = range.substr(dash + 1);
    if (dash == 0) {
        auto count = number(tail);
        return count > 0 ? ByteRange{size - std::min(count, size), size - 1} : ByteRange{-1, -1};
    }
    auto start = number(range.substr(0, dash));
    auto end = tail.empty() ? size - 1 : std::min(size - 1, number(tail));
    return start >= 0 && start <= end ? ByteRange{start, end} : ByteRange{-1, -1};
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

void sendCached(SOCKET client, const Request& request, const Track& track, const File& file,
                const std::atomic_bool& stopping) {
    LARGE_INTEGER size{};
    if (!GetFileSizeEx(file.get(), &size) || size.QuadPart <= 0) {
        fail(client, "500 Internal Server Error");
        return;
    }
    const auto range = resolveRange(request.range, size.QuadPart);
    if (range.start < 0) {
        sendHeaders(client, "416 Range Not Satisfiable", "Content-Length: 0\r\nContent-Range: bytes */" +
                    std::to_string(size.QuadPart) + "\r\n");
        return;
    }
    auto remaining = range.end - range.start + 1;
    const bool partial = !request.range.empty();
    StreamBody body(client, request, track);
    std::string headers = "Accept-Ranges: bytes\r\nContent-Type: " + mimeFor(track) +
        "\r\n" + body.headers + body.lengthHeader(remaining);
    if (partial)
        headers += "Content-Range: bytes " + std::to_string(range.start) + "-" +
                   std::to_string(range.end) + "/" + std::to_string(size.QuadPart) + "\r\n";
    LARGE_INTEGER start{};
    start.QuadPart = range.start;
    if (!SetFilePointerEx(file.get(), start, nullptr, FILE_BEGIN)) {
        fail(client, "500 Internal Server Error");
        return;
    }
    if (!sendHeaders(client, partial ? "206 Partial Content" : "200 OK", std::move(headers)) ||
        request.head) return;
    char buffer[32 * 1024];
    while (remaining > 0 && !stopping) {
        DWORD count = 0;
        if (!ReadFile(file.get(), buffer, static_cast<DWORD>(std::min<std::int64_t>(remaining, sizeof(buffer))),
                      &count, nullptr) || count == 0 || !body.send({buffer, count})) return;
        remaining -= count;
    }
}

bool completeRange(const std::string& range, std::int64_t length) {
    return length > 0 && range == "bytes 0-" + std::to_string(length - 1) + "/" + std::to_string(length);
}

void relay(SOCKET client, const Request& request, const Track& track, Response& upstream,
           Store& store, const std::atomic_bool& stopping) {
    auto mime = upstream.header(WINHTTP_QUERY_CONTENT_TYPE);
    auto length = number(upstream.header(WINHTTP_QUERY_CONTENT_LENGTH));
    if (upstream.status == 416) {
        auto range = upstream.header(L"Content-Range");
        sendHeaders(client, "416 Range Not Satisfiable", "Content-Length: 0\r\nContent-Range: " + range + "\r\n");
        return;
    }
    if ((upstream.status != 200 && upstream.status != 206) || (!mime.empty() && !audioType(mime))) {
        fail(client, "502 Bad Gateway");
        return;
    }
    auto range = upstream.header(L"Content-Range");
    auto acceptsRanges = upstream.header(L"Accept-Ranges");
    if (mime.empty()) mime = mimeFor(track);
    auto bytesToSend = length;
    std::int64_t skip = 0;
    auto downstreamStatus = upstream.status;
    if (!request.range.empty() && upstream.status == 200 && length > 0) {
        auto requested = resolveRange(request.range, length);
        if (requested.start < 0) {
            sendHeaders(client, "416 Range Not Satisfiable", "Content-Length: 0\r\nContent-Range: bytes */" +
                        std::to_string(length) + "\r\n");
            return;
        }
        skip = requested.start;
        bytesToSend = requested.end - requested.start + 1;
        range = "bytes " + std::to_string(requested.start) + "-" +
                std::to_string(requested.end) + "/" + std::to_string(length);
        downstreamStatus = 206;
        acceptsRanges = "bytes";
    }
    StreamBody body(client, request, track);
    auto headers = "Content-Type: " + mime + "\r\n" + body.headers + body.lengthHeader(bytesToSend);
    if (!range.empty()) headers += "Content-Range: " + range + "\r\n";
    if (!acceptsRanges.empty()) headers += "Accept-Ranges: " + acceptsRanges + "\r\n";
    if (!sendHeaders(client, downstreamStatus == 206 ? "206 Partial Content" : "200 OK", std::move(headers)) ||
        request.head) return;

    std::unique_ptr<CacheWrite> cache;
    const bool full = (upstream.status == 200 && skip == 0 && bytesToSend == length) ||
                      (upstream.status == 206 && completeRange(range, length));
    if (full && length > 0) {
        // A cache failure must never interrupt playback.
        try { cache = std::make_unique<CacheWrite>(store, track.key, length); } catch (...) {}
    }
    char buffer[32 * 1024];
    try {
        auto remaining = bytesToSend;
        while (remaining != 0 && !stopping) {
            auto limit = remaining < 0 ? sizeof(buffer) :
                static_cast<std::size_t>(std::min<std::int64_t>(skip + remaining, sizeof(buffer)));
            DWORD count = upstream.read(buffer, static_cast<DWORD>(limit));
            if (!count) break;
            auto discard = static_cast<DWORD>(std::min<std::int64_t>(skip, count));
            skip -= discard;
            count -= discard;
            if (count == 0) continue;
            if (cache) cache->write(buffer + discard, count);
            if (!body.send({buffer + discard, count})) return;
            if (remaining > 0) remaining -= count;
        }
        if (cache && remaining == 0) cache->finish(mime);
    } catch (...) {
        // The response is already open. Closing it lets the player handle the truncated body.
    }
}

} // namespace

Playback::Playback(Store& store) : store_(store), pathPrefix_("/" + store.playbackSecret() + "/") {
    WSADATA data{};
    if (WSAStartup(MAKEWORD(2, 2), &data) != 0)
        throw Error("Windows could not initialize local playback.");
    try {
        listener_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (listener_ == INVALID_SOCKET) throw Error("Windows could not open the local playback socket.");
        BOOL exclusive = TRUE;
        if (setsockopt(listener_, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, reinterpret_cast<const char*>(&exclusive),
                       sizeof(exclusive)) == SOCKET_ERROR) throw Error("Could not configure local playback.");
        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        address.sin_port = htons(static_cast<u_short>(store.playbackPort()));
        if (bind(listener_, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == SOCKET_ERROR) {
            address.sin_port = 0;
            if (bind(listener_, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == SOCKET_ERROR)
                throw Error("Could not bind the local playback socket.");
        }
        int length = sizeof(address);
        if (getsockname(listener_, reinterpret_cast<sockaddr*>(&address), &length) == SOCKET_ERROR ||
            ::listen(listener_, SOMAXCONN) == SOCKET_ERROR) throw Error("Could not start local playback.");
        const auto port = ntohs(address.sin_port);
        store.savePlaybackPort(port);
        urlPrefix_ = "http://127.0.0.1:" + std::to_string(port) + pathPrefix_;
        for (int i = 0; i < 4; ++i) workers_.emplace_back([this] { work(); });
        listenerThread_ = std::thread([this] { listen(); });
    } catch (...) {
        stop();
        throw;
    }
}

Playback::~Playback() { stop(); }

void Playback::stop() {
    {
        std::scoped_lock lock(queueMutex_);
        stopping_ = true;
        for (auto client : clients_) closesocket(client);
        clients_.clear();
    }
    queueReady_.notify_all();
    if (listenerThread_.joinable()) listenerThread_.join();
    if (listener_ != INVALID_SOCKET) closesocket(listener_);
    for (auto& worker : workers_) worker.join();
    WSACleanup();
}

std::wstring Playback::url(const Track& track) const {
    return wide(urlPrefix_ + std::to_string(track.key) + "/" + mediaFilename(track));
}

Id Playback::key(std::wstring_view url) const {
    auto encoded = utf8(url);
    if (encoded.compare(0, urlPrefix_.size(), urlPrefix_) != 0) return 0;
    return pathKey(std::string_view(encoded).substr(urlPrefix_.size() - pathPrefix_.size()));
}

Id Playback::pathKey(std::string_view path) const {
    if (path.substr(0, pathPrefix_.size()) != pathPrefix_) return 0;
    path.remove_prefix(pathPrefix_.size());
    auto slash = path.find('/');
    if (slash == std::string_view::npos || path.size() == slash + 1) return 0;
    auto id = number(path.substr(0, slash));
    return id > 0 ? id : 0;
}

void Playback::listen() {
    while (!stopping_) {
        fd_set ready;
        FD_ZERO(&ready);
        FD_SET(listener_, &ready);
        timeval wait{0, 250'000};
        auto selected = select(0, &ready, nullptr, nullptr, &wait);
        if (selected == SOCKET_ERROR) return;
        if (!selected || stopping_) continue;
        auto client = accept(listener_, nullptr, nullptr);
        if (client == INVALID_SOCKET) continue;
        DWORD timeout = 3000;
        setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));
        setsockopt(client, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));
        {
            std::scoped_lock lock(queueMutex_);
            if (stopping_ || clients_.size() >= 12) closesocket(client);
            else clients_.push_back(client);
        }
        queueReady_.notify_one();
    }
}

void Playback::work() {
    for (;;) {
        SOCKET client;
        {
            std::unique_lock lock(queueMutex_);
            queueReady_.wait(lock, [this] { return stopping_ || !clients_.empty(); });
            if (stopping_) return;
            client = clients_.front();
            clients_.pop_front();
        }
        try { serve(client); } catch (...) { fail(client, "502 Bad Gateway"); }
        shutdown(client, SD_SEND);
        closesocket(client);
    }
}

void Playback::serve(SOCKET client) {
    auto request = readRequest(client, stopping_);
    auto key = pathKey(request.path);
    auto track = key ? store_.track(key) : Track{};
    if (!track.key) {
        fail(client, "404 Not Found");
        return;
    }
    File file(track.bytes > 0 ? CreateFileW(store_.audioPath(key).c_str(), GENERIC_READ, FILE_SHARE_READ,
                                           nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr) :
                                INVALID_HANDLE_VALUE);
    if (file) {
        if (!request.head) store_.touch(key);
        sendCached(client, request, track, file, stopping_);
        return;
    }
    if (!store_.networkAvailable(track.server)) {
        fail(client, "503 Service Unavailable");
        return;
    }
    try {
        auto api = connect(store_.server(track.server));
        auto upstream = api->audio(track, request.range, request.head);
        if (request.head && (upstream->status == 404 || upstream->status == 405))
            upstream = api->audio(track, request.range, false);
        if (upstream->status == 408 || upstream->status >= 500) store_.connectionFailed(track.server);
        relay(client, request, track, *upstream, store_, stopping_);
    } catch (const ConnectionError&) {
        store_.connectionFailed(track.server);
        fail(client, "503 Service Unavailable");
    }
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
