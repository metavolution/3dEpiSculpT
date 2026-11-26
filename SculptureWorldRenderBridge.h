// Application-owned Vulkan presentation adapter for WorldSnapshotGpu.
// mGFX sees only complete transform matrices and materials; it never learns
// about preset names, OpenCL kernels, or epimorphism mathematics.

#pragma once

#include "SculptureWorldProducer.h"

#include <mgfx/Drawable.h>
#include <mgfx/FrameViews.h>
#include <mgfx/vk/Instance.h>
#include <mgfx/vk/Buffer.h>
#include <mgfx/vk/Pipeline.h>
#include <mgfx/vk/Descriptor.h>
#include <mgfx/Engine.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <vector>

namespace mgfx::vk {
class Mesh;
}

namespace epi_sculpt {

class SculptureWorldExternalTransport;

class SculptureWorldRenderBridge {
public:
    // Owners may replace transports only between frames, after GPU retirement.
    void setExternalTransport(const SculptureWorldExternalTransport* transport) noexcept {
        if (externalTransport_ != transport) reset();
        externalTransport_ = transport;
    }
    static constexpr std::size_t kPrimitiveCount = 8U;
    // The immutable OpenCL delivery contract has exactly Form, Structure, and
    // Accent. Keeping this equal to the compaction shader stride prevents
    // allocating eleven unreachable bands per primitive per renderer slot.
    static constexpr std::size_t kMaterialBandCapacity = 3U;
    static constexpr std::size_t kBatchCount =
        kPrimitiveCount * kMaterialBandCapacity;

    struct PrimitiveRegistry {
        // The application owns the static meshes. Null entries are a safe
        // omission: their batch is retained for diagnostics but not drawn.
        std::array<const mgfx::vk::Mesh*, kPrimitiveCount> meshes{};
    };

    struct CompactionPushConstants {
        // xyz = snapshot-local camera/focus, w = >0.5 enables distance-adaptive LOD & culling
        mgfx::math::vec4 cameraPosAndCullEnable{0.0f, 0.0f, 0.0f, 0.0f};
        // x = lod1Dist, y = lod2Dist, z = lod3Dist, w = maxRenderDistance (<=0 disables far clip)
        mgfx::math::vec4 lodDistancesAndMaxDist{35.0f, 75.0f, 140.0f, 0.0f};
        // xyz = normalized view forward vector, w = cos(halfConeFov) (<= -1.0 disables frustum cone test)
        mgfx::math::vec4 frustumForwardAndCosHalfFov{0.0f, 0.0f, -1.0f, -2.0f};
        // x = optical radius compensation blend (0..1, default 1.0 = full sqrt(stride)),
        // y = minimum instance scale threshold (default 1e-5),
        // z = local protected-caster radius about the focus (<=0 disables),
        // w = prototype origin-radius bound, resolved from meshes during draw.
        mgfx::math::vec4 lodParams{1.0f, 1.0e-5f, 0.0f, 0.0f};
    };
    static_assert(sizeof(CompactionPushConstants) == 64U,
                  "CompactionPushConstants must match world_compaction.comp push_constant layout");

    void setCompactionPushConstants(const CompactionPushConstants& pc) noexcept {
        compactionPushConstants_ = pc;
    }
    [[nodiscard]] const CompactionPushConstants& compactionPushConstants() const noexcept {
        return compactionPushConstants_;
    }

    // One conservative local-space distance envelope covers every selected eye.
    // Threshold inflation accounts for eye separation and inverse-root stretch,
    // including nonuniform scale/shear. Keeps the cone disabled for shared draws.
    // Empty views, invalid budgets or unusable/non-affine roots defer selection.
    // Published cascade bounds protect potential world casters from eye LOD.
    // Coverage is limited to the current finite directional cascades.
    [[nodiscard]] static std::optional<CompactionPushConstants> makeViewFamilyLodPushConstants(
        const mgfx::FrameViews& views,
        const mgfx::math::mat4& rootTransform = mgfx::math::mat4(1.0f),
        const mgfx::math::vec4& worldLodDistancesAndMaxDist = {45.0f, 95.0f, 175.0f, 320.0f}) noexcept;

