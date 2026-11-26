#include "RecursiveHopfExternalGrammar.h"

#include "RecursiveHopfTorusOpenCl.h"
#include "ExperimentResourceExternalTransport.h"
#include "vk/RenderGraph.h"
#include <mgfx/Engine.h>
#include <mgfx/vk/Buffer.h>
#include <mgfx/vk/Context.h>

#include <array>
#include <atomic>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <unistd.h>

namespace epi_sculpt::recursive_hopf {
namespace {
constexpr VkDeviceSize kGrammarBytes = sizeof(gpu_abi::RecursiveHopfGrammarGpu);
constexpr std::uint64_t kRingBytes = kGrammarBytes * kExperimentExternalSlotCount;
std::atomic<std::uint64_t> nextAllocation{1U};

std::uint64_t takeAllocation() noexcept {
    auto value = nextAllocation.load();
    while (value != 0U && value != std::numeric_limits<std::uint64_t>::max()) {
        if (nextAllocation.compare_exchange_weak(value, value + 1U)) return value;
    }
    return 0U; // identity exhaustion never wraps into a previous allocation
}

bool exportSupported(const mgfx::vk::Context& context) {
    if (!ExperimentResourceExternalTransport::isSupported(context)) return false;
    VkPhysicalDeviceExternalBufferInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_BUFFER_INFO;
    info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT
        | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    info.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
    VkExternalBufferProperties memory{};
    memory.sType = VK_STRUCTURE_TYPE_EXTERNAL_BUFFER_PROPERTIES;
    vkGetPhysicalDeviceExternalBufferProperties(context.device().physical(), &info, &memory);
    const auto& properties = memory.externalMemoryProperties;
    // Buffer's exporter does not currently request dedicated allocations.
    if (!(properties.externalMemoryFeatures & VK_EXTERNAL_MEMORY_FEATURE_EXPORTABLE_BIT)
        || (properties.externalMemoryFeatures & VK_EXTERNAL_MEMORY_FEATURE_DEDICATED_ONLY_BIT)
        || !(properties.compatibleHandleTypes & VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT)) return false;
    VkPhysicalDeviceExternalSemaphoreInfo semInfo{};
    semInfo.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_SEMAPHORE_INFO;
    semInfo.handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT;
    VkExternalSemaphoreProperties semaphore{};
    semaphore.sType = VK_STRUCTURE_TYPE_EXTERNAL_SEMAPHORE_PROPERTIES;
    vkGetPhysicalDeviceExternalSemaphoreProperties(context.device().physical(), &semInfo, &semaphore);
    return (semaphore.externalSemaphoreFeatures & VK_EXTERNAL_SEMAPHORE_FEATURE_EXPORTABLE_BIT)
        && (semaphore.compatibleHandleTypes & VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT);
}

void ownershipBarrier(VkCommandBuffer cmd, VkBuffer buffer,
                      std::uint32_t graphicsFamily, bool acquire) {
    VkBufferMemoryBarrier2 barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
    barrier.srcStageMask = acquire ? VK_PIPELINE_STAGE_2_NONE : VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    barrier.srcAccessMask = acquire ? 0U : VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
    barrier.dstStageMask = acquire ? VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT : VK_PIPELINE_STAGE_2_NONE;
    barrier.dstAccessMask = acquire ? VK_ACCESS_2_SHADER_READ_BIT : 0U;
    barrier.srcQueueFamilyIndex = acquire ? VK_QUEUE_FAMILY_EXTERNAL : graphicsFamily;
    barrier.dstQueueFamilyIndex = acquire ? graphicsFamily : VK_QUEUE_FAMILY_EXTERNAL;
    barrier.buffer = buffer;
    barrier.size = kGrammarBytes;
    VkDependencyInfo dependency{};
    dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dependency.bufferMemoryBarrierCount = 1U;
    dependency.pBufferMemoryBarriers = &barrier;
    vkCmdPipelineBarrier2(cmd, &dependency);
}

struct ExportAttempt {
    explicit ExportAttempt(ExperimentOpenClImportSlot exported) : value(exported) {}
    ExportAttempt(const ExportAttempt&) = delete;
    ExportAttempt& operator=(const ExportAttempt&) = delete;
    ExperimentOpenClImportSlot value;
    ~ExportAttempt() {
        for (int fd : {value.memoryFd, value.vulkanReleaseSemaphoreFd, value.openClReadySemaphoreFd})
            if (fd >= 0) ::close(fd);
    }
};
} // namespace

bool RecursiveHopfExternalGrammarView::matches(
    const mgfx::vk::Context& context, std::uint32_t generation) const noexcept {
    return usable_ && owner_ && buffer_ && context_ == &context
        && device_ == context.device().logical() && generation_ == generation && generation != 0U;
}
const mgfx::vk::Buffer* RecursiveHopfExternalGrammarView::buffer() const noexcept {
    return usable_ ? buffer_ : nullptr;
}
bool RecursiveHopfExternalGrammarView::matchesDelivery(
    const mgfx::vk::Context& context, const RecursiveHopfDelivery& delivery) const noexcept {
    return delivery.producedByOpenCl && matches(context, delivery.generation)
        && std::memcmp(&grammar_, &delivery.gpuGrammar, sizeof(grammar_)) == 0;
}
std::uint64_t RecursiveHopfExternalGrammarView::retainedBytes() const noexcept { return kRingBytes; }
std::uint64_t RecursiveHopfExternalGrammarView::allocationGeneration() const noexcept {
    return owner_ ? owner_->config().allocationGeneration : 0U;
}

struct RecursiveHopfExternalGrammarTransport::State {
    std::shared_ptr<ExperimentResourceExternalTransport> transport;
    std::array<SculptureExternalSlotTicket, kExperimentExternalSlotCount> writable{};
    std::array<bool, kExperimentExternalSlotCount> queued{};
    std::array<bool, kExperimentExternalSlotCount> disabled{};
    std::shared_ptr<RecursiveHopfExternalGrammarView> pending;
    std::shared_ptr<RecursiveHopfExternalGrammarView> active;
    std::string prefix;
    VkDevice device = VK_NULL_HANDLE;
    std::uint64_t nextTransfer = 1U;
    bool recording = false;
    std::uint32_t recordingGeneration = 0U;

