#pragma once

#include "SculptureCaptureExit.h"
#include "SculptureCameraPose.h"
#include "SculptureReliefContours.h"
#include "SculptureTemporalReset.h"
#include "SculptureCausalExecutionGraph.h"
#include "SculptureCausalExecutionHistory.h"
#include "SculptureCaptureAdmission.h"
#include "SculptureSequenceDestination.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>
#include <array>
#include <string>
#include <string_view>
#include <optional>
#include <stdexcept>

#include "SculptureGpuAbi.h"
#include "SculptureFrameProducer.h"
#include "SculptureFramePlan.h"
#include "SculptureStereoRig.h"
#include "SculptureCapturedWorldRecipe.h"
#include "SculptureCanonicalFeedbackSource.h"
#include "epi/backend/seed_image_input.hpp"
#include "SculptureCanonicalPublicationInput.h"
#include "SculptureFrameContext.h"
#include "SculptureProducerCadence.h"
#include "SculptureProductionControlPlane.h"
#include "SculptureProductionSchedule.h"
#include "SculptureProductionRouteSession.h"
#include "SculptureProductionReceipt.h"
#include "SculpturePresets.h"
#include "SculpturePresetLibrary.h"
#include "SculpturePerformanceDeck.h"
#include "SculptureStagedComputeGovernor.h"
#include "SculpturePresentationScalePolicy.h"
#include "SculptureWorldExternalTransport.h"
#include "SculptureMorphChoreographer.h"
#include "SculptureMorphSpace.h"
#include "SculptureRenderBridge.h"
#include "SculptureWorldRenderBridge.h"
#include "SculptureTelemetrySession.h"
#include "SculptureWorldQualityPolicy.h"
#include "SculptureWorldProfileDriver.h"
#include "SculptureWorldFraming.h"
#include "SculptureProductionRoute.h"
#include "SculpturePresentationReceipt.h"
#include "SculptureCommandCenter.h"
#include "SculptureLiveControls.hpp"
#include "SculptureStateRecorder.h"
#include "SculptureWorldStateOverride.h"

#include "SculpturePlaybackRate.h"
#include "SculptureControlSession.h"
#include "SculptureAudio.h"
#include "SculptureModulator.h"
#include "SculpturePresentationDirector.h"
#include "SculpturePresentationHost.h"
#include "SculptureDevicePresentationBindings.h"
#include "SculptureResourcePaths.h"
#include "ImplicitObservationCadence.h"
#include "SculptureRemoteServer.h"
#include "SculptureMidiServer.h"
#include "SculpturePipeline.h"
#include "SculptureTelemetry.h"
#include "EpiFeedbackFieldProducer.h"
#include "EpiFeedbackFieldExternalTransport.h"
#include "CanonicalFeedbackCheckpointStore.h"

#include "mgfx/Engine.h"
#include "mgfx/vk/Window.h"

#include "SculptureEventBus.h"
#include "EpimorphicReliefGenerator.h"

namespace epi_sculpt {

struct AppState {
    // Observation control stays above the renderer and resolves before freeze.
    SculptureReliefContourControl reliefContours;
    bool enableVR = false;
    bool audioEnabled = kSculptureAudioEnabledByDefault;
    bool temporalActive = false;
    SculptureCaptureAdmission capture;
    SculptureSequenceDestination sequenceDestination;
    bool requestTemporalReset = false;
    SculptureTemporalResetControl temporalReset;
    bool stateJustLoaded = false;
    // Already-admitted graph for the latest successful LoadState, consumed once
    // after playlist identity reconciliation. Preparation freezes resolved
    // defaults; pre-takeover rejection preserves this value and its pending flag.
    // An admitted sequence import or explicit preset takeover retires this
    // handoff, preserving its reset through requestTemporalReset. Preset takeover
    // first settles an inactive cinematic exit without replacing the new target.
    std::optional<SculptureControlSession::PreparedModulationGraph> loadedModulationGraph;
};

// -----------------------------------------------------------------------------
// Educational Annotation: The Application Facade Pattern
// -----------------------------------------------------------------------------
// By encapsulating the entire execution-flow into a unified application class,
// we isolate the core systems (Engine, Audio, Sequencer, Command Center) from
// the entry-point. Alternate hosts can reuse these boundaries without copying
// the frame loop. Native Context/Swapchain/FrameLoop/Engine ownership is grouped
// in SculpturePresentationHost. Same-device surface recovery is the first-line
// repair path; a stronger application-owned transaction can rebuild only that
// native device island while durable window, semantic and OpenCL producer state
// survive above it.
class SculptureApplication {
    SculptureCaptureExit captureExit_;
public:
    // Compatibility name retained for existing pipeline extensions. The data
    // contract is independent of this facade and contains no lifecycle owner.
    using FrameContext = SculptureFrameContext;

