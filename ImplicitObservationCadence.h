#pragma once

#include "ImplicitFrameRequest.h"
#include "ImplicitSubmissionResult.h"
#include <cmath>
#include <optional>

namespace epi_sculpt {

// Observation is scheduled on monotonic wall time, never the mathematical clock.
// Only an actual submission consumes dirtiness; a busy producer leaves the latest
// view pending. Rate-limit attempts too, so overload cannot cause catch-up bursts.
class ImplicitObservationCadence {
public:
    enum class ResetComparison {
        Payload,
        CommandToken,
    };

    void reset() noexcept {
        accepted_.reset();
        nextAttempt_ = 0.0;
    }

    [[nodiscard]] bool beginAttempt(double wallSeconds,
                                    const ImplicitFrameRequest& request,
                                    float updateHz,
                                    bool forceRefresh = false,
                                    ResetComparison resetComparison = ResetComparison::Payload) noexcept {
        // A pending reset is new command identity even if packed view values
        // match. It bypasses equality, never the wall-clock retry limit.
        if (!std::isfinite(wallSeconds) || wallSeconds < nextAttempt_
            || (!forceRefresh && accepted_ && sameView(*accepted_, request, resetComparison))) return false;
        nextAttempt_ = wallSeconds + (std::isfinite(updateHz) && updateHz > 0.0f
            ? 1.0 / static_cast<double>(updateHz) : 0.0);
        return true;
    }

    void completeAttempt(const ImplicitFrameRequest& request, bool submitted) noexcept {
        if (submitted) accepted_ = request;
    }

    void completeAttempt(const ImplicitFrameRequest& request, ImplicitSubmissionResult result) noexcept {
        completeAttempt(request, result == ImplicitSubmissionResult::Submitted);
    }

private:
    static bool sameView(const ImplicitFrameRequest& a,
                         const ImplicitFrameRequest& b,
                         ResetComparison resetComparison) noexcept {
        const bool sameParams = [&] {
            if (resetComparison == ResetComparison::Payload) return a.params == b.params;
            // Token-controlled callers admit resets through forceRefresh, not
            // view dirtiness. Ignore only the packed reset lane; retain original
            // accepted/request payloads for actual production and settlement.
            auto params = a.params;
            params.outputScale[3] = b.params.outputScale[3];
            return params == b.params;
        }();
        // Generation and per-frame delta are not image identity. A paused view
        // must not resubmit just because a caller allocated another generation.
        return sameParams && a.frame.time == b.frame.time
            && a.frame.morph == b.frame.morph
            && a.cameraPosition == b.cameraPosition && a.cameraTarget == b.cameraTarget
            && a.cameraUp == b.cameraUp && a.fovYRadians == b.fovYRadians
            && a.eyeSeparation == b.eyeSeparation
            && a.width == b.width && a.height == b.height
            && a.producerEpoch == b.producerEpoch;
    }

    std::optional<ImplicitFrameRequest> accepted_;
    double nextAttempt_ = 0.0;
};

} // namespace epi_sculpt
