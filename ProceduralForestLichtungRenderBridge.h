/**
 * ============================================================================
 * EPI2 / 3dEpiSculpTMorph - DEDICATED PROCEDURAL FOREST LICHTUNG RENDER BRIDGE
 * ============================================================================
 *
 * [ARCHITECTURAL NOTE: OPTION B1 — DEDICATED < 8 MB MULTI-ARCHETYPE BRIDGE]
 * ----------------------------------------------------------------------------
 * Keeps `SculptureWorldRenderBridge` untouched for existing sculpture routes
 * and provides a dedicated, ultra-lean (< 8 MB across 3 frame slots) Vulkan
 * instance & mesh archetype bridge tailored to the 8 botanical and geological
 * procedural mesh generators of `epi_sculpt::forest`:
 *
 *   0. TerrainValleyFloor      — Crack-free skirted terrain chunks with
 *                                mossy humus, wet granite, and footpath wear
 *   1. MossyBoulder            — Worley-chiseled granite boulder with moss cap
 *   2. TreeTrunkAndBranches    — Multi-species bifurcating trunk & root flare
 *   3. CanopyFoliageCluster    — Asymmetric phototropic leaf crown lobes
 *   4. BrackenFernFrond        — Multi-frond arched bracken fern rosette
 *   5. RootedBellFlower        — 5-petal cupped bell-flower with stem & leaves
 *   6. GrassAndUnderstoryClump — Quadratic-Bezier grass clump & woody shrub
 *   7. SwirlingPetalAndDebris  — Aerodynamic curved petal blade & floor litter
 *
 * Memory Efficiency vs. Sparse 24-Bucket World Bridge:
 *   - `SculptureWorldRenderBridge` allocates 24 * 16,384 * 112 B = 44.04 MB
 *     per slot (132.12 MB across 3 frame slots).
 *   - `ProceduralForestLichtungRenderBridge` packs all 8 archetypes into a
 *     single contiguous prefix-summed `mgfx::vk::InstanceData` arena of at
 *     most 16,384 instances (1.84 MB per slot, 5.51 MB across 3 frame slots)
 *     plus ~0.5 MB of shared static archetype VBO/IBOs = < 6.2 MB total
 *     (>95% VRAM reduction).
 * ============================================================================
 */

#pragma once

#include "ProceduralForestBotanicalHierarchy.h"
#include "ProceduralForestDynamicsAndLighting.h"
#include "ProceduralForestEcologyScatter.h"
#include "ProceduralForestFields.h"
#include "ProceduralForestGeometry.h"
#include "ProceduralForestLichtungAbi.h"
#include "ProceduralForestSpatialLodAndBridge.h"
#include "SculptureIsosurface.h"

#include <mgfx/Drawable.h>
#include <mgfx/Engine.h>
#include <mgfx/FrameSlotBufferCache.h>
#include <mgfx/MaterialLibrary.h>
#include <mgfx/vk/Instance.h>
#include <mgfx/vk/Mesh.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace epi_sculpt::forest {

enum class ForestMeshArchetype : std::uint8_t {
    TerrainValleyFloor      = 0U,
    MossyBoulder            = 1U,
    TreeTrunkAndBranches    = 2U,
    CanopyFoliageCluster    = 3U,
    BrackenFernFrond        = 4U,
    RootedBellFlower        = 5U,
    GrassAndUnderstoryClump = 6U,
    SwirlingPetalAndDebris  = 7U,
    Count                   = 8U
};

struct ForestHostArchetypeGeometry {
    ForestMeshArchetype archetype = ForestMeshArchetype::TerrainValleyFloor;
    std::vector<mgfx::vk::Vertex> vertices;
    std::vector<std::uint32_t> indices;
    SculptureVec3 aabbMin{-1.0f, -1.0f, -1.0f};
    SculptureVec3 aabbMax{ 1.0f,  1.0f,  1.0f};

    [[nodiscard]] std::size_t byteFootprint() const noexcept {
        return vertices.size() * sizeof(mgfx::vk::Vertex)
             + indices.size() * sizeof(std::uint32_t);
    }

    [[nodiscard]] bool empty() const noexcept {
        return vertices.empty() || indices.empty();
    }
};

struct ForestArchetypeDrawSlice {
    ForestMeshArchetype archetype = ForestMeshArchetype::TerrainValleyFloor;
    std::uint32_t firstInstance = 0U;
    std::uint32_t instanceCount = 0U;
    std::uint32_t featureFlags = 0U;
    bool doubleSided = false;
};

struct ForestLichtungBridgeMetrics {
    std::uint64_t generation = 0U;
    std::uint32_t sourceInstanceCount = 0U;
    std::uint32_t acceptedInstanceCount = 0U;
    std::uint32_t culledOrDitheredCount = 0U;
    std::uint32_t activeArchetypeBatchCount = 0U;
    std::array<std::uint32_t, static_cast<std::size_t>(ForestMeshArchetype::Count)>
        perArchetypeInstanceCounts{};
    std::uint32_t totalArchetypeVertexCount = 0U;
    std::uint32_t totalArchetypeIndexCount = 0U;
    std::uint64_t archetypeGeometryBytes = 0U;
    std::uint64_t singleSlotInstanceArenaBytes = 0U;
    std::uint64_t tripleSlotTotalGpuBytes = 0U;
    float vramSavingsRatioVsSparseWorldBridge = 0.0f;
    float rebuildMilliseconds = 0.0f;
};

