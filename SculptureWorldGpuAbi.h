// Versioned OpenCL/Vulkan transfer contract for bounded procedural worlds.
//
// This ABI is intentionally parallel to SculptureGpuAbi: compact epimorphism
// samples retain their stable format while dedicated world generators gain a
// transform-oriented output stream.

#pragma once

#include "SculptureWorldRecipe.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace epi_sculpt::world_gpu_abi {

// Version 3 adds the recipe-level mirror-face topology field in spare flag
// bits. Version 2 added opt-in per-axis instance scale; the high-bit instance
// flag keeps legacy uniform-scale records deterministic while producers migrate.
inline constexpr std::uint32_t kVersion = 3U;
inline constexpr std::uint32_t kRecipeBytes = 32U;
inline constexpr std::uint32_t kFrameBytes = 32U;
inline constexpr std::uint32_t kInstanceBytes = 96U;
inline constexpr std::uint32_t kAnisotropicScaleFlag = 0x80000000U;
inline constexpr std::uint32_t kUses4DProjectionFlag = 0x00000001U;
inline constexpr std::uint32_t kMirrorTopologyShift = 1U;
inline constexpr std::uint32_t kMirrorTopologyMask = 0x00000006U;
inline constexpr std::uint32_t kRecipeFlagsMask =
    kUses4DProjectionFlag | kMirrorTopologyMask;
// Covers the initial 64x36 Chladni grid and leaves room for a bounded forest.
inline constexpr std::uint32_t kMaxInstanceCount = 16384U;

// The recipe arrives inside each immutable delivery.  Keep decoding beside
// the bit contract so Vulkan presentation never needs a mutable mirror-mode
// override or a copy of the OpenCL recipe authority.
[[nodiscard]] constexpr MirrorCellTopology mirrorTopologyFromFlags(
    std::uint32_t flags) noexcept {
    const auto raw = (flags & kMirrorTopologyMask) >> kMirrorTopologyShift;
    return static_cast<MirrorCellTopology>(raw <= 3U ? raw : 0U);
}

enum class GeneratorId : std::uint32_t {
    OrbitSamples = 0U,
    KuramotoLattice = 1U,
    HopfFibers = 2U,
    GoldenPhyllotaxis = 3U,
    StrangeAttractorLanterns = 4U,
    Cell120Hologram = 5U,
    AmmannBeenkerField = 6U,
    ChladniMembrane = 7U,
    LichtenbergForest = 8U,
    EpimorphicGyroidMaze = 9U,
    HypercubeTesseract = 10U,
    ProceduralWoodland = 11U,
    BioluminescentSporeSwarm = 12U,
    NeuralSynapseLattice = 13U,
    KaliSetOrbits = 14U,
    MandelbulbFractal = 15U,
    DeepSeaSiphonophores = 16U,
    CalabiYauManifold = 17U,
    LSystemFractal = 18U,
    HydrogenAtomOrbitals = 19U,
    CliffordHopfSolitonLattice = 20U,
    PenroseTwistorFoliation = 21U,
    LieE8GossetLattice = 22U,
    ConformalRelief25D = 23U,
    ToroidalFoliation35D = 24U,
    DedekindEtaQuarticLattice4D = 25U,
    KuramotoHyperbolicCanopy35D = 26U,
    HyperbolicHoneycombHorosphere4D = 27U,
    Hecatonicosachoron120Cell4D = 28U,
    DiracMonopoleQuantumTunnel = 29U,
    Su2YangMillsInstanton = 30U,
    NeonBubblesphere = 31U,
};

enum class PrimitiveId : std::uint32_t {
    Sphere = 0U,
    TorusKnot = 1U,
    Cylinder = 2U,
    Tile = 3U,
    Cube = 4U,
    Pillar = 5U,
    BranchSegment = 6U,
    TrussStrut = 7U,
};