    void rebuild(const WorldSnapshotDelivery& delivery);

    // Emits one full-matrix instanced draw for each non-empty primitive and
    // material batch whose shared mesh/material has been supplied. Mesh index
    // headers are cached on the host and copied into command-buffer storage
    // when source, compaction values or the mesh template change. A retained
    // GPU snapshot supports view-dependent rebuilds after transport release.
    // Host deliveries are copied before their borrowed producer storage expires.
    // True means the selected generation's checked draw commands were admitted;
    // it is not evidence of native submission, presentation or GPU completion.
    // Frame presenters qualify their expected generation (zero means no world).
    // Bootstrap still runs before this qualification so warm-up can make progress.
    bool draw(mgfx::Drawable& draw,
              const PrimitiveRegistry& primitives,
              std::span<const mgfx::MaterialHandle> materials,
              const mgfx::math::mat4& rootTransform = mgfx::math::mat4(1.0f),
              std::optional<std::uint32_t> expectedGeneration = std::nullopt);

    void reset() noexcept {
        generation_ = 0U;
        instanceCount_ = 0U;
        droppedInstanceCount_ = 0U;
        activeBatchCount_ = 0U;
        externalSlotIndex_ = WorldSnapshotDelivery::kNoSlot;
        presentationTicket_ = {};
        hostSnapshot_.reset();
        metrics_ = {};
        compactionPushConstants_ = {};
        resources_.reset();
        templateMeshes_.fill(nullptr);
        templateMeshRegistryKnown_ = false;
        templateRevision_ = 0U;
    }

    [[nodiscard]] std::size_t instanceCount() const noexcept { return instanceCount_; }
    [[nodiscard]] std::size_t droppedInstanceCount() const noexcept { return droppedInstanceCount_; }
    [[nodiscard]] std::size_t activeBatchCount() const noexcept { return activeBatchCount_; }
    [[nodiscard]] std::uint32_t generation() const noexcept { return generation_; }
    [[nodiscard]] const WorldBridgeMetrics& metrics() const noexcept { return metrics_; }

private:
    struct Resources;

    std::size_t instanceCount_ = 0U;
    std::size_t droppedInstanceCount_ = 0U;
    std::size_t activeBatchCount_ = 0U;
    std::uint32_t generation_ = 0U;
    std::uint32_t externalSlotIndex_ = WorldSnapshotDelivery::kNoSlot;
    WorldPresentationTicket presentationTicket_{};
    std::unique_ptr<const world_gpu_abi::WorldSnapshotGpu> hostSnapshot_;
    WorldBridgeMetrics metrics_{};
    CompactionPushConstants compactionPushConstants_{};
    const SculptureWorldExternalTransport* externalTransport_ = nullptr;

    std::shared_ptr<Resources> resources_;
    std::array<const mgfx::vk::Mesh*, kPrimitiveCount> templateMeshes_{};
    using IndirectTemplate = std::array<VkDrawIndexedIndirectCommand, kBatchCount>;
    IndirectTemplate indirectTemplate_{};
    static_assert(sizeof(IndirectTemplate) <= 65536U && sizeof(IndirectTemplate) % 4U == 0U,
                  "vkCmdUpdateBuffer requires a small, four-byte-aligned payload");
    bool templateMeshRegistryKnown_ = false;
    std::uint64_t templateRevision_ = 0U;

    [[nodiscard]] std::shared_ptr<Resources> ensureCompute(mgfx::Engine& engine);
    [[nodiscard]] bool prepareCompaction(mgfx::Engine& engine,
        const std::shared_ptr<Resources>& owner, std::uint64_t derivativeGeneration);
    void refreshIndirectTemplate(const PrimitiveRegistry& primitives);
};

} // namespace epi_sculpt
