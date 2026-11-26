// Renderer-neutral aggregation for bounded dedicated-world profile sessions.
//
// This observes immutable delivery metadata only. It neither changes OpenCL
// generation nor makes presentation/quality decisions, so performance evidence
// cannot affect the visual identity being measured.

#pragma once

#include "SculptureWorldPerformance.h"
#include "SculptureWorldRecipe.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <utility>

namespace epi_sculpt {

struct WorldProfileIdentity {
    std::string canonicalPresetId;
    std::string sourceRevision;
    WorldRecipe requestedRecipe{};
    WorldRecipe effectiveRecipe{};
    std::string openClDeviceId;
    std::string vulkanDeviceId;
    std::string qualityTier;
    std::string viewConfiguration;

    [[nodiscard]] bool matches(const WorldProfileIdentity& other) const noexcept {
        return canonicalPresetId == other.canonicalPresetId
            && sourceRevision == other.sourceRevision
            && recipesMatch(requestedRecipe, other.requestedRecipe)
            && recipesMatch(effectiveRecipe, other.effectiveRecipe)
            && openClDeviceId == other.openClDeviceId
            && vulkanDeviceId == other.vulkanDeviceId
            && qualityTier == other.qualityTier
            && viewConfiguration == other.viewConfiguration;
    }

private:
    [[nodiscard]] static constexpr bool recipesMatch(
        const WorldRecipe& lhs, const WorldRecipe& rhs) noexcept {
        return lhs.generator == rhs.generator
            && lhs.primitive == rhs.primitive
            && lhs.delivery == rhs.delivery
            && lhs.instanceBudget == rhs.instanceBudget
            && lhs.generatorHz == rhs.generatorHz
            && lhs.materialBandCount == rhs.materialBandCount
            && lhs.uses4DProjection == rhs.uses4DProjection
            && lhs.mirrorTopology == rhs.mirrorTopology;
    }
};

// A timing remains unavailable until at least one finite, non-negative sample
// exists. This avoids turning unsupported profiling into a misleading 0 ms.
struct WorldProfileTiming {
    static constexpr float kUnavailableMilliseconds = -1.0f;

    std::uint32_t sampleCount = 0U;
    float minimumMilliseconds = kUnavailableMilliseconds;
    float meanMilliseconds = kUnavailableMilliseconds;
    float maximumMilliseconds = kUnavailableMilliseconds;

    void observe(float milliseconds) noexcept {
        if (!std::isfinite(milliseconds) || milliseconds < 0.0f) return;

        if (sampleCount == 0U) {
            minimumMilliseconds = milliseconds;
            meanMilliseconds = milliseconds;
            maximumMilliseconds = milliseconds;
            sampleCount = 1U;
            return;
        }

        minimumMilliseconds = std::min(minimumMilliseconds, milliseconds);
        maximumMilliseconds = std::max(maximumMilliseconds, milliseconds);
        meanMilliseconds += (milliseconds - meanMilliseconds)
            / static_cast<float>(sampleCount + 1U);
        ++sampleCount;
    }
};

struct WorldProfileSummary {
    WorldProfileIdentity identity;
    // A producer loss ends the measurement without relabelling partial data as
    // a successful comparable run.
    bool interrupted = false;
    std::uint32_t acceptedGenerationCount = 0U;
    std::uint32_t firstGeneration = 0U;
    std::uint32_t lastGeneration = 0U;
    WorldProfileTiming queueToDelivery;
    WorldProfileTiming kernel;
    WorldProfileTiming readback;
    WorldProfileTiming bridgeRebuild;
    WorldProfileTiming presentationSubmitCpu;
};

// This remains an operator-facing recommendation, never an automatic quality
// mutation. Queue-to-delivery overlaps the three direct stages, so it is
// reported separately and deliberately excluded from the bottleneck total.
enum class WorldProfileAssessmentStatus : std::uint8_t {
    InsufficientGenerations,
    IncompleteTiming,
    Ready,
};

enum class WorldProfileBottleneck : std::uint8_t {
    None,
    OpenClKernel,
    HostReadback,
    VulkanBridge,
    VulkanSubmission,
};

enum class WorldProfileRecommendation : std::uint8_t {
    CollectMoreEvidence,
    InvestigateOpenClKernel,
    InvestigateStagedTransport,
    InvestigatePresentationBatching,
    InvestigatePresentationSubmission,
};

struct WorldProfileAssessment {
    static constexpr std::uint32_t kMinimumComparableSamples = 8U;

