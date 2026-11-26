#include "RecursiveHopfDeviceInstanceStream.h"

#include "RecursiveHopfTorusOpenCl.h"
#include "RecursiveHopfExternalGrammar.h"
#include "RecursiveHopfTorusRenderBridge.h"
#include "vk/RenderGraph.h"

#include <mgfx/Engine.h>
#include <mgfx/FrameSlotBufferCache.h>
#include <mgfx/vk/Buffer.h>
#include <mgfx/vk/Context.h>
#include <mgfx/vk/Descriptor.h>
#include <mgfx/vk/Pipeline.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <string>
#include <stdexcept>
#include <limits>
#include <vector>

namespace epi_sculpt::recursive_hopf {
namespace {

constexpr std::uint32_t kInstanceCapacity = 32768U;
constexpr float kLod0Scale = 0.06f;
constexpr VkDeviceSize kOutputBytes = kInstanceCapacity * sizeof(mgfx::vk::InstanceData);

// CPU cache key. Grammar and the trailing 144-byte settings occupy separate
// shader bindings so native external delivery never re-uploads grammar bytes.
struct alignas(16) StreamInput {
    gpu_abi::RecursiveHopfGrammarGpu grammar{};
    mgfx::math::mat4 root{1.0f};
    mgfx::math::vec4 plusPrimary{};
    mgfx::math::vec4 plusDeep{};
    mgfx::math::vec4 minusPrimary{};
    mgfx::math::vec4 minusDeep{};
    mgfx::math::vec4 pbr{}; // metallic, roughness, emission, depth emission
};
static_assert(sizeof(gpu_abi::RecursiveHopfGrammarGpu) == 13440U);
static_assert(offsetof(StreamInput, root) == 13440U);
static_assert(offsetof(StreamInput, pbr) == 13568U);
static_assert(sizeof(StreamInput) == 13584U);
static_assert(sizeof(StreamInput) - offsetof(StreamInput, root) == 144U);

struct LevelDispatch {
    std::uint32_t depth = 0U;
    std::uint32_t levelStart = 0U;
    std::uint32_t instanceCount = 0U;
    std::uint32_t outputOffset = 0U;
    std::uint32_t colorMaxDepth = 0U;
    std::uint32_t reserved[3]{};
};
static_assert(sizeof(LevelDispatch) == 32U);

struct StreamPlan {
    std::array<LevelDispatch, kMaxLogicalDepth + 1U> levels{};
    std::uint32_t levelCount = 0U;
    std::uint32_t lod0Count = 0U;
    RecursiveHopfRenderBatches stats{};
};

bool makePlan(const RecursiveHopfDelivery& delivery,
              const RecursiveHopfRenderOptions& options, StreamPlan& out) {
    const auto& g = delivery.gpuGrammar;
    const auto& budget = options.budget;
    if (budget.enableViewCulling || options.siteMarkerMode != SiteMarkerMode::Off
        || budget.includeCrossings || budget.includeInterstitials
        || delivery.grammar.recipe().weaveAmplitude > 0.0
        || !std::isfinite(budget.minWorldScale) || budget.minWorldScale < 0.0
        || !gpu_abi::isValid(g) || delivery.generation == 0U
        || g.header.generation != delivery.generation
        || g.header.logicalDepth != g.recipe.logicalDepth
        || g.recipe.placementMode != static_cast<std::uint32_t>(PlacementMode::UniformSimilarity)
        || g.recipe.conformalScaleExponent != 0.0f || g.recipe.weaveAmplitude != 0.0f
        || g.header.rootCircleCount != 2U * g.recipe.fibersPerFamily) return false;
    const auto expectedRecipe = gpu_abi::packRecipe(delivery.grammar.recipe());
    if (std::memcmp(&expectedRecipe, &g.recipe, sizeof(expectedRecipe)) != 0) return false;

    const auto branching = g.header.childrenPerNode;
    const auto circles = g.header.rootCircleCount;
    if (branching == 0U && g.header.logicalDepth != 0U) return false;
    const float childScale = branching ? g.children[0].translationScale[3] : 0.0f;
    if (branching && (!std::isfinite(childScale) || childScale <= 0.0f || childScale >= 1.0f))
        return false;
    for (std::uint32_t i = 0U; i < branching; ++i)
        if (g.children[i].translationScale[3] != childScale) return false;

    // All nodes at a depth have the same scale in this deliberately narrow
    // route. A bounded breadth-first prefix therefore has known draw counts:
    // no atomic compaction, indirect-count readback or CPU node traversal.
    const auto maxDepth = std::min(g.header.logicalDepth, budget.maxDepth);
    const auto maxNodes = std::min(budget.maxNodes,
        std::min(budget.maxFibers, kInstanceCapacity) / circles);
    std::uint32_t levelStart = 0U, levelNodes = 1U;
    float scale = 1.0f;
    std::uint64_t candidateChildren = 0U;
    for (std::uint32_t depth = 0U; depth <= maxDepth; ++depth) {
        if (depth > 0U && scale < budget.minWorldScale) {
            out.stats.lodCulledNodes = levelNodes;
            break;
        }
        const auto selected = std::min(levelNodes, maxNodes - out.stats.visitedNodes);
        if (selected == 0U) break;
        const auto count = selected * circles;
        out.levels[out.levelCount++] = LevelDispatch{
            .depth = depth, .levelStart = levelStart, .instanceCount = count,
            .outputOffset = out.stats.emittedFibers, .colorMaxDepth = std::max(1U, maxDepth),
        };
        out.stats.visitedNodes += selected;
        out.stats.emittedFibers += count;
        out.stats.deepestObservedDepth = depth;
        if (scale >= kLod0Scale) out.lod0Count += count;
        if (depth < maxDepth) candidateChildren += static_cast<std::uint64_t>(selected) * branching;
        if (selected < levelNodes || depth == maxDepth) break;
        levelStart += levelNodes;
        // selected <= capacity/circles, branching <= 128: this product stays
        // bounded even when the logical hierarchy has uint64-scale counts.
        levelNodes = selected * branching;
        scale *= childScale;
    }
    const auto acceptedChildren = out.stats.visitedNodes ? out.stats.visitedNodes - 1U : 0U;
    out.stats.budgetOmittedNodes = static_cast<std::uint32_t>(
        candidateChildren - acceptedChildren - out.stats.lodCulledNodes);
    out.stats.truncatedByBudget = out.stats.visitedNodes == 0U
        || out.stats.budgetOmittedNodes != 0U || out.stats.lodCulledNodes != 0U;
    out.stats.deviceLocalInstanceStream = true;
    return true;
}

} // namespace

struct RecursiveHopfDeviceInstanceStream::Resources {
    struct Slot {
        std::unique_ptr<mgfx::vk::Buffer> grammar;
        std::unique_ptr<mgfx::vk::Buffer> settings;
        std::shared_ptr<const RecursiveHopfExternalGrammarView> externalGrammar;
        VkDescriptorSet set = VK_NULL_HANDLE;
        StreamInput key{};
        std::array<LevelDispatch, kMaxLogicalDepth + 1U> levels{};
        bool valid = false;
        bool recording = false;
        std::uint64_t revision = 0U;
    };

