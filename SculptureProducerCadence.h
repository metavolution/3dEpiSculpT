#pragma once

#include "SculptureProgramSchema.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

namespace epi_sculpt {

// -----------------------------------------------------------------------------
// Educational Annotation: SCULPTURE PRODUCER CADENCE & TEMPORAL DECOUPLING
// -----------------------------------------------------------------------------
// Decouples high-frequency presentation refresh rates (e.g. 120 FPS / 240 FPS
// Vulkan display compositing) from lower-frequency asynchronous mathematical
// simulations (e.g. 30 FPS / 60 FPS OpenCL compute dispatches).
//
// Key Invariants:
// 1. Simulation Delta Accuracy: Uses the represented simulation-time difference
//    between accepted dispatches without accumulating deadline rounding drift.
// 2. Rewind & Teleport Protection: If simulation time jumps backward or resets,
//    the cadence cleanly resets delta accumulation to zero.
// 3. Parameter-Change Preemption: Immediate submission when critical parameters
//    change, bypassing wait intervals for instant interactive feedback.
// 4. Adaptive Latency Throttling: When GPU compute or staging latency exceeds
//    the target frame budget, dynamically reduces effective dispatch rate to
//    prevent command queue backlog.
// -----------------------------------------------------------------------------

struct SculptureProducerCadenceConfig {
    // Positive rates are admitted to [1,240] Hz; zero means unrestricted.
    // Negative or non-finite rates fall back to the configured default (30 Hz).
    float updateHz = 30.0f;
    bool dispatchOnParameterChange = false;
    bool enableAdaptiveThrottling = false;
    float targetLatencyMilliseconds = 16.67f;
    float speedMultiplier = 1.0f;
    bool paused = false;
};

struct SculptureProducerDispatch {
    bool submit = false;
    float simulationDeltaSeconds = 0.0f;
};

// A cadence proposal is an immutable control-plane decision. Preparing it does
// not consume cadence state; callers commit only after the corresponding GPU
// submission has actually been accepted. This prevents backpressure, skipped
// presentation frames, or queue rejection from advancing simulation time.
template <typename TParams = SculptureProgramParams>
struct SculptureProducerCadenceProposal {
    bool submit = false;
    float simulationDeltaSeconds = 0.0f;
    float simulationTime = 0.0f;
    // Freeze the accepted period; later quality/rate changes apply to the
    // next accepted proposal rather than rewriting its existing deadline.
    float submissionInterval = 0.0f;
    bool cadenceDue = false;
    bool parameterTriggered = false;
    TParams params{};
    // Borrowed owner token: the owning cadence must outlive this proposal.
    // Flagship deferred callbacks are settled within the owning frame.
    const void* owner = nullptr;
    std::uint64_t revision = 0U;
};

struct SculptureProducerCadenceTelemetry {
    std::uint64_t totalDispatches = 0U;
    std::uint64_t skippedTicks = 0U;
    std::uint64_t parameterTriggeredDispatches = 0U;
    std::uint64_t deferredDispatches = 0U;
    float effectiveUpdateHz = 30.0f;
    float lastLatencyMilliseconds = 0.0f;
    float speedMultiplier = 1.0f;
    bool isPaused = false;
};

template <typename TParams = SculptureProgramParams>
class SculptureProducerCadence {
public:
    using EqualityPredicate = bool(*)(const TParams&, const TParams&);

    explicit SculptureProducerCadence(SculptureProducerCadenceConfig config = {}, EqualityPredicate equals = nullptr)
        : config_(normalizedConfig(config)),
          equals_(equals) {
        restoreConfiguredControls();
    }

    using Proposal = SculptureProducerCadenceProposal<TParams>;

