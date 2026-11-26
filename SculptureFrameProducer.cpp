#include "SculptureOpenClBackend.h"
#include "SculptureFrameProducer.h"
#include "SculptureProgram.h"
#include "SculptureFractalCatalog.h"
#include "SculptureTemporalHistory.h"
#include "SculptureVolumetricMemory.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <cstdint>
#include <filesystem>
#include <exception>
#include <fstream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <cstring>

#ifndef EPI_SCULPT_HAS_OPENCL
#define EPI_SCULPT_HAS_OPENCL 0
#endif

#if EPI_SCULPT_HAS_OPENCL
#define CL_TARGET_OPENCL_VERSION 300
#include <CL/cl.h>
#include <CL/cl_ext.h>
#endif

namespace epi_sculpt {


SculptureFrameProducer::SculptureFrameProducer()
    : SculptureFrameProducer(Config{}) {}

SculptureFrameProducer::SculptureFrameProducer(Config config) {
    // An explicit CPU selection is a deliberate debug/validation request;
    // only OpenCL failure recovery needs the separate opt-in.
    enableCpuFailureRecovery_ = config.enableCpuFailureRecovery || !config.preferOpenCl;
    if (!config.preferOpenCl) {
        backendName_ = "CPU";
        diagnostic_ = "CPU debug/validation backend selected by configuration";
        telemetry_.interop.noteHostStaged(SculptureInteropEvent::OpenClDisabled);
        return;
    }
    try {
        openCl_ = std::make_unique<OpenClState>(config);
#if EPI_SCULPT_HAS_OPENCL
        backendName_ = "OpenCL: " + openCl_->name();
        const SculptureInteropDecision interopDecision = decideSculptureInterop({
            config.vulkanExternalMemoryFd,
            config.vulkanExternalSemaphoreFd,
            config.matchDeviceUuid,
            openCl_->supportsExternalMemoryFd(),
            openCl_->supportsExternalMemorySynchronization(),
        });
        if (interopDecision.usesExternalMemory()) {
            telemetry_.interop.markEligible();
        } else {
            telemetry_.interop.noteHostStaged(
                SculptureInteropEvent::CapabilityHostStaged);
        }
        diagnostic_ = "triple-buffered OpenCL generation ready; "
            + std::string(interopDecision.reason);
#endif
    } catch (const std::exception& error) {
        openCl_.reset();
        backendName_ = "OpenCL unavailable";
        diagnostic_ = std::string("OpenCL initialization failed: ") + error.what();
        telemetry_.interop.noteHostStaged(
            SculptureInteropEvent::ProducerUnavailable);
    }
}

SculptureFrameProducer::~SculptureFrameProducer() {
    if (telemetry_.submissionIndeterminate && openCl_) {
        // A destructor cannot prove that native work or cross-API handoffs
        // retired. Stop before member destruction can free borrowed buffers.
        std::fprintf(stderr, "Sculpture producer: indeterminate native work; stopping before resource destruction\n");
        std::terminate();
    }
}

void SculptureFrameProducer::retainSafeObservation() noexcept {
    if (!lastSnapshotFromOpenCl_) return;
    delivery_ = safeStagedDelivery();
    if (delivery_.matchesSnapshot()) {
        lastSnapshotPtr_ = delivery_.snapshot;
        telemetry_.delivery = SculptureFrameDelivery::OpenClReused;
    } else {
        lastSnapshotPtr_ = nullptr;
        lastAnalyticsPtr_ = nullptr;
        hasLastSnapshot_ = false;
        lastSnapshotFromOpenCl_ = false;
        telemetry_.deliveredGeneration = 0U;
        telemetry_.outputFromOpenCl = false;
        telemetry_.delivery = SculptureFrameDelivery::None;
    }
}

void SculptureFrameProducer::enterExternalMemoryFallback(
    SculptureInteropEvent event, std::string diagnostic,
    bool preserveOpenClProducer) noexcept {
    telemetry_.interop.transitionToHostStaged(event);
    if (!preserveOpenClProducer) {
        retainSafeObservation();
        // Even an unrelated later fallback must not destroy a quarantined
        // backend whose native work has no completion evidence.
        if (!telemetry_.submissionIndeterminate) {
            openCl_.reset();
            backendName_ = "OpenCL unavailable";
        }
    }
    diagnostic_ = std::move(diagnostic);
}

// -----------------------------------------------------------------------------
// EDUCATIONAL ANNOTATION: ZERO-COPY INTEROP & FAULT TOLERANT FALLBACK
// -----------------------------------------------------------------------------
// This function establishes the highest-performance data pipeline between OpenCL 
// and Vulkan. Instead of copying gigabytes of math data across the PCIe bus, 
// we map Vulkan's device-local memory directly into OpenCL's address space 
// using POSIX file descriptors (Opaque FD KHR).
//
// FAULT TOLERANCE: If any capability is missing or if memory allocation fails,
// the engine gracefully degrades to "Host-Staged" mode (CPU memcpy over PCIe).
// -----------------------------------------------------------------------------
bool SculptureFrameProducer::importZeroCopyBuffers(std::span<const ZeroCopyBuffer> buffers) {
    if (telemetry_.submissionIndeterminate) return false;
    if (telemetry_.interop.state == SculptureInteropState::ExternalMemoryActive) {
        return true; // Already active and bound
    }
    
    if (telemetry_.interop.state != SculptureInteropState::ExternalMemoryEligible) {
        diagnostic_ = "host-staged Vulkan handoff: external memory requires verified device identity and synchronization";
        telemetry_.interop.noteHostStaged(SculptureInteropEvent::CapabilityHostStaged);
        return false;
    }
    
    // Strict ABI validation prevents driver crashes on invalid buffer descriptors
    if (!hasCompleteZeroCopyBufferSet(buffers)) {
        enterExternalMemoryFallback(
            SculptureInteropEvent::IncompleteSlotContract,
            "host-staged Vulkan handoff: incomplete external-memory slot contract", true);
        return false;
    }
    
    if (!openCl_) {
        enterExternalMemoryFallback(
            SculptureInteropEvent::ProducerUnavailable,
            "host-staged Vulkan handoff: OpenCL producer is unavailable", false);
        return false;
    }

    try {
        if (openCl_->importZeroCopyBuffers(buffers)) {
            telemetry_.interop.markActivated();
            diagnostic_ = "triple-buffered OpenCL generation ready; ZERO-COPY Vulkan ingest active";
            return true;
        }

        // Import can fail despite capability discovery (e.g., FD allocation limits)
        enterExternalMemoryFallback(
            SculptureInteropEvent::ImportRejected,
            "triple-buffered OpenCL generation ready; zero-copy failed, host fallback", true);
        return false;
        
    } catch (const std::exception& error) {
        // Critical OpenCL failure during memory rebinding.
        // Drop the broken OpenCL context entirely and fallback to pure CPU generation.
        enterExternalMemoryFallback(
            SculptureInteropEvent::ImportFailed,
            std::string("zero-copy import aborted; CPU fallback: ") + error.what(), false);
        return false;
    } catch (...) {
        enterExternalMemoryFallback(
            SculptureInteropEvent::ImportFailed,
            "zero-copy import aborted; CPU fallback (Unknown Error)", false);
        return false;
    }
}

void SculptureFrameProducer::clearZeroCopyBuffers() noexcept {
    if (telemetry_.submissionIndeterminate) return;
    // A native-device boundary invalidates every old semaphore epoch even when
    // the mathematical generation remains perfectly valid. Keep the latest
    // producer-owned host copy visible and erase all Vulkan-allocation identity.
    externalSlotOwnership_.forceReset();
    if (delivery_.usesExternalMemory()) {
        delivery_ = safeStagedDelivery();
        if (delivery_.matchesSnapshot()) {
            // The old mapped slot can disappear during binding release. Future
            // no-completion frames must continue from the owned host copy.
            lastSnapshotPtr_ = delivery_.snapshot;
        } else {
            lastSnapshotPtr_ = nullptr;
            lastAnalyticsPtr_ = nullptr;
            hasLastSnapshot_ = false;
            lastSnapshotFromOpenCl_ = false;
            telemetry_.deliveredGeneration = 0U;
            telemetry_.outputFromOpenCl = false;
            telemetry_.delivery = SculptureFrameDelivery::None;
        }
    }

    if (openCl_ == nullptr) return;
#if EPI_SCULPT_HAS_OPENCL
    try {
        if (openCl_->releaseExternalPresentationBindings()) {
            telemetry_.interop.transitionToHostStaged(
                SculptureInteropEvent::CapabilityHostStaged);
            diagnostic_ = "OpenCL generation ready; Vulkan external buffers detached";
            return;
        }
        enterExternalMemoryFallback(
            SculptureInteropEvent::ImportFailed,
            "OpenCL external buffers detached, but local output restoration failed",
            false);
    } catch (const std::exception& error) {
        enterExternalMemoryFallback(
            SculptureInteropEvent::ImportFailed,
            std::string("OpenCL external-buffer detach failed: ") + error.what(),
            false);
    } catch (...) {
        enterExternalMemoryFallback(
            SculptureInteropEvent::ImportFailed,
            "OpenCL external-buffer detach failed with unknown error",
            false);
    }
#endif
}

bool SculptureFrameProducer::requalifyExternalMemoryInterop(
    const std::array<std::uint8_t, 16>& vulkanDeviceUuid,
    bool vulkanExternalMemoryFd,
    bool vulkanExternalSemaphoreFd) noexcept {
    if (telemetry_.submissionIndeterminate) return false;
    if (openCl_ == nullptr) {
        telemetry_.interop.noteHostStaged(SculptureInteropEvent::ProducerUnavailable);
        return false;
    }

#if EPI_SCULPT_HAS_OPENCL
    const SculptureInteropDecision decision = decideSculptureInterop({
        vulkanExternalMemoryFd,
        vulkanExternalSemaphoreFd,
        openCl_->deviceUuidMatches(vulkanDeviceUuid),
        openCl_->supportsExternalMemoryFd(),
        openCl_->supportsExternalMemorySynchronization(),
    });
    if (decision.usesExternalMemory()) {
        telemetry_.interop.markEligible();
    } else {
        telemetry_.interop.noteHostStaged(SculptureInteropEvent::CapabilityHostStaged);
    }
    diagnostic_ = "triple-buffered OpenCL generation ready; "
        + std::string(decision.reason);
    return decision.usesExternalMemory();
#else
    (void)vulkanDeviceUuid;
    (void)vulkanExternalMemoryFd;
    (void)vulkanExternalSemaphoreFd;
    telemetry_.interop.noteHostStaged(SculptureInteropEvent::ProducerUnavailable);
    return false;
#endif
}

bool SculptureFrameProducer::externalMemoryInteropReady() const noexcept {
    return !telemetry_.submissionIndeterminate
        && telemetry_.interop.state != SculptureInteropState::HostStaged
        && openCl_ != nullptr;
}

SculptureInteropState SculptureFrameProducer::interopState() const noexcept {
    return telemetry_.interop.state;
}

const SculptureSnapshotDelivery& SculptureFrameProducer::delivery() const noexcept {
    return delivery_;
}

SculptureSnapshotDelivery SculptureFrameProducer::safeStagedDelivery() const noexcept {
    if (!hasLastSnapshot_ || telemetry_.deliveredGeneration == 0U) return {};

    std::uint64_t gen = 0U;
    const auto* snapshot = hostStagingRing_ ? hostStagingRing_->readLatest(&gen) : nullptr;
    if (snapshot != nullptr && gen == telemetry_.deliveredGeneration
        && snapshot->frame.generation == gen) {
        return {
            .snapshot = snapshot,
            .generation = static_cast<std::uint32_t>(gen),
            .storage = SculptureSnapshotStorage::HostStaged,
            .spatialLease = lastHostSpatialGeneration_ == gen ? lastHostSpatialLease_ : nullptr,
            .spatialGeneration = lastHostSpatialGeneration_ == gen ? lastHostSpatialGeneration_ : 0U,
        };
    }
    // Only CPU production populates lastSnapshot_. A previous CPU result may
    // still be present after OpenCL takes over; it is not an OpenCL fallback.
    if (lastSnapshotFromOpenCl_ || !lastCpuSnapshotValid_ || lastSnapshot_ == nullptr
        || lastCpuSnapshotGeneration_ != telemetry_.deliveredGeneration
        || lastSnapshot_->frame.generation != telemetry_.deliveredGeneration) return {};
    return {
        .snapshot = lastSnapshot_.get(),
        .generation = lastSnapshot_->frame.generation,
        .storage = SculptureSnapshotStorage::HostStaged,
    };
}

bool SculptureFrameProducer::acquireExternalDelivery(
    const SculptureSnapshotDelivery& delivery) noexcept {
    if (telemetry_.submissionIndeterminate) return false;
    const bool acquired = delivery.usesExternalMemory()
        && delivery.matchesSnapshot()
        && externalSlotOwnership_.acquireForVulkan(
            delivery.slotIndex, {delivery.generation, delivery.ownershipEpoch});
    if (!acquired) telemetry_.interop.noteAcquireRejected();
    return acquired;
}

bool SculptureFrameProducer::releaseExternalDelivery(
    const SculptureSnapshotDelivery& delivery) noexcept {
    if (telemetry_.submissionIndeterminate) return false;
    if (!delivery.usesExternalMemory() || !delivery.matchesSnapshot()) {
        telemetry_.interop.noteReleaseRejected(false);
        return false;
    }
    const SculptureExternalSlotTicket ticket{
        delivery.generation, delivery.ownershipEpoch};
    if (!externalSlotOwnership_.releaseFromVulkanRead(delivery.slotIndex, ticket)) {
        telemetry_.interop.noteReleaseRejected(
            markExternalSlotUnavailable(delivery.slotIndex, ticket));
        diagnostic_ = "external delivery release rejected; slot unavailable";
        return false;
    }
#if EPI_SCULPT_HAS_OPENCL
    const bool recycled = openCl_ != nullptr
        && openCl_->recycleExternalSlot(delivery.slotIndex, ticket);
    if (!recycled) {
        telemetry_.interop.noteReleaseRejected(
            markExternalSlotUnavailable(delivery.slotIndex, ticket));
        diagnostic_ = "external delivery recycle rejected; slot unavailable";
    }
    return recycled;
#else
    telemetry_.interop.noteReleaseRejected(
        markExternalSlotUnavailable(delivery.slotIndex, ticket));
    diagnostic_ = "external delivery release unavailable without OpenCL";
    return false;
#endif
}

bool SculptureFrameProducer::markExternalSlotUnavailable(
    std::uint32_t slotIndex, SculptureExternalSlotTicket ticket) noexcept {
    return externalSlotOwnership_.quarantine(slotIndex, ticket);
}

void SculptureFrameProducer::publishDelivery(
    const gpu_abi::PackedSculptureSnapshot* snapshot, const gpu_abi::SculptureAnalyticsGpu* analytics, bool fromOpenCl,
    std::shared_ptr<const std::vector<float>> spatialLease, std::uint32_t spatialGeneration) noexcept {
    delivery_ = {.snapshot = snapshot,
                 .analytics = analytics,
                 .generation = snapshot ? snapshot->frame.generation : 0U,
                 .slotIndex = SculptureSnapshotDelivery::kNoSlot,
                 .ownershipEpoch = 0U};
    if (fromOpenCl
        && telemetry_.interop.state == SculptureInteropState::ExternalMemoryActive
        && openCl_) {
        const auto slotIndex = openCl_->slotIndexOf(snapshot);
        SculptureExternalSlotTicket ticket{};
        if (slotIndex != SculptureSnapshotDelivery::kNoSlot
            && externalSlotOwnership_.readyTicket(slotIndex, &ticket)
            && ticket.generation == delivery_.generation) {
            delivery_.slotIndex = slotIndex;
            delivery_.ownershipEpoch = ticket.epoch;
            delivery_.storage = SculptureSnapshotStorage::ExternalMemory;
        }
    }
    if (delivery_.generation != 0U && spatialGeneration == delivery_.generation) {
        if (delivery_.usesExternalMemory() || spatialLease) {
            delivery_.spatialLease = std::move(spatialLease);
            delivery_.spatialGeneration = spatialGeneration;
        }
    }
    if (snapshot != nullptr && hostStagingRing_) {
        std::uint64_t stagedGeneration = 0U;
        const auto* staged = hostStagingRing_->readLatest(&stagedGeneration);
        // A reused delivery may already point at this owned snapshot. Avoid
        // copying a staging slot onto itself or rewriting its publication.
        if (snapshot != staged || stagedGeneration != delivery_.generation) {
            hostStagingRing_->write(delivery_.generation, *snapshot, delivery_.generation);
        }
    }
}


const gpu_abi::PackedSculptureSnapshot* SculptureFrameProducer::acceptOpenClCompletion(
    const gpu_abi::PackedSculptureSnapshot* completed,
    const gpu_abi::SculptureAnalyticsGpu* analytics,
    const std::shared_ptr<const std::vector<float>>& spatialLease,
    std::uint32_t spatialGeneration) noexcept {
    bool acceptedOpenClCompletion = false;
    bool freshExternalDelivery = false;
    if (completed != nullptr) {
        const std::uint32_t completedGeneration = completed->frame.generation;
        // Delivery slots can be queued for reuse immediately after a
        // Vulkan handoff. Compare against the copied scalar record,
        // never against memory an asynchronous producer may rewrite.
        // Compact generations are ordered without wrap in this session.
        if (!hasLastSnapshot_
            || completedGeneration >= telemetry_.deliveredGeneration) {
            lastSnapshotPtr_ = completed;
            lastHostSpatialLease_ = spatialLease;
            lastHostSpatialGeneration_ = spatialLease
                ? spatialGeneration : 0U;
            if (analytics) {
                // External slot metadata can be recycled after Vulkan
                // handoff; retain analytics as producer-owned values.
                lastAnalytics_ = *analytics;
                lastAnalyticsPtr_ = &lastAnalytics_;
            } else {
                lastAnalyticsPtr_ = nullptr;
            }
            hasLastSnapshot_ = true;
            lastSnapshotFromOpenCl_ = true;
            acceptedOpenClCompletion = true;
            freshExternalDelivery = telemetry_.interop.state
                == SculptureInteropState::ExternalMemoryActive;
        }
    }

    if (!acceptedOpenClCompletion
        && telemetry_.interop.state == SculptureInteropState::ExternalMemoryActive
        && lastSnapshotFromOpenCl_ && lastSnapshotPtr_ != lastSnapshot_.get()) {
        // The external slot may already belong to Vulkan. Reuse only
        // the owned copy of the same generation, never the unrelated
        // CPU-recovery buffer or a recycled shared allocation.
        const SculptureSnapshotDelivery staged = safeStagedDelivery();
        if (!staged.matchesSnapshot()) {
            hasLastSnapshot_ = false;
            lastSnapshotFromOpenCl_ = false;
            lastSnapshotPtr_ = nullptr;
            lastAnalyticsPtr_ = nullptr;
            telemetry_.deliveredGeneration = 0U;
            telemetry_.outputFromOpenCl = false;
            telemetry_.delivery = SculptureFrameDelivery::None;
            delivery_ = {};
            return nullptr;
        }
        lastSnapshotPtr_ = staged.snapshot;
    }

    if (!hasLastSnapshot_) {
        // A queued OpenCL dispatch has not completed yet.  Preserve
        // the GPU-only contract during warm-up rather than evaluating
        // the field again on CPU.
        telemetry_.outputFromOpenCl = false;
        delivery_ = {};
        return nullptr;
    } else if (!acceptedOpenClCompletion) {
        ++telemetry_.staleFrameReuses;
        telemetry_.delivery = lastSnapshotFromOpenCl_
            ? SculptureFrameDelivery::OpenClReused
            : SculptureFrameDelivery::CpuGenerated;
    } else {
        telemetry_.delivery = lastSnapshotFromOpenCl_
            ? SculptureFrameDelivery::OpenClFresh
            : SculptureFrameDelivery::CpuGenerated;
    }

    telemetry_.deliveredGeneration = lastSnapshotPtr_->frame.generation;
    telemetry_.outputFromOpenCl = lastSnapshotFromOpenCl_;
    // OpenCL owns this completed frame; CPU evaluation time must stay
    // visibly zero so telemetry cannot imply a hidden sidecar lane.
    telemetry_.cpuCallMilliseconds = 0.0f;

    publishDelivery(lastSnapshotPtr_, lastAnalyticsPtr_, freshExternalDelivery,
        lastHostSpatialLease_, freshExternalDelivery
            ? spatialGeneration : lastHostSpatialGeneration_);
    return lastSnapshotPtr_;
}

const gpu_abi::PackedSculptureSnapshot* SculptureFrameProducer::pollCompleted() {
    telemetry_.submissionAccepted = false;
    if (telemetry_.submissionIndeterminate || !openCl_)
        return delivery_.matchesSnapshot() ? delivery_.snapshot : nullptr;
#if EPI_SCULPT_HAS_OPENCL
    try {
        const auto result = openCl_->pollAndTakeLatest(
            telemetry_.interop.state == SculptureInteropState::ExternalMemoryActive
                ? &externalSlotOwnership_ : nullptr);
        telemetry_.pendingSlots = result.pendingSlots;
        telemetry_.openClCompletionMilliseconds = result.completionMilliseconds;
        return acceptOpenClCompletion(result.completed, result.analytics,
            result.spatialLease, result.spatialGeneration);
    } catch (const std::exception& error) {
        telemetry_.submissionIndeterminate = true;
        openCl_->quarantineBackgroundWork();
        openCl_->settleCompilerPublication();
        externalSlotOwnership_.noteQuarantinedNativeWork();
        retainSafeObservation();
        enterExternalMemoryFallback(SculptureInteropEvent::DispatchFailed,
            std::string("OpenCL completion outcome indeterminate: ") + error.what(), true);
    } catch (...) {
        telemetry_.submissionIndeterminate = true;
        openCl_->quarantineBackgroundWork();
        openCl_->settleCompilerPublication();
        externalSlotOwnership_.noteQuarantinedNativeWork();
        retainSafeObservation();
        enterExternalMemoryFallback(SculptureInteropEvent::DispatchFailed,
            "OpenCL completion outcome indeterminate: unknown exception", true);
    }
    telemetry_.temporalSourceGeneration = 0U;
    telemetry_.temporalHistoryUsed = false;
#endif
    return delivery_.matchesSnapshot() ? delivery_.snapshot : nullptr;
}

const gpu_abi::PackedSculptureSnapshot* SculptureFrameProducer::produce(
    const SculptureProgramParams& params,
    float time,
    float morph,
    float deltaSeconds,
    std::uint32_t generation) {
    const auto begin = std::chrono::steady_clock::now();
    telemetry_.requestedGeneration = generation;
    telemetry_.submissionAccepted = false;
    if (generation <= lastAcceptedGeneration_) {
        // Zero is absent; a previously accepted identity must never run a
        // second mathematical step. Backpressure does not advance this mark.
        ++telemetry_.droppedSubmissions;
        return nullptr;
    }
    if (telemetry_.submissionIndeterminate) {
        // Neither retry nor CPU recovery can establish whether the failed
        // native request changed mathematical history. Preserve observation
        // only; the native owner must resolve retirement separately.
        ++telemetry_.droppedSubmissions;
        return delivery_.matchesSnapshot() ? delivery_.snapshot : nullptr;
    }
    if (openCl_ != nullptr) {
        try {
#if EPI_SCULPT_HAS_OPENCL
            auto result = openCl_->submitAndTakeLatest(
                params, time, morph, deltaSeconds, generation,
                telemetry_.interop.state == SculptureInteropState::ExternalMemoryActive
                    ? &externalSlotOwnership_
                    : nullptr);
            telemetry_.pendingSlots = result.pendingSlots;
            telemetry_.submissionAccepted = result.submitted;
            if (result.submitted) lastAcceptedGeneration_ = generation;
            telemetry_.openClCompletionMilliseconds =
                result.completionMilliseconds;
            telemetry_.temporalSourceGeneration =
                result.temporalSourceGeneration;
            telemetry_.temporalHistoryUsed = result.temporalHistoryUsed;
            telemetry_.pendingCompilations = openCl_->isCompiling() ? 1U : 0U;
            telemetry_.programCacheSize = openCl_->programCacheSize();
            telemetry_.programCacheHits = openCl_->programCacheHits();
            telemetry_.programCacheMisses = openCl_->programCacheMisses();
            if (!result.submitted) {
                ++telemetry_.droppedSubmissions;
            }

            
            return acceptOpenClCompletion(result.completed, result.analytics,
                result.spatialLease, result.spatialGeneration);
#endif
        } catch (const std::exception& error) {
            // enqueue may have issued native work before throwing. Retain
            // the backend and publish only an already-owned staged copy.
            telemetry_.submissionIndeterminate = true;
            if (!telemetry_.submissionAccepted) ++telemetry_.droppedSubmissions;
#if EPI_SCULPT_HAS_OPENCL
            openCl_->quarantineBackgroundWork();
            openCl_->settleCompilerPublication();
#endif
            externalSlotOwnership_.noteQuarantinedNativeWork();
            retainSafeObservation();
            enterExternalMemoryFallback(
                SculptureInteropEvent::DispatchFailed,
                std::string("OpenCL dispatch outcome indeterminate: ") + error.what(), true);
            telemetry_.temporalSourceGeneration = 0U;
            telemetry_.temporalHistoryUsed = false;
            return delivery_.matchesSnapshot() ? delivery_.snapshot : nullptr;
        } catch (...) {
            telemetry_.submissionIndeterminate = true;
            if (!telemetry_.submissionAccepted) ++telemetry_.droppedSubmissions;
#if EPI_SCULPT_HAS_OPENCL
            openCl_->quarantineBackgroundWork();
            openCl_->settleCompilerPublication();
#endif
            externalSlotOwnership_.noteQuarantinedNativeWork();
            retainSafeObservation();
            enterExternalMemoryFallback(
                SculptureInteropEvent::DispatchFailed,
                "OpenCL dispatch outcome indeterminate: unknown exception", true);
            telemetry_.temporalSourceGeneration = 0U;
            telemetry_.temporalHistoryUsed = false;
            return delivery_.matchesSnapshot() ? delivery_.snapshot : nullptr;
        }
    }
    if (!enableCpuFailureRecovery_) {
        // Preserve temporal continuity without silently changing numerical
        // authority. `safeStagedDelivery` is a producer-owned copy, so an
        // external-memory failure cannot leave Vulkan referencing a retired
        // import or a slot that has just been quarantined.
        const SculptureSnapshotDelivery preserved = safeStagedDelivery();
        if (lastSnapshotFromOpenCl_ && preserved.matchesSnapshot()) {
            delivery_ = preserved;
            telemetry_.deliveredGeneration = preserved.generation;
            telemetry_.outputFromOpenCl = true;
            telemetry_.delivery = SculptureFrameDelivery::OpenClReused;
            telemetry_.cpuCallMilliseconds = 0.0f;
            ++telemetry_.staleFrameReuses;
            return preserved.snapshot;
        }
        telemetry_.outputFromOpenCl = false;
        telemetry_.delivery = SculptureFrameDelivery::None;
        delivery_ = {};
        return nullptr;
    }
    backendName_ = "CPU recovery";
    (void)produceCpu(
        params, time, morph, deltaSeconds, generation);
    lastAcceptedGeneration_ = generation;
    telemetry_.submissionAccepted = true;
    // CPU evaluation does not produce the OpenCL analytics sidecar. Do not
    // attach a previous GPU frame's measurements to this new CPU snapshot.
    lastAnalyticsPtr_ = nullptr;
    hasLastSnapshot_ = true;
    lastSnapshotFromOpenCl_ = false;
    lastSnapshotPtr_ = lastSnapshot_.get();
    telemetry_.deliveredGeneration = generation;
    telemetry_.outputFromOpenCl = false;
    telemetry_.delivery = SculptureFrameDelivery::CpuGenerated;
    telemetry_.pendingSlots = 0U;
    telemetry_.temporalSourceGeneration = 0U;
    telemetry_.temporalHistoryUsed = false;
    telemetry_.cpuCallMilliseconds = std::chrono::duration<float, std::milli>(
        std::chrono::steady_clock::now() - begin).count();
    publishDelivery(lastSnapshot_.get(), lastAnalyticsPtr_, false);
    return lastSnapshot_.get();
}

WorldSnapshotDelivery SculptureFrameProducer::produceWorld(
    const SculptureProgramParams& params,
    const WorldRecipe& recipe,
    float time,
    float morph,
    float deltaSeconds,
    std::uint32_t generation) noexcept {
    worldTelemetry_.requestedGeneration = generation;
    worldTelemetry_.submissionAccepted = false;
    if (telemetry_.submissionIndeterminate) {
        worldTelemetry_.outputFromOpenCl = false;
        return {};
    }
    if (!recipe.needsDedicatedGenerator() || openCl_ == nullptr) {
        worldTelemetry_.outputFromOpenCl = false;
        worldTelemetry_.pendingSlots = 0U;
        return worldProducer_.latestDelivery();
    }

#if EPI_SCULPT_HAS_OPENCL
    try {
        const auto result = openCl_->submitWorldAndTakeLatest(
            params, recipe, time, morph, deltaSeconds, generation);
        worldRuntimeDiagnostic_.clear();
        worldTelemetry_.openClCompletionMilliseconds = result.completionMilliseconds;
        worldTelemetry_.pendingSlots = result.pendingSlots;
        worldTelemetry_.pendingCompilations = result.pendingCompilations;
        worldTelemetry_.submissionAccepted = result.submitted;
        if (!result.submitted) ++worldTelemetry_.droppedSubmissions;
        if (result.completed != nullptr) {
            // Publication pins this slot awaiting the ticket's Vulkan submit
            // callback. Acknowledgment alone never permits another GPU write.
            const bool published = result.completedMetrics != nullptr
                && worldProducer_.publishOpenClCompletion(
                    *result.completed, *result.completedMetrics, result.completedTicket);
            if (published) {
                openCl_->acknowledgeWorldDelivery(result.completed);
                worldTelemetry_.deliveredGeneration = result.completedMetrics->generation;
                worldTelemetry_.outputFromOpenCl = true;
                worldTelemetry_.openClCompletionMilliseconds =
                    result.completedMetrics->queueToDeliveryMilliseconds;
                worldTelemetry_.openClKernelMilliseconds =
                    result.completedMetrics->kernelMilliseconds;
                worldTelemetry_.openClReadbackMilliseconds =
                    result.completedMetrics->readbackMilliseconds;
                worldTelemetry_.openClReadbackBytes =
                    result.completedMetrics->readbackBytes;
                worldTelemetry_.activeInstanceCount =
                    result.completedMetrics->activeInstanceCount;
            } else {
                // A rejected external delivery has no Vulkan consumer. Keep it
                // pinned rather than recycling an unconsumed ready semaphore.
                worldTelemetry_.outputFromOpenCl = false;
                try {
                    worldRuntimeDiagnostic_ =
                        "dedicated world completion metadata rejected; external slot retained";
                } catch (...) {
                    worldRuntimeDiagnostic_.clear();
                }
            }
        }
    } catch (const std::exception& error) {
        // The compact lane remains useful if an optional world kernel becomes
        // unavailable.  No CPU world generation is substituted here.
        worldTelemetry_.outputFromOpenCl = false;
        try {
            worldRuntimeDiagnostic_ = std::string("dedicated world dispatch failed: ")
                + error.what();
        } catch (...) {
            worldRuntimeDiagnostic_.clear();
        }
    } catch (...) {
        worldTelemetry_.outputFromOpenCl = false;
        try {
            worldRuntimeDiagnostic_ = "dedicated world dispatch failed: unknown error";
        } catch (...) {
            worldRuntimeDiagnostic_.clear();
        }
    }
#else
    (void)params;
    (void)time;
    (void)morph;
    (void)deltaSeconds;
    (void)generation;
#endif
    return worldProducer_.latestDelivery();
}

WorldSnapshotDelivery SculptureFrameProducer::worldDelivery() const noexcept {
    if (telemetry_.submissionIndeterminate) return {};
    return worldProducer_.latestDelivery();
}

const gpu_abi::PackedSculptureSnapshot* SculptureFrameProducer::produceCpu(
    const SculptureProgramParams& params,
    float time,
    float morph,
    float deltaSeconds,
    std::uint32_t generation) const {
    const SculptureProgram program{params};
    
    // Emergency recovery only: OpenCL normally owns every mathematical frame.
    // Keep a dedicated CPU state buffer rather than aliasing a packed OpenCL
    // buffer; those layouts are intentionally incompatible.
    // GPU-to-CPU recovery deliberately starts without a CPU field checkpoint.
    // For CPU continuation keep input and output distinct: evaluate mutates
    // its output before consuming spatial history and can throw partway through.
    if (!candidateCpuSnapshot_) candidateCpuSnapshot_ = std::make_unique<SculptureSnapshot>();
    const SculptureSnapshot* previous = hasLastSnapshot_ && !lastSnapshotFromOpenCl_
        && lastCpuSnapshotValid_ && canUseTemporalHistory(params, lastCpuTemporalFamily_)
        ? lastCpuSnapshot_.get() : nullptr;
    program.evaluate(time, morph, *candidateCpuSnapshot_, previous);
    gpu_abi::packSnapshot(*candidateCpuSnapshot_, deltaSeconds, generation, *lastSnapshot_);
    lastCpuSnapshot_.swap(candidateCpuSnapshot_);
    lastCpuSnapshotGeneration_ = generation;
    lastCpuSnapshotValid_ = true;
    lastCpuTemporalFamily_ = temporalFamily(params);
        
    std::uint32_t effStageMask = params.stageMask;
    if (params.outputMode == 1 || params.outputModeB == 1) effStageMask |= 32768U;
    
    lastSnapshot_->reserved0 = (effStageMask << 8) | (static_cast<std::uint32_t>(params.colorEffect) & 0xFF);
    
    return lastSnapshot_.get();
}

void SculptureFrameProducer::cancelExternalDelivery(
    const SculptureSnapshotDelivery& delivery) noexcept {
    if (telemetry_.submissionIndeterminate) return;
    if (!delivery.usesExternalMemory() || !delivery.matchesSnapshot()) {
        return;
    }
    const SculptureExternalSlotTicket ticket{
        delivery.generation, delivery.ownershipEpoch};
    telemetry_.interop.noteReleaseRejected(
        markExternalSlotUnavailable(delivery.slotIndex, ticket));
    diagnostic_ = "external delivery cancelled (queue submit failed); slot quarantined";
}

bool SculptureFrameProducer::usingOpenCl() const noexcept {
    return openCl_ != nullptr && !telemetry_.submissionIndeterminate;
}

bool SculptureFrameProducer::worldKernelReady() const noexcept {
#if EPI_SCULPT_HAS_OPENCL
    return usingOpenCl() && openCl_->worldKernelReady();
#else
    return false;
#endif
}

bool SculptureFrameProducer::prewarmWorldKernel(WorldGeneratorKind generator) noexcept {
#if EPI_SCULPT_HAS_OPENCL
    return usingOpenCl() && openCl_->prewarmWorldKernel(generator);
#else
    (void)generator;
    return false;
#endif
}

bool SculptureFrameProducer::isWorldKernelCompiled(WorldGeneratorKind generator) const noexcept {
#if EPI_SCULPT_HAS_OPENCL
    return usingOpenCl() && openCl_->isWorldKernelCompiled(generator);
#else
    (void)generator;
    return false;
#endif
}

std::size_t SculptureFrameProducer::compiledWorldKernelCount() const noexcept {
#if EPI_SCULPT_HAS_OPENCL
    return usingOpenCl() ? openCl_->compiledWorldKernelCount() : 0U;
#else
    return 0U;
#endif
}

std::string_view SculptureFrameProducer::backendName() const noexcept {
    if (telemetry_.submissionIndeterminate) return "OpenCL faulted";
    return backendName_;
}

std::string_view SculptureFrameProducer::diagnostic() const noexcept {
    return diagnostic_;
}

std::string_view SculptureFrameProducer::worldDiagnostic() const noexcept {
    if (telemetry_.submissionIndeterminate)
        return "OpenCL producer faulted; world production disabled";
#if EPI_SCULPT_HAS_OPENCL
    if (!worldRuntimeDiagnostic_.empty()) {
        return worldRuntimeDiagnostic_;
    }
    // Keep the view bound to OpenClState's persistent diagnostic string.
    // A conditional expression between std::string and a literal would create
    // a temporary std::string and leave this returned view dangling.
    if (openCl_ != nullptr) {
        return std::string_view{openCl_->worldDiagnostic()};
    }
    return "OpenCL producer unavailable";
#else
    return "OpenCL support unavailable";
#endif
}

float SculptureFrameProducer::lastFrameMilliseconds() const noexcept {
    return telemetry_.cpuCallMilliseconds;
}

const SculptureProducerTelemetry& SculptureFrameProducer::telemetry() const noexcept {
    return telemetry_;
}

const SculptureWorldProducerTelemetry& SculptureFrameProducer::worldTelemetry() const noexcept {
    return worldTelemetry_;
}

bool SculptureFrameProducer::waitIdle() noexcept {
    // A queue drain alone cannot establish settlement of a failed partial
    // enqueue and its external semaphore handoff.
    if (telemetry_.submissionIndeterminate) return false;
    if (openCl_ == nullptr) {
        return true;
    }
    try {
#if EPI_SCULPT_HAS_OPENCL
        openCl_->waitIdle();
#endif
    } catch (const std::exception& error) {
        std::fprintf(stderr, "[OpenCL] Producer drain failed: %s\n", error.what());
        return false;
    } catch (...) {
        std::fprintf(stderr, "[OpenCL] Producer drain failed with unknown exception\n");
        return false;
    }
    return true;
}

bool SculptureFrameProducer::reloadKernels() noexcept {
#if EPI_SCULPT_HAS_OPENCL
    try {
        if (usingOpenCl()) return openCl_->triggerHotReload();
    } catch (const std::exception& error) {
        std::fprintf(stderr, "[OpenCL] Reload request failed: %s\n", error.what());
    } catch (...) {
        std::fprintf(stderr, "[OpenCL] Reload request failed\n");
    }
#endif
    return false;
}

bool SculptureFrameProducer::isKernelCompilationActive() const noexcept {
#if EPI_SCULPT_HAS_OPENCL
    if (openCl_) {
        return openCl_->isCompiling();
    }
#endif
    return false;
}

SculptureKernelBuildIdentity SculptureFrameProducer::kernelBuildIdentity() const noexcept {
#if EPI_SCULPT_HAS_OPENCL
    if (openCl_) return openCl_->kernelBuildIdentity();
#endif
    return {};
}

std::size_t SculptureFrameProducer::programCacheSize() const noexcept {
#if EPI_SCULPT_HAS_OPENCL
    if (openCl_) {
        return openCl_->programCacheSize();
    }
#endif
    return 0U;
}

std::size_t SculptureFrameProducer::programCacheHits() const noexcept {
#if EPI_SCULPT_HAS_OPENCL
    if (openCl_) {
        return openCl_->programCacheHits();
    }
#endif
    return 0U;
}

std::size_t SculptureFrameProducer::programCacheMisses() const noexcept {
#if EPI_SCULPT_HAS_OPENCL
    if (openCl_) {
        return openCl_->programCacheMisses();
    }
#endif
    return 0U;
}

void SculptureFrameProducer::startPrewarming(const SculpturePresetLibrary& library) noexcept {
#if EPI_SCULPT_HAS_OPENCL
    if (usingOpenCl()) {
        openCl_->startPrewarming(library);
    }
#endif
}

void SculptureFrameProducer::stopPrewarming() noexcept {
#if EPI_SCULPT_HAS_OPENCL
    if (openCl_) {
        openCl_->stopPrewarming();
    }
#endif
}

bool SculptureFrameProducer::isPrewarming() const noexcept {
#if EPI_SCULPT_HAS_OPENCL
    if (openCl_) {
        return openCl_->isPrewarming();
    }
#endif
    return false;
}

void SculptureFrameProducer::triggerAsyncRecompile(const SculptureProgramParams& params) noexcept {
#if EPI_SCULPT_HAS_OPENCL
    if (usingOpenCl()) {
        openCl_->triggerAsyncRecompile(params);
    }
#else
    (void)params;
#endif
}




bool SculptureFrameProducer::importGridExternalOutput(std::uint32_t slotIndex, const SculptureOpenClImportSlot& importSlot) {
    if (!usingOpenCl()) return false;
    return openCl_->importGridExternalOutput(slotIndex, importSlot);
}

bool SculptureFrameProducer::submitGridTransform(const SculptureProgramParams& params, float time, float morph, std::uint32_t gridResolution, std::uint32_t slotIndex) {
    if (!usingOpenCl()) return false;
    return openCl_->submitGridTransform(params, time, morph, gridResolution, slotIndex);
}

bool SculptureFrameProducer::importWorldExternalOutput(std::uint32_t slotIndex, const WorldOpenClImportSlot& importSlot) {
#if EPI_SCULPT_HAS_OPENCL
    if (!usingOpenCl() || !openCl_->worldDispatcher_) return false;
    return openCl_->worldDispatcher_->importExternalOutput(slotIndex, importSlot);
#else
    return false;
#endif
}

bool SculptureFrameProducer::clearWorldExternalOutputs() noexcept {
    if (telemetry_.submissionIndeterminate) return false;
#if EPI_SCULPT_HAS_OPENCL
    if (openCl_ && openCl_->worldDispatcher_) {
        if (!openCl_->worldDispatcher_->clearExternalOutputs()) return false;
    }
#endif
    // Published world tickets refer to allocations owned by the retiring
    // Vulkan device. Preserve the producer itself, never those native leases.
    worldProducer_.reset();
    return true;
}

bool SculptureFrameProducer::initializeVolumetricMemoryGpu(
    SculptureVolumetricMemory& volMem,
    const std::string& kernelSourceOrPath) noexcept {
#if EPI_SCULPT_HAS_OPENCL
    if (!usingOpenCl() || !openCl_->context_ || !openCl_->device_) return false;
    if (volMem.isGpuInitialized() && volMem.openClContext() == openCl_->context_) {
        return volMem.isGpuActive();
    }
    try {
        return volMem.initGpu(openCl_->context_, openCl_->device_, kernelSourceOrPath);
    } catch (...) {
        return false;
    }
#else
    (void)volMem;
    (void)kernelSourceOrPath;
    return false;
#endif
}

void SculptureFrameProducer::bindWorldVolumetricCoupling(
    const SculptureVolumetricMemory& volMem,
    const volumetric_gpu_abi::VolumetricCouplerConfigGpu& couplerConfig) noexcept {
#if EPI_SCULPT_HAS_OPENCL
    if (!usingOpenCl() || !openCl_->worldDispatcher_) return;
    if (!volMem.isGpuActive() || volMem.openClContext() != openCl_->context_) {
        openCl_->worldDispatcher_->clearVolumetricCoupling();
        return;
    }
    openCl_->worldDispatcher_->bindVolumetricCoupling(
        volMem.activeGpuReadBuffer(),
        volMem.descriptorGpuBuffer(),
        couplerConfig);
#else
    (void)volMem;
    (void)couplerConfig;
#endif
}

void SculptureFrameProducer::clearWorldVolumetricCoupling() noexcept {
#if EPI_SCULPT_HAS_OPENCL
    if (usingOpenCl() && openCl_->worldDispatcher_) {
        openCl_->worldDispatcher_->clearVolumetricCoupling();
    }
#endif
}

bool SculptureFrameProducer::isWorldVolumetricCouplingBound() const noexcept {
#if EPI_SCULPT_HAS_OPENCL
    if (usingOpenCl() && openCl_->worldDispatcher_) {
        return openCl_->worldDispatcher_->isVolumetricCouplingBound();
    }
#endif
    return false;
}
} // namespace epi_sculpt
