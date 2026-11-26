// -----------------------------------------------------------------------------
// Educational Annotation: PROGRAM STATE SCHEMA
// -----------------------------------------------------------------------------
// The schema defines the authored mathematical vocabulary shared by tools and
// execution backends. Keeping these value types separate from the CPU evaluator
// makes the dependency direction explicit and lets headless tooling stay headless.
// -----------------------------------------------------------------------------
// Renderer-neutral epimorphism sculpture schema.
//
// This file is the stable value vocabulary shared by authoring, serialization,
// CPU reference evaluation, OpenCL producers and presentation adapters. It
// intentionally contains no evaluator lifecycle and no mGFX/Vulkan/OpenCL types.
// Keeping authored state here makes dependency direction visible:
//   schema -> authoring/serialization -> execution -> presentation.

#pragma once

#include <vector>
#include <array>
#include "SculptureLearnMode.h"
#include "SculptureSampleHistory.h"
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <optional>

namespace epi_sculpt {

inline constexpr std::size_t kSampleCount = 32768; // 32k samples for dense instance-field ABI and volume output

// The compact first catalog is the stable, directly corresponding prefix of
// epiC's 2D InnerFunction catalog.
enum class InnerMap : std::uint8_t {
    Identity,
    Square,
    Cube,
    Sin,
    Cos,
    Sinh,
    Cosh,
    Tanh,
    Exp,
    Invert,
    Kaleidoscope,
    Log,
    Henon,
    Clifford,
    Lissajous,
    Mandelbox,
    Gyroid,
    Drop,
    Mobius,
    QuaternionJulia,
    QuaternionMandelbrot,
    OctonionFeedback,
    PoincareHyperbolic,
    KleinProjective,
    CalabiYauQuintic,
    OctonionCayley,
    CliffordHopf,
    LorenzAttractor,
    RosslerAttractor,
    AizawaAttractor,
    ChenAttractor,
    ReactionDiffusionTuring,
    HyperbolicMandelboxFold,
    HyperbolicExponentialHomotopy,
    OctonionAssociatorManifold,
    HolomorphicPotential,
    HyperbolicHoneycomb,
    KuramotoLattice,
    QuantumWave,
    JacobiEllipticSnCn,
    ChebyshevPolynomial,
    PenroseTiling4D,
    SiphonophoreAttractor,
    KaliSetOrbit,
};

enum class ReductionMode : std::uint8_t {
    Torus,
    Grid,
    Clamp,
    PareidoliaCathedral,
    AmmannBeenker,
    PoincareHyperbolic,
    CliffordFoliation,
};

enum class StageResolutionScale : std::uint8_t {
    Full = 1,    // 50^3 (125,000 voxels)
    Half = 2,    // 25^3 (15,625 voxels)
    Quarter = 4, // 12^3 (1,728 voxels)
    Eighth = 8,  // 6^3 (216 voxels)
};

// The stored values are scale divisors, not ordinal UI indices. Admission at
// mutable owners must precede publishing an override, descriptor or resource.
[[nodiscard]] inline constexpr bool isValidStageResolutionScale(StageResolutionScale scale) noexcept {
    switch (scale) {
    case StageResolutionScale::Full:
    case StageResolutionScale::Half:
    case StageResolutionScale::Quarter:
    case StageResolutionScale::Eighth:
        return true;
    default:
        return false;
    }
}

enum class SeedType : std::uint8_t {
    Border,
    WaveRing,
    Cross,
    Dipole,
    Airy,
    Lissajous,
    Attractor,
    Hopf,
    HyperSphere4D,
    CrossPolytope4D,
    TorusKnotPQ,
    ChladniCymatics,
    TuringPerturbation,
    JuliaQuaternionicSlice,
    OctonionFibrationS7,
    FibonacciPhyllotaxis,
    KuramotoSynchronization,
    NavierStokesVortex,
    HyperbolicTessellation,
    CalabiYauManifold,
    QuantumProbabilityCloud,
};

enum class ColorEffectType : std::uint8_t {
    StandardHSV,
    DistanceHSV,
    ChannelSwap,
    Glitch,
    ThermalHologram,
    Vaporwave,
    GoldenRatio,
    HopfFibration,
    QuaternionCurvature,
    ThinFilmIridescence,
    ChromaticDispersion,
    PhenomenologicalPalette,
    OpArtIsoluminance,
    KineticVelocity,
    Bioluminescence = 15, // Biologically inspired pulsing self-emission glow
    
    // Extended Quantum & Acoustic
    QuantumInterference = 16,
    ChladniAcoustic = 17,
    LivingPaletteAgent = 21,
};

enum class SculptureLodTier : std::uint8_t {
    LOD0_Full = 0,
    LOD1_Medium = 1,
    LOD2_Low = 2,
    LOD_Culled = 3
};

struct SculptureLodConfig {
    bool enableDynamicLod = false;
    float nearDistance = 4.0f;
    float farDistance = 12.0f;
    float cullDistance = 25.0f;
    float lodBias = 1.0f;