    explicit SculptureApplication(int argc = 0, char** argv = nullptr);
    ~SculptureApplication();

    // Enters the primary execution-flow and blocking render loop.
    [[nodiscard]] bool run();

    // Live parameter and subsystem inspection
    [[nodiscard]] const SculptureProgramParams& activeParams() const noexcept { return activeParams_; }
    [[nodiscard]] const SculpturePerformanceDeck& performanceDeck() const noexcept { return control_.sources().deck(); }
    [[nodiscard]] SculpturePerformanceDeck& performanceDeck() noexcept { return control_.sources().deck(); }
    [[nodiscard]] bool isPerformanceDeckActive() const noexcept { return control_.sources().deckActive(); }
    void setPerformanceDeckActive(bool active) noexcept {
        if (active) clearCapturedSources();
        control_.sources().setDeckActive(active);
    }

    [[nodiscard]] const SculptureMorphSpace& morphSpace() const noexcept { return control_.sources().morphSpace(); }

    // Derived inspection surface only. Consumers may render/export this graph,
    // but cannot use it to mutate scheduling or simulation authority.
    [[nodiscard]] const std::optional<SculptureCausalExecutionSnapshot>&
    latestCausalExecutionGraph() const noexcept {
        return latestCausalExecutionGraph_;
    }
    [[nodiscard]] SculptureCausalExecutionUpdate readCausalExecutionGraphAfter(
        std::uint64_t revision) const noexcept {
        return causalExecutionHistory_.readAfter(revision);
    }
    [[nodiscard]] SculptureMorphSpace& morphSpace() noexcept { return control_.sources().morphSpace(); }
    [[nodiscard]] bool isMorphSpaceActive() const noexcept { return control_.sources().morphSpace().isActive(); }
    void setMorphSpaceActive(bool active) noexcept {
        if (active) clearCapturedSources();
        control_.sources().morphSpace().setActive(active);
    }
    void populateMorphSpaceFromPresetLibrary() noexcept;
    bool bindMorphSpaceToCategory(std::string_view category, bool autoLoad = true) noexcept;
    bool syncMorphSpaceToActiveCategory() noexcept;
    std::size_t synthesizeCategoryMorphBanks() noexcept;

    [[nodiscard]] const SculptureStagedComputeGovernor& stagedGovernor() const noexcept { return stagedGovernor_; }
    [[nodiscard]] SculptureStagedComputeGovernor& stagedGovernor() noexcept { return stagedGovernor_; }

    [[nodiscard]] std::optional<ReliefManifoldProjection> activeReliefProjectionOverride() const noexcept {
        return activeReliefProjectionOverride_;
    }
    void setActiveReliefProjectionOverride(std::optional<ReliefManifoldProjection> proj) noexcept {
        activeReliefProjectionOverride_ = proj;
    }
    [[nodiscard]] ReliefManifoldProjection resolveActiveReliefProjection() const noexcept;
    void cycleReliefProjection(int direction = 1) noexcept;

