#include "SculptureMorphChoreographer.h"
#include "SculpturePresets.h"
#include "SculptureParameterRegistry.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace epi_sculpt {
namespace {

[[nodiscard]] float mix(float a, float b, float t) noexcept {
    return a + (b - a) * t;
}

[[nodiscard]] float mixHue(float a, float b, float t) noexcept {
    float diff = std::fmod(b - a + 0.5f, 1.0f);
    if (diff < 0.0f) diff += 1.0f;
    float res = std::fmod(a + std::clamp(t, 0.0f, 1.0f) * (diff - 0.5f), 1.0f);
    if (res < 0.0f) res += 1.0f;
    return res;
}

[[nodiscard]] ComplexAffine mix(const ComplexAffine& a,
                                const ComplexAffine& b,
                                float t) noexcept {
    const float u = std::clamp(t, 0.0f, 1.0f);
    // Geodesic Polar Interpolation: Preserves scale magnitude and avoids collapse through origin
    const float rA = std::sqrt(a.scaleReal * a.scaleReal + a.scaleImag * a.scaleImag);
    const float rB = std::sqrt(b.scaleReal * b.scaleReal + b.scaleImag * b.scaleImag);
    const float r = rA + (rB - rA) * u;

    const float thetaA = (rA > 1.0e-6f) ? std::atan2(a.scaleImag, a.scaleReal) : 0.0f;
    const float thetaB = (rB > 1.0e-6f) ? std::atan2(b.scaleImag, b.scaleReal) : 0.0f;

    float dTheta = std::fmod(thetaB - thetaA + 3.14159265359f, 2.0f * 3.14159265359f);
    if (dTheta < 0.0f) dTheta += 2.0f * 3.14159265359f;
    dTheta -= 3.14159265359f;

    const float theta = thetaA + u * dTheta;
    const float sReal = (r > 1.0e-6f) ? r * std::cos(theta) : 0.0f;
    const float sImag = (r > 1.0e-6f) ? r * std::sin(theta) : 0.0f;

    return {

        sReal,
        sImag,
        mix(a.offsetReal, b.offsetReal, u),
        mix(a.offsetImag, b.offsetImag, u),
    };
}

[[nodiscard]] float smoothstep(float t) noexcept {
    const float u = std::clamp(t, 0.0f, 1.0f);
    return u * u * (3.0f - 2.0f * u);
}

const std::array<SculpturePreset, 37> kPresets = {{
    {
        .name = "Epimorphic Torus Chamber",
        .description = "Torus SDF, 3D Torus Reduction, tri-planar sinh, repeated arches, orbit-driven color.",
        .params = SculptureProgramParams{
            .pre = {1.50f, 0.08f, 0.0f, 0.0f},
            .post = {0.70f, -0.05f, 0.0f, 0.0f},
            .inner = InnerMap::Sinh,
            .reduction = ReductionMode::Torus,
            .seedType = SeedType::WaveRing,
            .colorEffect = ColorEffectType::DistanceHSV,
            .feedbackIterations = 5,
            .patternSpeed = 0.06f,
            .seedWidth = 0.65f,
            .seedAlpha = 0.38f,
            .seedBaseline = 0.05f,
            .seedPulseFrequency = 0.15f,
            .seedPulseAmplitude = 0.40f,
            .hueRotation = 0.12f,
            .hueSpeed = 0.02f,
            .worldScale = 0.85f,
            .radiusScale = 0.90f,
            .chromaBreath = 0.25f,
            .lumaDrift = 0.18f,
            .paletteBlend = 0.0f,
        },
        .routingMap = {
            "audioBass -> preScale : 0.4",
            "audioMid -> hueRotation : 1.5",
            "audioFlux -> pulseAmplitude : 0.6",
            "latentLFO -> latentW : 0.5"
        }
    },
    {
        .name = "Audio-Kinetic Voxel Advection",
        .description = "Fully fluid volumetric SDF advection driven by chaotic kinetic curl fields and audio transients.",
        .params = SculptureProgramParams{
            .pre = {1.20f, 0.05f, 0.0f, 0.0f},
            .post = {1.0f, 0.0f, 0.0f, 0.0f},
            .inner = InnerMap::CliffordHopf,
            .reduction = ReductionMode::Grid,
            .seedType = SeedType::Hopf,
            .colorEffect = ColorEffectType::KineticVelocity,
            .feedbackIterations = 3,
            .patternSpeed = 0.15f,
            .outputMode = 1U,
            .outputModeB = 1U,
            .seedWidth = 1.0f,
            .seedAlpha = 0.8f,
            .seedBaseline = 0.1f,
            .hueRotation = 0.6f,
            .hueSpeed = 0.05f,
            .worldScale = 1.0f,
            .radiusScale = 0.8f,
            .volumetricDensityScale = 1.5f,
            .advectionSpeed = 0.05f,
            .curlStrength = 0.0f,
            .trailDecay = 0.95f,
            .kineticViscosity = 0.1f
        },
        .routingMap = {
            "audioBass -> advectionSpeed : 1.25",
            "audioFlux -> curlStrength : 2.50",
            "audioTreble -> hueRotation : 1.0",
            "latentLFO -> latentW : 0.8"
        }
    },
    {
        .name = "Fourth-Axis Cathedral",
        .description = "4D rotation changes a 3D slice over time, causing rooms and sculptures to reconnect.",
        .params = SculptureProgramParams{
            .pre = {1.20f, 0.04f, 0.0f, 0.0f},
            .post = {0.82f, 0.0f, 0.0f, 0.0f},
            .inner = InnerMap::Clifford,
            .reduction = ReductionMode::Grid,
            .seedType = SeedType::Border,
            .colorEffect = ColorEffectType::Vaporwave,
            .feedbackIterations = 4,
            .patternSpeed = 0.045f,
            .enable4D = 1U,
            .seedWidth = 0.85f,
            .seedAlpha = 0.45f,
            .seedBaseline = 0.15f,
            .seedPulseFrequency = 0.18f,
            .seedPulseAmplitude = 0.35f,
            .hueRotation = 0.45f,
            .hueSpeed = 0.04f,
            .worldScale = 1.0f,
            .radiusScale = 0.80f,
            .chromaBreath = 0.40f,
            .lumaDrift = 0.30f,
            .paletteBlend = 0.8f,
        },
        .routingMap = {
            "audioBass -> latentW : 1.0",
            "audioFlux -> patternSpeed : 0.3",
            "audioMid -> postScale : 0.4",
            "latentLFO -> latentW : 0.6"
        }
    },
    {
        .name = "AiryDiffraction",
        .description = "Optical Airy disk diffraction patterns driving chaotic waves",
        .params = SculptureProgramParams{
            .pre = {1.05f, 0.02f, 0.0f, 0.0f},
            .post = {0.90f, 0.0f, 0.0f, 0.0f},
            .inner = InnerMap::Sinh,
            .reduction = ReductionMode::Grid,
            .seedType = SeedType::Airy,
            .colorEffect = ColorEffectType::StandardHSV,
            .feedbackIterations = 4,
            .patternSpeed = 0.05f,
            .seedWidth = 0.90f,
            .seedAlpha = 0.35f,
            .seedBaseline = 0.12f,
            .seedPulseFrequency = 0.15f,
            .seedPulseAmplitude = 0.25f,
            .hueRotation = 0.45f,
            .hueSpeed = 0.03f,
            .worldScale = 1.0f,
            .radiusScale = 0.85f,
            .chromaBreath = 0.20f,
            .lumaDrift = 0.15f,
            .paletteBlend = 0.0f,
        },
        .routingMap = {
            "audioMid -> preScale : 0.45",
            "audioHigh -> hueRotation : 1.8",
            "audioFlux -> pulseFrequency : 0.3"
        }
    },
    {
        .name = "LissajousGlitch",
        .description = "Lissajous chaotic attractor with spatial chromatic glitch effects",
        .params = SculptureProgramParams{
            .pre = {1.20f, 0.05f, 0.0f, 0.0f},
            .post = {0.85f, -0.05f, 0.0f, 0.0f},
            .inner = InnerMap::Lissajous,
            .reduction = ReductionMode::Torus,
            .seedType = SeedType::Lissajous,
            .colorEffect = ColorEffectType::Glitch,
            .feedbackIterations = 3,
            .patternSpeed = 0.07f,
            .seedWidth = 0.85f,
            .seedAlpha = 0.45f,
            .seedBaseline = 0.12f,
            .seedPulseFrequency = 0.25f,
            .seedPulseAmplitude = 0.30f,
            .hueRotation = 0.55f,
            .hueSpeed = 0.04f,
            .worldScale = 0.90f,
            .radiusScale = 0.85f,
            .chromaBreath = 0.60f,
            .lumaDrift = 0.45f,
            .paletteBlend = 1.0f,
        },
        .routingMap = {
            "audioFlux -> topologyBlend : 0.6",
            "audioBass -> radiusScale : 0.35",
            "audioTreble -> hueRotation : 2.5"
        }
    },
    {
        .name = "Vaporwave",
        .description = "Aesthetic grid expansion with neon cyan/magenta mapping",
        .params = SculptureProgramParams{
            .pre = {1.05f, 0.0f, 0.0f, 0.0f},
            .post = {1.0f, 0.0f, 0.0f, 0.0f},
            .inner = InnerMap::Drop,
            .reduction = ReductionMode::Grid,
            .seedType = SeedType::Border,
            .colorEffect = ColorEffectType::Vaporwave,
            .feedbackIterations = 3,
            .patternSpeed = 0.06f,
            .seedWidth = 0.8f,
            .seedAlpha = 0.35f,
            .seedBaseline = 0.05f,
            .seedPulseFrequency = 0.2f,
            .seedPulseAmplitude = 0.3f,
            .hueRotation = 0.0f,
            .hueSpeed = 0.0f,
            .worldScale = 0.9f,
            .radiusScale = 0.9f,
            .chromaBreath = 0.30f,
            .lumaDrift = 0.25f,
            .paletteBlend = 0.0f,
        },
        .routingMap = {
            "audioBass -> preScale : 0.3",
            "audioMid -> hueRotation : 0.8",
            "latentLFO -> latentW : 0.4"
        }
    },
    {
        .name = "GoldenRatio",
        .description = "Divine proportion attractor with golden filament coloring",
        .params = SculptureProgramParams{
            .pre = {0.98f, 0.05f, 0.02f, -0.01f},
            .post = {0.85f, -0.02f, 0.0f, 0.0f},
            .inner = InnerMap::Henon,
            .reduction = ReductionMode::Torus,
            .seedType = SeedType::Attractor,
            .colorEffect = ColorEffectType::GoldenRatio,
            .feedbackIterations = 4,
            .patternSpeed = 0.04f,
            .seedWidth = 0.6f,
            .seedAlpha = 0.28f,
            .seedBaseline = 0.08f,
            .seedPulseFrequency = 0.15f,
            .seedPulseAmplitude = 0.4f,
            .hueRotation = 0.1f,
            .hueSpeed = 0.02f,
            .worldScale = 1.0f,
            .radiusScale = 0.95f,
            .chromaBreath = 0.22f,
            .lumaDrift = 0.12f,
            .paletteBlend = 0.618f,
        },
        .routingMap = {
            "audioBass -> preOffset : 0.05",
            "audioFlux -> pulseAmplitude : 0.6",
            "hueLFO -> hueRotation : 0.5"
        }
    },
    {
        .name = "Hyperbolic Poincaré Dodecahedral Chamber",
        .description = "Non-Euclidean Poincaré conformal inversion and Klein projective geodesic folding with Hopf fibration seeds.",
        .params = SculptureProgramParams{
            .pre = {1.15f, 0.06f, 0.0f, 0.0f},
            .post = {0.80f, -0.02f, 0.0f, 0.0f},
            .inner = InnerMap::PoincareHyperbolic,
            .innerB = InnerMap::KleinProjective,
            .reduction = ReductionMode::Torus,
            .seedType = SeedType::Hopf,
            .seedTypeB = SeedType::HyperSphere4D,
            .colorEffect = ColorEffectType::GoldenRatio,
            .feedbackIterations = 4,
            .patternSpeed = 0.055f,
            .latentW = 0.20f,
            .topologyBlend = 0.35f,
            .enable4D = 1U,
            .seedWidth = 0.70f,
            .seedAlpha = 0.38f,
            .seedBaseline = 0.05f,
            .seedPulseFrequency = 0.16f,
            .seedPulseAmplitude = 0.40f,
            .hueRotation = 0.12f,
            .hueSpeed = 0.025f,
            .worldScale = 1.05f,
            .radiusScale = 0.95f,
            .chromaBreath = 0.35f,
            .lumaDrift = 0.20f,
            .paletteBlend = 0.4f,
        },
        .routingMap = {
            "audioMid -> preScale : 0.25",
            "audioTreble -> topologyBlend : 0.3",
            "audioFlux -> pulseAmplitude : 0.5",
            "orbitLFO -> latentW : 0.35"
        }
    },
    {
        .name = "Volumetric Henyey-Greenstein Nebula",
        .description = "Continuous participating media radiative transfer with forward Henyey-Greenstein Mie scattering and variable-resolution fluid dynamics.",
        .params = SculptureProgramParams{
            .pre = {1.20f, 0.05f, 0.0f, 0.0f},
            .post = {0.75f, -0.04f, 0.0f, 0.0f},
            .inner = InnerMap::Sinh,
            .innerB = InnerMap::Tanh,
            .reduction = ReductionMode::Torus,
            .seedType = SeedType::WaveRing,
            .seedTypeB = SeedType::Hopf,
            .colorEffect = ColorEffectType::ThermalHologram,
            .feedbackIterations = 3,
            .patternSpeed = 0.045f,
            .latentW = 0.30f,
            .topologyBlend = 0.40f,
            .enable4D = 0U,
            .outputMode = 1,
            .outputModeB = 1,
            .seedWidth = 0.85f,
            .seedAlpha = 0.45f,
            .seedBaseline = 0.08f,
            .seedPulseFrequency = 0.12f,
            .seedPulseAmplitude = 0.35f,
            .hueRotation = 0.45f,
            .hueSpeed = 0.015f,
            .worldScale = 1.0f,
            .radiusScale = 1.0f,
            .chromaBreath = 0.40f,
            .lumaDrift = 0.25f,
            .paletteBlend = 0.5f,
            .volumetricDensityScale = 0.85f,
            .temporalFeedbackBlend = 0.55f,
        },
        .routingMap = {
            "audioBass -> volumetricDensityScale : 0.45",
            "audioMid -> chromaBreath : 0.40",
            "latentLFO -> latentW : 0.50"
        }
    },
    {
        .name = "Calabi-Yau Quintic Knot Manifold",
        .description = "Algebraic Fermat quintic threefold cross-sections in CP4 with (p, q) torus knot streamlines, 4D isoclinic rotation, and golden ratio palette dynamics.",
        .params = SculptureProgramParams{
            .pre = {1.35f, 0.08f, 0.0f, 0.0f},
            .post = {0.82f, -0.03f, 0.0f, 0.0f},
            .inner = InnerMap::CalabiYauQuintic,
            .innerB = InnerMap::OctonionCayley,
            .reduction = ReductionMode::Torus,
            .seedType = SeedType::TorusKnotPQ,
            .seedTypeB = SeedType::CrossPolytope4D,
            .colorEffect = ColorEffectType::GoldenRatio,
            .feedbackIterations = 4,
            .patternSpeed = 0.06f,
            .latentW = 0.40f,
            .topologyBlend = 0.45f,
            .enable4D = 1U,
            .seedWidth = 0.75f,
            .seedAlpha = 0.40f,
            .seedBaseline = 0.06f,
            .seedPulseFrequency = 0.15f,
            .seedPulseAmplitude = 0.42f,
            .hueRotation = 0.618f,
            .hueSpeed = 0.02f,
            .worldScale = 1.10f,
            .radiusScale = 0.92f,
            .chromaBreath = 0.50f,
            .lumaDrift = 0.35f,
            .paletteBlend = 0.6f,
            .volumetricDensityScale = 0.70f,
            .temporalFeedbackBlend = 0.40f,
        },
        .routingMap = {
            "audioTreble -> preScale : 0.35",
            "audioBass -> latentW : 0.60",
            "audioFlux -> chromaBreath : 0.45",
            "latentLFO -> topologyBlend : 0.50"
        }
    },
    {
        .name = "Kinetic Flow Streamline Nexus",
        .description = "Analytical 3D curl vector field and phase-gradient advection with kinetic viscosity damping and flow trails.",
        .params = SculptureProgramParams{
            .pre = {1.28f, 0.07f, 0.0f, 0.0f},
            .post = {0.78f, -0.03f, 0.0f, 0.0f},
            .inner = InnerMap::Sinh,
            .innerB = InnerMap::Clifford,
            .reduction = ReductionMode::Torus,
            .seedType = SeedType::TorusKnotPQ,
            .seedTypeB = SeedType::Hopf,
            .colorEffect = ColorEffectType::DistanceHSV,
            .feedbackIterations = 3,
            .patternSpeed = 0.065f,
            .latentW = 0.35f,
            .topologyBlend = 0.30f,
            .enable4D = 0U,
            .outputMode = 0,
            .outputModeB = 0,
            .seedWidth = 0.80f,
            .seedAlpha = 0.42f,
            .seedBaseline = 0.06f,
            .seedPulseFrequency = 0.18f,
            .seedPulseAmplitude = 0.38f,
            .hueRotation = 0.33f,
            .hueSpeed = 0.03f,
            .worldScale = 1.05f,
            .radiusScale = 0.95f,
            .chromaBreath = 0.45f,
            .lumaDrift = 0.25f,
            .paletteBlend = 0.5f,
            .volumetricDensityScale = 0.60f,
            .advectionSpeed = 0.65f,
            .curlStrength = 0.50f,
            .trailDecay = 0.92f,
            .kineticViscosity = 0.15f,
            .temporalFeedbackBlend = 0.60f,
        },
        .routingMap = {
            "audioFlux -> advectionSpeed : 0.40",
            "audioBass -> curlStrength : 0.35",
            "audioMid -> chromaBreath : 0.45",
            "latentLFO -> latentW : 0.40"
        }
    },
    {
        .name = "Topological Homotopy Continuum",
        .description = "Algebraic homotopy regularization between non-Euclidean Poincaré disk and Calabi-Yau quintic manifolds with phase-coherent singularity smoothing.",
        .params = SculptureProgramParams{
            .pre = {1.22f, 0.06f, 0.0f, 0.0f},
            .post = {0.80f, -0.02f, 0.0f, 0.0f},
            .inner = InnerMap::PoincareHyperbolic,
            .innerB = InnerMap::CalabiYauQuintic,
            .reduction = ReductionMode::Torus,
            .seedType = SeedType::Hopf,
            .seedTypeB = SeedType::TorusKnotPQ,
            .colorEffect = ColorEffectType::GoldenRatio,
            .feedbackIterations = 4,
            .patternSpeed = 0.055f,
            .latentW = 0.30f,
            .topologyBlend = 0.50f,
            .enable4D = 1U,
            .outputMode = 0,
            .outputModeB = 0,
            .seedWidth = 0.75f,
            .seedAlpha = 0.40f,
            .seedBaseline = 0.06f,
            .seedPulseFrequency = 0.16f,
            .seedPulseAmplitude = 0.40f,
            .hueRotation = 0.50f,
            .hueSpeed = 0.025f,
            .worldScale = 1.05f,
            .radiusScale = 0.94f,
            .chromaBreath = 0.45f,
            .lumaDrift = 0.30f,
            .paletteBlend = 0.55f,
            .volumetricDensityScale = 0.75f,
            .advectionSpeed = 0.55f,
            .curlStrength = 0.45f,
            .trailDecay = 0.94f,
            .kineticViscosity = 0.18f,
            .temporalFeedbackBlend = 0.50f,
        },
        .routingMap = {
            "audioMid -> topologyBlend : 0.45",
            "audioFlux -> advectionSpeed : 0.40",
            "audioBass -> latentW : 0.50",
            "latentLFO -> preScale : 0.30"
        }
    },
    {
        .name = "Acoustic Cymatic Chladni Resonator",
        .description = "3D Helmholtz acoustic standing-wave modal field with Chladni nodal trapping and audio-reactive dynamic advection.",
        .params = SculptureProgramParams{
            .pre = {1.35f, 0.075f, 0.0f, 0.0f},
            .post = {0.75f, -0.04f, 0.0f, 0.0f},
            .inner = InnerMap::Sinh,
            .innerB = InnerMap::Gyroid,
            .reduction = ReductionMode::Torus,
            .seedType = SeedType::ChladniCymatics,
            .seedTypeB = SeedType::WaveRing,
            .colorEffect = ColorEffectType::DistanceHSV,
            .feedbackIterations = 3,
            .patternSpeed = 0.07f,
            .latentW = 0.40f,
            .topologyBlend = 0.35f,
            .enable4D = 0U,
            .outputMode = 0,
            .outputModeB = 0,
            .seedWidth = 0.85f,
            .seedAlpha = 0.45f,
            .seedBaseline = 0.05f,
            .seedPulseFrequency = 0.22f,
            .seedPulseAmplitude = 0.45f,
            .hueRotation = 0.15f,
            .hueSpeed = 0.04f,
            .worldScale = 1.08f,
            .radiusScale = 0.96f,
            .chromaBreath = 0.50f,
            .lumaDrift = 0.32f,
            .paletteBlend = 0.60f,
            .volumetricDensityScale = 0.80f,
            .advectionSpeed = 0.70f,
            .curlStrength = 0.55f,
            .trailDecay = 0.95f,
            .kineticViscosity = 0.12f,
            .temporalFeedbackBlend = 0.65f,
        },
        .routingMap = {
            "audioBass -> seedPulseAmplitude : 0.50",
            "audioMid -> seedWidth : 0.40",
            "audioTreble -> curlStrength : 0.45",
            "audioFlux -> advectionSpeed : 0.55"
        }
    },
    {
        .name = "Clifford Hopf Fibration Continuum",
        .description = "4D Clifford flat torus S^3 -> S^2 Hopf fibration with Villarceau circular fiber streamlines and SO(4) rotation.",
        .params = SculptureProgramParams{
            .pre = {1.20f, 0.06f, 0.0f, 0.0f},
            .post = {0.80f, -0.05f, 0.0f, 0.0f},
            .inner = InnerMap::CliffordHopf,
            .innerB = InnerMap::Sinh,
            .reduction = ReductionMode::Torus,
            .seedType = SeedType::Hopf,
            .seedTypeB = SeedType::TorusKnotPQ,
            .colorEffect = ColorEffectType::HopfFibration,
            .feedbackIterations = 3,
            .patternSpeed = 0.065f,
            .latentW = 0.50f,
            .topologyBlend = 0.40f,
            .enable4D = 1U,
            .outputMode = 0,
            .outputModeB = 0,
            .seedWidth = 0.70f,
            .seedAlpha = 0.50f,
            .seedBaseline = 0.08f,
            .seedPulseFrequency = 0.18f,
            .seedPulseAmplitude = 0.40f,
            .hueRotation = 0.30f,
            .hueSpeed = 0.035f,
            .worldScale = 1.15f,
            .radiusScale = 1.00f,
            .chromaBreath = 0.45f,
            .lumaDrift = 0.28f,
            .paletteBlend = 0.65f,
            .volumetricDensityScale = 0.85f,
            .advectionSpeed = 0.60f,
            .curlStrength = 0.50f,
            .trailDecay = 0.94f,
            .kineticViscosity = 0.15f,
            .temporalFeedbackBlend = 0.60f,
        },
        .routingMap = {
            "audioMid -> topologyBlend : 0.45",
            "audioBass -> latentW : 0.55",
            "audioTreble -> curlStrength : 0.40",
            "audioFlux -> advectionSpeed : 0.50"
        }
    },
    {
        .name = "Nonlinear Chaos Nexus",
        .description = "Lorenz and Aizawa non-linear strange attractor dynamics with kinetic flow advection and thermal hologram palette.",
        .params = SculptureProgramParams{
            .pre = {1.10f, 0.05f, 0.0f, 0.0f},
            .post = {0.95f, -0.02f, 0.0f, 0.0f},
            .inner = InnerMap::LorenzAttractor,
            .innerB = InnerMap::AizawaAttractor,
            .reduction = ReductionMode::Torus,
            .seedType = SeedType::Attractor,
            .seedTypeB = SeedType::TorusKnotPQ,
            .colorEffect = ColorEffectType::ThermalHologram,
            .feedbackIterations = 4,
            .patternSpeed = 0.055f,
            .latentW = 0.42f,
            .topologyBlend = 0.35f,
            .enable4D = 1U,
            .outputMode = 0,
            .outputModeB = 0,
            .seedWidth = 0.65f,
            .seedAlpha = 0.55f,
            .seedBaseline = 0.06f,
            .seedPulseFrequency = 0.22f,
            .seedPulseAmplitude = 0.35f,
            .hueRotation = 0.15f,
            .hueSpeed = 0.028f,
            .worldScale = 1.10f,
            .radiusScale = 1.00f,
            .chromaBreath = 0.50f,
            .lumaDrift = 0.30f,
            .paletteBlend = 0.70f,
            .volumetricDensityScale = 0.75f,
            .advectionSpeed = 0.65f,
            .curlStrength = 0.60f,
            .trailDecay = 0.90f,
            .kineticViscosity = 0.12f,
            .temporalFeedbackBlend = 0.65f,
        },
        .routingMap = {
            "audioBass -> curlStrength : 0.60",
            "audioFlux -> advectionSpeed : 0.65",
            "audioMid -> topologyBlend : 0.40",
            "audioTreble -> chromaBreath : 0.45"
        }
    },
    {
        .name = "Morphogenetic Turing Soliton Continuum",
        .description = "3D Gray-Scott reaction-diffusion kinetics, morphogenetic Turing spots and solitons, dynamic audio coupling.",
        .params = SculptureProgramParams{
            .pre = {1.25f, 0.04f, 0.0f, 0.0f},
            .post = {0.80f, 0.0f, 0.0f, 0.0f},
            .inner = InnerMap::ReactionDiffusionTuring,
            .reduction = ReductionMode::Torus,
            .seedType = SeedType::TuringPerturbation,
            .colorEffect = ColorEffectType::ThermalHologram,
            .feedbackIterations = 5,
            .patternSpeed = 0.05f,
            .seedWidth = 0.85f,
            .seedAlpha = 0.42f,
            .seedBaseline = 0.08f,
            .seedPulseFrequency = 0.16f,
            .seedPulseAmplitude = 0.35f,
            .hueRotation = 0.22f,
            .hueSpeed = 0.025f,
            .worldScale = 0.95f,
            .radiusScale = 0.88f,
            .chromaBreath = 0.30f,
            .lumaDrift = 0.20f,
            .paletteBlend = 0.0f,
            .volumetricDensityScale = 0.75f,
            .advectionSpeed = 0.15f,
            .curlStrength = 0.25f,
            .trailDecay = 0.92f,
            .kineticViscosity = 0.12f,
        },
        .routingMap = {
            "audioBass -> seedPulseAmplitude : 0.5",
            "audioFlux -> advectionSpeed : 0.4",
            "audioMid -> volumetricDensityScale : 0.3",
            "latentLFO -> latentW : 0.4"
        }
    },
    {
        .name = "Iridescent Soap-Film Continuum",
        .description = "Analytical multi-beam thin-film optical wave interference, Poincaré hyperbolic geodesics, and anisotropic flow sheen.",
        .params = SculptureProgramParams{
            .pre = {1.15f, 0.05f, 0.0f, 0.0f},
            .post = {0.88f, 0.0f, 0.0f, 0.0f},
            .inner = InnerMap::PoincareHyperbolic,
            .innerB = InnerMap::ReactionDiffusionTuring,
            .reduction = ReductionMode::Torus,
            .seedType = SeedType::WaveRing,
            .seedTypeB = SeedType::TuringPerturbation,
            .colorEffect = ColorEffectType::ThinFilmIridescence,
            .feedbackIterations = 5,
            .patternSpeed = 0.045f,
            .seedWidth = 0.75f,
            .seedAlpha = 0.48f,
            .seedBaseline = 0.06f,
            .seedPulseFrequency = 0.15f,
            .seedPulseAmplitude = 0.40f,
            .hueRotation = 0.0f,
            .hueSpeed = 0.015f,
            .worldScale = 1.05f,
            .radiusScale = 0.95f,
            .chromaBreath = 0.40f,
            .lumaDrift = 0.25f,
            .paletteBlend = 0.35f,
            .volumetricDensityScale = 0.70f,
            .advectionSpeed = 0.20f,
            .curlStrength = 0.30f,
            .trailDecay = 0.94f,
            .kineticViscosity = 0.10f,
            .filmThickness = 520.0f,
            .filmIOR = 1.42f,
        },
        .routingMap = {
            "audioTreble -> filmThickness : 150.0",
            "audioMid -> seedPulseAmplitude : 0.45",
            "audioFlux -> advectionSpeed : 0.5",
            "latentLFO -> latentW : 0.35"
        }
    },
    {
        .name = "Hyperbolic Mandelbox Citadel",
        .description = "Multi-scale hyperbolic box/sphere fold inner map, 4D quaternionic Julia seed slice, thin-film iridescence, and geometric architectural feedback.",
        .params = SculptureProgramParams{
            .pre = {1.85f, 0.00f, 0.35f, -0.25f},
            .post = {1.00f, 0.00f, 0.00f, 0.00f},
            .inner = InnerMap::HyperbolicMandelboxFold,
            .innerB = InnerMap::QuaternionJulia,
            .reduction = ReductionMode::Torus,
            .seedType = SeedType::JuliaQuaternionicSlice,
            .seedTypeB = SeedType::Border,
            .colorEffect = ColorEffectType::ThinFilmIridescence,
            .feedbackIterations = 5,
            .patternSpeed = 0.05f,
            .latentW = 0.0f,
            .topologyBlend = 0.0f,
            .seedWidth = 0.45f,
            .seedAlpha = 0.88f,
            .seedPulseFrequency = 0.12f,
            .seedPulseAmplitude = 0.35f,
            .hueRotation = 0.18f,
            .hueSpeed = 0.01f,
            .worldScale = 0.95f,
            .radiusScale = 1.00f,
            .chromaBreath = 0.35f,
            .lumaDrift = 0.20f,
            .paletteBlend = 0.40f,
            .volumetricDensityScale = 0.85f,
            .advectionSpeed = 0.15f,
            .curlStrength = 0.25f,
            .trailDecay = 0.95f,
            .kineticViscosity = 0.08f,
            .filmThickness = 480.0f,
            .filmIOR = 1.45f,
        },
        .routingMap = {
            "audioBass -> seedPulseAmplitude : 0.5",
            "audioTreble -> filmThickness : 180.0",
            "audioFlux -> curlStrength : 0.4",
            "latentLFO -> latentW : 0.3"
        }
    },
    {
        .name = "Hyperbolic-Exponential Homotopy Cascade",
        .description = "Conformal geodesic homotopy bridging tensioned hyperbolic sheets (sinh) and multiscale radial starburst cascades (exp) with thin-film iridescence.",
        .params = SculptureProgramParams{
            .pre = {1.15f, 0.05f, 0.02f, -0.01f},
            .post = {0.85f, -0.02f, 0.00f, 0.00f},
            .inner = InnerMap::HyperbolicExponentialHomotopy,
            .innerB = InnerMap::Sinh,
            .reduction = ReductionMode::Torus,
            .seedType = SeedType::Border,
            .seedTypeB = SeedType::WaveRing,
            .colorEffect = ColorEffectType::ThinFilmIridescence,
            .feedbackIterations = 4,
            .patternSpeed = 0.065f,
            .latentW = 0.0f,
            .topologyBlend = 0.0f,
            .seedWidth = 0.65f,
            .seedAlpha = 0.35f,
            .seedPulseFrequency = 0.18f,
            .seedPulseAmplitude = 0.30f,
            .hueRotation = 0.08f,
            .hueSpeed = 0.025f,
            .worldScale = 1.00f,
            .radiusScale = 1.00f,
            .chromaBreath = 0.25f,
            .lumaDrift = 0.15f,
            .paletteBlend = 0.30f,
            .volumetricDensityScale = 0.70f,
            .advectionSpeed = 0.10f,
            .curlStrength = 0.15f,
            .trailDecay = 0.95f,
            .kineticViscosity = 0.10f,
            .filmThickness = 520.0f,
            .filmIOR = 1.48f,
        },
        .routingMap = {
            "audioBass -> seedPulseAmplitude : 0.4",
            "audioMid -> filmThickness : 150.0",
            "audioFlux -> curlStrength : 0.3",
            "latentLFO -> latentW : 0.25"
        }
    },
    {
        .name = "Parametric Rose Curve Spirograph",
        .description = "Multi-track harmonic rose curves and hypotrochoid spirographs driving 4D isoclinic foliations with dynamic thin-film iridescence.",
        .params = SculptureProgramParams{
            .pre = {1.20f, 0.08f, 0.0f, 0.0f},
            .post = {0.80f, -0.05f, 0.0f, 0.0f},
            .inner = InnerMap::HyperbolicExponentialHomotopy,
            .innerB = InnerMap::CliffordHopf,
            .reduction = ReductionMode::Torus,
            .seedType = SeedType::Border,
            .seedTypeB = SeedType::JuliaQuaternionicSlice,
            .colorEffect = ColorEffectType::ThinFilmIridescence,
            .feedbackIterations = 5,
            .patternSpeed = 0.08f,
            .latentW = 0.15f,
            .topologyBlend = 0.35f,
            .seedWidth = 0.70f,
            .seedAlpha = 0.40f,
            .seedPulseFrequency = 0.20f,
            .seedPulseAmplitude = 0.35f,
            .hueRotation = 0.12f,
            .hueSpeed = 0.030f,
            .worldScale = 1.00f,
            .radiusScale = 1.00f,
            .chromaBreath = 0.30f,
            .lumaDrift = 0.20f,
            .paletteBlend = 0.40f,
            .volumetricDensityScale = 0.75f,
            .advectionSpeed = 0.12f,
            .curlStrength = 0.18f,
            .trailDecay = 0.95f,
            .kineticViscosity = 0.08f,
            .filmThickness = 550.0f,
            .filmIOR = 1.52f,
        },
        .routingMap = {
            "roseCurve -> preOffset : 0.20",
            "spirograph -> postOffset : 0.15",
            "hopfOrbit -> latentW : 0.30",
            "breatheLFO -> seedPulseAmplitude : 0.35",
            "audioBass -> seedPulseAmplitude : 0.40",
            "audioMid -> filmThickness : 160.0",
            "audioFlux -> curlStrength : 0.35"
        }
    },
    {
        .name = "Volumetric Anisotropic Media Nebula",
        .description = "Variable-rate thermal fluid convection with Henyey-Greenstein forward anisotropic scattering and multi-track rose curve orbital modulation.",
        .params = SculptureProgramParams{
            .pre = {1.10f, 0.04f, 0.00f, 0.00f},
            .post = {0.90f, -0.02f, 0.00f, 0.00f},
            .inner = InnerMap::HyperbolicExponentialHomotopy,
            .innerB = InnerMap::ReactionDiffusionTuring,
            .reduction = ReductionMode::Torus,
            .seedType = SeedType::Border,
            .seedTypeB = SeedType::TuringPerturbation,
            .colorEffect = ColorEffectType::ThinFilmIridescence,
            .feedbackIterations = 4,
            .patternSpeed = 0.075f,
            .latentW = 0.20f,
            .topologyBlend = 0.30f,
            .outputMode = 1,
            .outputModeB = 1,
            .seedWidth = 0.75f,
            .seedAlpha = 0.45f,
            .seedPulseFrequency = 0.22f,
            .seedPulseAmplitude = 0.40f,
            .hueRotation = 0.15f,
            .hueSpeed = 0.035f,
            .worldScale = 1.00f,
            .radiusScale = 1.05f,
            .chromaBreath = 0.35f,
            .lumaDrift = 0.25f,
            .paletteBlend = 0.45f,
            .volumetricDensityScale = 1.25f,
            .fogAnisotropy = 0.65f,
            .auxiliaryResolutionScale = StageResolutionScale::Half,
            .advectionSpeed = 0.18f,
            .curlStrength = 0.22f,
            .trailDecay = 0.94f,
            .kineticViscosity = 0.07f,
            .filmThickness = 580.0f,
            .filmIOR = 1.54f,
        },
        .routingMap = {
            "roseCurve -> preOffset : 0.25",
            "spirograph -> postOffset : 0.18",
            "hopfOrbit -> latentW : 0.35",
            "breatheLFO -> volumetricDensityScale : 0.40",
            "audioBass -> seedPulseAmplitude : 0.45",
            "audioTreble -> filmThickness : 180.0",
            "audioFlux -> curlStrength : 0.40"
        }
    },
    {
        .name = "Hypercomplex Quaternionic Blossom",
        .description = "4D Quaternion Julia fractal dynamics with hyper-spherical orbit trapping and dynamic Clifford-Hopf isoclinic rotation.",
        .params = SculptureProgramParams{
            .pre = {1.20f, 0.05f, 0.00f, 0.00f},
            .post = {0.85f, -0.04f, 0.00f, 0.00f},
            .inner = InnerMap::QuaternionJulia,
            .innerB = InnerMap::QuaternionMandelbrot,
            .reduction = ReductionMode::Torus,
            .seedType = SeedType::HyperSphere4D,
            .seedTypeB = SeedType::CrossPolytope4D,
            .colorEffect = ColorEffectType::QuaternionCurvature,
            .feedbackIterations = 5,
            .patternSpeed = 0.08f,
            .latentW = 0.35f,
            .topologyBlend = 0.50f,
            .enable4D = 1,
            .outputMode = 1,
            .outputModeB = 1,
            .seedWidth = 0.85f,
            .seedAlpha = 0.50f,
            .seedPulseFrequency = 0.25f,
            .seedPulseAmplitude = 0.45f,
            .hueRotation = 0.20f,
            .hueSpeed = 0.04f,
            .worldScale = 0.90f,
            .radiusScale = 0.95f,
            .chromaBreath = 0.40f,
            .lumaDrift = 0.20f,
            .paletteBlend = 0.50f,
            .volumetricDensityScale = 1.10f,
            .filmThickness = 520.0f,
            .filmIOR = 1.50f,
        },
        .routingMap = {
            "roseCurve -> preOffset : 0.20",
            "hopfOrbit -> latentW : 0.40",
            "audioMid -> hueRotation : 1.2",
            "audioBass -> preScale : 0.35",
            "audioFlux -> seedPulseAmplitude : 0.50"
        }
    },
    {
        .name = "Resonant Acoustic Cymatic Nebula",
        .description = "Multi-band audio spectrum-driven Turing reaction-diffusion with modal cymatics and thin-film wave interference.",
        .params = SculptureProgramParams{
            .pre = {1.30f, 0.06f, 0.00f, 0.00f},
            .post = {0.80f, -0.03f, 0.00f, 0.00f},
            .inner = InnerMap::ReactionDiffusionTuring,
            .innerB = InnerMap::HyperbolicExponentialHomotopy,
            .reduction = ReductionMode::Grid,
            .seedType = SeedType::ChladniCymatics,
            .seedTypeB = SeedType::TuringPerturbation,
            .colorEffect = ColorEffectType::ThinFilmIridescence,
            .feedbackIterations = 5,
            .patternSpeed = 0.10f,
            .latentW = 0.25f,
            .topologyBlend = 0.40f,
            .outputMode = 1,
            .outputModeB = 1,
            .seedWidth = 0.90f,
            .seedAlpha = 0.60f,
            .seedPulseFrequency = 0.30f,
            .seedPulseAmplitude = 0.65f,
            .hueRotation = 0.35f,
            .hueSpeed = 0.06f,
            .worldScale = 0.90f,
            .radiusScale = 1.10f,
            .chromaBreath = 0.45f,
            .lumaDrift = 0.30f,
            .paletteBlend = 0.60f,
            .volumetricDensityScale = 1.40f,
            .advectionSpeed = 0.22f,
            .curlStrength = 0.30f,
            .trailDecay = 0.92f,
            .kineticViscosity = 0.08f,
            .filmThickness = 620.0f,
            .filmIOR = 1.56f,
        },
        .routingMap = {
            "audioBass -> preScale : 0.50",
            "audioSubBass -> worldScale : 0.30",
            "audioMid -> hueRotation : 2.0",
            "audioTreble -> filmThickness : 220.0",
            "audioFlux -> seedPulseAmplitude : 0.60",
            "breatheLFO -> volumetricDensityScale : 0.35"
        }
    },
    {
        .name = "Chromatic Prismatic Birefringence Cathedral",
        .description = "Cauchy physical wavelength dispersion and Fresnel polarization birefringence splitting along 4D Clifford-Hopf isoclinic geodesics with volumetric god-rays.",
        .params = SculptureProgramParams{
            .pre = {1.25f, 0.05f, 0.00f, 0.00f},
            .post = {0.80f, -0.04f, 0.00f, 0.00f},
            .inner = InnerMap::CliffordHopf,
            .innerB = InnerMap::HyperbolicExponentialHomotopy,
            .reduction = ReductionMode::Torus,
            .seedType = SeedType::JuliaQuaternionicSlice,
            .seedTypeB = SeedType::Hopf,
            .colorEffect = ColorEffectType::ChromaticDispersion,
            .feedbackIterations = 5,
            .patternSpeed = 0.08f,
            .latentW = 0.20f,
            .topologyBlend = 0.45f,
            .outputMode = 1,
            .outputModeB = 1,
            .seedWidth = 0.80f,
            .seedAlpha = 0.55f,
            .seedPulseFrequency = 0.24f,
            .seedPulseAmplitude = 0.50f,
            .hueRotation = 0.10f,
            .hueSpeed = 0.035f,
            .worldScale = 0.90f,
            .radiusScale = 1.00f,
            .chromaBreath = 0.40f,
            .lumaDrift = 0.25f,
            .paletteBlend = 0.55f,
            .volumetricDensityScale = 1.30f,
            .filmThickness = 600.0f,
            .filmIOR = 1.58f,
        },
        .routingMap = {
            "roseCurve -> preOffset : 0.22",
            "hopfOrbit -> latentW : 0.35",
            "audioBass -> preScale : 0.40",
            "audioMid -> hueRotation : 1.5",
            "audioTreble -> filmThickness : 240.0",
            "audioFlux -> seedPulseAmplitude : 0.55"
        }
    },
    {
        .name = "8D Octonionic G2 Exceptional Lie Cathedral",
        .description = "Non-associative octonionic dynamics with continuous G2 Lie automorphism rotation, S^7 Hopf hyperspherical foliation, and Cauchy chromatic dispersion.",
        .params = SculptureProgramParams{
            .pre = {1.20f, 0.04f, 0.00f, 0.00f},
            .post = {0.85f, -0.03f, 0.00f, 0.00f},
            .inner = InnerMap::OctonionAssociatorManifold,
            .innerB = InnerMap::OctonionFeedback,
            .reduction = ReductionMode::Torus,
            .seedType = SeedType::OctonionFibrationS7,
            .seedTypeB = SeedType::JuliaQuaternionicSlice,
            .colorEffect = ColorEffectType::ChromaticDispersion,
            .feedbackIterations = 5,
            .patternSpeed = 0.08f,
            .latentW = 0.25f,
            .topologyBlend = 0.40f,
            .enable4D = 1,
            .outputMode = 1,
            .outputModeB = 1,
            .seedWidth = 0.85f,
            .seedAlpha = 0.55f,
            .seedPulseFrequency = 0.25f,
            .seedPulseAmplitude = 0.50f,
            .hueRotation = 0.15f,
            .hueSpeed = 0.04f,
            .worldScale = 0.90f,
            .radiusScale = 1.00f,
            .chromaBreath = 0.40f,
            .lumaDrift = 0.25f,
            .paletteBlend = 0.50f,
            .volumetricDensityScale = 1.35f,
            .advectionSpeed = 0.20f,
            .curlStrength = 0.28f,
            .trailDecay = 0.93f,
            .kineticViscosity = 0.08f,
            .filmThickness = 580.0f,
            .filmIOR = 1.55f,
        },
        .routingMap = {
            "hopfOrbit -> latentW : 0.40",
            "audioMid -> hueRotation : 1.5",
            "audioBass -> preScale : 0.35",
            "audioFlux -> seedPulseAmplitude : 0.50",
            "breatheLFO -> volumetricDensityScale : 0.35"
        }
    },
    {
        .name = "Quantum Holomorphic Potential Singularity",
        .description = "Multipole quantum holomorphic complex potential field with conformal streamlines, Cauchy chromatic dispersion, and resonant cymatics.",
        .params = SculptureProgramParams{
            .pre = {1.10f, 0.03f, 0.00f, 0.00f},
            .post = {0.80f, -0.02f, 0.00f, 0.00f},
            .inner = InnerMap::HolomorphicPotential,
            .innerB = InnerMap::HyperbolicExponentialHomotopy,
            .reduction = ReductionMode::Torus,
            .seedType = SeedType::ChladniCymatics,
            .seedTypeB = SeedType::TuringPerturbation,
            .colorEffect = ColorEffectType::ChromaticDispersion,
            .feedbackIterations = 5,
            .patternSpeed = 0.075f,
            .latentW = 0.22f,
            .topologyBlend = 0.40f,
            .enable4D = 0,
            .outputMode = 0,
            .outputModeB = 0,
            .seedWidth = 0.82f,
            .seedAlpha = 0.52f,
            .seedPulseFrequency = 0.22f,
            .seedPulseAmplitude = 0.48f,
            .hueRotation = 0.28f,
            .hueSpeed = 0.045f,
            .worldScale = 0.85f,
            .radiusScale = 0.85f,
            .chromaBreath = 0.38f,
            .lumaDrift = 0.22f,
            .paletteBlend = 0.50f,
            .volumetricDensityScale = 1.30f,
            .advectionSpeed = 0.18f,
            .curlStrength = 0.26f,
            .trailDecay = 0.94f,
            .kineticViscosity = 0.08f,
            .filmThickness = 610.0f,
            .filmIOR = 1.56f,
        },
        .routingMap = {
            "roseCurve -> preOffset : 0.24",
            "hopfOrbit -> latentW : 0.35",
            "audioBass -> preScale : 0.42",
            "audioMid -> hueRotation : 1.8",
            "audioTreble -> filmThickness : 210.0",
            "audioFlux -> seedPulseAmplitude : 0.55",
            "breatheLFO -> volumetricDensityScale : 0.38"
        }
    },
    {
        .name = "Non-Associative Octonion G2 Associator Vortex",
        .description = "8D Cayley-Dickson non-associative associator manifold [x,y,z]=(xy)z-x(yz) modulated by continuous G2 Lie automorphism rotations, S7 Hopf fibration, and holographic iridescence.",
        .params = SculptureProgramParams{
            .pre = {1.25f, 0.05f, 0.00f, 0.00f},
            .post = {0.88f, -0.04f, 0.00f, 0.00f},
            .inner = InnerMap::OctonionAssociatorManifold,
            .innerB = InnerMap::OctonionFeedback,
            .reduction = ReductionMode::Torus,
            .seedType = SeedType::OctonionFibrationS7,
            .seedTypeB = SeedType::HyperSphere4D,
            .colorEffect = ColorEffectType::QuaternionCurvature,
            .feedbackIterations = 6,
            .patternSpeed = 0.085f,
            .latentW = 0.28f,
            .topologyBlend = 0.45f,
            .enable4D = 1,
            .outputMode = 1,
            .outputModeB = 1,
            .seedWidth = 0.88f,
            .seedAlpha = 0.58f,
            .seedPulseFrequency = 0.24f,
            .seedPulseAmplitude = 0.52f,
            .hueRotation = 0.32f,
            .hueSpeed = 0.05f,
            .worldScale = 0.92f,
            .radiusScale = 0.95f,
            .chromaBreath = 0.42f,
            .lumaDrift = 0.28f,
            .paletteBlend = 0.55f,
            .volumetricDensityScale = 1.40f,
            .advectionSpeed = 0.22f,
            .curlStrength = 0.30f,
            .trailDecay = 0.95f,
            .kineticViscosity = 0.08f,
            .filmThickness = 640.0f,
            .filmIOR = 1.58f,
            .temporalFeedbackBlend = 0.70f,
            .temporalFade = 0.96f,
        },
        .routingMap = {
            "hopfOrbit -> latentW : 0.45",
            "breatheLFO -> preScale : 0.30",
            "audioBass -> seedPulseAmplitude : 0.50",
            "audioMid -> hueRotation : 1.6",
            "audioFlux -> curlStrength : 0.35",
            "audioTreble -> filmThickness : 260.0",
            "breatheLFO -> volumetricDensityScale : 0.40"
        }
    },
    {
        .name = "4D Non-Euclidean Hyperbolic Honeycomb & Seifert Nebula",
        .description = "4D non-Euclidean {5,3,4} hyperbolic Coxeter honeycomb tiling foliation in Poincare ball space H4 coupled with knotted Seifert topological boundary projections and Cauchy chromatic dispersion.",
        .params = SculptureProgramParams{
            .pre = {1.18f, 0.08f, 0.00f, 0.00f},
            .post = {0.92f, -0.06f, 0.00f, 0.00f},
            .inner = InnerMap::HyperbolicHoneycomb,
            .innerB = InnerMap::PoincareHyperbolic,
            .reduction = ReductionMode::Torus,
            .seedType = SeedType::TorusKnotPQ,
            .seedTypeB = SeedType::HyperSphere4D,
            .colorEffect = ColorEffectType::ChromaticDispersion,
            .feedbackIterations = 5,
            .patternSpeed = 0.075f,
            .latentW = 0.35f,
            .topologyBlend = 0.40f,
            .enable4D = 1,
            .outputMode = 1,
            .outputModeB = 1,
            .seedWidth = 0.75f,
            .seedAlpha = 0.62f,
            .seedPulseFrequency = 0.30f,
            .seedPulseAmplitude = 0.48f,
            .hueRotation = 0.45f,
            .hueSpeed = 0.04f,
            .worldScale = 0.94f,
            .radiusScale = 0.90f,
            .chromaBreath = 0.38f,
            .lumaDrift = 0.22f,
            .paletteBlend = 0.60f,
            .volumetricDensityScale = 1.35f,
            .advectionSpeed = 0.18f,
            .curlStrength = 0.28f,
            .trailDecay = 0.94f,
            .kineticViscosity = 0.06f,
            .filmThickness = 580.0f,
            .filmIOR = 1.62f,
            .temporalFeedbackBlend = 0.68f,
            .temporalFade = 0.95f,
        },
        .routingMap = {
            "hopfOrbit -> latentW : 0.40",
            "breatheLFO -> preScale : 0.25",
            "audioBass -> seedPulseAmplitude : 0.55",
            "audioMid -> hueRotation : 1.5",
            "audioFlux -> curlStrength : 0.30",
            "audioTreble -> filmThickness : 240.0",
            "breatheLFO -> volumetricDensityScale : 0.35"
        }
    },
    {
        .name = "120-Cell Hyper-Dodecahedral Quantum Hologram",
        .description = "600-vertex 120-cell regular hyper-dodecahedral 4-polytope foliation on S3 coupled with Octonion G2 Lie automorphisms, Cauchy chromatic dispersion, and fluid kinetic vorticity.",
        .params = SculptureProgramParams{
            .pre = {1.22f, 0.09f, 0.00f, 0.00f},
            .post = {0.88f, -0.07f, 0.00f, 0.00f},
            .inner = InnerMap::OctonionAssociatorManifold,
            .innerB = InnerMap::HyperbolicMandelboxFold,
            .reduction = ReductionMode::Torus,
            .seedType = SeedType::HyperSphere4D,
            .seedTypeB = SeedType::TorusKnotPQ,
            .colorEffect = ColorEffectType::ChromaticDispersion,
            .feedbackIterations = 5,
            .patternSpeed = 0.080f,
            .latentW = 0.38f,
            .topologyBlend = 0.50f,
            .enable4D = 1,
            .outputMode = 1,
            .outputModeB = 1,
            .seedWidth = 0.82f,
            .seedAlpha = 0.65f,
            .seedPulseFrequency = 0.32f,
            .seedPulseAmplitude = 0.55f,
            .hueRotation = 0.52f,
            .hueSpeed = 0.045f,
            .worldScale = 0.95f,
            .radiusScale = 0.92f,
            .chromaBreath = 0.44f,
            .lumaDrift = 0.25f,
            .paletteBlend = 0.65f,
            .volumetricDensityScale = 1.45f,
            .advectionSpeed = 0.24f,
            .curlStrength = 0.32f,
            .trailDecay = 0.96f,
            .kineticViscosity = 0.07f,
            .filmThickness = 680.0f,
            .filmIOR = 1.65f,
            .temporalFeedbackBlend = 0.72f,
            .temporalFade = 0.97f,
        },
        .routingMap = {
            "hopfOrbit -> latentW : 0.50",
            "breatheLFO -> preScale : 0.35",
            "audioBass -> seedPulseAmplitude : 0.60",
            "audioMid -> hueRotation : 1.8",
            "audioFlux -> curlStrength : 0.40",
            "audioTreble -> filmThickness : 280.0",
            "breatheLFO -> volumetricDensityScale : 0.45"
        }
    },
    {
        .name = "Silent Lorenz Lanterns",
        .description = "A self-driven Lorenz attractor garden: torus-bounded recursive lanterns with non-audio LFO motion and gentle temporal memory.",
        .params = SculptureProgramParams{
            .pre = {1.08f, 0.035f, 0.0f, 0.0f},
            .post = {0.72f, -0.028f, 0.0f, 0.0f},
            .inner = InnerMap::LorenzAttractor,
            .innerB = InnerMap::Sinh,
            .reduction = ReductionMode::Torus,
            .seedType = SeedType::Attractor,
            .seedTypeB = SeedType::WaveRing,
            .colorEffect = ColorEffectType::ChromaticDispersion,
            .feedbackIterations = 5,
            .patternSpeed = 0.032f,
            .latentW = 0.18f,
            .topologyBlend = 0.34f,
            .enable4D = 1U,
            .seedWidth = 0.68f,
            .seedAlpha = 0.44f,
            .seedBaseline = 0.07f,
            .seedPulseFrequency = 0.11f,
            .seedPulseAmplitude = 0.30f,
            .hueRotation = 0.58f,
            .hueSpeed = 0.018f,
            .worldScale = 0.88f,
            .radiusScale = 0.76f,
            .chromaBreath = 0.28f,
            .lumaDrift = 0.16f,
            .paletteBlend = 0.42f,
            .advectionSpeed = 0.08f,
            .curlStrength = 0.14f,
            .trailDecay = 0.93f,
            .kineticViscosity = 0.10f,
            .filmThickness = 420.0f,
            .filmIOR = 1.48f,
            .temporalFeedbackBlend = 0.58f,
            .temporalFade = 0.94f,
        },
        .routingMap = {
            "latentLFO -> latentW : 0.32",
            "breatheLFO -> seedPulseAmplitude : 0.18",
            "breatheLFO -> hueRotation : 0.12",
            "latentLFO -> curlStrength : 0.08"
        }
    },
    
    {
        .name = "Bioluminescent Floral Phyllotaxis",
        .description = "Deep ocean bioluminescence pulsing over a spherical Fibonacci golden lattice, driven by Clifford Hopf homotopy and Aizawa chaotic attractors.",
        .params = SculptureProgramParams{
            .pre = {1.00f, 0.05f, 0.0f, 0.0f},
            .post = {1.00f, -0.05f, 0.0f, 0.0f},
            .inner = InnerMap::CliffordHopf,
            .innerB = InnerMap::AizawaAttractor,
            .reduction = ReductionMode::Torus,
            .seedType = SeedType::FibonacciPhyllotaxis,
            .seedTypeB = SeedType::FibonacciPhyllotaxis,
            .colorEffect = ColorEffectType::Bioluminescence,
            .feedbackIterations = 5,
            .patternSpeed = 0.12f,
            .latentW = 0.25f,
            .topologyBlend = 0.35f,
            .outputMode = 1,
            .outputModeB = 1,
            .seedWidth = 0.70f,
            .seedAlpha = 0.60f,
            .seedPulseFrequency = 0.25f,
            .seedPulseAmplitude = 0.50f,
            .hueRotation = 0.0f,
            .hueSpeed = 0.05f,
            .worldScale = 1.0f,
            .radiusScale = 1.05f,
            .chromaBreath = 0.40f,
            .lumaDrift = 0.20f,
            .volumetricDensityScale = 1.25f,
            .advectionSpeed = 0.10f,
            .curlStrength = 0.20f,
        },
        .routingMap = {
            "latentLFO -> latentW : 0.25",
            "audioHigh -> topologyBlend : 0.20"
        }
    },
    {
        .name = "Ammann-Beenker Quasicrystal Field",
        .description = "Generative 8-fold aperiodic tilings exploring non-Euclidean quasi-lattices via iterative domain reduction.",
        .params = SculptureProgramParams{
            .pre = {1.15f, 0.05f, 0.0f, 0.0f},
            .post = {1.05f, -0.05f, 0.0f, 0.0f},
            .inner = InnerMap::Identity,
            .innerB = InnerMap::Sinh,
            .reduction = ReductionMode::AmmannBeenker,
            .seedType = SeedType::WaveRing,
            .seedTypeB = SeedType::Border,
            .colorEffect = ColorEffectType::DistanceHSV,
            .feedbackIterations = 6,
            .patternSpeed = 0.10f,
            .latentW = 0.5f,
            .topologyBlend = 0.0f,
            .outputMode = 1,
            .outputModeB = 1,
            .seedWidth = 0.5f,
            .seedAlpha = 0.8f,
            .seedPulseFrequency = 0.3f,
            .seedPulseAmplitude = 0.6f,
            .hueRotation = 0.1f,
            .hueSpeed = 0.02f,
            .worldScale = 1.0f,
            .radiusScale = 1.0f,
            .chromaBreath = 0.5f,
            .lumaDrift = 0.2f,
            .volumetricDensityScale = 1.5f,
            .advectionSpeed = 0.1f,
            .curlStrength = 0.2f,
        },
        .routingMap = {
            "latentLFO -> latentW : 0.4",
            "audioBass -> scaleAmplitude : 1.2",
            "audioMid -> hueRotation : 0.8"
        }
    },
    {
        .name = "Jacobi Elliptic Soliton Lattice",
        .description = "Doubly periodic Jacobi sn/cn elliptic modulus manifold with Gothic Pareidolia Cathedral fold and dielectric thin-film iridescence.",
        .params = SculptureProgramParams{
            .pre = {1.25f, 0.06f, 0.0f, 0.0f},
            .post = {0.85f, -0.04f, 0.0f, 0.0f},
            .inner = InnerMap::JacobiEllipticSnCn,
            .innerB = InnerMap::ChebyshevPolynomial,
            .reduction = ReductionMode::PareidoliaCathedral,
            .seedType = SeedType::FibonacciPhyllotaxis,
            .seedTypeB = SeedType::ChladniCymatics,
            .colorEffect = ColorEffectType::ThinFilmIridescence,
            .feedbackIterations = 6,
            .patternSpeed = 0.08f,
            .latentW = 0.42f,
            .topologyBlend = 0.35f,
            .outputMode = 1,
            .outputModeB = 1,
            .seedWidth = 0.65f,
            .seedAlpha = 0.75f,
            .seedPulseFrequency = 0.28f,
            .seedPulseAmplitude = 0.52f,
            .hueRotation = 0.32f,
            .hueSpeed = 0.03f,
            .worldScale = 1.0f,
            .radiusScale = 0.95f,
            .chromaBreath = 0.42f,
            .lumaDrift = 0.24f,
            .volumetricDensityScale = 1.40f,
            .advectionSpeed = 0.14f,
            .curlStrength = 0.24f,
            .filmThickness = 480.0f,
            .filmIOR = 1.55f,
        },
        .routingMap = {
            "latentLFO -> latentW : 0.35",
            "audioBass -> preScale : 0.30",
            "audioTreble -> filmThickness : 220.0"
        }
    },
    {
        .name = "Bioluminescent Hydro-Dynamo",
        .description = "Enzymatic oceanic luciferin emission pulses driven by quantum harmonic oscillator wave packets and kinetic RK2 vortex advection.",
        .params = SculptureProgramParams{
            .pre = {1.12f, 0.04f, 0.0f, 0.0f},
            .post = {0.95f, -0.05f, 0.0f, 0.0f},
            .inner = InnerMap::QuantumWave,
            .innerB = InnerMap::KuramotoLattice,
            .reduction = ReductionMode::Torus,
            .seedType = SeedType::TuringPerturbation,
            .seedTypeB = SeedType::WaveRing,
            .colorEffect = ColorEffectType::Bioluminescence,
            .feedbackIterations = 5,
            .patternSpeed = 0.09f,
            .latentW = 0.25f,
            .topologyBlend = 0.50f,
            .outputMode = 1,
            .outputModeB = 1,
            .seedWidth = 0.70f,
            .seedAlpha = 0.70f,
            .seedPulseFrequency = 0.35f,
            .seedPulseAmplitude = 0.60f,
            .hueRotation = 0.50f,
            .hueSpeed = 0.02f,
            .worldScale = 0.98f,
            .radiusScale = 1.05f,
            .chromaBreath = 0.50f,
            .lumaDrift = 0.28f,
            .volumetricDensityScale = 1.60f,
            .advectionSpeed = 0.22f,
            .curlStrength = 0.38f,
            .trailDecay = 0.92f,
            .kineticViscosity = 0.08f,
        },
        .routingMap = {
            "breatheLFO -> volumetricDensityScale : 0.45",
            "audioBass -> curlStrength : 0.35",
            "audioMid -> hueRotation : 0.8"
        }
    },
    {
        .name = "Epimorphic Poincare Hyper-Relief",
        .description = "Conformal hyperbolic disc projection mapping 2D feedback manifolds into 2.5D stereo relief chamber with Pareidolia Cathedral reduction.",
        .params = SculptureProgramParams{
            .pre = {1.25f, 0.05f, 0.00f, 0.00f},
            .post = {0.80f, -0.02f, 0.00f, 0.00f},
            .inner = InnerMap::HyperbolicMandelboxFold,
            .innerB = InnerMap::PoincareHyperbolic,
            .reduction = ReductionMode::PareidoliaCathedral,
            .seedType = SeedType::HyperSphere4D,
            .seedTypeB = SeedType::Hopf,
            .colorEffect = ColorEffectType::QuaternionCurvature,
            .feedbackIterations = 6,
            .patternSpeed = 0.065f,
            .latentW = 0.707f,
            .topologyBlend = 0.50f,
            .outputMode = 0,
            .outputModeB = 0,
            .seedWidth = 0.80f,
            .seedAlpha = 0.90f,
            .seedPulseFrequency = 0.18f,
            .seedPulseAmplitude = 0.42f,
            .hueRotation = 0.618f,
            .hueSpeed = 0.012f,
            .worldScale = 1.15f,
            .radiusScale = 0.85f,
            .chromaBreath = 0.35f,
            .lumaDrift = 0.25f,
            .volumetricDensityScale = 1.25f,
            .fogAnisotropy = 0.45f,
            .filmThickness = 580.0f,
            .filmIOR = 1.54f,
            .temporalFeedbackBlend = 0.40f,
            .temporalFade = 0.990f,
        },
        .routingMap = {
            "poincareOrbit -> latentW : 0.45",
            "audioBass -> preScale : 0.35",
            "audioMid -> hueRotation : 1.2",
            "audioFlux -> seedPulseAmplitude : 0.50"
        }
    },
    {
        .name = "Calabi-Yau Non-Euclidean Geodesic Flow",
        .description = "3.5D Calabi-Yau quintic variety intersected with hyperbolic exponential homotopy under Ammann-Beenker quasicrystal reduction.",
        .params = SculptureProgramParams{
            .pre = {1.30f, 0.03f, 0.00f, 0.00f},
            .post = {0.75f, -0.04f, 0.00f, 0.00f},
            .inner = InnerMap::CalabiYauQuintic,
            .innerB = InnerMap::HyperbolicExponentialHomotopy,
            .reduction = ReductionMode::AmmannBeenker,
            .seedType = SeedType::HyperSphere4D,
            .seedTypeB = SeedType::FibonacciPhyllotaxis,
            .colorEffect = ColorEffectType::QuaternionCurvature,
            .feedbackIterations = 7,
            .patternSpeed = 0.055f,
            .latentW = 0.850f,
            .topologyBlend = 0.40f,
            .outputMode = 0,
            .outputModeB = 0,
            .seedWidth = 0.70f,
            .seedAlpha = 0.88f,
            .seedPulseFrequency = 0.20f,
            .seedPulseAmplitude = 0.45f,
            .hueRotation = 0.750f,
            .hueSpeed = 0.010f,
            .worldScale = 1.25f,
            .radiusScale = 0.75f,
            .chromaBreath = 0.38f,
            .lumaDrift = 0.28f,
            .volumetricDensityScale = 1.35f,
            .fogAnisotropy = 0.55f,
            .filmThickness = 620.0f,
            .filmIOR = 1.56f,
            .temporalFeedbackBlend = 0.38f,
            .temporalFade = 0.993f,
        },
        .routingMap = {
            "calabiFlow -> preOffset : 0.30",
            "audioBass -> worldScale : 0.25",
            "audioTreble -> filmThickness : 200.0",
            "latentLFO -> latentW : 0.40"
        }
    },
    {
        .name = "Kuramoto Hyperbolic Sync Soliton",
        .description = "Coupled nonlinear Kuramoto oscillator network with non-associative octonionic associator foliation in non-Euclidean hyperbolic space.",
        .params = SculptureProgramParams{
            .pre = {1.15f, 0.06f, 0.00f, 0.00f},
            .post = {0.90f, -0.01f, 0.00f, 0.00f},
            .inner = InnerMap::KuramotoLattice,
            .innerB = InnerMap::OctonionAssociatorManifold,
            .reduction = ReductionMode::Torus,
            .seedType = SeedType::ChladniCymatics,
            .seedTypeB = SeedType::Hopf,
            .colorEffect = ColorEffectType::PhenomenologicalPalette,
            .feedbackIterations = 6,
            .patternSpeed = 0.080f,
            .latentW = 0.650f,
            .topologyBlend = 0.55f,
            .outputMode = 0,
            .outputModeB = 0,
            .seedWidth = 0.85f,
            .seedAlpha = 0.92f,
            .seedPulseFrequency = 0.25f,
            .seedPulseAmplitude = 0.50f,
            .hueRotation = 0.333f,
            .hueSpeed = 0.018f,
            .worldScale = 1.10f,
            .radiusScale = 0.90f,
            .chromaBreath = 0.42f,
            .lumaDrift = 0.32f,
            .volumetricDensityScale = 1.45f,
            .fogAnisotropy = 0.60f,
            .filmThickness = 540.0f,
            .filmIOR = 1.52f,
            .temporalFeedbackBlend = 0.45f,
            .temporalFade = 0.988f,
        },
        .routingMap = {
            "syncLattice -> seedWidth : 0.40",
            "audioBass -> seedPulseAmplitude : 0.60",
            "audioMid -> hueRotation : 1.4",
            "breatheLFO -> chromaBreath : 0.30"
        }
    }
}};

} // namespace

