#include "browse_sort.hpp"
#include <windows.h>
#include <algorithm>
#include <charconv>
#include <random>
#include <numeric>

namespace tangerine {
namespace {
constexpr SortChoice sequence{"sequence", L"Original order"}, name{"name", L"Name"}, id{"id", L"ID"},
    album{"album", L"Album"}, artist{"artist", L"Artist"}, albumArtist{"albumArtist", L"Album Artist"},
    duration{"duration", L"Duration", true, true}, plays{"playCount", L"Most played", true, true},
    randomOrder{"random", L"Random"}, rating{"rating", L"Rating", true, true},
    added{"created", L"Recently added", false, true}, played{"played", L"Recently played", false, true},
    songs{"songCount", L"Song count", true, true}, starred{"starred", L"Favorited", false, true},
    release{"releaseYear", L"Release year", true, true}, year{"year", L"Year", true, true},
    albums{"albumCount", L"Album count", true, true}, bpm{"bpm", L"BPM", true, true},
    channels{"channels", L"Channels", true}, comment{"comment", L"Comment"}, genre{"genre", L"Genre"},
    owner{"owner", L"Owner"}, visibility{"public", L"Public", true, true},
    updated{"updated", L"Recently updated", false, true}, sortName{"sortName", L"Sort name"};

std::string tag(const SortTags& tags, std::string_view key, const std::string& fallback = {}) {
    const auto it = tags.find(key);
    return it != tags.end() && !it->second.empty() ? it->second : fallback;
}

std::string number(int64_t value, int minimum = 1) {
    return value >= minimum ? std::to_string(value) : std::string{};
}

std::string field(const LibraryItem& item, std::string_view key) {
    if (key == "name") return item.target.title;
    if (key == "id") return item.target.id;
    if (key == "songCount") return number(item.songCount, 0);
    if (key == "duration") return number(item.duration);
    if ((key == "artist" || key == "albumArtist") && item.target.kind == BrowseKind::albumTracks)
        return tag(item.sortTags, key, item.subtitle);
    if (key == "releaseYear") return tag(item.sortTags, key, tag(item.sortTags, "year"));
    return tag(item.sortTags, key);
}

std::string field(const Track& t, std::string_view key) {
    if (key == "name") return t.title.empty() ? t.id : t.title;
    if (key == "id") return t.id;
    if (key == "artist") return tag(t.sortTags, key, t.artist);
    if (key == "albumArtist") return tag(t.sortTags, key, t.albumArtist.empty() ? t.artist : t.albumArtist);
    if (key == "album") return t.album;
    if (key == "genre") return t.genre;
    if (key == "year") return number(t.year);
    if (key == "releaseYear") return tag(t.sortTags, key, number(t.year));
    if (key == "duration") return number(t.duration);
    if (key == "sortName") return tag(t.sortTags, key, t.title);
    return tag(t.sortTags, key);
}

int collate(const std::wstring& a, const std::wstring& b) {
    return CompareStringEx(LOCALE_NAME_USER_DEFAULT, NORM_IGNORECASE | SORT_DIGITSASNUMBERS,
        a.c_str(), -1, b.c_str(), -1, nullptr, nullptr, 0) - CSTR_EQUAL;
}

// Navidrome and OpenSubsonic timestamps can have different UTC offsets and precision.
int64_t timestamp(std::string_view value) {
    auto digits = [&](size_t pos, size_t width) -> int {
        if (pos + width > value.size()) return 0;
        int n = 0;
        auto first = value.data() + pos, last = first + width;
        return std::from_chars(first, last, n).ptr == last ? n : 0;
    };
    if (value.size() < 10) return 0;
    SYSTEMTIME time{};
    time.wYear = static_cast<WORD>(digits(0, 4));
    time.wMonth = static_cast<WORD>(digits(5, 2));
    time.wDay = static_cast<WORD>(digits(8, 2));
    time.wHour = static_cast<WORD>(digits(11, 2));
    time.wMinute = static_cast<WORD>(digits(14, 2));
    time.wSecond = static_cast<WORD>(digits(17, 2));
    FILETIME file{};
    if (!SystemTimeToFileTime(&time, &file)) return 0;
    int64_t ticks = (static_cast<int64_t>(file.dwHighDateTime) << 32) + file.dwLowDateTime;
    if (value.size() > 19 && value[19] == '.') {
        int place = 1000000;
        for (size_t i = 20; i < value.size() && place && value[i] >= '0' && value[i] <= '9'; ++i) {
            ticks += (value[i] - '0') * place;
            place /= 10;
        }
    }
    const auto zone = value.find_first_of("+-", 19);
    if (zone != value.npos) {
        const auto minute = zone + 3 < value.size() && value[zone + 3] == ':' ? zone + 4 : zone + 3;
        const auto minutes = 60 * digits(zone + 1, 2) + digits(minute, 2);
        ticks += (value[zone] == '-' ? 1LL : -1LL) * minutes * 60 * 10000000;
    }
    return ticks;
}

template<class T>
std::vector<size_t> sort(std::vector<T>& rows, const SortChoice& choice, bool descending) {
    const std::string_view key(choice.key);
    if (key == "sequence" || rows.size() < 2) return {};
    struct Entry {
        std::wstring value, title;
        int64_t number;
    };
    std::vector<size_t> indices(rows.size());
    std::iota(indices.begin(), indices.end(), size_t{0});
    if (key == "random") {
        std::mt19937 generator(std::random_device{}());
        std::shuffle(indices.begin(), indices.end(), generator);
    } else {
        std::vector<Entry> entries;
        entries.reserve(rows.size());
        const bool date = key == "created" || key == "played" || key == "starred" || key == "updated";
        for (const auto& row : rows) {
            const auto value = field(row, key);
            int64_t n = 0;
            if (choice.numeric) std::from_chars(value.data(), value.data() + value.size(), n);
            if (date) n = timestamp(value);
            entries.push_back({wide(value), key == "name" ? L"" : wide(field(row, "name")), n});
        }
        std::stable_sort(indices.begin(), indices.end(), [&](size_t left, size_t right) {
            const auto& a = entries[left];
            const auto& b = entries[right];
            if (a.value.empty() != b.value.empty()) return b.value.empty();
            int order = choice.numeric || date ? (a.number > b.number) - (a.number < b.number)
                                               : collate(a.value, b.value);
            if (order) return descending ? order > 0 : order < 0;
            return collate(a.title, b.title) < 0;
        });
    }
    std::vector<T> sorted;
    sorted.reserve(rows.size());
    for (auto i : indices) sorted.push_back(std::move(rows[i]));
    rows.swap(sorted);
    return indices;
}
} // namespace

const std::vector<SortChoice>& sortChoices(BrowseKind kind) {
    static const std::vector<SortChoice> albumChoices{albumArtist, id, artist, duration, plays, name,
        randomOrder, rating, added, played, songs, starred, release};
    static const std::vector<SortChoice> artistChoices{albums, starred, plays, name, rating, songs};
    static const std::vector<SortChoice> genreChoices{albums, name, songs};
    static const std::vector<SortChoice> playlistChoices{duration, name, owner, visibility, songs, updated};
    static const std::vector<SortChoice> trackChoices{album, albumArtist, artist, bpm, channels, comment,
        duration, starred, genre, name, plays, randomOrder, rating, added, played, release, year};
    static const std::vector<SortChoice> orderedTracks = [] {
        std::vector<SortChoice> choices{sequence, id, sortName};
        choices.insert(choices.end(), trackChoices.begin(), trackChoices.end());
        return choices;
    }();
    static const std::vector<SortChoice> folderChoices = [] {
        std::vector<SortChoice> choices{id};
        choices.insert(choices.end(), trackChoices.begin(), trackChoices.end());
        return choices;
    }();
    switch (kind) {
    case BrowseKind::genres: return genreChoices;
    case BrowseKind::artists: case BrowseKind::albumArtists: return artistChoices;
    case BrowseKind::playlists: return playlistChoices;
    case BrowseKind::folders: case BrowseKind::directory: case BrowseKind::folderIndex: return folderChoices;
    case BrowseKind::albumTracks: case BrowseKind::playlistTracks: return orderedTracks;
    case BrowseKind::song: case BrowseKind::tracks: case BrowseKind::offline: return trackChoices;
    default: return albumChoices;
    }
}

const SortChoice& defaultSort(BrowseKind kind) {
    return kind == BrowseKind::playlistTracks || kind == BrowseKind::albumTracks ? sequence : name;
}
std::vector<size_t> sortRows(std::vector<Track>& rows, const SortChoice& choice, bool descending) {
    return sort(rows, choice, descending);
}
std::vector<size_t> sortRows(std::vector<LibraryItem>& rows, const SortChoice& choice, bool descending) {
    return sort(rows, choice, descending);
}
} // namespace tangerine