    friend void formatSculptureApplicationHud(char* buffer, std::size_t size,
                                             const SculptureFramePlan& framePlan,
                                             const SculptureApplication& app);

private:
    // ========================================================================
    // Decoupled Pipeline Execution Stages
    // ========================================================================
    void executeFrameLifecycle(FrameContext& ctx);
    void executeStageTimeAndInput(FrameContext& ctx);
    void executeStageAdvanceSimulation(FrameContext& ctx);
    void executeStageParameterComposition(FrameContext& ctx);
    void executeStageFreezeFramePlan(FrameContext& ctx);
    void executeStageProduction(FrameContext& ctx);
    void executeStageAuxiliaryFeedback(FrameContext& ctx);
    void executeStagePresentation(FrameContext& ctx);
    void handleEvent(const SculptureEvent& event);
    [[nodiscard]] std::optional<SculptureAcousticMistObservation> composeParameters(
        const SculptureProgramParams& base, float time, float deltaSeconds,
        const SculptureAudioObservation& audio, SculptureCompositionObservation& composition);
    [[nodiscard]] SculptureProductionCapabilities productionCapabilities() const noexcept;
    [[nodiscard]] SculptureProductionDecision resolveActiveProductionDecision() const noexcept;
    void applyModulationGraph(const std::vector<std::string>& routingMap);
    void handleFileAction(const EventFileAction& action);
    [[nodiscard]] SculptureWorldMode makeFrameCaptureWorldMode(const SculptureFramePlan& framePlan) const;
    void flushPendingStateSaves(const SculptureFramePlan& framePlan) noexcept;
    bool selectPreset(std::size_t index, float durationSeconds,
                      SculpturePresetTakeover takeover = SculpturePresetTakeover::Retarget);
    bool reloadPresetLibrary();
    // Full selection effects, after semantic admission/catalog publication.
    // Identity-only bindings use applyPresetTransition without loading a deck.
    void applyPresetSelection(const SculpturePresetTransition& transition);
    void applyPresetTransition(const SculpturePresetTransition& transition);
    // Applies only discrete asset identity: route, cadence, controls, and
    // temporal invalidation. Control owns parameter interpolation.
    void applyPresetIdentity(std::size_t index);
    void beginCompactHostStagedTransition() noexcept;
    void recordCompactSubmission(
        std::uint32_t generation,
        std::uint64_t leaseEpoch) noexcept;
    [[nodiscard]] bool deliveryMatchesCompactLease(
        const SculptureSnapshotDelivery& delivery,
        std::uint64_t leaseEpoch) const noexcept;
    void appendLibraryPlaylistCapture(SculptureWorldMode& mode) const;
    void appendMorphSpaceCapture(SculptureWorldMode& mode) const;
    // Populates a private decoded-moment candidate; false requests moment-only fallback.
    [[nodiscard]] bool prepareLibraryPlaylistCapture(
        const SculpturePlaylistCaptureState& state, SculptureMorphChoreographer& restored) const;
    [[nodiscard]] const SculpturePresetAsset& presetAt(std::size_t index) const noexcept;
    [[nodiscard]] const SculpturePresetAsset& selectedPreset() const noexcept;
    [[nodiscard]] SculptureWorldState resolveSimulationState(
        const SculptureProductionDecision& production) const noexcept;
    void handleFrameCapture(FrameContext& ctx, const mgfx::Camera& captureCamera);
    // Called only after native frame callbacks have returned/unwound. Idempotent
    // and nonthrowing: preserves accepted work, closes only pending attempts.
    void finalizeFrameExecution(
        FrameContext& ctx, SculptureProductionDeferralReason pendingReason) noexcept;
    static void applyPresentationReceiptToCapture(
        SculptureWorldMode& worldMode,
        const SculpturePresentationReceipt& receipt) noexcept;
    void commitPresentationReceipt(const SculpturePresentationReceipt& receipt) noexcept;
    void updateSonification(const gpu_abi::SculptureAnalyticsGpu* analytics,
                            float morph,
                            const SculptureProgramParams& params);
    [[nodiscard]] mgfx::Camera applyStartupCamera(mgfx::Camera camera) const noexcept;
    void renderFrame(FrameContext& ctx);
    void formatHud(char* buffer, std::size_t size, const SculptureFramePlan& framePlan);
    // Returns dedicated-world draw admission; other route receipt owners stay local.
    bool renderScenePass(
        mgfx::Drawable& draw,
        const mgfx::Camera& camera,
        const SculptureProgramParams& params,
        const SculptureWorldState& worldState,
        const SculptureSnapshotDelivery& delivery,
        const WorldSnapshotDelivery& worldDelivery,
        const EpiFeedbackFieldDelivery* canonicalField,
        bool dedicatedWorld,
        bool canonicalReliefRequested,
        bool canonicalFeedbackUnavailable,
        float deltaSeconds,
        const char* hud);
    void configureWorldCadence(const WorldRecipe& recipe, std::optional<float> effectiveHz = {}) noexcept;
    void clearCapturedSources() noexcept;
    [[nodiscard]] const WorldRecipe& activeWorldRecipe() const noexcept;
    void observeWorldProfile(float deltaSeconds);
    void observePresentationPerformance() noexcept;