struct alignas(16) WorldRecipeGpu {
    std::uint32_t abiVersion = kVersion;
    std::uint32_t byteSize = kRecipeBytes;
    std::uint32_t generatorId = 0U;
    std::uint32_t primitiveId = 0U;
    std::uint32_t instanceBudget = 0U;
    std::uint32_t generatorHz = 30U;
    std::uint32_t materialBandCount = 3U;
    // bit 0: 4D projection; bits 1..2: MirrorCellTopology enum value.
    std::uint32_t flags = 0U;
};

struct alignas(16) WorldFrameGpu {
    std::uint32_t abiVersion = kVersion;
    std::uint32_t byteSize = kFrameBytes;
    std::uint32_t generation = 0U;
    std::uint32_t activeInstanceCount = 0U;
    float time = 0.0f;
    float deltaSeconds = 0.0f;
    float morph = 0.0f;
    float reserved = 0.0f;
};

// A shared mesh is transformed by one record. Position/scale, orientation,
// tint/emission, and primitive/material routing are all OpenCL-authored.
// axisScale is read only when flags contains kAnisotropicScaleFlag. Retaining
// positionScale.w preserves a compact, deterministic uniform-scale default
// for every existing world writer while allowing new generators to express
// actual truss/edge proportions without CPU geometry expansion.
struct alignas(16) WorldInstanceGpu {
    std::array<float, 4> positionScale{}; // xyz position, uniform scale
    std::array<float, 4> axisScale{1.0f, 1.0f, 1.0f, 1.0f}; // xyz local scale
    std::array<float, 4> orientation{};   // normalized xyzw quaternion
    std::array<float, 4> tintEmission{};  // rgb tint, emissive intensity
    std::array<float, 4> pbrOverride{-1.0f, -1.0f, -1.0f, 0.0f}; // x=metallic, y=roughness, z=reserved, w=reserved
    std::uint32_t primitiveId = 0U;
    std::uint32_t materialBand = 0U;
    std::uint32_t flags = 0U;
    std::uint32_t sourceIndex = 0U;
};

struct alignas(16) WorldSnapshotGpu {
    WorldFrameGpu frame{};
    WorldRecipeGpu recipe{};
    std::array<WorldInstanceGpu, kMaxInstanceCount> instances{};
};

[[nodiscard]] WorldRecipeGpu packWorldRecipe(const WorldRecipe& recipe) noexcept;
[[nodiscard]] bool isValid(const WorldRecipeGpu& recipe) noexcept;
// Validate delivered metadata without touching GPU-resident instance records.
[[nodiscard]] bool isValid(const WorldFrameGpu& frame, const WorldRecipeGpu& recipe) noexcept;
[[nodiscard]] bool isValid(const WorldSnapshotGpu& snapshot) noexcept;

static_assert(std::is_standard_layout_v<WorldRecipeGpu>);
static_assert(std::is_trivially_copyable_v<WorldRecipeGpu>);
static_assert(std::is_standard_layout_v<WorldFrameGpu>);
static_assert(std::is_trivially_copyable_v<WorldFrameGpu>);
static_assert(std::is_standard_layout_v<WorldInstanceGpu>);
static_assert(std::is_trivially_copyable_v<WorldInstanceGpu>);
static_assert(sizeof(WorldRecipeGpu) == kRecipeBytes);
static_assert(sizeof(WorldFrameGpu) == kFrameBytes);
static_assert(sizeof(WorldInstanceGpu) == kInstanceBytes);
static_assert(alignof(WorldInstanceGpu) == 16U);
static_assert(offsetof(WorldInstanceGpu, positionScale) == 0U);
static_assert(offsetof(WorldInstanceGpu, axisScale) == 16U);
static_assert(offsetof(WorldInstanceGpu, orientation) == 32U);
static_assert(offsetof(WorldInstanceGpu, tintEmission) == 48U);
static_assert(offsetof(WorldInstanceGpu, pbrOverride) == 64U);
static_assert(offsetof(WorldInstanceGpu, primitiveId) == 80U);
static_assert(offsetof(WorldSnapshotGpu, instances) == kRecipeBytes + kFrameBytes);

} // namespace epi_sculpt::world_gpu_abi
