#include "SculpturePerformanceMonitor.h"
#include "SculptureStagedComputeGovernor.h"
#include "SculptureEventBus.h"
#include "SculptureWorldNavigator.h"

#include <cmath>
#include <algorithm>
#include <numeric>
#include <cstdio>
#include <sstream>

namespace epi_sculpt {

SculpturePerformanceMonitor::SculpturePerformanceMonitor(PerformanceMonitorConfig config) noexcept
    : config_(config) {
    const std::size_t cap = config_.historyCapacity > 0U ? config_.historyCapacity : 120U;
    frameTimeHistory_.resize(cap, 16.6667f);
    sortedScratch_.resize(cap, 16.6667f);
    reset();
}

void SculpturePerformanceMonitor::setConfig(const PerformanceMonitorConfig& config) noexcept {
    config_ = config;
    if (config_.historyCapacity != frameTimeHistory_.size() && config_.historyCapacity > 0U) {
        frameTimeHistory_.assign(config_.historyCapacity, 16.6667f);
        sortedScratch_.resize(config_.historyCapacity, 16.6667f);
        historyHead_ = 0U;
        historyCount_ = 0U;
    }
    syncGovernorTarget();
    updateBudgetStatus();
}

void SculpturePerformanceMonitor::setTargetTier(TargetFrameRateTier tier) noexcept {
    config_.targetTier = tier;
    telemetry_.targetTier = tier;
    telemetry_.targetFps = tierTargetFps(tier);
    telemetry_.targetBudgetMs = tierBudgetMs(tier);
    syncGovernorTarget();
    updateBudgetStatus();
}

void SculpturePerformanceMonitor::cycleTargetTier() noexcept {
    auto nextTier = static_cast<std::uint8_t>(config_.targetTier) + 1U;
    if (nextTier > static_cast<std::uint8_t>(TargetFrameRateTier::Unlimited)) {
        nextTier = 0U;
    }
    setTargetTier(static_cast<TargetFrameRateTier>(nextTier));
}

void SculpturePerformanceMonitor::bindGovernor(SculptureStagedComputeGovernor* governor) noexcept {
    governor_ = governor;
    syncGovernorTarget();
}

void SculpturePerformanceMonitor::syncGovernorTarget() noexcept {
    if (governor_ != nullptr) {
        const float fps = targetFps();
        if (fps > 1.0f) {
            governor_->setTargetFrameRate(fps);
        }
    }
}

void SculpturePerformanceMonitor::reset() noexcept {
    telemetry_ = PerformanceTelemetry{};
    telemetry_.targetTier = config_.targetTier;
    telemetry_.targetFps = tierTargetFps(config_.targetTier);
    telemetry_.targetBudgetMs = tierBudgetMs(config_.targetTier);
    telemetry_.smoothedFrameTimeMs = telemetry_.targetBudgetMs > 0.0f ? telemetry_.targetBudgetMs : 16.6667f;
    telemetry_.smoothedFps = telemetry_.targetFps > 0.0f ? telemetry_.targetFps : 60.0f;
    telemetry_.minFrameTimeMs = telemetry_.smoothedFrameTimeMs;
    telemetry_.maxFrameTimeMs = telemetry_.smoothedFrameTimeMs;

    historyHead_ = 0U;
    historyCount_ = 0U;
    hasPreviousFrame_ = false;
    consecutiveHitchCount_ = 0U;
    stableHeadroomCount_ = 0U;
    updateBudgetStatus();
}

void SculpturePerformanceMonitor::beginFrame() noexcept {
    if (!enabled_) return;
    frameStartTime_ = Clock::now();
}

void SculpturePerformanceMonitor::markCpuWorkEnd() noexcept {
    if (!enabled_) return;
    const auto now = Clock::now();
    const float cpuMs = std::chrono::duration<float, std::milli>(now - frameStartTime_).count();
    telemetry_.lastCpuTimeMs = cpuMs;

    const float alpha = config_.smoothingAlpha;
    telemetry_.smoothedCpuTimeMs = (telemetry_.smoothedCpuTimeMs == 0.0f)
        ? cpuMs
        : (alpha * cpuMs + (1.0f - alpha) * telemetry_.smoothedCpuTimeMs);
}

void SculpturePerformanceMonitor::recordGpuLatency(float gpuMilliseconds) noexcept {
    if (!enabled_) return;
    telemetry_.lastGpuTimeMs = gpuMilliseconds;

    const float alpha = config_.smoothingAlpha;
    telemetry_.smoothedGpuTimeMs = (telemetry_.smoothedGpuTimeMs == 0.0f)
        ? gpuMilliseconds
        : (alpha * gpuMilliseconds + (1.0f - alpha) * telemetry_.smoothedGpuTimeMs);

    // Feed closed-loop GPU latency into Staged Compute Governor if bound
    if (governor_ != nullptr && config_.enableAutoGovernorPacing) {
        governor_->observeGpuLatency(gpuMilliseconds);
    }
}

void SculpturePerformanceMonitor::endFrame() noexcept {
    if (!enabled_) return;
    const auto now = Clock::now();

    if (hasPreviousFrame_) {
        const float deltaMs = std::chrono::duration<float, std::milli>(now - lastFrameEndTime_).count();
        recordFrameSample(deltaMs, telemetry_.lastCpuTimeMs, telemetry_.lastGpuTimeMs);
    } else {
        hasPreviousFrame_ = true;
    }
    lastFrameEndTime_ = now;
}

void SculpturePerformanceMonitor::recordFrameSample(float frameIntervalMs, float cpuMs, float gpuMs) noexcept {
    if (frameIntervalMs <= 0.001f) {
        frameIntervalMs = 0.001f;
    }

    telemetry_.totalFrames++;
    telemetry_.currentFrameTimeMs = frameIntervalMs;
    telemetry_.currentFps = 1000.0f / frameIntervalMs;
    telemetry_.lastCpuTimeMs = cpuMs;
    telemetry_.lastGpuTimeMs = gpuMs;

    // Exponential moving average smoothing
    const float alpha = config_.smoothingAlpha;
    if (telemetry_.totalFrames <= 1U) {
        telemetry_.smoothedFrameTimeMs = frameIntervalMs;
        telemetry_.smoothedFps = telemetry_.currentFps;
        telemetry_.minFrameTimeMs = frameIntervalMs;
        telemetry_.maxFrameTimeMs = frameIntervalMs;
    } else {
        telemetry_.smoothedFrameTimeMs = alpha * frameIntervalMs + (1.0f - alpha) * telemetry_.smoothedFrameTimeMs;
        telemetry_.smoothedFps = 1000.0f / std::max(telemetry_.smoothedFrameTimeMs, 0.001f);
        telemetry_.minFrameTimeMs = std::min(telemetry_.minFrameTimeMs, frameIntervalMs);
        telemetry_.maxFrameTimeMs = std::max(telemetry_.maxFrameTimeMs, frameIntervalMs);
    }

    // Insert into sliding ring buffer
    if (!frameTimeHistory_.empty()) {
        frameTimeHistory_[historyHead_] = frameIntervalMs;
        historyHead_ = (historyHead_ + 1U) % frameTimeHistory_.size();
        if (historyCount_ < frameTimeHistory_.size()) {
            historyCount_++;
        }
    }

    // Hitch detection & budget headroom
    const float budget = telemetry_.targetBudgetMs;
    if (budget > 0.0f) {
        telemetry_.headroomRatio = budget / frameIntervalMs;
        telemetry_.headroomMs = budget - frameIntervalMs;
        telemetry_.budgetExceeded = (frameIntervalMs > budget);

        if (frameIntervalMs > (budget * config_.hitchThresholdFactor)) {
            telemetry_.hitchCount++;
            consecutiveHitchCount_++;
            stableHeadroomCount_ = 0U;
        } else {
            consecutiveHitchCount_ = 0U;
            if (telemetry_.headroomRatio >= 1.15f) {
                stableHeadroomCount_++;
            } else {
                stableHeadroomCount_ = 0U;
            }
        }
    } else {
        telemetry_.headroomRatio = 1.0f;
        telemetry_.headroomMs = 0.0f;
        telemetry_.budgetExceeded = false;
    }

    // Recalculate statistical metrics (standard deviation jitter, 1% low)
    recalculateStatistics();
    updateBudgetStatus();

    // Closed-loop governor auto-adaptation
    if (governor_ != nullptr && config_.enableAutoGovernorPacing && budget > 0.0f) {
        // If consecutive hitches occur under pressure, downscale governor tier
        if (consecutiveHitchCount_ >= config_.consecutiveHitchesForThrottle) {
            auto currentScale = governor_->resolutionScale();
            if (currentScale == StageResolutionScale::Full) {
                governor_->setResolutionScale(StageResolutionScale::Half);
                consecutiveHitchCount_ = 0U;
            } else if (currentScale == StageResolutionScale::Half) {
                governor_->setResolutionScale(StageResolutionScale::Quarter);
                consecutiveHitchCount_ = 0U;
            } else if (currentScale == StageResolutionScale::Quarter) {
                governor_->setResolutionScale(StageResolutionScale::Eighth);
                consecutiveHitchCount_ = 0U;
            }
        }
        // If sustained headroom exists, promote back towards Full
        else if (stableHeadroomCount_ >= config_.stableHeadroomFramesForRecovery) {
            auto currentScale = governor_->resolutionScale();
            if (currentScale == StageResolutionScale::Eighth) {
                governor_->setResolutionScale(StageResolutionScale::Quarter);
                stableHeadroomCount_ = 0U;
            } else if (currentScale == StageResolutionScale::Quarter) {
                governor_->setResolutionScale(StageResolutionScale::Half);
                stableHeadroomCount_ = 0U;
            } else if (currentScale == StageResolutionScale::Half) {
                governor_->setResolutionScale(StageResolutionScale::Full);
                stableHeadroomCount_ = 0U;
            }
        }
    }

    // Post decoupled metrics event to EventBus if connected
    if (eventBus_ != nullptr && (telemetry_.totalFrames % 15U == 0U)) {
        eventBus_->post(EventPerformanceMetrics{
            .fps = telemetry_.smoothedFps,
            .frameTimeMs = telemetry_.smoothedFrameTimeMs,
            .gpuTimeMs = telemetry_.smoothedGpuTimeMs,
            .jitterMs = telemetry_.jitterStdDevMs,
            .low1PercentFps = telemetry_.low1PercentFps,
            .hitchCount = telemetry_.hitchCount,
            .budgetExceeded = telemetry_.budgetExceeded
        });
    }
}

void SculpturePerformanceMonitor::recalculateStatistics() noexcept {
    if (historyCount_ == 0U) {
        telemetry_.jitterStdDevMs = 0.0f;
        telemetry_.low1PercentFps = telemetry_.currentFps;
        return;
    }

    // Compute mean and variance
    double sum = 0.0;
    for (std::size_t i = 0U; i < historyCount_; ++i) {
        sum += frameTimeHistory_[i];
    }
    const double mean = sum / static_cast<double>(historyCount_);

    double varianceSum = 0.0;
    for (std::size_t i = 0U; i < historyCount_; ++i) {
        const double diff = frameTimeHistory_[i] - mean;
        varianceSum += diff * diff;
    }
    telemetry_.jitterStdDevMs = static_cast<float>(std::sqrt(varianceSum / static_cast<double>(historyCount_)));

    // Calculate 1% low: find the 99th percentile slowest frame time in the history buffer using zero-allocation scratch
    std::copy_n(frameTimeHistory_.begin(), historyCount_, sortedScratch_.begin());
    const std::size_t p99Index = static_cast<std::size_t>(std::floor(static_cast<double>(historyCount_ - 1U) * 0.99));
    std::nth_element(sortedScratch_.begin(), sortedScratch_.begin() + p99Index, sortedScratch_.begin() + historyCount_);
    const float p99TimeMs = sortedScratch_[p99Index];
    telemetry_.low1PercentFps = 1000.0f / std::max(p99TimeMs, 0.001f);
}

void SculpturePerformanceMonitor::updateBudgetStatus() noexcept {
    if (config_.targetTier == TargetFrameRateTier::Unlimited) {
        telemetry_.budgetStatus = "Unlimited";
        return;
    }

    if (consecutiveHitchCount_ > 0U) {
        telemetry_.budgetStatus = "Hitching";
    } else if (telemetry_.smoothedFrameTimeMs > telemetry_.targetBudgetMs) {
        telemetry_.budgetStatus = "Throttled";
    } else if (telemetry_.headroomRatio >= 1.20f) {
        telemetry_.budgetStatus = "Optimal";
    } else {
        telemetry_.budgetStatus = "Nominal";
    }
}

std::string SculpturePerformanceMonitor::formatHud() const {
    char buf[256];
    const auto& t = telemetry_;
    if (config_.targetTier == TargetFrameRateTier::Unlimited) {
        std::snprintf(buf, sizeof(buf),
                      "%.1f FPS (%.1fms \xc2\xb1%.1fms, 1%% low: %.1f) [GPU: %.1fms, CPU: %.1fms] | TARGET: Uncapped",
                      t.smoothedFps, t.smoothedFrameTimeMs, t.jitterStdDevMs, t.low1PercentFps,
                      t.smoothedGpuTimeMs, t.smoothedCpuTimeMs);
    } else {
        std::snprintf(buf, sizeof(buf),
                      "%.1f FPS (%.1fms \xc2\xb1%.1fms, Headroom: %+0.1fms, 1%% low: %.1f) [GPU: %.1fms, CPU: %.1fms] | TARGET: %.0f FPS (%s)",
                      t.smoothedFps, t.smoothedFrameTimeMs, t.jitterStdDevMs, t.headroomMs, t.low1PercentFps,
                      t.smoothedGpuTimeMs, t.smoothedCpuTimeMs,
                      t.targetFps, t.budgetStatus.c_str());
    }
    return std::string(buf);
}

std::string SculpturePerformanceMonitor::formatSparkline(std::size_t width) const {
    if (historyCount_ == 0U || width == 0U) {
        return "";
    }

    const std::size_t sampleCount = std::min(width, historyCount_);
    static const char* const kGlyphs[] = {" ", "\xe2\x96\x81", "\xe2\x96\x82", "\xe2\x96\x83", "\xe2\x96\x84", "\xe2\x96\x85", "\xe2\x96\x86", "\xe2\x96\x87", "\xe2\x96\x88"};
    const float ceiling = (telemetry_.targetBudgetMs > 0.0f) ? telemetry_.targetBudgetMs * 1.5f : 33.333f;

    std::string sparkline;
    sparkline.reserve(sampleCount * 4U);

    for (std::size_t i = 0U; i < sampleCount; ++i) {
        // Read samples backwards from newest to oldest
        const std::size_t idx = (historyHead_ + frameTimeHistory_.size() - sampleCount + i) % frameTimeHistory_.size();
        const float val = frameTimeHistory_[idx];
        const float norm = std::clamp(val / ceiling, 0.0f, 1.0f);
        const auto glyphIdx = static_cast<std::size_t>(std::round(norm * 8.0f));
        sparkline += kGlyphs[std::min(glyphIdx, std::size_t{8U})];
    }
    return sparkline;
}

// -----------------------------------------------------------------------------
// INavigatorObserver Implementation
// -----------------------------------------------------------------------------
void SculpturePerformanceMonitor::onWorldSelected(std::size_t, const WorldCatalogEntry&, float transitionDuration) {
    if (transitionDuration > 0.0f) {
        inTransition_ = true;
    }
}

void SculpturePerformanceMonitor::onWorldTransitionStarted(std::size_t, std::size_t, const WorldCatalogEntry&, float) {
    inTransition_ = true;
}

void SculpturePerformanceMonitor::onWorldTransitionCompleted(std::size_t, const WorldCatalogEntry&) {
    inTransition_ = false;
    // Reset consecutive hitch count upon transition completion to prevent warm-up transient penalties
    consecutiveHitchCount_ = 0U;
    stableHeadroomCount_ = 0U;
}

void SculpturePerformanceMonitor::onActionDispatched(NavigatorAction action, float, std::string_view) {
    // Reset hitch counter on large parameter resets or world changes
    if (action == NavigatorAction::NextWorld || action == NavigatorAction::PrevWorld || action == NavigatorAction::ResetParameters) {
        consecutiveHitchCount_ = 0U;
    }
}

} // namespace epi_sculpt
