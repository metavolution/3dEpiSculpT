#pragma once

#include "SculptureModulationEngine.h"
#include "SculptureModulationCurves.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <optional>
#include <string_view>

namespace epi_sculpt::modulation_inspection {

[[nodiscard]] inline std::string_view name(ModulationRouteError error) noexcept {
    switch (error) {
    case ModulationRouteError::None: return "none";
    case ModulationRouteError::MissingArrow: return "missing-arrow";
    case ModulationRouteError::UnknownSource: return "unknown-source";
    case ModulationRouteError::UnknownTarget: return "unknown-target";
    case ModulationRouteError::NonfiniteScale: return "nonfinite-scale";
    case ModulationRouteError::NonfiniteBias: return "nonfinite-bias";
    case ModulationRouteError::NonfiniteSlew: return "nonfinite-slew";
    case ModulationRouteError::InvalidScale: return "invalid-scale";
    case ModulationRouteError::InvalidBias: return "invalid-bias";
    case ModulationRouteError::InvalidSlew: return "invalid-slew";
    case ModulationRouteError::UnknownCurve: return "unknown-curve";
    case ModulationRouteError::UnknownModifier: return "unknown-modifier";
    }
    return "unknown";
}

// Canonical display names project the real parser IDs; aliases keep one identity.
[[nodiscard]] inline std::string_view name(ModulationSourceId id) noexcept {
    static constexpr std::array names{
        std::string_view{"lfo0"},
        std::string_view{"lfo1"},
        std::string_view{"lfo2"},
        std::string_view{"lfo3"},
        std::string_view{"chaos:x"},
        std::string_view{"chaos:y"},
        std::string_view{"chaos:z"},
        std::string_view{"roseCurve"},
        std::string_view{"spirograph"},
        std::string_view{"lissajous"},
        std::string_view{"hopfOrbit"},
        std::string_view{"breatheLFO"},
        std::string_view{"audio:sub"},
        std::string_view{"audio:bass"},
        std::string_view{"audio:mid"},
        std::string_view{"audio:treble"},
        std::string_view{"audio:flux"},
        std::string_view{"gesture:dist"},
        std::string_view{"gesture:rot"},
        std::string_view{"gesture:vortex"},
        std::string_view{"gesture:leftTrigger"},
        std::string_view{"gesture:rightTrigger"},
        std::string_view{"gesture:leftGrip"},
        std::string_view{"gesture:rightGrip"},
        std::string_view{"kinematics:speed"},
        std::string_view{"kinematics:maxSpeed"},
        std::string_view{"kinematics:energy"},
        std::string_view{"resonance:modal0"},
        std::string_view{"resonance:modal1"},
        std::string_view{"resonance:pressure"},
        std::string_view{"resonance:energy"},
        std::string_view{"resonance:centroid"},
        std::string_view{"resonance:qFactor"},
        std::string_view{"resonance:damping"},
        std::string_view{"kuramotoOrder"},
        std::string_view{"kuramotoPhase"},
        std::string_view{"kuramotoEntrainment"},
        std::string_view{"macro0"},
        std::string_view{"macro1"},
        std::string_view{"macro2"},
        std::string_view{"macro3"},
        std::string_view{"macro:warp"},
        std::string_view{"macro:chaos"},
        std::string_view{"macro:decay"},
        std::string_view{"macro:lift"},
        std::string_view{"psychoFlow"},
        std::string_view{"psychoTension"},
        std::string_view{"psychoAura"},
        std::string_view{"time"},
    };
    static_assert(names.size() == static_cast<std::size_t>(ModulationSourceId::None));
    const auto index = static_cast<std::size_t>(id);
    return index < names.size() ? names[index] : "unknown";
}

[[nodiscard]] inline std::string_view name(ModulationTargetId id) noexcept {
    static constexpr std::array names{
        std::string_view{"preScale"},
        std::string_view{"postScale"},
        std::string_view{"preOffset"},
        std::string_view{"postOffset"},
        std::string_view{"topologyBlend"},
        std::string_view{"latentW"},
        std::string_view{"seedWidth"},
        std::string_view{"seedAlpha"},
        std::string_view{"seedPulseFreq"},
        std::string_view{"seedPulseAmp"},
        std::string_view{"hueRotation"},
        std::string_view{"hueSpeed"},
        std::string_view{"chromaBreath"},
        std::string_view{"lumaDrift"},
        std::string_view{"paletteBlend"},
        std::string_view{"filmThickness"},
        std::string_view{"filmIOR"},
        std::string_view{"volumetricDensity"},
        std::string_view{"fogAnisotropy"},
        std::string_view{"advectionSpeed"},
        std::string_view{"curlStrength"},
        std::string_view{"temporalFeedbackBlend"},
        std::string_view{"temporalFade"},
        std::string_view{"worldScale"},
        std::string_view{"radiusScale"},
        std::string_view{"trailDecay"},
        std::string_view{"kineticViscosity"},
        std::string_view{"rotorAlpha"},
        std::string_view{"rotorBeta"},
        std::string_view{"rotorTheta4D"},
        std::string_view{"patternSpeed"},
    };
    static_assert(names.size() == static_cast<std::size_t>(ModulationTargetId::None));
    const auto index = static_cast<std::size_t>(id);
    return index < names.size() ? names[index] : "unknown";
}

[[nodiscard]] inline std::string_view name(TransferCurve curve) noexcept {
    return modulation_curves::name(curve);
}

// -----------------------------------------------------------------------------
// Educational Annotation: Deterministic Bipolar Probe Sampling
// -----------------------------------------------------------------------------
// To inspect a compiled modulation route's transfer curve and scale/bias range
// without advancing live oscillator clocks or mutating slewed smoothing history,
// we evaluate 9 symmetric bipolar probes across [-1, 1]. Both single-route CLI
// response inspection and multi-route target fan-in analysis reuse this helper.
// -----------------------------------------------------------------------------

inline constexpr std::array<float, 9> kStandardBipolarProbes{
    -1.0f, -0.75f, -0.5f, -0.1f, 0.0f, 0.1f, 0.5f, 0.75f, 1.0f};

struct RouteProbeSample {
    float input = 0.0f;
    float shaped = 0.0f;
    float contribution = 0.0f;
};

struct RouteProbeEvaluation {
    std::array<RouteProbeSample, kStandardBipolarProbes.size()> samples{};
    float minContribution = 0.0f;
    float maxContribution = 0.0f;
    std::optional<float> failedInput;

    [[nodiscard]] constexpr bool ready() const noexcept { return !failedInput.has_value(); }
};

[[nodiscard]] inline RouteProbeEvaluation evaluateRouteProbes(
    const CompiledModulationRoute& route) noexcept {
    RouteProbeEvaluation result;
    float minVal = std::numeric_limits<float>::infinity();
    float maxVal = -std::numeric_limits<float>::infinity();
    for (std::size_t i = 0; i < kStandardBipolarProbes.size(); ++i) {
        const float input = kStandardBipolarProbes[i];
        const float shaped = ModulationOscillator::applyTransferCurve(input, route.transferCurve);
        const float contribution = shaped * route.scale + route.bias;
        result.samples[i] = {input, shaped, contribution};
        if (!std::isfinite(shaped) || !std::isfinite(contribution)) {
            result.failedInput = input;
            result.minContribution = 0.0f;
            result.maxContribution = 0.0f;
            return result;
        }
        minVal = std::min(minVal, contribution);
        maxVal = std::max(maxVal, contribution);
    }
    result.minContribution = minVal == 0.0f ? 0.0f : minVal;
    result.maxContribution = maxVal == 0.0f ? 0.0f : maxVal;
    return result;
}

} // namespace epi_sculpt::modulation_inspection
