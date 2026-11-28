#include "library_view.hpp"
#include <windows.h>
#include <algorithm>
#include <charconv>
#include <climits>
#include <date/date.h>
#include <tuple>

namespace tangerine {
namespace {
const Column title{"name", L"Title", 160}, details{"details", L"Details", 120}, album{"album", L"Album", 150},
    artist{"artist", L"Artist", 125}, albumArtist{"albumArtist", L"Album artist", 125}, genre{"genre", L"Genre", 95},
    year{"year", L"Year", 48}, duration{"duration", L"Length", 56}, count{"songCount", L"Tracks", 55},
    rating{"rating", L"Rating", 48}, plays{"playCount", L"Play count", 66}, added{"created", L"Date added", 135},
    played{"played", L"Last played", 135}, favorite{"starred", L"Favorited", 120}, id{"id", L"ID", 160},
    format{"format", L"Format", 55}, channels{"channels", L"Channels", 62}, bitrate{"bitRate", L"Bitrate (kbps)", 80},
    rate{"sampleRate", L"Sample rate (Hz)", 94}, depth{"bitDepth", L"Bit depth", 62};

bool albums(BrowseKind kind) {
    return kind == BrowseKind::albums || kind == BrowseKind::genreAlbums || kind == BrowseKind::artistAlbums;
}

std::vector<std::string> decode(const std::string& value, const Columns& allowed, size_t limit = SIZE_MAX) {
    std::vector<std::string> keys;
    size_t start = 0;
    while (start < value.size() && keys.size() < limit) {
        auto end = value.find(',', start);
        auto key = value.substr(start, end - start);
        if (findColumn(allowed, key) && std::find(keys.begin(), keys.end(), key) == keys.end())
            keys.push_back(std::move(key));
        if (end == value.npos) break;
        start = end + 1;
    }
    return keys;
}
std::string encode(const std::vector<std::string>& keys) {
    std::string result;
    for (auto& key : keys) { if (!result.empty()) result += ','; result += key; }
    return result;
}

struct NaturalOrder {
    bool operator()(const std::wstring& left, const std::wstring& right) const {
        return CompareStringEx(LOCALE_NAME_USER_DEFAULT, NORM_IGNORECASE | SORT_DIGITSASNUMBERS,
            left.c_str(), -1, right.c_str(), -1, nullptr, nullptr, 0) == CSTR_LESS_THAN;
    }
};

struct PresetFilter {
    std::string key, cutoff, todayText;
    int year = 0, decade = 0;
    explicit PresetFilter(const std::string& preset) : key(preset) {
        if (key.empty()) return;
        auto today = date::floor<date::days>(std::chrono::system_clock::now());
        year = static_cast<int>(date::year_month_day{today}.year());
        if (key == "created" || key == "played" || key == "updated") {
            cutoff = date::format("%F", today - date::days{30});
            todayText = date::format("%F", today);
        }
        if (key.size() == 4) std::from_chars(key.data(), key.data() + key.size(), decade);
    }
    bool includes(const Track& t) const {
        if (key.empty()) return true;
        if (decade) return t.year >= decade && t.year < decade + 10;
        if (key == "thisYear") return t.year == year;
        if (key == "missing") return t.title.empty() || t.album.empty() || t.artist.empty();
        if (key == "rock" || key == "classical") return matches(t.genre, key == "rock" ? L"rock" : L"classical");
        if (!cutoff.empty()) {
            auto value = browseField(t, key);
            return value.size() >= 10 && value.compare(0, 10, cutoff) >= 0 && value.compare(0, 10, todayText) <= 0;
        }
        if (key == "rating") {
            auto value = browseField(t, "rating");
            return value == "4" || value == "5";
        }
        const auto count = browseField(t, "playCount");
        const bool played = (!count.empty() && count != "0") || !browseField(t, "played").empty();
        return key == "unplayed" ? !played : played;
    }
};
}

const std::vector<MusicPreset>& musicPresets(BrowseKind kind) {
    static const std::vector<MusicPreset> music{
        {"thisYear", L"Audio from this year"}, {"plays", L"Most played", "playCount"},
        {"unplayed", L"Never played"}, {"created", L"Recently added (30 days)", "created"},
        {"updated", L"Recently modified (30 days)", "updated"}, {"played", L"Recently played (30 days)", "played"},
        {"rating", L"Top rated (4 or 5 stars)", "rating"},
        {"1960", L"1960s music"}, {"1970", L"1970s music"}, {"1980", L"1980s music"}, {"1990", L"1990s music"},
        {"2000", L"2000s music"}, {"2010", L"2010s music"}, {"2020", L"2020s music"},
        {"classical", L"Classical music"}, {"rock", L"Rock music"}, {"missing", L"Missing title, artist or album"}};
    static const std::vector<MusicPreset> none;
    return trackView(kind) || kind == BrowseKind::albums ? music : none;
}

const Column* findColumn(const Columns& fields, std::string_view key) {
    auto it = std::find_if(fields.begin(), fields.end(), [&](const Column& c) { return c.key == key; });
    return it == fields.end() ? nullptr : &*it;
}

Columns viewFields(BrowseKind kind) {
    if (trackView(kind)) return {title, artist, album, duration, {"cache", L"Cache", 84}, albumArtist,
        {"track", L"Track #", 53}, {"disc", L"Disc #", 48}, genre, year, rating, plays, added, played,
        favorite, format, bitrate, channels, rate, depth, {"bpm", L"BPM", 45}, {"comment", L"Comment", 150},
        {"path", L"Path", 190}, id};
    if (albums(kind)) return {{"name", L"Album", 160}, albumArtist, artist, genre, year, count, duration, plays,
        rating, added, played, favorite, id};
    if (kind == BrowseKind::playlists) return {{"name", L"Playlist", 160}, details, {"owner", L"Owner", 100},
        count, duration, {"public", L"Public", 52}, {"updated", L"Last updated", 135}, id};
    if (kind == BrowseKind::genres || kind == BrowseKind::artists || kind == BrowseKind::albumArtists)
        return {{"name", kind == BrowseKind::genres ? L"Genre" : kind == BrowseKind::artists ? L"Artist" : L"Album artist", 160},
            details, {"albumCount", L"Albums", 60}, count, id};
    return {{"name", L"Name", 160}, details, artist, albumArtist, genre, year, count, duration, id};
}

Columns filterFields(BrowseKind kind) {
    if (trackView(kind) || kind == BrowseKind::albums) return {album, albumArtist, artist, genre, year, format,
        rating, channels, bitrate, rate, depth, {"cache", L"Cache", 84}};
    if (albums(kind)) return {albumArtist, artist, genre, year, rating, {"name", L"Album", 160}};
    auto fields = viewFields(kind);
    fields.erase(std::remove_if(fields.begin(), fields.end(), [](const Column& c) {
        return std::string_view(c.key) == "id" || std::string_view(c.key) == "duration";
    }), fields.end());
    return fields;
}

Columns summaryFields(const wchar_t* label) {
    return {{"value", label, 155}, year, count, duration, {"items", L"Items", 55}};
}
std::vector<std::string> defaultSummaryColumns(std::string_view field) {
    return field == "album" ? std::vector<std::string>{"value","year","songCount"} :
                              std::vector<std::string>{"value","songCount"};
}
std::vector<std::string> defaultColumns(BrowseKind kind) {
    if (trackView(kind)) return {"name","artist","album","duration","cache"};
    if (albums(kind)) return {"name","albumArtist","year","songCount"};
    return {"name","details"};
}

ViewOptions readViewOptions(BrowseKind kind, const SortTags& values, bool textCatalog) {
    ViewOptions o;
    auto get = [&](const std::string& key) {
        auto it = values.find(key);
        return it == values.end() ? std::string{} : it->second;
    };
    auto mode = get("mode");
    if (mode == "filtered") o.mode = ViewMode::filtered;
    else if (mode == "list" || (mode.empty() && (trackView(kind) || (textCatalog && kind != BrowseKind::albums))))
        o.mode = ViewMode::list;
    o.albumArt = get("albumArt") != "0" && !(mode.empty() && textCatalog && kind == BrowseKind::albums);
    o.albumResults = get("albumResults") == "1";
    o.albumsRight = get("albumsRight") == "1";
    o.filtersRight = get("filtersRight") == "1";
    o.resultArt = get("resultArt") == "1";
    o.coverTitles = get("coverTitles") != "0";
    o.coverDetails = get("coverDetails") == "1";
    o.trackSort = get("trackSort");
    const auto preset = get("musicPreset");
    for (const auto& entry : musicPresets(kind)) if (preset == entry.key) o.musicPreset = preset;
    auto split = get("split");
    int percent = 0;
    std::from_chars(split.data(), split.data() + split.size(), percent);
    if (percent >= 15 && percent <= 85) o.splitPercent = percent;
    auto fields = filterFields(kind);
    o.filters = decode(get("filters"), fields, 3);
    if (o.filters.empty()) o.filters.push_back(fields.front().key);
    o.trackColumns = decode(get("trackColumns"), viewFields(BrowseKind::tracks));
    if (o.trackColumns.empty()) o.trackColumns = defaultColumns(BrowseKind::tracks);
    const auto itemKind = trackView(kind) ? BrowseKind::albums : kind;
    o.itemColumns = decode(get("itemColumns"), viewFields(itemKind));
    if (o.itemColumns.empty()) o.itemColumns = defaultColumns(itemKind);
    for (const auto& f : fields) {
        auto columns = decode(get(std::string("columns.") + f.key), summaryFields(f.label));
        o.filterColumns[f.key] = columns.empty() ? defaultSummaryColumns(f.key) : std::move(columns);
    }
    return o;
}
SortTags writeViewOptions(const ViewOptions& o) {
    SortTags tags{{"mode", o.mode == ViewMode::covers ? "covers" : o.mode == ViewMode::list ? "list" : "filtered"},
        {"albumResults", o.albumResults ? "1" : "0"}, {"albumsRight", o.albumsRight ? "1" : "0"},
        {"filtersRight", o.filtersRight ? "1" : "0"}, {"resultArt", o.resultArt ? "1" : "0"},
        {"albumArt", o.albumArt ? "1" : "0"}, {"coverTitles", o.coverTitles ? "1" : "0"}, {"coverDetails", o.coverDetails ? "1" : "0"},
        {"split", std::to_string(o.splitPercent)}, {"filters", encode(o.filters)}, {"trackSort", o.trackSort},
        {"musicPreset", o.musicPreset},
        {"trackColumns", encode(o.trackColumns)}, {"itemColumns", encode(o.itemColumns)}};
    for (const auto& [field, columns] : o.filterColumns) tags["columns." + field] = encode(columns);
    return tags;
}

std::wstring formatCell(std::string_view key, const std::string& value) {
    if (key == "duration" && !value.empty()) {
        std::int64_t n = 0;
        std::from_chars(value.data(), value.data() + value.size(), n);
        return std::to_wstring(n / 60) + (n % 60 < 10 ? L":0" : L":") + std::to_wstring(n % 60);
    }
    if ((key == "public" || key == "starred") && (value == "0" || value == "1"))
        return value == "1" ? L"Yes" : L"No";
    return wide(value);
}
std::wstring summaryCell(const FilterSummary& row, std::string_view key) {
    if (key == "year") return wide(row.year);
    if (key == "songCount") return row.songs < 0 ? L"" : std::to_wstring(row.songs);
    if (key == "items") return std::to_wstring(row.items);
    if (key == "duration") return row.seconds > 0 ? formatCell(key, std::to_string(row.seconds)) : L"";
    return row.value;
}

ViewProjection projectRows(const BrowsePage& source, bool songs, const ViewOptions& options,
                           const FilterSelection& selection, SortChoice sort, bool descending,
                           std::shared_ptr<const std::vector<size_t>> order, const std::map<Id,std::string>& cache,
                           const Cancelled& cancelled) {
    ViewProjection out;
    // The Name header in a cover grid labels albums.
    const bool albumOrder = songs && options.mode == ViewMode::covers &&
        (std::string_view(sort.key) == "name" || std::string_view(sort.key) == "album");
    if (albumOrder) sort.key = "album";
    out.order = order ? std::move(order) : std::make_shared<std::vector<size_t>>(songs ?
        sortIndices(source.tracks, sort, descending) : sortIndices(source.items, sort, descending));
    const size_t size = songs ? source.tracks.size() : source.items.size();
    PresetFilter preset(options.musicPreset);
    if (options.mode == ViewMode::list) out.matches.reserve(size);
    std::array<std::map<std::wstring, FilterSummary, NaturalOrder>, 3> groups;
    using AlbumKey = std::tuple<Id, std::string, std::string, std::string>;
    std::map<AlbumKey, size_t> albums;
    for (size_t pos = 0; pos < size; ++pos) {
        if ((pos % 256) == 0 && cancelled()) return {};
        const int row = static_cast<int>(out.order->empty() ? pos : (*out.order)[pos]);
        if (songs && !preset.includes(source.tracks[row])) continue;
        if (options.mode == ViewMode::list) { out.matches.push_back(row); continue; }
        if (options.mode == ViewMode::covers) {
            const auto& t = source.tracks[row];
            // A missing album ID must not merge unrelated artists or two servers' releases.
            AlbumKey key{t.server, t.albumId, t.albumId.empty() ? t.albumArtist.empty() ? t.artist : t.albumArtist : "",
                         t.albumId.empty() ? t.album : ""};
            auto [it, fresh] = albums.emplace(std::move(key), out.albums.size());
            if (fresh) out.albums.push_back({{{BrowseKind::albumTracks, t.albumId, t.album.empty() ? "(no album)" : t.album,
                t.server}, t.albumArtist.empty() ? t.artist : t.albumArtist, t.coverId, 0, 0, t.sortTags}, {}});
            auto& group = out.albums[it->second];
            if (group.item.coverId.empty()) group.item.coverId = t.coverId;
            ++group.item.songCount;
            group.item.duration += t.duration;
            group.rows.push_back(row);
            continue;
        }
        auto field = [&](std::string_view key) {
            if (songs && key == "cache") {
                auto it = cache.find(source.tracks[row].key);
                if (it != cache.end()) return it->second;
            }
            return songs ? browseField(source.tracks[row], key) : browseField(source.items[row], key);
        };
        bool match = true;
        for (size_t i = 0; match && i < options.filters.size(); ++i) {
            const auto& key = options.filters[i];
            auto value = formatCell(key, field(key));
            auto [it, fresh] = groups[i].try_emplace(std::move(value));
            auto& g = it->second;
            auto y = field("year");
            if (fresh) { g.value = it->first; g.year = std::move(y); }
            else if (y != g.year) g.year.clear();
            ++g.items;
            int count = songs ? 1 : source.items[row].songCount;
            g.songs = count < 0 || g.songs < 0 ? -1 : g.songs + count;
            g.seconds += songs ? source.tracks[row].duration : source.items[row].duration;
            match = selection[i].empty() || selection[i].count(it->first);
        }
        if (match) out.matches.push_back(row);
    }
    if (albumOrder) {
        auto position = [&](int row) {
            const auto& t = source.tracks[row];
            return std::pair{std::max(1, t.discNumber), t.trackNumber > 0 ? t.trackNumber : INT_MAX};
        };
        for (auto& group : out.albums)
            std::stable_sort(group.rows.begin(), group.rows.end(), [&](int a, int b) { return position(a) < position(b); });
    }
    for (size_t i = 0; i < groups.size(); ++i) {
        auto& list = out.groups[i];
        FilterSummary all;
        all.value = L"All (" + std::to_wstring(groups[i].size()) + L")";
        for (auto& entry : groups[i]) {
            all.items += entry.second.items;
            all.songs = all.songs < 0 || entry.second.songs < 0 ? -1 : all.songs + entry.second.songs;
            all.seconds += entry.second.seconds;
        }
        list.push_back(std::move(all));
        for (auto& entry : groups[i]) list.push_back(std::move(entry.second));
    }
    return out;
}
} // namespace tangerine
