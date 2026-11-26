#pragma once

#include "SculptureProgramSchema.h"
#include "SculptureMorphChoreographer.h"
#include "SculpturePresetLibrary.h"
#include "SculptureMacroAutomation.h"
#include "SculptureCompositionObservation.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <functional>
#include <numbers>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace epi_sculpt {

enum class DeckId : std::uint8_t {
    DeckA = 0,
    DeckB = 1
};

enum class CrossfadeCurve : std::uint8_t {
    Linear = 0,
    EqualPower = 1,     // Constant acoustic/visual energy: (cos(pi/2 * x), sin(pi/2 * x))
    Smoothstep = 2,     // Hermite cubic ease: 3x^2 - 2x^3
    SharpCut = 3        // Hard DJ battle switch at center threshold
};

struct DeckSlot {
    SculptureProgramParams params{};
    std::string presetId;
    std::string presetTitle = "Init Preset";
    float playbackRate = 1.0f;
    bool isMuted = false;
};

struct PerformanceCuePoint {
    SculptureProgramParams params{};
    std::string name;
    std::string presetId;
    bool active = false;
};

class SculpturePerformanceDeck {
public:
    static constexpr std::size_t kMaxCueBanks = 10;
    static constexpr std::size_t kMaxTapHistory = 8;

    SculpturePerformanceDeck() noexcept;

    // -------------------------------------------------------------------------
    // Deck Parameter & Scene Loading
    // -------------------------------------------------------------------------
    void loadDeck(DeckId deck, const SculptureProgramParams& params,
                  std::string_view presetId = "", std::string_view title = "");
    void loadDeck(DeckId deck, const SculpturePresetAsset& asset);

    [[nodiscard]] const DeckSlot& deck(DeckId deck) const noexcept {
        return deck == DeckId::DeckA ? deckA_ : deckB_;
    }
    [[nodiscard]] DeckSlot& deck(DeckId deck) noexcept {
        return deck == DeckId::DeckA ? deckA_ : deckB_;
    }

    void swapDecks() noexcept;

    // -------------------------------------------------------------------------
    // Master Crossfader Controls
    // -------------------------------------------------------------------------
    // Nonfinite positions/deltas or unrepresentable sums leave the owner alone.
    // Admitted positions retain the [0, 1] clamp; they do not cancel a fade.
    void setCrossfadePosition(float position) noexcept;
    [[nodiscard]] float crossfadePosition() const noexcept { return crossfadePos_; }
    void nudgeCrossfade(float delta) noexcept;

    void setCrossfadeCurve(CrossfadeCurve curve) noexcept { crossfadeCurve_ = curve; }
    [[nodiscard]] CrossfadeCurve crossfadeCurve() const noexcept { return crossfadeCurve_; }

    // Evaluates analytical deck blending weights [0.0, 1.0]
    [[nodiscard]] static std::pair<float, float> computeDeckGains(
        float pos, CrossfadeCurve curve) noexcept;

    // Blends Deck A and Deck B into canonical master parameters
    // Optional call-scoped output observes only the checked program macro stage.
    [[nodiscard]] SculptureProgramParams composeMasterParams(
        SculptureCompositionAdmission* macroAdmission = nullptr) const;

    // Auto-Crossfade: smooth musical transition over specified beat count or duration
    // Nonfinite duration holds the entire transition. An admitted seconds-based
    // request replaces both the running mode and any queued downbeat transition.
    void triggerAutoCrossfade(float durationSeconds, std::optional<DeckId> targetDeck = std::nullopt);
    void cancelAutoCrossfade() noexcept;
    [[nodiscard]] bool isAutoCrossfading() const noexcept { return autoCrossfading_; }
    [[nodiscard]] float autoCrossfadeProgress() const noexcept { return autoCrossfadeProgress_; }

    // -------------------------------------------------------------------------
    // Beat-Quantized Auto-Crossfade & Transition Queuing
    // -------------------------------------------------------------------------
    void triggerBeatSyncedCrossfade(
        std::uint32_t barCount = 2,
        std::optional<DeckId> targetDeck = std::nullopt,
        bool snapToNextDownbeat = true
    );

    [[nodiscard]] bool isBeatSyncCrossfade() const noexcept { return isBeatSyncCrossfade_; }
    [[nodiscard]] bool isPendingDownbeat() const noexcept { return pendingDownbeat_; }
    [[nodiscard]] std::uint32_t beatSyncTargetBars() const noexcept { return beatSyncTargetBars_; }

