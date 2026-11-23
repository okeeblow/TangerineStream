#include "server.hpp"

#include <bcrypt.h>
#include <algorithm>
#include <charconv>
#include <chrono>
#include <climits>
#include <cstdlib>
#include <limits>
#include <map>
#include <set>
#include <variant>

namespace tangerine {
namespace {

std::string hex(const unsigned char* data, std::size_t count) {
    constexpr char digits[] = "0123456789abcdef";
    std::string result;
    result.reserve(count * 2);
    for (std::size_t i = 0; i < count; ++i) {
        result += digits[data[i] >> 4];
        result += digits[data[i] & 15];
    }
    return result;
}

std::string md5(const std::string& value) {
    BCRYPT_ALG_HANDLE algorithm = nullptr;
    unsigned char digest[16];
    if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_MD5_ALGORITHM, nullptr, 0))
        throw Error("Windows could not initialize Subsonic token authentication.");
    auto status = BCryptHash(algorithm, nullptr, 0,
        reinterpret_cast<PUCHAR>(const_cast<char*>(value.data())),
        static_cast<ULONG>(value.size()), digest, sizeof(digest));
    BCryptCloseAlgorithmProvider(algorithm, 0);
    if (status) throw Error("Windows could not create a Subsonic token.");
    return hex(digest, sizeof(digest));
}

std::string percent(std::string_view value) {
    std::string result;
    for (unsigned char ch : value) {
        if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
            (ch >= '0' && ch <= '9') || ch == '-' || ch == '.' || ch == '_' || ch == '~') {
            result += static_cast<char>(ch);
        } else {
            result += '%';
            result += hex(&ch, 1);
        }
    }
    return result;
}

std::string queryString(const Parameters& parameters) {
    std::string result;
    for (const auto& [key, value] : parameters) {
        result += result.empty() ? '?' : '&';
        result += key + "=" + percent(value);
    }
    return result;
}

struct Address {
    std::wstring host, target;
    INTERNET_PORT port;
    bool secure;
};

Address splitUrl(const std::wstring& url) {
    URL_COMPONENTS parts{};
    parts.dwStructSize = sizeof(parts);
    parts.dwHostNameLength = parts.dwUrlPathLength = parts.dwExtraInfoLength =
        parts.dwUserNameLength = parts.dwPasswordLength = DWORD(-1);
    if (!WinHttpCrackUrl(url.c_str(), 0, 0, &parts) || !parts.dwHostNameLength ||
        parts.dwUserNameLength || parts.dwPasswordLength ||
        (parts.nScheme != INTERNET_SCHEME_HTTP && parts.nScheme != INTERNET_SCHEME_HTTPS))
        throw Error("Enter an HTTP or HTTPS server address.");
    std::wstring target = parts.dwUrlPathLength
        ? std::wstring(parts.lpszUrlPath, parts.dwUrlPathLength) : L"/";
    if (parts.dwExtraInfoLength) target.append(parts.lpszExtraInfo, parts.dwExtraInfoLength);
    return {{parts.lpszHostName, parts.dwHostNameLength}, std::move(target),
            parts.nPort, parts.nScheme == INTERNET_SCHEME_HTTPS};
}

[[noreturn]] void networkError() {
    throw Error("Server connection failed (Windows error " + std::to_string(GetLastError()) + ").");
}

struct Json {
    using Array = std::vector<Json>;
    using Object = std::map<std::string, Json, std::less<>>;
    std::variant<std::monostate, bool, double, std::string, Array, Object> value;

    const Json& at(std::string_view key) const {
        static const Json missing{};
        auto object = std::get_if<Object>(&value);
        if (!object) return missing;
        auto entry = object->find(key);
        return entry == object->end() ? missing : entry->second;
    }
    const Array& array() const {
        static const Array empty;
        auto array = std::get_if<Array>(&value);
        return array ? *array : empty;
    }
    std::string string() const {
        auto text = std::get_if<std::string>(&value);
        return text ? *text : "";
    }
    int seconds() const {
        auto number = std::get_if<double>(&value);
        return number && *number > 0 && *number <= INT_MAX ? static_cast<int>(*number) : 0;
    }
};

constexpr unsigned char controlLimit = 0x20;

class JsonParser {
public:
    explicit JsonParser(std::string_view input) : input_(input) {}

    Json parse() {
        auto result = value(0);
        space();
        if (!input_.empty()) fail();
        return result;
    }

private:
    std::string_view input_;

