// Renderer-neutral presentation recipe for a bounded procedural world.
//
// A SculptureProgramParams describes mathematical evolution.  A WorldRecipe
// describes how that evolving state becomes a visible world without leaking
// Vulkan meshes or OpenCL handles into the preset/serialization layer.

#pragma once

#include "SculptureProgramSchema.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

namespace epi_sculpt {

enum class WorldGeneratorKind : std::uint8_t {
    OrbitSamples,
    KuramotoLattice,
    HopfFibers,
    GoldenPhyllotaxis,
    StrangeAttractorLanterns,
    Cell120Hologram,
    AmmannBeenkerField,
    ChladniMembrane,
    LichtenbergForest,
    EpimorphicGyroidMaze,
    HypercubeTesseract,
    ProceduralWoodland,
    BioluminescentSporeSwarm,
    NeuralSynapseLattice,
    KaliSetOrbits,
    MandelbulbFractal,
    DeepSeaSiphonophores,
    CalabiYauManifold,
    LSystemFractal,
    HydrogenAtomOrbitals,
    CliffordHopfSolitonLattice,
    PenroseTwistorFoliation,
    LieE8GossetLattice,
    ConformalRelief25D,
    ToroidalFoliation35D,
    DedekindEtaQuarticLattice4D,
    KuramotoHyperbolicCanopy35D,
    HyperbolicHoneycombHorosphere4D,
    Hecatonicosachoron120Cell4D,
    DiracMonopoleQuantumTunnel,
    Su2YangMillsInstanton,
    // Three-scale composition: faceted core, mirror/truss shell, and orbiting
    // luminous particles. It writes existing WorldInstanceGpu records.
    NeonBubblesphere,
};

inline constexpr std::size_t kWorldGeneratorCount =
    static_cast<std::size_t>(WorldGeneratorKind::NeonBubblesphere) + 1U;

enum class WorldPrimitiveKind : std::uint8_t {
    Sphere,
    TorusKnot,
    Cylinder,
    Tile,
    Cube,
    Pillar,
    BranchSegment,
    // Thin unit-cylinder mesh reserved for luminous graph/truss members.
    // Keeping it separate from Cylinder preserves existing world proportions.
    TrussStrut,
};

// This is intentionally not a hardware capability query.  It states whether
// the requested visual language has a complete generator-to-presentation
// route in the shared application, preventing a poetic preset name from being
// mistaken for an implemented topology.
enum class WorldRecipeDelivery : std::uint8_t {
    ExistingCompactPath,
    DedicatedGeneratorRequired,
};

// The output substrate is independent of the mathematical generator.  A
// recipe can therefore describe a bounded instance world, a camera-space
// implicit field, or an experiment manifest without leaking OpenCL/Vulkan
// implementation details into authored preset metadata.
enum class WorldPresentationKind : std::uint8_t {
    InstancedWorld,
    ImplicitField,
    ExperimentResources,
};

// A compact face grammar for finite reflected cells. It deliberately describes
// the local graph, not a physically recursive mirror simulation: OpenCL can
// render any option within one fixed world-instance budget.
enum class MirrorCellTopology : std::uint8_t {
    Triangular = 0U,
    Quadrilateral = 1U,
    Pentagonal = 2U,
    Hexagonal = 3U,
};

inline constexpr std::size_t kMirrorCellTopologyCount =
    static_cast<std::size_t>(MirrorCellTopology::Hexagonal) + 1U;

struct WorldRecipe {
    WorldGeneratorKind generator = WorldGeneratorKind::OrbitSamples;
    WorldPrimitiveKind primitive = WorldPrimitiveKind::Sphere;
    WorldRecipeDelivery delivery = WorldRecipeDelivery::ExistingCompactPath;
    WorldPresentationKind presentation = WorldPresentationKind::InstancedWorld;
    std::uint32_t instanceBudget = 0U;
    std::uint32_t generatorHz = 30U;
    std::uint8_t materialBandCount = 3U;
    bool uses4DProjection = false;
    MirrorCellTopology mirrorTopology = MirrorCellTopology::Triangular;

    // Compare declared recipe values, never native resource/device identity.
    // Defaulting includes future fields automatically and preserves aggregation.
    [[nodiscard]] constexpr bool operator==(const WorldRecipe&) const noexcept = default;

    [[nodiscard]] constexpr bool needsDedicatedGenerator() const noexcept {
        return delivery == WorldRecipeDelivery::DedicatedGeneratorRequired;
    }

