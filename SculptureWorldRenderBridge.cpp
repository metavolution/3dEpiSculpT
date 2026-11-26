#include "SculptureWorldRenderBridge.h"
#include "SculptureRenderMath.h"
#include "SculptureWorldExternalTransport.h"
#include "vk/RenderGraph.h"
#include <mgfx/Engine.h>
#include <mgfx/FrameSlotBufferCache.h>
#include <mgfx/StorageComputeResources.h>
#include <mgfx/vk/Mesh.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace epi_sculpt {

struct SculptureWorldRenderBridge::Resources {
    static constexpr VkDeviceSize kSourceBytes = sizeof(world_gpu_abi::WorldSnapshotGpu);
    static constexpr VkDeviceSize kDenseInstanceBytes =
        sizeof(mgfx::vk::InstanceData) * world_gpu_abi::kMaxInstanceCount;
    static constexpr VkDeviceSize kSparseInstanceBytes = kDenseInstanceBytes * kBatchCount;
    // std430 mirror: selected local scale followed by batch/local allocation.
    // Invalid records initialize address.x only; scatter never reads their scale.
    struct alignas(16) SelectionGpu {
        std::array<float, 4U> scale;
        std::array<std::uint32_t, 4U> address;
    };
    static_assert(sizeof(SelectionGpu) == 32U && offsetof(SelectionGpu, address) == 16U);
    static_assert(world_gpu_abi::kMaxInstanceCount == 16384U && kBatchCount == 24U,
        "Update the world compaction shader capacity/selection contract together");
    // std430 uvec2[24] header carries prefix base/count for scatter. Keeping
    // these reads out of the indirect buffer preserves its final writer state
    // until the render graph makes commands visible to DRAW_INDIRECT.
    static constexpr VkDeviceSize kSelectionHeaderBytes =
        sizeof(std::array<std::uint32_t, 2U>) * kBatchCount;
    static_assert(kSelectionHeaderBytes % alignof(SelectionGpu) == 0U);
    static constexpr VkDeviceSize kSelectionBytes = kSelectionHeaderBytes
        + sizeof(SelectionGpu) * world_gpu_abi::kMaxInstanceCount;
    static constexpr VkDeviceSize kCommandBytes = sizeof(IndirectTemplate);

    std::shared_ptr<mgfx::StorageComputeResources> compute;
    mgfx::FrameSlotBufferCache sources;
    mgfx::FrameSlotBufferCache instances;
    mgfx::FrameSlotBufferCache commands;
    bool hostStaged = false;
    bool dense = false;

    [[nodiscard]] VkDeviceSize instanceBytes() const noexcept {
        return dense ? kDenseInstanceBytes : kSparseInstanceBytes;
    }

    // A derivative identity covers the values actually used by compaction.
    // Its safe physical slot is selected independently by each cache.
    std::uint32_t sourceGeneration = 0U;
    std::uint64_t meshRevision = 0U;
    CompactionPushConstants pushConstants{};
    std::uint64_t derivativeGeneration = 0U;
    std::uint64_t selectionAttempt = 0U;

    [[nodiscard]] bool matchesSelection(std::uint32_t source, std::uint64_t mesh,
                                       const CompactionPushConstants& pc) const noexcept {
        const auto equal = [](const auto& a, const auto& b) {
            return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
        };
        return derivativeGeneration != 0U && sourceGeneration == source && meshRevision == mesh
            && equal(pushConstants.cameraPosAndCullEnable, pc.cameraPosAndCullEnable)
            && equal(pushConstants.lodDistancesAndMaxDist, pc.lodDistancesAndMaxDist)
            && equal(pushConstants.frustumForwardAndCosHalfFov, pc.frustumForwardAndCosHalfFov)
            && equal(pushConstants.lodParams, pc.lodParams);
    }
};

