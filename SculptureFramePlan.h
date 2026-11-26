#pragma once

#include "SculptureProductionRoute.h"
#include "SculptureTemporalReset.h"
#include "SculptureProgramSchema.h"
#include "SculptureCompositionObservation.h"
#include "SculptureWorldRecipe.h"
#include "SculptureStereoRig.h"
#include "SculptureVolumePresentationState.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>

namespace epi { struct VisualMode; }

namespace epi_sculpt {

struct SculptureCapturedWorldRecipe;
// Keep the retained owner opaque here: the frame contract needs no mode-codec
// or generated-math headers. The supplied canonical pointers belong to this owner.
struct SculptureCanonicalFeedbackSource;

// Immutable semantic contract for one display-frame evaluation.
//
// The application is intentionally allowed to keep mutable live state while
// processing input and composing parameters. Once FreezeFramePlan completes,
// every compute and presentation stage for that frame must consume this frozen
// value instead of reaching back into mutable application members. This keeps
// route choice, program parameters, world framing, quality-resolved recipe,
// and presentation policy coherent even as later work is queued asynchronously.
class SculptureFramePlan final {
public:
    struct PresentationState {
        bool cinematic = false;
        bool vrEnabled = false;
        bool temporalActive = false;
        float cinematicTime = 0.0f;
        SculptureStereoRig stereoRig{};
        SculptureVolumePresentationState volumePresentation{};
    };

    SculptureFramePlan(
        SculptureProgramParams params,
        const epi::VisualMode* canonicalFeedbackModeBase,
        const SculptureProgramParams* canonicalFeedbackProjectionBaseline,
        SculptureWorldState worldState,
        SculptureProductionDecision production,
        WorldRecipe requestedWorldRecipe,
        WorldRecipe effectiveWorldRecipe,
        float effectiveWorldUpdateHz,
        std::size_t presetIndex,
        SculpturePresentationOwnership presentationOwnership,
        std::uint64_t compactPresentationLeaseEpoch,
        std::uint64_t frameSequence,
        PresentationState presentation,
        SculptureTemporalResetIntent temporalReset = {},
        const SculptureCapturedWorldRecipe* capturedWorldRecipe = nullptr,
        std::shared_ptr<const SculptureCanonicalFeedbackSource> canonicalSource = {},
        std::uint64_t seedImageSelectionRevision = 0U,
        SculptureCompositionObservation composition = {}) noexcept
        : params_(params),
          canonicalFeedbackModeBase_(canonicalFeedbackModeBase),
          canonicalFeedbackProjectionBaseline_(canonicalFeedbackProjectionBaseline),
          worldState_(worldState),
          production_(production),
          requestedWorldRecipe_(requestedWorldRecipe),
          effectiveWorldRecipe_(effectiveWorldRecipe),
          effectiveWorldUpdateHz_(effectiveWorldUpdateHz),
          presetIndex_(presetIndex),
          presentationOwnership_(presentationOwnership),
          compactPresentationLeaseEpoch_(compactPresentationLeaseEpoch),
          frameSequence_(frameSequence),
          presentation_(presentation),
          temporalReset_(temporalReset),
          capturedWorldRecipe_(capturedWorldRecipe),
          canonicalSource_(std::move(canonicalSource)),
          seedImageSelectionRevision_(seedImageSelectionRevision),
          composition_(composition) {}

    [[nodiscard]] const SculptureProgramParams& params() const noexcept {
        return params_;
    }

    [[nodiscard]] const SculptureTemporalResetIntent& temporalReset() const noexcept {
        return temporalReset_;
    }

    [[nodiscard]] SculptureProgramParams paramsForProduction(
        SculptureProductionRoute route) const noexcept {
        SculptureProgramParams result = params_;
        // Legacy lightweight callers without a command token retain their
        // explicit parameter bit. Application frames use contribution identity.
        if (temporalReset_.sequence != 0U) result.temporalReset = temporalReset_.pending(route);
        return result;
    }

    [[nodiscard]] SculptureProgramParams paramsForCadence(
        SculptureProductionRoute /* route */) const noexcept {
        auto result = params_;
        // A token admits its command through force-submit, independently of
        // parameter dirtiness. Acknowledgment must not create a second step
        // when the producer payload's reset bit clears. The cadence view is
        // therefore route-independent; raw callers keep their original bit.
        if (temporalReset_.sequence != 0U) result.temporalReset = false;
        return result;
    }