    [[nodiscard]] constexpr bool usesImplicitField() const noexcept {
        return presentation == WorldPresentationKind::ImplicitField;
    }
};

enum class WorldRouteAvailability : std::uint8_t {
    Implemented,
    Planned,
};

inline constexpr std::size_t kWorldCatalogAliasCapacity = 8U;

// One declarative record is the source of truth for authored ids, friendly
// aliases, bounded compute cost, and renderer-neutral presentation routing.
// Keeping this metadata together prevents CLI demos and the preset library
// from silently assigning different topologies to the same mathematical idea.
struct WorldCatalogEntry {
    std::string_view canonicalPresetId;
    std::string_view title;
    std::array<std::string_view, kWorldCatalogAliasCapacity> aliases{};
    WorldRouteAvailability availability = WorldRouteAvailability::Planned;
    WorldGeneratorKind generator = WorldGeneratorKind::OrbitSamples;
    WorldPrimitiveKind primitive = WorldPrimitiveKind::Sphere;
    WorldRecipeDelivery delivery = WorldRecipeDelivery::DedicatedGeneratorRequired;
    WorldPresentationKind presentation = WorldPresentationKind::InstancedWorld;
    std::uint32_t instanceBudget = 0U;
    std::uint32_t generatorHz = 30U;
    std::uint8_t materialBandCount = 3U;
    bool uses4DProjection = false;
    MirrorCellTopology mirrorTopology = MirrorCellTopology::Triangular;

    [[nodiscard]] constexpr bool matches(std::string_view candidate) const noexcept {
        if (candidate == canonicalPresetId || candidate == title) return true;
        const auto candSlash = candidate.rfind('/');
        const std::string_view candBase = (candSlash != std::string_view::npos)
            ? candidate.substr(candSlash + 1)
            : candidate;
        const auto canonSlash = canonicalPresetId.rfind('/');
        const std::string_view canonBase = (canonSlash != std::string_view::npos)
            ? canonicalPresetId.substr(canonSlash + 1)
            : canonicalPresetId;
        if (!candBase.empty() && candBase == canonBase) return true;
        for (const std::string_view alias : aliases) {
            if (alias.empty()) continue;
            if (candidate == alias || candBase == alias) return true;
            const auto aliasSlash = alias.rfind('/');
            const std::string_view aliasBase = (aliasSlash != std::string_view::npos)
                ? alias.substr(aliasSlash + 1)
                : alias;
            if (!candBase.empty() && candBase == aliasBase) return true;
        }
        return false;
    }

    [[nodiscard]] constexpr bool hasCompleteRoute() const noexcept {
        return availability == WorldRouteAvailability::Implemented;
    }

    [[nodiscard]] constexpr WorldRecipe recipe() const noexcept {
        return {
            .generator = generator,
            .primitive = primitive,
            .delivery = delivery,
            .presentation = presentation,
            .instanceBudget = instanceBudget,
            .generatorHz = generatorHz,
            .materialBandCount = materialBandCount,
            .uses4DProjection = uses4DProjection,
            .mirrorTopology = mirrorTopology,
        };
    }
};

// A root 2D epimorphism pattern and a 3D world recipe answer different
// questions: the pattern owns the mathematical parameters, while the recipe
// owns a bounded OpenCL-to-Vulkan delivery topology.  Keeping this bridge as
// immutable data prevents a name lookup from accidentally replacing either.
struct PatternWorldBinding {
    std::string_view patternId;
    std::string_view preferredWorldAlias;
    // Optional because most pattern-to-world choices need no geometry change.
    // When present, the existing world ABI carries this finite cell grammar to
    // OpenCL and to immutable delivery metadata for Vulkan presentation.
    std::optional<MirrorCellTopology> mirrorTopologyOverride;