std::shared_ptr<SculptureWorldRenderBridge::Resources>
SculptureWorldRenderBridge::ensureCompute(mgfx::Engine& engine) {
    const bool hostStaged = hostSnapshot_ != nullptr;
    if (resources_ && resources_->compute->matches(engine)) {
        if (resources_->hostStaged == hostStaged) return resources_;
        // A mode change may replace the resource island between attempts, but
        // cannot select another source mode after this attempt admitted draws.
        if (resources_->selectionAttempt == engine.activeFrameAttempt()) return {};
    }

    // Construct privately, publish last. A missing shader or allocation failure
    // must not leave a supposedly initialized bridge with partial native state.
    auto candidate = std::make_shared<Resources>();
    candidate->hostStaged = hostStaged;
    candidate->dense = engine.context().device().supportsDrawIndirectFirstInstance();
    if (candidate->dense) {
        const std::array kernels{
            mgfx::StorageComputeResources::KernelSpec{
                "shaders/world_compaction.comp.EPI_WORLD_DENSE_STAGE_1.spv",
                4U, sizeof(CompactionPushConstants)},
            mgfx::StorageComputeResources::KernelSpec{
                "shaders/world_compaction.comp.EPI_WORLD_DENSE_STAGE_2.spv", 4U, 0U},
            mgfx::StorageComputeResources::KernelSpec{
                "shaders/world_compaction.comp.EPI_WORLD_DENSE_STAGE_3.spv", 4U, 0U},
        };
        const std::array scratch{mgfx::StorageComputeResources::ScratchSpec{
            Resources::kSelectionBytes, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT}};
        candidate->compute = mgfx::StorageComputeResources::create(engine, kernels, scratch);
    } else {
        const std::array kernels{mgfx::StorageComputeResources::KernelSpec{
            "shaders/world_compaction.comp.spv", 3U, sizeof(CompactionPushConstants)}};
        candidate->compute = mgfx::StorageComputeResources::create(engine, kernels);
    }
    if (!candidate->compute) return {};

    constexpr auto copies = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    constexpr auto noStaging = mgfx::FrameSlotBufferCache::StagingPolicy::Disabled;
    const auto sourceStaging = hostStaged
        ? mgfx::FrameSlotBufferCache::StagingPolicy::Allowed : noStaging;
    if (!candidate->sources.ensure(engine, Resources::kSourceBytes,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | copies, "WorldSource", sourceStaging)
        || !candidate->instances.ensure(engine, candidate->instanceBytes(),
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | copies,
            "WorldInstances", noStaging)
        || !candidate->commands.ensure(engine, Resources::kCommandBytes,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | copies,
            "WorldIndirect", noStaging)) return {};

    resources_ = candidate;
    return resources_;
}

