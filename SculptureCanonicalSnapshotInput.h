#pragma once

#include "SculptureCanonicalPublicationInput.h"
#include "SculptureCaptureIdentity.h"
#include "SculpturePresentationReceipt.h"
#include "SculptureStateRecorder.h"
#include "epi/mode/pattern_mirror.hpp"

namespace epi_sculpt {

struct CanonicalSnapshotInputPreparation {
    bool admitted = true;
    bool inputUnknown = false;
    std::optional<CanonicalFeedbackCaptureState> state;
};

// Snapshot-demand adapter only: producers, schedules and native work stay with
// their owners. The immutable publication supplies math; settled observation
// supplies submission evidence. Neither is reconstructed from current intent.
[[nodiscard]] inline CanonicalSnapshotInputPreparation prepareCanonicalSnapshotInput(
    const SculptureCanonicalPublicationInput* input,
    const SculpturePresentationReceipt& receipt, const SculptureCaptureIdentity& identity,
    std::uint64_t readbackSerial, bool submitted, SculptureWorldMode& world,
    const std::string& pngPath = {}) {
    if (!input) return {true, receipt.canonicalFeedbackGeneration != 0U, {}};
    if (!input->published() || !receipt.submitted()
        || !identity.matchesReadback({receipt.frameSequence, receipt.deviceGeneration,
                                     receipt.surfaceGeneration}, readbackSerial, submitted)
        || input->generation != receipt.canonicalFeedbackGeneration
        || input->resetEpoch != receipt.canonicalFeedbackResetEpoch
        || input->productionReceiptSequence == 0U
        || input->productionReceiptSequence != receipt.canonicalFeedbackProductionReceiptSequence)
        return {false, false, {}};

    CanonicalFeedbackCaptureState state;
    if (input->seedImageInput
        && epi::pattern_mirror::seed_type_from_mode_int(input->mode.seed_type) == epi::SeedType::Image) {
        // Adapt a private copy only on snapshot demand. The published mode and
        // producer keep their admitted descriptive path and immutable owner.
        if (pngPath.empty() || input->seedImageInput->path() != input->mode.seed_image_path)
            return {false, false, {}};
        state.seedImageFile = SculptureStateRecorder::canonicalSeedImagePath(pngPath).filename().string();
        if (!captureSeedImageSiblingName(state.seedImageFile)) return {false, false, {}};
        auto bundledMode = input->mode;
        bundledMode.seed_image_path = state.seedImageFile;
        state.serializedMode = bundledMode.serialize();
        state.seedImageInput = input->seedImageInput;
    } else {
        state.serializedMode = input->mode.serialize();
    }
    state.sourceStateId = input->sourceStateId;
    state.generation = input->generation;
    state.resetEpoch = input->resetEpoch;
    state.submissionSerial = identity.submissionSerial();
    state.readbackSubmissionSerial = readbackSerial;
    state.fieldWidth = input->publication().width;
    state.fieldHeight = input->publication().height;
    state.kernelTime = input->publication().kernelTime;
    state.acceptedControlTime = input->acceptedControlTime;
    state.productionDelta = input->productionDelta;
    state.association = CanonicalFeedbackCaptureAssociation{
        input->sourceStateId, input->presetId, input->presetRevision,
        input->productionReceiptSequence,
        {receipt.frameSequence, receipt.deviceGeneration, receipt.surfaceGeneration}};
    if (!state.valid()) return {false, false, {}};

    // The sidecar describes selected math, while the existing WorldMode controls
    // remain the captured presentation/current-control baseline (v1 role).
    // Use the selected source's recorded provenance, including unknown revision.
    world.presetName = input->presetId;
    world.presetRevision = input->presetRevision;
    if (!state.matchesWorldMode(world)) return {false, false, {}};
    return {true, false, std::move(state)};
}

} // namespace epi_sculpt