    [[nodiscard]] constexpr bool matches(std::string_view candidate) const noexcept {
        if (candidate == patternId) return true;
        const auto candidateSlash = candidate.rfind('/');
        const std::string_view candidateBase = candidateSlash == std::string_view::npos
            ? candidate
            : candidate.substr(candidateSlash + 1U);
        return !candidateBase.empty() && candidateBase == patternId;
    }
};

[[nodiscard]] std::span<const WorldCatalogEntry> worldCatalog() noexcept;
[[nodiscard]] const WorldCatalogEntry* findWorldCatalogEntry(
    std::string_view presetId) noexcept;
[[nodiscard]] const WorldCatalogEntry* findAvailableWorldCatalogEntry(
    std::string_view presetId) noexcept;

// Curated source-preserving bridges from the root pattern catalogue to the
// implemented 3D world catalogue.  The returned world is always available;
// callers still obtain mathematical parameters from the original Pattern.
[[nodiscard]] std::span<const PatternWorldBinding> patternWorldBindings() noexcept;
[[nodiscard]] const PatternWorldBinding* findPatternWorldBinding(
    std::string_view patternId) noexcept;
[[nodiscard]] const WorldCatalogEntry* findPreferredWorldCatalogEntryForPattern(
    std::string_view patternId) noexcept;

// Resolves an authored identity to a bounded world recipe.  Unknown and
// built-in presets deliberately retain the proven compact orbit-sample path.
[[nodiscard]] WorldRecipe makeWorldRecipe(
    std::string_view presetId,
    const SculptureProgramParams& params) noexcept;

[[nodiscard]] std::string_view worldGeneratorName(WorldGeneratorKind kind) noexcept;
[[nodiscard]] std::string_view worldPrimitiveName(WorldPrimitiveKind kind) noexcept;
[[nodiscard]] std::string_view mirrorCellTopologyName(MirrorCellTopology topology) noexcept;

struct WorldMaterialBandStyle {
    std::array<float, 4> baseColor{1.0f, 1.0f, 1.0f, 1.0f};
    float metallic = 0.5f;
    float roughness = 0.2f;
    std::array<float, 3> emission{0.0f, 0.0f, 0.0f};
};

// OpenCL writes this compact semantic role into each instance's materialBand.
// Presentation may change colour and atmosphere, but it must not reinterpret
// these roles as geometry categories or introduce a second world generator.
enum class WorldMaterialRole : std::uint32_t {
    Form = 0U,       // Core facets and primary orbital mass.
    Structure = 1U,  // Truss members and connective lattice.
    Accent = 2U,     // Sparse knot nodes and high-energy particles.
};

inline constexpr std::size_t kWorldMaterialRoleCount = 3U;

[[nodiscard]] constexpr std::size_t worldMaterialRoleIndex(
    WorldMaterialRole role) noexcept {
    return static_cast<std::size_t>(role);
}

using WorldMaterialPalette = std::array<WorldMaterialBandStyle, kWorldMaterialRoleCount>;

// Keep palette construction role-addressed at authoring sites. This prevents a
// visually plausible but semantically wrong swap between core, truss, and
// orbital-accent treatments as more procedural worlds are added.
[[nodiscard]] constexpr WorldMaterialPalette makeWorldMaterialPalette(
    WorldMaterialBandStyle form,
    WorldMaterialBandStyle structure,
    WorldMaterialBandStyle accent) noexcept {
    WorldMaterialPalette palette{};
    palette[worldMaterialRoleIndex(WorldMaterialRole::Form)] = form;
    palette[worldMaterialRoleIndex(WorldMaterialRole::Structure)] = structure;
    palette[worldMaterialRoleIndex(WorldMaterialRole::Accent)] = accent;
    return palette;
}

[[nodiscard]] WorldMaterialPalette lookupWorldMaterialPalette(WorldGeneratorKind kind) noexcept;

// Renderer-neutral post controls paired with a world material palette.  The
// values are deliberately multipliers so the application can preserve its
// authored mGFX post baseline while a delivered world changes visual density.
// Only generators that explicitly opt in may use topology as a selector.
struct WorldPostProcessPreset {
    bool topologyAware = false;
    float fogStrengthScale = 1.0f;
    float fogDistanceDensityScale = 1.0f;
    float fogInScatterScale = 1.0f;
    float vignetteScale = 1.0f;
    std::array<float, 3> fogTint{0.55f, 0.62f, 0.72f};
};

struct WorldPresentationPreset {
    WorldMaterialPalette materials{};
    WorldPostProcessPreset post{};
};

// This is a pure style lookup, not a second geometry or simulation lane.
// Presentation reads it only after OpenCL has completed an immutable world
// snapshot.  NeonBubblesphere maps sparse triangular cells to clearer, more
// emissive treatment and dense hexagons to softer, fog-balanced treatment.
[[nodiscard]] WorldPresentationPreset lookupWorldPresentationPreset(
    WorldGeneratorKind kind,
    MirrorCellTopology topology = MirrorCellTopology::Triangular) noexcept;

// -----------------------------------------------------------------------------
// [EDUCATIONAL NOTE] Unified Mathematical Generator & Epimorphic Affinity Registry
// -----------------------------------------------------------------------------
// Bridges the 6-stage epimorphic pipeline vocabulary (InnerMap, ReductionMode,
// SeedType, ColorEffectType, 4D SO(4) isoclinic rotors, and non-Euclidean relief
// metric projections) with the 32 bounded 3D/4D procedural world generators.
//
// Replacing ad-hoc substring heuristics in world selection with this immutable,
// table-driven registry guarantees that every procedural world carries an
// authoritative mathematical specification, execution capability profile, and
// Living Textbook formula summary.
// -----------------------------------------------------------------------------

enum class WorldTopologicalDomain : std::uint8_t {
    Euclidean3D = 0U,           // Standard R^3 orbital, kinetic, or botanical manifolds
    ConformalRelief25D = 1U,    // Non-Euclidean 2.5D conformal/projective disk elevations
    FoliatedManifold35D = 2U,   // 3.5D toroidal or hyperbolic foliated chambers
    CoxeterPolytope4D = 3U,     // 4D regular polychora (8/16/24/120/600-cell) & E8 root lattices
    HopfTwistorFibration4D = 4U,// S^3 -> S^2 Hopf fibrations, Clifford tori & Penrose twistors
    GaugeFieldSoliton4D = 5U,   // Dirac monopoles, SU(2) Yang-Mills BPST instantons & skyrmions
    QuasicrystalAperiodic = 6U, // Ammann-Beenker & higher-dimensional cut-and-project tilings
    ImplicitRelativistic = 7U,  // Camera-space general-relativistic / SDF raymarched fields
};

namespace world_capability {
inline constexpr std::uint32_t kNone                   = 0U;
inline constexpr std::uint32_t kOpenClDedicatedKernel  = 1U << 0U;
inline constexpr std::uint32_t kGpuVolumetricCoupling  = 1U << 1U;
inline constexpr std::uint32_t kGpu4DHomotopy          = 1U << 2U;
inline constexpr std::uint32_t kHybridReliefCompanion  = 1U << 3U;
inline constexpr std::uint32_t kImplicitFieldRaymarch  = 1U << 4U;
} // namespace world_capability

struct WorldMathematicalAffinity {
    InnerMap primaryInner = InnerMap::Sinh;
    InnerMap secondaryInner = InnerMap::Identity;
    ReductionMode primaryReduction = ReductionMode::Torus;
    SeedType primarySeed = SeedType::Border;
    ColorEffectType primaryColorEffect = ColorEffectType::StandardHSV;
    WorldTopologicalDomain domain = WorldTopologicalDomain::Euclidean3D;
    std::uint32_t capabilityFlags =
        world_capability::kOpenClDedicatedKernel | world_capability::kGpuVolumetricCoupling;