std::optional<SculptureWorldRenderBridge::CompactionPushConstants>
SculptureWorldRenderBridge::makeViewFamilyLodPushConstants(
    const mgfx::FrameViews& views,
    const mgfx::math::mat4& rootTransform,
    const mgfx::math::vec4& worldLodDistancesAndMaxDist) noexcept {
    if (views.count == 0U || views.count > views.views.size()) return std::nullopt;
    for (int column = 0; column < 4; ++column) {
        if (!std::isfinite(worldLodDistancesAndMaxDist[column])) return std::nullopt;
        for (int row = 0; row < 4; ++row)
            if (!std::isfinite(rootTransform[column][row])) return std::nullopt;
    }
    if (rootTransform[0][3] != 0.0f || rootTransform[1][3] != 0.0f ||
        rootTransform[2][3] != 0.0f || rootTransform[3][3] != 1.0f ||
        worldLodDistancesAndMaxDist.x < 0.0f ||
        worldLodDistancesAndMaxDist.y < worldLodDistancesAndMaxDist.x ||
        worldLodDistancesAndMaxDist.z < worldLodDistancesAndMaxDist.y)
        return std::nullopt;

    const glm::dmat3 linear(rootTransform);
    const double determinant = glm::determinant(linear);
    if (!std::isfinite(determinant) || determinant == 0.0) return std::nullopt;
    const glm::dmat3 inverse = glm::inverse(linear);
    // ||A^-1||_2 <= sqrt(max absolute row sum of (A^-1)^T A^-1).
    // Unlike division by the largest root scale, this bounds local distance
    // in every direction. It is tight for uniform scale/rotation in exact math.
    double normSquared = 0.0;
    for (int row = 0; row < 3; ++row) {
        double sum = 0.0;
        for (int column = 0; column < 3; ++column) {
            const double entry = glm::dot(inverse[row], inverse[column]);
            if (!std::isfinite(entry)) return std::nullopt;
            sum += std::abs(entry);
        }
        normSquared = std::max(normSquared, sum);
    }
    constexpr double guard = 1.0 + 32.0 * std::numeric_limits<float>::epsilon();
    const double inverseStretch = std::sqrt(normSquared) * guard;
    if (!std::isfinite(inverseStretch) || inverseStretch <= 0.0) return std::nullopt;
    const double linearFrobenius = std::sqrt(glm::dot(linear[0], linear[0]) +
        glm::dot(linear[1], linear[1]) + glm::dot(linear[2], linear[2]));
    // Do not use an extremely ill-conditioned inverse as observation evidence.
    // This upper bound on condition number leaves room for float packing guards.
    if (linearFrobenius * inverseStretch > 1.0e6) return std::nullopt;

    std::array<glm::dvec3, mgfx::kMaxPresentationViews> eyes{};
    glm::dvec3 center(0.0);
    for (std::uint32_t eye = 0; eye < views.count; ++eye) {
        const auto& position = views.views[eye].eye;
        if (!std::isfinite(position.x) || !std::isfinite(position.y) ||
            !std::isfinite(position.z)) return std::nullopt;
        eyes[eye] = inverse * (glm::dvec3(position) - glm::dvec3(rootTransform[3]));
        center += eyes[eye] / static_cast<double>(views.count);
    }
    for (int component = 0; component < 3; ++component)
        if (!std::isfinite(center[component]) ||
            std::abs(center[component]) > std::numeric_limits<float>::max())
            return std::nullopt;
    const mgfx::math::vec3 localCenter(center);
    double eyeRadius = 0.0;
    for (std::uint32_t eye = 0; eye < views.count; ++eye) {
        const double radius = glm::length(eyes[eye] - glm::dvec3(localCenter));
        if (!std::isfinite(radius)) return std::nullopt;
        eyeRadius = std::max(eyeRadius, radius);
    }

    CompactionPushConstants pc{};
    pc.cameraPosAndCullEnable = mgfx::math::vec4(localCenter, 1.0f);
    // If any eye is within world distance D, its local point is within
    // inverseStretch*D + eyeRadius of the packed center (triangle inequality).
    // The existing shader therefore keeps at least the nearest eye's LOD.
    for (int component = 0; component < 4; ++component) {
        const float distance = worldLodDistancesAndMaxDist[component];
        if (component == 3 && distance <= 0.0f) {
            pc.lodDistancesAndMaxDist[component] = 0.0f; // Disabled far clip stays disabled.
            continue;
        }
        const double threshold = inverseStretch * distance + eyeRadius * guard;
        if (!std::isfinite(threshold) || threshold >= std::numeric_limits<float>::max())
            return std::nullopt;
        const float packed = std::nextafter(static_cast<float>(threshold),
                                           std::numeric_limits<float>::infinity());
        if (!std::isfinite(packed)) return std::nullopt;
        pc.lodDistancesAndMaxDist[component] = packed;
    }
    if (views.shadows) {
        const auto& bounds = views.shadows->casterBounds;
        if (!std::isfinite(bounds.center.x) || !std::isfinite(bounds.center.y) ||
            !std::isfinite(bounds.center.z) || !std::isfinite(bounds.radius) ||
            bounds.radius <= 0.0f) return std::nullopt;
        const glm::dvec3 localCasterCenter = inverse *
            (glm::dvec3(bounds.center) - glm::dvec3(rootTransform[3]));
        const double radius = (glm::length(localCasterCenter - glm::dvec3(localCenter)) +
            inverseStretch * bounds.radius) * guard;
        if (!std::isfinite(radius) || radius >= std::numeric_limits<float>::max())
            return std::nullopt;
        pc.lodParams.z = std::nextafter(static_cast<float>(radius),
            std::numeric_limits<float>::infinity());
        if (!std::isfinite(pc.lodParams.z)) return std::nullopt;
    }
    return pc;
}

