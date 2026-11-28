#pragma once

#include "browse_sort.hpp"
#include <array>
#include <functional>
#include <memory>

namespace tangerine {

enum class ViewMode { list, covers, filtered };
struct ViewOptions {
    ViewMode mode = ViewMode::covers;
    bool albumResults = false, albumsRight = false, filtersRight = false, resultArt = false;
    bool albumArt = true, coverTitles = true, coverDetails = false;
    std::vector<std::string> filters, trackColumns, itemColumns;
    std::string trackSort;
    std::string musicPreset;
    std::map<std::string, std::vector<std::string>> filterColumns;
    int splitPercent = 45;
};

struct Column {
    const char* key;
    const wchar_t* label;
    int width; // Relative width, also used as a readable minimum on crowded headers.
};
using Columns = std::vector<Column>;
struct MusicPreset {
    const char* key;
    const wchar_t* label;
    const char* sort = nullptr;
};
const std::vector<MusicPreset>& musicPresets(BrowseKind kind);
Columns viewFields(BrowseKind kind);
Columns filterFields(BrowseKind kind);
Columns summaryFields(const wchar_t* valueLabel);
ViewOptions readViewOptions(BrowseKind kind, const SortTags& values, bool textCatalog);
SortTags writeViewOptions(const ViewOptions& options);
std::vector<std::string> defaultColumns(BrowseKind kind);
std::vector<std::string> defaultSummaryColumns(std::string_view field);
const Column* findColumn(const Columns& fields, std::string_view key);
std::wstring formatCell(std::string_view key, const std::string& value);

struct FilterSummary {
    std::wstring value;
    std::string year;
    int items = 0;
    std::int64_t songs = 0, seconds = 0;
};
std::wstring summaryCell(const FilterSummary& row, std::string_view key);
using FilterSelection = std::array<std::set<std::wstring>, 3>;
struct AlbumGroup {
    LibraryItem item;
    std::vector<int> rows;
};
struct ViewProjection {
    std::array<std::vector<FilterSummary>, 3> groups;
    std::vector<int> matches;
    std::vector<AlbumGroup> albums;
    std::shared_ptr<const std::vector<size_t>> order;
};
using Cancelled = std::function<bool()>;
ViewProjection projectRows(const BrowsePage& source, bool songs, const ViewOptions& options,
                           const FilterSelection& selection, SortChoice sort, bool descending,
                           std::shared_ptr<const std::vector<size_t>> order, const std::map<Id,std::string>& cache,
                           const Cancelled& cancelled);

} // namespace tangerine
