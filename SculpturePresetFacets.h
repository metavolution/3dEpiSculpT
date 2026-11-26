#pragma once

#include "SculpturePresetSearch.h"

#include <algorithm>
#include <cstddef>
#include <string_view>
#include <vector>

namespace epi_sculpt {

// Borrowed metadata only. Category equivalence matches --category's ASCII
// folding; a deterministic spelling makes the output independent of load order.
struct SculpturePresetCategoryFacet {
    std::string_view category;
    std::size_t count = 0U;
};

[[nodiscard]] inline std::vector<SculpturePresetCategoryFacet> presetCategoryFacets(
    std::vector<std::string_view> categories) {
    const auto foldedLess = [](std::string_view left, std::string_view right) {
        return std::lexicographical_compare(left.begin(), left.end(), right.begin(), right.end(),
            [](char a, char b) {
                return static_cast<unsigned char>(preset_search_detail::fold(a))
                    < static_cast<unsigned char>(preset_search_detail::fold(b));
            });
    };
    std::sort(categories.begin(), categories.end(), [&](auto left, auto right) {
        if (foldedLess(left, right)) return true;
        if (foldedLess(right, left)) return false;
        return left < right;
    });
    std::vector<SculpturePresetCategoryFacet> result;
    for (const auto category : categories) {
        if (result.empty() || !matchesPresetCategory(result.back().category, category)) {
            result.push_back({category, 1U});
        } else {
            ++result.back().count;
        }
    }
    return result;
}

} // namespace epi_sculpt
