#pragma once

#include <algorithm>
#include <cstddef>
#include <utility>
#include <vector>

namespace epi_sculpt {

// Streaming first-K selection: equivalent to a stable sort of every observed
// candidate followed by taking K, while retaining at most K records. Insert
// after equivalent values so the caller's discovery order settles score ties.
//
// Callers own ranking/admission policy and reserve capacity. `ranked` must
// already be sorted under `before`, with size <= limit. Taking value by value
// also makes insertion safe when the candidate aliases an existing element.
template<class T, class Before>
inline void retainBoundedSorted(std::vector<T>& ranked, T value, std::size_t limit, Before before) {
    if (limit == 0) return;
    const auto index = static_cast<std::size_t>(std::upper_bound(
        ranked.begin(), ranked.end(), value, before) - ranked.begin());
    if (index >= limit) return;
    if (ranked.size() == limit) ranked.pop_back();
    ranked.insert(ranked.begin() + index, std::move(value));
}

} // namespace epi_sculpt