    // Restored sources retain one immutable owner, including their baseline.
    // Replacing a live capture cannot invalidate an already frozen plan.
    // The ordinary preset path remains borrowed: preset-library
    // mutation is confined to the pre-freeze input stage, so these references
    // remain stable through this frame's later stages and synchronous submit
    // settlement. No VisualMode copy/allocation is incurred for display-only
    // frames; production resolves a copy only when canonical work is due.
    [[nodiscard]] const epi::VisualMode* canonicalFeedbackModeBase() const noexcept {
        return canonicalFeedbackModeBase_;
    }
    [[nodiscard]] const SculptureProgramParams* canonicalFeedbackProjectionBaseline() const noexcept {
        return canonicalFeedbackProjectionBaseline_;
    }
    [[nodiscard]] const SculptureCanonicalFeedbackSource* capturedCanonicalSource() const noexcept {
        return canonicalSource_.get();
    }
    [[nodiscard]] std::uint64_t seedImageSelectionRevision() const noexcept {
        return seedImageSelectionRevision_;
    }

    [[nodiscard]] const SculptureWorldState& worldState() const noexcept {
        return worldState_;
    }

    [[nodiscard]] SculptureProductionDecision production() const noexcept {
        return production_;
    }

    [[nodiscard]] const WorldRecipe& requestedWorldRecipe() const noexcept {
        return requestedWorldRecipe_;
    }

    [[nodiscard]] const WorldRecipe& effectiveWorldRecipe() const noexcept {
        return effectiveWorldRecipe_;
    }

    [[nodiscard]] float effectiveWorldUpdateHz() const noexcept {
        return effectiveWorldUpdateHz_;
    }

    [[nodiscard]] std::size_t presetIndex() const noexcept {
        return presetIndex_;
    }

    // The application keeps this value alive through frame settlement, like
    // the borrowed canonical mode above. Captures copy its identity immediately.
    [[nodiscard]] const SculptureCapturedWorldRecipe* capturedWorldRecipe() const noexcept {
        return capturedWorldRecipe_;
    }

    [[nodiscard]] SculpturePresentationOwnership presentationOwnership() const noexcept {
        return presentationOwnership_;
    }

    [[nodiscard]] std::uint64_t compactPresentationLeaseEpoch() const noexcept {
        return compactPresentationLeaseEpoch_;
    }

    [[nodiscard]] std::uint64_t frameSequence() const noexcept {
        return frameSequence_;
    }

    [[nodiscard]] const PresentationState& presentation() const noexcept {
        return presentation_;
    }

    // Observed authoring admission for this intent, not GPU acceptance or a
    // guarantee about later composition stages. No live owner is consulted.
    [[nodiscard]] const SculptureCompositionObservation& composition() const noexcept {
        return composition_;
    }

private:
    SculptureProgramParams params_{};
    const epi::VisualMode* canonicalFeedbackModeBase_ = nullptr;
    const SculptureProgramParams* canonicalFeedbackProjectionBaseline_ = nullptr;
    SculptureWorldState worldState_{};
    SculptureProductionDecision production_{};
    WorldRecipe requestedWorldRecipe_{};
    WorldRecipe effectiveWorldRecipe_{};
    float effectiveWorldUpdateHz_ = 0.0f;
    std::size_t presetIndex_ = 0U;
    SculpturePresentationOwnership presentationOwnership_ =
        SculpturePresentationOwnership::CompactInstances;
    std::uint64_t compactPresentationLeaseEpoch_ = 0U;
    std::uint64_t frameSequence_ = 0U;
    PresentationState presentation_{};
    SculptureTemporalResetIntent temporalReset_{};
    const SculptureCapturedWorldRecipe* capturedWorldRecipe_ = nullptr;
    std::shared_ptr<const SculptureCanonicalFeedbackSource> canonicalSource_;
    std::uint64_t seedImageSelectionRevision_ = 0U;
    SculptureCompositionObservation composition_{};
};

} // namespace epi_sculpt
