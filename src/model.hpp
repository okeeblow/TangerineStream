#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace tangerine {

using Id = std::int64_t;
constexpr std::int64_t MiB = 1024 * 1024;
constexpr int pageSize = 100;

struct Server {
    Id id = 0;
    std::string name;
    std::string url;
    std::string user;
    std::string password;
    bool native = false;
    bool cacheFavorites = false;
};

struct Track {
    Id key = 0;
    Id server = 0;
    std::string id;
    std::string title;
    std::string artist;
    std::string album;
    std::string suffix;
    std::string mime;
    int duration = 0;
    bool pinned = false;
    bool favorite = false;
    std::int64_t bytes = 0;
    std::string coverId, albumId, artistId, albumArtistId, albumArtist, genre;
    int discNumber = 0, trackNumber = 0, year = 0;
};

// These values are also stored in the catalog database.
enum class BrowseKind {
    albums, genres, albumArtists, artists, folders, playlists, favorites, tracks, offline,
    genreAlbums, artistAlbums, folderIndex, directory, albumTracks, playlistTracks, song
};

struct BrowseLocation {
    BrowseKind kind;
    std::string id;
    std::string title;
    Id server = 0;
};

inline bool localLocation(const BrowseLocation& location) {
    return !location.id.empty() && location.id.front() == '\x1f';
}

struct LibraryItem {
    BrowseLocation target;
    std::string subtitle;
    std::string coverId;
    int songCount = -1;
    std::int64_t duration = 0;
};

struct BrowsePage {
    std::vector<LibraryItem> items;
    std::vector<Track> tracks;
    std::vector<Track> relatedTracks;
    bool more = false;
};

struct Lyrics {
    std::string plain;
    // Original OpenSubsonic lyrics response, including offsets and any line or cue timing.
    std::string structured;
};

inline bool trackView(BrowseKind kind) {
    return kind == BrowseKind::tracks || kind == BrowseKind::offline ||
        kind == BrowseKind::albumTracks || kind == BrowseKind::playlistTracks ||
        kind == BrowseKind::song;
}

struct Report {
    Id key;
    std::string id;
    std::int64_t time;
};

class Error : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class ConnectionError : public Error {
public:
    using Error::Error;
};

std::wstring wide(std::string_view value);
std::string utf8(std::wstring_view value);
bool matches(const std::string& value, const std::wstring& query);
std::string playlistTitle(const Track& track);
const wchar_t* artworkExtension(std::string_view data);
std::string randomHex();
std::int64_t nowMs();

} // namespace tangerine
