#pragma once

#include "SculptureProgramSchema.h"
#include "epi/mode/visual_mode.hpp"

#include <optional>
#include <string>
#include <string_view>

namespace epi_sculpt::mode_projection {

/**
 * Compatibility projection between the canonical epi2 VisualMode vocabulary
 * and the subset of shared controls represented by SculptureProgramParams.
 *
 * Architectural rule:
 *   VisualMode is canonical mathematical state when one exists.
 *   SculptureProgramParams is a spatial/live-control projection, never a
 *   lossless serialization of VisualMode.
 *
 * Keeping these functions stateless and product-local prevents the shared epi2
 * core from depending upward on 3D sculpture types and makes the lossy boundary
 * explicit at every call site.
 */

[[nodiscard]] InnerMap innerFunctionToInnerMap(epi::InnerFunction fn) noexcept;
[[nodiscard]] epi::InnerFunction innerMapToInnerFunction(InnerMap map) noexcept;

[[nodiscard]] ReductionMode reductionTypeToReductionMode(epi::ReductionType red) noexcept;
[[nodiscard]] epi::ReductionType reductionModeToReductionType(ReductionMode mode) noexcept;

[[nodiscard]] ColorEffectType colorModeToColorEffect(int colorMode) noexcept;
[[nodiscard]] int colorEffectToColorMode(ColorEffectType effect) noexcept;

[[nodiscard]] SeedType seedTypeFrom2D(int seedType) noexcept;
[[nodiscard]] int seedTypeTo2D(SeedType seed) noexcept;

[[nodiscard]] StageResolutionScale divisorToStageResolution(int div) noexcept;
[[nodiscard]] int stageResolutionToDivisor(StageResolutionScale scale) noexcept;

// Lossy compatibility projection. Canonical production should retain the
// complete VisualMode separately whenever one is available.
void visualModeToSculptureParams(
    const epi::VisualMode& mode,
    SculptureProgramParams& outParams) noexcept;

// Naming and copying a mode may allocate; propagate preparation failures to
// the caller before mathematical acceptance rather than terminating the host.
void sculptureParamsToVisualMode(
    const SculptureProgramParams& params,
    epi::VisualMode& outMode,
    const std::string& name = {});

// Start from exact canonical state and apply only shared controls that differ
// from the original sculpture projection. Root-only fields therefore survive.
[[nodiscard]] epi::VisualMode resolveCanonicalModeWithSculptureOverrides(
    const epi::VisualMode& canonicalMode,
    const SculptureProgramParams& projectedBaseline,
    const SculptureProgramParams& liveParams);

// Root Pattern catalog lifting returns complete canonical state. World-selection
// fallback policy deliberately lives with the world navigator, not this mapper.
[[nodiscard]] std::optional<epi::VisualMode> fetchRootPatternMode(
    std::string_view presetName);

} // namespace epi_sculpt::mode_projection