    [[noreturn]] static void fail() { throw Error("The server returned invalid JSON."); }
    char pop() {
        if (input_.empty()) fail();
        char ch = input_.front();
        input_.remove_prefix(1);
        return ch;
    }
    void space() {
        while (!input_.empty() && std::string_view(" \t\r\n").find(input_.front()) !=
            std::string_view::npos) input_.remove_prefix(1);
    }
    bool take(char ch) {
        space();
        if (input_.empty() || input_.front() != ch) return false;
        input_.remove_prefix(1);
        return true;
    }
    void require(char ch) { if (!take(ch)) fail(); }
    bool digit() const { return !input_.empty() && input_.front() >= '0' && input_.front() <= '9'; }
    void digits() {
        if (!digit()) fail();
        while (digit()) input_.remove_prefix(1);
    }
    void literal(std::string_view word) {
        if (input_.substr(0, word.size()) != word) fail();
        input_.remove_prefix(word.size());
    }
    unsigned codeUnit() {
        unsigned number = 0;
        for (int i = 0; i < 4; ++i) {
            char ch = pop();
            if (ch >= '0' && ch <= '9') number = number * 16 + (ch - '0');
            else if (ch >= 'a' && ch <= 'f') number = number * 16 + (ch - 'a' + 10);
            else if (ch >= 'A' && ch <= 'F') number = number * 16 + (ch - 'A' + 10);
            else fail();
        }
        return number;
    }
    std::string string() {
        require('"');
        std::string result;
        for (char ch = pop(); ch != '"'; ch = pop()) {
            if (static_cast<unsigned char>(ch) < controlLimit) fail();
            if (ch != '\\') {
                result += ch;
                continue;
            }
            ch = pop();
            if (ch == 'u') {
                unsigned first = codeUnit();
                std::wstring code{static_cast<wchar_t>(first)};
                if (first >= 0xd800 && first <= 0xdbff) {
                    literal("\\u");
                    unsigned second = codeUnit();
                    if (second < 0xdc00 || second > 0xdfff) fail();
                    code += static_cast<wchar_t>(second);
                } else if (first >= 0xdc00 && first <= 0xdfff) fail();
                result += utf8(code);
            } else {
                auto pos = std::string_view("\"\\/bfnrt").find(ch);
                if (pos == std::string_view::npos) fail();
                result += "\"\\/\b\f\n\r\t"[pos];
            }
        }
        return result;
    }
    double number() {
        const char* start = input_.data();
        if (!input_.empty() && input_.front() == '-') pop();
        if (input_.empty()) fail();
        if (input_.front() == '0') pop();
        else digits();
        if (!input_.empty() && input_.front() == '.') {
            pop();
            digits();
        }
        if (!input_.empty() && (input_.front() == 'e' || input_.front() == 'E')) {
            pop();
            if (!input_.empty() && (input_.front() == '-' || input_.front() == '+')) pop();
            digits();
        }
        double result = 0;
        auto conversion = std::from_chars(start, input_.data(), result);
        if (conversion.ec != std::errc{}) fail();
        return result;
    }
    Json value(int depth) {
        space();
        if (input_.empty() || depth == 64) fail();
        switch (input_.front()) {
        case '{': {
            pop();
            Json::Object object;
            if (!take('}')) {
                do {
                    auto key = string();
                    require(':');
                    object.insert_or_assign(std::move(key), value(depth + 1));
                } while (take(','));
                require('}');
            }
            return {std::move(object)};
        }
        case '[': {
            pop();
            Json::Array array;
            if (!take(']')) {
                do { array.push_back(value(depth + 1)); } while (take(','));
                require(']');
            }
            return {std::move(array)};
        }
        case '"': return {string()};
        case 't': literal("true"); return {true};
        case 'f': literal("false"); return {false};
        case 'n': literal("null"); return {};
        default: return {number()};
        }
    }
};

std::string jsonString(std::string_view value) {
    std::string result = "\"";
    for (unsigned char ch : value) {
        if (ch < controlLimit) result += "\\u00" + hex(&ch, 1);
        else {
            if (ch == '"' || ch == '\\') result += '\\';
            result += static_cast<char>(ch);
        }
    }
    return result + "\"";
}

Json readJson(Response& response) {
    if (response.status != 200)
        throw Error("The server returned HTTP " + std::to_string(response.status) + ".");
    return JsonParser(response.text()).parse();
}

std::string trackSuffix(const Json& value) {
    auto suffix = value.at("suffix").string();
    if (suffix.empty() || suffix.size() > 12 ||
        suffix.find_first_not_of("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz") !=
        std::string::npos) return "mp3";
    return suffix;
}

void appendTracks(std::vector<Track>& result, Id server, const Json& items) {
    for (const auto& item : items.array()) {
        auto id = item.at("id").string();
        if (id.empty() || id.size() > 512 || id.find('\0') != std::string::npos) continue;
        Track track;
        track.server = server;
        track.id = std::move(id);
        track.title = item.at("title").string();
        track.artist = item.at("artist").string();
        track.album = item.at("album").string();
        track.suffix = trackSuffix(item);
        track.mime = item.at("contentType").string();
        track.duration = item.at("duration").seconds();
        result.push_back(std::move(track));
    }
}

