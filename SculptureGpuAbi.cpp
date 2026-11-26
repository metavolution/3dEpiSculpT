#include <cmath>

#include "SculptureGpuAbi.h"

#include <algorithm>

namespace epi_sculpt::gpu_abi {

std::uint32_t innerId(InnerMap inner) noexcept {
    switch (inner) {
    case InnerMap::Identity: return kInnerIdentity;
    case InnerMap::Square: return kInnerSquare;
    case InnerMap::Cube: return kInnerCube;
    case InnerMap::Sin: return kInnerSin;
    case InnerMap::Cos: return kInnerCos;
    case InnerMap::Sinh: return kInnerSinh;
    case InnerMap::Cosh: return kInnerCosh;
    case InnerMap::Tanh: return kInnerTanh;
    case InnerMap::Exp: return kInnerExp;
    case InnerMap::Invert: return kInnerInvert;
    case InnerMap::Kaleidoscope: return kInnerKaleidoscope;
    case InnerMap::Log: return kInnerLog;
    case InnerMap::Henon: return kInnerHenon;
    case InnerMap::Clifford: return kInnerClifford;
    case InnerMap::Lissajous: return kInnerLissajous;
    case InnerMap::Mandelbox: return kInnerMandelbox;
    case InnerMap::Gyroid: return kInnerGyroid;
    case InnerMap::Drop: return kInnerDrop;
    case InnerMap::Mobius: return kInnerMobius;
    case InnerMap::QuaternionJulia: return kInnerQuaternionJulia;
    case InnerMap::QuaternionMandelbrot: return kInnerQuaternionMandelbrot;
    case InnerMap::OctonionFeedback: return kInnerOctonionFeedback;
    case InnerMap::PoincareHyperbolic: return kInnerPoincareHyperbolic;
    case InnerMap::KleinProjective: return kInnerKleinProjective;
    case InnerMap::CalabiYauQuintic: return kInnerCalabiYauQuintic;
    case InnerMap::OctonionCayley: return kInnerOctonionCayley;
    case InnerMap::CliffordHopf: return kInnerCliffordHopf;
    case InnerMap::LorenzAttractor: return kInnerLorenzAttractor;
    case InnerMap::RosslerAttractor: return kInnerRosslerAttractor;
    case InnerMap::AizawaAttractor: return kInnerAizawaAttractor;
    case InnerMap::ChenAttractor: return kInnerChenAttractor;
    case InnerMap::ReactionDiffusionTuring: return kInnerReactionDiffusionTuring;
    case InnerMap::HyperbolicMandelboxFold: return kInnerHyperbolicMandelboxFold;
    case InnerMap::HyperbolicExponentialHomotopy: return kInnerHyperbolicExponentialHomotopy;
    case InnerMap::OctonionAssociatorManifold: return kInnerOctonionAssociatorManifold;
    case InnerMap::HolomorphicPotential: return kInnerHolomorphicPotential;
    case InnerMap::HyperbolicHoneycomb: return kInnerHyperbolicHoneycomb;
    case InnerMap::KuramotoLattice: return kInnerKuramotoLattice;
    case InnerMap::QuantumWave: return kInnerQuantumWave;
    case InnerMap::JacobiEllipticSnCn: return kInnerJacobiEllipticSnCn;
    case InnerMap::ChebyshevPolynomial: return kInnerChebyshevPolynomial;
    case InnerMap::PenroseTiling4D: return kInnerPenroseTiling4D;
    case InnerMap::SiphonophoreAttractor: return kInnerSiphonophoreAttractor;
    case InnerMap::KaliSetOrbit: return kInnerKaliSetOrbit;
    }
    return kInnerIdentity;
}

std::uint32_t reductionId(ReductionMode reduction) noexcept {
    switch (reduction) {
    case ReductionMode::Torus: return kReductionTorus;
    case ReductionMode::Grid: return kReductionGrid;
    case ReductionMode::Clamp: return kReductionClamp;
    case ReductionMode::PareidoliaCathedral: return kReductionPareidoliaCathedral;
    case ReductionMode::AmmannBeenker: return kReductionAmmannBeenker;
    case ReductionMode::PoincareHyperbolic: return kReductionPoincareHyperbolic;
    case ReductionMode::CliffordFoliation: return kReductionCliffordFoliation;
    }
    return kReductionTorus;
}

std::uint32_t seedId(SeedType seed) noexcept {
    switch (seed) {
    case SeedType::Border: return 0U;
    case SeedType::WaveRing: return 1U;
    case SeedType::Cross: return 2U;
    case SeedType::Dipole: return kSeedDipole;
    case SeedType::Airy: return kSeedAiry;
    case SeedType::Lissajous: return kSeedLissajous;
    case SeedType::Attractor: return kSeedAttractor;
    case SeedType::Hopf: return kSeedHopf;
    case SeedType::HyperSphere4D: return kSeedHyperSphere4D;
    case SeedType::CrossPolytope4D: return kSeedCrossPolytope4D;
    case SeedType::TorusKnotPQ: return kSeedTorusKnotPQ;
    case SeedType::ChladniCymatics: return kSeedChladni;
    case SeedType::TuringPerturbation: return kSeedTuringPerturbation;
    case SeedType::JuliaQuaternionicSlice: return kSeedJuliaQuaternionicSlice;
    case SeedType::OctonionFibrationS7: return kSeedOctonionFibrationS7;
    case SeedType::FibonacciPhyllotaxis: return kSeedFibonacciPhyllotaxis;
    case SeedType::KuramotoSynchronization: return kSeedKuramotoSynchronization;
    case SeedType::NavierStokesVortex: return kSeedNavierStokesVortex;
    case SeedType::HyperbolicTessellation: return kSeedHyperbolicTessellation;
    case SeedType::CalabiYauManifold: return kSeedCalabiYauManifold;
    case SeedType::QuantumProbabilityCloud: return kSeedQuantumProbabilityCloud;
    }
    return 0U;
}

std::uint32_t colorEffectId(ColorEffectType effect) noexcept {
    switch (effect) {
    case ColorEffectType::StandardHSV: return 0U;
    case ColorEffectType::DistanceHSV: return 1U;
    case ColorEffectType::ChannelSwap: return 2U;
    case ColorEffectType::Glitch: return 3U;
    case ColorEffectType::ThermalHologram: return 4U;
    case ColorEffectType::Vaporwave: return 5U;
    case ColorEffectType::GoldenRatio: return 6U;
    case ColorEffectType::HopfFibration: return 7U;
    case ColorEffectType::QuaternionCurvature: return kColorQuaternionCurvature;
    case ColorEffectType::ThinFilmIridescence: return kColorThinFilmIridescence;
    case ColorEffectType::ChromaticDispersion: return kColorChromaticDispersion;
    case ColorEffectType::PhenomenologicalPalette: return kColorPhenomenologicalPalette;
    case ColorEffectType::OpArtIsoluminance: return kColorOpArtIsoluminance;
    case ColorEffectType::KineticVelocity: return kColorKineticVelocity;
    case ColorEffectType::Bioluminescence: return kColorBioluminescence;
    case ColorEffectType::QuantumInterference: return 16U;
    case ColorEffectType::ChladniAcoustic: return 17U;
    case ColorEffectType::LivingPaletteAgent: return 21U;
    }
    return 0U;
}

SculptureProgramParamsGpu packProgramParams(
    const SculptureProgramParams& params) noexcept {
    SculptureProgramParamsGpu packed{};
    packed.innerId = static_cast<std::uint16_t>(innerId(params.inner));
    packed.innerIdB = static_cast<std::uint16_t>(innerId(params.innerB));
    packed.reductionId = static_cast<std::uint16_t>(reductionId(params.reduction));
    packed.reductionIdB = static_cast<std::uint16_t>(reductionId(params.reductionB));
    packed.seedId = static_cast<std::uint16_t>(seedId(params.seedType));
    packed.seedIdB = static_cast<std::uint16_t>(seedId(params.seedTypeB));
    packed.colorEffectId = static_cast<std::uint16_t>(colorEffectId(params.colorEffect));
    packed.enable4D = static_cast<std::uint16_t>((params.enable4D & 0xFFU) | ((params.enable4DB & 0xFFU) << 8U));
    packed.feedbackIterations = std::clamp(params.feedbackIterations, 1U, 12U);
    packed.latentW = params.latentW;
    packed.topologyBlend = std::clamp(params.topologyBlend, 0.0f, 1.0f);
    packed.outputMode = params.outputMode;
    packed.outputModeB = params.outputModeB;
    packed.stageMask = learn_mode::normalize(params.stageMask);
    packed.preAffine = {
        params.pre.scaleReal, params.pre.scaleImag,
        params.pre.offsetReal, params.pre.offsetImag,
    };
    packed.postAffine = {
        params.post.scaleReal, params.post.scaleImag,
        params.post.offsetReal, params.post.offsetImag,
    };
    packed.dynamicsSeed = {
        params.patternSpeed, params.seedWidth,
        params.seedAlpha, params.seedBaseline,
    };
    packed.pulseMaterial = {
        params.seedPulseFrequency, params.seedPulseAmplitude,
        params.hueRotation, params.hueSpeed,
    };
    packed.outputScale = {
        params.worldScale, params.radiusScale, params.temporalFeedbackBlend, params.temporalReset ? 1.0f : 0.0f,
    };
    packed.kineticFlow = {
        params.advectionSpeed, params.curlStrength,
        params.trailDecay, params.kineticViscosity
    };
    packed.audioSpectrum = {
        params.audioSpectrum[0], params.audioSpectrum[1],
        params.audioSpectrum[2], params.audioSpectrum[3]
    };
    packed.auxiliaryResolutionScale = static_cast<std::uint32_t>(params.auxiliaryResolutionScale);
    packed.attractorMode = params.attractorMode;
    packed.attractorStrength = params.attractorStrength;
    packed.chromaBreath = params.chromaBreath;
    packed.lumaDrift = params.lumaDrift;
    packed.rotorAlpha = params.rotorAlpha;
    packed.rotorBeta = params.rotorBeta;
    packed.rotorTheta4D = params.rotorTheta4D;
    packed.opticalVolumetric = {
        params.volumetricDensityScale, params.fogAnisotropy,
        params.filmThickness, params.filmIOR
    };
    packed.temporalControl = {
        params.temporalFade, params.temporalFixedDelta,
        0.0f, 0.0f
    };
    return packed;

}

SculptureFrameGpu packFrame(float time,
                             float morph,
                             float deltaSeconds,
                             std::uint32_t generation) noexcept {
    SculptureFrameGpu packed{};
    packed.generation = generation;
    packed.time = time;
    packed.morph = std::clamp(morph, 0.0f, 1.0f);
    packed.deltaSeconds = std::max(deltaSeconds, 0.0f);
    return packed;

}

SculptureSampleGpu packSample(const SculptureSample& sample) noexcept {
    SculptureSampleGpu packed{};
    packed.positionRadius = {sample.x, sample.y, sample.z, sample.radius};
    packed.colorTint = {sample.r, sample.g, sample.b, sample.a};
    packed.normalVector = {sample.nx, sample.ny, sample.nz, sample.nw};
    packed.velocity = {sample.vx, sample.vy, sample.vz, sample.vw};
    packed.materialRole = sample.materialRole;
    return packed;

}

SculptureAnalyticsGpu packAnalytics(const PackedSculptureSnapshot& packed) noexcept {
    SculptureAnalyticsGpu analytics{};
    analytics.activeInstanceCount = packed.activeInstanceCount;
    analytics.generation = packed.frame.generation;
    analytics.boundsMin = {1e9f, 1e9f, 1e9f};
    analytics.boundsMax = {-1e9f, -1e9f, -1e9f};
    float speedSum = 0.0f;

    for (std::uint32_t i = 0; i < packed.activeInstanceCount; ++i) {
        float speed = std::sqrt(
            packed.samples[i].velocity[0] * packed.samples[i].velocity[0] +
            packed.samples[i].velocity[1] * packed.samples[i].velocity[1] +
            packed.samples[i].velocity[2] * packed.samples[i].velocity[2]);
            
        if (i < 100) {
            analytics.totalOrbit += packed.samples[i].positionRadius[3];
            analytics.totalKineticEnergy += speed;
        }
        
        if (i % 64 == 0) {
            analytics.boundsMin[0] = std::min(analytics.boundsMin[0], packed.samples[i].positionRadius[0]);
            analytics.boundsMin[1] = std::min(analytics.boundsMin[1], packed.samples[i].positionRadius[1]);
            analytics.boundsMin[2] = std::min(analytics.boundsMin[2], packed.samples[i].positionRadius[2]);
            
            analytics.boundsMax[0] = std::max(analytics.boundsMax[0], packed.samples[i].positionRadius[0]);
            analytics.boundsMax[1] = std::max(analytics.boundsMax[1], packed.samples[i].positionRadius[1]);
            analytics.boundsMax[2] = std::max(analytics.boundsMax[2], packed.samples[i].positionRadius[2]);
            
            speedSum += speed;
        }
    }
    
    std::uint32_t sampledCount = (packed.activeInstanceCount + 63) / 64;
    analytics.averageSpeed = sampledCount > 0 ? speedSum / sampledCount : 0.0f;
    return analytics;
}

void packSnapshot(const SculptureSnapshot& snapshot,
                                     float deltaSeconds,
                                     std::uint32_t generation,
                                     PackedSculptureSnapshot& outPacked) noexcept {

    outPacked.frame = packFrame(
        snapshot.time, snapshot.morph, deltaSeconds, generation);
    std::uint32_t activeCount = 0U;
    for (std::size_t index = 0; index < snapshot.samples.size(); ++index) {
        if (snapshot.samples[index].radius > 0.0f) {
            outPacked.samples[activeCount++] = packSample(snapshot.samples[index]);
        }
    }
    outPacked.activeInstanceCount = activeCount;
}

} // namespace epi_sculpt::gpu_abi