    // -------------------------------------------------------------------------
    // Musical Rhythm & Phase-Locked LFO Synthesis
    // -------------------------------------------------------------------------
    [[nodiscard]] float barPhase() const noexcept {
        return (static_cast<float>(beatInBar() - 1U) + beatPhase_) / 4.0f;
    }
    [[nodiscard]] float phrasePhase(std::uint32_t bars = 4) const noexcept {
        if (bars == 0) return 0.0f;
        const std::uint32_t barInPhrase = (currentBar() - 1U) % bars;
        return (static_cast<float>(barInPhrase) + barPhase()) / static_cast<float>(bars);
    }
    [[nodiscard]] float downbeatEnvelope() const noexcept { return downbeatEnvelope_; }
    [[nodiscard]] float beatSine(float cyclesPerBar = 1.0f) const noexcept;
    [[nodiscard]] float beatTri(float cyclesPerBar = 1.0f) const noexcept;

    // -------------------------------------------------------------------------
    // Autonomous Musical Bar-Synchronized Tour Progression
    // -------------------------------------------------------------------------
    struct TourBeatSyncConfig {
        bool enabled = false;
        std::uint32_t barsPerPreset = 8;     // Dwell length in bars (e.g. 4, 8, 16, 32)
        std::uint32_t transitionBars = 2;    // Transition duration in bars (e.g. 1, 2, 4)
    };

    void setTourBeatSyncConfig(const TourBeatSyncConfig& config) noexcept {
        tourSyncConfig_ = config;
        // Match transition admission: a cadence always names at least one bar.
        tourSyncConfig_.barsPerPreset = std::max(1U, config.barsPerPreset);
        tourSyncConfig_.transitionBars = std::max(1U, config.transitionBars);
    }
    [[nodiscard]] const TourBeatSyncConfig& tourBeatSyncConfig() const noexcept { return tourSyncConfig_; }
    void toggleTourBeatSync() noexcept { tourSyncConfig_.enabled = !tourSyncConfig_.enabled; }
    [[nodiscard]] bool isTourBeatSyncEnabled() const noexcept { return tourSyncConfig_.enabled; }

    using TourAdvanceCallback = std::function<void(DeckId targetDeck)>;
    void setTourAdvanceCallback(TourAdvanceCallback cb) { tourAdvanceCallback_ = std::move(cb); }

    // Proactive background JIT pre-warming callback dispatched on deck load or cue recall
    using DeckPrewarmCallback = std::function<void(DeckId deck, const SculptureProgramParams& params)>;
    void setPrewarmCallback(DeckPrewarmCallback cb) noexcept { prewarmCallback_ = std::move(cb); }
    [[nodiscard]] bool hasPrewarmCallback() const noexcept { return static_cast<bool>(prewarmCallback_); }

    // -------------------------------------------------------------------------
    // Tap-Tempo & Musical Beat Engine (BPM Clock)
    // -------------------------------------------------------------------------
    void tapTempo(float explicitTimestamp = -1.0f) noexcept;
    void setBpm(float bpm) noexcept;
    void nudgeBpm(float deltaBpm) noexcept { setBpm(bpm_ + deltaBpm); }
    [[nodiscard]] float bpm() const noexcept { return bpm_; }
    void resetBeatPhase() noexcept;

    [[nodiscard]] float beatPhase() const noexcept { return beatPhase_; } // [0.0, 1.0) inside current beat
    [[nodiscard]] std::uint64_t totalBeats() const noexcept { return totalBeats_; }
    [[nodiscard]] std::uint64_t totalBeatsElapsed() const noexcept { return totalBeats_; }
    [[nodiscard]] std::uint32_t currentBar() const noexcept { return static_cast<std::uint32_t>(totalBeats_ / 4U) + 1U; }
    [[nodiscard]] std::uint32_t beatInBar() const noexcept { return static_cast<std::uint32_t>(totalBeats_ % 4U) + 1U; }
    [[nodiscard]] bool isDownbeat() const noexcept { return isDownbeat_; }
    [[nodiscard]] bool isMusicalDownbeat() const noexcept { return isDownbeat_; }

    // -------------------------------------------------------------------------
    // Performance Cue Banks (Instant Preset Recall)
    // -------------------------------------------------------------------------
    void storeCue(std::size_t bankIndex, const SculptureProgramParams& params,
                  std::string_view name = "", std::string_view presetId = "");
    bool recallCue(std::size_t bankIndex, DeckId targetDeck);
    bool recallCueToOppositeDeck(std::size_t bankIndex);
    // Read-only bank admission, with no copies or prewarm callback. The borrow
    // lasts until this bank is stored/replaced or its owning deck is destroyed.
    [[nodiscard]] const PerformanceCuePoint* cue(std::size_t bankIndex) const noexcept;

    // -------------------------------------------------------------------------
    // VJ Master Bus Performance FX Section (Momentary Drops, Tape Stop, Beat Roll)
    // -------------------------------------------------------------------------
    struct MasterFxState {
        bool tapeStopActive = false;
        float tapeStopDuration = 1.0f;
        float tapeStopElapsed = 0.0f;
        float tapeStopSpeedFactor = 1.0f;

