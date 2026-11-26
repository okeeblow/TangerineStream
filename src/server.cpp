#include "server.hpp"
#include "tangerine_product.h"

#include <bcrypt.h>
#include <algorithm>
#include <simdjson.h>
#include <uriparser/Uri.h>
#include <cctype>
#include <chrono>
#include <climits>
#include <cstdlib>
#include <map>
#include <mutex>
#include <set>
#include <optional>

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
    BCRYPT_HASH_HANDLE hash = nullptr;
    auto status = BCryptCreateHash(algorithm, &hash, nullptr, 0, nullptr, 0, 0);
    if (BCRYPT_SUCCESS(status)) {
        status = BCryptHashData(hash, reinterpret_cast<PUCHAR>(const_cast<char*>(value.data())),
                                static_cast<ULONG>(value.size()), 0);
        if (BCRYPT_SUCCESS(status)) status = BCryptFinishHash(hash, digest, sizeof(digest), 0);
        BCryptDestroyHash(hash);
    }
    BCryptCloseAlgorithmProvider(algorithm, 0);
    if (!BCRYPT_SUCCESS(status)) throw Error("Windows could not create a Subsonic token.");
    return hex(digest, sizeof(digest));
}

// uriparser's string APIs stop at NUL even with an explicit end pointer.
void appendEscaped(std::string& output, std::string_view bytes) {
    while (!bytes.empty()) {
        const auto part = bytes.substr(0, bytes.find('\0'));
        const auto start = output.size();
        if (part.size() > (output.max_size() - start - 1) / 3) throw Error("URL is too long.");
        output.resize(start + 3 * part.size() + 1);
        auto end = uriEscapeExA(part.data(), part.data() + part.size(), output.data() + start,
                                URI_FALSE, URI_FALSE);
        output.resize(static_cast<std::size_t>(end - output.data()));
        if (part.size() == bytes.size()) break;
        output += "%00";
        bytes.remove_prefix(part.size() + 1);
    }
}

