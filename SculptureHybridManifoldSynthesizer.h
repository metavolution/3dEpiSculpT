#pragma once

#include "EpimorphicReliefGenerator.h"
#include "Sculpture4DPolychoronHomotopy.h"
#include "SculpturePipelineCoordinator.h"
#include "SculptureVolumetricFieldCoupler.h"
#include "SculptureWorldProducer.h"
#include "SculptureWorldRecipe.h"
#include "SculptureProceduralGeometryRequest.h"
#include "mgfx/Engine.h"
#include "mgfx/Drawable.h"
#include "mgfx/vk/Mesh.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

namespace epi_sculpt {

enum class WorldPresentationMode : std::uint8_t {
    PointCloud = 0,         // Standard 3D procedural world primitive instancing
    ReliefManifold = 1,     // 2.5D / 3.5D continuous epimorphic relief surface
    HybridVolumetric = 2    // Both volumetric point cloud AND continuous relief manifold
};

[[nodiscard]] constexpr const char* name(WorldPresentationMode mode) noexcept {
    switch (mode) {
    case WorldPresentationMode::PointCloud: return "PointCloud";
    case WorldPresentationMode::ReliefManifold: return "ReliefManifold";
    case WorldPresentationMode::HybridVolumetric: return "HybridVolumetric";
    default: return "Unknown";
    }
}

// Complete private results: preparation cannot change the navigator's selected
// geometry, request identity or native owner. A null mesh means valid empty
// geometry or an explicit CPU-only request, rather than failed native allocation.
struct ReliefMeshCandidate {
    EpimorphicReliefData data{};
    std::shared_ptr<const mgfx::vk::Mesh> mesh{};
    ReliefMeshRequest request{};
};

struct PolychoronMeshCandidate {
    IsosurfaceMesh data{};
    std::shared_ptr<const mgfx::vk::Mesh> mesh{};
    PolychoronMeshRequest request{};
};

// Adapts relief / S^3 tube geometry and volumetric instance deliveries.
// The navigator owns publication; checked Drawable selection retains each shared
// Mesh through the existing Engine frame-slot lease and completion boundary.
class SculptureHybridManifoldSynthesizer {
public:
    // Synchronizes relief manifold metric projection and presentation mode from
    // the declarative WorldMathematicalAffinity table.
    static void syncReliefProjectionForRecipe(
        const WorldRecipe& recipe,
        EpimorphicReliefConfig& reliefConfig,
        WorldPresentationMode& presentationMode) noexcept;

    // Prepare CPU geometry first, then optionally its native mesh. No candidate
    // means unavailable/invalid geometry; allocation errors propagate. Both leave
    // published state untouched. Valid empty geometry is a publishable candidate.
    // The candidate carries exactly the request consumed during generation.
    // Recording-time lease-capacity preflight precedes native allocation and is
    // advisory; the selected owner must still be retained before publication.
    [[nodiscard]] static std::optional<ReliefMeshCandidate> prepareReliefMesh(
        mgfx::Engine* engine,
        const SculpturePipelineCoordinator& coordinator,
        const ReliefMeshRequest& request);

    // Same publication contract, including CPU-only tube generation and a valid
    // empty slice. The host supplies an explicitly sampled animation time.
    [[nodiscard]] static std::optional<PolychoronMeshCandidate> preparePolychoronMesh(
        mgfx::Engine* engine,
        const PolychoronMeshRequest& request);

    // Couples host-delivered world instances with the 3D volumetric fluid/memory
    // substrate using zero-waste bounded active-instance copying.
    [[nodiscard]] static const WorldSnapshotDelivery& prepareInstanceDelivery(
        const WorldSnapshotDelivery& delivery,
        const SculptureVolumetricMemory* volMem,
        SculptureVolumetricFieldCoupler& coupler,
        world_gpu_abi::WorldSnapshotGpu& coupledSnapshot,
        WorldSnapshotDelivery& coupledDelivery);

    // Synthesizes a fallback 4D polychoron world snapshot delivery when no
    // external OpenCL world producer snapshot is present.
    [[nodiscard]] static WorldSnapshotDelivery synthesizePolychoronDelivery(
        const Sculpture4DPolychoronHomotopy& polychoronHomotopy,
        const WorldRecipe& currentRecipe,
        float simulatedTime,
        std::uint64_t totalFramesCoordinated,
        world_gpu_abi::WorldSnapshotGpu& coupledSnapshot,
        WorldGenerationMetrics& polychoronMetrics,
        WorldSnapshotDelivery& coupledDelivery);
};

} // namespace epi_sculpt
