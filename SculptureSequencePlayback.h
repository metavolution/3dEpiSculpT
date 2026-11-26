#pragma once

#include "SculptureMorphChoreographer.h"
#include "SculpturePresets.h"

namespace epi_sculpt::sequence_playback {

enum class CueNavigation { Next, Previous, Select };
enum class TransportAction { Play, Pause, Stop, TogglePlayPause };

struct TransportOutcome {
    bool handled = false;
    // Only explicit cue selection needs application-side routing reapplication.
    bool selectedCue = false;
};

[[nodiscard]] inline TransportOutcome transport(bool cinematic, SculptureSequencer& sequence,
                                                TransportAction action) noexcept {
    if (!cinematic) return {};
    switch (action) {
    case TransportAction::Pause:
        sequence.pause();
        return {true, false};
    case TransportAction::Stop:
        sequence.pause();
        return {true, sequence.seekKeyframe(0)};
    case TransportAction::TogglePlayPause:
        if (sequence.active()) {
            sequence.pause();
            return {true, false};
        }
        [[fallthrough]];
    case TransportAction::Play: {
        bool selected = false;
        if (!sequence.active() && sequence.currentTimelineTime() >= sequence.totalDuration()) {
            selected = sequence.seekKeyframe(0);
            if (!selected) return {true, false};
        }
        sequence.resume();
        return {true, selected};
    }
    }
    return {};
}

// A navigation request observes the current owner; it never takes over playback
// or starts a stopped sequence. The application applies selected routing itself.
[[nodiscard]] inline bool navigate(bool cinematic, SculptureSequencer& sequence,
                                    CueNavigation action, std::size_t index = 0) noexcept {
    if (!cinematic) return false;
    switch (action) {
    case CueNavigation::Next: return sequence.stepKeyframe(1);
    case CueNavigation::Previous: return sequence.stepKeyframe(-1);
    case CueNavigation::Select: return sequence.seekKeyframe(index);
    }
    return false;
}

// Application policy only: the caller owns the cinematic flag and supplies the
// accepted runtime delta. These helpers own no clock, resource or cached state.
inline void enter(bool& cinematic, SculptureSequencer& sequence) noexcept {
    // An explicit entry resumes a paused sample, or restarts a completed run.
    // Ordinary frame advancement never calls this function.
    (void)transport(true, sequence, TransportAction::Play);
    cinematic = true;
}

inline void leave(bool& cinematic, SculptureSequencer& sequence,
                  SculptureMorphChoreographer& morph, bool preserveLoadedState) {
    // Completion has already stopped the sequencer; its final sample still
    // belongs in ordinary preset control when the user explicitly exits.
    cinematic = false;
    sequence.pause();
    if (!preserveLoadedState) {
        morph = SculptureMorphChoreographer(sequence.currentParams());
    }
}

[[nodiscard]] inline bool acceptLoad(bool loaded, bool& cinematic,
                                     SculptureSequencer& sequence,
                                     SculptureMorphChoreographer& morph) noexcept {
    if (!loaded) return false;
    morph.stopPlaylist();
    enter(cinematic, sequence);
    return true;
}

inline void advance(bool& cinematic, SculptureSequencer& sequence,
                    SculptureMorphChoreographer& morph, float simulationDelta,
                    bool preserveLoadedState) {
    if (cinematic) {
        sequence.update(simulationDelta);
    } else if (sequence.active()) {
        // Other successful state imports also leave cinematic mode. Their
        // loaded sample takes precedence over the old sequence's handoff.
        leave(cinematic, sequence, morph, preserveLoadedState);
    }
}

} // namespace epi_sculpt::sequence_playback