    mgfx::Engine* engine = nullptr;
    VkDevice device = VK_NULL_HANDLE;
    VkDescriptorSetLayout layout = VK_NULL_HANDLE;
    std::unique_ptr<mgfx::vk::DescriptorAllocator> allocator;
    std::unique_ptr<mgfx::vk::Pipeline> pipeline;
    std::vector<Slot> slots;
    mgfx::FrameSlotBufferCache outputs;
    std::string prefix;

    ~Resources() {
        pipeline.reset();
        allocator.reset();
        if (layout != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(device, layout, nullptr);
    }

    void initialize(mgfx::Engine& owner) {
        engine = &owner;
        auto& ctx = owner.context();
        device = ctx.device().logical();
        // Different bridges must not register different physical buffers under
        // one logical RenderGraph name. This identity is transient, not persisted.
        prefix = "RecursiveHopf.Stream." + std::to_string(reinterpret_cast<std::uintptr_t>(this));
        const auto count = owner.frameLoop().framesInFlight();
        if (!outputs.ensure(owner, kOutputBytes,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                "RecursiveHopf.Instances"))
            throw std::runtime_error("Hopf output cache allocation failed");
        mgfx::vk::DescriptorSetLayoutBuilder builder;
        builder.addBinding(0U, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);
        builder.addBinding(1U, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);
        builder.addBinding(2U, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);
        layout = builder.build(device);
        allocator = std::make_unique<mgfx::vk::DescriptorAllocator>(ctx, count,
            std::vector<VkDescriptorPoolSize>{{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, count * 3U}});
        mgfx::vk::Pipeline::ComputeCreateInfo info{};
        info.computeShaderPath = "shaders/recursive_hopf_instances.comp.spv";
        info.pushConstantSize = sizeof(LevelDispatch);
        info.setLayouts = {layout};
        pipeline = std::make_unique<mgfx::vk::Pipeline>(ctx, info);
        slots.resize(count);
        for (std::uint32_t index = 0U; index < count; ++index) {
            auto& slot = slots[index];
            slot.grammar = std::make_unique<mgfx::vk::Buffer>(ctx, sizeof(gpu_abi::RecursiveHopfGrammarGpu),
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, mgfx::vk::BufferLocation::HostVisible);
            slot.settings = std::make_unique<mgfx::vk::Buffer>(ctx, sizeof(StreamInput) - offsetof(StreamInput, root),
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, mgfx::vk::BufferLocation::HostVisible);
            slot.set = allocator->allocate(layout);
            std::array<VkDescriptorBufferInfo, 3> buffers{{
                {slot.grammar->handle(), 0U, sizeof(gpu_abi::RecursiveHopfGrammarGpu)},
                {slot.settings->handle(), 0U, sizeof(StreamInput) - offsetof(StreamInput, root)},
                {outputs.buffer(index)->handle(), 0U, kOutputBytes},
            }};
            std::array<VkWriteDescriptorSet, 3> writes{};
            for (std::uint32_t i = 0U; i < writes.size(); ++i) {
                writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                writes[i].dstSet = slot.set;
                writes[i].dstBinding = i;
                writes[i].descriptorCount = 1U;
                writes[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                writes[i].pBufferInfo = &buffers[i];
            }
            vkUpdateDescriptorSets(device, static_cast<std::uint32_t>(writes.size()),
                writes.data(), 0U, nullptr);
        }
    }
};

RecursiveHopfDeviceInstanceStream::RecursiveHopfDeviceInstanceStream() = default;
RecursiveHopfDeviceInstanceStream::~RecursiveHopfDeviceInstanceStream() = default;

bool RecursiveHopfDeviceInstanceStream::accepts(
    const RecursiveHopfDelivery& delivery, const RecursiveHopfRenderOptions& options) noexcept {
    StreamPlan plan{};
    return makePlan(delivery, options, plan) && plan.stats.emittedFibers != 0U;
}

bool RecursiveHopfDeviceInstanceStream::tryDraw(
    mgfx::Drawable& drawable, const RecursiveHopfDelivery& delivery,
    const RecursiveHopfRenderOptions& options,
    std::shared_ptr<const mgfx::vk::Mesh> lod0,
    std::shared_ptr<const mgfx::vk::Mesh> lod1,
    mgfx::MaterialHandle material, RecursiveHopfRenderBatches& statistics,
    std::shared_ptr<const RecursiveHopfExternalGrammarView> externalGrammar) {
    StreamPlan plan{};
    if (!makePlan(delivery, options, plan)) return false;
    if (plan.stats.emittedFibers == 0U) { statistics = plan.stats; return true; }

    auto& engine = drawable.engine();
    if (externalGrammar && !externalGrammar->matchesDelivery(engine.context(), delivery)) externalGrammar.reset();
    plan.stats.externalGrammar = externalGrammar != nullptr;
    if (externalGrammar) {
        plan.stats.externalGrammarAllocation = externalGrammar->allocationGeneration();
        plan.stats.externalGrammarHandoff = externalGrammar->handoffSerial();
    }
    if (!resources_) {
        try {
            auto candidate = std::make_shared<Resources>();
            candidate->initialize(engine);
            resources_ = std::move(candidate);
        } catch (...) { return false; }
    }
    auto owner = resources_;
    // Native resources belong to one Engine/device generation. Reconstruction
    // recreates the bridge; never bind old descriptors to a replacement device.
    if (owner->engine != &engine || owner->device != engine.context().device().logical()
        || owner->slots.size() != engine.frameLoop().framesInFlight()) return false;
    const auto index = engine.activeFrameIndex();
    if (index >= owner->slots.size()) return false;
    auto& slot = owner->slots[index];
    StreamInput input{};
    input.grammar = delivery.gpuGrammar;
    input.root = options.rootTransform;
    input.plusPrimary = mgfx::math::vec4(options.palette.plusPrimary, 0.0f);
    input.plusDeep = mgfx::math::vec4(options.palette.plusDeep, 0.0f);
    input.minusPrimary = mgfx::math::vec4(options.palette.minusPrimary, 0.0f);
    input.minusDeep = mgfx::math::vec4(options.palette.minusDeep, 0.0f);
    input.pbr = {options.palette.metallic, options.palette.roughness,
                 options.palette.baseEmission, options.palette.depthEmissionBoost};
    const bool dirty = !slot.valid || slot.externalGrammar != externalGrammar || std::memcmp(&slot.key, &input, sizeof(input)) != 0
        || std::memcmp(slot.levels.data(), plan.levels.data(), sizeof(plan.levels)) != 0;
    // One bridge can emit more than once in a frame, but a second selection
    // cannot overwrite the bytes/descriptors already selected by an earlier draw.
    if (slot.recording && dirty) return false;
    if (!slot.recording && !engine.frameLoop().canEnqueueSubmission({.commitCallbacks = 1U})) return false;
    const std::uint64_t retainedBytes = owner->slots.size() * sizeof(StreamInput);
    if (!engine.retainFramePayloadResources(owner, retainedBytes)
        || !owner->outputs.retainCurrent(engine)) return false;
    if (externalGrammar && !engine.retainFramePayloadResources(externalGrammar, externalGrammar->retainedBytes())) return false;
    const auto retainPrototype = [&engine](const std::shared_ptr<const mgfx::vk::Mesh>& mesh) {
        if (!mesh || !mesh->ownsBuffers()) return false;
        return engine.retainFramePayloadResources(mesh,
            mesh->vertexBufferOwner()->size() + mesh->indexBufferOwner()->size());
    };
    if (!retainPrototype(lod0) || !retainPrototype(lod1)) return false;
    if (!slot.recording) {
        if (slot.revision == std::numeric_limits<std::uint64_t>::max()) return false;
        const auto revision = ++slot.revision;
        if (!owner->outputs.reserveCurrentGeneration(engine, revision, kOutputBytes)) return false;
        if (dirty) {
            slot.valid = false;
            if (!externalGrammar) slot.grammar->write(&input.grammar, sizeof(input.grammar));
            slot.settings->write(reinterpret_cast<const std::byte*>(&input) + offsetof(StreamInput, root),
                sizeof(StreamInput) - offsetof(StreamInput, root));
            // Only the active, fence-settled slot's source binding changes.
            VkDescriptorBufferInfo sourceInfo{externalGrammar ? externalGrammar->buffer()->handle()
                : slot.grammar->handle(), 0U, sizeof(gpu_abi::RecursiveHopfGrammarGpu)};
            VkWriteDescriptorSet update{};
            update.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            update.dstSet = slot.set;
            update.dstBinding = 0U;
            update.descriptorCount = 1U;
            update.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            update.pBufferInfo = &sourceInfo;
            vkUpdateDescriptorSets(owner->device, 1U, &update, 0U, nullptr);
        }
        if (!engine.frameLoop().enqueueSubmitCommit([owner, index, revision](bool success) {
                auto& submitted = owner->slots[index];
                if (submitted.revision == revision) {
                    submitted.recording = false;
                    if (!success) {
                        submitted.valid = false;
                        owner->outputs.invalidateGeneration(index, revision);
                    }
                }
            })) return false;

        const std::string suffix = "." + std::to_string(index);
        const std::string inputName = externalGrammar ? externalGrammar->resourceName()
            : owner->prefix + ".Grammar" + suffix;
        const std::string settingsName = owner->prefix + ".Settings" + suffix;
        const std::string outputName = owner->outputs.resourceName(index);
        // Freeze the physical source now. A later selection may retire this
        // view before deferred declarations replay; that cannot change bytes
        // already selected by an earlier draw in this same recording.
        const VkBuffer grammarHandle = externalGrammar ? externalGrammar->buffer()->handle() : slot.grammar->handle();
        engine.deferGraphPass([owner, index, dirty, plan, inputName, settingsName, outputName, externalGrammar, grammarHandle](mgfx::vk::RenderGraph& graph) {
            auto& active = owner->slots[index];
            const auto output = graph.registerBuffer(outputName, owner->outputs.buffer(index)->handle(),
                {VK_IMAGE_LAYOUT_UNDEFINED, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                 VK_ACCESS_2_SHADER_WRITE_BIT, 0U}, 0U, kOutputBytes);
            if (dirty) {
                const auto source = graph.registerBuffer(inputName,
                    grammarHandle,
                    externalGrammar ? mgfx::vk::ResourceState{} : mgfx::vk::ResourceState{
                        VK_IMAGE_LAYOUT_UNDEFINED, VK_PIPELINE_STAGE_2_HOST_BIT, VK_ACCESS_2_HOST_WRITE_BIT, 0U},
                    0U, sizeof(gpu_abi::RecursiveHopfGrammarGpu));
                const auto settings = graph.registerBuffer(settingsName, active.settings->handle(),
                    {VK_IMAGE_LAYOUT_UNDEFINED, VK_PIPELINE_STAGE_2_HOST_BIT, VK_ACCESS_2_HOST_WRITE_BIT, 0U},
                    0U, sizeof(StreamInput) - offsetof(StreamInput, root));
                graph.addPass(outputName + ".Expand")
                    .readBuffer(source, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT)
                    .readBuffer(settings, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT)
                    .writeBuffer(output, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT)
                    .execute([owner, index, plan](VkCommandBuffer cmd) {
                        const auto& current = owner->slots[index];
                        const auto& pipeline = *owner->pipeline;
                        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.handle());
                        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
                            pipeline.layout(), 0U, 1U, &current.set, 0U, nullptr);
                        for (std::uint32_t i = 0U; i < plan.levelCount; ++i) {
                            const auto& level = plan.levels[i];
                            vkCmdPushConstants(cmd, pipeline.layout(), VK_SHADER_STAGE_COMPUTE_BIT,
                                0U, sizeof(level), &level);
                            vkCmdDispatch(cmd, (level.instanceCount + 63U) / 64U, 1U, 1U);
                        }
                    }).build();
            }
            // Drawable admits the typed instance view and declares its consumer
            // after this producer declaration, for both dirty and reused slots.
        });
        slot.externalGrammar = std::move(externalGrammar);
        slot.key = input;
        slot.levels = plan.levels;
        slot.valid = true; // provisional until queue acceptance; failure invalidates it
        slot.recording = true;
        owner->outputs.markGeneration(index, revision, kOutputBytes);
    }
    const auto instances = owner->outputs.currentView(engine, slot.revision,
        mgfx::RenderBufferRole::Instances, kOutputBytes);
    if (!drawable.prepareStream(instances, lod0) || !drawable.prepareStream(instances, lod1)) return false;
    if (plan.lod0Count && !drawable.tryDrawInstanceStream(
            lod0, material, instances, 0U, plan.lod0Count)) return false;
    const auto lod1Count = plan.stats.emittedFibers - plan.lod0Count;
    if (lod1Count && !drawable.tryDrawInstanceStream(
            lod1, material, instances, plan.lod0Count, lod1Count)) return false;
    statistics = plan.stats;
    return true;
}

} // namespace epi_sculpt::recursive_hopf
