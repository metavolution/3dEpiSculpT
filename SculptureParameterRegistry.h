#pragma once

#include "SculptureProgramSchema.h"
#include <string_view>
#include <array>

namespace epi_sculpt {

// -----------------------------------------------------------------------------
// Educational Annotation: Reflective Parameter Meta-Registry
// -----------------------------------------------------------------------------
// Instead of hardcoding serialization, interpolation, and modulation for every
// single mathematical parameter, we map them as pointer-to-members here. 
// This acts as a single authoritative source of truth. Adding a new visual 
// parameter to the program only requires adding it to this array, and the entire
// engine (JSON saves, cinematic splines, audio-reactivity) will instantly support it.
// -----------------------------------------------------------------------------

struct FloatParamMeta {
    std::string_view name;
    float SculptureProgramParams::* member;
    bool isInterpolable;
};

inline constexpr std::array<FloatParamMeta, 31> kFloatParams = {{
    {"patternSpeed", &SculptureProgramParams::patternSpeed, true},
    {"latentW", &SculptureProgramParams::latentW, true},
    {"rotorAlpha", &SculptureProgramParams::rotorAlpha, true},
    {"rotorBeta", &SculptureProgramParams::rotorBeta, true},
    {"rotorTheta4D", &SculptureProgramParams::rotorTheta4D, true},
    {"seedWidth", &SculptureProgramParams::seedWidth, true},
    {"seedAlpha", &SculptureProgramParams::seedAlpha, true},
    {"seedBaseline", &SculptureProgramParams::seedBaseline, true},
    {"seedPulseFrequency", &SculptureProgramParams::seedPulseFrequency, true},
    {"seedPulseAmplitude", &SculptureProgramParams::seedPulseAmplitude, true},
    {"hueRotation", &SculptureProgramParams::hueRotation, true},
    {"hueSpeed", &SculptureProgramParams::hueSpeed, true},
    {"worldScale", &SculptureProgramParams::worldScale, true},
    {"radiusScale", &SculptureProgramParams::radiusScale, true},
    {"reliefContourDensity", &SculptureProgramParams::reliefContourDensity, true},
    {"chromaBreath", &SculptureProgramParams::chromaBreath, true},
    {"lumaDrift", &SculptureProgramParams::lumaDrift, true},
    {"paletteBlend", &SculptureProgramParams::paletteBlend, true},
    {"volumetricDensityScale", &SculptureProgramParams::volumetricDensityScale, true},
    {"fogAnisotropy", &SculptureProgramParams::fogAnisotropy, true},
    {"advectionSpeed", &SculptureProgramParams::advectionSpeed, true},
    {"curlStrength", &SculptureProgramParams::curlStrength, true},
    {"trailDecay", &SculptureProgramParams::trailDecay, true},
    {"kineticViscosity", &SculptureProgramParams::kineticViscosity, true},
    {"attractorStrength", &SculptureProgramParams::attractorStrength, true},
    {"filmThickness", &SculptureProgramParams::filmThickness, true},
    {"filmIOR", &SculptureProgramParams::filmIOR, true},
    {"temporalFeedbackBlend", &SculptureProgramParams::temporalFeedbackBlend, true},
    {"temporalFade", &SculptureProgramParams::temporalFade, true},
    {"temporalFixedDelta", &SculptureProgramParams::temporalFixedDelta, false},
    {"topologyBlend", &SculptureProgramParams::topologyBlend, false}
}};

struct UintParamMeta {
    std::string_view name;
    std::uint32_t SculptureProgramParams::* member;
};

inline constexpr std::array<UintParamMeta, 9> kUintParams = {{
    {"reliefContourStyle", &SculptureProgramParams::reliefContourStyle},
    {"feedbackIterations", &SculptureProgramParams::feedbackIterations},
    {"enable4D", &SculptureProgramParams::enable4D},
    {"enable4DB", &SculptureProgramParams::enable4DB},
    {"outputMode", &SculptureProgramParams::outputMode},
    {"outputModeB", &SculptureProgramParams::outputModeB},
    {"stageMask", &SculptureProgramParams::stageMask},
    {"instanceMeshType", &SculptureProgramParams::instanceMeshType},
    {"attractorMode", &SculptureProgramParams::attractorMode},
}};

} // namespace epi_sculpt