class Subsonic : public ServerApi {
public:
    explicit Subsonic(Server server) : server_(std::move(server)) {}
    std::vector<Track> songs(const std::string& query, int offset) override {
        auto response = api("search3", {{"query", query}, {"artistCount", "0"}, {"albumCount", "0"},
            {"songCount", std::to_string(pageSize)}, {"songOffset", std::to_string(offset)}});
        std::vector<Track> result;
        appendTracks(result, server_.id, response.at("searchResult3").at("song"));
        return result;
    }

    std::vector<Track> favorites(const std::atomic_bool& stopping) override {
        auto response = api("getStarred2");
        const auto& starred = response.at("starred2");
        if (!std::holds_alternative<Json::Object>(starred.value))
            throw Error("The server returned an unexpected favorites response.");
        std::vector<Track> result;
        appendTracks(result, server_.id, starred.at("song"));
        std::set<std::string> albums;
        for (const auto& entry : starred.at("album").array()) albums.insert(entry.at("id").string());
        for (const auto& entry : starred.at("artist").array()) {
            if (stopping) throw Error("Favorite sync interrupted.");
            const auto artist = api("getArtist", {{"id", entry.at("id").string()}});
            for (const auto& album : artist.at("artist").at("album").array())
                albums.insert(album.at("id").string());
        }
        for (const auto& id : albums) {
            if (stopping) throw Error("Favorite sync interrupted.");
            if (id.empty()) continue;
            auto album = api("getAlbum", {{"id", id}});
            appendTracks(result, server_.id, album.at("album").at("song"));
        }
        return result;
    }

    std::unique_ptr<Response> audio(const Track& track, const std::string& range,
                                    bool head) override {
        auto headers = range.empty() ? L"" : L"Range: " + wide(range) + L"\r\n";
        return std::make_unique<Response>(url("stream", {{"id", track.id}, {"format", "raw"}}),
                                          head ? L"HEAD" : L"GET", headers);
    }

    void submit(const std::vector<Report>& reports) override {
        Parameters parameters{{"submission", "true"}};
        for (const auto& report : reports) {
            parameters.emplace_back("id", report.id);
            parameters.emplace_back("time", std::to_string(report.time));
        }
        api("scrobble", std::move(parameters));
    }

protected:
    Server server_;

private:
    std::string url(const std::string& endpoint, Parameters parameters) {
        auto salt = randomHex();
        parameters.insert(parameters.end(), {{"u", server_.user}, {"s", salt},
            {"t", md5(server_.password + salt)}, {"v", "1.16.1"},
            {"c", "ml_tangerinestream"}, {"f", "json"}});
        return server_.url + "/rest/" + endpoint + ".view" + queryString(parameters);
    }
    Json api(const std::string& endpoint, Parameters parameters = {}) {
        Response response(url(endpoint, std::move(parameters)), L"GET", L"");
        auto envelope = readJson(response);
        const auto& result = envelope.at("subsonic-response");
        if (result.at("status").string() != "ok") {
            auto reason = result.at("error").at("message").string();
            throw Error(reason.empty() ? "The Subsonic request failed." : reason.substr(0, 300));
        }
        return result;
    }
};

class Navidrome final : public Subsonic {
public:
    using Subsonic::Subsonic;
    std::vector<Track> songs(const std::string& query, int offset) override {
        Parameters parameters{{"_start", std::to_string(offset)}, {"_end", std::to_string(offset + pageSize)},
            {"_sort", "title"}, {"_order", "ASC"}};
        if (!query.empty()) parameters.emplace_back("title", query);
        auto url = server_.url + "/api/song" + queryString(parameters);
        auto response = authorizedGet(url);
        if (response->status == 401) {
            token_.clear();
            response = authorizedGet(url);
        }
        auto renewed = response->header(L"x-nd-authorization");
        if (validToken(renewed)) token_ = std::move(renewed);
        auto items = readJson(*response);
        if (!std::holds_alternative<Json::Array>(items.value))
            throw Error("Navidrome returned an unexpected song list.");
        std::vector<Track> result;
        appendTracks(result, server_.id, items);
        return result;
    }

private:
    std::string token_;

