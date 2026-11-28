#pragma once
#include "core/model.hpp"
#include <algorithm>
#include <cctype>

namespace tangerine {
inline void addExtensions(InputExtensions& result, std::string_view group) {
    size_t pos = 0;
    while ((pos = group.find_first_not_of(" ;*.\t", pos)) != group.npos) {
        const auto end = group.find_first_of(" ;\t", pos);
        std::string suffix(group.substr(pos, end - pos));
        std::transform(suffix.begin(), suffix.end(), suffix.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        result.insert(std::move(suffix));
        if (end == group.npos) break;
        pos = end + 1;
    }
}
}