    [[nodiscard]] constexpr SculptureLodTier calculateTier(float distanceToCam) const noexcept {
        const float bias = lodBias > 0.01f ? lodBias : 0.01f;
        const float nearD = nearDistance * bias;
        const float farD = farDistance * bias;
        const float cullD = cullDistance * bias;

        if (distanceToCam >= cullD) return SculptureLodTier::LOD_Culled;
        if (distanceToCam >= farD) return SculptureLodTier::LOD2_Low;
        if (distanceToCam >= nearD) return SculptureLodTier::LOD1_Medium;
        return SculptureLodTier::LOD0_Full;
    }
};

struct SculptureLodStats {
    std::size_t lod0Count = 0U;
    std::size_t lod1Count = 0U;
    std::size_t lod2Count = 0U;
    std::size_t lodCulledCount = 0U;

    [[nodiscard]] constexpr std::size_t totalActive() const noexcept {
        return lod0Count + lod1Count + lod2Count;
    }
    [[nodiscard]] constexpr float lod0Ratio() const noexcept {
        const std::size_t total = totalActive();
        return total > 0 ? static_cast<float>(lod0Count) / static_cast<float>(total) : 0.0f;
    }
    [[nodiscard]] constexpr float lod1Ratio() const noexcept {
        const std::size_t total = totalActive();
        return total > 0 ? static_cast<float>(lod1Count) / static_cast<float>(total) : 0.0f;
    }
    [[nodiscard]] constexpr float lod2Ratio() const noexcept {
        const std::size_t total = totalActive();
        return total > 0 ? static_cast<float>(lod2Count) / static_cast<float>(total) : 0.0f;
    }
};

struct ComplexAffine {
    float scaleReal = 1.0f;
    float scaleImag = 0.0f;
    float offsetReal = 0.0f;
    float offsetImag = 0.0f;
};

struct SculptureVec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct SculptureBoundingVolume {
    SculptureVec3 minBounds{0.0f, 0.0f, 0.0f};
    SculptureVec3 maxBounds{0.0f, 0.0f, 0.0f};
    SculptureVec3 center{0.0f, 0.0f, 0.0f};
    float boundingRadius{0.0f};
    bool valid{false};
};

struct SculptureMotionKinematics {
    SculptureVec3 averageVelocity{0.0f, 0.0f, 0.0f};
    float maxSpeed{0.0f};
    float kineticEnergy{0.0f};
    std::size_t activeCount{0};
    // True only when velocity, maximum speed and energy were actually measured.
    // activeCount may still be known when those measurements are unavailable.
    bool valid{false};
};

struct SculptureProgramParams {
    // Mirrors epiC's canonical pre-affine -> inner -> post-affine transform.
    ComplexAffine pre{1.08f, 0.08f, 0.025f, -0.015f};
    ComplexAffine post{0.78f, -0.035f, 0.0f, 0.0f};
    InnerMap inner = InnerMap::Sinh;
    InnerMap innerB = InnerMap::Identity;
    ReductionMode reduction = ReductionMode::Torus;
    ReductionMode reductionB = ReductionMode::Torus;
    
    SeedType seedType = SeedType::Border;
    SeedType seedTypeB = SeedType::Border;
    ColorEffectType colorEffect = ColorEffectType::StandardHSV;

    std::uint32_t feedbackIterations = 3;
    float patternSpeed = 0.075f;
    float latentW = 0.0f; // 4th dimensional projection offset
    float topologyBlend = 0.0f; // Crossfades between inner/seed A and B
    std::uint32_t enable4D = 0; // Toggle full 6-plane 4D manifold
    std::uint32_t enable4DB = 0; // Secondary 4D manifold target for smooth homotopy morphing
    float rotorAlpha = 0.0f;    // Left isoclinic rotor angle alpha in radians (SO(4) rotation)
    float rotorBeta = 0.0f;     // Right isoclinic rotor angle beta in radians (SO(4) rotation)
    float rotorTheta4D = 0.0f;  // Hyperplane rotation angle around the zw-plane
    std::uint32_t outputMode = 0; // 0 = 1D knot, 1 = 3D volume
    std::uint32_t outputModeB = 0;
    std::uint32_t attractorMode = 0;
    float attractorStrength = 0.0f;
    std::uint32_t instanceMeshType = 0; // 0=Sphere, 1=TorusKnot, 2=Cube, 3=Cylinder

