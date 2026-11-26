// Lightweight presentation-policy vocabulary.
//
// These types describe *what* output an authored state requests. They carry no
// math, GPU, renderer, or persistence dependency, so command routing, preset
// tooling, and tests can make the same policy decision without importing a
// heavyweight implementation surface.

#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace epi_sculpt {

// Authored presentation ownership. This is deliberately distinct from the
// mathematical program and from the primary production route: a composite
// presentation can select more than one independently produced contribution.
enum class SculpturePresentationOwnership : std::uint8_t {
    CompactInstances,
    CanonicalFeedback,
    DedicatedWorld,
    ImplicitField,
    UnifiedCompositeField,
};

[[nodiscard]] constexpr bool includesImplicitField(
    SculpturePresentationOwnership ownership) noexcept {
    return ownership == SculpturePresentationOwnership::ImplicitField
        || ownership == SculpturePresentationOwnership::UnifiedCompositeField;
}

[[nodiscard]] constexpr bool includesCompactInstances(
    SculpturePresentationOwnership ownership) noexcept {
    return ownership == SculpturePresentationOwnership::CompactInstances
        || ownership == SculpturePresentationOwnership::UnifiedCompositeField;
}

// Presentation strategy is an observer choice. It never grants a renderer
// permission to change the numerical producer or evaluate feedback per eye.
enum class RenderStrategy : std::uint8_t {
    PointInstances = 0,
    ReliefChamber = 13,
};

// Public cardinality for UI/tests. Do not use it to increment sparse enum IDs;
// use nextRenderStrategy() below for the declared transition order.
inline constexpr std::size_t kRenderStrategyCount = 2U;

enum class RenderStrategyCapability : std::uint8_t {
    GpuNative,
    CpuRecoveryOnly,
};

[[nodiscard]] constexpr RenderStrategyCapability capability(
    RenderStrategy) noexcept {
    return RenderStrategyCapability::GpuNative;
}

[[nodiscard]] constexpr bool isGpuNative(RenderStrategy) noexcept {
    return true;
}

// Serialized/input IDs are intentionally sparse, so cycling must never use
// arithmetic on their underlying values.  Keep the user-visible order here,
// next to the vocabulary every command, preset, and route decision shares.
[[nodiscard]] constexpr RenderStrategy nextRenderStrategy(
    RenderStrategy strategy) noexcept {
    switch (strategy) {
    case RenderStrategy::PointInstances: return RenderStrategy::ReliefChamber;
    case RenderStrategy::ReliefChamber: return RenderStrategy::PointInstances;
    }
    return RenderStrategy::PointInstances;
}

[[nodiscard]] constexpr std::string_view name(RenderStrategy strategy) noexcept {
    switch (strategy) {
    case RenderStrategy::PointInstances: return "PointInstances (Beads)";
    case RenderStrategy::ReliefChamber: return "ReliefChamber (2.5D Epimorphic Relief)";
    }
    return "Unknown";
}

} // namespace epi_sculpt