std::span<const SculpturePreset> sculpturePresets() noexcept {
    static std::vector<SculpturePreset> s_allPresets;
    if (s_allPresets.empty()) {
        s_allPresets.assign(kPresets.begin(), kPresets.end());

        auto makePreset = [](const char* name, const char* desc, std::uint8_t meshType,
                             float speed, float wScale, float rScale,
                             std::uint32_t attractor = 0, float attractorStr = 0.0f,
                             float latentW = 0.0f, float rotorTheta = 0.0f) {
            SculptureProgramParams p{};
            p.pre = {1.0f, 0.0f, 0.0f, 0.0f};
            p.post = {1.0f, 0.0f, 0.0f, 0.0f};
            p.instanceMeshType = meshType;
            p.outputMode = 1;
            p.patternSpeed = speed;
            p.worldScale = wScale;
            p.radiusScale = rScale;
            p.feedbackIterations = 2;
            p.attractorMode = attractor;
            p.attractorStrength = attractorStr;
            p.latentW = latentW;
            p.rotorTheta4D = rotorTheta;
            p.seedWidth = 0.35f;
            p.seedAlpha = 0.85f;
            p.temporalFeedbackBlend = 0.92f;
            p.temporalFade = 0.98f;
            return SculpturePreset{
                .name = name,
                .description = desc,
                .params = p,
                .routingMap = {}
            };
        };

        s_allPresets.push_back(makePreset(
            "kuramoto_synchronization_manifold",
            "OpenCL Kuramoto 16K Lattice (GPU Zero-Copy)",
            1, 0.08f, 1.0f, 1.0f));
        s_allPresets.push_back(makePreset(
            "quantum_wave_interference",
            "Quantum Wave / Kronig-Penney Lattice (GPU Zero-Copy)",
            0, 0.05f, 1.0f, 1.0f));
        s_allPresets.push_back(makePreset(
            "hopf_fibration_drift",
            "Hopf Fibration Topological Drift (GPU Zero-Copy)",
            1, 0.04f, 1.0f, 1.0f, 0, 0.0f, 0.0f, 0.25f));
        s_allPresets.push_back(makePreset(
            "exp_golden_phyllotaxis_cathedral",
            "Golden Phyllotaxis Cathedral",
            3, 0.06f, 1.0f, 1.0f));
        s_allPresets.push_back(makePreset(
            "120-Cell Hyper-Dodecahedral Quantum Hologram",
            "120-Cell Quantum Hologram",
            2, 0.05f, 1.0f, 1.0f, 0, 0.0f, 0.0f, 0.35f));
        s_allPresets.push_back(makePreset(
            "rossler_lantern_garden",
            "Rössler Attractor Lanterns",
            0, 0.05f, 1.0f, 1.0f, 2, 1.0f, 0.5f));
    }
    return s_allPresets;
}

