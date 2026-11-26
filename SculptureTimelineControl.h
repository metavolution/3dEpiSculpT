#pragma once

#include "SculptureSequencePlayback.h"
#include "SculptureTimelineCommand.h"

namespace epi_sculpt {

struct TimelineControlOutcome {
    bool handled = false;
    bool selectedSequenceCue = false;
};

// Resolve ownership once. Mutate only that owner's values, then report facts;
// native modulation/graphics side effects remain in the application consumer.
[[nodiscard]] inline TimelineControlOutcome applyTimelineControl(
    const EventTimelineControl& command, bool cinematic,
    SculptureSequencer& sequence, SculptureMorphChoreographer& playlist) noexcept {
    using Action = EventTimelineControl::Action;
    if (cinematic) {
        const auto transport = [&](sequence_playback::TransportAction action) {
            const auto result = sequence_playback::transport(true, sequence, action);
            return TimelineControlOutcome{result.handled, result.selectedCue};
        };
        const auto navigate = [&](sequence_playback::CueNavigation action) {
            return TimelineControlOutcome{true,
                sequence_playback::navigate(true, sequence, action, command.cueIndex)};
        };
        switch (command.action) {
        case Action::Play: return transport(sequence_playback::TransportAction::Play);
        case Action::Pause: return transport(sequence_playback::TransportAction::Pause);
        case Action::Stop: return transport(sequence_playback::TransportAction::Stop);
        case Action::TogglePlayPause:
            return transport(sequence_playback::TransportAction::TogglePlayPause);
        case Action::NextCue: return navigate(sequence_playback::CueNavigation::Next);
        case Action::PreviousCue: return navigate(sequence_playback::CueNavigation::Previous);
        case Action::SkipToCue: return navigate(sequence_playback::CueNavigation::Select);
        default: return {}; // File/build/export requests stay with their host.
        }
    }

    switch (command.action) {
    case Action::Play:
        if (!playlist.isPlaylistActive()) playlist.startPlaylist();
        else if (playlist.isPlaylistPaused()) playlist.togglePlaylistPause();
        break;
    case Action::Pause:
        if (playlist.isPlaylistActive() && !playlist.isPlaylistPaused()) {
            playlist.togglePlaylistPause();
        }
        break;
    case Action::Stop:
        playlist.stopPlaylist();
        break;
    case Action::TogglePlayPause:
        if (playlist.isPlaylistActive()) playlist.togglePlaylistPause();
        else playlist.startPlaylist();
        break;
    case Action::NextCue:
        playlist.stepPlaylistNext();
        break;
    case Action::PreviousCue:
        playlist.stepPlaylistPrevious();
        break;
    case Action::SkipToCue:
        if (command.cueIndex < playlist.playlistSize()) {
            playlist.startPlaylistFrom(command.cueIndex);
        }
        break;
    default: return {};
    }
    return {true, false};
}

} // namespace epi_sculpt
