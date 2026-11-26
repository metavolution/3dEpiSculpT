#pragma once

namespace epi_sculpt {

// Submission is distinct from both device health and eventual display. Callers
// must not infer admission from telemetry counters or from a healthy queue.
enum class ImplicitSubmissionResult {
    Submitted,
    Backpressure,
    Superseded,
    Failed,
};

[[nodiscard]] constexpr bool implicitSubmissionHealthy(ImplicitSubmissionResult result) noexcept {
    return result != ImplicitSubmissionResult::Failed;
}

} // namespace epi_sculpt