        bool beatRollActive = false;
        float beatRollSubdivision = 0.25f; // 1/4 beat loop
        float beatRollDurationBeats = 1.0f;
        float beatRollElapsedBeats = 0.0f;

        float chaosIntensity = 0.0f;
        float chaosDecayRate = 2.5f;

        float masterFilter = 0.0f; // [-1.0 (Low-Pass) .. 0.0 (Bypass) .. +1.0 (High-Pass)]
    };

    void triggerTapeStop(float durationSeconds = 1.0f) noexcept;
    void releaseTapeStop() noexcept;
    [[nodiscard]] bool isTapeStopActive() const noexcept;
    [[nodiscard]] float tapeStopSpeedFactor() const noexcept;

    void triggerBeatRoll(float subdivision = 0.25f, float durationBeats = 1.0f) noexcept;
    void releaseBeatRoll() noexcept;
    [[nodiscard]] bool isBeatRollActive() const noexcept;

    void triggerChaosBurst(float intensity = 1.0f, float decayRate = 2.5f) noexcept;
    [[nodiscard]] float chaosIntensity() const noexcept;

    void setMasterFilter(float filterValue) noexcept;
    [[nodiscard]] float masterFilter() const noexcept;

    [[nodiscard]] const MasterFxState& masterFx() const noexcept { return masterFx_; }

    // Musical Macro Automation Accessors
    [[nodiscard]] SculptureMacroAutomationEngine& macroEngine() noexcept { return macroEngine_; }
    [[nodiscard]] const SculptureMacroAutomationEngine& macroEngine() const noexcept { return macroEngine_; }

    // -------------------------------------------------------------------------
    // Real-Time Clock Simulation Tick
    // -------------------------------------------------------------------------
    // Refuse invalid/unrepresentable clock proposals before effects/callbacks.
    // Finite nonpositive deltas retain the existing zero-time update behavior.
    // Macro admission stays NotEvaluated if the clock refuses or a fade owns it.
    [[nodiscard]] bool tryUpdate(float deltaSeconds,
        SculptureCompositionAdmission* macroAdmission = nullptr) noexcept;
    // Legacy callers hold this owner on refusal; other clocks are not rewound.
    void update(float deltaSeconds) noexcept;
    
    // Apply internal deck-driven FX mathematically to the global parameter payload
    void applyMasterFx(SculptureProgramParams& params, float time, float& deltaSeconds) const noexcept;

    // -------------------------------------------------------------------------
    // Diagnostic & HUD Formatting
    // -------------------------------------------------------------------------
    [[nodiscard]] std::string formatDeckStatus() const;

private:
    DeckSlot deckA_{};
    DeckSlot deckB_{};
    float crossfadePos_ = 0.0f; // 0.0 = 100% Deck A, 1.0 = 100% Deck B
    CrossfadeCurve crossfadeCurve_ = CrossfadeCurve::EqualPower;

    // Auto-crossfader state
    bool autoCrossfading_ = false;
    float autoCrossfadeStartPos_ = 0.0f;
    float autoCrossfadeTargetPos_ = 1.0f;
    float autoCrossfadeDuration_ = 4.0f;
    float autoCrossfadeElapsed_ = 0.0f;
    float autoCrossfadeProgress_ = 0.0f;

    // Beat-synchronized crossfade state
    bool isBeatSyncCrossfade_ = false;
    bool pendingDownbeat_ = false;
    std::uint32_t beatSyncTargetBars_ = 2;
    std::optional<DeckId> pendingTargetDeck_ = std::nullopt;
    float beatSyncElapsedBeats_ = 0.0f;
    float beatSyncTotalBeats_ = 8.0f;

    // Downbeat decay envelope
    float downbeatEnvelope_ = 0.0f;

    // Musical tour sync
    TourBeatSyncConfig tourSyncConfig_{};
    TourAdvanceCallback tourAdvanceCallback_{};
    DeckPrewarmCallback prewarmCallback_{};
    std::uint32_t lastTourTriggerBar_ = 0;

    // Tempo clock state
    float bpm_ = 120.0f;
    float accumulatedTime_ = 0.0f;
    float beatPhase_ = 0.0f;
    std::uint64_t totalBeats_ = 0U;
    bool isDownbeat_ = false;

    // Tap-tempo filtering
    std::vector<float> tapTimestamps_{};

    // Cue banks
    std::array<PerformanceCuePoint, kMaxCueBanks> cueBanks_{};

    // Master bus performance FX
    MasterFxState masterFx_{};

    // Musical Macro Automation Engine
    SculptureMacroAutomationEngine macroEngine_{};
};

} // namespace epi_sculpt