    // Replaceable native presentation island. Resource paths are durable values
    // rather than process-cwd assumptions, so this owner can later rebuild the
    // island without reconstructing application/mathematical state.
    SculptureResourcePaths resourcePaths_{};
    std::unique_ptr<SculpturePresentationHost> presentationHost_;

    // Modular decoupled architecture components
    std::unique_ptr<SculptureEventBus> eventBus_;
    std::unique_ptr<SculptureCommandCenter> commandCenter_;
    // Pointer-only mode shares the focused demos' camera grammar without
    // competing with the full application's semantic keyboard event router.
    std::unique_ptr<SculptureLiveControls> liveCameraControls_;
    std::unique_ptr<SculptureRemoteServer> remoteServer_;
    std::unique_ptr<SculptureMidiServer> midiServer_;
    
    AppState appState_;
    SculptureControlSession control_;
    SculpturePresetLibrary presetLibrary_;
    std::optional<SculptureCapturedWorldRecipe> capturedWorldRecipe_;
    std::shared_ptr<const SculptureCanonicalFeedbackSource> capturedCanonicalSource_;
    // Deck browsing is not the selected mathematical asset identity.
    std::size_t deckLibraryStep_ = 0U;
    // Per-session tour position; reusable hosts must not share a static cursor.
    std::size_t tourPlaylistStep_ = 0;
    // Category browsing has its own application lifetime, independent of the
    // selected mathematical asset, playlist position and simulation clocks.
    std::size_t categoryTourCycleIndex_ = 0U;
    std::optional<std::size_t> appliedPlaylistTargetIndex_;
    SculptureMultiTrackChoreographer choreographer_;
    SculptureStagedComputeGovernor stagedGovernor_;
    // Presentation-pixel policy is deliberately separate from stagedGovernor_,
    // which controls auxiliary mathematical compute resolution/cadence.
    SculpturePresentationScalePolicy presentationScalePolicy_{};
    SculptureTelemetrySession telemetry_;
    SculptureWorldQualityPolicy worldQualityPolicy_;
    WorldPresentationMetrics worldPresentationMetrics_{};
    SculptureWorldProfileDriver worldProfileDriver_;
    // Only an explicit --profile-world session sets this. Normal live
    // performance remains open-ended after ordinary capture or telemetry.
    bool worldProfileExitRequested_ = false;
    bool profileSessionIsInteractive_ = false;