SculptureProgramParams blendProgramParams(const SculptureProgramParams& source,
                                          const SculptureProgramParams& target,
                                          float amount) noexcept {
    const float t = std::clamp(amount, 0.0f, 1.0f);
    SculptureProgramParams result{};
    result.pre = mix(source.pre, target.pre, t);
    result.post = mix(source.post, target.post, t);
    
    // Topology Blending (Dual Slot crossfade)
    result.inner = source.inner;
    result.innerB = target.inner;
    result.seedType = source.seedType;
    result.seedTypeB = target.seedType;
    result.outputMode = source.outputMode;
    result.outputModeB = target.outputMode;
    result.reduction = source.reduction;
    result.reductionB = target.reduction;
    result.colorEffect = t < 0.5f ? source.colorEffect : target.colorEffect;
    
    for (const auto& meta : kUintParams) {
        result.*meta.member = t < 0.5f ? source.*meta.member : target.*meta.member;
    }

    // Dual-slot crossfade assignment for discrete geometric modes
    if (t < 0.5f) {
        result.outputMode = source.outputMode;
        result.outputModeB = target.outputMode;
        result.enable4D = source.enable4D;
        result.enable4DB = target.enable4D;
    } else {
        result.outputMode = target.outputMode;
        result.outputModeB = source.outputMode;
        result.enable4D = target.enable4D;
        result.enable4DB = source.enable4D;
    }
    
    for (const auto& meta : kFloatParams) {
        if (meta.isInterpolable) {
            result.*meta.member = mix(source.*meta.member, target.*meta.member, t);
        } else {
            result.*meta.member = t < 0.5f ? source.*meta.member : target.*meta.member;
        }
    }
    
    // Explicit override for the structural crossfader parameter and circular hue interpolation
    result.topologyBlend = t;
    result.hueRotation = mixHue(source.hueRotation, target.hueRotation, t);
    for (std::size_t i = 0; i < 4; ++i) {
        result.audioSpectrum[i] = mix(source.audioSpectrum[i], target.audioSpectrum[i], t);
    }

    return result;
}