void SculptureWorldRenderBridge::refreshIndirectTemplate(
    const PrimitiveRegistry& primitives) {
    if (templateMeshRegistryKnown_ &&
        templateMeshes_ == primitives.meshes) {
        // A replacement mesh can occupy the same host address. The consumed
        // index-count headers, not pointer equality alone, decide template reuse.
        bool sameCounts = true;
        for (std::size_t primitive = 0U; primitive < kPrimitiveCount; ++primitive) {
            const auto* mesh = primitives.meshes[primitive];
            if (indirectTemplate_[primitive * kMaterialBandCapacity].indexCount
                != (mesh ? mesh->indexCount() : 0U)) {
                sameCounts = false;
                break;
            }
        }
        if (sameCounts) return;
    }
    if (templateRevision_ == std::numeric_limits<std::uint64_t>::max())
        throw std::overflow_error("world mesh template revision exhausted");
    IndirectTemplate commands{};
    for (std::size_t primitive = 0U; primitive < kPrimitiveCount; ++primitive) {
        const mgfx::vk::Mesh* mesh = primitives.meshes[primitive];
        if (mesh == nullptr) continue;
        for (std::size_t materialBand = 0U;
             materialBand < kMaterialBandCapacity; ++materialBand) {
            const std::size_t batchIndex = primitive * kMaterialBandCapacity + materialBand;
            commands[batchIndex].indexCount = mesh->indexCount();
            // Dense prefix authors GPU batch bases; sparse fallback binds a
            // CPU-known per-batch vertex offset. Both templates start at zero.
            commands[batchIndex].firstInstance = 0U;
        }
    }
    indirectTemplate_ = commands;
    templateMeshes_ = primitives.meshes;
    templateMeshRegistryKnown_ = true;
    ++templateRevision_;
}

void SculptureWorldRenderBridge::rebuild(const WorldSnapshotDelivery& delivery) {
    if (!delivery) return;
    const bool external = delivery.usesExternalMemory();
    if (generation_ == delivery.generation && external == !hostSnapshot_) return;
    if (!world_gpu_abi::isValid(delivery.snapshot->frame, delivery.snapshot->recipe)) return;

    const auto rebuildBegin = std::chrono::steady_clock::now();

    std::unique_ptr<const world_gpu_abi::WorldSnapshotGpu> hostSnapshot;
    if (external) {
        // External publication contains only a host header. Never treat its
        // unmaterialized instances as staged data when native admission fails.
        if (externalTransport_ == nullptr || !delivery.presentationTicket.owner
            || delivery.presentationTicket.generation != delivery.generation
            || delivery.presentationTicket.slotIndex != delivery.slotIndex) return;
    } else {
        if (delivery.presentationTicket.owner || !world_gpu_abi::isValid(*delivery.snapshot)) return;
        // The producer ring is borrowed until its next publication/reset.
        // Copy once at adoption; cache staging owns later GPU upload bytes.
        hostSnapshot = std::make_unique<world_gpu_abi::WorldSnapshotGpu>(*delivery.snapshot);
    }

    // Publish the coherent observation last. Failed validation/allocation leaves
    // the previous owned selection intact for retry without advancing any math.
    hostSnapshot_ = std::move(hostSnapshot);
    generation_ = delivery.generation;
    externalSlotIndex_ = delivery.slotIndex;
    presentationTicket_ = delivery.presentationTicket;
    instanceCount_ = delivery.snapshot->frame.activeInstanceCount;
    droppedInstanceCount_ = 0U;
    // These are source/candidate-command facts, not GPU-visible post-LOD counts.
    activeBatchCount_ = instanceCount_ > 0U ? kBatchCount : 0U;
    
    metrics_ = {
        .generation = generation_,
        .sourceInstanceCount = static_cast<std::uint32_t>(instanceCount_),
        .acceptedInstanceCount = static_cast<std::uint32_t>(instanceCount_),
        .activeBatchCount = static_cast<std::uint32_t>(activeBatchCount_),
        .droppedInstanceCount = 0U,
        .rebuildMilliseconds = std::chrono::duration<float, std::milli>(
            std::chrono::steady_clock::now() - rebuildBegin).count(),
    };
}