    // Constructor failure and normal shutdown explicitly drain/detach imports
    // before reverse member destruction can release their native exporters.
    std::unique_ptr<SculptureFrameProducer> sculptureProducer_;
    bool externalInteropRequested_ = true;
    EpiFeedbackFieldProducer::Config canonicalFeedbackProducerConfig_{};
    std::unique_ptr<EpiFeedbackFieldProducer> canonicalFeedbackProducer_;
    // One aggregate encoded-payload policy across active/frozen/backend inputs.
    // Input reservations keep accounting alive through asynchronous retirement.
    epi::SeedImageRetentionBudget canonicalSeedImageRetentionBudget_{};
    CanonicalFeedbackCheckpointStore canonicalFeedbackCheckpoints_{};
    // Replaceable device presentation adapters + Vulkan/OpenCL transports.
    // Declared after durable producers so it is destroyed first; the host was
    // declared earlier and therefore outlives both during normal teardown.
    std::unique_ptr<SculptureDevicePresentationBindings> presentationBindings_;
    

    EpiFeedbackFieldDeliveryRing canonicalFeedbackFields_{};
    SculptureCanonicalPublicationInputs canonicalPublicationInputs_{};
    SculptureAudio sculptureAudio_;
    SculpturePipeline pipeline_;

    // Procedural parameter modulators
    SculptureLFO latentLFO_;
    SculptureLFO hueLFO_;
    SculptureMutator mutator_{42};


    // Borrowed only during one synchronous production/settlement call. Native
    // ownership stays with the existing device-island transport.
    struct CanonicalExternalSubmission {
        EpiFeedbackFieldExternalTransport& transport;
        epi::FeedbackExternalSlotTicket ticket;
    };
    void deferCanonicalProduction(
        const SculptureProducerCadenceProposal<SculptureProgramParams>& proposal,
        std::uint64_t scheduleSequence, SculptureProductionDeferralReason reason) noexcept;
    void executeCanonicalFeedbackProduction(
        const SculptureFramePlan& framePlan,
        const SculptureProductionSchedule& schedule,
        const SculptureProducerCadenceProposal<SculptureProgramParams>& proposal,
        SculptureCanonicalPublicationInputs::Candidate input, std::uint32_t ringIndex,
        std::optional<CanonicalExternalSubmission> external = std::nullopt);
    void dispatchCanonicalFeedback(
        const SculptureFramePlan& framePlan,
        const SculptureProductionSchedule& schedule,
        const SculptureProducerCadenceProposal<SculptureProgramParams>& proposal);
    void disableCanonicalExternalDelivery(const char* reason) noexcept;
    void refreshExternalInteropEligibility() noexcept;
    [[nodiscard]] bool bindCanonicalProducerToTransport(
        EpiFeedbackFieldExternalTransport& transport);
    [[nodiscard]] bool establishCanonicalExternalTransport();
    [[nodiscard]] bool rebindCanonicalExternalTransport();
    [[nodiscard]] bool establishWorldExternalTransport();
    [[nodiscard]] bool reconstructPresentationDevice(const char* reason) noexcept;
    void retirePresentationForShutdown() noexcept;
    void maybeRetainCanonicalFeedbackCheckpoint(
        const EpiFeedbackFieldDelivery& delivery) noexcept;
    void noteCanonicalFeedbackProducerRejection() noexcept;
    [[nodiscard]] bool recoverCanonicalFeedbackProducer(
        const SculptureFramePlan& framePlan,
        bool allowCheckpointRestore) noexcept;
    void tryRestoreCanonicalExternalDelivery();
    [[nodiscard]] bool clearCanonicalExternalPresentationTargets() noexcept;
    [[nodiscard]] bool dispatchWorldProduction(
        const SculptureFramePlan& framePlan,
        const SculptureProductionSchedule& schedule);
    void observeCompactProductionPublication(
        const SculptureSnapshotDelivery& delivery) noexcept;
    void observeWorldProductionPublication(
        const WorldSnapshotDelivery& delivery) noexcept;
    void observeCanonicalProductionPublication(
        const EpiFeedbackFieldDelivery& delivery) noexcept;
    void observeImplicitProductionPublication(
        const ImplicitFrameIdentity& publishedFrame) noexcept;
    void prepareImplicitProduction(
        FrameContext& ctx,
        const mgfx::Camera& camera);
    static void observeImplicitProductionAttempt(
        void* context,
        std::uint64_t scheduleSequence,
        const ImplicitProductionAttemptReport& report) noexcept;
    void settleImplicitProductionAttempt(
        std::uint64_t scheduleSequence,
        const ImplicitProductionAttemptReport& report) noexcept;
    void cancelPendingImplicitProductions(
        SculptureProductionDeferralReason reason) noexcept;
    [[nodiscard]] std::uint64_t publishedProductionReceiptSequence(
        SculptureProductionRoute route,
        std::uint64_t generation,
        std::uint64_t resetEpoch = 0U) const noexcept;
    void updateCausalExecutionGraph(
        const SculptureFramePlan& framePlan,
        const SculptureProductionScheduleSet& productionSchedules,
        const SculpturePresentationReceipt& presentation) noexcept;