SculptureWorldState blendWorldState(const SculptureWorldState& source,
                                    const SculptureWorldState& target,
                                    float amount) noexcept {
    const float t = std::clamp(amount, 0.0f, 1.0f);
    SculptureWorldState result{};
    result.version = source.version;
    result.time = mix(source.time, target.time, t);
    result.morph = mix(source.morph, target.morph, t);
    result.cameraAngle = mix(source.cameraAngle, target.cameraAngle, t);
    return result;
}

SculptureProgramParams splineProgramParams(
    const SculptureProgramParams& p0,
    const SculptureProgramParams& p1,
    const SculptureProgramParams& p2,
    const SculptureProgramParams& p3,
    float t) noexcept {
    return SculptureMorphChoreographer::interpolateParamsSpline(p0, p1, p2, p3, t);
}

SculptureWorldState splineWorldState(
    const SculptureWorldState& s0,
    const SculptureWorldState& s1,
    const SculptureWorldState& s2,
    const SculptureWorldState& s3,
    float t) noexcept {
    const float u = std::clamp(t, 0.0f, 1.0f);
    SculptureWorldState result{};
    result.version = s1.version;
    result.time = SculptureMorphChoreographer::evaluateCatmullRomSpline(s0.time, s1.time, s2.time, s3.time, u, CatmullRomType::Centripetal);
    result.morph = std::clamp(SculptureMorphChoreographer::evaluateCatmullRomSpline(s0.morph, s1.morph, s2.morph, s3.morph, u, CatmullRomType::Centripetal), 0.0f, 1.0f);
    
    // Geodesic shortest-arc camera angle unwrapping for smooth spline rotation
    const float angle0 = s0.cameraAngle;
    const float angle1 = s1.cameraAngle;
    const float angle2 = angle1 + SculptureMorphChoreographer::shortestAngleDeltaRad(angle1, s2.cameraAngle);
    const float angle3 = angle2 + SculptureMorphChoreographer::shortestAngleDeltaRad(s2.cameraAngle, s3.cameraAngle);
    result.cameraAngle = SculptureMorphChoreographer::evaluateCatmullRomSpline(angle0, angle1, angle2, angle3, u, CatmullRomType::Centripetal);
    return result;
}