    WorldProfileAssessmentStatus status =
        WorldProfileAssessmentStatus::InsufficientGenerations;
    WorldProfileBottleneck bottleneck = WorldProfileBottleneck::None;
    WorldProfileRecommendation recommendation =
        WorldProfileRecommendation::CollectMoreEvidence;
    float directStageMeanMilliseconds = WorldProfileTiming::kUnavailableMilliseconds;
    float bottleneckMeanMilliseconds = WorldProfileTiming::kUnavailableMilliseconds;
    float bottleneckFraction = 0.0f;
};

[[nodiscard]] inline constexpr std::string_view name(
    WorldProfileAssessmentStatus value) noexcept {
    switch (value) {
    case WorldProfileAssessmentStatus::InsufficientGenerations: return "insufficient_generations";
    case WorldProfileAssessmentStatus::IncompleteTiming: return "incomplete_timing";
    case WorldProfileAssessmentStatus::Ready: return "ready";
    }
    return "unknown";
}

[[nodiscard]] inline constexpr std::string_view name(
    WorldProfileBottleneck value) noexcept {
    switch (value) {
    case WorldProfileBottleneck::None: return "none";
    case WorldProfileBottleneck::OpenClKernel: return "opencl_kernel";
    case WorldProfileBottleneck::HostReadback: return "host_readback";
    case WorldProfileBottleneck::VulkanBridge: return "vulkan_bridge";
    case WorldProfileBottleneck::VulkanSubmission: return "vulkan_submission";
    }
    return "unknown";
}

[[nodiscard]] inline constexpr std::string_view name(
    WorldProfileRecommendation value) noexcept {
    switch (value) {
    case WorldProfileRecommendation::CollectMoreEvidence: return "collect_more_evidence";
    case WorldProfileRecommendation::InvestigateOpenClKernel: return "investigate_opencl_kernel";
    case WorldProfileRecommendation::InvestigateStagedTransport: return "investigate_staged_transport";
    case WorldProfileRecommendation::InvestigatePresentationBatching: return "investigate_presentation_batching";
    case WorldProfileRecommendation::InvestigatePresentationSubmission: return "investigate_presentation_submission";
    }
    return "unknown";
}

[[nodiscard]] inline WorldProfileAssessment assessWorldProfile(
    const WorldProfileSummary& summary) noexcept {
    WorldProfileAssessment assessment{};
    if (summary.acceptedGenerationCount
        < WorldProfileAssessment::kMinimumComparableSamples) {
        return assessment;
    }

    const auto comparable = [](const WorldProfileTiming& timing) noexcept {
        return timing.sampleCount >= WorldProfileAssessment::kMinimumComparableSamples
            && std::isfinite(timing.meanMilliseconds)
            && timing.meanMilliseconds >= 0.0f;
    };
    if (!comparable(summary.kernel) || !comparable(summary.readback)
        || !comparable(summary.bridgeRebuild)
        || !comparable(summary.presentationSubmitCpu)) {
        assessment.status = WorldProfileAssessmentStatus::IncompleteTiming;
        return assessment;
    }

    assessment.status = WorldProfileAssessmentStatus::Ready;
    assessment.directStageMeanMilliseconds = summary.kernel.meanMilliseconds
        + summary.readback.meanMilliseconds + summary.bridgeRebuild.meanMilliseconds
        + summary.presentationSubmitCpu.meanMilliseconds;
    const WorldProfileTiming* dominant = &summary.kernel;
    assessment.bottleneck = WorldProfileBottleneck::OpenClKernel;
    assessment.recommendation = WorldProfileRecommendation::InvestigateOpenClKernel;
    if (summary.readback.meanMilliseconds > dominant->meanMilliseconds) {
        dominant = &summary.readback;
        assessment.bottleneck = WorldProfileBottleneck::HostReadback;
        assessment.recommendation = WorldProfileRecommendation::InvestigateStagedTransport;
    }
    if (summary.bridgeRebuild.meanMilliseconds > dominant->meanMilliseconds) {
        dominant = &summary.bridgeRebuild;
        assessment.bottleneck = WorldProfileBottleneck::VulkanBridge;
        assessment.recommendation =
            WorldProfileRecommendation::InvestigatePresentationBatching;
    }
    if (summary.presentationSubmitCpu.meanMilliseconds > dominant->meanMilliseconds) {
        dominant = &summary.presentationSubmitCpu;
        assessment.bottleneck = WorldProfileBottleneck::VulkanSubmission;
        assessment.recommendation =
            WorldProfileRecommendation::InvestigatePresentationSubmission;
    }
    assessment.bottleneckMeanMilliseconds = dominant->meanMilliseconds;
    assessment.bottleneckFraction = assessment.directStageMeanMilliseconds > 0.0f
        ? assessment.bottleneckMeanMilliseconds / assessment.directStageMeanMilliseconds
        : 0.0f;
    return assessment;
}

// A session is deliberately bounded both by callers' duration and by its
// fixed generation ledger. The ledger rejects duplicate and stale delivery
// metadata without allocating in the observation path.
class SculptureWorldProfile {
public:
    static constexpr std::size_t kGenerationCapacity = 1024U;