    // WCA-inspired seed stages
    float seedWidth = 0.72f;
    float seedAlpha = 0.30f;
    float seedBaseline = 0.055f;
    float seedPulseFrequency = 0.19f;
    float seedPulseAmplitude = 0.35f;

    // The color stage becomes a material-coordinate stage in 3D.
    float hueRotation = 0.04f;
    float hueSpeed = 0.035f;

    // Presentation-neutral feature scales. These are part of world-program
    // output shaping, not camera placement or renderer state.
    float worldScale = 1.0f;
    float radiusScale = 1.0f;
    // Presentation-only contour observation of the canonical relief field.
    // Zero disables it; positive values count levels per linear luminance unit.
    // Retained in authored/captured state, excluded from the OpenCL GPU ABI.
    float reliefContourDensity = 0.0f;
    // Presentation-only style: 0 ink, 1 signed L-0.5, 2 major/minor contours.
    std::uint32_t reliefContourStyle = 0U;
    
    // Phenomenology Engine (Aesthetic Post-Processing)
    float chromaBreath = 0.0f; // Pulsates saturation
    float lumaDrift = 0.0f;    // Drifts hue while locking luminance
    float paletteBlend = 0.0f; // Interpolates between multiple worldviews
    std::array<float, 4> audioSpectrum{}; // (Bass, LowMid, HighMid, Treble)
    
    float volumetricDensityScale = 0.65f; // Scale multiplier for participating media fog density
    float fogAnisotropy = 0.45f;          // Henyey-Greenstein scattering asymmetry g in (-1, 1)
    StageResolutionScale auxiliaryResolutionScale = StageResolutionScale::Full; // Decoupled auxiliary grid rate

    // Kinetic Flow-Field Advection & Streamlines
    float advectionSpeed = 0.0f;    // 0 = static trajectory, >0 = streamflow advection
    float curlStrength = 0.0f;      // Strength of 3D curl velocity field
    float trailDecay = 0.95f;       // Kinetic velocity / trail persistence factor
    float kineticViscosity = 0.1f;  // Fluid damping

    // Thin-Film Optical Wave Iridescence & Structural Coloration
    float filmThickness = 480.0f;   // Optical film thickness d in nanometers (100nm to 1200nm)
    float filmIOR = 1.45f;          // Refractive index n_film of the thin dielectric layer

