#pragma once

#include "SculptureCausalExecutionGraph.h"
#include "SculptureCaptureIdentity.h"
#include "SculptureFieldCaptureEvidence.h"
#include "SculptureFramePlan.h"
#include "SculptureCompositionObservation.h"
#include "SculptureAudioModulation.h"
#include "SculptureFrameLifecycle.h"
#include "SculptureFrameProducer.h"
#include "SculpturePresentationReceipt.h"
#include "SculptureProducerCadence.h"
#include "SculptureProductionSchedule.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>

namespace epi_sculpt {

class SculptureCanonicalPublicationInput;

// A queued readback can outlive its originating display frame. This one shared
// envelope carries copied facts and immutable semantic input, never GPU owners.
// Allocate it only for an actual snapshot request, not for every display tick.
struct SculptureFrameCaptureObservation final {
    explicit SculptureFrameCaptureObservation(SculptureCaptureFrameIdentity frame) noexcept
        : identity(frame) {}
    SculptureCaptureIdentity identity;
    SculpturePresentationReceipt presentation{};
    std::optional<SculptureFieldCaptureCandidate> fieldCandidate;
    bool fieldAccepted = false;
    bool mono = false;
    std::optional<SculptureCausalExecutionSnapshot> causal{};
    std::shared_ptr<const SculptureCanonicalPublicationInput> canonicalPublicationInput;
};

// Per-evaluation data passed between the application's seven phases. This is
// not another scheduler: SculptureApplication remains the single state owner.
struct SculptureFrameContext final {
    SculptureFrameLifecycleTrace lifecycle{};
    float deltaSeconds = 0.0f;
    float simulationDelta = 0.0f;
    // Actual checked stage results collected locally, then copied at freeze.
    SculptureCompositionObservation composition{};
    // FreezeFramePlan freezes semantic intent; downstream stages consume it.
    std::optional<SculptureFramePlan> framePlan;
    // Owned observation from this evaluation, sealed before semantic freeze.
    // Presentation samples this value, never the next mutable control state.
    std::optional<SculptureAcousticMistObservation> acousticMist;
    SculptureProductionScheduleSet productionSchedules{};
    // Canonical admission is prepared in Production but settled during Vulkan
    // recording. Keep its cadence-specific proposal out of the generic schedule.
    std::optional<SculptureProducerCadenceProposal<SculptureProgramParams>>
        canonicalCadenceProposal;
    const gpu_abi::PackedSculptureSnapshot* packedSculpture = nullptr;
    SculptureSnapshotDelivery delivery{};
    std::uint64_t frameCounter = 0U;
    char hud[4096]{};

    SculpturePresentationReceipt presentationReceipt{};
    // Frozen semantic owner for the selected field; never reconstructed from
    // this frame's newer intent. Snapshot persistence is a separate consumer.
    std::shared_ptr<const SculptureCanonicalPublicationInput> canonicalPublicationInput;
    std::shared_ptr<SculptureFrameCaptureObservation> captureObservation;
    std::optional<SculptureFieldCaptureCandidate> fieldCandidate;
    // Application evaluation completion is distinct from receipt.settled:
    // an exception finishes this evaluation without proving a native outcome.
    bool observationFinalized = false;

    [[nodiscard]] const SculptureFramePlan& plan() const {
        if (!framePlan) {
            throw std::logic_error(
                "Sculpture frame plan requested before FreezeFramePlan");
        }
        return *framePlan;
    }
};

} // namespace epi_sculpt
