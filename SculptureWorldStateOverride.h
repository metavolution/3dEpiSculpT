#pragma once

#include "SculptureProgramSchema.h"

namespace epi_sculpt {

// A loaded capture owns the camera-independent world coordinates until live
// simulation resumes. Rendering policy and GPU state deliberately stay outside
// this small application-state boundary.
class SculptureWorldStateOverride {
public:
    void restore(const SculptureWorldState& state) noexcept {
        state_ = state;
        active_ = true;
    }

    void clear() noexcept { active_ = false; }

    [[nodiscard]] bool active() const noexcept { return active_; }

    [[nodiscard]] SculptureWorldState resolve(
        const SculptureWorldState& liveState) const noexcept {
        return active_ ? state_ : liveState;
    }

private:
    SculptureWorldState state_{};
    bool active_ = false;
};

} // namespace epi_sculpt