bool SculptureWorldRenderBridge::prepareCompaction(
    mgfx::Engine& engine, const std::shared_ptr<Resources>& owner,
    std::uint64_t derivativeGeneration) {
    const auto slot = engine.activeFrameIndex();
    const auto sourceGeneration = generation_;
    const bool retainedSource = owner->sources.containsGeneration(sourceGeneration);
    const bool externalImport = !retainedSource && !owner->hostStaged;
    auto& loop = engine.frameLoop();
    // Source upload/propagation or external handoff, plus our author callback.
    // Host uploads consume no external semaphore budget.
    if (!loop.canEnqueueSubmission({
            .externalWaits = externalImport ? 1U : 0U,
            .externalSignals = externalImport ? 1U : 0U,
            .commitCallbacks = 2U,
        })
        || !owner->sources.reserveCurrentGeneration(
            engine, sourceGeneration, Resources::kSourceBytes)) return false;

    mgfx::NativeBufferReadView importedSource;
    if (retainedSource) {
        if (!owner->sources.ensureCurrentGeneration(engine, sourceGeneration,
                Resources::kSourceBytes, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                VK_ACCESS_2_SHADER_READ_BIT)) return false;
    } else if (owner->hostStaged) {
        if (!hostSnapshot_ || hostSnapshot_->frame.generation != sourceGeneration) return false;
        const auto bytes = std::as_bytes(std::span(hostSnapshot_.get(), 1U));
        if (!owner->sources.uploadCurrentGeneration(engine, sourceGeneration, bytes,
                VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT)) return false;
    } else {
        if (externalTransport_ == nullptr || presentationTicket_.generation != sourceGeneration
            || presentationTicket_.slotIndex != externalSlotIndex_) return false;
        importedSource = externalTransport_->acquirePresentationRead(engine, presentationTicket_);
        if (!importedSource.validFor(engine)) return false;
    }

    const auto rejectInstances = owner->instances.rejectionToken(engine, derivativeGeneration);
    const auto rejectCommands = owner->commands.rejectionToken(engine, derivativeGeneration);
    // Host upload/propagation owns its rejection callback and retained staging.
    // Only a fresh native import is authored directly here.
    const auto rejectSource = externalImport
        ? owner->sources.rejectionToken(engine, sourceGeneration)
        : mgfx::FrameSlotBufferCache::RejectionToken{};
    const auto ticket = presentationTicket_;
    if (!loop.enqueueSubmitCommit(
            [rejectInstances, rejectCommands, rejectSource, externalImport, ticket](bool success) noexcept {
                if (!success || (externalImport
                        && ticket.status() != WorldPresentationHandoff::Status::Released)) {
                    rejectInstances.reject();
                    rejectCommands.reject();
                    rejectSource.reject();
                }
            })) {
        // Acquisition may already have queued a binary wait/release. Aborting
        // recording lets FrameLoop reject that transaction exactly once.
        throw std::logic_error("world compaction callback capacity changed after preflight");
    }

    if (externalImport) {
        engine.deferGraphPass([owner, slot, importedSource, enginePtr = &engine](mgfx::vk::RenderGraph& graph) {
            if (!importedSource.validFor(*enginePtr))
                throw std::logic_error("world source read expired before snapshot copy");
            const auto binding = importedSource.binding();
            const auto source = graph.registerBuffer(owner->sources.resourceName(slot) + ".Import",
                binding.buffer, {VK_IMAGE_LAYOUT_UNDEFINED, VK_PIPELINE_STAGE_2_NONE, 0U, 0U},
                binding.offset, binding.range);
            const auto destination = graph.registerBuffer(owner->sources.resourceName(slot),
                owner->sources.buffer(slot)->handle(),
                {VK_IMAGE_LAYOUT_UNDEFINED, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                 VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT, 0U},
                0U, Resources::kSourceBytes);
            graph.addPass(owner->sources.resourceName(slot) + ".ImportCopy")
                .readBuffer(source, VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_READ_BIT)
                .writeBuffer(destination, VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT)
                .execute([owner, slot, importedSource, enginePtr](VkCommandBuffer command) {
                    if (!importedSource.validFor(*enginePtr))
                        throw std::logic_error("world source read expired before snapshot recording");
                    const auto sourceBinding = importedSource.binding();
                    VkBufferCopy copy{};
                    copy.srcOffset = sourceBinding.offset;
                    copy.size = Resources::kSourceBytes;
                    vkCmdCopyBuffer(command, sourceBinding.buffer,
                        owner->sources.buffer(slot)->handle(), 1U, &copy);
                }).build();
        });
        owner->sources.markGeneration(slot, sourceGeneration, Resources::kSourceBytes);
    }

    // Descriptors follow the settled GRAPHICS slot, never the imported slot.
    // Each stage has its own set. Rewrite exact ranges only in this safe slot.
    const std::uint32_t kernelCount = owner->dense ? 3U : 1U;
    const std::uint32_t bindingCount = owner->dense ? 4U : 3U;
    std::array<VkDescriptorSet, 3U> descriptors{};
    const std::array<VkDescriptorBufferInfo, 4U> bindings{{
        {owner->sources.buffer(slot)->handle(), 0U, Resources::kSourceBytes},
        {owner->instances.buffer(slot)->handle(), 0U, owner->instanceBytes()},
        {owner->commands.buffer(slot)->handle(), 0U, Resources::kCommandBytes},
        {owner->dense ? owner->compute->scratch(slot)->handle() : VK_NULL_HANDLE,
         0U, owner->dense ? Resources::kSelectionBytes : 0U},
    }};
    for (std::uint32_t kernel = 0U; kernel < kernelCount; ++kernel) {
        descriptors[kernel] = owner->compute->descriptor(kernel, slot);
        std::array<VkWriteDescriptorSet, 4U> writes{};
        for (std::uint32_t binding = 0U; binding < bindingCount; ++binding) {
            writes[binding].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[binding].dstSet = descriptors[kernel];
            writes[binding].dstBinding = binding;
            writes[binding].descriptorCount = 1U;
            writes[binding].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            writes[binding].pBufferInfo = &bindings[binding];
        }
        vkUpdateDescriptorSets(engine.context().device().logical(),
            bindingCount, writes.data(), 0U, nullptr);
    }

    engine.deferGraphPass([owner, slot, descriptors, commands = indirectTemplate_,
                           pc = owner->pushConstants](mgfx::vk::RenderGraph& graph) {
        const mgfx::vk::ResourceState previousAccess{VK_IMAGE_LAYOUT_UNDEFINED,
            VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
            VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT, 0U};
        const auto source = graph.registerBuffer(owner->sources.resourceName(slot),
            owner->sources.buffer(slot)->handle(), previousAccess, 0U, Resources::kSourceBytes);
        const auto instances = graph.registerBuffer(owner->instances.resourceName(slot),
            owner->instances.buffer(slot)->handle(), previousAccess, 0U, owner->instanceBytes());
        const auto indirect = graph.registerBuffer(owner->commands.resourceName(slot),
            owner->commands.buffer(slot)->handle(), previousAccess, 0U, Resources::kCommandBytes);
        graph.addPass(owner->commands.resourceName(slot) + ".Reset")
            .writeBuffer(indirect, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT)
            .execute([owner, slot, commands](VkCommandBuffer command) {
                vkCmdUpdateBuffer(command, owner->commands.buffer(slot)->handle(),
                    0U, sizeof(commands), commands.data());
            }).build();
        const auto dispatch = [owner, descriptors, pc](VkCommandBuffer command,
                std::uint32_t kernel, std::uint32_t groups) {
            const auto* pipeline = owner->compute->pipeline(kernel);
            vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->handle());
            vkCmdBindDescriptorSets(command, VK_PIPELINE_BIND_POINT_COMPUTE,
                pipeline->layout(), 0U, 1U, &descriptors[kernel], 0U, nullptr);
            if (kernel == 0U) {
                vkCmdPushConstants(command, pipeline->layout(), VK_SHADER_STAGE_COMPUTE_BIT,
                    0U, sizeof(pc), &pc);
            }
            vkCmdDispatch(command, groups, 1U, 1U);
        };
        constexpr auto groups = (world_gpu_abi::kMaxInstanceCount + 255U) / 256U;
        if (owner->dense) {
            const auto selection = graph.registerBuffer(
                owner->compute->slotName(slot) + ".WorldSelection",
                owner->compute->scratch(slot)->handle(), previousAccess,
                0U, Resources::kSelectionBytes);
            graph.addPass(owner->instances.resourceName(slot) + ".Count")
                .readBuffer(source, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT)
                .writeBuffer(selection, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT)
                .readBuffer(indirect, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT)
                .writeBuffer(indirect, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT)
                .execute([dispatch](VkCommandBuffer command) { dispatch(command, 0U, groups); }).build();
            graph.addPass(owner->instances.resourceName(slot) + ".Prefix")
                .readBuffer(indirect, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT)
                .writeBuffer(indirect, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT)
                .writeBuffer(selection, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT)
                .execute([dispatch](VkCommandBuffer command) { dispatch(command, 1U, 1U); }).build();
            graph.addPass(owner->instances.resourceName(slot) + ".Scatter")
                .readBuffer(source, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT)
                .readBuffer(selection, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT)
                .writeBuffer(instances, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT)
                .execute([dispatch](VkCommandBuffer command) { dispatch(command, 2U, groups); }).build();
        } else {
            graph.addPass(owner->instances.resourceName(slot) + ".Compact")
                .readBuffer(source, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT)
                .writeBuffer(instances, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT)
                .readBuffer(indirect, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT)
                .writeBuffer(indirect, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT)
                .execute([dispatch](VkCommandBuffer command) { dispatch(command, 0U, groups); }).build();
        }
    });
    // Provisional authoring: exact cache tokens above invalidate rejected work.
    owner->instances.markGeneration(slot, derivativeGeneration, owner->instanceBytes());
    owner->commands.markGeneration(slot, derivativeGeneration, Resources::kCommandBytes);
    return true;
}

