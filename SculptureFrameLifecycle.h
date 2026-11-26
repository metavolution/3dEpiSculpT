#pragma once

#include <array>
#include <cstdint>
#include <stdexcept>
#include <string_view>

namespace epi_sculpt {

// The flagship lifecycle is a correctness contract, not a plugin surface.
// Keeping the seven phases as a typed fixed sequence removes per-frame string
// lookup/type erasure and, more importantly, makes reordering/disabling an
// authority-changing operation that must be expressed in source review.
enum class SculptureFrameStage : std::uint8_t {
    TimeAndInput = 0,
    AdvanceSimulation,
    ParameterComposition,
    FreezeFramePlan,
    Production,
    AuxiliaryFeedback,
    Presentation,
    Count,
};

inline constexpr std::array<SculptureFrameStage, 7> kSculptureFrameStageOrder{
    SculptureFrameStage::TimeAndInput,
    SculptureFrameStage::AdvanceSimulation,
    SculptureFrameStage::ParameterComposition,
    SculptureFrameStage::FreezeFramePlan,
    SculptureFrameStage::Production,
    SculptureFrameStage::AuxiliaryFeedback,
    SculptureFrameStage::Presentation,
};

[[nodiscard]] constexpr std::string_view sculptureFrameStageName(
    SculptureFrameStage stage) noexcept {
    switch (stage) {
        case SculptureFrameStage::TimeAndInput: return "TimeAndInput";
        case SculptureFrameStage::AdvanceSimulation: return "AdvanceSimulation";
        case SculptureFrameStage::ParameterComposition: return "ParameterComposition";
        case SculptureFrameStage::FreezeFramePlan: return "FreezeFramePlan";
        case SculptureFrameStage::Production: return "Production";
        case SculptureFrameStage::AuxiliaryFeedback: return "AuxiliaryFeedback";
        case SculptureFrameStage::Presentation: return "Presentation";
        case SculptureFrameStage::Count: break;
    }
    return "None";
}

// Device-free execution trace carried by one application evaluation. It is
// intentionally tiny: diagnostics can identify the failing phase without a
// dynamic pipeline object becoming another scheduler or mutable source of
// stage order.
struct SculptureFrameLifecycleTrace final {
    SculptureFrameStage activeStage = SculptureFrameStage::Count;
    SculptureFrameStage failedStage = SculptureFrameStage::Count;
    std::uint8_t completedStageCount = 0U;

    void begin(SculptureFrameStage stage) {
        if (activeStage != SculptureFrameStage::Count ||
            completedStageCount >= kSculptureFrameStageOrder.size() ||
            kSculptureFrameStageOrder[completedStageCount] != stage) {
            throw std::logic_error("Sculpture frame lifecycle stage order violated");
        }
        activeStage = stage;
    }

    void complete(SculptureFrameStage stage) {
        if (activeStage != stage) {
            throw std::logic_error("Sculpture frame lifecycle completed the wrong stage");
        }
        activeStage = SculptureFrameStage::Count;
        ++completedStageCount;
    }

    void fail(SculptureFrameStage stage) noexcept {
        failedStage = stage;
        activeStage = SculptureFrameStage::Count;
    }

    [[nodiscard]] bool finished() const noexcept {
        return completedStageCount == kSculptureFrameStageOrder.size() &&
               failedStage == SculptureFrameStage::Count;
    }
};

} // namespace epi_sculpt