    // Maps 1:1 to ReliefManifoldProjection (0=Planar, 1=PoincareDisk, 2=KleinBeltrami,
    // 3=RiemannSphere, 4=ToroidalChamber, 5=HyperbolicPseudosphere) without coupling
    // the schema/authoring layer to EpimorphicReliefGenerator.h.
    std::uint8_t preferredReliefProjection = 0U;
    bool hasDedicatedReliefProjection = false;
    bool autoPromoteHybridVolumetric = false;

    // Maps 1:1 to PolychoronKind (0=Tesseract8Cell, 1=Hexadecachoron16,
    // 2=Icositetrachoron24, 3=Hexacosichoron600, 4=Hecatonicosachoron120, 5=CliffordTorus).
    std::uint8_t preferredPolychoron = 0U;

    // Canonical 3D/4D mathematical state defaults
    std::uint32_t defaultEnable4D = 0U;
    std::uint32_t defaultOutputMode = 0U;
    std::uint32_t defaultInstanceMeshType = 0U;
    std::uint32_t defaultAttractorMode = 0U;
    float defaultAttractorStrength = 0.0f;
    float defaultRotorAlpha = 0.0f;
    float defaultRotorBeta = 0.0f;
    float defaultRotorTheta4D = 0.0f;
    float defaultLatentW = 0.0f;
    float defaultTopologyBlend = 0.0f;
    float defaultPatternSpeed = 0.075f;

    // Concise Living Textbook mathematical formula summary for HUD & inspection
    std::string_view canonicalEquation = "z_{n+1} = R(P(f(T(z_n))) + S(z_0))";
};

[[nodiscard]] const WorldMathematicalAffinity& lookupWorldMathematicalAffinity(
    WorldGeneratorKind kind) noexcept;

void applyWorldMathematicalAffinity(
    WorldGeneratorKind kind,
    SculptureProgramParams& params) noexcept;

// Deterministic, table-driven resolution from any catalog preset ID or alias to
// its canonical SculptureProgramParams (replacing ad-hoc substring heuristics).
[[nodiscard]] SculptureProgramParams resolveWorldProgramParams(
    std::string_view presetId) noexcept;

} // namespace epi_sculpt

