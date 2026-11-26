// Versioned host/OpenCL/Vulkan exchange contract for 3dEpiSculpTMorph.
//
// This is deliberately separate from epiC's 2D GPUKernelParams. Every field
// has fixed width and every record is aligned for OpenCL/Vulkan buffer use.
// C++ enums, bools, pointers, GLM values, and implementation-owned padding are
// forbidden at this boundary.

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "SculptureProgramSchema.h"

namespace epi_sculpt::gpu_abi {

inline constexpr std::uint32_t kVersion = 1U;
inline constexpr std::uint32_t kProgramParamsBytes = 224U;
inline constexpr std::uint32_t kFrameBytes = 32U;
inline constexpr std::uint32_t kSampleBytes = 80U;

// Educational Annotation: Volumetric Fluid Memory Grid Boundaries
// Phase F introduces 3D cellular automata diffusion. The simulation is physically 
// bounded to a 50x50x50 spatial grid (125,000 floats). To optimize raymarching 
// and kernel execution, we divide this space into 10x10x10 macro-blocks, resulting 
// in exactly 125 sparse evaluation regions.
inline constexpr std::uint32_t kVolumeGridSize = 50U;
inline constexpr std::uint32_t kVolumeVoxelCount = kVolumeGridSize * kVolumeGridSize * kVolumeGridSize;
inline constexpr std::uint32_t kVolumeMacroSize = 10U;
inline constexpr std::uint32_t kVolumeMacroBlockCount = (kVolumeGridSize / kVolumeMacroSize) * 
                                                         (kVolumeGridSize / kVolumeMacroSize) * 
                                                         (kVolumeGridSize / kVolumeMacroSize);

// Stable numeric IDs mirrored literally in SculptureGpuAbi.clh.
inline constexpr std::uint32_t kInnerIdentity = 0U;
inline constexpr std::uint32_t kInnerSquare = 1U;
inline constexpr std::uint32_t kInnerCube = 2U;
inline constexpr std::uint32_t kInnerSin = 3U;
inline constexpr std::uint32_t kInnerCos = 4U;
inline constexpr std::uint32_t kInnerSinh = 5U;
inline constexpr std::uint32_t kInnerCosh = 6U;
inline constexpr std::uint32_t kInnerTanh = 7U;
inline constexpr std::uint32_t kInnerExp = 8U;
inline constexpr std::uint32_t kInnerInvert = 9U;
inline constexpr std::uint32_t kInnerKaleidoscope = 10U;
inline constexpr std::uint32_t kInnerLog = 11U;
inline constexpr std::uint32_t kInnerHenon = 12U;
inline constexpr std::uint32_t kInnerClifford = 13U;
inline constexpr std::uint32_t kInnerLissajous = 14U;
inline constexpr std::uint32_t kInnerMandelbox = 15U;
inline constexpr std::uint32_t kInnerGyroid = 16U;
inline constexpr std::uint32_t kInnerDrop = 17U;
inline constexpr std::uint32_t kInnerMobius = 18U;
inline constexpr std::uint32_t kInnerQuaternionJulia = 19U;
inline constexpr std::uint32_t kInnerQuaternionMandelbrot = 20U;
inline constexpr std::uint32_t kInnerOctonionFeedback = 21U;
inline constexpr std::uint32_t kInnerPoincareHyperbolic = 22U;
inline constexpr std::uint32_t kInnerKleinProjective = 23U;
inline constexpr std::uint32_t kInnerCalabiYauQuintic = 24U;
inline constexpr std::uint32_t kInnerOctonionCayley = 25U;
inline constexpr std::uint32_t kInnerCliffordHopf = 26U;
inline constexpr std::uint32_t kInnerLorenzAttractor = 27U;
inline constexpr std::uint32_t kInnerRosslerAttractor = 28U;
inline constexpr std::uint32_t kInnerAizawaAttractor = 29U;
inline constexpr std::uint32_t kInnerChenAttractor = 30U;
inline constexpr std::uint32_t kInnerReactionDiffusionTuring = 31U;
inline constexpr std::uint32_t kInnerHyperbolicMandelboxFold = 32U;
inline constexpr std::uint32_t kInnerHyperbolicExponentialHomotopy = 33U;
inline constexpr std::uint32_t kInnerOctonionAssociatorManifold = 34U;
inline constexpr std::uint32_t kInnerHolomorphicPotential = 35U;
inline constexpr std::uint32_t kInnerHyperbolicHoneycomb = 36U;
inline constexpr std::uint32_t kInnerKuramotoLattice = 37U;
inline constexpr std::uint32_t kInnerQuantumWave = 38U;
inline constexpr std::uint32_t kInnerJacobiEllipticSnCn = 39U;
inline constexpr std::uint32_t kInnerChebyshevPolynomial = 40U;
inline constexpr std::uint32_t kInnerPenroseTiling4D = 41U;
inline constexpr std::uint32_t kInnerSiphonophoreAttractor = 42U;
inline constexpr std::uint32_t kInnerKaliSetOrbit = 43U;

inline constexpr std::uint32_t kReductionTorus = 0U;
inline constexpr std::uint32_t kReductionGrid = 1U;
inline constexpr std::uint32_t kReductionClamp = 2U;
inline constexpr std::uint32_t kReductionPareidoliaCathedral = 3U;
inline constexpr std::uint32_t kReductionAmmannBeenker = 4U;
inline constexpr std::uint32_t kReductionPoincareHyperbolic = 5U;
inline constexpr std::uint32_t kReductionCliffordFoliation = 6U;
inline constexpr std::uint32_t kSeedDipole = 3U;
inline constexpr std::uint32_t kSeedAiry = 4U;
inline constexpr std::uint32_t kSeedLissajous = 5U;
inline constexpr std::uint32_t kSeedAttractor = 6U;
inline constexpr std::uint32_t kSeedHopf = 7U;
inline constexpr std::uint32_t kSeedHyperSphere4D = 8U;
inline constexpr std::uint32_t kSeedCrossPolytope4D = 9U;
inline constexpr std::uint32_t kSeedTorusKnotPQ = 10U;
inline constexpr std::uint32_t kSeedChladni = 11U;
inline constexpr std::uint32_t kSeedTuringPerturbation = 12U;
inline constexpr std::uint32_t kSeedJuliaQuaternionicSlice = 13U;
inline constexpr std::uint32_t kSeedOctonionFibrationS7 = 14U;
inline constexpr std::uint32_t kSeedFibonacciPhyllotaxis = 15U;
inline constexpr std::uint32_t kSeedKuramotoSynchronization = 16U;
inline constexpr std::uint32_t kSeedNavierStokesVortex = 17U;
inline constexpr std::uint32_t kSeedHyperbolicTessellation = 18U;
inline constexpr std::uint32_t kSeedCalabiYauManifold = 19U;
inline constexpr std::uint32_t kSeedQuantumProbabilityCloud = 20U;

inline constexpr std::uint32_t kColorQuaternionCurvature = 8U;
inline constexpr std::uint32_t kColorThinFilmIridescence = 9U;
inline constexpr std::uint32_t kColorChromaticDispersion = 10U;
inline constexpr std::uint32_t kColorPhenomenologicalPalette = 11U;
inline constexpr std::uint32_t kColorOpArtIsoluminance = 12U;
inline constexpr std::uint32_t kColorKineticVelocity = 13U;
inline constexpr std::uint32_t kColorGeometricCurvature = 14U;
inline constexpr std::uint32_t kColorBioluminescence = 15U;

inline constexpr std::uint16_t kOutputModeKnot = 0U;
inline constexpr std::uint16_t kOutputModeVolume = 1U;
inline constexpr std::uint16_t kOutputModeTorus = 2U;
inline constexpr std::uint16_t kOutputModeSphere = 3U;
inline constexpr std::uint16_t kOutputModeCliffordHopf = 4U;
inline constexpr std::uint16_t kOutputModePseudosphere = 5U;
inline constexpr std::uint16_t kOutputModeStarburst = 6U;
inline constexpr std::uint16_t kOutputModeCount = 7U;

struct alignas(16) SculptureProgramParamsGpu {
    // Header: version and byte size make stale host/kernel combinations
    // diagnosable before dispatch.
    std::uint32_t abiVersion = kVersion;
    std::uint32_t byteSize = kProgramParamsBytes;
    std::uint16_t innerId = kInnerIdentity;
    std::uint16_t innerIdB = kInnerIdentity;
    std::uint16_t reductionId = static_cast<std::uint16_t>(kReductionTorus);
    std::uint16_t reductionIdB = static_cast<std::uint16_t>(kReductionTorus);
    std::uint16_t outputMode = 0; // 0=knot, 1=volume, 2=torus, 3=sphere, 4=clifford_hopf, 5=pseudosphere, 6=starburst
    std::uint16_t outputModeB = 0;
    std::uint16_t seedId = 0U; // 0=border, 1=wave_ring, 2=cross
    std::uint16_t seedIdB = 0U;
    std::uint16_t colorEffectId = 0U; // 0=standard, 1=distance, 2=channel_swap
    std::uint16_t enable4D = 0U;
    