    // Computes whether work is due without mutating cadence history. The
    // proposal may safely cross a deferred submission boundary (for example a
    // Vulkan frame-recording callback) and is committed only after acceptance.
    // An explicit pending command can request submission even when its payload
    // equals the last accepted one (for example two consecutive reset commands).
    // Nonfinite timestamps or unrepresentable deltas yield no submission and
    // cannot replace accepted clock history. Callers retain reset/re-offer policy.
    [[nodiscard]] Proposal prepare(
        float simulationTime,
        const TParams& params,
        bool forceSubmit = false) const noexcept {
        if (!std::isfinite(simulationTime)
            || revision_ == std::numeric_limits<std::uint64_t>::max()) return {};
        const bool rewound = hasSubmission_ && simulationTime < lastSimulationTime_;
        const float interval = effectiveHz_ > 0.0f ? 1.0f / effectiveHz_ : 0.0f;
        // Subtract in double: adding a short interval to a large float timestamp
        // may round back to that timestamp and make every frame appear due.
        const double elapsed = static_cast<double>(simulationTime)
            - static_cast<double>(lastSimulationTime_);
        const bool cadenceDue = !hasSubmission_ || rewound
            || elapsed >= static_cast<double>(submissionInterval_) - 1.0e-5;
        // Comparing a full producer payload can be expensive. Only inspect it
        // when parameter-change dispatch is enabled and no force command has
        // already established that this opportunity is parameter-triggered.
        const bool parameterTriggered = forceSubmit
            || (config_.dispatchOnParameterChange
                && (!hasSubmission_ || parametersDiffer(params)));
        const bool submit = cadenceDue || parameterTriggered;
        float simulationDeltaSeconds = 0.0f;
        if (submit && hasSubmission_ && !rewound && !paused_) {
            // Admit representability before float arithmetic; do not silently
            // saturate a mathematical step or change ordinary float rounding.
            constexpr double maximumDelta = std::numeric_limits<float>::max();
            if (elapsed > maximumDelta) return {};
            const float rawElapsed = std::max(0.0f, simulationTime - lastSimulationTime_);
            if (static_cast<double>(rawElapsed) * speedMultiplier_ > maximumDelta) return {};
            simulationDeltaSeconds = rawElapsed * speedMultiplier_;
        }
        return {
            .submit = submit,
            .simulationDeltaSeconds = simulationDeltaSeconds,
            .simulationTime = simulationTime,
            .submissionInterval = interval,
            .cadenceDue = cadenceDue,
            .parameterTriggered = parameterTriggered,
            .params = params,
            .owner = this,
            .revision = revision_,
        };
    }

    // Use immediately before producer admission. The owner and control-plane
    // revision qualify the proposal; this does not prove GPU acceptance.
    [[nodiscard]] bool canCommit(const Proposal& proposal) const noexcept {
        return proposal.submit && proposal.owner == this && proposal.revision == revision_
            && revision_ != std::numeric_limits<std::uint64_t>::max();
    }

    void commit(const Proposal& proposal) noexcept {
        if (!proposal.submit) {
            ++telemetry_.skippedTicks;
            return;
        }
        // Commit remains a post-acceptance operation. Callers that defer work
        // must preflight before production; do not discard already-accepted math.
        invalidateProposals();
        lastParams_ = proposal.params;
        lastSimulationTime_ = proposal.simulationTime;
        submissionInterval_ = proposal.submissionInterval;
        hasSubmission_ = true;
        ++telemetry_.totalDispatches;
        telemetry_.speedMultiplier = speedMultiplier_;
        telemetry_.isPaused = paused_;
        if (proposal.parameterTriggered && !proposal.cadenceDue) {
            ++telemetry_.parameterTriggeredDispatches;
        }
    }

    // Records an accepted cadence decision that could not be submitted. The
    // temporal clock remains untouched so the same simulation interval remains
    // due on the next opportunity.
    void defer(const Proposal& proposal) noexcept {
        if (proposal.submit) ++telemetry_.deferredDispatches;
        else ++telemetry_.skippedTicks;
    }

    // Compatibility helper for secondary/demo call sites that still have an
    // immediate submission contract. Flagship runtime code uses prepare/commit.
    [[nodiscard]] SculptureProducerDispatch next(
        float simulationTime,
        const TParams& params) noexcept {
        const Proposal proposal = prepare(simulationTime, params);
        if (!proposal.submit) {
            defer(proposal);
            return {};
        }
        commit(proposal);
        return {.submit = true, .simulationDeltaSeconds = proposal.simulationDeltaSeconds};
    }

    void setSpeedMultiplier(float speed) noexcept {
        if (std::isfinite(speed)) {
            const float next = std::clamp(speed, 0.05f, 20.0f);
            if (next != speedMultiplier_) invalidateProposals();
            speedMultiplier_ = next;
            telemetry_.speedMultiplier = speedMultiplier_;
        }
    }

    [[nodiscard]] float speedMultiplier() const noexcept {
        return speedMultiplier_;
    }

    void increaseSpeed(float factor = 1.2f) noexcept {
        if (std::isfinite(factor) && factor > 1.0f) {
            // Compare before multiplying so a finite factor cannot overflow
            // to infinity and bypass the speed setter's finite-value guard.
            setSpeedMultiplier(factor >= 20.0f / speedMultiplier_
                ? 20.0f : speedMultiplier_ * factor);
        }
    }

    void decreaseSpeed(float factor = 1.2f) noexcept {
        if (std::isfinite(factor) && factor > 1.0f) {
            setSpeedMultiplier(speedMultiplier_ / factor);
        }
    }

    void resetSpeed() noexcept {
        setSpeedMultiplier(1.0f);
    }

    void setPaused(bool paused) noexcept {
        if (paused != paused_) invalidateProposals();
        paused_ = paused;
        telemetry_.isPaused = paused_;
    }

    void togglePause() noexcept { setPaused(!paused_); }

    [[nodiscard]] bool paused() const noexcept {
        return paused_;
    }

