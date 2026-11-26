#pragma once

#include "mgfx/Engine.h"
#include "mgfx/vk/Mesh.h"
#include "mgfx/MaterialLibrary.h"
#include "IExperimentExternalProducer.h"
#include "ExperimentGenericImageRenderBridge.h"
#include "ExperimentMistVolumeRenderBridge.h"
#include "ExperimentGroundWetnessRenderBridge.h"
#include "ExperimentResourceExternalTransport.h"
#include <glm/glm.hpp>

namespace epi_sculpt {

// Generalizes and encapsulates the complex rendering routing logic required to visually present 
// scientific experiments (2D Heightmaps, 3D Mist Volumetrics, Ground Wetness PBR substrates).
// This refactoring allows seamless integration of experiments into any mGFX application window.
class SculptureExperimentRenderBridge {
public:
    // Consumed means input was copied or native work was recorded. Queue
    // acceptance and completion remain the FrameLoop/transport's authority.
    enum class DeliveryResult { Consumed, Retryable, Rejected };
    SculptureExperimentRenderBridge(mgfx::Engine& engine, bool isVolumetric, std::uint32_t axis);
    ~SculptureExperimentRenderBridge() = default;

    // Evaluates the latest simulation delivery and dynamically rebuilds the necessary
    // Vulkan zero-copy buffer projections or host-staged payload textures.
    [[nodiscard]] DeliveryResult processDelivery(
        mgfx::Engine& engine, 
        const GenericExternalDelivery& delivery, 
        ExperimentResourceExternalTransport* transport,
        const struct ExperimentMistOpticsConfig& optics = {});

    // Submits the active experiment bridges and draws the appropriate geometry meshes.
    void draw(mgfx::Engine& engine, mgfx::Drawable& draw, const glm::mat4& rootMatrix);

    mgfx::MaterialHandle activeMaterial() const { return activeMaterial_; }
    mgfx::MaterialHandle groundMaterial() const { return groundMaterial_; }
    
    void updateActiveMaterial(mgfx::Engine& engine, const mgfx::pbr::Material& m) {
        engine.materials()->updateMaterial(activeMaterial_, m);
    }

private:
    bool isVolumetric_;
    std::uint32_t axis_;
    std::uint32_t elementCount_;
    std::uint64_t renderedGeneration_ = 0U;

    ExperimentGenericImageRenderBridge imageBridge_;
    ExperimentMistVolumeRenderBridge mistBridge_;
    ExperimentGroundWetnessRenderBridge wetnessBridge_;

    mgfx::vk::Mesh activeMesh_;
    mgfx::vk::Mesh groundMesh_;
    
    mgfx::MaterialHandle activeMaterial_;
    mgfx::MaterialHandle groundMaterial_;
};

} // namespace epi_sculpt