    // Aligns to 16-byte boundary
    std::uint32_t stageMask = learn_mode::kAllStages; // Canonical Learn Mode ABI

    std::uint32_t feedbackIterations = 1U;
    std::uint32_t sampleCount = static_cast<std::uint32_t>(kSampleCount);
    float latentW = 0.0f; // 4th dimensional hyper-coordinate
    float topologyBlend = 0.0f;

    // float4-compatible groups, named by semantic tuple:
    // (scale.real, scale.imag, offset.real, offset.imag).
    std::array<float, 4> preAffine{};
    std::array<float, 4> postAffine{};

    // (patternSpeed, seedWidth, seedAlpha, seedBaseline).
    std::array<float, 4> dynamicsSeed{};
    // (seedPulseFrequency, seedPulseAmplitude, hueRotation, hueSpeed).
    std::array<float, 4> pulseMaterial{};
    // (worldScale, radiusScale, temporalFeedbackBlend, temporalReset).
    std::array<float, 4> outputScale{};
    // (advectionSpeed, curlStrength, trailDecay, kineticViscosity).
    std::array<float, 4> kineticFlow{};
    std::array<float, 4> audioSpectrum{};
    
    // Auxiliary resolution scale and 4D rotor parameters (aligned to 16 bytes)
    std::uint32_t auxiliaryResolutionScale = 1U;
    std::uint32_t attractorMode = 0U;
    float attractorStrength = 0.0f;
    float chromaBreath = 0.0f;
    float lumaDrift = 0.0f;
    float rotorAlpha = 0.0f;
    float rotorBeta = 0.0f;
    float rotorTheta4D = 0.0f;
    std::array<float, 4> opticalVolumetric{};
    std::array<float, 4> temporalControl{};