    static bool validToken(const std::string& token) {
        return !token.empty() && token.find_first_not_of(
            "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_.") == std::string::npos;
    }
    std::unique_ptr<Response> authorizedGet(const std::string& url) {
        if (token_.empty()) token_ = login();
        return std::make_unique<Response>(url, L"GET",
            L"x-nd-authorization: Bearer " + wide(token_) + L"\r\n");
    }
    std::string login() {
        auto body = "{\"username\":" + jsonString(server_.user) +
                    ",\"password\":" + jsonString(server_.password) + "}";
        Response response(server_.url + "/auth/login", L"POST",
                          L"Content-Type: application/json\r\n", body);
        auto token = readJson(response).at("token").string();
        if (!validToken(token))
            throw Error("Navidrome login returned an invalid token.");
        return token;
    }
};

} // namespace

std::wstring wide(std::string_view value) {
    if (value.empty()) return {};
    int length = MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0);
    std::wstring result(length, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), result.data(), length);
    return result;
}

std::string utf8(std::wstring_view value) {
    if (value.empty()) return {};
    int length = WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()),
                                     nullptr, 0, nullptr, nullptr);
    std::string result(length, '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()),
                       result.data(), length, nullptr, nullptr);
    return result;
}

std::string randomHex() {
    unsigned char data[16];
    if (BCryptGenRandom(nullptr, data, sizeof(data), BCRYPT_USE_SYSTEM_PREFERRED_RNG))
        throw Error("Windows could not generate random data.");
    return hex(data, sizeof(data));
}

std::int64_t nowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

std::string normalizeUrl(std::string url) {
    if (url.find_first_of("?#\\ \t\r\n") != std::string::npos ||
        url.find('\0') != std::string::npos)
        throw Error("Use the server base URL, including any installation subpath.");
    splitUrl(wide(url));
    while (!url.empty() && url.back() == '/') url.pop_back();
    return url;
}

Response::Response(const std::string& url, const wchar_t* method,
                   const std::wstring& headers, const std::string& body) {
    auto address = splitUrl(wide(url));
    session_.reset(WinHttpOpen(L"ml_tangerinestream/0.1", WINHTTP_ACCESS_TYPE_NO_PROXY,
                               WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0));
    if (!session_) networkError();
    if (!WinHttpSetTimeouts(session_.get(), 3000, 4000, 4000, 5000)) networkError();
    connection_.reset(WinHttpConnect(session_.get(), address.host.c_str(), address.port, 0));
    if (!connection_) networkError();
    request_.reset(WinHttpOpenRequest(connection_.get(), method, address.target.c_str(),
        nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_ESCAPE_DISABLE | (address.secure ? WINHTTP_FLAG_SECURE : 0)));
    if (!request_) networkError();
    DWORD disable = WINHTTP_DISABLE_REDIRECTS | WINHTTP_DISABLE_COOKIES | WINHTTP_DISABLE_AUTHENTICATION;
    if (!WinHttpSetOption(request_.get(), WINHTTP_OPTION_DISABLE_FEATURE, &disable, sizeof(disable)))
        networkError();
    auto length = static_cast<DWORD>(body.size());
    if (!WinHttpSendRequest(request_.get(), headers.c_str(), static_cast<DWORD>(headers.size()),
        length ? const_cast<char*>(body.data()) : nullptr, length, length, 0) ||
        !WinHttpReceiveResponse(request_.get(), nullptr)) networkError();
    DWORD size = sizeof(status);
    if (!WinHttpQueryHeaders(request_.get(), WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                            nullptr, &status, &size, nullptr)) networkError();
}

Response::~Response() = default;

namespace {
std::string readHeader(HINTERNET request, DWORD id, const wchar_t* name) {
    DWORD size = 0;
    WinHttpQueryHeaders(request, id, name, nullptr, &size, nullptr);
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER || size > 32768) return {};
    std::wstring result(size / sizeof(wchar_t), L'\0');
    if (!WinHttpQueryHeaders(request, id, name, result.data(), &size, nullptr)) return {};
    result.resize(size / sizeof(wchar_t));
    if (result.find_first_of(L"\r\n") != std::wstring::npos) return {};
    return utf8(result);
}
} // namespace

std::string Response::header(DWORD id) const { return readHeader(request_.get(), id, nullptr); }
std::string Response::header(const wchar_t* name) const {
    return readHeader(request_.get(), WINHTTP_QUERY_CUSTOM, name);
}

DWORD Response::read(void* buffer, DWORD size) {
    DWORD count;
    if (!WinHttpReadData(request_.get(), buffer, size, &count)) networkError();
    return count;
}

std::string Response::text() {
    std::string result;
    char buffer[8192];
    while (auto count = read(buffer, sizeof(buffer))) {
        if (result.size() + count > 32 * MiB) throw Error("The server response is too large.");
        result.append(buffer, count);
    }
    return result;
}

std::unique_ptr<ServerApi> connect(const Server& server) {
    if (server.native) return std::make_unique<Navidrome>(server);
    return std::make_unique<Subsonic>(server);
}

} // namespace tangerine