    explicit SculptureWorldProfile(WorldProfileIdentity identity = {})
        : identity_(std::move(identity)) {
        summary_.identity = identity_;
    }

    void reset(WorldProfileIdentity identity) {
        identity_ = std::move(identity);
        observedGenerations_.fill(0U);
        observedCount_ = 0U;
        summary_ = {};
        summary_.identity = identity_;
    }

    // Both records must describe the same accepted generation. The bridge
    // record may carry unavailable timing, but its generation must be valid:
    // a partial producer-only observation is not an end-to-end profile sample.
    [[nodiscard]] bool observe(const WorldGenerationMetrics& generation,
                               const WorldBridgeMetrics& bridge,
                               const WorldPresentationMetrics* presentation = nullptr) noexcept {
        if (!generation.matches(generation.generation)
            || !bridge.matches(generation.generation)
            || !isStrictlyNew(generation.generation)
            || observedCount_ == observedGenerations_.size()) {
            return false;
        }

        observedGenerations_[observedCount_++] = generation.generation;
        if (summary_.acceptedGenerationCount == 0U) {
            summary_.firstGeneration = generation.generation;
        }
        ++summary_.acceptedGenerationCount;
        summary_.lastGeneration = generation.generation;
        summary_.queueToDelivery.observe(generation.queueToDeliveryMilliseconds);
        summary_.kernel.observe(generation.kernelMilliseconds);
        summary_.readback.observe(generation.readbackMilliseconds);
        summary_.bridgeRebuild.observe(bridge.rebuildMilliseconds);
        if (presentation != nullptr && presentation->matches(generation.generation)) {
            summary_.presentationSubmitCpu.observe(presentation->submitCpuMilliseconds);
        }
        return true;
    }

    [[nodiscard]] const WorldProfileSummary& summary() const noexcept {
        return summary_;
    }

    void markInterrupted() noexcept { summary_.interrupted = true; }

private:
    [[nodiscard]] bool isStrictlyNew(std::uint32_t generation) const noexcept {
        if (generation == 0U) return false;
        if (observedCount_ == 0U) return true;

        // Generation counters are uint32_t; signed subtraction preserves the
        // producer's usual wrap-safe ordering rule.
        return static_cast<std::int32_t>(
            generation - summary_.lastGeneration) > 0;
    }

    WorldProfileIdentity identity_;
    std::array<std::uint32_t, kGenerationCapacity> observedGenerations_{};
    std::size_t observedCount_ = 0U;
    WorldProfileSummary summary_{};
};

} // namespace epi_sculpt
