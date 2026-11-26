#pragma once

#include <cstddef>
#include <string_view>

namespace epi_sculpt {

enum class PresetPathObjective { FewestHops, GentlestSteps };

[[nodiscard]] constexpr std::string_view presetPathObjectiveName(PresetPathObjective objective) noexcept {
    switch (objective) {
    case PresetPathObjective::FewestHops: return "fewest-hops";
    case PresetPathObjective::GentlestSteps: return "gentlest";
    }
    return "unknown";
}

// Inspection policy shared by value queries and CLI admission. These are cold
// host work/output bounds, not mathematical cadence or graphics quality limits.
inline constexpr std::size_t kDefaultPresetNeighborLimit = 5;
inline constexpr std::size_t kMaxPresetNeighborLimit = 32;
inline constexpr std::size_t kMaxPresetJourneySteps = 64;
inline constexpr std::size_t kDefaultPresetMorphStudySamples = 9;
inline constexpr std::size_t kMinPresetMorphStudySamples = 2;
inline constexpr std::size_t kMaxPresetMorphStudySamples = 33;
inline constexpr std::size_t kDefaultPresetMorphRefinements = 8;
inline constexpr std::size_t kMaxPresetMorphRefinements = 12;
inline constexpr double kDefaultPresetPathMaxDistance = 0.01;
inline constexpr std::size_t kDefaultPresetPathHops = 8;
inline constexpr std::size_t kMaxPresetPathHops = 16;
inline constexpr std::size_t kMaxPresetPathCatalog = 4096;
inline constexpr std::size_t kMaxPresetPathCandidates = 512;

} // namespace epi_sculpt