// SculptureSequencer Implementation
// -----------------------------------------------------------------------------
void SculptureSequencer::addKeyframe(const SculptureProgramParams& params,
                                     const SculptureWorldState& state,
                                     float duration,
                                     KeyframeEasing easing,
                                     std::vector<std::string> routingMap) noexcept {
    keyframes_.push_back({params, state, std::max(duration, kMinimumKeyframeDuration), easing, std::move(routingMap)});
    if (keyframes_.size() == 1) {
        currentParams_ = params;
        currentState_ = state;
        currentRoutingMap_ = keyframes_.front().routingMap;
    }
}

void SculptureSequencer::addKeyframe(std::size_t presetIndex,
                                     float duration,
                                     KeyframeEasing easing) noexcept {
    if (presetIndex < kPresets.size()) {
        SculptureWorldState state{};
        state.time = static_cast<float>(presetIndex) * 5.0f;
        addKeyframe(kPresets[presetIndex].params, state, duration, easing, kPresets[presetIndex].routingMap);
    }
}

void SculptureSequencer::clear() noexcept {
    keyframes_.clear();
    currentIndex_ = 0;
    elapsedInCurrentFrame_ = 0.0f;
    timelineTime_ = 0.0f;
    playing_ = false;
    currentRoutingMap_.clear();
}

