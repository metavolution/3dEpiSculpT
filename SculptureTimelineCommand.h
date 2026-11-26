#pragma once

#include <cstddef>
#include <string>

namespace epi_sculpt {

// Semantic intent only. Adapters may construct timeline commands without
// depending on the event bus, a playback owner or native graphics.
struct EventTimelineControl {
    enum class Action {
        Play,
        Pause,
        Stop,
        TogglePlayPause,
        NextCue,
        PreviousCue,
        SkipToCue,
        LoadFile,
        BuildTour,
        BuildBeatSynced,
        Export
    };
    Action action = Action::TogglePlayPause;
    std::string argument{}; // Filepath, Tour name, or export path
    std::size_t cueIndex = 0;
    float dwellSec = 4.0f;
    float transSec = 2.0f;
    float bpm = 0.0f;
};

} // namespace epi_sculpt
