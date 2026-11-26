#pragma once

#include "SculptureLiveTransport.h"

#include <algorithm>

namespace epi_sculpt {

// Live-performance time control scales simulation progression without mutating
// authored world parameters or a captured WorldMode.
class SculpturePlaybackRate {
public:
    static constexpr float kMinimum = kLivePlaybackRateMinimum;
    static constexpr float kMaximum = kLivePlaybackRateMaximum;
    static constexpr float kStep = kLivePlaybackRateStep;

    void increase() noexcept { set(value_ * kStep); }
    void decrease() noexcept { set(value_ / kStep); }
    void reset() noexcept { value_ = 1.0f; }

    [[nodiscard]] float value() const noexcept { return value_; }
    [[nodiscard]] float scale(float deltaSeconds) const noexcept {
        return std::max(0.0f, deltaSeconds) * value_;
    }

private:
    void set(float value) noexcept { value_ = std::clamp(value, kMinimum, kMaximum); }
    float value_ = 1.0f;
};

} // namespace epi_sculpt
