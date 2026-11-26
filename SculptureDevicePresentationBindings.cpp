#include "SculptureDevicePresentationBindings.h"

#include "EpiFeedbackFieldProducer.h"
#include "SculptureFrameProducer.h"
#include "SculptureFieldAppearance.h"

#include <mgfx/Engine.h>

#include <cstddef>
#include <cstdio>
#include <exception>
#include <stdexcept>
#include <utility>

namespace epi_sculpt {

SculptureDevicePresentationBindings::SculptureDevicePresentationBindings(
    mgfx::Engine& engine,
    Recipe recipe)
    : implicitProgram_(std::move(recipe.implicitProgram)),
      implicitSession_(std::make_unique<SculptureImplicitObservationSession>(
          engine, implicitProgram_)),
      director_(std::make_unique<SculpturePresentationDirector>(
          engine, *implicitSession_)),
      compactBridge_(recipe.reliefQuality) {
    compactBridge_.setVolumeRecipe(recipe.volumeRecipe);
    compactBridge_.setFieldAppearance(recipe.fieldAppearance);
    director_->setHudVisible(recipe.hudVisible);
    if (recipe.semanticState) {
        director_->restoreSemanticState(*recipe.semanticState);
    }
}

SculptureDevicePresentationBindings::Recipe
SculptureDevicePresentationBindings::recipeForReconstruction() const {
    Recipe recipe{};
    recipe.implicitProgram = implicitProgram_;
    recipe.semanticState = director_->captureSemanticState();
    recipe.volumeRecipe = compactBridge_.volumeRecipe();
    recipe.fieldAppearance = compactBridge_.fieldAppearance();
    recipe.reliefQuality = compactBridge_.reliefQuality();
    recipe.hudVisible = director_->hudVisible();
    return recipe;
}

SculptureVolumePresentationState
SculptureDevicePresentationBindings::volumePresentation() const noexcept {
    return captureSculptureVolumePresentation(
        compactBridge_.volumeRecipe(), compactBridge_.fieldAppearance());
}

void SculptureDevicePresentationBindings::setVolumePresentation(
    const SculptureVolumePresentationState& state) {
    const auto recipe = static_cast<mgfx::ProceduralGeometryRecipe>(state.recipe);
    const auto appearance = restoreSculptureFieldAppearance(state);
    if (!state.valid() || !mgfx::validProceduralGeometryRecipe(recipe) || !appearance.valid())
        throw std::invalid_argument("invalid sculpture volume presentation");
    // Both setters only validate then assign. With the pair admitted above,
    // neither operation allocates or fails after the first value has changed.
    compactBridge_.setVolumeRecipe(recipe);
    compactBridge_.setFieldAppearance(appearance);
}

SculptureDevicePresentationBindings::~SculptureDevicePresentationBindings() {
    // Raw bridge links must disappear before the transport owners do. Producer
    // imports are detached explicitly by SculptureApplication while producers
    // are still alive; this destructor only guarantees local pointer hygiene.
    compactBridge_.setCanonicalFeedbackTransport(nullptr);
    worldBridge_.setExternalTransport(nullptr);
}

void SculptureDevicePresentationBindings::adoptWorldTransport(
    std::unique_ptr<SculptureWorldExternalTransport> transport) noexcept {
    worldBridge_.setExternalTransport(nullptr);
    worldTransport_ = std::move(transport);
    worldBridge_.setExternalTransport(worldTransport_.get());
}

void SculptureDevicePresentationBindings::clearWorldTransport() noexcept {
    worldBridge_.setExternalTransport(nullptr);
    worldTransport_.reset();
}

void SculptureDevicePresentationBindings::adoptCanonicalTransport(
    std::unique_ptr<EpiFeedbackFieldExternalTransport> transport) noexcept {
    compactBridge_.setCanonicalFeedbackTransport(nullptr);
    canonicalTransport_ = std::move(transport);
    compactBridge_.setCanonicalFeedbackTransport(canonicalTransport_.get());
}

void SculptureDevicePresentationBindings::clearCanonicalTransport() noexcept {
    compactBridge_.setCanonicalFeedbackTransport(nullptr);
    canonicalTransport_.reset();
}

void SculptureDevicePresentationBindings::detachExternalInterop(
    SculptureFrameProducer* sculptureProducer,
    EpiFeedbackFieldProducer* canonicalProducer) noexcept {
    if (sculptureProducer && sculptureProducer->telemetry().submissionIndeterminate) {
        std::fprintf(stderr, "Sculpture interop: indeterminate OpenCL work; stopping before resource detachment\n");
        std::terminate();
    }
    compactBridge_.setCanonicalFeedbackTransport(nullptr);
    worldBridge_.setExternalTransport(nullptr);

    if (canonicalProducer != nullptr) {
        canonicalProducer->set_external_presentation_enabled(false);
        for (std::size_t slot = 0;
             slot < EpiFeedbackFieldExternalTransport::kSlotCount;
             ++slot) {
            if (!canonicalProducer->clear_external_presentation_target_after_native_retirement(slot)) {
                std::fprintf(stderr, "Sculpture interop: canonical import retirement failed; retaining device island\n");
                std::terminate();
            }
        }
    }
    if (sculptureProducer != nullptr) {
        if (!sculptureProducer->clearWorldExternalOutputs()) {
            std::fprintf(stderr, "Sculpture interop: world import retirement failed; retaining device island\n");
            std::terminate();
        }
        sculptureProducer->clearZeroCopyBuffers();
    }

    canonicalTransport_.reset();
    worldTransport_.reset();
}

} // namespace epi_sculpt
