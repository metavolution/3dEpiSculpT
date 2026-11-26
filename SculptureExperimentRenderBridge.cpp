#include "SculptureExperimentRenderBridge.h"
#include "ExperimentMistFieldAbi.h"
#include "ExperimentGroundWetnessFieldAbi.h"
#include "ExperimentZeroCopyHandoff.h"
#include <glm/gtc/matrix_transform.hpp>
#include <span>
#include <array>

namespace epi_sculpt {

SculptureExperimentRenderBridge::SculptureExperimentRenderBridge(mgfx::Engine& engine, bool isVolumetric, std::uint32_t axis)
    : isVolumetric_(isVolumetric), axis_(axis), elementCount_(axis * axis),
      activeMesh_(isVolumetric ? mgfx::vk::Mesh::makeCube(engine.context()) : mgfx::vk::Mesh::makeDenseGrid(engine.context(), axis, axis)),
      groundMesh_(mgfx::vk::Mesh::makeDenseGrid(engine.context(), 128, 128)) 
{
    mgfx::pbr::Material gm{};
    gm.baseColorFactor = mgfx::math::vec4(0.4f, 0.45f, 0.4f, 1.0f);
    gm.roughness = 0.9f;
    gm.metallic = 0.05f;
    groundMaterial_ = engine.materials()->createMaterial(gm);
    
    mgfx::pbr::Material m{};
    m.baseColorFactor = mgfx::math::vec4(0.8f, 0.9f, 1.0f, 1.0f);
    m.emissiveFactor = mgfx::math::vec3(0.85f);
    m.roughness = 0.4f;
    m.metallic = 0.2f;
    m.parallaxHeightScale = 1.0f; 
    m.clearcoatFactor = 1.0f;     
    activeMaterial_ = engine.materials()->createMaterial(m);
}

SculptureExperimentRenderBridge::DeliveryResult SculptureExperimentRenderBridge::processDelivery(
    mgfx::Engine& engine, 
    const GenericExternalDelivery& delivery, 
    ExperimentResourceExternalTransport* transport,
    const ExperimentMistOpticsConfig& optics) 
{
    if (!delivery.valid() || delivery.generation == 0U) return DeliveryResult::Rejected;
    if (delivery.generation == renderedGeneration_) return DeliveryResult::Consumed;
    
    const ExperimentResourceReference* ref = delivery.manifest->find(ExperimentResourceRole::DenseMedium);
    bool primaryPresentationAccepted = false;
    // Volumetric replacement is transactional: the bridge keeps the last
    // compatible descriptor bundle alive until a complete candidate is published.
    // Explicit mode/route retirement still clears the Engine payload.
    if (ref && ref->externalLease.has_value() && transport) {
        auto lease = ref->externalLease.value();
        primaryPresentationAccepted = rebuildFromExternalVulkanRead(
            *transport, lease, delivery.generation, engine.frameLoop(),
            [&](const mgfx::vk::Buffer& source) {
                if (isVolumetric_) {
                    experiment_mist_field_gpu_abi::MistFieldDescriptorGpu fd{};
                    fd.identity[0] = experiment_mist_field_gpu_abi::kVersion;
                    fd.identity[1] = experiment_mist_field_gpu_abi::kDescriptorBytes;
                    fd.identity[2] = axis_;
                    fd.identity[3] = axis_;
                    fd.extentCadence = {axis_, 33333U,
                        experiment_mist_field_gpu_abi::MistFieldCarriesMeanVelocity
                            | experiment_mist_field_gpu_abi::MistFieldCarriesOpticalMoments, 0U};
                    fd.worldMinimum = {-4.0f, 0.0f, -4.0f, 0.0f};
                    fd.worldMaximum = {4.0f, 4.0f, 4.0f, 0.0f};
                    return mistBridge_.rebuildExternal(
                        engine, *delivery.manifest, fd, source, optics);
                }

                const bool rebuilt = imageBridge_.rebuildExternal(
                        engine, *delivery.manifest, axis_, axis_, elementCount_, source);
                if (rebuilt) {
                    constexpr std::array<std::uint32_t, 2> bindings{
                        mgfx::pbr::kEmissiveBinding, mgfx::pbr::kHeightBinding};
                    imageBridge_.bindMaterialImage(engine, activeMaterial_, bindings);
                }
                return rebuilt;
            },
            [this, generation = delivery.generation] {
                if (renderedGeneration_ == generation) renderedGeneration_ = 0U;
            });
        if (!primaryPresentationAccepted) {
            // Only the exact still-released ticket can be retried. A failed
            // rebuild after acquisition quarantines the slot in the helper.
            if (!lease.valid() || lease.allocationGeneration != transport->config().allocationGeneration ||
                lease.slotIndex >= ExperimentResourceExternalTransport::kSlotCount)
                return DeliveryResult::Rejected;
            const auto slot = transport->diagnostics().slots[lease.slotIndex];
            return slot.state == SculptureExternalSlotState::OpenClReleased &&
                   slot.ticket.generation == delivery.generation && slot.ticket.epoch == lease.readySerial
                ? DeliveryResult::Retryable : DeliveryResult::Rejected;
        }
    } else if (ref && !ref->externalLease.has_value()) {
        if (!isVolumetric_) {
            primaryPresentationAccepted = imageBridge_.rebuild(
                engine, *delivery.manifest, axis_, axis_, elementCount_, ref->hostStagedPayload);
            if (!primaryPresentationAccepted) return DeliveryResult::Rejected;
            constexpr std::array<std::uint32_t, 2> bindings{
                mgfx::pbr::kEmissiveBinding, mgfx::pbr::kHeightBinding};
            imageBridge_.bindMaterialImage(engine, activeMaterial_, bindings);
        } else {
            if (ref->hostStagedPayload == nullptr) return DeliveryResult::Rejected;
            experiment_mist_field_gpu_abi::MistFieldDescriptorGpu fd{};
            fd.identity[0] = experiment_mist_field_gpu_abi::kVersion;
            fd.identity[1] = experiment_mist_field_gpu_abi::kDescriptorBytes;
            fd.identity[2] = axis_;
            fd.identity[3] = axis_;
            fd.extentCadence = {axis_, 33333U,
                experiment_mist_field_gpu_abi::MistFieldCarriesMeanVelocity
                    | experiment_mist_field_gpu_abi::MistFieldCarriesOpticalMoments, 0U};
            fd.worldMinimum = {-4.0f, 0.0f, -4.0f, 0.0f};
            fd.worldMaximum = {4.0f, 4.0f, 4.0f, 0.0f};
            const std::span<const experiment_mist_field_gpu_abi::MistVoxelGpu> voxels(
                static_cast<const experiment_mist_field_gpu_abi::MistVoxelGpu*>(
                    ref->hostStagedPayload),
                ref->descriptor.elementCount);
            primaryPresentationAccepted = mistBridge_.rebuild(
                engine, *delivery.manifest, fd, voxels, optics);
            if (!primaryPresentationAccepted) return DeliveryResult::Rejected;
        }
    } else {
        return DeliveryResult::Rejected;
    }

    if (isVolumetric_) {
        const ExperimentResourceReference* wetnessRef = delivery.manifest->find(ExperimentResourceRole::SurfaceSubstrate);
        if (wetnessRef && !wetnessRef->externalLease.has_value() && wetnessRef->hostStagedPayload) {
            experiment_ground_wetness_gpu_abi::GroundWetnessFieldDescriptorGpu wetnessFd{};
            wetnessFd.identity[0] = experiment_ground_wetness_gpu_abi::kVersion;
            wetnessFd.identity[1] = experiment_ground_wetness_gpu_abi::kDescriptorBytes;
            wetnessFd.identity[2] = 128U;
            wetnessFd.identity[3] = 128U;
            wetnessFd.worldBounds = {-8.0f, -8.0f, 8.0f, 8.0f};
            wetnessFd.dynamics = {0.0012f, 0.00015f, 2.0f, 1.0f / 120.0f};
            wetnessFd.transport = {0.035f, 0.0018f, 0.0f, 0.0f};
            std::span<const experiment_ground_wetness_gpu_abi::GroundWetnessCellGpu> cells(
                static_cast<const experiment_ground_wetness_gpu_abi::GroundWetnessCellGpu*>(wetnessRef->hostStagedPayload),
                wetnessRef->descriptor.elementCount
            );
            if (wetnessBridge_.rebuild(engine, delivery.generation, wetnessFd, cells)) {
                constexpr std::array<std::uint32_t, 2> bindings{
                    mgfx::pbr::kMraoBinding, mgfx::pbr::kAlbedoBinding};
                wetnessBridge_.bindMaterialImage(engine, groundMaterial_, bindings, mgfx::MraoAlphaMeaning::Wetness);
            }
        }
    }
    
    renderedGeneration_ = delivery.generation;
    return DeliveryResult::Consumed;
}

void SculptureExperimentRenderBridge::draw(mgfx::Engine& engine, mgfx::Drawable& draw, const glm::mat4& rootMatrix) {
    if (isVolumetric_) {
        draw.draw(groundMesh_, groundMaterial_, rootMatrix);
        // Engine snapshots ProceduralPayload only after this Drawable callback
        // returns. Keep the volume bound through that snapshot; clearing here
        // would make the procedural pass disappear from the same frame.
        static_cast<void>(mistBridge_.bind(engine));
        draw.draw(activeMesh_, activeMaterial_, rootMatrix);
    } else {
        // Explicitly retire any previous volumetric payload when this bridge
        // changes presentation mode.
        mistBridge_.clear(engine);
        draw.draw(activeMesh_, activeMaterial_, rootMatrix);
    }
}

} // namespace epi_sculpt
