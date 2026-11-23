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
    throw ConnectionError("Server connection failed (Windows error " + std::to_string(GetLastError()) + ").");
}

class SubsonicError : public Error {
public:
    SubsonicError(int code, const std::string& message) : Error(message), code(code) {}
    int code;
};

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

std::string windowsLines(std::string_view text) {
    std::string result;
    result.reserve(text.size());
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '\n' && (i == 0 || text[i - 1] != '\r')) result += '\r';
        result += text[i];
    }
    return result;
}

Json readJson(Response& response) {
    if (response.status == 408 || (response.status >= 500 && response.status <= 599))
        throw ConnectionError("The server returned HTTP " + std::to_string(response.status) + ".");
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

bool usableId(const std::string& id) {
    return !id.empty() && id.size() <= 512 && id.front() != '\x1f' && id.find('\0') == std::string::npos;
}

SortTags sortTags(const Json& node) {
    SortTags tags;
    auto value = [](const Json& v) -> std::string {
        if (const auto* number = std::get_if<double>(&v.value)) {
            if (*number >= 0 && *number <= INT_MAX) return std::to_string(static_cast<int>(*number));
        }
        if (const auto* flag = std::get_if<bool>(&v.value)) return *flag ? "1" : "0";
        return v.string();
    };
    auto field = [&](const char* key, std::initializer_list<const char*> aliases) {
        for (auto alias : aliases) {
            auto text = value(node.at(alias));
            if (!text.empty()) { tags.emplace(key, std::move(text)); return; }
        }
    };
    field("created", {"importedAt", "created", "createdAt"});
    field("played", {"played", "playDate"});
    field("starred", {"starredAt", "starred"});
    if (tags.count("starred") && tags.at("starred") == "0") tags.erase("starred");
    field("rating", {"userRating", "rating"});
    field("playCount", {"playCount"});
    field("albumCount", {"albumCount"});
    field("channels", {"channelCount", "channels"});
    field("bpm", {"bpm"});
    field("comment", {"comment"});
    field("owner", {"owner", "ownerName"});
    field("public", {"public"});
    field("updated", {"changed", "updatedAt"});
    field("sortName", {"sortName", "sortTitle", "orderTitle"});
    field("album", {"album"});
    field("artist", {"artist"});
    field("albumArtist", {"albumArtist"});
    field("genre", {"genre"});
    field("year", {"year", "maxYear"});
    field("releaseYear", {"releaseYear"});
    if (!tags.count("releaseYear")) {
        auto year = value(node.at("releaseDate").at("year"));
        if (!year.empty()) tags.emplace("releaseYear", year);
    }
    for (const auto& credit : {std::pair{"artist", "artists"}, std::pair{"albumArtist", "albumArtists"}}) {
        std::string names;
        for (const auto& artist : node.at(credit.second).array()) {
            const auto name = artist.at("name").string();
            if (name.empty()) continue;
            if (!names.empty()) names += ", ";
            names += name;
        }
        if (!names.empty()) tags.insert_or_assign(credit.first, std::move(names));
    }
    return tags;
}

void appendTrack(std::vector<Track>& result, Id server, const Json& item) {
    auto id = item.at("id").string();
    if (!usableId(id)) return;
    Track track;
    track.server = server;
    track.id = std::move(id);
    track.title = item.at("title").string();
    track.artist = item.at("artist").string();
    track.album = item.at("album").string();
    track.suffix = trackSuffix(item);
    track.mime = item.at("contentType").string();
    track.duration = item.at("duration").seconds();
    track.coverId = item.at("coverArt").string();
    track.albumId = item.at("albumId").string();
    track.artistId = item.at("artistId").string();
    track.albumArtist = item.at("albumArtist").string();
    track.albumArtistId = item.at("albumArtistId").string();
    const auto& albumArtists = item.at("albumArtists").array();
    if (!albumArtists.empty()) {
        if (track.albumArtist.empty()) track.albumArtist = albumArtists.front().at("name").string();
        if (track.albumArtistId.empty() && albumArtists.size() == 1)
            track.albumArtistId = albumArtists.front().at("id").string();
    }
    track.genre = item.at("genre").string();
    track.discNumber = item.at("discNumber").seconds();
    track.trackNumber = item.at("track").seconds();
    if (!track.trackNumber) track.trackNumber = item.at("trackNumber").seconds();
    track.year = item.at("year").seconds();
    track.sortTags = sortTags(item);
    result.push_back(std::move(track));
}

void appendTracks(std::vector<Track>& result, Id server, const Json& items) {
    for (const auto& item : items.array()) appendTrack(result, server, item);
}

std::string itemCount(int count, const char* singular, const char* plural) {
    return std::to_string(count) + " " + (count == 1 ? singular : plural);
}

void appendItems(std::vector<LibraryItem>& result, const Json& items, BrowseKind kind,
                 const std::wstring& query = {}) {
    for (const auto& node : items.array()) {
        auto name = node.at("name").string();
        if (name.empty()) name = node.at("title").string();
        if (kind == BrowseKind::genreAlbums) name = node.at("value").string();
        auto id = kind == BrowseKind::genreAlbums ? name : node.at("id").string();
        if (!usableId(id)) continue;
        if (name.empty()) name = id;
        auto artist = node.at("artist").string();
        if (!matches(name, query) && !matches(artist, query)) continue;
        std::string subtitle = std::move(artist);
        switch (kind) {
        case BrowseKind::genreAlbums:
        case BrowseKind::artistAlbums:
            subtitle = itemCount(node.at("albumCount").seconds(), "album", "albums");
            break;
        case BrowseKind::playlistTracks:
            subtitle = itemCount(node.at("songCount").seconds(), "track", "tracks");
            break;
        default: break;
        }
        const auto& count = node.at("songCount");
        result.push_back({{kind, std::move(id), std::move(name)}, std::move(subtitle),
                          node.at("coverArt").string(), kind == BrowseKind::song ? 1 :
                          (std::holds_alternative<double>(count.value) ? count.seconds() : -1),
                          node.at("duration").seconds(), sortTags(node)});
    }
}

class Subsonic : public ServerApi {
public:
    explicit Subsonic(Server server) : server_(std::move(server)) {}
    void ping() override { api("ping"); }

    BrowsePage browse(const BrowseLocation& location, const std::string& query, int offset) override {
        BrowsePage result;
        auto filter = wide(query);
        switch (location.kind) {
        case BrowseKind::albums:
        case BrowseKind::genreAlbums:
            return albums(location, query, offset);
        case BrowseKind::genres: {
            auto response = api("getGenres");
            appendItems(result.items, response.at("genres").at("genre"), BrowseKind::genreAlbums, filter);
            break;
        }
        case BrowseKind::albumArtists: {
            auto response = api("getArtists");
            for (const auto& index : response.at("artists").at("index").array())
                appendItems(result.items, index.at("artist"), BrowseKind::artistAlbums, filter);
            break;
        }
        case BrowseKind::artists: {
            auto response = api("search3", {{"query", query}, {"albumCount", "0"}, {"songCount", "0"},
                {"artistCount", std::to_string(pageSize)}, {"artistOffset", std::to_string(offset)}});
            const auto& entries = response.at("searchResult3").at("artist");
            result.more = entries.array().size() == pageSize;
            appendItems(result.items, entries, BrowseKind::artistAlbums);
            break;
        }
        case BrowseKind::artistAlbums: {
            auto response = api("getArtist", {{"id", location.id}});
            appendItems(result.items, response.at("artist").at("album"), BrowseKind::albumTracks, filter);
            break;
        }
        case BrowseKind::folders: {
            auto response = api("getMusicFolders");
            const auto& folders = response.at("musicFolders").at("musicFolder");
            for (const auto& folder : folders.array()) {
                // Some servers encode the library ID as a JSON number.
                auto number = std::get_if<double>(&folder.at("id").value);
                auto id = number && *number >= 0 && *number <= INT_MAX
                    ? std::to_string(static_cast<int>(*number)) : folder.at("id").string();
                auto name = folder.at("name").string();
                if (usableId(id) && matches(name, filter))
                    result.items.push_back({{BrowseKind::folderIndex, std::move(id), std::move(name)}, {}, {}});
            }
            break;
        }
        case BrowseKind::folderIndex: {
            auto response = api("getIndexes", {{"musicFolderId", location.id}});
            const auto& indexes = response.at("indexes");
            for (const auto& index : indexes.at("index").array())
                appendItems(result.items, index.at("artist"), BrowseKind::directory, filter);
            appendDirectory(result.items, indexes.at("child"), filter);
            appendDirectoryTracks(result.relatedTracks, indexes.at("child"));
            break;
        }
        case BrowseKind::directory: {
            auto response = api("getMusicDirectory", {{"id", location.id}});
            appendDirectory(result.items, response.at("directory").at("child"), filter);
            appendDirectoryTracks(result.relatedTracks, response.at("directory").at("child"));
            break;
        }
        case BrowseKind::playlists: {
            auto response = api("getPlaylists");
            appendItems(result.items, response.at("playlists").at("playlist"), BrowseKind::playlistTracks, filter);
            break;
        }
        case BrowseKind::favorites: {
            auto response = api("getStarred2");
            const auto& starred = response.at("starred2");
            appendStarred(result.items, starred, filter);
            appendTracks(result.relatedTracks, server_.id, starred.at("song"));
            break;
        }
        case BrowseKind::tracks:
            result.tracks = songs(query, offset);
            result.more = result.tracks.size() == pageSize;
            return result;
        case BrowseKind::albumTracks: {
            auto response = api("getAlbum", {{"id", location.id}});
            appendTracks(result.tracks, server_.id, response.at("album").at("song"));
            for (auto& track : result.tracks)
                if (track.albumId.empty()) track.albumId = location.id;
            break;
        }
        case BrowseKind::playlistTracks: {
            auto response = api("getPlaylist", {{"id", location.id}});
            appendTracks(result.tracks, server_.id, response.at("playlist").at("entry"));
            break;
        }
        case BrowseKind::song: {
            auto response = api("getSong", {{"id", location.id}});
            appendTrack(result.tracks, server_.id, response.at("song"));
            break;
        }
        case BrowseKind::offline:
            break;
        }
        if (!filter.empty()) {
            result.tracks.erase(std::remove_if(result.tracks.begin(), result.tracks.end(),
                [&](const Track& track) {
                    return !matches(track.title, filter) && !matches(track.artist, filter) &&
                        !matches(track.album, filter);
                }), result.tracks.end());
        }
        return result;
    }

    std::string cover(const LibraryItem& item, int size) override {
        auto id = item.coverId;
        if (!usableId(id) || item.target.kind == BrowseKind::artistAlbums) {
            auto representative = representativeCover(item.target);
            if (!representative.empty()) id = std::move(representative);
        }
        if (!usableId(id)) return {};
        Response response(url("getCoverArt", {{"id", id}, {"size", std::to_string(size)}}), L"GET", L"");
        if (response.status != 200) return {};
        auto mime = response.header(WINHTTP_QUERY_CONTENT_TYPE);
        if (!mime.empty() && mime.compare(0, 6, "image/") != 0 &&
            mime.compare(0, 24, "application/octet-stream") != 0) return {};
        return response.text(8 * MiB);
    }

    Lyrics lyrics(const Track& track) override {
        Parameters params{{"id", track.id}};
        Lyrics result;
        auto structured = [&](bool enhanced) {
            auto parameters = params;
            if (enhanced) parameters.emplace_back("enhanced", "true");
            try {
                return api("getLyricsBySongId", std::move(parameters), &result.structured);
            } catch (const SubsonicError& error) {
                if (error.code != 0 && error.code != 10 && error.code != 70) throw;
                return Json{};
            }
        };
        auto response = structured(true);
        if (result.structured.empty()) response = structured(false);
        const auto& variants = response.at("lyricsList").at("structuredLyrics").array();
        for (const auto& variant : variants) {
            auto kind = variant.at("kind").string();
            if (!kind.empty() && kind != "main") continue;
            const auto& lines = variant.at("line").array();
            for (std::size_t i = 0; i < lines.size(); ++i) {
                if (i) result.plain += "\r\n";
                result.plain += lines[i].at("value").string();
            }
            if (result.plain.find_first_not_of("\r\n\t ") != std::string::npos) {
                result.plain = windowsLines(result.plain);
                return result;
            }
            result.plain.clear();
        }
        if (!track.artist.empty() && !track.title.empty()) {
            try {
                result.plain = windowsLines(api("getLyrics", {{"artist", track.artist}, {"title", track.title}})
                    .at("lyrics").at("value").string());
            } catch (const SubsonicError& error) {
                if (error.code != 0 && error.code != 10 && error.code != 70) throw;
            }
        }
        return result;
    }

    virtual std::vector<Track> songs(const std::string& query, int offset) {
        auto response = api("search3", {{"query", query}, {"artistCount", "0"}, {"albumCount", "0"},
            {"songCount", std::to_string(pageSize)}, {"songOffset", std::to_string(offset)}});
        std::vector<Track> result;
        appendTracks(result, server_.id, response.at("searchResult3").at("song"));
        return result;
    }

    BrowsePage favorites(const std::atomic_bool& stopping) override {
        auto response = api("getStarred2");
        const auto& starred = response.at("starred2");
        if (!std::holds_alternative<Json::Object>(starred.value))
            throw Error("The server returned an unexpected favorites response.");
        BrowsePage result;
        appendStarred(result.items, starred, {});
        appendTracks(result.tracks, server_.id, starred.at("song"));
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
            auto first = result.tracks.size();
            appendTracks(result.tracks, server_.id, album.at("album").at("song"));
            for (; first < result.tracks.size(); ++first)
                if (result.tracks[first].albumId.empty()) result.tracks[first].albumId = id;
        }
        return result;
    }

    std::string streamUrl(const Track& track) override {
        return url("stream", {{"id", track.id}, {"format", "raw"}});
    }

    std::unique_ptr<Response> audio(const Track& track, const std::string& range,
                                    bool head) override {
        auto headers = range.empty() ? L"" : L"Range: " + wide(range) + L"\r\n";
        return std::make_unique<Response>(streamUrl(track),
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
    static void appendStarred(std::vector<LibraryItem>& items, const Json& starred,
                              const std::wstring& query) {
        appendItems(items, starred.at("album"), BrowseKind::albumTracks, query);
        appendItems(items, starred.at("artist"), BrowseKind::artistAlbums, query);
        appendItems(items, starred.at("song"), BrowseKind::song, query);
    }

    void appendDirectoryTracks(std::vector<Track>& result, const Json& items) {
        for (const auto& node : items.array()) {
            auto directory = std::get_if<bool>(&node.at("isDir").value);
            if (!directory || !*directory) appendTrack(result, server_.id, node);
        }
    }

    static void appendDirectory(std::vector<LibraryItem>& result, const Json& items,
                                const std::wstring& query) {
        for (const auto& node : items.array()) {
            auto id = node.at("id").string();
            auto title = node.at("title").string();
            auto artist = node.at("artist").string();
            if (!usableId(id) || (!matches(title, query) && !matches(artist, query))) continue;
            if (title.empty()) title = id;
            auto isDirectory = std::get_if<bool>(&node.at("isDir").value);
            auto kind = isDirectory && *isDirectory ? BrowseKind::directory : BrowseKind::song;
            result.push_back({{kind, std::move(id), std::move(title)}, std::move(artist),
                              node.at("coverArt").string(), kind == BrowseKind::song ? 1 : -1,
                              node.at("duration").seconds(), sortTags(node)});
        }
    }

    BrowsePage albums(const BrowseLocation& location, const std::string& query, int offset) {
        BrowsePage result;
        if (location.kind == BrowseKind::albums && !query.empty()) {
            auto response = api("search3", {{"query", query}, {"artistCount", "0"}, {"songCount", "0"},
                {"albumCount", std::to_string(pageSize)}, {"albumOffset", std::to_string(offset)}});
            const auto& entries = response.at("searchResult3").at("album");
            result.more = entries.array().size() == pageSize;
            appendItems(result.items, entries, BrowseKind::albumTracks);
        } else {
            Parameters params{{"type", "alphabeticalByName"}, {"size", std::to_string(pageSize)},
                              {"offset", std::to_string(offset)}};
            if (location.kind == BrowseKind::genreAlbums) {
                params.front().second = "byGenre";
                params.emplace_back("genre", location.id);
            }
            auto response = api("getAlbumList2", std::move(params));
            const auto& entries = response.at("albumList2").at("album");
            result.more = entries.array().size() == pageSize;
            appendItems(result.items, entries, BrowseKind::albumTracks, wide(query));
        }
        return result;
    }

    std::string representativeCover(const BrowseLocation& location) {
        Json response;
        const Json* entries = nullptr;
        switch (location.kind) {
        case BrowseKind::genreAlbums:
            response = api("getAlbumList2", {{"type", "byGenre"}, {"genre", location.id}, {"size", "5"}});
            entries = &response.at("albumList2").at("album");
            break;
        case BrowseKind::artistAlbums:
            response = api("getArtist", {{"id", location.id}});
            entries = &response.at("artist").at("album");
            break;
        case BrowseKind::playlistTracks:
            response = api("getPlaylist", {{"id", location.id}});
            entries = &response.at("playlist").at("entry");
            break;
        case BrowseKind::directory:
            response = api("getMusicDirectory", {{"id", location.id}});
            entries = &response.at("directory").at("child");
            break;
        case BrowseKind::song:
            return api("getSong", {{"id", location.id}}).at("song").at("coverArt").string();
        case BrowseKind::albumTracks:
            if (!location.id.empty())
                return api("getAlbum", {{"id", location.id}}).at("album").at("coverArt").string();
            return {};
        default:
            return {};
        }
        for (const auto& entry : entries->array()) {
            auto cover = entry.at("coverArt").string();
            if (usableId(cover)) return cover;
        }
        return {};
    }

    std::string url(const std::string& endpoint, Parameters parameters) {
        auto salt = randomHex();
        parameters.insert(parameters.end(), {{"u", server_.user}, {"s", salt},
            {"t", md5(server_.password + salt)}, {"v", "1.16.1"},
            {"c", "ml_tangerinestream"}, {"f", "json"}});
        return server_.url + "/rest/" + endpoint + ".view" + queryString(parameters);
    }
    Json api(const std::string& endpoint, Parameters parameters = {}, std::string* original = nullptr) {
        Response response(url(endpoint, std::move(parameters)), L"GET", L"");
        if (response.status == 404 || response.status == 501)
            throw SubsonicError(70, "The server does not provide this resource.");
        std::string payload;
        if (original && response.status == 200) payload = response.text(2 * MiB);
        auto envelope = payload.empty() ? readJson(response) : JsonParser(payload).parse();
        const auto& result = envelope.at("subsonic-response");
        if (result.at("status").string() != "ok") {
            const auto& error = result.at("error");
            auto reason = error.at("message").string();
            throw SubsonicError(error.at("code").seconds(),
                reason.empty() ? "The Subsonic request failed." : reason.substr(0, 300));
        }
        if (original) *original = std::move(payload);
        return result;
    }
};

class Navidrome final : public Subsonic {
public:
    using Subsonic::Subsonic;

    BrowsePage browse(const BrowseLocation& location, const std::string& query, int offset) override {
        const char* endpoint = nullptr;
        const char* artPrefix = nullptr;
        BrowseKind target;
        Parameters filters;
        switch (location.kind) {
        case BrowseKind::albums:
            endpoint = "album"; target = BrowseKind::albumTracks; artPrefix = "al-"; break;
        case BrowseKind::albumArtists:
        case BrowseKind::artists:
            endpoint = "artist"; target = BrowseKind::artistAlbums; artPrefix = "ar-";
            if (location.kind == BrowseKind::albumArtists) filters.emplace_back("role", "albumartist");
            break;
        case BrowseKind::playlists:
            endpoint = "playlist"; target = BrowseKind::playlistTracks; artPrefix = "pl-"; break;
        default:
            return Subsonic::browse(location, query, offset);
        }
        if (!query.empty()) filters.emplace_back(location.kind == BrowseKind::playlists ? "q" : "name", query);
        auto nodes = list(endpoint, offset, "name", std::move(filters));
        BrowsePage page;
        page.more = nodes.array().size() == pageSize;
        appendItems(page.items, nodes, target);
        for (auto& item : page.items)
            if (item.coverId.empty()) item.coverId = artPrefix + item.target.id;
        return page;
    }

    std::vector<Track> songs(const std::string& query, int offset) override {
        Parameters parameters;
        if (!query.empty()) parameters.emplace_back("title", query);
        auto items = list("song", offset, "title", std::move(parameters));
        std::vector<Track> result;
        appendTracks(result, server_.id, items);
        return result;
    }

private:
    std::string token_;

    Json list(const char* endpoint, int offset, const char* order, Parameters parameters) {
        parameters.insert(parameters.end(), {{"_start", std::to_string(offset)},
            {"_end", std::to_string(offset + pageSize)}, {"_sort", order}, {"_order", "ASC"}});
        auto url = server_.url + "/api/" + endpoint + queryString(parameters);
        auto response = authorizedGet(url);
        if (response->status == 401) {
            token_.clear();
            response = authorizedGet(url);
        }
        auto renewed = response->header(L"x-nd-authorization");
        if (validToken(renewed)) token_ = std::move(renewed);
        auto items = readJson(*response);
        if (!std::holds_alternative<Json::Array>(items.value))
            throw Error("Navidrome returned an unexpected library response.");
        return items;
    }

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

std::string encodeSortTags(const SortTags& tags) {
    if (tags.empty()) return {};
    std::string text = "{";
    for (const auto& [key, value] : tags) {
        if (text.size() > 1) text += ',';
        text += jsonString(key) + ":" + jsonString(value);
    }
    return text + "}";
}

SortTags decodeSortTags(const std::string& text) {
    if (text.empty()) return {};
    auto parsed = JsonParser(text).parse();
    SortTags tags;
    if (auto object = std::get_if<Json::Object>(&parsed.value))
        for (const auto& [key, value] : *object) tags.emplace(key, value.string());
    return tags;
}

bool matches(const std::string& value, const std::wstring& query) {
    if (query.empty()) return true;
    auto text = wide(value);
    return !text.empty() && FindNLSStringEx(LOCALE_NAME_INVARIANT,
        FIND_FROMSTART | NORM_IGNORECASE, text.data(), static_cast<int>(text.size()),
        query.data(), static_cast<int>(query.size()), nullptr, nullptr, nullptr, 0) >= 0;
}

std::string playlistTitle(const Track& track) {
    auto title = track.title.empty() ? track.id : track.title;
    return track.artist.empty() ? title : track.artist + " - " + title;
}

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

std::string Response::text(std::size_t limit) {
    std::string result;
    char buffer[8192];
    while (auto count = read(buffer, sizeof(buffer))) {
        if (result.size() + count > limit) throw Error("The server response is too large.");
        result.append(buffer, count);
    }
    return result;
}

std::unique_ptr<ServerApi> connect(const Server& server) {
    if (server.native) return std::make_unique<Navidrome>(server);
    return std::make_unique<Subsonic>(server);
}

} // namespace tangerine
