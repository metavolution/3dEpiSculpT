#pragma once

#include "SculptureModulationEngine.h"
#include "SculptureMorphChoreographer.h"
#include "SculpturePresetIdentityIndex.h"
#include "SculpturePlaylistCapture.h"

#include <array>
#include <span>
#include <type_traits>
#include <utility>

namespace epi_sculpt {

// Cold semantic restoration, independent of file IO and native presentation.
// Absence is a no-op. Present captures replace the complete playlist, starting
// from the destination's current parameter value (the captured moment).
// Refusal or a preparation exception leaves the destination entirely unchanged.
// The caller retains responsibility for publishing the rest of the loaded world.
[[nodiscard]] inline bool tryRestoreLibraryPlaylist(
    std::span<const SculpturePresetAsset> catalog,
    const SculpturePlaylistCaptureState& state,
    SculptureMorphChoreographer& destination) {
    if (!state.hasLibraryTargets) return true;
    if (!isPlaylistCaptureValid(state)) return false;

    // One borrowed index for this transaction; never retain it across reload.
    // Duplicate IDs are ambiguous, and titles cannot substitute for missing IDs.
    const SculpturePresetIdentityIndex identities{catalog};
    std::array<const SculpturePresetAsset*, SculpturePlaylistCaptureState::kMaxTargets> assets{};
    for (std::size_t index = 0; index < state.targets.size(); ++index) {
        const auto& cue = state.targets[index];
        const auto* asset = identities.resolveExactId(cue.presetId).asset;
        if (!asset) return false;
        // Empty revisions preserve the existing unversioned/legacy contract.
        if (!cue.presetRevision.empty() && cue.presetRevision != asset->revision) return false;
        for (const auto& route : asset->routingMap) {
            if (!SculptureModulationEngine::inspectRouteResult(route).route) return false;
        }
        assets[index] = asset;
    }

    SculptureMorphChoreographer candidate(destination.currentParams());
    for (std::size_t index = 0; index < state.targets.size(); ++index) {
        const auto& cue = state.targets[index];
        candidate.addPlaylistAsset(*assets[index], cue.durationSeconds, cue.dwellSeconds, cue.easing);
    }
    // Playback retains its own timing backstop when installing the admitted mode.
    if (!candidate.trySetTourPlaybackMode(state.playbackMode)) return false;
    candidate.restorePlaylistRuntime(
        state.targetIndex, state.loop, state.paused, state.dwellSeconds, state.active);

    // Preparation may allocate; publication cannot. Keep this contract explicit
    // if the choreographer gains new state with different move semantics.
    static_assert(std::is_nothrow_move_assignable_v<SculptureMorphChoreographer>);
    destination = std::move(candidate);
    return true;
}

} // namespace epi_sculpt