void SculptureSequencer::setLooping(bool loop) noexcept {
    loop_ = loop;
}

void SculptureSequencer::play(bool loop) noexcept {
    if (keyframes_.empty()) return;
    playing_ = true;
    loop_ = loop;
}

SculptureSequencer SculptureSequencer::makeReplacementCandidate() const noexcept {
    SculptureSequencer candidate;
    // Only policy crosses this boundary. A first admitted keyframe supplies
    // fresh samples; copying the old timeline just to clear it wastes host work.
    candidate.loop_ = loop_;
    return candidate;
}

void SculptureSequencer::resume() noexcept {
    if (!keyframes_.empty()) playing_ = true;
}

void SculptureSequencer::pause() noexcept {
    playing_ = false;
}

float SculptureSequencer::totalDuration() const noexcept {
    float total = 0.0f;
    for (const auto& kf : keyframes_) {
        total += kf.durationSeconds;
    }
    return total;
}

float SculptureSequencer::currentTimelineTime() const noexcept {
    return timelineTime_;
}

std::size_t SculptureSequencer::keyframeCount() const noexcept {
    return keyframes_.size();
}

std::size_t SculptureSequencer::currentIndex() const noexcept {
    return currentIndex_;
}

const std::vector<std::string>& SculptureSequencer::currentRoutingMap() const noexcept {
    return currentRoutingMap_;
}