    std::uint64_t sculptureGeneration_{0};
    std::uint32_t worldGeneration_{0};
    std::uint64_t canonicalFeedbackGeneration_{0};
    // Resource-attempt sequence used only to rotate external transport slots.
    // It is deliberately independent from canonicalFeedbackGeneration_: a
    // rejected/aborted native handoff must never create a hole in the
    // mathematical generation history.
    std::uint64_t canonicalFeedbackTransportAttemptSequence_{0};
    // Last *accepted* implicit mathematical generation. Attempt identity lives
    // in the schedule sequence; rejected/backpressured attempts retry the same
    // next generation just like compact/world/canonical producers do.
    std::uint32_t implicitGeneration_{0U};
    SculptureProductionControlPlane productionControl_{};
    bool productionScheduleExhaustionReported_{false};
    std::uint64_t causalExecutionGraphRevision_{0};
    std::optional<SculptureCausalExecutionSnapshot> latestCausalExecutionGraph_{};
    // Readbacks can outlive a frame; callbacks share this bounded in-process reference.
    std::shared_ptr<SculptureFieldCaptureTracker> fieldCaptureTracker_ =
        std::make_shared<SculptureFieldCaptureTracker>();
    SculptureCausalExecutionHistory<> causalExecutionHistory_{};
    std::uint64_t canonicalFeedbackResetEpoch_{1};
    // Last committed capture-resource selection. Never rewound on source clear;
    // frozen frames may still own an older source with the same image pathname.
    std::uint64_t canonicalSeedImageSelectionRevision_ = 0U;
    std::uint64_t canonicalFeedbackResetRequestSequence_ = 0U;
    // Producer reconstruction is deliberately slower than ordinary transport
    // fallback: one failed publication does not imply a dead OpenCL producer.
    // Only repeated pre-acceptance rejection requests replacement, and failed
    // replacement attempts back off for a bounded number of presentation frames.
    std::uint32_t canonicalFeedbackProducerRejectStreak_{0U};
    bool canonicalFeedbackProducerRecoveryPending_{false};
    std::uint32_t canonicalFeedbackProducerRecoveryCooldownFrames_{0U};
    static constexpr std::uint32_t kCanonicalFeedbackProducerRecoveryRejectThreshold_ = 3U;
    static constexpr std::uint32_t kCanonicalFeedbackProducerRecoveryCooldownFrames_ = 60U;
    SculptureProductionRouteSession productionRouteSession_{};
    // External presentation is admitted only after both APIs are explicitly
    // qualified against the current Vulkan device generation. Host staging is
    // the safe startup/recovery default.
    bool canonicalExternalDeliveryEnabled_{false};
    bool canonicalExternalTransportSupported_{false};
    EpiFeedbackFieldExternalTransport::Config canonicalFeedbackTransportConfig_{};
    std::uint64_t canonicalExternalRecoveryNextAttemptGeneration_{0};
    static constexpr std::uint64_t kCanonicalExternalRecoveryRetryGenerations_ = 120U;
    std::uint32_t lastPresentedCompactGeneration_{0};
    std::uint32_t lastPresentedWorldGeneration_{0};
    std::uint32_t lastPresentedImplicitGeneration_{0};
    // Both staged and external deliveries must meet the current compact floor.
    // A rejected external delivery still needs in-recording ticket retirement.
    std::uint32_t compactHostStagedGenerationFloor_{0};
    struct CompactSubmissionStamp {
        std::uint32_t generation = 0U;
        std::uint64_t leaseEpoch = 0U;
        bool valid = false;
    };
    // Compact production has a small bounded slot ring. Keeping a slightly
    // larger fixed ledger makes delayed results fail closed without coupling
    // application policy to producer/OpenCL storage internals.
    static constexpr std::size_t kCompactSubmissionStampCapacity = 16U;
    std::array<CompactSubmissionStamp, kCompactSubmissionStampCapacity>
        compactSubmissionStamps_{};
    std::uint64_t compactPresentationLeaseEpoch_{1U};
    SculpturePresentationReceipt lastPresentationReceipt_{};
    SculpturePresentationOwnership presentationOwnership_ =
        SculpturePresentationOwnership::CompactInstances;
    std::string producerBackend_;
    // Command-owned live intent; presentation consumes the FramePlan copy.
    SculptureStereoRig stereoRig_{};