    float temporalFeedbackBlend = 0.0f; // 0 = finite orbit (stateless), 1 = full temporal feedback
    float temporalFade = 0.98f;         // Energy decay factor per sub-step in (0, 1]
    float temporalFixedDelta = 0.0166667f; // Fixed sub-stepping interval (e.g. 60Hz)
    bool temporalReset = false; // Set to true for 1 frame to clear the ghost buffer
    std::uint32_t stageMask = learn_mode::kAllStages;
};

// Conservative producer-input equality: observation-only edits must not
// preempt cadence or advance recursive feedback. Relief contour density/style
// are excluded; every other authored field retains its existing dirty behavior.
// Keep this semantic comparison beside the schema, without GPU packing or
// byte-wise comparison (padding is not program state).
[[nodiscard]] constexpr bool equivalentProducerParams(
    const SculptureProgramParams& lhs,
    const SculptureProgramParams& rhs) noexcept {
    const auto equivalentAffine = [](const ComplexAffine& a, const ComplexAffine& b) {
        return a.scaleReal == b.scaleReal && a.scaleImag == b.scaleImag
            && a.offsetReal == b.offsetReal && a.offsetImag == b.offsetImag;
    };
    return equivalentAffine(lhs.pre, rhs.pre)
        && equivalentAffine(lhs.post, rhs.post)
        && lhs.inner == rhs.inner && lhs.innerB == rhs.innerB
        && lhs.reduction == rhs.reduction && lhs.reductionB == rhs.reductionB
        && lhs.seedType == rhs.seedType && lhs.seedTypeB == rhs.seedTypeB
        && lhs.colorEffect == rhs.colorEffect
        && lhs.feedbackIterations == rhs.feedbackIterations
        && lhs.patternSpeed == rhs.patternSpeed && lhs.latentW == rhs.latentW
        && lhs.rotorAlpha == rhs.rotorAlpha && lhs.rotorBeta == rhs.rotorBeta && lhs.rotorTheta4D == rhs.rotorTheta4D
        && lhs.topologyBlend == rhs.topologyBlend && lhs.enable4D == rhs.enable4D
        && lhs.enable4DB == rhs.enable4DB
        && lhs.outputMode == rhs.outputMode && lhs.outputModeB == rhs.outputModeB
        && lhs.attractorMode == rhs.attractorMode && lhs.attractorStrength == rhs.attractorStrength
        && lhs.instanceMeshType == rhs.instanceMeshType
        && lhs.seedWidth == rhs.seedWidth && lhs.seedAlpha == rhs.seedAlpha
        && lhs.seedBaseline == rhs.seedBaseline
        && lhs.seedPulseFrequency == rhs.seedPulseFrequency
        && lhs.seedPulseAmplitude == rhs.seedPulseAmplitude
        && lhs.hueRotation == rhs.hueRotation && lhs.hueSpeed == rhs.hueSpeed
        && lhs.worldScale == rhs.worldScale && lhs.radiusScale == rhs.radiusScale
        && lhs.chromaBreath == rhs.chromaBreath && lhs.lumaDrift == rhs.lumaDrift
        && lhs.paletteBlend == rhs.paletteBlend && lhs.audioSpectrum == rhs.audioSpectrum
        && lhs.volumetricDensityScale == rhs.volumetricDensityScale
        && lhs.fogAnisotropy == rhs.fogAnisotropy
        && lhs.auxiliaryResolutionScale == rhs.auxiliaryResolutionScale
        && lhs.advectionSpeed == rhs.advectionSpeed && lhs.curlStrength == rhs.curlStrength
        && lhs.trailDecay == rhs.trailDecay && lhs.kineticViscosity == rhs.kineticViscosity
        && lhs.filmThickness == rhs.filmThickness && lhs.filmIOR == rhs.filmIOR
        && lhs.temporalFeedbackBlend == rhs.temporalFeedbackBlend
        && lhs.temporalFade == rhs.temporalFade
        && lhs.temporalFixedDelta == rhs.temporalFixedDelta
        && lhs.temporalReset == rhs.temporalReset && lhs.stageMask == rhs.stageMask;
}

// Authored identity also includes presentation intent for diffs, saved state
// and captures. Share the producer field list so these policies cannot drift.
[[nodiscard]] constexpr bool equivalentProgramParams(
    const SculptureProgramParams& lhs,
    const SculptureProgramParams& rhs) noexcept {
    return equivalentProducerParams(lhs, rhs)
        && lhs.reliefContourDensity == rhs.reliefContourDensity
        && lhs.reliefContourStyle == rhs.reliefContourStyle;
}

[[nodiscard]] constexpr bool operator==(
    const SculptureProgramParams& lhs,
    const SculptureProgramParams& rhs) noexcept {
    return equivalentProgramParams(lhs, rhs);
}

[[nodiscard]] constexpr bool operator!=(
    const SculptureProgramParams& lhs,
    const SculptureProgramParams& rhs) noexcept {
    return !equivalentProgramParams(lhs, rhs);
}

struct SculptureWorldState {
    int version = 1;
    float time = 0.0f;
    float morph = 0.0f;
    float cameraAngle = 0.0f;
    float cameraEye[3] = {0.0f, 0.0f, 15.0f};
    float cameraTarget[3] = {0.0f, 0.0f, 0.0f};
    float cameraUp[3] = {0.0f, 1.0f, 0.0f};
    // Captures set this explicitly. Live route framing may populate camera
    // coordinates without claiming ownership over manual/cinematic cameras.
    bool hasCameraOverride = false;
};

struct SculptureSample {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float radius = 0.0f;
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float a = 0.0f;
    float nx = 0.0f;
    float ny = 0.0f;
    float nz = 0.0f;
    float nw = 0.0f;
    float vx = 0.0f;
    float vy = 0.0f;
    float vz = 0.0f;
    float vw = 0.0f;
    std::uint32_t materialRole = 0U;
};

struct SculptureSnapshot {
    float time = 0.0f;
    float morph = 0.0f;
    std::array<SculptureSample, kSampleCount> samples{};
    std::vector<float> spatialBuffer; // Variable-rate volumetric fluid grid
    // Dense source-indexed CPU history, independent of compacted render order.
    // This private producer history is not part of the packed delivery ABI.
    std::vector<SculptureSourceSample> sourceHistory;
    SculptureTemporalFamily sourceHistoryFamily = SculptureTemporalFamily::Surface;
    StageResolutionScale sourceHistoryScale = StageResolutionScale::Full;
    bool sourceHistoryValid = false;
};

// Human-readable vocabulary belongs to the schema layer because presets, serializers,
// inspectors and the CPU oracle all describe the same mathematical program. These
// helpers own no evaluation cadence, GPU object, or renderer state.
[[nodiscard]] std::string_view name(InnerMap inner) noexcept;
[[nodiscard]] std::string_view name(ReductionMode reduction) noexcept;
[[nodiscard]] std::string_view name(StageResolutionScale scale) noexcept;
[[nodiscard]] std::string_view name(SeedType seed) noexcept;
[[nodiscard]] std::string_view name(ColorEffectType effect) noexcept;
[[nodiscard]] std::string getPipelineDescription(const SculptureProgramParams& params);

} // namespace epi_sculpt