bool SculptureWorldRenderBridge::draw(
    mgfx::Drawable& draw,
    const PrimitiveRegistry& primitives,
    std::span<const mgfx::MaterialHandle> materials,
    const mgfx::math::mat4& rootTransform,
    std::optional<std::uint32_t> expectedGeneration) {
    if (materials.empty() || std::none_of(primitives.meshes.begin(), primitives.meshes.end(),
            [](const auto* mesh) { return mesh != nullptr; })) return false;
    auto& engine = draw.engine();
    // Bootstrap is a separate binary-semaphore submission. Empty completed
    // worlds still import, initialize zero commands and settle their handoff.
    const bool priming = externalTransport_ != nullptr
        && externalTransport_->primeVulkanReleaseSemaphores(engine);
    if (generation_ == 0U
        || (expectedGeneration && generation_ != *expectedGeneration)
        || (priming && !hostSnapshot_)) return false;
    if (!hostSnapshot_ && (externalTransport_ == nullptr
        || externalSlotIndex_ >= SculptureWorldExternalTransport::kSlotCount)) return false;
    CompactionPushConstants pc = compactionPushConstants_;
    const auto finite = [](const auto& value) {
        return std::isfinite(value.x) && std::isfinite(value.y)
            && std::isfinite(value.z) && std::isfinite(value.w);
    };
    if (!finite(pc.cameraPosAndCullEnable)
        || !finite(pc.lodDistancesAndMaxDist)
        || !finite(pc.frustumForwardAndCosHalfFov)
        || !finite(pc.lodParams)) return false;
    pc.lodParams.w = 1.0f;
    if (pc.lodParams.z > 0.0f) {
        double prototypeRadius = 1.0;
        bool knownBounds = true;
        for (const auto* mesh : primitives.meshes) {
            if (!mesh || mesh->indexCount() == 0U) continue;
            const auto& bounds = mesh->bounds();
            const double radius = glm::length(glm::dvec3(bounds.center)) + bounds.radius;
            if (!std::isfinite(radius) || !std::isfinite(bounds.radius) || bounds.radius <= 0.0f) {
                knownBounds = false;
                break;
            }
            prototypeRadius = std::max(prototypeRadius, radius);
        }
        prototypeRadius *= 1.0 + 32.0 * std::numeric_limits<float>::epsilon();
        if (knownBounds && prototypeRadius < std::numeric_limits<float>::max()) {
            pc.lodParams.w = std::nextafter(static_cast<float>(prototypeRadius),
                std::numeric_limits<float>::infinity());
            knownBounds = std::isfinite(pc.lodParams.w);
        } else {
            knownBounds = false;
        }
        // GPU-only/borrowed meshes may not publish bounds. Keep their complete
        // bounded source rather than inventing a radius and losing a caster.
        if (!knownBounds) {
            const float minimumScale = pc.lodParams.y;
            pc = CompactionPushConstants{};
            pc.lodParams.y = minimumScale;
            // Disabled LOD consumes no view/bounds/compensation values. Keep
            // its key canonical so camera edits can reuse this full stream.
        }
    }

    const auto owner = ensureCompute(engine);
    if (!owner || !owner->compute->retainCurrent(engine)) return false;
    refreshIndirectTemplate(primitives);
    const auto attempt = engine.activeFrameAttempt();
    const bool sameSelection = owner->matchesSelection(
        generation_, templateRevision_, pc);
    // Repeated observers may share a selection; no second observer can rewrite
    // that attempt's descriptors/storage with a different compaction policy.
    if (owner->selectionAttempt == attempt && !sameSelection) return false;
    if (!sameSelection
        && owner->derivativeGeneration == std::numeric_limits<std::uint64_t>::max())
        throw std::overflow_error("world derivative generation exhausted");
    const auto derivative = sameSelection
        ? owner->derivativeGeneration : owner->derivativeGeneration + 1U;
    if (!owner->instances.reserveCurrentGeneration(engine, derivative, owner->instanceBytes())
        || !owner->commands.reserveCurrentGeneration(engine, derivative, Resources::kCommandBytes)) return false;
    owner->sourceGeneration = generation_;
    owner->meshRevision = templateRevision_;
    owner->pushConstants = pc;
    owner->derivativeGeneration = derivative;
    owner->selectionAttempt = attempt;

    if (owner->instances.containsGeneration(derivative)
        && owner->commands.containsGeneration(derivative)) {
        // A paused publication can rotate graphics slots without reacquiring
        // its consumed transport signal. Cold slots use existing GPU copies.
        if (!owner->instances.ensureCurrentGeneration(engine, derivative,
                owner->instanceBytes(), VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT,
                VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT)
            || !owner->commands.ensureCurrentGeneration(engine, derivative,
                Resources::kCommandBytes, VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT,
                VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT)) return false;
    } else if (!prepareCompaction(engine, owner, derivative)) {
        return false;
    }

    const auto instances = owner->instances.currentView(engine, derivative,
        mgfx::RenderBufferRole::Instances, owner->instanceBytes());
    const auto commands = owner->commands.currentView(engine, derivative,
        mgfx::RenderBufferRole::DrawIndexedIndirect, Resources::kCommandBytes);
    for (std::size_t primitive = 0U; primitive < kPrimitiveCount; ++primitive) {
        const auto* mesh = primitives.meshes[primitive];
        if (mesh == nullptr) continue;
        for (std::size_t materialBand = 0U; materialBand < kMaterialBandCapacity; ++materialBand) {
            const auto batch = static_cast<std::uint32_t>(primitive * kMaterialBandCapacity + materialBand);
            const auto firstInstance = owner->dense
                ? 0U : batch * world_gpu_abi::kMaxInstanceCount;
            if (!draw.tryDrawIndexedIndirect(*mesh, materials[materialBand % materials.size()],
                    instances, firstInstance, world_gpu_abi::kMaxInstanceCount,
                    commands, batch, 1U, rootTransform))
                throw std::logic_error("world indexed-indirect selection failed admission");
        }
    }
    return true;
}

} // namespace epi_sculpt
