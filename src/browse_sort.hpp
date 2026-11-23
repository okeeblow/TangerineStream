#pragma once

#include "model.hpp"

namespace tangerine {

struct SortChoice {
    const char* key;
    const wchar_t* label;
    bool numeric = false;
    bool descending = false;
};

const std::vector<SortChoice>& sortChoices(BrowseKind kind);
const SortChoice& defaultSort(BrowseKind kind);
std::vector<size_t> sortRows(std::vector<Track>& rows, const SortChoice& choice, bool descending);
std::vector<size_t> sortRows(std::vector<LibraryItem>& rows, const SortChoice& choice, bool descending);

} // namespace tangerine
