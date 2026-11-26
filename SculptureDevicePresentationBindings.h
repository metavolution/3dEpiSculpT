#pragma once

#include "EpiFeedbackFieldExternalTransport.h"
#include "SculptureAcousticMistBridge.h"
#include "SculpturePresentationDirector.h"
#include "SculptureRenderBridge.h"
#include "SculptureWorldExternalTransport.h"
#include "SculptureWorldRenderBridge.h"
#include "SculptureVolumePresentationState.h"
#include <mgfx/ProceduralFieldAppearance.h>

#include <memory>
#include <optional>

namespace mgfx { class Engine; }

namespace epi_sculpt {

class EpiFeedbackFieldProducer;
class SculptureFrameProducer;

// Device-bound presentation adapters and cross-API transports live beneath one
// application owner. Durable mathematical producers intentionally do not: a
// future logical-device replacement can tear this island down, rebuild it for
// a new Vulkan device, and then rebind the surviving producer state.
class SculptureDevicePresentationBindings final {
public:
    struct Recipe final {
        ImplicitFieldProgram implicitProgram{};
        std::optional<SculpturePresentationSemanticState> semanticState{};
        mgfx::ProceduralGeometryRecipe volumeRecipe = mgfx::ProceduralGeometryRecipe::Field;
        mgfx::ProceduralFieldAppearance fieldAppearance{};
        SculptureReliefQuality reliefQuality = SculptureReliefQuality::Standard128;
        bool hudVisible = true;
    };

    SculptureDevicePresentationBindings(mgfx::Engine& engine, Recipe recipe);
    ~SculptureDevicePresentationBindings();

    SculptureDevicePresentationBindings(const SculptureDevicePresentationBindings&) = delete;
    SculptureDevicePresentationBindings& operator=(const SculptureDevicePresentationBindings&) = delete;

    [[nodiscard]] SculpturePresentationDirector& director() noexcept { return *director_; }
    [[nodiscard]] const SculpturePresentationDirector& director() const noexcept { return *director_; }
    [[nodiscard]] SculptureRenderBridge& compactBridge() noexcept { return compactBridge_; }
    [[nodiscard]] const SculptureRenderBridge& compactBridge() const noexcept { return compactBridge_; }
    [[nodiscard]] SculptureWorldRenderBridge& worldBridge() noexcept { return worldBridge_; }
    [[nodiscard]] const SculptureWorldRenderBridge& worldBridge() const noexcept { return worldBridge_; }
    [[nodiscard]] SculptureAcousticMistBridge& acousticMist() noexcept { return acousticMist_; }
    [[nodiscard]] const SculptureAcousticMistBridge& acousticMist() const noexcept { return acousticMist_; }

    [[nodiscard]] Recipe recipeForReconstruction() const;

    [[nodiscard]] SculptureVolumePresentationState volumePresentation() const noexcept;
    // Validate the complete candidate before either bridge value changes.
    void setVolumePresentation(const SculptureVolumePresentationState& state);

    void adoptWorldTransport(std::unique_ptr<SculptureWorldExternalTransport> transport) noexcept;
    void clearWorldTransport() noexcept;
    [[nodiscard]] SculptureWorldExternalTransport* worldTransport() noexcept { return worldTransport_.get(); }
    [[nodiscard]] const SculptureWorldExternalTransport* worldTransport() const noexcept { return worldTransport_.get(); }

    void adoptCanonicalTransport(std::unique_ptr<EpiFeedbackFieldExternalTransport> transport) noexcept;
    void clearCanonicalTransport() noexcept;
    [[nodiscard]] EpiFeedbackFieldExternalTransport* canonicalTransport() noexcept { return canonicalTransport_.get(); }
    [[nodiscard]] const EpiFeedbackFieldExternalTransport* canonicalTransport() const noexcept { return canonicalTransport_.get(); }

    // Cross-API imports are producer-owned views of resources exported by this
    // island. Detach them explicitly before destroying/replacing the Vulkan
    // device so no surviving OpenCL producer can retain a native resource from
    // the retired device generation.
    void detachExternalInterop(
        SculptureFrameProducer* sculptureProducer,
        EpiFeedbackFieldProducer* canonicalProducer) noexcept;

private:
    ImplicitFieldProgram implicitProgram_{};
    // Declared first so the borrowing director is destroyed before the session.
    // Native work/callback retirement still precedes destruction of this island.
    std::unique_ptr<SculptureImplicitObservationSession> implicitSession_;
    std::unique_ptr<SculpturePresentationDirector> director_;
    SculptureRenderBridge compactBridge_{};
    SculptureWorldRenderBridge worldBridge_{};
    SculptureAcousticMistBridge acousticMist_{};
    std::unique_ptr<SculptureWorldExternalTransport> worldTransport_;
    std::unique_ptr<EpiFeedbackFieldExternalTransport> canonicalTransport_;
};

} // namespace epi_sculpt
