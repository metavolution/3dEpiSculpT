// Fixed seven-phase execution, failure containment and exactly-once frame observation.
// These translation units share one SculptureApplication owner; none is a second scheduler.

#include "SculptureApplication.h"
#include "SculptureFrameRecoveryPolicy.h"
#include "SculptureCapturePresetProvenance.h"
#include "epi/core/execution_resilience.hpp"

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <span>
#include <stdexcept>
#include <thread>

namespace epi_sculpt {
namespace {

[[nodiscard]] SculptureFrameCommitFacts collectFrameCommitFacts(
    const SculptureFrameContext& frame,
    const SculptureProductionControlPlane& productionControl) noexcept {
    SculptureFrameCommitFacts facts{};
    facts.failedStage = frame.lifecycle.failedStage;
    facts.framePlanFrozen = frame.framePlan.has_value();
    facts.presentationSettled = frame.presentationReceipt.settled;
    facts.presentationSubmitted = frame.presentationReceipt.submitted();

    for (const SculptureProductionSchedule& schedule : frame.productionSchedules.schedules()) {
        if (schedule.scheduleSequence == 0U) continue;
        ++facts.scheduledProductions;
        const SculptureProductionReceipt* receipt =
            productionControl.find(schedule.scheduleSequence);
        if (receipt == nullptr) continue;
        if (receipt->accepted()) ++facts.acceptedProductions;
        if (receipt->published()) ++facts.publishedProductions;
    }
    return facts;
}

void logPreservedGenericFailureState(
    const SculptureFrameCommitFacts& facts,
    const SculptureGenericRecoveryDecision& recovery,
    epi::core::FailureEscalation escalation) {
    if (escalation != epi::core::FailureEscalation::FirstRecovery
        && escalation != epi::core::FailureEscalation::SecondRecovery) {
        return;
    }
    std::cerr << "[3dEpiSculpTMorph] Generic recovery escalation retained "
              << "semantic/scientific commits (domain " << name(recovery.domain)
              << ", accepted " << facts.acceptedProductions
              << ", published " << facts.publishedProductions
              << ", presentation "
              << (facts.presentationSubmitted ? "submitted"
                  : (facts.presentationSettled ? "settled-not-submitted" : "unsettled"))
              << "). Retry pacing changed; no mathematical epoch, cadence or "
                 "temporal semantic reset was inferred.\n";
}

[[nodiscard]] constexpr const char* frameOutcomeLabel(mgfx::vk::FrameOutcome outcome) noexcept {
    using mgfx::vk::FrameOutcome;
    switch (outcome) {
    case FrameOutcome::SkippedZeroExtent: return "skipped-zero-extent";
    case FrameOutcome::SkippedSwapchainRecreated: return "skipped-swapchain-recreated";
    case FrameOutcome::SkippedRenderTargetRebuildFailed: return "skipped-target-rebuild-failed";
    case FrameOutcome::SkippedImageUnavailable: return "skipped-image-unavailable";
    case FrameOutcome::SubmittedPresented: return "submitted-presented";
    case FrameOutcome::SubmittedSwapchainRecreated: return "submitted-swapchain-recreated";
    case FrameOutcome::SubmittedSwapchainRecreationDeferred: return "submitted-recreation-deferred";
    case FrameOutcome::SurfaceReplacementRequired: return "surface-replacement-required";
    case FrameOutcome::DeviceReconstructionRequired: return "device-reconstruction-required";
    case FrameOutcome::SkippedPresentationUnavailable: return "skipped-presentation-unavailable";
    case FrameOutcome::SubmittedPresentationSkipped: return "submitted-presentation-skipped";
    case FrameOutcome::SkippedRenderTargetRetirementPending: return "skipped-target-retirement-pending";
    }
    return "unknown";
}

} // namespace

void SculptureApplication::executeFrameLifecycle(FrameContext& ctx) {
    // Rebuild native imports only outside Engine/FrameLoop recording. Cold
    // settlement may wait for both APIs; production callbacks never do so.
    tryRestoreCanonicalExternalDelivery();
    using StageMethod = void (SculptureApplication::*)(FrameContext&);
    struct StageBinding final {
        SculptureFrameStage stage;
        StageMethod execute;
    };

    // The order is intentionally data-visible but compile-time fixed.  It is
    // the executable counterpart of the architecture document: changing it is
    // a semantic/lifecycle change, not a runtime customization.
    static constexpr std::array<StageBinding, 7> kStages{{
        {SculptureFrameStage::TimeAndInput,
         &SculptureApplication::executeStageTimeAndInput},
        {SculptureFrameStage::AdvanceSimulation,
         &SculptureApplication::executeStageAdvanceSimulation},
        {SculptureFrameStage::ParameterComposition,
         &SculptureApplication::executeStageParameterComposition},
        {SculptureFrameStage::FreezeFramePlan,
         &SculptureApplication::executeStageFreezeFramePlan},
        {SculptureFrameStage::Production,
         &SculptureApplication::executeStageProduction},
        {SculptureFrameStage::AuxiliaryFeedback,
         &SculptureApplication::executeStageAuxiliaryFeedback},
        {SculptureFrameStage::Presentation,
         &SculptureApplication::executeStagePresentation},
    }};

    for (const StageBinding& binding : kStages) {
        ctx.lifecycle.begin(binding.stage);
        try {
            (this->*binding.execute)(ctx);
            ctx.lifecycle.complete(binding.stage);
        } catch (...) {
            ctx.lifecycle.fail(binding.stage);
            throw;
        }
    }
}

bool SculptureApplication::run() {
    if (captureExit_.started())
        throw std::logic_error("SculptureApplication cannot run after capture finalization");
    previousFrameTime_ = std::chrono::steady_clock::now();
    // The budget begins after cold construction. Check only between complete
    // frame transactions, preserving the normal capture and retirement path.
    const auto deadline = runSeconds_
        ? std::optional{previousFrameTime_ + std::chrono::duration_cast<std::chrono::steady_clock::duration>(
              std::chrono::duration<double>(*runSeconds_))}
        : std::nullopt;
    const auto captureDeadline = captureSeconds_
        ? std::optional{previousFrameTime_ + std::chrono::duration_cast<std::chrono::steady_clock::duration>(
              std::chrono::duration<double>(*captureSeconds_))}
        : std::nullopt;
    bool timedCaptureRequested = false;

    // Retry pacing is shared mechanism. Typed owners decide actual recovery;
    // a generic exception counter only controls backoff/circuit-break and is
    // never evidence that semantic or mathematical state should reset.
    constexpr epi::core::ExecutionResiliencePolicy kFailurePolicy{
        .firstRecoveryFailure = 5U,
        .secondRecoveryFailure = 15U,
        .circuitBreakFailure = 60U,
        .baseBackoffMilliseconds = 16U,
        .maximumBackoffMilliseconds = 500U,
    };

    std::uint32_t consecutiveErrors = 0U;
    bool normalExit = true;
    std::uint64_t totalFrames = 0U;

    while ((!deadline || std::chrono::steady_clock::now() < *deadline)
           && !presentationHost_->context().shouldClose() && !worldProfileExitRequested_) {
        FrameContext frameCtx;
        frameCtx.frameCounter = ++totalFrames;
        if (!timedCaptureRequested && captureDeadline
            && std::chrono::steady_clock::now() >= *captureDeadline) {
            appState_.capture.onEvent(EventCapture{});
            timedCaptureRequested = true;
        }

        try {
            // This scope outlives every nested Engine/FrameLoop call. Native
            // submit callbacks settle first; only then may application facts
            // close. Even non-std exceptions take this nonthrowing exit path.
            struct FrameExit final {
                SculptureApplication& app;
                FrameContext& frame;
                ~FrameExit() noexcept {
                    app.finalizeFrameExecution(
                        frame, SculptureProductionDeferralReason::FrameAborted);
                }
            } frameExit{*this, frameCtx};

            executeFrameLifecycle(frameCtx);
            finalizeFrameExecution(
                frameCtx, SculptureProductionDeferralReason::FrameNotRecorded);

            // Native recovery is deliberately layered. First attempt the
            // smallest legal transaction (same-device surface replacement).
            // Escalation rebuilds only the device-owned presentation island;
            // application semantics and OpenCL mathematical producers remain.
            if (frameCtx.presentationReceipt.settled &&
                mgfx::vk::frameOutcomeRequiresHostRecovery(
                    frameCtx.presentationReceipt.outcome)) {
                std::string reconstructionReason = "logical-device recovery requested";
                if (mgfx::vk::frameOutcomeRequiresSurfaceReplacement(
                        frameCtx.presentationReceipt.outcome)) {
                    const SculpturePresentationRecoveryResult recovery =
                        presentationHost_->recoverSurface();
                    if (recovery.recovered()) {
                        std::cerr << "[3dEpiSculpTMorph] Native surface recovered "
                                  << "(device " << recovery.deviceGeneration
                                  << ", surface " << recovery.surfaceGeneration
                                  << "); continuing session.\n";
                        previousFrameTime_ = std::chrono::steady_clock::now();
                        consecutiveErrors = 0U;
                        continue;
                    }
                    reconstructionReason = recovery.diagnostic;
                    std::cerr << "[3dEpiSculpTMorph] Surface-only recovery escalated: "
                              << recovery.diagnostic << " (VkResult "
                              << static_cast<int>(recovery.nativeResult) << ").\n";
                }

                if (reconstructPresentationDevice(reconstructionReason.c_str())) {
                    previousFrameTime_ = std::chrono::steady_clock::now();
                    consecutiveErrors = 0U;
                    continue;
                }

                std::cerr << "[3dEpiSculpTMorph] Native presentation recovery exhausted; "
                             "ending this session without resetting mathematical state.\n";
                normalExit = false;
                break;
            }
            consecutiveErrors = 0U;
            continue;
        } catch (const std::exception& error) {
            ++consecutiveErrors;
            const auto decision = epi::core::decideExecutionFailure(
                consecutiveErrors, kFailurePolicy);
            const SculptureFrameStage failedStage = frameCtx.lifecycle.failedStage;
            std::cerr << "[3dEpiSculpTMorph] Error during frame execution (count "
                      << consecutiveErrors << ", stage "
                      << sculptureFrameStageName(failedStage) << "): "
                      << error.what() << "\n";

            // A typed native presentation fault can also escape through an
            // exception path (for example while allocating/rebuilding a native
            // adapter). Once stack unwinding closes the frame, rebuild the full
            // device island rather than attempting to reuse a FrameLoop whose
            // recording transaction has been marked failed. Application-only
            // recording exceptions remain terminal for this native generation.
            if (presentationHost_->frameLoop().presentationFault().active()) {
                if (reconstructPresentationDevice("typed native fault escaped frame execution")) {
                    previousFrameTime_ = std::chrono::steady_clock::now();
                    consecutiveErrors = 0U;
                    continue;
                }
                std::cerr << "[3dEpiSculpTMorph] Native presentation recovery failed after exception.\n";
                normalExit = false;
                break;
            }
            if (presentationHost_->frameLoop().terminalFailure()) {
                std::cerr << "[3dEpiSculpTMorph] Rendering session stopped after an untyped frame failure; "
                             "leaving the frame loop for explicit retirement.\n";
                normalExit = false;
                break;
            }

            const SculptureFrameCommitFacts commitFacts =
                collectFrameCommitFacts(frameCtx, productionControl_);
            const SculptureGenericRecoveryDecision recovery =
                decideSculptureGenericRecovery(commitFacts, decision.escalation);
            logPreservedGenericFailureState(
                commitFacts, recovery, decision.escalation);
            if (recovery.stopSession) {
                std::cerr << "[3dEpiSculpTMorph] Circuit breaker tripped ("
                          << consecutiveErrors
                          << " consecutive failures). Ending the session while preserving "
                             "accepted mathematical/publication state for explicit retirement.\n";
                normalExit = false;
            }
            if (!normalExit) break;
            std::this_thread::sleep_for(decision.backoff);
            previousFrameTime_ = std::chrono::steady_clock::now();
        } catch (...) {
            ++consecutiveErrors;
            const auto decision = epi::core::decideExecutionFailure(
                consecutiveErrors, kFailurePolicy);
            std::cerr << "[3dEpiSculpTMorph] Unknown frame failure (count "
                      << consecutiveErrors << ", stage "
                      << sculptureFrameStageName(frameCtx.lifecycle.failedStage)
                      << ").\n";
            if (presentationHost_->frameLoop().presentationFault().active()) {
                if (reconstructPresentationDevice("typed native fault escaped as unknown exception")) {
                    previousFrameTime_ = std::chrono::steady_clock::now();
                    consecutiveErrors = 0U;
                    continue;
                }
                normalExit = false;
                break;
            }
            if (presentationHost_->frameLoop().terminalFailure()) {
                normalExit = false;
                break;
            }
            const SculptureFrameCommitFacts commitFacts =
                collectFrameCommitFacts(frameCtx, productionControl_);
            const SculptureGenericRecoveryDecision recovery =
                decideSculptureGenericRecovery(commitFacts, decision.escalation);
            logPreservedGenericFailureState(
                commitFacts, recovery, decision.escalation);
            if (recovery.stopSession) {
                normalExit = false;
                break;
            }
            std::this_thread::sleep_for(decision.backoff);
            previousFrameTime_ = std::chrono::steady_clock::now();
        }
    }

    // Finish only after the loop has stopped, while readback buffers and captured
    // state still live. A failed reconstruction intentionally leaves no native
    // island to retire; its old resources were already abandoned by that
    // transaction, so never dereference a nonexistent FrameLoop here.
    if (presentationHost_->hasDeviceIsland()) {
        SculptureCaptureExit::requireRetired(captureExit_.finish(
            normalExit, presentationHost_->frameLoop(),
            [this] { return presentationHost_->engine().waitIdleChecked(); }));
    }
    if (runSeconds_) {
        const bool islandAvailable = presentationHost_->hasDeviceIsland();
        const auto& receipt = lastPresentationReceipt_;
        std::cerr << "[3dEpiSculpTMorph] finite-summary"
                  << " normal_exit=" << normalExit
                  << " attempted_app_frames=" << totalFrames
                  << " snapshot_available=" << latestCausalExecutionGraph_.has_value()
                  << " snapshot_frame_sequence=" << (latestCausalExecutionGraph_
                         ? latestCausalExecutionGraph_->frameSequence() : 0U)
                  << " receipt_frame_sequence=" << receipt.frameSequence
                  << " receipt_route=" << name(receipt.route)
                  << " settled=" << receipt.settled
                  << " submitted=" << receipt.submitted()
                  << " outcome=" << (receipt.settled ? frameOutcomeLabel(receipt.outcome) : "unsettled")
                  << " snapshot_device_generation=" << receipt.deviceGeneration
                  << " snapshot_surface_generation=" << receipt.surfaceGeneration
                  << " current_island_available=" << islandAvailable
                  << " current_device_generation=" << (islandAvailable ? presentationHost_->deviceGeneration() : 0U)
                  << " current_surface_generation=" << (islandAvailable ? presentationHost_->context().surfaceGeneration() : 0U)
                  << " current_last_submitted_serial=" << (islandAvailable ? presentationHost_->frameLoop().lastSubmittedSerial() : 0U)
                  << " current_completed_submission_serial=" << (islandAvailable ? presentationHost_->frameLoop().completedSubmissionSerial() : 0U)
                  << '\n';
        if (islandAvailable && presentationBindings_) {
            const auto volume = presentationBindings_->compactBridge().volumePublicationCounters();
            std::cerr << "[3dEpiSculpTMorph] finite-volume"
                      << " accepted_ingests=" << volume.acceptedIngests
                      << " accepted_replays=" << volume.acceptedReplays << '\n';
        }
        if (latestCausalExecutionGraph_) {
            const auto& graph = *latestCausalExecutionGraph_;
            for (const auto& node : graph.nodes()) {
                if (node.key.kind != SculptureCausalNodeKind::ProductionExecution) continue;
                bool selected = false;
                for (const auto& edge : graph.edges()) {
                    if (edge.kind == SculptureCausalEdgeKind::SelectedForPresentation
                        && edge.from == node.key) {
                        selected = true;
                        break;
                    }
                }
                const auto& production = node.production;
                std::cerr << "[3dEpiSculpTMorph] finite-production"
                          << " schedule_sequence=" << production.scheduleSequence
                          << " route=" << name(production.route)
                          << " generation_intent=" << production.generationIntent
                          << " committed_generation=" << production.committedGeneration
                          << " committed_reset_epoch=" << production.committedResetEpoch
                          << " publication_generation=" << production.publication.generation
                          << " publication_reset_epoch=" << production.publication.resetEpoch
                          << " transport=" << name(production.publication.transport)
                          << " accepted=" << production.accepted()
                          << " published=" << production.published()
                          << " selected=" << selected << '\n';
            }
        }
    }
    return normalExit;
}

void SculptureApplication::finalizeFrameExecution(
    FrameContext& ctx, SculptureProductionDeferralReason pendingReason) noexcept {
    if (ctx.observationFinalized) return;
    ctx.observationFinalized = true;

    // A deferred canonical proposal may never have reached recording. Defer
    // cadence telemetry once, and only when the native owner did not settle it.
    if (const auto* schedule = ctx.productionSchedules.find(
            SculptureProductionRoute::CanonicalFeedback)) {
        const auto* receipt = productionControl_.find(schedule->scheduleSequence);
        if (receipt && receipt->pending() && ctx.canonicalCadenceProposal) {
            canonicalFeedbackCadence_.defer(*ctx.canonicalCadenceProposal);
        }
    }

    // Normally the attempt observer has already cleared these entries. Close
    // only this frame's orphaned attempts; never cancel another frame's work.
    for (auto& pending : pendingImplicitProductions_) {
        if (pending.scheduleSequence == 0U) continue;
        for (const auto& schedule : ctx.productionSchedules.schedules()) {
            if (!pending.matches(schedule.scheduleSequence)) continue;
            implicitObservationCadence_.completeAttempt(pending.request, false);
            pending.clear();
            break;
        }
    }
    static_cast<void>(productionControl_.deferPending(
        ctx.productionSchedules, pendingReason));

    // Input/composition can fail before semantic commit. Do not fabricate a
    // FramePlan or graph node for intent that was never frozen.
    if (!ctx.framePlan) return;
    const auto& plan = *ctx.framePlan;
    auto& presentation = ctx.presentationReceipt;
    presentation.route = plan.production().route;
    presentation.frameSequence = plan.frameSequence();
    // An exceptional renderer exit has no FrameOutcome. Keep settled=false;
    // selection is not proof of submission, and certainly not presentation.
    commitPresentationReceipt(presentation);
    updateCausalExecutionGraph(plan, ctx.productionSchedules, presentation);
    if (ctx.captureObservation) {
        ctx.captureObservation->presentation = presentation;
        ctx.captureObservation->causal = latestCausalExecutionGraph_;
        ctx.captureObservation->identity.finalize();
    }
}

void SculptureApplication::commitPresentationReceipt(
    const SculpturePresentationReceipt& receipt) noexcept {
    lastPresentationReceipt_ = receipt;
    if (!receipt.presentationConfirmed()) return;

    if (receipt.compactGeneration != 0U) {
        lastPresentedCompactGeneration_ = receipt.compactGeneration;
    }
    if (receipt.worldGeneration != 0U) {
        lastPresentedWorldGeneration_ = receipt.worldGeneration;
    }
    if (receipt.implicitGeneration != 0U) {
        lastPresentedImplicitGeneration_ = receipt.implicitGeneration;
    }
}

void SculptureApplication::updateCausalExecutionGraph(
    const SculptureFramePlan& framePlan,
    const SculptureProductionScheduleSet& productionSchedules,
    const SculpturePresentationReceipt& presentation) noexcept {
    std::array<const SculptureProductionReceipt*, SculptureProductionScheduleSet::kCapacity>
        currentProductions{};
    std::size_t currentProductionCount = 0U;
    for (const SculptureProductionSchedule& schedule : productionSchedules.schedules()) {
        if (schedule.scheduleSequence == 0U) continue;
        if (const SculptureProductionReceipt* receipt =
                productionControl_.find(schedule.scheduleSequence)) {
            currentProductions[currentProductionCount++] = receipt;
        }
    }

    std::array<const SculptureProductionReceipt*,
               SculpturePresentationReceipt::kContributionCapacity>
        selectedProductions{};
    std::size_t selectedProductionCount = 0U;
    for (const SculpturePresentationContribution& contribution
         : presentation.selectedProductions()) {
        if (!contribution.selected() || contribution.productionReceiptSequence == 0U) {
            continue;
        }
        // Selected provenance must survive unrelated journal turnover. Pin only
        // finalized selection facts; this is neither a GPU lease nor proof of
        // submission/presentation, which remain explicit in the native receipt.
        if (const SculptureProductionReceipt* selected =
                productionControl_.retainSelectedPublication(
                    contribution.route, contribution.generation,
                    contribution.productionReceiptSequence, contribution.resetEpoch)) {
            selectedProductions[selectedProductionCount++] = selected;
        }
    }

    ++causalExecutionGraphRevision_;
    if (causalExecutionGraphRevision_ == 0U) ++causalExecutionGraphRevision_;

    const auto& preset = presetAt(framePlan.presetIndex());
    const auto presetSource = resolveCapturePresetProvenance(framePlan, preset.id, preset.revision);
    latestCausalExecutionGraph_ = SculptureCausalExecutionGraphProjector::project(
        causalExecutionGraphRevision_,
        framePlan,
        std::span<const SculptureProductionReceipt* const>(
            currentProductions.data(), currentProductionCount),
        presentation,
        std::span<const SculptureProductionReceipt* const>(
            selectedProductions.data(), selectedProductionCount),
        presetSource.id);
    causalExecutionHistory_.publish(*latestCausalExecutionGraph_);
}

} // namespace epi_sculpt