    // Vulkan may present every display tick, while compact OpenCL generation
    // advances at a bounded simulation cadence or immediately after a live
    // mathematical edit. This avoids invisible duplicate dispatches.
    SculptureProducerCadence<SculptureProgramParams> compactProducerCadence_{
        {.updateHz = 30.0f, .dispatchOnParameterChange = true}, &equivalentProducerParams};
    SculptureProducerCadence<SculptureProgramParams> worldProducerCadence_{
        {.updateHz = 30.0f, .dispatchOnParameterChange = true}, &equivalentProducerParams};
    // Canonical feedback is one camera-independent simulation stream. Vulkan
    // may present its newest delivery at display rate, but must not dispatch
    // a duplicate feedback step for every eye or present tick.
    SculptureProducerCadence<SculptureProgramParams> canonicalFeedbackCadence_{
        {.updateHz = 30.0f, .dispatchOnParameterChange = true}, &equivalentProducerParams};
    // Implicit observation is camera/extent-sensitive and therefore evaluated
    // at frame-recording time, but cadence authority still belongs here in the
    // application control plane rather than inside presentation code.
    ImplicitObservationCadence implicitObservationCadence_{};
    struct PendingImplicitProduction final {
        std::uint64_t scheduleSequence = 0U;
        ImplicitFrameRequest request{};
        SculptureTemporalResetIntent temporalReset{};

        [[nodiscard]] bool matches(std::uint64_t sequence) const noexcept {
            return scheduleSequence != 0U && scheduleSequence == sequence;
        }
        void clear() noexcept {
            scheduleSequence = 0U;
            request = {};
            temporalReset = {};
        }
    };
    std::array<PendingImplicitProduction, SculptureProductionScheduleSet::kCapacity>
        pendingImplicitProductions_{};

    // Extracted parameters mapping
    SculptureProgramParams activeParams_;
    std::optional<ReliefManifoldProjection> activeReliefProjectionOverride_{std::nullopt};

    // Save commands resolve at the next frame freeze, after parameter composition.
    // Store only destinations; frame plans contain call-scoped catalog borrows.
    static constexpr std::size_t kMaxPendingStateSaves = 8U;
    std::vector<std::string> pendingStateSavePaths_;

    // Centralized dynamic execution pipeline
    std::optional<double> runSeconds_{};
    std::optional<double> captureSeconds_{};
    std::optional<SculptureCameraPose> startupCameraPose_{};
    std::chrono::steady_clock::time_point previousFrameTime_{};
};

} // namespace epi_sculpt
