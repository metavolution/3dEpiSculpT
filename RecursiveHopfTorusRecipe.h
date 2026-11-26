#pragma once

#include <cstdint>
#include <string_view>

namespace epi_sculpt::recursive_hopf {

inline constexpr std::uint32_t kRecipeVersion = 1U;
inline constexpr std::uint32_t kMaxFibersPerFamily = 8U;
inline constexpr std::uint32_t kMaxLogicalDepth = 8U;

struct PhaseMotion {
    double initialRadians = 0.0;
    double radiansPerSecond = 0.0;

    [[nodiscard]] bool operator==(const PhaseMotion&) const noexcept = default;
};

enum class PlacementMode : std::uint8_t {
    UniformSimilarity = 0,    // Uniform childScale across all 2m^2 sites (artistic overlap permitted).
    ConformalMetric = 1,      // Modulates childScale by (sqrt(2)/(sqrt(2)-sin(v)))^conformalScaleExponent.
    StrictLocalClearance = 2, // Fits each child within the exact 4-fiber Euclidean clearance radius at site (k,l,n).
};

// Authored exact-circle geometry, in dimensionless mathematical z-up space.
// Camera, tessellation, observed depth and GPU handles belong downstream.
// Uniform child scaling permits artistic overlap; StrictLocalClearance bounds
// each child by its 4 bounding Villarceau circles.
struct Recipe {
    std::uint32_t version = kRecipeVersion;
    std::uint32_t fibersPerFamily = 3U;
    std::uint32_t logicalDepth = 2U; // Root is level zero.
    PlacementMode placementMode = PlacementMode::UniformSimilarity;
    PhaseMotion plus{0.0, 0.1};
    PhaseMotion minus{0.0, -0.1};
    double childScale = 0.1;
    double childTwistRadians = 0.0;
    double childNormalOffset = 0.0;
    double fiberRadius = 0.02;
    // Optional stereographic metric modulation exponent gamma >= 0.
    // When > 0 (or in ConformalMetric mode), child scale is modulated by (sqrt(2)/(sqrt(2) - sin(v)))^gamma.
    double conformalScaleExponent = 0.0;
    // Optional over/under normal braid amplitude along outward torus normal.
    // When 0, fibers are exact Euclidean circles of radius sqrt(2).
    double weaveAmplitude = 0.0;

    [[nodiscard]] bool operator==(const Recipe&) const noexcept = default;
};

enum class RecipeIssue : std::uint8_t {
    None,
    UnsupportedVersion,
    FiberCountOutOfRange,
    DepthOutOfRange,
    NonFiniteParameter,
    InvalidContraction,
    InvalidFiberRadius,
    UnrepresentableBounds,
    InvalidConformalExponent,
    InvalidWeaveAmplitude,
    InvalidPlacementMode,
};

[[nodiscard]] RecipeIssue validateRecipe(const Recipe& recipe) noexcept;
[[nodiscard]] std::string_view describe(RecipeIssue issue) noexcept;
[[nodiscard]] std::string_view describePlacementMode(PlacementMode mode) noexcept;

} // namespace epi_sculpt::recursive_hopf