void SculptureSequencer::seek(float timelineSeconds) noexcept {
    if (keyframes_.empty() || !std::isfinite(timelineSeconds)) return;
    const float total = totalDuration();
    if (!std::isfinite(total) || total <= 0.0f) return;

    float t = std::max(timelineSeconds, 0.0f);
    if (loop_) {
        t = std::fmod(t, total);
    } else {
        t = std::min(t, total);
    }

    timelineTime_ = t;
    float accumulated = 0.0f;
    currentIndex_ = 0;
    for (std::size_t i = 0; i < keyframes_.size(); ++i) {
        if (accumulated + keyframes_[i].durationSeconds > t || i == keyframes_.size() - 1) {
            currentIndex_ = i;
            elapsedInCurrentFrame_ = t - accumulated;
            break;
        }
        accumulated += keyframes_[i].durationSeconds;
    }
    sampleCurrentPosition();
}

bool SculptureSequencer::seekKeyframe(std::size_t index) noexcept {
    if (index >= keyframes_.size()) return false;
    float start = 0.0f;
    float total = 0.0f;
    // Authored imports already validate this domain. Programmatic timelines can
    // still contain bad/rounded-away durations: reject before changing phase.
    for (std::size_t i = 0; i < keyframes_.size(); ++i) {
        if (i == index) start = total;
        const float duration = keyframes_[i].durationSeconds;
        const float next = total + duration;
        if (!std::isfinite(duration) || duration <= 0.0f
            || !std::isfinite(next) || next <= total) return false;
        total = next;
    }
    currentIndex_ = index;
    elapsedInCurrentFrame_ = 0.0f;
    timelineTime_ = start;
    sampleCurrentPosition();
    return true;
}

bool SculptureSequencer::stepKeyframe(int direction) noexcept {
    if (keyframes_.empty() || direction == 0) return false;
    std::size_t index = currentIndex_;
    if (direction > 0) {
        if (index < keyframes_.size() - 1) ++index;
        else if (loop_) index = 0;
        else return false;
    } else {
        if (index > 0) --index;
        else if (loop_) index = keyframes_.size() - 1;
        else return false;
    }
    return seekKeyframe(index);
}

void SculptureSequencer::update(float deltaSeconds) noexcept {
    if (!playing_ || keyframes_.empty()) return;
    if (!std::isfinite(deltaSeconds) || deltaSeconds < 0.0f) return;

    float nextElapsed = elapsedInCurrentFrame_ + deltaSeconds;
    float nextTimeline = timelineTime_ + deltaSeconds;
    if (!std::isfinite(nextElapsed) || !std::isfinite(nextTimeline)) return;
    if (loop_ && nextElapsed >= keyframes_[currentIndex_].durationSeconds) {
        const float cycleDuration = totalDuration();
        if (!std::isfinite(cycleDuration) || cycleDuration <= 0.0f) return;
        // There are no transition callbacks here: full cycles change only phase.
        // Fold them before walking intervals so a long delta cannot require an
        // unbounded number of cycle traversals. Resolve global phase just once.
        nextElapsed = std::fmod(nextElapsed, cycleDuration);
        nextTimeline = std::fmod(nextTimeline, cycleDuration);
    }
    elapsedInCurrentFrame_ = nextElapsed;
    timelineTime_ = nextTimeline;

    while (elapsedInCurrentFrame_ >= keyframes_[currentIndex_].durationSeconds) {
        // Segment lengths may differ; changing index also changes the duration
        // to consume at the next iteration.
        elapsedInCurrentFrame_ -= keyframes_[currentIndex_].durationSeconds;
        currentIndex_++;
        
        if (currentIndex_ >= keyframes_.size()) {
            if (loop_) {
                currentIndex_ = 0;
            } else {
                currentIndex_ = keyframes_.size() - 1;
                elapsedInCurrentFrame_ = keyframes_.back().durationSeconds;
                timelineTime_ = totalDuration();
                playing_ = false;
                break;
            }
        }
    }

    sampleCurrentPosition();
}

void SculptureSequencer::sampleCurrentPosition() noexcept {
    if (keyframes_.empty()) return;
    // Seek and playback share one observation path; a paused seek must not need
    // a synthetic update or a temporary playback takeover to refresh its values.
    // The final nonlooping keyframe is an authored hold with no return edge.
    const bool terminalHold = !loop_ && currentIndex_ == keyframes_.size() - 1;
    if (keyframes_.size() > 1 && !terminalHold) {
        const std::size_t n = keyframes_.size();
        const std::size_t i1 = currentIndex_;
        const std::size_t i2 = (i1 + 1) % n;
        
        const std::size_t i0 = (i1 == 0) ? (loop_ ? n - 1 : 0) : i1 - 1;
        std::size_t i3 = (i2 + 1) % n;
        if (!loop_ && i2 == n - 1) {
            i3 = i2;
        }
        
        const float linearT = std::clamp(elapsedInCurrentFrame_ / keyframes_[i1].durationSeconds, 0.0f, 1.0f);
        float t = linearT;

        const auto easing = keyframes_[i1].easing;
        switch (easing) {
            case KeyframeEasing::Linear:
                currentParams_ = blendProgramParams(keyframes_[i1].params, keyframes_[i2].params, linearT);
                currentState_ = blendWorldState(keyframes_[i1].state, keyframes_[i2].state, linearT);
                break;
            case KeyframeEasing::Smoothstep:
                t = smoothstep(linearT);
                currentParams_ = blendProgramParams(keyframes_[i1].params, keyframes_[i2].params, t);
                currentState_ = blendWorldState(keyframes_[i1].state, keyframes_[i2].state, t);
                break;
            case KeyframeEasing::Exponential:
                t = (std::exp(2.0f * linearT) - 1.0f) / (std::exp(2.0f) - 1.0f);
                currentParams_ = blendProgramParams(keyframes_[i1].params, keyframes_[i2].params, t);
                currentState_ = blendWorldState(keyframes_[i1].state, keyframes_[i2].state, t);
                break;
            case KeyframeEasing::CatmullRom:
            case KeyframeEasing::CubicHermite:
            default:
                currentParams_ = splineProgramParams(
                    keyframes_[i0].params,
                    keyframes_[i1].params,
                    keyframes_[i2].params,
                    keyframes_[i3].params,
                    linearT
                );
                currentState_ = splineWorldState(
                    keyframes_[i0].state,
                    keyframes_[i1].state,
                    keyframes_[i2].state,
                    keyframes_[i3].state,
                    linearT
                );
                break;
        }

        // Active routing map matches source or target based on segment progress
        currentRoutingMap_ = (linearT < 0.5f) ? keyframes_[i1].routingMap : keyframes_[i2].routingMap;
    } else {
        currentParams_ = keyframes_[currentIndex_].params;
        currentState_ = keyframes_[currentIndex_].state;
        currentRoutingMap_ = keyframes_[currentIndex_].routingMap;
    }
}

const SculptureProgramParams& SculptureSequencer::currentParams() const noexcept {
    return currentParams_;
}

const SculptureWorldState& SculptureSequencer::currentState() const noexcept {
    return currentState_;
}

bool SculptureSequencer::active() const noexcept {
    return playing_;
}

// -----------------------------------------------------------------------------
// SculptureLFO Implementation
// -----------------------------------------------------------------------------
float SculptureLFO::evaluate(float time) const noexcept {
    const float t = time * freq_ + phase_;
    
    switch (shape_) {
        case LFOShape::Sine:
            return amp_ * std::sin(2.0f * 3.14159265359f * t);
        case LFOShape::Triangle: {
            const float mod = std::fmod(t, 1.0f);
            return amp_ * (4.0f * std::abs(mod - 0.5f) - 1.0f);
        }
        case LFOShape::Square:
            return amp_ * (std::fmod(t, 1.0f) < 0.5f ? 1.0f : -1.0f);
    }
    return 0.0f;
}

// -----------------------------------------------------------------------------
// SculptureGuidedTour Implementation
// -----------------------------------------------------------------------------
void SculptureGuidedTour::start() noexcept {
    active_ = true;
    dwellRemaining_ = dwellTimeSeconds_;
}

void SculptureGuidedTour::stop() noexcept {
    active_ = false;
}

void SculptureGuidedTour::toggle() noexcept {
    if (active_) {
        stop();
    } else {
        start();
    }
}

