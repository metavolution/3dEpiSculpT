#include "SculptureModeProjection.h"

#include "epi/mode/mode_runtime_bridge.hpp"
#include "epi/presets/pattern_presets.hpp"

#include <cmath>
#include <string>

namespace epi_sculpt::mode_projection {

InnerMap innerFunctionToInnerMap(epi::InnerFunction fn) noexcept {
    switch (fn) {
        case epi::InnerFunction::Identity: return InnerMap::Identity;
        case epi::InnerFunction::Square: return InnerMap::Square;
        case epi::InnerFunction::Cube: return InnerMap::Cube;
        case epi::InnerFunction::Sin: return InnerMap::Sin;
        case epi::InnerFunction::Cos: return InnerMap::Cos;
        case epi::InnerFunction::Sinh: return InnerMap::Sinh;
        case epi::InnerFunction::Cosh: return InnerMap::Cosh;
        case epi::InnerFunction::Tanh: return InnerMap::Tanh;
        case epi::InnerFunction::Exp: return InnerMap::Exp;
        case epi::InnerFunction::Invert: return InnerMap::Invert;
        case epi::InnerFunction::HolomorphicPotential: return InnerMap::HolomorphicPotential;
        case epi::InnerFunction::LissajousGlitch: return InnerMap::Lissajous;
        case epi::InnerFunction::FluidVortex: return InnerMap::PoincareHyperbolic;
        case epi::InnerFunction::MagneticDipole: return InnerMap::Mobius;
        case epi::InnerFunction::Moire: return InnerMap::Clifford;
        case epi::InnerFunction::SymmetryBreak: return InnerMap::Henon;
        case epi::InnerFunction::FractalStar: return InnerMap::Mandelbox;
        case epi::InnerFunction::SpiralGalaxy: return InnerMap::Gyroid;
        case epi::InnerFunction::Glitch: return InnerMap::Drop;
        case epi::InnerFunction::P1: return InnerMap::QuaternionJulia;
        case epi::InnerFunction::P2: return InnerMap::QuaternionMandelbrot;
        case epi::InnerFunction::TP0: return InnerMap::OctonionFeedback;
        case epi::InnerFunction::TP1: return InnerMap::KleinProjective;
        case epi::InnerFunction::TP3: return InnerMap::CalabiYauQuintic;
        case epi::InnerFunction::TP4: return InnerMap::OctonionCayley;
        case epi::InnerFunction::TP5: return InnerMap::CliffordHopf;
        case epi::InnerFunction::TP6: return InnerMap::LorenzAttractor;
        case epi::InnerFunction::F0: return InnerMap::RosslerAttractor;
        case epi::InnerFunction::F1: return InnerMap::AizawaAttractor;
        case epi::InnerFunction::F2: return InnerMap::ChenAttractor;
        case epi::InnerFunction::F3: return InnerMap::ReactionDiffusionTuring;
        case epi::InnerFunction::F4: return InnerMap::HyperbolicMandelboxFold;
        case epi::InnerFunction::F5: return InnerMap::HyperbolicExponentialHomotopy;
        case epi::InnerFunction::F6: return InnerMap::OctonionAssociatorManifold;
        case epi::InnerFunction::F7: return InnerMap::KuramotoLattice;
        case epi::InnerFunction::F8: return InnerMap::QuantumWave;
        case epi::InnerFunction::F13: return InnerMap::Kaleidoscope;
        case epi::InnerFunction::TP2: return InnerMap::Log;
        case epi::InnerFunction::HyperbolicHoneycomb: return InnerMap::HyperbolicHoneycomb;
        case epi::InnerFunction::JacobiEllipticSnCn: return InnerMap::JacobiEllipticSnCn;
        case epi::InnerFunction::ChebyshevPolynomial: return InnerMap::ChebyshevPolynomial;
        default: return InnerMap::Sinh;
    }
}

epi::InnerFunction innerMapToInnerFunction(InnerMap map) noexcept {
    switch (map) {
        case InnerMap::Identity: return epi::InnerFunction::Identity;
        case InnerMap::Square: return epi::InnerFunction::Square;
        case InnerMap::Cube: return epi::InnerFunction::Cube;
        case InnerMap::Sin: return epi::InnerFunction::Sin;
        case InnerMap::Cos: return epi::InnerFunction::Cos;
        case InnerMap::Sinh: return epi::InnerFunction::Sinh;
        case InnerMap::Cosh: return epi::InnerFunction::Cosh;
        case InnerMap::Tanh: return epi::InnerFunction::Tanh;
        case InnerMap::Exp: return epi::InnerFunction::Exp;
        case InnerMap::Invert: return epi::InnerFunction::Invert;
        case InnerMap::HolomorphicPotential: return epi::InnerFunction::HolomorphicPotential;
        case InnerMap::HyperbolicHoneycomb: return epi::InnerFunction::HyperbolicHoneycomb;
        case InnerMap::JacobiEllipticSnCn: return epi::InnerFunction::JacobiEllipticSnCn;
        case InnerMap::ChebyshevPolynomial: return epi::InnerFunction::ChebyshevPolynomial;
        case InnerMap::KuramotoLattice: return epi::InnerFunction::F7;
        case InnerMap::QuantumWave: return epi::InnerFunction::F8;
        case InnerMap::Lissajous: return epi::InnerFunction::LissajousGlitch;
        case InnerMap::PoincareHyperbolic: return epi::InnerFunction::FluidVortex;
        case InnerMap::Mobius: return epi::InnerFunction::MagneticDipole;
        case InnerMap::Clifford: return epi::InnerFunction::Moire;
        case InnerMap::Henon: return epi::InnerFunction::SymmetryBreak;
        case InnerMap::Mandelbox: return epi::InnerFunction::FractalStar;
        case InnerMap::Gyroid: return epi::InnerFunction::SpiralGalaxy;
        case InnerMap::Drop: return epi::InnerFunction::Glitch;
        case InnerMap::QuaternionJulia: return epi::InnerFunction::P1;
        case InnerMap::QuaternionMandelbrot: return epi::InnerFunction::P2;
        case InnerMap::OctonionFeedback: return epi::InnerFunction::TP0;
        case InnerMap::KleinProjective: return epi::InnerFunction::TP1;
        case InnerMap::CalabiYauQuintic: return epi::InnerFunction::TP3;
        case InnerMap::OctonionCayley: return epi::InnerFunction::TP4;
        case InnerMap::CliffordHopf: return epi::InnerFunction::TP5;
        case InnerMap::LorenzAttractor: return epi::InnerFunction::TP6;
        case InnerMap::RosslerAttractor: return epi::InnerFunction::F0;
        case InnerMap::AizawaAttractor: return epi::InnerFunction::F1;
        case InnerMap::ChenAttractor: return epi::InnerFunction::F2;
        case InnerMap::ReactionDiffusionTuring: return epi::InnerFunction::F3;
        case InnerMap::HyperbolicMandelboxFold: return epi::InnerFunction::F4;
        case InnerMap::HyperbolicExponentialHomotopy: return epi::InnerFunction::F5;
        case InnerMap::OctonionAssociatorManifold: return epi::InnerFunction::F6;
        case InnerMap::Kaleidoscope: return epi::InnerFunction::F13;
        case InnerMap::Log: return epi::InnerFunction::TP2;
        default: return epi::InnerFunction::Sinh;
    }
}

ReductionMode reductionTypeToReductionMode(epi::ReductionType red) noexcept {
    switch (red) {
        case epi::ReductionType::Torus: return ReductionMode::Torus;
        case epi::ReductionType::Grid:  return ReductionMode::Grid;
        case epi::ReductionType::Clamp: return ReductionMode::Clamp;
        case epi::ReductionType::PareidoliaCathedral: return ReductionMode::PareidoliaCathedral;
        case epi::ReductionType::AmmannBeenker: return ReductionMode::AmmannBeenker;
        case epi::ReductionType::None:  return ReductionMode::Torus;
        default: return ReductionMode::Torus;
    }
}

epi::ReductionType reductionModeToReductionType(ReductionMode mode) noexcept {
    switch (mode) {
        case ReductionMode::Torus: return epi::ReductionType::Torus;
        case ReductionMode::Grid:  return epi::ReductionType::Grid;
        case ReductionMode::Clamp: return epi::ReductionType::Clamp;
        case ReductionMode::PareidoliaCathedral: return epi::ReductionType::PareidoliaCathedral;
        case ReductionMode::AmmannBeenker: return epi::ReductionType::AmmannBeenker;
        default: return epi::ReductionType::Torus;
    }
}

ColorEffectType colorModeToColorEffect(int colorMode) noexcept {
    switch (colorMode) {
        case 0: return ColorEffectType::StandardHSV;
        case 1: return ColorEffectType::DistanceHSV;
        case 2: return ColorEffectType::Vaporwave;
        case 3: return ColorEffectType::Glitch;
        case 4: return ColorEffectType::ChannelSwap;
        case 5: return ColorEffectType::ThinFilmIridescence;
        case 6: return ColorEffectType::ChromaticDispersion;
        case 11: return ColorEffectType::PhenomenologicalPalette;
        case 12: return ColorEffectType::OpArtIsoluminance;
        case 7: return ColorEffectType::ThermalHologram;
        case 8: return ColorEffectType::GoldenRatio;
        case 9: return ColorEffectType::HopfFibration;
        case 10: return ColorEffectType::QuaternionCurvature;
        case 14: return ColorEffectType::KineticVelocity;
        case 15: return ColorEffectType::Bioluminescence;
        // This is the one extended epi2 color mode whose semantic identity is
        // represented explicitly by the sculpture presentation catalog too.
        // Do not numerically pass through the other extended modes: their IDs
        // name different effects in the two catalogs.
        case 21: return ColorEffectType::LivingPaletteAgent;
        default: return ColorEffectType::StandardHSV;
    }
}

int colorEffectToColorMode(ColorEffectType effect) noexcept {
    switch (effect) {
        case ColorEffectType::StandardHSV: return 0;
        case ColorEffectType::DistanceHSV: return 1;
        case ColorEffectType::Vaporwave: return 2;
        case ColorEffectType::Glitch: return 3;
        case ColorEffectType::ChannelSwap: return 4;
        case ColorEffectType::ThinFilmIridescence: return 5;
        case ColorEffectType::ChromaticDispersion: return 6;
        case ColorEffectType::PhenomenologicalPalette: return 11;
        case ColorEffectType::OpArtIsoluminance: return 12;
        case ColorEffectType::ThermalHologram: return 7;
        case ColorEffectType::GoldenRatio: return 8;
        case ColorEffectType::HopfFibration: return 9;
        case ColorEffectType::QuaternionCurvature: return 10;
        case ColorEffectType::KineticVelocity: return 14;
        case ColorEffectType::Bioluminescence: return 15;
        case ColorEffectType::LivingPaletteAgent: return 21;
        default: return 0;
    }
}

SeedType seedTypeFrom2D(int seedType) noexcept {
    switch (seedType) {
        case 0: return SeedType::Border;
        case 1: return SeedType::WaveRing;
        case 2: return SeedType::Cross;
        case 3: return SeedType::Dipole;
        case 4: return SeedType::Airy;
        case 5: return SeedType::Lissajous;
        case 6: return SeedType::Attractor;
        case 7: return SeedType::Hopf;
        case 8: return SeedType::HyperSphere4D;
        case 9: return SeedType::CrossPolytope4D;
        case 10: return SeedType::TorusKnotPQ;
        case 11: return SeedType::ChladniCymatics;
        case 12: return SeedType::TuringPerturbation;
        case 13: return SeedType::JuliaQuaternionicSlice;
        case 14: return SeedType::OctonionFibrationS7;
        case 15: return SeedType::FibonacciPhyllotaxis;
        case 16: return SeedType::KuramotoSynchronization;
        case 17: return SeedType::NavierStokesVortex;
        case 18: return SeedType::HyperbolicTessellation;
        case 19: return SeedType::CalabiYauManifold;
        case 20: return SeedType::QuantumProbabilityCloud;
        default: return SeedType::Border;
    }
}

int seedTypeTo2D(SeedType seed) noexcept {
    return static_cast<int>(seed);
}

StageResolutionScale divisorToStageResolution(int div) noexcept {
    switch (div) {
        case 1: return StageResolutionScale::Full;
        case 2: return StageResolutionScale::Half;
        case 4: return StageResolutionScale::Quarter;
        case 8: return StageResolutionScale::Eighth;
        default: return StageResolutionScale::Full;
    }
}

int stageResolutionToDivisor(StageResolutionScale scale) noexcept {
    return static_cast<int>(scale);
}

void visualModeToSculptureParams(const epi::VisualMode& mode, SculptureProgramParams& outParams) noexcept {
    // 1. Affine Maps (zn0..zn3)
    outParams.pre.scaleReal = mode.zn0.re;
    outParams.pre.scaleImag = mode.zn0.im;
    outParams.pre.offsetReal = mode.zn1.re;
    outParams.pre.offsetImag = mode.zn1.im;

    outParams.post.scaleReal = mode.zn2.re;
    outParams.post.scaleImag = mode.zn2.im;
    outParams.post.offsetReal = mode.zn3.re;
    outParams.post.offsetImag = mode.zn3.im;

    // 2. Nonlinear Inner Warp & Reduction
    outParams.inner = innerFunctionToInnerMap(mode.inner);
    outParams.reduction = reductionTypeToReductionMode(mode.reduction);

    // 3. WCA Seed Parameters
    outParams.seedWidth = mode.seed_border_width;
    outParams.seedAlpha = mode.seed_alpha;
    outParams.seedPulseFrequency = mode.seed_pulse_freq;
    outParams.seedPulseAmplitude = mode.seed_pulse_amp;
    outParams.seedType = seedTypeFrom2D(mode.seed_type);

    // 4. Color / Material Modulation
    outParams.hueRotation = mode.hue_rotation;
    outParams.hueSpeed = mode.hue_speed;
    outParams.chromaBreath = mode.chroma_breath_amp;
    outParams.lumaDrift = mode.luma_shape_depth;
    outParams.colorEffect = colorModeToColorEffect(mode.color_mode);

    // 5. Cadence & Stage Execution
    outParams.patternSpeed = mode.speed;
    outParams.stageMask = static_cast<std::uint32_t>(mode.stage_mask);
    outParams.temporalFade = mode.fade;
    outParams.temporalFeedbackBlend = (mode.fade < 0.999f) ? 1.0f : 0.0f;
    outParams.auxiliaryResolutionScale = divisorToStageResolution(mode.seed_resolution_div);
}

void sculptureParamsToVisualMode(const SculptureProgramParams& params, epi::VisualMode& outMode, const std::string& name) {
    // 1. Affine Maps (zn0..zn3)
    outMode.zn0 = epi::Complexf{params.pre.scaleReal, params.pre.scaleImag};
    outMode.zn1 = epi::Complexf{params.pre.offsetReal, params.pre.offsetImag};
    outMode.zn2 = epi::Complexf{params.post.scaleReal, params.post.scaleImag};
    outMode.zn3 = epi::Complexf{params.post.offsetReal, params.post.offsetImag};

    // 2. Nonlinear Inner Warp & Reduction
    outMode.inner = innerMapToInnerFunction(params.inner);
    outMode.reduction = reductionModeToReductionType(params.reduction);

    // 3. WCA Seed Parameters
    outMode.seed_border_width = params.seedWidth;
    outMode.seed_alpha = params.seedAlpha;
    outMode.seed_pulse_freq = params.seedPulseFrequency;
    outMode.seed_pulse_amp = params.seedPulseAmplitude;
    outMode.seed_type = seedTypeTo2D(params.seedType);

    // 4. Color / Material Modulation
    outMode.hue_rotation = params.hueRotation;
    outMode.hue_speed = params.hueSpeed;
    outMode.chroma_breath_amp = params.chromaBreath;
    outMode.luma_shape_depth = params.lumaDrift;
    outMode.color_mode = colorEffectToColorMode(params.colorEffect);

    // 5. Cadence & Stage Execution
    outMode.speed = params.patternSpeed;
    outMode.stage_mask = static_cast<int>(params.stageMask);
    outMode.fade = params.temporalFade;
    outMode.seed_resolution_div = stageResolutionToDivisor(params.auxiliaryResolutionScale);

    if (!name.empty()) {
        outMode.name = name;
        outMode.preset_name = name;
    }
}

epi::VisualMode resolveCanonicalModeWithSculptureOverrides(
    const epi::VisualMode& canonicalMode,
    const SculptureProgramParams& projectedBaseline,
    const SculptureProgramParams& liveParams) {
    epi::VisualMode resolved = canonicalMode;

    // The canonical mode is authoritative. SculptureProgramParams is only a
    // projection of part of that state, so copying it back wholesale would
    // recreate the lossy round trip this seam exists to remove. Apply only
    // explicit deviations from the projection baseline.
    const auto scalarChanged = [](float live, float baseline) noexcept {
        return std::abs(live - baseline) > 1.0e-6f;
    };

    if (scalarChanged(liveParams.pre.scaleReal, projectedBaseline.pre.scaleReal))
        resolved.zn0.re = liveParams.pre.scaleReal;
    if (scalarChanged(liveParams.pre.scaleImag, projectedBaseline.pre.scaleImag))
        resolved.zn0.im = liveParams.pre.scaleImag;
    if (scalarChanged(liveParams.pre.offsetReal, projectedBaseline.pre.offsetReal))
        resolved.zn1.re = liveParams.pre.offsetReal;
    if (scalarChanged(liveParams.pre.offsetImag, projectedBaseline.pre.offsetImag))
        resolved.zn1.im = liveParams.pre.offsetImag;
    if (scalarChanged(liveParams.post.scaleReal, projectedBaseline.post.scaleReal))
        resolved.zn2.re = liveParams.post.scaleReal;
    if (scalarChanged(liveParams.post.scaleImag, projectedBaseline.post.scaleImag))
        resolved.zn2.im = liveParams.post.scaleImag;
    if (scalarChanged(liveParams.post.offsetReal, projectedBaseline.post.offsetReal))
        resolved.zn3.re = liveParams.post.offsetReal;
    if (scalarChanged(liveParams.post.offsetImag, projectedBaseline.post.offsetImag))
        resolved.zn3.im = liveParams.post.offsetImag;

    if (liveParams.inner != projectedBaseline.inner)
        resolved.inner = innerMapToInnerFunction(liveParams.inner);
    if (liveParams.reduction != projectedBaseline.reduction)
        resolved.reduction = reductionModeToReductionType(liveParams.reduction);

    if (scalarChanged(liveParams.seedWidth, projectedBaseline.seedWidth))
        resolved.seed_border_width = liveParams.seedWidth;
    if (scalarChanged(liveParams.seedAlpha, projectedBaseline.seedAlpha))
        resolved.seed_alpha = liveParams.seedAlpha;
    if (scalarChanged(liveParams.seedPulseFrequency, projectedBaseline.seedPulseFrequency))
        resolved.seed_pulse_freq = liveParams.seedPulseFrequency;
    if (scalarChanged(liveParams.seedPulseAmplitude, projectedBaseline.seedPulseAmplitude))
        resolved.seed_pulse_amp = liveParams.seedPulseAmplitude;
    if (liveParams.seedType != projectedBaseline.seedType)
        resolved.seed_type = seedTypeTo2D(liveParams.seedType);

    if (scalarChanged(liveParams.hueRotation, projectedBaseline.hueRotation))
        resolved.hue_rotation = liveParams.hueRotation;
    if (scalarChanged(liveParams.hueSpeed, projectedBaseline.hueSpeed))
        resolved.hue_speed = liveParams.hueSpeed;
    if (scalarChanged(liveParams.chromaBreath, projectedBaseline.chromaBreath))
        resolved.chroma_breath_amp = liveParams.chromaBreath;
    if (scalarChanged(liveParams.lumaDrift, projectedBaseline.lumaDrift))
        resolved.luma_shape_depth = liveParams.lumaDrift;
    if (liveParams.colorEffect != projectedBaseline.colorEffect)
        resolved.color_mode = colorEffectToColorMode(liveParams.colorEffect);

    if (scalarChanged(liveParams.patternSpeed, projectedBaseline.patternSpeed))
        resolved.speed = liveParams.patternSpeed;
    if (liveParams.stageMask != projectedBaseline.stageMask)
        resolved.stage_mask = static_cast<int>(liveParams.stageMask);
    if (scalarChanged(liveParams.temporalFade, projectedBaseline.temporalFade))
        resolved.fade = liveParams.temporalFade;
    if (liveParams.auxiliaryResolutionScale != projectedBaseline.auxiliaryResolutionScale)
        resolved.seed_resolution_div = stageResolutionToDivisor(liveParams.auxiliaryResolutionScale);

    return resolved;
}


std::optional<epi::VisualMode> fetchRootPatternMode(
    std::string_view presetName) {
    const int idx = epi::presets::find_index_by_name(std::string(presetName));
    if (idx < 0) return std::nullopt;

    const auto& entry = epi::presets::registry()[idx];
    epi::Pattern pattern = entry.build();
    epi::mode_command::RuntimeState runtime{};
    epi::VisualMode mode =
        epi::mode_bridge::mode_from_pattern_runtime(pattern, 1, 1, runtime);
    // mode_from_pattern_runtime intentionally serializes Pattern fields but does
    // not infer registry identity. Preserve the canonical preset key explicitly
    // so pattern_from_mode can restore preset-authored Pattern.motion later.
    if (!entry.names.empty() && entry.names.front() != nullptr) {
        mode.preset_name = entry.names.front();
    } else {
        mode.preset_name = std::string(presetName);
    }
    return mode;
}


} // namespace epi_sculpt::mode_projection
