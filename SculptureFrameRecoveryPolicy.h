#pragma once

#include "SculptureFrameLifecycle.h"
#include "epi/core/execution_resilience.hpp"

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace epi_sculpt {

// Commit facts available after an exceptional frame has unwound. These are
// observational facts only: they never own or roll back producer/native work.
struct SculptureFrameCommitFacts final {
    SculptureFrameStage failedStage = SculptureFrameStage::Count;
    bool framePlanFrozen = false;
    std::size_t scheduledProductions = 0U;
    std::size_t acceptedProductions = 0U;
    std::size_t publishedProductions = 0U;
    bool presentationSettled = false;
    bool presentationSubmitted = false;

    [[nodiscard]] constexpr bool mathematicsCommitted() const noexcept {
        return acceptedProductions != 0U;
    }

    [[nodiscard]] constexpr bool publicationCommitted() const noexcept {
        return publishedProductions != 0U;
    }
};

enum class SculptureFrameFailureDomain : std::uint8_t {
    SemanticPreparation,
    Production,
    Observation,
    Unknown,
};

[[nodiscard]] constexpr SculptureFrameFailureDomain sculptureFrameFailureDomain(
    SculptureFrameStage stage) noexcept {
    switch (stage) {
    case SculptureFrameStage::TimeAndInput:
    case SculptureFrameStage::AdvanceSimulation:
    case SculptureFrameStage::ParameterComposition:
    case SculptureFrameStage::FreezeFramePlan:
        return SculptureFrameFailureDomain::SemanticPreparation;
    case SculptureFrameStage::Production:
        return SculptureFrameFailureDomain::Production;
    case SculptureFrameStage::AuxiliaryFeedback:
    case SculptureFrameStage::Presentation:
        return SculptureFrameFailureDomain::Observation;
    case SculptureFrameStage::Count:
        return SculptureFrameFailureDomain::Unknown;
    }
    return SculptureFrameFailureDomain::Unknown;
}

[[nodiscard]] constexpr std::string_view name(
    SculptureFrameFailureDomain domain) noexcept {
    switch (domain) {
    case SculptureFrameFailureDomain::SemanticPreparation: return "semantic-preparation";
    case SculptureFrameFailureDomain::Production: return "production";
    case SculptureFrameFailureDomain::Observation: return "observation";
    case SculptureFrameFailureDomain::Unknown: return "unknown";
    }
    return "unknown";
}

// Generic exceptions do not prove which semantic/scientific owner is invalid.
// Retry thresholds may pace repeated failure and eventually stop the session,
// but must not fabricate reset authority. Typed native faults are handled by
// PresentationDeviceHost before this policy is consulted.
struct SculptureGenericRecoveryDecision final {
    SculptureFrameFailureDomain domain = SculptureFrameFailureDomain::Unknown;
    bool stopSession = false;
    bool preserveSemanticState = true;
    bool preserveAcceptedMathematics = true;
    bool preservePublishedResources = true;
};

[[nodiscard]] constexpr SculptureGenericRecoveryDecision decideSculptureGenericRecovery(
    const SculptureFrameCommitFacts& facts,
    epi::core::FailureEscalation escalation) noexcept {
    return {
        .domain = sculptureFrameFailureDomain(facts.failedStage),
        .stopSession = escalation == epi::core::FailureEscalation::CircuitBreak,
        .preserveSemanticState = true,
        .preserveAcceptedMathematics = true,
        .preservePublishedResources = true,
    };
}

} // namespace epi_sculpt
