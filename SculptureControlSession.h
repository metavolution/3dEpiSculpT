#pragma once

#include "SculptureRuntimeFlow.h"
#include "SculptureAudioModulation.h"
#include "SculptureParameterSources.h"
#include "SculptureCompositionObservation.h"
#include "SculptureModulationReconciliation.h"
#include "SculptureSequenceLoadLimits.h"

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace epi_sculpt {

class SculptureMorphChoreographer;
class SculpturePresetLibrary;
struct SculpturePresetAsset;

enum class SculpturePresetTakeover {
    // Retargeting alone keeps playback policy. Catalog replacement separately
    // admits retained cue revisions before deciding whether to keep the tour.
    Retarget,
    // An explicit selection leaves the tour and the restored world override.
    ReplaceSource,
    // Restore the selected base and clear manual/rotor history after admission.
    // Retain the captured-world override, matching the existing reset policy.
    ResetBase,
};

// Synchronous handoff to application-owned route/presentation effects. The
// asset is borrowed until the next library reload; never queue this value.
struct SculpturePresetTransition {
    std::size_t index;
    const SculpturePresetAsset& asset;
    // Selection/reload has installed routing in control. An identity-only bind
    // leaves its existing application routing policy to the synchronous caller.
    bool modulationAdmitted = false;
    // Successful catalog replacement retired retained cues whose source
    // revisions could not bind to the published catalog. Never set on refusal.
    bool playlistRetired = false;
};

// One evaluation's owned values. latentW is derived from the caller's current
// base, never retained as a second canonical field or application clock.
struct SculptureRotorComposition {
    float alpha = 0.0f;
    float beta = 0.0f;
    float theta = 0.0f;
    float latentW = 0.0f;
};

enum class SculptureManualParameter {
    LatentW, RotorAlpha, RotorBeta, RotorTheta, TopologyBlend, Hue, SeedAlpha,
};

// Read-only by value outside the control session; edits go through admission.
struct SculptureManualOverrides {
    float latentW = 0.0f;
    std::optional<bool> enable4D;
    float rotorAlpha = 0.0f;
    float rotorBeta = 0.0f;
    float rotorTheta = 0.0f;
    float topologyBlend = 0.0f;
    float hue = 0.0f;
    float seedAlpha = 0.0f;
};

// Device-free semantic state: identity, clocks, parameter source controllers,
// manual overrides and rotor/audio/modulation history. Native resources and
// producer accounting stay outside.
class SculptureControlSession {
public:
    // A cold load owns this handoff until the source stage publishes it. Both
    // text snapshots are allocated before source takeover; none is live truth.
    class PreparedModulationGraph {
    public:
        PreparedModulationGraph(PreparedModulationGraph&&) noexcept = default;
        PreparedModulationGraph& operator=(PreparedModulationGraph&&) noexcept = default;
        [[nodiscard]] const std::vector<std::string>& installedRoutingMap() const noexcept {
            return installed_;
        }
        [[nodiscard]] const std::vector<std::string>& requestedRoutingMap() const noexcept {
            return requested_;
        }
    private:
        friend class SculptureControlSession;
        PreparedModulationGraph(SculptureModulationEngine::PreparedRoutes routes,
            std::vector<std::string> installed, std::vector<std::string> requested) noexcept
            : routes_(std::move(routes)), installed_(std::move(installed)),
              requested_(std::move(requested)) {}
        SculptureModulationEngine::PreparedRoutes routes_;
        std::vector<std::string> installed_;
        std::vector<std::string> requested_;
    };

    [[nodiscard]] std::size_t presetIndex() const noexcept { return presetIndex_; }
    [[nodiscard]] SculptureRuntimeFlow& runtime() noexcept { return runtime_; }
    [[nodiscard]] const SculptureRuntimeFlow& runtime() const noexcept { return runtime_; }
    [[nodiscard]] SculptureAudioModulation& audioModulation() noexcept { return audioModulation_; }
    [[nodiscard]] const SculptureAudioModulation& audioModulation() const noexcept { return audioModulation_; }
    [[nodiscard]] SculptureParameterSources& sources() noexcept { return sources_; }
    [[nodiscard]] const SculptureParameterSources& sources() const noexcept { return sources_; }
    [[nodiscard]] SculptureSourceAdvance advanceSources(float wallDeltaSeconds) noexcept;

    [[nodiscard]] SculptureManualOverrides manualOverrides() const noexcept { return manual_; }
    // Adjust retains the live control's clamps; Set accepts any finite offset.
    // Rejected proposals leave their paired semantic values unchanged.
    bool adjustManualOffset(SculptureManualParameter parameter, float delta) noexcept;
    bool setManualOffset(SculptureManualParameter parameter, float value) noexcept;
    bool steerManual(float deltaLatentW, float deltaTime) noexcept;
    // params is a call-scoped derived value; neither it nor native state is retained.
    bool nudgeManualRotors(SculptureProgramParams& params,
                           float deltaAlpha, float deltaBeta, float deltaTheta) noexcept;
    void setManual4D(std::optional<bool> enabled) noexcept { manual_.enable4D = enabled; }
    void toggleManual4D(bool effectiveEnabled) noexcept;
    // Continuous history has a separate reset; preset/load callers clear only
    // the override(s) required by their existing transition policy.
    void resetManualOverrides() noexcept { manual_ = {}; }

    // Called after governor policy and before audio evaluation. All modified
    // values and rotor history commit together; false leaves both unchanged.
    // This is local admission, not whole-frame validation or clock rollback.
    [[nodiscard]] bool applyManualAndRotorOverrides(SculptureProgramParams& params,
        float timeSeconds, float deltaSeconds, bool audioReactive) noexcept;
    void resetContinuousRotors() noexcept;

    // Evaluate only the post-base semantic stages. The host has already applied
    // governor policy; neither params nor the input audio observation is retained.
    // Order matters: rotors use prior resonance, routing uses this evaluation's
    // resonance, and the explicit temporal override follows routed modifiers.
    [[nodiscard]] std::optional<SculptureAcousticMistObservation> composePostBase(
        SculptureProgramParams& params, float timeSeconds, float deltaSeconds,
        const SculptureAudioObservation& audio, bool temporalActive,
        SculptureCompositionObservation& composition);

    // One owner for compiled routes/history and their installed/requested text.
    // Rejection preserves installed truth; requested text only deduplicates
    // attempts. No engine mutation port is exposed to the application.
    [[nodiscard]] modulation_routing::ReconcileOutcome reconcileModulationGraph(
        const std::vector<std::string>& routingMap, bool audioEnabled,
        ModulationRouteDiagnostic& diagnostic);
    // Unlike ordinary request deduplication, every load must prove admission
    // before takeover. Rejection/preparation exceptions leave the session intact.
    // Empty intent resolves visual/optional-audio defaults at this boundary.
    [[nodiscard]] static std::optional<PreparedModulationGraph> prepareModulationGraph(
        const std::vector<std::string>& routingMap, bool audioEnabled,
        ModulationRouteDiagnostic& diagnostic);
    // No parsing/allocation. Compare at publication, since playlist identity may
    // have changed routing after preparation; equal installed text keeps slew.
    [[nodiscard]] modulation_routing::ReconcileOutcome publishModulationGraph(
        PreparedModulationGraph prepared) noexcept;
    [[nodiscard]] const std::vector<std::string>& installedRoutingMap() const noexcept {
        return installedRoutingMap_;
    }
    [[nodiscard]] const std::vector<std::string>& lastRequestedRoutingMap() const noexcept {
        return lastRequestedRoutingMap_;
    }

    // Admit routing before authored automation or source takeover. Automation
    // uses its transactional decoder; missing text preserves program/history.
    // After admission, graph/target/identity publish without allocation. Native
    // effects follow only after the host receives the transition. Empty maps use
    // the caller's audio policy; omitted policy selects visual-only defaults.
    [[nodiscard]] std::optional<SculpturePresetTransition> selectPreset(
        const SculpturePresetLibrary& library,
        std::size_t index,
        float durationSeconds,
        SculpturePresetTakeover takeover = SculpturePresetTakeover::Retarget,
        bool audioEnabled = false) noexcept;

    // Candidate discovery is the caller's responsibility. Retain the selected
    // exact unique ID, admit its Retarget selection, then publish by no-throw
    // swap. Retain a tour only when each library-backed cue has an exact unique
    // ID and matching nonempty revision; otherwise clear it after publication,
    // retaining the selected-preset retarget. Parameter-only cues need no binding.
    // Refusal changes neither library nor session. On success candidate
    // owns the retired catalog and the transition borrows the published asset.
    [[nodiscard]] std::optional<SculpturePresetTransition> replacePresetLibrary(
        SculpturePresetLibrary& library,
        SculpturePresetLibrary& candidate,
        float durationSeconds, bool audioEnabled = false) noexcept;

    // Playlist identity and capture provenance bind without retargeting their
    // already established parameter moment, importing asset automation or changing
    // their clock policy.
    [[nodiscard]] std::optional<SculpturePresetTransition> bindPreset(
        const SculpturePresetLibrary& library, std::size_t index) noexcept;

    // At most two catalog scans, independent of offset magnitude. Zero is a
    // no-op; unavailable planned routes, invalid authored modulation and rejected
    // automation imports are skipped. Probes never mutate live state.
    [[nodiscard]] std::optional<std::size_t> cyclePresetIndex(
        const SculpturePresetLibrary& library, int offset) const noexcept;

    // Cold route admission across all cues in a sequence candidate. Uses the
    // installation parser without publishing routes or mutating live state.
    [[nodiscard]] static bool admitSequenceRoutes(
        const SculptureSequencer& sequence,
        ModulationRouteDiagnostic* diagnostic = nullptr) noexcept;

    // Stage timeline decoding into a replacement candidate, admit every cue's
    // authored modulation routes before takeover, and commit sequence playback
    // only on complete admission. Refusal preserves the live sequence, active
    // tour, and cinematic ownership.
    [[nodiscard]] bool loadSequence(
        const SculpturePresetLibrary& library,
        const std::filesystem::path& path,
        const SequenceLoadLimits& limits = {},
        ModulationRouteDiagnostic* diagnostic = nullptr) noexcept;

private:
    [[nodiscard]] float* manualOffset(SculptureManualParameter parameter) noexcept;
    [[nodiscard]] std::optional<SculptureRotorComposition> prepareContinuousRotors(
        float latentW, bool enabled, float timeSeconds, float deltaSeconds,
        bool audioReactive) const noexcept;
    SculptureRuntimeFlow runtime_{};
    SculptureAudioModulation audioModulation_{};
    SculptureParameterSources sources_{};
    SculptureModulationEngine modulation_{};
    std::vector<std::string> installedRoutingMap_;
    std::vector<std::string> lastRequestedRoutingMap_;
    SculptureManualOverrides manual_{};
    float continuousRotorAlpha_ = 0.0f;
    float continuousRotorBeta_ = 0.0f;
    float continuousRotorTheta_ = 0.0f;
    std::size_t presetIndex_ = 0U;
};

} // namespace epi_sculpt