void SculptureGuidedTour::update(float deltaSeconds, SculptureMorphChoreographer& morph) noexcept {
    if (!active_ || kPresets.empty()) {
        return;
    }

    if (morph.isActive()) {
        return;
    }

    dwellRemaining_ -= std::max(deltaSeconds, 0.0f);
    if (dwellRemaining_ <= 0.0f) {
        dwellRemaining_ = dwellTimeSeconds_;
        currentPresetIndex_ = (currentPresetIndex_ + 1) % kPresets.size();

        SculptureProgramParams targetParams = kPresets[currentPresetIndex_].params;
        targetParams = mutator_.mutate(targetParams, 0.12f);
        morph.setTarget(targetParams, morphDurationSeconds_);
    }
}

// -----------------------------------------------------------------------------
// SculptureBarycentricBlender Implementation
// -----------------------------------------------------------------------------
SculptureProgramParams SculptureBarycentricBlender::blend(
    std::span<const WeightedPreset> weightedPresets
) noexcept {
    if (weightedPresets.empty()) {
        return {};
    }
    if (weightedPresets.size() == 1) {
        return weightedPresets.front().params;
    }

    float totalWeight = 0.0f;
    float maxWeight = -1.0f;
    std::size_t dominantIndex = 0;

    for (std::size_t i = 0; i < weightedPresets.size(); ++i) {
        const float w = std::max(weightedPresets[i].weight, 0.0f);
        totalWeight += w;
        if (w > maxWeight) {
            maxWeight = w;
            dominantIndex = i;
        }
    }

    if (totalWeight <= 1e-6f) {
        return weightedPresets[dominantIndex].params;
    }

    // Initialize with dominant discrete structural topology
    SculptureProgramParams result = weightedPresets[dominantIndex].params;

    // Reset continuous accumulators
    result.pre = {0.0f, 0.0f, 0.0f, 0.0f};
    result.post = {0.0f, 0.0f, 0.0f, 0.0f};
    result.topologyBlend = 0.0f;
    result.audioSpectrum.fill(0.0f);

    for (const auto& meta : kFloatParams) {
        if (meta.isInterpolable) {
            result.*meta.member = 0.0f;
        }
    }

    for (const auto& wp : weightedPresets) {
        const float nw = std::max(wp.weight, 0.0f) / totalWeight;
        const auto& p = wp.params;

        result.pre.scaleReal += p.pre.scaleReal * nw;
        result.pre.scaleImag += p.pre.scaleImag * nw;
        result.pre.offsetReal += p.pre.offsetReal * nw;
        result.pre.offsetImag += p.pre.offsetImag * nw;

        result.post.scaleReal += p.post.scaleReal * nw;
        result.post.scaleImag += p.post.scaleImag * nw;
        result.post.offsetReal += p.post.offsetReal * nw;
        result.post.offsetImag += p.post.offsetImag * nw;

        for (const auto& meta : kFloatParams) {
            if (meta.isInterpolable) {
                result.*meta.member += (p.*meta.member) * nw;
            }
        }

        result.topologyBlend += p.topologyBlend * nw;

        for (std::size_t s = 0; s < 4; ++s) {
            result.audioSpectrum[s] += p.audioSpectrum[s] * nw;
        }
    }

    return result;
}

// -----------------------------------------------------------------------------
// SculptureMultiTrackChoreographer Implementation
// -----------------------------------------------------------------------------
SculptureMultiTrackChoreographer::SculptureMultiTrackChoreographer() = default;

void SculptureMultiTrackChoreographer::setTrackKeyframes(
    TrackLayer layer, std::vector<SequenceKeyframe> keyframes
) noexcept {
    const std::size_t idx = static_cast<std::size_t>(layer);
    if (idx >= tracks_.size()) return;

    tracks_[idx].clear();
    for (const auto& kf : keyframes) {
        tracks_[idx].addKeyframe(kf.params, kf.state, kf.durationSeconds, kf.easing, kf.routingMap);
    }
}

void SculptureMultiTrackChoreographer::setTrackSpeed(TrackLayer layer, float speedMultiplier) noexcept {
    const std::size_t idx = static_cast<std::size_t>(layer);
    if (idx < speeds_.size()) {
        speeds_[idx] = std::max(speedMultiplier, 0.0f);
    }
}

void SculptureMultiTrackChoreographer::setTrackLoop(TrackLayer layer, bool loop) noexcept {
    const std::size_t idx = static_cast<std::size_t>(layer);
    if (idx < tracks_.size()) {
        tracks_[idx].play(loop);
    }
}

void SculptureMultiTrackChoreographer::play() noexcept {
    playing_ = true;
    for (auto& track : tracks_) {
        track.play(true);
    }
}

void SculptureMultiTrackChoreographer::pause() noexcept {
    playing_ = false;
    for (auto& track : tracks_) {
        track.pause();
    }
}

void SculptureMultiTrackChoreographer::setTrackMuted(TrackLayer layer, bool muted) noexcept {
    const std::size_t idx = static_cast<std::size_t>(layer);
    if (idx < muted_.size()) {
        muted_[idx] = muted;
    }
}

bool SculptureMultiTrackChoreographer::isTrackMuted(TrackLayer layer) const noexcept {
    const std::size_t idx = static_cast<std::size_t>(layer);
    return idx < muted_.size() ? muted_[idx] : false;
}

void SculptureMultiTrackChoreographer::setTrackSolo(TrackLayer layer, bool solo) noexcept {
    const std::size_t idx = static_cast<std::size_t>(layer);
    if (idx < solo_.size()) {
        solo_[idx] = solo;
    }
}

bool SculptureMultiTrackChoreographer::isTrackSolo(TrackLayer layer) const noexcept {
    const std::size_t idx = static_cast<std::size_t>(layer);
    return idx < solo_.size() ? solo_[idx] : false;
}

void SculptureMultiTrackChoreographer::clearSolos() noexcept {
    solo_.fill(false);
}

void SculptureMultiTrackChoreographer::setTrackLFO(TrackLayer layer, LFOShape shape, float freq, float amp, float phase) noexcept {
    const std::size_t idx = static_cast<std::size_t>(layer);
    if (idx < lfos_.size()) {
        lfos_[idx].emplace(shape, freq, amp, phase);
    }
}

void SculptureMultiTrackChoreographer::clearTrackLFO(TrackLayer layer) noexcept {
    const std::size_t idx = static_cast<std::size_t>(layer);
    if (idx < lfos_.size()) {
        lfos_[idx].reset();
    }
}

bool SculptureMultiTrackChoreographer::isLayerActive(TrackLayer layer) const noexcept {
    const std::size_t idx = static_cast<std::size_t>(layer);
    if (idx >= tracks_.size()) return false;
    const bool anySolo = std::any_of(solo_.begin(), solo_.end(), [](bool s){ return s; });
    if (anySolo && !solo_[idx]) return false;
    if (muted_[idx]) return false;
    return tracks_[idx].active();
}

const SculptureSequencer& SculptureMultiTrackChoreographer::track(TrackLayer layer) const noexcept {
    const std::size_t idx = std::min(static_cast<std::size_t>(layer), tracks_.size() - 1U);
    return tracks_[idx];
}

SculptureSequencer& SculptureMultiTrackChoreographer::track(TrackLayer layer) noexcept {
    const std::size_t idx = std::min(static_cast<std::size_t>(layer), tracks_.size() - 1U);
    return tracks_[idx];
}

void SculptureMultiTrackChoreographer::update(float deltaSeconds) noexcept {
    if (!playing_) return;
    globalTimelineTime_ += deltaSeconds;
    for (std::size_t i = 0; i < tracks_.size(); ++i) {
        tracks_[i].update(deltaSeconds * speeds_[i]);
    }
}

void SculptureMultiTrackChoreographer::seek(float timelineSeconds) noexcept {
    globalTimelineTime_ = timelineSeconds;
    for (auto& track : tracks_) {
        track.seek(timelineSeconds);
    }
}

float SculptureMultiTrackChoreographer::trackTime(TrackLayer layer) const noexcept {
    const std::size_t idx = static_cast<std::size_t>(layer);
    if (idx < tracks_.size()) {
        return tracks_[idx].currentTimelineTime();
    }
    return 0.0f;
}

SculptureProgramParams SculptureMultiTrackChoreographer::composeProgramParams(
    const SculptureProgramParams& baseParams
) const noexcept {
    SculptureProgramParams out = baseParams;

    const bool anySolo = std::any_of(solo_.begin(), solo_.end(), [](bool s){ return s; });

    // Track 0: Geometry Layer
    if ((!anySolo || solo_[0]) && !muted_[0] && tracks_[0].active()) {
        const auto& geom = tracks_[0].currentParams();
        out.inner = geom.inner;
        out.innerB = geom.innerB;
        out.reduction = geom.reduction;
        out.seedType = geom.seedType;
        out.seedTypeB = geom.seedTypeB;
        out.pre = geom.pre;
        out.post = geom.post;
        out.worldScale = geom.worldScale;
        out.radiusScale = geom.radiusScale;

        if (lfos_[0].has_value()) {
            const float lfoVal = lfos_[0]->evaluate(globalTimelineTime_);
            out.worldScale = std::max(0.01f, out.worldScale * (1.0f + lfoVal));
        }
    }

    // Track 1: Dynamics Layer
    if ((!anySolo || solo_[1]) && !muted_[1] && tracks_[1].active()) {
        const auto& dyn = tracks_[1].currentParams();
        out.enable4D = dyn.enable4D;
        out.latentW = dyn.latentW;
        out.rotorAlpha = dyn.rotorAlpha;
        out.rotorBeta = dyn.rotorBeta;
        out.rotorTheta4D = dyn.rotorTheta4D;
        out.patternSpeed = dyn.patternSpeed;
        out.seedPulseFrequency = dyn.seedPulseFrequency;
        out.seedPulseAmplitude = dyn.seedPulseAmplitude;
        out.temporalFeedbackBlend = dyn.temporalFeedbackBlend;
        out.temporalFade = dyn.temporalFade;
        out.temporalFixedDelta = dyn.temporalFixedDelta;
        out.topologyBlend = dyn.topologyBlend;
        out.seedWidth = dyn.seedWidth;
        out.seedAlpha = dyn.seedAlpha;
        out.seedBaseline = dyn.seedBaseline;
        out.advectionSpeed = dyn.advectionSpeed;
        out.curlStrength = dyn.curlStrength;
        out.trailDecay = dyn.trailDecay;
        out.kineticViscosity = dyn.kineticViscosity;

        if (lfos_[1].has_value()) {
            const float lfoVal = lfos_[1]->evaluate(globalTimelineTime_);
            out.latentW = std::clamp(out.latentW + lfoVal, 0.0f, 1.0f);
        }
    }

    // Track 2: Material & Color Layer
    if ((!anySolo || solo_[2]) && !muted_[2] && tracks_[2].active()) {
        const auto& mat = tracks_[2].currentParams();
        out.colorEffect = mat.colorEffect;
        out.hueRotation = mat.hueRotation;
        out.hueSpeed = mat.hueSpeed;
        out.chromaBreath = mat.chromaBreath;
        out.lumaDrift = mat.lumaDrift;
        out.paletteBlend = mat.paletteBlend;
        out.filmThickness = mat.filmThickness;
        out.filmIOR = mat.filmIOR;
        out.audioSpectrum = mat.audioSpectrum;

        if (lfos_[2].has_value()) {
            const float lfoVal = lfos_[2]->evaluate(globalTimelineTime_);
            out.hueRotation = std::fmod(out.hueRotation + lfoVal + 1.0f, 1.0f);
        }
    }

    // Track 3: Atmosphere Layer
    if ((!anySolo || solo_[3]) && !muted_[3] && tracks_[3].active()) {
        const auto& atm = tracks_[3].currentParams();
        out.volumetricDensityScale = atm.volumetricDensityScale;
        out.fogAnisotropy = atm.fogAnisotropy;
        out.auxiliaryResolutionScale = atm.auxiliaryResolutionScale;
        out.stageMask = atm.stageMask;

        if (lfos_[3].has_value()) {
            const float lfoVal = lfos_[3]->evaluate(globalTimelineTime_);
            out.volumetricDensityScale = std::max(0.0f, out.volumetricDensityScale * (1.0f + lfoVal));
        }
    }

    return out;
}

} // namespace epi_sculpt