    static ExperimentExternalLease lease(const RecursiveHopfExternalGrammarView& view) {
        return {view.slot_, view.owner_->config().allocationGeneration, view.epoch_};
    }

    // Transfer serials identify storage work, not mathematical generations.
    // Accepted queue submission authorizes OpenCL's next copy, with no producer
    // callback or speculative domain advancement inside submit settlement.
    bool release(mgfx::Engine& engine, std::uint32_t slot,
                 std::shared_ptr<RecursiveHopfExternalGrammarView> previous,
                 const std::shared_ptr<State>& owner) {
        if (disabled[slot] || queued[slot] || nextTransfer == 0U) return false;
        auto& loop = engine.frameLoop();
        const auto* source = transport->bufferForVulkanRelease(slot);
        if (!source || !loop.canEnqueueSubmission({.externalSignals = 1U, .commitCallbacks = 1U})) return false;
        const auto serial = nextTransfer++;
        if (!loop.enqueueSubmitCommit([owner, slot, serial, previous](bool accepted) {
                owner->queued[slot] = false;
                if (!accepted) {
                    if (previous) static_cast<void>(owner->transport->quarantine(lease(*previous), previous->transfer_));
                    return;
                }
                SculptureExternalSlotTicket ticket{};
                if (!owner->transport->markVulkanReleaseSubmitted(slot, serial, &ticket)) {
                    owner->disabled[slot] = true; // a signal was submitted; never signal it twice
                    if (previous) static_cast<void>(owner->transport->quarantine(lease(*previous), previous->transfer_));
                    return;
                }
                owner->writable[slot] = ticket;
            })) return false;
        if (!loop.enqueueExternalSignal(transport->vulkanReleaseSemaphore(slot), VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT))
            throw std::logic_error("Hopf external release capacity changed after preflight");
        queued[slot] = true;
        if (previous) previous->usable_ = false;
        const auto handle = source->handle();
        const auto family = engine.context().device().queueFamilies().graphics;
        const auto name = prefix + ".Slot." + std::to_string(slot);
        engine.deferGraphPass([owner, handle, family, name](mgfx::vk::RenderGraph& graph) {
            const auto resource = graph.registerBuffer(name, handle, {}, 0U, kGrammarBytes);
            graph.addPass(name + ".Release")
                .readBuffer(resource, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, VK_ACCESS_2_MEMORY_READ_BIT)
                .execute([handle, family](VkCommandBuffer cmd) { ownershipBarrier(cmd, handle, family, false); }).build();
        });
        return true;
    }
};

RecursiveHopfExternalGrammarTransport::RecursiveHopfExternalGrammarTransport(
    mgfx::vk::Context& context, RecursiveHopfOpenClProducer& producer)
    : context_(&context), producer_(&producer) {}

RecursiveHopfExternalGrammarTransport::~RecursiveHopfExternalGrammarTransport() {
    if (ownsImports_) producer_->clearExternalGrammarImports();
}

bool RecursiveHopfExternalGrammarTransport::initialize() {
    if (attempted_) return state_ != nullptr;
    attempted_ = true;
    diagnostic_ = "host grammar (external capabilities unavailable)";
    const auto uuid = producer_->selectedDeviceUuid();
    if (!uuid || *uuid != mgfx::vk::queryPhysicalDeviceUuid(context_->device().physical())
        || !producer_->requiredDeviceUuidMatched() || !producer_->supportsExternalGrammar()
        || producer_->hasExternalGrammarImports() || !exportSupported(*context_)) return false;
    std::shared_ptr<State> candidate;
    try {
        candidate = std::make_shared<State>();
        const auto allocation = takeAllocation();
        if (allocation == 0U) return false;
        candidate->transport = std::make_shared<ExperimentResourceExternalTransport>(*context_,
            ExperimentResourceExternalTransport::Config{
                .elementStrideBytes = static_cast<std::uint32_t>(kGrammarBytes), .elementCount = 1U,
                .allocationSizeBytes = static_cast<std::uint32_t>(kGrammarBytes), .allocationGeneration = allocation});
        candidate->device = context_->device().logical();
        candidate->prefix = "RecursiveHopf.External." + std::to_string(allocation);
        ownsImports_ = true;
        for (std::uint32_t slot = 0U; slot < kExperimentExternalSlotCount; ++slot) {
            auto exported = candidate->transport->exportOpenClImport(slot);
            if (!exported) throw std::runtime_error("Hopf grammar FD export failed");
            ExportAttempt attempt{*exported};
            if (!producer_->importExternalGrammar(attempt.value)) throw std::runtime_error("Hopf grammar FD import failed");
        }
        state_ = std::move(candidate);
        diagnostic_ = "external grammar imported (awaiting graphics release)";
        return true;
    } catch (...) {
        if (ownsImports_) producer_->clearExternalGrammarImports();
        ownsImports_ = false;
        diagnostic_ = "host grammar (external import rejected)";
        return false;
    }
}

bool RecursiveHopfExternalGrammarTransport::publish(const RecursiveHopfDelivery& delivery) {
    if (!delivery.producedByOpenCl || delivery.generation == 0U || !initialize()) return false;
    auto owner = state_;
    if (owner->device != context_->device().logical()) {
        diagnostic_ = "host grammar (recreate external transport for replacement device)";
        return false;
    }
    if (owner->active && owner->active->matchesDelivery(*context_, delivery)) return true;
    if (owner->pending) return owner->pending->generation_ == delivery.generation
        && std::memcmp(&owner->pending->grammar_, &delivery.gpuGrammar, sizeof(delivery.gpuGrammar)) == 0;
    for (std::uint32_t slot = 0U; slot < kExperimentExternalSlotCount; ++slot) {
        const auto ticket = owner->writable[slot];
        if (owner->disabled[slot] || !owner->transport->canDispatchOpenClWrite(slot, ticket)) continue;
        // Allocate metadata before enqueueing ownership-changing work.
        auto view = std::shared_ptr<RecursiveHopfExternalGrammarView>(new RecursiveHopfExternalGrammarView);
        view->owner_ = owner->transport;
        view->context_ = context_;
        view->device_ = owner->device;
        view->generation_ = delivery.generation;
        view->grammar_ = delivery.gpuGrammar;
        view->slot_ = slot;
        view->transfer_ = ticket.generation;
        view->epoch_ = ticket.epoch;
        view->name_ = owner->prefix + ".Slot." + std::to_string(slot);
        const auto copied = producer_->copyExternalGrammar(slot, delivery);
        if (copied == RecursiveHopfOpenClProducer::ExternalGrammarCopyResult::Rejected) return false;
        if (copied == RecursiveHopfOpenClProducer::ExternalGrammarCopyResult::Failed) {
            static_cast<void>(owner->transport->quarantineTicket(slot, ticket));
            continue;
        }
        if (!owner->transport->markOpenClWriteComplete(slot, ticket)) {
            static_cast<void>(owner->transport->quarantineTicket(slot, ticket));
            continue;
        }
        owner->pending = std::move(view);
        diagnostic_ = "external grammar queued";
        return true;
    }
    return false;
}

std::shared_ptr<const RecursiveHopfExternalGrammarView>
RecursiveHopfExternalGrammarTransport::prepare(mgfx::Engine& engine, const RecursiveHopfDelivery* selection) {
    const auto generation = selection ? selection->generation : 0U;
    auto owner = state_;
    if (!owner || &engine.context() != context_ || owner->device != context_->device().logical()) return {};
    if (owner->recording) return selection && owner->recordingGeneration == generation && owner->active
        && owner->active->matchesDelivery(*context_, *selection) ? owner->active : nullptr;
    auto& loop = engine.frameLoop();
    if (!loop.canEnqueueSubmission({.commitCallbacks = 1U})
        || !engine.retainFramePayloadResources(owner, kRingBytes)) return {};
    if (!loop.enqueueSubmitCommit([owner](bool) { owner->recording = false; })) return {};
    owner->recording = true;
    owner->recordingGeneration = generation;

    // Retire a previous resident only when replacing/withdrawing its selection.
    if (owner->active && (!selection || !owner->active->matchesDelivery(*context_, *selection))) {
        if (!owner->active->usable_) owner->active.reset();
        else if (owner->release(engine, owner->active->slot_, owner->active, owner)) owner->active.reset();
        else return {};
    }
    if (owner->pending) {
        auto view = owner->pending;
        const bool selected = selection && selection->producedByOpenCl && generation != 0U
            && view->generation_ == generation
            && std::memcmp(&view->grammar_, &selection->gpuGrammar, sizeof(view->grammar_)) == 0;
        if (!loop.canEnqueueSubmission({.externalWaits = 1U, .externalSignals = selected ? 0U : 1U,
                .commitCallbacks = selected ? 1U : 2U})) return {};
        const auto lease = State::lease(*view);
        if (!owner->transport->acquireForVulkanRead(lease, view->transfer_)) return {};
        view->buffer_ = owner->transport->bufferFor(lease, view->transfer_);
        if (!view->buffer_) {
            static_cast<void>(owner->transport->quarantine(lease, view->transfer_));
            owner->pending.reset();
            return {};
        }
        if (!loop.enqueueSubmitCommit([owner, view, lease](bool accepted) {
                if (!accepted) {
                    view->usable_ = false;
                    static_cast<void>(owner->transport->quarantine(lease, view->transfer_));
                }
            })) throw std::logic_error("Hopf external acquire capacity changed after preflight");
        if (!loop.enqueueExternalWait(owner->transport->openClReadySemaphore(view->slot_),
                VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT))
            throw std::logic_error("Hopf external ready wait capacity changed after preflight");
        const auto handle = view->buffer_->handle();
        const auto family = context_->device().queueFamilies().graphics;
        const auto name = view->name_;
        engine.deferGraphPass([owner, handle, family, name](mgfx::vk::RenderGraph& graph) {
            const auto resource = graph.registerBuffer(name, handle, {}, 0U, kGrammarBytes);
            graph.addPass(name + ".Acquire")
                .readBuffer(resource, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT)
                .execute([handle, family](VkCommandBuffer cmd) { ownershipBarrier(cmd, handle, family, true); }).build();
        });
        view->usable_ = true; // same-recording consumer; submit failure rejects it
        owner->pending.reset();
        if (selected) owner->active = view;
        else if (!owner->release(engine, view->slot_, view, owner))
            throw std::logic_error("Hopf external discard capacity changed after preflight");
    }
    if (generation != 0U) {
        for (std::uint32_t slot = 0U; slot < kExperimentExternalSlotCount; ++slot)
            if (owner->transport->canScheduleVulkanRelease(slot))
                static_cast<void>(owner->release(engine, slot, {}, owner));
    }
    if (selection && owner->active && owner->active->matchesDelivery(*context_, *selection)) {
        diagnostic_ = "external grammar selected";
        return owner->active;
    }
    return {};
}

} // namespace epi_sculpt::recursive_hopf