    // Dynamic latency feedback: adjusts effective dispatch frequency to match GPU throughput
    void observeProducerLatency(float latencyMilliseconds) noexcept {
        if (!config_.enableAdaptiveThrottling || !std::isfinite(latencyMilliseconds)
            || latencyMilliseconds <= 0.0f) {
            return;
        }
        telemetry_.lastLatencyMilliseconds = latencyMilliseconds;

        // Zero means unrestricted cadence, not a rate for adaptation to invent.
        // For positive rates the configured value is always the upper bound.
        const float baseHz = config_.updateHz;
        if (baseHz == 0.0f) return;
        const float minimumHz = std::min(10.0f, baseHz);
        const float previousHz = effectiveHz_;
        // Widen the thresholds: an admitted finite float target can overflow
        // when multiplied in float, suppressing an overload decision.
        const double targetMs = config_.targetLatencyMilliseconds;
        if (static_cast<double>(latencyMilliseconds) > targetMs * 1.15) {
            // Overloaded: reduce rate by 10%
            effectiveHz_ *= 0.90f;
        } else if (static_cast<double>(latencyMilliseconds) < targetMs * 0.70) {
            // Headroom: smoothly restore toward configured rate
            effectiveHz_ *= 1.05f;
        }
        effectiveHz_ = std::clamp(effectiveHz_, minimumHz, baseHz);
        if (effectiveHz_ != previousHz) invalidateProposals();
        telemetry_.effectiveUpdateHz = effectiveHz_;
    }

    void setConfig(const SculptureProducerCadenceConfig& config) noexcept {
        invalidateProposals();
        config_ = normalizedConfig(config);
        restoreConfiguredControls();
    }

    [[nodiscard]] const SculptureProducerCadenceConfig& config() const noexcept {
        return config_;
    }

    [[nodiscard]] const SculptureProducerCadenceTelemetry& telemetry() const noexcept {
        return telemetry_;
    }

    [[nodiscard]] float effectiveUpdateHz() const noexcept {
        return effectiveHz_;
    }

    // An inactive producer branch (for example fullscreen implicit rendering)
    // must not resume with a large artificial temporal step.
    void reset() noexcept {
        invalidateProposals();
        hasSubmission_ = false;
        restoreConfiguredControls();
    }

private:
    [[nodiscard]] bool parametersDiffer(const TParams& params) const noexcept {
        if (equals_) return !equals_(params, lastParams_);
        if constexpr (requires { params != lastParams_; }) {
            return params != lastParams_;
        } else if constexpr (requires { params == lastParams_; }) {
            return !(params == lastParams_);
        } else {
            return false;
        }
    }

    void invalidateProposals() noexcept {
        // Saturation is fail-closed: prepare/canCommit refuse further work.
        if (revision_ != std::numeric_limits<std::uint64_t>::max()) ++revision_;
    }

    [[nodiscard]] static SculptureProducerCadenceConfig normalizedConfig(
        SculptureProducerCadenceConfig config) noexcept {
        config.updateHz = normalizedUpdateHz(config.updateHz);
        config.speedMultiplier = std::isfinite(config.speedMultiplier)
            ? std::clamp(config.speedMultiplier, 0.05f, 20.0f)
            : SculptureProducerCadenceConfig{}.speedMultiplier;
        config.targetLatencyMilliseconds = std::isfinite(config.targetLatencyMilliseconds)
            ? std::max(1.0f, config.targetLatencyMilliseconds)
            : SculptureProducerCadenceConfig{}.targetLatencyMilliseconds;
        return config;
    }

    // Restore the admitted baseline without touching accepted clock history or
    // lifetime counters. reset() separately discards only cadence eligibility.
    void restoreConfiguredControls() noexcept {
        effectiveHz_ = config_.updateHz;
        speedMultiplier_ = config_.speedMultiplier;
        paused_ = config_.paused;
        telemetry_.effectiveUpdateHz = effectiveHz_;
        telemetry_.speedMultiplier = speedMultiplier_;
        telemetry_.isPaused = paused_;
    }

    [[nodiscard]] static float normalizedUpdateHz(float hz) noexcept {
        if (!std::isfinite(hz) || hz < 0.0f) {
            return SculptureProducerCadenceConfig{}.updateHz;
        }
        if (hz == 0.0f) return 0.0f;
        return std::clamp(hz, 1.0f, 240.0f);
    }

    SculptureProducerCadenceConfig config_{};
    SculptureProducerCadenceTelemetry telemetry_{};
    TParams lastParams_{};
    EqualityPredicate equals_ = nullptr;
    float effectiveHz_ = 30.0f;
    float speedMultiplier_ = 1.0f;
    float lastSimulationTime_ = 0.0f;
    float submissionInterval_ = 0.0f;
    bool hasSubmission_ = false;
    bool paused_ = false;
    std::uint64_t revision_ = 1U;
};

} // namespace epi_sculpt
