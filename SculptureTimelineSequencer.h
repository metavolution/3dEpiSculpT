#pragma once

#include "SculpturePerformanceDeck.h"
#include "SculpturePresetLibrary.h"

#include <string>
#include <string_view>
#include <vector>
#include <cstdint>
#include <filesystem>
#include <optional>

namespace epi_sculpt {

// -----------------------------------------------------------------------------
// Educational Annotation: SCRIPT-BASED TIMELINE SEQUENCER
// -----------------------------------------------------------------------------
// Realizes the vision requirement for "Script-based programs & animation paths"
// and "Timeline-based editing" (docu/project-vision.txt:97, 264-266).
//
// The SculptureTimelineSequencer acts as an autonomous composition conductor:
// 1. Ingests structured JSON cue playlists with per-cue dwell and transition timings.
// 2. Automates the dual-deck SculpturePerformanceDeck (Deck A / Deck B alternating).
// 3. Automatically stages the upcoming cue onto the inactive deck and triggers
//    JIT binary cache pre-warming BEFORE the transition begins.
// 4. Executes equal-power / smoothstep crossfades between decks at the exact time.
// -----------------------------------------------------------------------------

struct TimelineCue {
    std::string presetIdOrName{};
    std::string title{};
    float dwellSeconds = 4.0f;
    float transitionSeconds = 2.0f;
    CrossfadeCurve curve = CrossfadeCurve::EqualPower;
    float cameraEye[3] = {0.0f, 0.0f, 0.0f};
    float cameraTarget[3] = {0.0f, 0.0f, 0.0f};
    bool useExplicitCamera = false;
};

struct TimelineSequence {
    std::string name = "Untitled Timeline";
    std::string description{};
    bool loop = true;
    std::vector<TimelineCue> cues{};

    [[nodiscard]] float totalDuration() const noexcept {
        float sum = 0.0f;
        for (const auto& cue : cues) {
            sum += cue.dwellSeconds + cue.transitionSeconds;
        }
        return sum;
    }
};

enum class TimelineStage : std::uint8_t {
    Idle = 0,
    Dwelling = 1,
    Transitioning = 2,
    Finished = 3
};

struct TimelinePlayheadState {
    bool isPlaying = false;
    bool isPaused = false;
    std::size_t activeCueIndex = 0U;
    std::size_t nextCueIndex = 0U;
    TimelineStage stage = TimelineStage::Idle;
    float cueTimeRemaining = 0.0f;
    float totalElapsedTime = 0.0f;
    float totalDuration = 0.0f;
    DeckId liveDeck = DeckId::DeckA;
    std::string activePresetTitle{};
    std::string nextPresetTitle{};
};

class SculptureTimelineSequencer {
public:
    SculptureTimelineSequencer() noexcept = default;
    explicit SculptureTimelineSequencer(TimelineSequence sequence);

    // -------------------------------------------------------------------------
    // Timeline Loading & Construction
    // -------------------------------------------------------------------------
    void loadTimeline(TimelineSequence sequence);
    bool loadTimelineFromJson(std::string_view jsonStr);
    bool loadTimelineFromFile(const std::filesystem::path& path);

    // Helper: Build a timeline directly from a curated tour in the preset library
    bool buildFromTour(
        const SculpturePresetLibrary& library,
        std::string_view tourName,
        float dwellSeconds = 4.0f,
        float transitionSeconds = 2.0f,
        CrossfadeCurve curve = CrossfadeCurve::EqualPower,
        bool loop = true
    );

    // -------------------------------------------------------------------------
    // Playhead Transport Controls
    // -------------------------------------------------------------------------
    void play();
    void pause() noexcept;
    void resume() noexcept;
    void stop() noexcept;
    void togglePlayPause();
    void nextCue(SculpturePerformanceDeck& deck, const SculpturePresetLibrary* library);
    void previousCue(SculpturePerformanceDeck& deck, const SculpturePresetLibrary* library);
    void skipToCue(std::size_t cueIndex, SculpturePerformanceDeck& deck, const SculpturePresetLibrary* library);

    // -------------------------------------------------------------------------
    // Simulation / Frame Tick Update
    // -------------------------------------------------------------------------
    // Advances playback and orchestrates Deck A/B transitions
    void update(float deltaSeconds, SculpturePerformanceDeck& deck, const SculpturePresetLibrary* library);

    // -------------------------------------------------------------------------
    // State Queries & Telemetry
    // -------------------------------------------------------------------------
    [[nodiscard]] bool isPlaying() const noexcept { return isPlaying_; }
    [[nodiscard]] bool isPaused() const noexcept { return isPaused_; }
    [[nodiscard]] TimelineStage stage() const noexcept { return stage_; }
    [[nodiscard]] const TimelineSequence& sequence() const noexcept { return sequence_; }
    [[nodiscard]] TimelinePlayheadState playheadState() const noexcept;
    [[nodiscard]] std::string formatHudString() const;
    [[nodiscard]] std::string formatSummary() const;

private:
    void stageCurrentAndNextCues(SculpturePerformanceDeck& deck, const SculpturePresetLibrary* library);
    void stageCueToDeck(DeckId targetDeck, const TimelineCue& cue, SculpturePerformanceDeck& deck, const SculpturePresetLibrary* library);

    TimelineSequence sequence_{};
    bool isPlaying_ = false;
    bool isPaused_ = false;
    std::size_t activeCueIndex_ = 0U;
    std::size_t nextCueIndex_ = 0U;
    TimelineStage stage_ = TimelineStage::Idle;
    float cueCountdown_ = 0.0f;
    float totalElapsedTime_ = 0.0f;
    DeckId liveDeck_ = DeckId::DeckA;
    bool cuesStaged_ = false;
};

} // namespace epi_sculpt