    // Semantic comparison avoids padding bytes when observing parameter changes.
    bool operator==(const SculptureProgramParamsGpu&) const = default;
};

struct alignas(16) SculptureFrameGpu {
    std::uint32_t abiVersion = kVersion;
    std::uint32_t byteSize = kFrameBytes;
    std::uint32_t sampleCount = static_cast<std::uint32_t>(kSampleCount);
    std::uint32_t generation = 0U;

    float time = 0.0f;
    float morph = 0.0f;
    float deltaSeconds = 0.0f;
    float reservedF32 = 0.0f;
};

struct alignas(16) SculptureSampleGpu {
    std::array<float, 4> positionRadius{};
    std::array<float, 4> colorTint{};
    std::array<float, 4> normalVector{};
    std::array<float, 4> velocity{};
    std::uint32_t materialRole = 0U;
    std::uint32_t reserved0 = 0U;
    std::uint32_t reserved1 = 0U;
    std::uint32_t reserved2 = 0U;
};

struct alignas(16) SculptureAnalyticsGpu {
    std::uint32_t activeInstanceCount = 0U;
    std::uint32_t generation = 0U;
    float totalOrbit = 0.0f;
    float totalKineticEnergy = 0.0f;

    std::array<float, 3> boundsMin{0.0f, 0.0f, 0.0f};
    float averageSpeed = 0.0f;