class ProceduralForestLichtungRenderBridge {
public:
    static constexpr std::size_t kArchetypeCount =
        static_cast<std::size_t>(ForestMeshArchetype::Count);
    static constexpr std::size_t kMaxInstanceCapacity = 16384U;
    static constexpr std::uint32_t kFrameSlotCount = 3U;

    // 8 MB strict ceiling for 3-slot instance arena + all 8 static botanical meshes
    static constexpr std::uint64_t kMaxTotalGpuBudgetBytes = 8ULL * 1024ULL * 1024ULL;

    // Reference sparse 24-bucket world bridge footprint (24 * 16384 * 112 * 3 = 132.12 MB)
    static constexpr std::uint64_t kSparseWorldBridgeTripleSlotBytes =
        24ULL * 16384ULL * sizeof(mgfx::vk::InstanceData) * kFrameSlotCount;

    // Feature flag matching shaders/mesh.vert bit 8 (Procedural Wind Foliage Sway)
    static constexpr std::uint32_t kFeatureFlagWindSway = 256U;

    ProceduralForestLichtungRenderBridge() = default;
    ~ProceduralForestLichtungRenderBridge() = default;

    ProceduralForestLichtungRenderBridge(const ProceduralForestLichtungRenderBridge&) = delete;
    ProceduralForestLichtungRenderBridge& operator=(const ProceduralForestLichtungRenderBridge&) = delete;

    // Converts an IsosurfaceMesh into canonical 96-byte mgfx::vk::Vertex records
    // with orthonormal Gram-Schmidt tangents, UV parameterization, and tinting.
    [[nodiscard]] static ForestHostArchetypeGeometry convertIsosurfaceToHostArchetype(
        ForestMeshArchetype archetype,
        const IsosurfaceMesh& sourceMesh,
        const mgfx::math::vec4& baseVertexColor = mgfx::math::vec4(1.0f));

    // Synthesizes all 8 canonical botanical & geological archetype geometries on CPU
    // (100% device-free and deterministic for unit tests or lazy Vulkan upload).
    void buildHostArchetypeGeometries(
        const ForestFieldGrid& fieldGrid,
        const ForestCameraFrustum& frustum = {},
        std::uint32_t seed = 0x4C494348U);

    // Rebuilds the prefix-summed contiguous mgfx::vk::InstanceData stream and
    // per-archetype draw slices from the stratified forest ecosystem state.
    void rebuild(
        std::uint64_t generation,
        const ForestFieldGrid& fieldGrid,
        const ForestEcosystemPopulation& population,
        const ForestCameraFrustum& frustum = {},
        const ForestSeasonWeatherConfig& seasonWeather = {},
        std::span<const ForestSwirlingPetalGpu> swirlingPetals = {},
        float timeSeconds = 0.0f);

    // Lazily uploads the 8 botanical archetype meshes and default PBR materials
    // to the Vulkan engine, stages the current generation's prefix-summed
    // instance arena into the 3-slot FrameSlotBufferCache, and emits 1 instanced
    // draw call per active archetype slice.
    [[nodiscard]] bool draw(
        mgfx::Engine& engine,
        mgfx::Drawable& drawable,
        std::span<const mgfx::MaterialHandle> customMaterials = {},
        const mgfx::math::mat4& rootTransform = mgfx::math::mat4(1.0f));

    void reset() noexcept;

    [[nodiscard]] const std::array<ForestHostArchetypeGeometry, kArchetypeCount>&
    hostArchetypes() const noexcept {
        return hostArchetypes_;
    }

    [[nodiscard]] const std::array<ForestArchetypeDrawSlice, kArchetypeCount>&
    archetypeSlices() const noexcept {
        return archetypeSlices_;
    }

    [[nodiscard]] std::span<const mgfx::vk::InstanceData> instances() const noexcept {
        return instances_;
    }

    [[nodiscard]] const ForestLichtungBridgeMetrics& metrics() const noexcept {
        return metrics_;
    }

    [[nodiscard]] bool hasHostArchetypes() const noexcept {
        return hostArchetypesBuilt_;
    }

private:
    [[nodiscard]] bool ensureGpuArchetypeMeshes(mgfx::Engine& engine);
    [[nodiscard]] bool ensureDefaultMaterials(mgfx::Engine& engine);

    bool hostArchetypesBuilt_ = false;
    std::uint32_t builtTerrainGridWidth_ = 0U;
    std::uint32_t builtTerrainGridHeight_ = 0U;
    std::uint32_t builtSeed_ = 0U;

    std::array<ForestHostArchetypeGeometry, kArchetypeCount> hostArchetypes_{};
    std::array<ForestArchetypeDrawSlice, kArchetypeCount> archetypeSlices_{};
    std::vector<mgfx::vk::InstanceData> instances_;
    ForestLichtungBridgeMetrics metrics_{};

    // Vulkan device-local resources (lazily allocated only when draw() is invoked)
    mgfx::Engine* boundEngine_ = nullptr;
    std::array<std::shared_ptr<mgfx::vk::Mesh>, kArchetypeCount> gpuMeshes_{};
    std::array<mgfx::MaterialHandle, kArchetypeCount> defaultMaterials_{};
    bool defaultMaterialsCreated_ = false;
    mgfx::FrameSlotBufferCache instanceCache_{};
};

} // namespace epi_sculpt::forest