std::string queryString(const Parameters& parameters) {
    if (parameters.empty()) return {};
    std::vector<UriQueryListA> list(parameters.size());
    for (std::size_t i = 0; i < list.size(); ++i) {
        const auto& [key, value] = parameters[i];
        if (key.find('\0') != key.npos || value.find('\0') != value.npos) {
            std::string query;
            for (const auto& [k, v] : parameters) {
                query += query.empty() ? '?' : '&';
                appendEscaped(query, k);
                query += '=';
                appendEscaped(query, v);
            }
            return query;
        }
        list[i] = {key.c_str(), value.c_str(), i + 1 < list.size() ? &list[i + 1] : nullptr};
    }
    char* raw = nullptr;
    if (uriComposeQueryMallocExA(&raw, list.data(), URI_FALSE, URI_FALSE) != URI_SUCCESS)
        throw Error("The request URL could not be encoded.");
    std::unique_ptr<char, decltype(&std::free)> query(raw, &std::free);
    return std::string("?") + query.get();
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

// Missing optional fields behave like empty collections or strings in both APIs.
struct JsonArray {
    simdjson::simdjson_result<simdjson::dom::array> value;
    using Iterator = simdjson::dom::array::iterator;

    Iterator begin() const { return value.error() ? Iterator{} : value.value_unsafe().begin(); }
    Iterator end() const { return value.error() ? Iterator{} : value.value_unsafe().end(); }
    std::size_t size() const { return value.error() ? 0 : value.value_unsafe().size(); }
};

struct Json {
    simdjson::simdjson_result<simdjson::dom::element> value{simdjson::NO_SUCH_FIELD};

    Json() = default;
    Json(simdjson::dom::element element) : value(std::move(element)) {}

    Json at(std::string_view key) const {
        auto object = value.get_object();
        Json found;
        if (!object.error()) {
            // Keep the last value for a repeated key, as in cached metadata.
            for (auto field : object.value_unsafe())
                if (field.key == key) found = field.value;
        }
        return found;
    }
    JsonArray array() const { return {value.get_array()}; }
    std::string string() const {
        auto text = value.get_string();
        return text.error() ? std::string{} : std::string(text.value_unsafe());
    }
    std::optional<double> number() const {
        auto result = value.get_double();
        if (result.error()) return {};
        return result.value_unsafe();
    }
    std::optional<bool> boolean() const {
        auto result = value.get_bool();
        if (result.error()) return {};
        return result.value_unsafe();
    }
    int seconds() const {
        auto n = number();
        return n && *n > 0 && *n <= INT_MAX ? static_cast<int>(*n) : 0;
    }
};

// Elements borrow the document's tape. Keep its address stable when returning
// a document or selecting a Subsonic envelope as its root.
struct JsonDocument : Json {
    std::unique_ptr<simdjson::dom::document> storage;
};

JsonDocument parseJson(const std::string& input) {
    JsonDocument document;
    document.storage = std::make_unique<simdjson::dom::document>();
    simdjson::dom::parser parser;
    if (auto error = parser.allocate(input.size(), 64))
        throw Error(simdjson::error_message(error));
    document.value = parser.parse_into_document(*document.storage, input);
    if (document.value.error())
        throw Error(std::string("Invalid JSON: ") + simdjson::error_message(document.value.error()));
    return document;
}

std::string jsonString(std::string_view value) {
    simdjson::builder::string_builder builder;
    builder.escape_and_append_with_quotes(value);
    auto text = builder.view();
    if (text.error()) throw Error(simdjson::error_message(text.error()));
    return std::string(text.value_unsafe());
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

JsonDocument readJson(Response& response) {
    if (response.status == 408 || (response.status >= 500 && response.status <= 599))
        throw ConnectionError("The server returned HTTP " + std::to_string(response.status) + ".");
    if (response.status != 200)
        throw Error("The server returned HTTP " + std::to_string(response.status) + ".");
    return parseJson(response.text());
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
        if (const auto number = v.number()) {
            if (*number >= 0 && *number <= INT_MAX) return std::to_string(static_cast<int>(*number));
        }
        if (const auto flag = v.boolean()) return *flag ? "1" : "0";
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
        for (Json artist : node.at(credit.second).array()) {
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
    if (albumArtists.size()) {
        const Json first = *albumArtists.begin();
        if (track.albumArtist.empty()) track.albumArtist = first.at("name").string();
        if (track.albumArtistId.empty() && albumArtists.size() == 1)
            track.albumArtistId = first.at("id").string();
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
    for (Json item : items.array()) appendTrack(result, server, item);
}

std::string itemCount(int count, const char* singular, const char* plural) {
    return std::to_string(count) + " " + (count == 1 ? singular : plural);
}

void appendItems(std::vector<LibraryItem>& result, const Json& items, BrowseKind kind,
                 const std::wstring& query = {}) {
    for (Json node : items.array()) {
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
                          (count.value.is_number() ? count.seconds() : -1),
                          node.at("duration").seconds(), sortTags(node)});
    }
}

struct AudioFormat {
    const char* codec;
    const char* container;
    const char* suffix;
};

constexpr AudioFormat audioFormats[] = {
    {"mp3", "mp3", "mp3"}, {"opus", "ogg", "opus"}, {"aac", "mp4", "m4a"},
    {"aac", "aac", "aac"}, {"vorbis", "ogg", "ogg"}, {"flac", "flac", "flac"},
    {"wav", "wav", "wav"}
};

unsigned choice(const AudioFormat& format) {
    return 2u << (&format - audioFormats);
}

std::string playableSuffix(const InputExtensions& inputs, const AudioFormat& format) {
    if (inputs.count(format.suffix)) return format.suffix;
    if (std::string_view(format.container) == "mp4" && inputs.count("mp4")) return "mp4";
    return {};
}

using Targets = std::vector<const AudioFormat*>;

std::string clientCapabilities(int kbps, const Targets& targets, bool directPlay, bool allowAlac,
                               const std::string& extraContainer) {
    std::string direct, transcode;
    for (const auto* f : targets) {
        const auto codec = jsonString(f->codec), container = jsonString(f->container);
        if (directPlay) {
            if (!direct.empty()) direct += ',';
            auto containers = container;
            if (std::string_view(f->suffix) != f->container) containers += "," + jsonString(f->suffix);
            auto codecs = codec;
            if (allowAlac && std::string_view(f->container) == "mp4") codecs += ",\"alac\"";
            if (std::string_view(f->codec) == "wav") codecs += ",\"pcm\",\"pcm_s16le\",\"pcm_s24le\"";
            direct += "{\"containers\":[" + containers + "],\"audioCodecs\":[" + codecs + "],\"protocols\":[\"http\"]}";
        }
        if (!transcode.empty()) transcode += ',';
        transcode += "{\"container\":" + container + ",\"audioCodec\":" + codec + ",\"protocol\":\"http\"}";
    }
    if (!extraContainer.empty()) {
        if (!direct.empty()) direct += ',';
        direct += "{\"containers\":[" + jsonString(extraContainer) + "],\"protocols\":[\"http\"]}";
    }
    const auto bps = std::to_string(kbps * 1000);
    return "{\"name\":" + jsonString(TS_PRODUCT_NAME) + ",\"platform\":\"WACUP\",\"maxAudioBitrate\":" + bps +
        ",\"maxTranscodingAudioBitrate\":" + bps + ",\"directPlayProfiles\":[" + direct +
        "],\"transcodingProfiles\":[" + transcode + "],\"codecProfiles\":[]}";
}

bool matchesStream(const AudioFormat& f, const Json& details, bool allowAlac, bool direct) {
    const auto container = details.at("container").string();
    if (container != f.container && container != f.suffix) return false;
    const auto codec = details.at("codec").string();
    if (codec == f.codec) return true;
    if (direct && allowAlac && std::string_view(f.container) == "mp4" && codec == "alac") return true;
    return direct && container == "wav" && (codec == "pcm" || codec.find("pcm_") == 0);
}

std::string escapedDecisionToken(const std::string& token) {
    // The extension defines an opaque, already escaped query value. Escape any
    // remaining reserved characters without encoding existing %HH sequences twice.
    std::string escaped;
    std::string_view remaining(token);
    std::size_t search = 0;
    while ((search = remaining.find('%', search)) != remaining.npos) {
        if (remaining.size() - search >= 3 &&
            std::isxdigit(static_cast<unsigned char>(remaining[search + 1])) &&
            std::isxdigit(static_cast<unsigned char>(remaining[search + 2]))) {
            appendEscaped(escaped, remaining.substr(0, search));
            escaped.append(remaining.substr(search, 3));
            remaining.remove_prefix(search + 3);
            search = 0;
        } else ++search;
    }
    appendEscaped(escaped, remaining);
    return escaped;
}

class Subsonic : public ServerApi {
public:
    Subsonic(HttpClient& http, Server server) : http_(http), server_(std::move(server)) {}
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
            for (Json index : response.at("artists").at("index").array())
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
            for (Json folder : folders.array()) {
                // Some servers encode the library ID as a JSON number.
                auto number = folder.at("id").number();
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
            for (Json index : indexes.at("index").array())
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
        Response response(http_, url("getCoverArt", {{"id", id}, {"size", std::to_string(size)}}), L"GET", L"");
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
                return JsonDocument{};
            }
        };
        auto response = structured(true);
        if (result.structured.empty()) response = structured(false);
        const auto& variants = response.at("lyricsList").at("structuredLyrics").array();
        for (Json variant : variants) {
            auto kind = variant.at("kind").string();
            if (!kind.empty() && kind != "main") continue;
            const auto& lines = variant.at("line").array();
            bool firstLine = true;
            for (Json line : lines) {
                if (!firstLine) result.plain += "\r\n";
                result.plain += line.at("value").string();
                firstLine = false;
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
        if (!starred.value.is_object())
            throw Error("The server returned an unexpected favorites response.");
        BrowsePage result;
        appendStarred(result.items, starred, {});
        appendTracks(result.tracks, server_.id, starred.at("song"));
        std::set<std::string> albums;
        for (Json entry : starred.at("album").array()) albums.insert(entry.at("id").string());
        for (Json entry : starred.at("artist").array()) {
            if (stopping) throw Error("Favorite sync interrupted.");
            const auto artist = api("getArtist", {{"id", entry.at("id").string()}});
            for (Json album : artist.at("artist").at("album").array())
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

    AudioStream stream(const Track& track, const StreamQuality& quality,
                       const InputExtensions& inputs, unsigned excluded) override {
        const bool preferOriginal = quality.format == "raw";
        const bool direct = !(excluded & originalSource) && acceptsExtension(inputs, track.suffix);
        const int kbps = preferOriginal ? 0 : quality.kbps;
        if (kbps < 0 || kbps > 100000) throw Error("Invalid streaming bitrate in server settings.");
        auto original = [&] { return AudioStream{url("stream", {{"id", track.id}, {"format", "raw"}}), originalSource, {}}; };

        Targets preferred, alternatives;
        for (const auto& f : audioFormats) {
            if ((excluded & choice(f)) || playableSuffix(inputs, f).empty()) continue;
            (quality.format == f.codec ? preferred : alternatives).push_back(&f);
        }
        if (preferOriginal) {
            const auto lossless = std::find_if(alternatives.begin(), alternatives.end(), [](const AudioFormat* f) {
                return std::string_view(f->codec) == "flac";
            });
            if (lossless != alternatives.end()) std::rotate(alternatives.begin(), lossless, lossless + 1);
        }
        const bool directOnly = preferred.empty() && alternatives.empty();
        if (directOnly && !direct) throw Error("No enabled WACUP input can play the remaining stream formats.");
        if (!hasTranscoding()) {
            if (!kbps && direct && (preferOriginal || quality.format == "auto" || directOnly)) return original();
            if (directOnly) throw Error("No enabled WACUP input supports a transcoding target within the selected bitrate.");
            const auto& f = *(preferred.empty() ? alternatives.front() : preferred.front());
            const std::string format = std::string_view(f.codec) == "vorbis" ? "ogg" : f.codec;
            Parameters params{{"id", track.id}, {"format", format}};
            if (kbps) params.emplace_back("maxBitRate", std::to_string(kbps));
            return {url("stream", std::move(params)), choice(f), playableSuffix(inputs, f)};
        }

        Parameters media{{"mediaId", track.id}, {"mediaType", "song"}};
        std::string reason;
        for (const auto* targets : {&preferred, &alternatives}) {
            if (targets->empty() && !(directOnly && targets == &alternatives)) continue;
            const bool allowAlac = targets == &alternatives;
            // A specialist input (e.g. Monkey's Audio) may play originals even
            // though the server has no corresponding transcoding target.
            std::string extraContainer;
            if (direct && allowAlac && (directOnly || std::none_of(std::begin(audioFormats), std::end(audioFormats),
                [&](const AudioFormat& f) { return track.suffix == f.suffix || track.suffix == f.container; })))
                extraContainer = track.suffix;
            bool offerDirect = direct;
            for (;;) {
                JsonDocument reply;
                try {
                    reply = api("getTranscodeDecision", media, nullptr,
                                clientCapabilities(kbps, *targets, offerDirect, allowAlac, offerDirect ? extraContainer : ""));
                } catch (const SubsonicError& error) {
                    // Some servers report an unavailable encoder as a generic API error.
                    if (error.code != 0 || targets != &preferred || alternatives.empty()) throw;
                    break;
                }
                const auto& decision = reply.at("transcodeDecision");
                auto allowed = [&](const char* key) {
                    return decision.at(key).boolean().value_or(false);
                };
                auto withinRate = [&](const Json& details) {
                    return !kbps || details.at("audioBitrate").seconds() <= kbps * 1000;
                };
                const auto& source = decision.at("sourceStream");
                if (offerDirect && allowed("canDirectPlay") && withinRate(source)) {
                    if (!extraContainer.empty() && source.at("container").string() == extraContainer) return original();
                    for (const auto* f : *targets)
                        if (matchesStream(*f, source, allowAlac, true)) return original();
                }
                const auto& output = decision.at("transcodeStream");
                const auto token = decision.at("transcodeParams").string();
                if (allowed("canTranscode") && !token.empty() && output.at("protocol").string() == "http" && withinRate(output)) {
                    for (const auto* f : *targets) {
                        if (matchesStream(*f, output, false, false))
                            return {url("getTranscodeStream", media) + "&transcodeParams=" + escapedDecisionToken(token),
                                    choice(*f), playableSuffix(inputs, *f)};
                    }
                }
                reason = decision.at("errorReason").string();
                if (!offerDirect || !allowed("canDirectPlay") || targets->empty()) break;
                offerDirect = false;
            }
        }
        throw Error(reason.empty() ? "The server could not supply a format playable by the enabled WACUP inputs." : reason.substr(0, 300));
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
    HttpClient& http_;
    Server server_;

private:
    static void appendStarred(std::vector<LibraryItem>& items, const Json& starred,
                              const std::wstring& query) {
        appendItems(items, starred.at("album"), BrowseKind::albumTracks, query);
        appendItems(items, starred.at("artist"), BrowseKind::artistAlbums, query);
        appendItems(items, starred.at("song"), BrowseKind::song, query);
    }

    void appendDirectoryTracks(std::vector<Track>& result, const Json& items) {
        for (Json node : items.array()) {
            if (!node.at("isDir").boolean().value_or(false)) appendTrack(result, server_.id, node);
        }
    }

    static void appendDirectory(std::vector<LibraryItem>& result, const Json& items,
                                const std::wstring& query) {
        for (Json node : items.array()) {
            auto id = node.at("id").string();
            auto title = node.at("title").string();
            auto artist = node.at("artist").string();
            if (!usableId(id) || (!matches(title, query) && !matches(artist, query))) continue;
            if (title.empty()) title = id;
            auto kind = node.at("isDir").boolean().value_or(false) ? BrowseKind::directory : BrowseKind::song;
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
        JsonDocument response;
        Json entries;
        switch (location.kind) {
        case BrowseKind::genreAlbums:
            response = api("getAlbumList2", {{"type", "byGenre"}, {"genre", location.id}, {"size", "5"}});
            entries = response.at("albumList2").at("album");
            break;
        case BrowseKind::artistAlbums:
            response = api("getArtist", {{"id", location.id}});
            entries = response.at("artist").at("album");
            break;
        case BrowseKind::playlistTracks:
            response = api("getPlaylist", {{"id", location.id}});
            entries = response.at("playlist").at("entry");
            break;
        case BrowseKind::directory:
            response = api("getMusicDirectory", {{"id", location.id}});
            entries = response.at("directory").at("child");
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
        for (Json entry : entries.array()) {
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
    bool hasTranscoding() {
        using Clock = std::chrono::steady_clock;
        static std::mutex mutex;
        static std::map<std::pair<std::string,std::string>, std::pair<Clock::time_point,bool>> known;
        const auto key = std::make_pair(server_.url, server_.user);
        {
            std::scoped_lock lock(mutex);
            auto it = known.find(key);
            if (it != known.end() && it->second.first > Clock::now()) return it->second.second;
        }
        bool supported = false;
        try {
            auto extensions = api("getOpenSubsonicExtensions");
            for (Json e : extensions.at("openSubsonicExtensions").array())
                if (e.at("name").string() == "transcoding")
                    for (Json version : e.at("versions").array()) supported |= version.seconds() == 1;
        } catch (const SubsonicError& error) {
            if (error.code != 0 && error.code != 70) throw;
        }
        std::scoped_lock lock(mutex);
        known[key] = {Clock::now() + std::chrono::minutes(10), supported};
        return supported;
    }
    JsonDocument api(const std::string& endpoint, Parameters parameters = {}, std::string* original = nullptr,
             const std::string& body = {}) {
        Response response(http_, url(endpoint, std::move(parameters)), body.empty() ? L"GET" : L"POST",
                          body.empty() ? L"" : L"Content-Type: application/json\r\n", body);
        if (response.status == 404 || response.status == 501)
            throw SubsonicError(70, "The server does not provide this resource.");
        std::string payload;
        if (original && response.status == 200) payload = response.text(2 * MiB);
        auto envelope = payload.empty() ? readJson(response) : parseJson(payload);
        const auto& result = envelope.at("subsonic-response");
        if (result.at("status").string() != "ok") {
            const auto& error = result.at("error");
            auto reason = error.at("message").string();
            throw SubsonicError(error.at("code").seconds(),
                reason.empty() ? "The Subsonic request failed." : reason.substr(0, 300));
        }
        if (original) *original = std::move(payload);
        envelope.value = result.value;
        return envelope;
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

    JsonDocument list(const char* endpoint, int offset, const char* order, Parameters parameters) {
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
        if (!items.value.is_array())
            throw Error("Navidrome returned an unexpected library response.");
        return items;
    }

    static bool validToken(const std::string& token) {
        return !token.empty() && token.find_first_not_of(
            "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_.") == std::string::npos;
    }
    std::unique_ptr<Response> authorizedGet(const std::string& url) {
        if (token_.empty()) token_ = login();
        return std::make_unique<Response>(http_, url, L"GET",
            L"x-nd-authorization: Bearer " + wide(token_) + L"\r\n");
    }
    std::string login() {
        auto body = "{\"username\":" + jsonString(server_.user) +
                    ",\"password\":" + jsonString(server_.password) + "}";
        Response response(http_, server_.url + "/auth/login", L"POST",
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
    auto parsed = parseJson(text);
    SortTags tags;
    auto object = parsed.value.get_object();
    if (!object.error())
        for (auto field : object.value_unsafe())
            tags.insert_or_assign(std::string(field.key), Json(field.value).string());
    return tags;
}

bool matches(std::wstring_view text, std::wstring_view query) {
    if (query.empty()) return true;
    return !text.empty() && FindNLSStringEx(LOCALE_NAME_INVARIANT,
        FIND_FROMSTART | NORM_IGNORECASE, text.data(), static_cast<int>(text.size()),
        query.data(), static_cast<int>(query.size()), nullptr, nullptr, nullptr, 0) >= 0;
}

bool matches(const std::string& value, const std::wstring& query) {
    return query.empty() || matches(wide(value), query);
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

bool sameServerEndpoint(const std::string& savedUrl, const std::string& editedUrl) {
    if (savedUrl == editedUrl) return true;
    try {
        const auto saved = splitUrl(wide(savedUrl));
        const auto edited = splitUrl(wide(normalizeUrl(editedUrl)));
        if (_wcsicmp(saved.host.c_str(), edited.host.c_str()) || saved.target != edited.target) return false;
        if (saved.secure == edited.secure) return saved.port == edited.port;
        return edited.secure && (saved.port == edited.port ||
            (saved.port == INTERNET_DEFAULT_HTTP_PORT && edited.port == INTERNET_DEFAULT_HTTPS_PORT));
    } catch (const Error&) {
        // The settings dialog also checks incomplete addresses as the user types.
        return false;
    }
}

bool acceptsExtension(const InputExtensions& inputs, std::string suffix) {
    std::transform(suffix.begin(), suffix.end(), suffix.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return inputs.count(suffix) != 0;
}

bool acceptsCodec(const InputExtensions& inputs, std::string_view codec) {
    return std::any_of(std::begin(audioFormats), std::end(audioFormats), [&](const AudioFormat& f) {
        return f.codec == codec && !playableSuffix(inputs, f).empty();
    });
}

std::shared_ptr<void> HttpClient::connection(const std::wstring& host, INTERNET_PORT port, bool secure) {
    std::scoped_lock lock(mutex_);
    auto found = std::find_if(origins_.begin(), origins_.end(), [&](const Origin& origin) {
        return origin.port == port && origin.secure == secure && !_wcsicmp(origin.host.c_str(), host.c_str());
    });
    if (found != origins_.end()) {
        origins_.splice(origins_.begin(), origins_, found);
        return found->handle;
    }
    if (!session_) {
        InternetHandle session(WinHttpOpen(L"ml_tangerinestream/" TS_VERSION_STRING, WINHTTP_ACCESS_TYPE_NO_PROXY,
                                           WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0));
        if (!session || !WinHttpSetTimeouts(session.get(), 3000, 4000, 4000, 5000)) networkError();
        session_ = std::move(session);
    }
    InternetHandle handle(WinHttpConnect(session_.get(), host.c_str(), port, 0));
    if (!handle) networkError();
    origins_.push_front({host, port, secure, std::shared_ptr<void>(std::move(handle))});
    constexpr size_t maxIdleOrigins = 16;
    if (origins_.size() > maxIdleOrigins) origins_.pop_back();
    return origins_.front().handle;
}

Response::Response(HttpClient& http, const std::string& url, const wchar_t* method,
                   const std::wstring& headers, const std::string& body) {
    auto address = splitUrl(wide(url));
    connection_ = http.connection(address.host, address.port, address.secure);
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

std::unique_ptr<ServerApi> HttpClient::connect(const Server& server) {
    if (server.native) return std::make_unique<Navidrome>(*this, server);
    return std::make_unique<Subsonic>(*this, server);
}

} // namespace tangerine