    std::array<float, 3> boundsMax{0.0f, 0.0f, 0.0f};
    float reserved = 0.0f;
};

struct alignas(16) PackedSculptureSnapshot {
    SculptureFrameGpu frame{};
    std::uint32_t activeInstanceCount = 0U;
    std::uint32_t reserved0 = 0U;
    std::uint32_t reserved1 = 0U;
    std::uint32_t reserved2 = 0U;
    std::array<SculptureSampleGpu, kSampleCount> samples{};
};

[[nodiscard]] std::uint32_t innerId(InnerMap inner) noexcept;
[[nodiscard]] std::uint32_t reductionId(ReductionMode reduction) noexcept;
[[nodiscard]] std::uint32_t seedId(SeedType seed) noexcept;
[[nodiscard]] std::uint32_t colorEffectId(ColorEffectType effect) noexcept;

[[nodiscard]] SculptureProgramParamsGpu packProgramParams(
    const SculptureProgramParams& params) noexcept;
[[nodiscard]] SculptureFrameGpu packFrame(float time,
                                           float morph,
                                           float deltaSeconds,
                                           std::uint32_t generation) noexcept;
[[nodiscard]] SculptureSampleGpu packSample(
    const SculptureSample& sample) noexcept;
void packSnapshot(
    const SculptureSnapshot& snapshot,
    float deltaSeconds,
    std::uint32_t generation,
    PackedSculptureSnapshot& outPacked) noexcept;
[[nodiscard]] SculptureAnalyticsGpu packAnalytics(
    const PackedSculptureSnapshot& packed) noexcept;

static_assert(std::is_standard_layout_v<SculptureProgramParamsGpu>);
static_assert(std::is_trivially_copyable_v<SculptureProgramParamsGpu>);
static_assert(sizeof(SculptureProgramParamsGpu) == kProgramParamsBytes);
static_assert(sizeof(SculptureProgramParamsGpu) == 224U);
static_assert(sizeof(SculptureProgramParamsGpu) % 16U == 0U);
static_assert(alignof(SculptureProgramParamsGpu) == 16U);
static_assert(offsetof(SculptureProgramParamsGpu, abiVersion) == 0U);
static_assert(offsetof(SculptureProgramParamsGpu, byteSize) == 4U);
static_assert(offsetof(SculptureProgramParamsGpu, innerId) == 8U);
static_assert(offsetof(SculptureProgramParamsGpu, innerIdB) == 10U);
static_assert(offsetof(SculptureProgramParamsGpu, reductionId) == 12U);
static_assert(offsetof(SculptureProgramParamsGpu, outputMode) == 16U);
static_assert(offsetof(SculptureProgramParamsGpu, outputModeB) == 18U);
static_assert(offsetof(SculptureProgramParamsGpu, seedId) == 20U);
static_assert(offsetof(SculptureProgramParamsGpu, seedIdB) == 22U);
static_assert(offsetof(SculptureProgramParamsGpu, colorEffectId) == 24U);
static_assert(offsetof(SculptureProgramParamsGpu, enable4D) == 26U);
static_assert(offsetof(SculptureProgramParamsGpu, stageMask) == 28U);
static_assert(offsetof(SculptureProgramParamsGpu, feedbackIterations) == 32U);
static_assert(offsetof(SculptureProgramParamsGpu, sampleCount) == 36U);
static_assert(offsetof(SculptureProgramParamsGpu, latentW) == 40U);
static_assert(offsetof(SculptureProgramParamsGpu, topologyBlend) == 44U);
static_assert(offsetof(SculptureProgramParamsGpu, preAffine) == 48U);
static_assert(offsetof(SculptureProgramParamsGpu, postAffine) == 64U);
static_assert(offsetof(SculptureProgramParamsGpu, dynamicsSeed) == 80U);
static_assert(offsetof(SculptureProgramParamsGpu, pulseMaterial) == 96U);
static_assert(offsetof(SculptureProgramParamsGpu, outputScale) == 112U);
static_assert(offsetof(SculptureProgramParamsGpu, kineticFlow) == 128U);
static_assert(offsetof(SculptureProgramParamsGpu, audioSpectrum) == 144U);
static_assert(offsetof(SculptureProgramParamsGpu, auxiliaryResolutionScale) == 160U);
static_assert(offsetof(SculptureProgramParamsGpu, attractorMode) == 164U);
static_assert(offsetof(SculptureProgramParamsGpu, attractorStrength) == 168U);
static_assert(offsetof(SculptureProgramParamsGpu, chromaBreath) == 172U);
static_assert(offsetof(SculptureProgramParamsGpu, lumaDrift) == 176U);
static_assert(offsetof(SculptureProgramParamsGpu, rotorAlpha) == 180U);
static_assert(offsetof(SculptureProgramParamsGpu, rotorBeta) == 184U);
static_assert(offsetof(SculptureProgramParamsGpu, rotorTheta4D) == 188U);
static_assert(offsetof(SculptureProgramParamsGpu, opticalVolumetric) == 192U);
static_assert(offsetof(SculptureProgramParamsGpu, temporalControl) == 208U);

static_assert(std::is_standard_layout_v<SculptureFrameGpu>);
static_assert(std::is_trivially_copyable_v<SculptureFrameGpu>);
static_assert(sizeof(SculptureFrameGpu) == kFrameBytes);
static_assert(sizeof(SculptureFrameGpu) == 32U);
static_assert(sizeof(SculptureFrameGpu) % 16U == 0U);
static_assert(alignof(SculptureFrameGpu) == 16U);
static_assert(offsetof(SculptureFrameGpu, abiVersion) == 0U);
static_assert(offsetof(SculptureFrameGpu, byteSize) == 4U);
static_assert(offsetof(SculptureFrameGpu, sampleCount) == 8U);
static_assert(offsetof(SculptureFrameGpu, generation) == 12U);
static_assert(offsetof(SculptureFrameGpu, time) == 16U);
static_assert(offsetof(SculptureFrameGpu, morph) == 20U);
static_assert(offsetof(SculptureFrameGpu, deltaSeconds) == 24U);
static_assert(offsetof(SculptureFrameGpu, reservedF32) == 28U);

static_assert(std::is_standard_layout_v<SculptureSampleGpu>);
static_assert(std::is_trivially_copyable_v<SculptureSampleGpu>);
static_assert(sizeof(SculptureSampleGpu) == kSampleBytes);
static_assert(sizeof(SculptureSampleGpu) == 80U);
static_assert(sizeof(SculptureSampleGpu) % 16U == 0U);
static_assert(alignof(SculptureSampleGpu) == 16U);
static_assert(offsetof(SculptureSampleGpu, positionRadius) == 0U);
static_assert(offsetof(SculptureSampleGpu, colorTint) == 16U);
static_assert(offsetof(SculptureSampleGpu, normalVector) == 32U);
static_assert(offsetof(SculptureSampleGpu, velocity) == 48U);
static_assert(offsetof(SculptureSampleGpu, materialRole) == 64U);

static_assert(std::is_standard_layout_v<SculptureAnalyticsGpu>);
static_assert(std::is_trivially_copyable_v<SculptureAnalyticsGpu>);
static_assert(sizeof(SculptureAnalyticsGpu) == 48U);
static_assert(alignof(SculptureAnalyticsGpu) == 16U);
static_assert(offsetof(SculptureAnalyticsGpu, activeInstanceCount) == 0U);
static_assert(offsetof(SculptureAnalyticsGpu, generation) == 4U);
static_assert(offsetof(SculptureAnalyticsGpu, totalOrbit) == 8U);
static_assert(offsetof(SculptureAnalyticsGpu, totalKineticEnergy) == 12U);
static_assert(offsetof(SculptureAnalyticsGpu, boundsMin) == 16U);
static_assert(offsetof(SculptureAnalyticsGpu, averageSpeed) == 28U);
static_assert(offsetof(SculptureAnalyticsGpu, boundsMax) == 32U);
static_assert(offsetof(SculptureAnalyticsGpu, reserved) == 44U);

static_assert(std::is_standard_layout_v<PackedSculptureSnapshot>);
static_assert(std::is_trivially_copyable_v<PackedSculptureSnapshot>);
static_assert(offsetof(PackedSculptureSnapshot, frame) == 0U);
static_assert(offsetof(PackedSculptureSnapshot, activeInstanceCount) == 32U);
static_assert(offsetof(PackedSculptureSnapshot, reserved0) == 36U);
static_assert(offsetof(PackedSculptureSnapshot, reserved1) == 40U);
static_assert(offsetof(PackedSculptureSnapshot, reserved2) == 44U);
static_assert(offsetof(PackedSculptureSnapshot, samples) == 48U);

// Interop boundary struct for Zero-Copy Vulkan/OpenCL framebuffer handoff
struct alignas(8) SculptureExternalMemoryHandoff {
    int fileDescriptor = -1;
    std::uint32_t allocationSize = 0U;
    std::uint32_t memoryTypeBits = 0U;
    std::uint32_t flags = 0U;
};

static_assert(std::is_standard_layout_v<SculptureExternalMemoryHandoff>);
static_assert(std::is_trivially_copyable_v<SculptureExternalMemoryHandoff>);
static_assert(sizeof(SculptureExternalMemoryHandoff) == 16U);
static_assert(offsetof(SculptureExternalMemoryHandoff, fileDescriptor) == 0U);
static_assert(offsetof(SculptureExternalMemoryHandoff, allocationSize) == 4U);
static_assert(offsetof(SculptureExternalMemoryHandoff, memoryTypeBits) == 8U);
static_assert(offsetof(SculptureExternalMemoryHandoff, flags) == 12U);

} // namespace epi_sculpt::gpu_abi
