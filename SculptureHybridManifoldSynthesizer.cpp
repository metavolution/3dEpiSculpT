#include "SculptureHybridManifoldSynthesizer.h"
#include "mgfx/vk/FrameLoop.h"

#include <algorithm>
#include <cstring>
#include <limits>

namespace epi_sculpt {

namespace {

// Mesh uses uint32 counts and indexed triangles. Reject partial/malformed output
// before native allocation; only two empty arrays describe an empty success.
[[nodiscard]] bool hasCompleteTriangles(
    std::size_t vertexCount,
    const std::vector<std::uint32_t>& indices) noexcept {
    if (vertexCount == 0U || indices.empty()) {
        return vertexCount == 0U && indices.empty();
    }
    constexpr auto maxCount = std::numeric_limits<std::uint32_t>::max();
    return vertexCount <= maxCount && indices.size() <= maxCount
        && indices.size() % 3U == 0U
        && std::all_of(indices.begin(), indices.end(), [vertexCount](std::uint32_t index) {
            return index < vertexCount;
        });
}

[[nodiscard]] bool canPrepareNativeMesh(
    mgfx::Engine& engine, std::size_t vertexCount, std::size_t indexCount) noexcept {
    // Counts have passed uint32 admission. This matches Mesh's two requested
    // buffer sizes, not allocator padding, upload scratch or total VRAM.
    const auto bytes = static_cast<std::uint64_t>(vertexCount) * sizeof(mgfx::vk::Vertex)
        + static_cast<std::uint64_t>(indexCount) * sizeof(std::uint32_t);
    // Explicit preparation outside a frame remains useful. During recording,
    // avoid constructing/uploading a new owner that cannot fit the current lease.
    return !engine.frameLoop().frameActive()
        || engine.canRetainAdditionalFramePayloadResources(bytes);
}

} // namespace

void SculptureHybridManifoldSynthesizer::syncReliefProjectionForRecipe(
    const WorldRecipe& recipe,
    EpimorphicReliefConfig& reliefConfig,
    WorldPresentationMode& presentationMode) noexcept {
    const WorldMathematicalAffinity& aff = lookupWorldMathematicalAffinity(recipe.generator);
    if (aff.hasDedicatedReliefProjection &&
        aff.preferredReliefProjection < kReliefManifoldProjectionCount) {
        reliefConfig.projection =
            static_cast<ReliefManifoldProjection>(aff.preferredReliefProjection);
        if (aff.autoPromoteHybridVolumetric) {
            presentationMode = WorldPresentationMode::HybridVolumetric;
        }
    }
}

std::optional<ReliefMeshCandidate> SculptureHybridManifoldSynthesizer::prepareReliefMesh(
    mgfx::Engine* engine,
    const SculpturePipelineCoordinator& coordinator,
    const ReliefMeshRequest& request) {
    ReliefMeshCandidate candidate;
    candidate.request = request;
    candidate.data = coordinator.generateEpimorphicRelief(request);
    if (!candidate.data.valid ||
        !hasCompleteTriangles(candidate.data.vertices.size(), candidate.data.indices)) {
        return std::nullopt;
    }
    if (engine == nullptr || candidate.data.vertices.empty()) {
        return candidate;
    }
    if (!canPrepareNativeMesh(*engine, candidate.data.vertices.size(), candidate.data.indices.size())) {
        return std::nullopt;
    }

    std::vector<mgfx::vk::Vertex> vkVertices;
    vkVertices.reserve(candidate.data.vertices.size());
    for (const auto& v : candidate.data.vertices) {
        mgfx::vk::Vertex vert{};
        vert.position = mgfx::math::vec3(v.position.x, v.position.y, v.position.z);
        vert.normal = mgfx::math::vec3(v.normal.x, v.normal.y, v.normal.z);
        vert.uv = mgfx::math::vec2(v.u, v.v);
        vert.color = mgfx::math::vec4(v.r, v.g, v.b, v.a);
        vert.tangent = mgfx::math::vec4(1.0f, 0.0f, 0.0f, 1.0f);
        vkVertices.push_back(vert);
    }

    candidate.mesh = std::make_shared<mgfx::vk::Mesh>(
        engine->context(),
        vkVertices,
        candidate.data.indices);
    return candidate;
}

std::optional<PolychoronMeshCandidate> SculptureHybridManifoldSynthesizer::preparePolychoronMesh(
    mgfx::Engine* engine,
    const PolychoronMeshRequest& request) {
    PolychoronMeshCandidate candidate;
    candidate.request = request;
    const Sculpture4DPolychoronHomotopy polychoronHomotopy(request.config);

    StreamlineTubeConfig tubeCfg{};
    tubeCfg.radius = 0.025f * request.worldScale;
    tubeCfg.radialSegments = 6U;
    tubeCfg.smoothNormals = true;
    tubeCfg.generateCaps = true;

    candidate.data = polychoronHomotopy.generateHomotopyMesh(
        request.sampledTime, tubeCfg);
    if (!hasCompleteTriangles(candidate.data.vertices.size(), candidate.data.indices)) {
        return std::nullopt;
    }
    if (engine == nullptr || candidate.data.empty()) {
        return candidate;
    }
    if (!canPrepareNativeMesh(*engine, candidate.data.vertices.size(), candidate.data.indices.size())) {
        return std::nullopt;
    }

    std::vector<mgfx::vk::Vertex> vkVertices;
    vkVertices.reserve(candidate.data.vertices.size());
    for (const auto& v : candidate.data.vertices) {
        mgfx::vk::Vertex vert{};
        vert.position = mgfx::math::vec3(v.position.x, v.position.y, v.position.z);
        vert.normal = mgfx::math::vec3(v.normal.x, v.normal.y, v.normal.z);
        vert.uv = mgfx::math::vec2(v.materialCoord, v.density);
        vert.color = mgfx::math::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        vert.tangent = mgfx::math::vec4(1.0f, 0.0f, 0.0f, 1.0f);
        vkVertices.push_back(vert);
    }

    candidate.mesh = std::make_shared<mgfx::vk::Mesh>(
        engine->context(),
        vkVertices,
        candidate.data.indices);
    return candidate;
}

const WorldSnapshotDelivery& SculptureHybridManifoldSynthesizer::prepareInstanceDelivery(
    const WorldSnapshotDelivery& delivery,
    const SculptureVolumetricMemory* volMem,
    SculptureVolumetricFieldCoupler& coupler,
    world_gpu_abi::WorldSnapshotGpu& coupledSnapshot,
    WorldSnapshotDelivery& coupledDelivery) {
    if (!delivery) {
        return delivery;
    }

    const bool hasHostInstances = delivery.snapshot != nullptr
        && delivery.metrics != nullptr
        && delivery.metrics->externalSlotIndex == WorldGenerationMetrics::kNoExternalSlot
        && !delivery.metrics->gpuVolumetricCoupled;

    if (coupler.config().enableCoupling && volMem != nullptr && hasHostInstances) {
        constexpr std::size_t headerBytes = offsetof(world_gpu_abi::WorldSnapshotGpu, instances);
        std::memcpy(&coupledSnapshot, delivery.snapshot, headerBytes);

        const std::size_t activeCount = std::min(
            static_cast<std::size_t>(delivery.snapshot->frame.activeInstanceCount),
            static_cast<std::size_t>(world_gpu_abi::kMaxInstanceCount));
        if (activeCount > 0U) {
            std::memcpy(
                coupledSnapshot.instances.data(),
                delivery.snapshot->instances.data(),
                activeCount * sizeof(world_gpu_abi::WorldInstanceGpu));
        }

        coupler.coupleSnapshot(
            *volMem,
            coupledSnapshot,
            delivery.snapshot->frame.deltaSeconds,
            delivery.snapshot->frame.time);
        if (coupler.telemetry().hostFieldUnavailable) {
            // Coupling was refused before mutation. Keep the accepted source
            // delivery rather than publish a replacement observation wrapper.
            return delivery;
        }
        coupledDelivery = delivery;
        coupledDelivery.snapshot = &coupledSnapshot;
        return coupledDelivery;
    }

    return delivery;
}

WorldSnapshotDelivery SculptureHybridManifoldSynthesizer::synthesizePolychoronDelivery(
    const Sculpture4DPolychoronHomotopy& polychoronHomotopy,
    const WorldRecipe& currentRecipe,
    float simulatedTime,
    std::uint64_t totalFramesCoordinated,
    world_gpu_abi::WorldSnapshotGpu& coupledSnapshot,
    WorldGenerationMetrics& polychoronMetrics,
    WorldSnapshotDelivery& coupledDelivery) {
    const auto instances = polychoronHomotopy.generateWorldInstances(simulatedTime);

    constexpr std::size_t headerBytes = offsetof(world_gpu_abi::WorldSnapshotGpu, instances);
    std::memset(&coupledSnapshot, 0, headerBytes);
    coupledSnapshot.frame.generation = static_cast<std::uint32_t>(totalFramesCoordinated);
    if (coupledSnapshot.frame.generation == 0U) {
        coupledSnapshot.frame.generation = 1U;
    }
    coupledSnapshot.frame.time = simulatedTime;
    coupledSnapshot.frame.deltaSeconds = 0.016f;
    coupledSnapshot.frame.activeInstanceCount = static_cast<std::uint32_t>(instances.size());
    coupledSnapshot.recipe = world_gpu_abi::packWorldRecipe(currentRecipe);

    const std::size_t copyCount = std::min(
        instances.size(),
        static_cast<std::size_t>(world_gpu_abi::kMaxInstanceCount));
    if (copyCount > 0U) {
        std::memcpy(
            coupledSnapshot.instances.data(),
            instances.data(),
            copyCount * sizeof(world_gpu_abi::WorldInstanceGpu));
    }

    polychoronMetrics.generation = coupledSnapshot.frame.generation;
    polychoronMetrics.activeInstanceCount = static_cast<std::uint32_t>(copyCount);
    polychoronMetrics.externalSlotIndex = WorldGenerationMetrics::kNoExternalSlot;

    coupledDelivery.snapshot = &coupledSnapshot;
    coupledDelivery.metrics = &polychoronMetrics;
    coupledDelivery.generation = coupledSnapshot.frame.generation;
    coupledDelivery.slotIndex = WorldGenerationMetrics::kNoExternalSlot;

    return coupledDelivery;
}

} // namespace epi_sculpt
