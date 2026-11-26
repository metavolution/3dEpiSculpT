// Application-owned adapter from compact sculpture ABI samples to mGFX's
// existing per-frame instancing path. Procedural semantics stay upstream.

#pragma once

#include "SculptureReliefPresentation.h"
#include "SculptureFieldCaptureEvidence.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>
#include <memory>
#include <optional>
#include "mgfx/scene/Scene.h"
#include "mgfx/vk/Pipeline.h"
#include "mgfx/vk/Descriptor.h"
#include "mgfx/vk/Buffer.h"
#include "mgfx/vk/Semaphore.h"
#include "mgfx/vk/Image.h"

#include "SculptureGpuAbi.h"
#include "SculptureLodEngine.h"
#include "SculptureIsosurface.h"
#include "mgfx/vk/Instance.h"
#include "mgfx/Drawable.h"
#include "mgfx/ProceduralFieldMapping.h"
#include "mgfx/ProceduralFieldContentIdentity.h"
#include "mgfx/ProceduralFieldAppearance.h"
#include "mgfx/ProceduralGeometryRecipe.h"
#include "SculptureFrameProducer.h"
#include "EpiFeedbackFieldDelivery.h"

namespace epi::presentation { class VulkanFeedbackExternalTransport; }
namespace mgfx { struct ProceduralPayload; }

namespace epi_sculpt {

using EpiFeedbackFieldExternalTransport = epi::presentation::VulkanFeedbackExternalTransport;

// -----------------------------------------------------------------------------
// Educational Annotation: ZERO-ALLOCATION GPU RENDER BRIDGE & LOD STAGING
// -----------------------------------------------------------------------------
// Serves as the high-throughput bridge between the continuous epimorphism
// simulation snapshot (CPU/OpenCL) and mGFX's Vulkan rendering pipeline.
//
// Key Responsibilities:
// 1. Zero-Allocation Instance Staging:
//    Maintains pre-allocated CPU instance batches for 3 PBR material bands.
//
// 2. High-Performance Frustum & Density Culling:
//    Extracts 6 analytical frustum planes from view-projection matrices and
//    culls out-of-view samples prior to batch submission.
//
// 3. Distance-Adaptive Dynamic LOD Decimation:
//    Applies stride-based instance decimation (16K -> 8K -> 4K) and optical
//    radius compensation (sqrt(stride)) to sustain 60+ FPS at 3.3K/8K displays.
// -----------------------------------------------------------------------------

// The bridge is the sole authority for choosing how an immutable sculpture
// snapshot reaches mGFX. Keeping this state explicit makes fallback behavior
// observable without leaking renderer details into the producer.
enum class SculptureRenderPath : std::uint8_t {
    NoRenderableSnapshot,
    MappedCompute,
    StagedCpuInstances,
};

[[nodiscard]] constexpr const char* renderPathName(SculptureRenderPath path) noexcept {
    switch (path) {
    case SculptureRenderPath::MappedCompute: return "mapped-compute";
    case SculptureRenderPath::StagedCpuInstances: return "staged-cpu";
    case SculptureRenderPath::NoRenderableSnapshot: return "unavailable";
    }
    return "unavailable";
}

// Frame-local presentation evidence. Collected means draw-queue insertion;
// queue acceptance, GPU completion and display confirmation belong to receipts.
enum class ReliefDrawOutcome : std::uint8_t {
    Collected,
    Deferred, // Current presentation resources or submission capacity unavailable.
    Rejected, // Invalid input or an external delivery that cannot be resolved.
};

[[nodiscard]] constexpr const char* reliefDrawOutcomeName(ReliefDrawOutcome outcome) noexcept {
    switch (outcome) {
    case ReliefDrawOutcome::Collected: return "DRAW COLLECTED";
    case ReliefDrawOutcome::Deferred: return "DRAW DEFERRED";
    case ReliefDrawOutcome::Rejected: return "DRAW REJECTED";
    }
    return "DRAW REJECTED";
}

// -----------------------------------------------------------------------------
// Educational Annotation: SHARED PROCEDURAL PAYLOAD
// This C++ struct strictly mirrors the layout(push_constant) block in both
// expand_instances.comp and procedural_world.frag. By centralizing it, we 
// ensure cross-shader memory alignment and simplify the execution flow.
// -----------------------------------------------------------------------------
struct ProceduralPushConstants {
    mgfx::math::mat4 groupModel;
    std::uint32_t activeCount;
    std::uint32_t colorEffect;
    std::uint32_t stageMask;
    float time;
    float chromaBreath;
    float lumaDrift;
    float paletteBlend;
    float hueRotation;
    mgfx::math::vec4 audioSpectrum;
    float temporalFeedbackBlend;
    std::uint32_t temporalReset;
};

static_assert(sizeof(ProceduralPushConstants) <= 128, "ProceduralPushConstants exceeds 128 bytes");

struct FrustumPlane {
    mgfx::math::vec3 normal{0.0f};
    float distance{0.0f};

    [[nodiscard]] constexpr float signedDistance(const mgfx::math::vec3& point) const noexcept {
        return glm::dot(normal, point) + distance;
    }
};

struct CullingFrustum {
    std::array<FrustumPlane, 6> planes{};

    [[nodiscard]] bool isSphereInside(const mgfx::math::vec3& center, float radius) const noexcept {
        for (const auto& plane : planes) {
            if (plane.signedDistance(center) < -radius) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] bool isAABBInside(const mgfx::math::vec3& minBounds, const mgfx::math::vec3& maxBounds) const noexcept {
        for (const auto& plane : planes) {
            mgfx::math::vec3 pVertex = minBounds;
            if (plane.normal.x >= 0.0f) pVertex.x = maxBounds.x;
            if (plane.normal.y >= 0.0f) pVertex.y = maxBounds.y;
            if (plane.normal.z >= 0.0f) pVertex.z = maxBounds.z;
            if (plane.signedDistance(pVertex) < 0.0f) {
                return false;
            }
        }
        return true;
    }
};

#include "SculptureProgramSchema.h"

struct InstanceFilterOptions {
    float minDensityThreshold = 0.0f;
    const mgfx::math::mat4* viewProjMatrix = nullptr;
    const CullingFrustum* frustum = nullptr;
    const mgfx::math::vec3* cameraPosition = nullptr;
    SculptureLodConfig lodConfig{};
};



class SculptureRenderBridge {
public:
    static constexpr std::size_t kMaterialBandCount = 3U;
    // The OpenCL snapshot and the direct Vulkan expansion have the same
    // bounded cardinality. The output record size is always derived from the
    // renderer's authoritative InstanceData layout, never hand-maintained.
    static constexpr std::size_t kDirectInstanceCapacity = kSampleCount;
    struct VolumePublicationCounters {
        std::uint64_t acceptedIngests = 0;
        std::uint64_t acceptedReplays = 0;
    };

    explicit SculptureRenderBridge(SculptureReliefQuality reliefQuality = SculptureReliefQuality::Standard128);
    ~SculptureRenderBridge();
    [[nodiscard]] SculptureReliefQuality reliefQuality() const noexcept { return reliefQuality_; }

    void rebuild(const gpu_abi::PackedSculptureSnapshot* snapshot);
    void rebuild(const gpu_abi::PackedSculptureSnapshot* snapshot,
                 const InstanceFilterOptions& filterOptions);

    [[nodiscard]] bool drawDirectly(
        mgfx::Drawable& draw,
        const mgfx::vk::Mesh& mesh,
        std::span<const mgfx::MaterialHandle> materials,
        const mgfx::math::mat4& rootTransform,
        const SculptureProgramParams& params,
        const SculptureSnapshotDelivery& delivery,
        const InstanceFilterOptions& filterOptions = {});

    // Collection is not submission or physical presentation. The attempt
    // token prevents a route change from reusing a previous frame's fact.
    [[nodiscard]] std::uint32_t collectedGenerationFor(std::uint64_t attempt) const noexcept {
        return attempt != 0U && attempt == collectedAttempt_ ? collectedGeneration_ : 0U;
    }
    [[nodiscard]] std::optional<SculptureFieldCaptureCandidate> fieldCandidateFor(
        std::uint64_t attempt) const noexcept {
        return fieldCandidate_ && fieldCandidate_->attempt == attempt ? fieldCandidate_ : std::nullopt;
    }
    [[nodiscard]] bool fieldCandidateAccepted(const SculptureFieldCaptureCandidate& candidate,
                                               std::uint64_t serial) const noexcept;
    [[nodiscard]] VolumePublicationCounters volumePublicationCounters() const noexcept;

    // Presentation policy is independent of producer parameters and accepted
    // field storage. A recipe change can reuse that storage without CL work.
    void setVolumeRecipe(mgfx::ProceduralGeometryRecipe recipe);
    [[nodiscard]] mgfx::ProceduralGeometryRecipe volumeRecipe() const noexcept { return volumeRecipe_; }
    void setFieldAppearance(const mgfx::ProceduralFieldAppearance& appearance);
    [[nodiscard]] const mgfx::ProceduralFieldAppearance& fieldAppearance() const noexcept {
        return fieldAppearance_;
    }

    // Retires one externally shared compact slot through the current frame's
    // Vulkan submission. This is for presentation branches which intentionally
    // do not consume the compact payload but must still settle its lease.
    [[nodiscard]] bool retireExternalDelivery(
        mgfx::Drawable& draw,
        const SculptureSnapshotDelivery& delivery);

    // Collect a 2.5D relief draw, or report why no draw was collected. Native
    // preparation/recording exceptions propagate; they are not normal deferrals.
    [[nodiscard]] ReliefDrawOutcome drawRelief(
        mgfx::Drawable& draw,
        mgfx::MaterialHandle material,
        const mgfx::math::mat4& rootTransform,
        const SculptureProgramParams& params,
        const EpiFeedbackFieldDelivery& field,
        bool applyPoincareDisk = false);

    // Generates CPU-side vertex and index buffers for 2.5D epimorphic relief geometry

    // 4D Manifold / Isosurface Mesh Rendering Path
    void drawManifoldMesh(
        mgfx::Drawable& draw,
        mgfx::MaterialHandle material,
        const mgfx::math::mat4& rootTransform,
        const IsosurfaceMesh& isoMesh);

    // Converts Isosurface polygon mesh into indexed Vulkan GPU mesh
    [[nodiscard]] static std::unique_ptr<mgfx::vk::Mesh> createManifoldVkMesh(
        const mgfx::vk::Context& context,
        const IsosurfaceMesh& isoMesh);

    // A retained isosurface snapshot for Engine scene or domain-slice adoption.
    [[nodiscard]] static std::optional<mgfx::scene::Scene> makeManifoldScene(
        const IsosurfaceMesh& isoMesh, mgfx::scene::MaterialDef material);

    [[nodiscard]] std::span<const mgfx::vk::CompactInstanceData> band(
        std::size_t materialBand) const noexcept;
    [[nodiscard]] std::size_t instanceCount() const noexcept;
    [[nodiscard]] std::size_t batchCount() const noexcept;
    [[nodiscard]] std::size_t droppedSampleCount() const noexcept;
    [[nodiscard]] std::size_t prunedSampleCount() const noexcept;
    [[nodiscard]] std::size_t culledSampleCount() const noexcept;
    [[nodiscard]] const SculptureLodStats& lodStats() const noexcept;
    [[nodiscard]] std::uint32_t generation() const noexcept;
    [[nodiscard]] SculptureRenderPath renderPath() const noexcept;
    [[nodiscard]] const SculptureBoundingVolume& boundingVolume() const noexcept { return boundingVolume_; }
    [[nodiscard]] const SculptureMotionKinematics& motionKinematics() const noexcept { return motionKinematics_; }

    [[nodiscard]] static CullingFrustum extractFrustum(const mgfx::math::mat4& viewProj) noexcept;
    [[nodiscard]] static SculptureLodTier calculateLodTier(float distanceToCam, const SculptureLodConfig& config) noexcept;

    // Allocates the direct Vulkan ingestion resources required by every render
    // path. External FDs are an optional export capability layered on top.
    void initializeGpuIngest(mgfx::Engine& engine, SculptureFrameProducer& producer);

    void setCanonicalFeedbackTransport(EpiFeedbackFieldExternalTransport* transport) noexcept {
        canonicalFeedbackTransport_ = transport;
    }

    void initCompute(mgfx::Engine& engine);

private:
    [[nodiscard]] bool canAcquireExternalDelivery(
        const SculptureSnapshotDelivery& delivery) const noexcept;
    void enqueueAcquiredExternalDeliveryHandoff(
        mgfx::Drawable& draw,
        const SculptureSnapshotDelivery& delivery);

    std::size_t instanceCount_ = 0U;
    std::size_t activeBatchCount_ = 0U;
    std::size_t droppedSampleCount_ = 0U;
    std::size_t prunedSampleCount_ = 0U;
    std::size_t culledSampleCount_ = 0U;
    SculptureLodStats lodStats_{};
    std::uint32_t generation_ = 0U;
    std::uint64_t collectedAttempt_ = 0U;
    std::uint32_t collectedGeneration_ = 0U;
    std::optional<SculptureFieldCaptureCandidate> fieldCandidate_{};
    SculptureRenderPath renderPath_ = SculptureRenderPath::NoRenderableSnapshot;
    SculptureBoundingVolume boundingVolume_{};
    SculptureMotionKinematics motionKinematics_{};
    std::array<std::vector<mgfx::vk::CompactInstanceData>, kMaterialBandCount> cpuBatches_;

    struct ExternalSemaphorePair {
        std::unique_ptr<mgfx::vk::Semaphore> vulkanRelease = nullptr;
        std::unique_ptr<mgfx::vk::Semaphore> openClRelease = nullptr;
    };

    // Completion-owned GPU ingest and compute resources. Frame retention and
    // the submit callback keep this bundle alive; synchronous graph recording
    // may borrow it without enlarging bounded graph callback storage.
    struct GpuResources {
        // Borrowed solely to enforce the original producer binding. The
        // producer outlives this binding; its address is never an identity.
        const SculptureFrameProducer* boundProducer = nullptr;
        std::uint64_t fieldPublisherNamespace = 0U;
        std::uint64_t fieldActivationEpoch = 0U;
        std::uint64_t lastFieldGeneration = 0U;
        std::vector<std::unique_ptr<mgfx::vk::Buffer>> ingestBuffers;
        std::vector<std::unique_ptr<mgfx::vk::Buffer>> ingestSpatialBuffers;
        std::vector<std::unique_ptr<mgfx::vk::Image>> ingestVolumeImages;
        std::vector<SculptureFrameProducer::ZeroCopyBuffer> ingestSlots;
        // Only [0, hostSlotBase) can be exported to OpenCL. Host staging is
        // indexed by the fence-retired Engine frame slot, never by generation.
        std::size_t hostSlotBase = 0U;
        VkDevice externalSemaphoreDevice = VK_NULL_HANDLE;
        std::vector<ExternalSemaphorePair> externalSemaphores;

        bool computeInitialized = false;
        bool computeInitializationFailed = false;
        VkDevice computeDevice = VK_NULL_HANDLE;
        std::unique_ptr<mgfx::vk::Pipeline> computePipeline;
        std::unique_ptr<mgfx::vk::Pipeline> volumeTransferPipeline;
        std::unique_ptr<mgfx::vk::DescriptorAllocator> descriptorAllocator;
        VkDescriptorSetLayout computeSetLayout = VK_NULL_HANDLE;
        VkDescriptorSetLayout volumeSetLayout = VK_NULL_HANDLE;
        std::vector<std::unique_ptr<mgfx::vk::Buffer>> instanceBuffers;
        std::vector<std::unique_ptr<mgfx::vk::Buffer>> proceduralUniformBuffers;
        std::vector<VkDescriptorSet> descriptorSets;
        std::vector<VkDescriptorSet> volumeDescriptorSets;
        struct PreparedIngest {
            ProceduralPushConstants push{};
            bool volume = false;
            bool replay = false;
            std::uint32_t generation = 0U;
            mgfx::ProceduralFieldMapping mapping{};
            mgfx::ProceduralFieldContentIdentity identity{};
        };
        struct VolumePublication {
            ProceduralPushConstants push{};
            std::size_t index = 0U;
            std::uint32_t generation = 0U;
            std::uint64_t submissionSerial = 0U;
            mgfx::ProceduralFieldMapping mapping{};
            mgfx::ProceduralFieldContentIdentity identity{};
            [[nodiscard]] bool matches(bool volume, bool qualified, std::uint32_t current) const noexcept {
                return volume && qualified && generation != 0U && generation == current && submissionSerial != 0U;
            }
        };
        VolumePublication acceptedVolume{};
        VolumePublicationCounters volumeCounters{};
        std::uint64_t preparedAttempt = 0U;
        void settlePrepared(std::size_t index, bool accepted, bool attempted,
                            std::uint64_t serial) noexcept {
            const auto& candidate = preparedIngest[index];
            if (!accepted) {
                if (attempted) acceptedVolume = {};
                return;
            }
            if (candidate.replay) {
                if (!acceptedVolume.matches(true, true, candidate.generation) ||
                    acceptedVolume.index != index || acceptedVolume.identity != candidate.identity || serial == 0U) {
                    acceptedVolume = {};
                    return;
                }
                acceptedVolume.submissionSerial = serial;
                if (volumeCounters.acceptedReplays != UINT64_MAX) ++volumeCounters.acceptedReplays;
            } else if (candidate.volume && candidate.generation != 0U && serial != 0U && candidate.identity.known()) {
                acceptedVolume = {candidate.push, index, candidate.generation, serial, candidate.mapping, candidate.identity};
                if (volumeCounters.acceptedIngests != UINT64_MAX) ++volumeCounters.acceptedIngests;
            } else if (candidate.volume) {
                acceptedVolume = {};
            } else if (!candidate.volume) {
                // An accepted compact-mode frame also ends volume-mode
                // continuity, even when its output uses a different slot.
                acceptedVolume = {};
            }
        }
        // Declaration/recording is synchronous on the owner thread. Native
        // push commands copy these bytes before any later collection rewrites
        // them; GPU resources are retained separately by the frame lease.
        std::vector<PreparedIngest> preparedIngest;
        std::vector<std::string> instanceResourceNames;
        std::vector<std::string> volumeBufferResourceNames;
        std::vector<std::string> volumeTextureResourceNames;
        std::vector<std::string> computePassNames;
        std::vector<std::string> vertexReadyPassNames;

        ~GpuResources() {
            computePipeline.reset();
            volumeTransferPipeline.reset();
            descriptorAllocator.reset();
            if (computeDevice != VK_NULL_HANDLE) {
                if (computeSetLayout != VK_NULL_HANDLE) {
                    vkDestroyDescriptorSetLayout(computeDevice, computeSetLayout, nullptr);
                    computeSetLayout = VK_NULL_HANDLE;
                }
                if (volumeSetLayout != VK_NULL_HANDLE) {
                    vkDestroyDescriptorSetLayout(computeDevice, volumeSetLayout, nullptr);
                    volumeSetLayout = VK_NULL_HANDLE;
                }
            }
        }

        [[nodiscard]] std::uint64_t approxBytes() const noexcept {
            std::uint64_t total = 0U;
            for (const auto& buf : ingestBuffers) if (buf) total += buf->size();
            for (const auto& buf : ingestSpatialBuffers) if (buf) total += buf->size();
            for (const auto& img : ingestVolumeImages) if (img) total += img->allocationSize();
            for (const auto& buf : instanceBuffers) if (buf) total += buf->size();
            for (const auto& buf : proceduralUniformBuffers) if (buf) total += buf->size();
            return std::max<std::uint64_t>(1U, total);
        }
    };

    std::shared_ptr<GpuResources> gpu_ = std::make_shared<GpuResources>();
    mgfx::ProceduralGeometryRecipe volumeRecipe_ = mgfx::ProceduralGeometryRecipe::Field;
    mgfx::ProceduralFieldAppearance fieldAppearance_{};
    [[nodiscard]] mgfx::ProceduralPayload makeVolumePayload(
        const std::shared_ptr<GpuResources>& gpu, std::size_t index,
        const ProceduralPushConstants& push, const mgfx::ProceduralFieldMapping& mapping,
        mgfx::ProceduralFieldContentIdentity identity);
    SculptureFrameProducer* externalProducer_ = nullptr;
    EpiFeedbackFieldExternalTransport* canonicalFeedbackTransport_ = nullptr;

    struct ReliefDrawResources final {
        std::unique_ptr<mgfx::vk::Mesh> mesh;
        mgfx::MaterialHandle material = mgfx::kInvalidMaterial;
    };
    void prepareReliefDrawResources(mgfx::Engine& engine, ReliefDrawResources& resources);
    const SculptureReliefQuality reliefQuality_;
    struct StagedReliefSlot final {
        ReliefDrawResources draw;
        std::unique_ptr<mgfx::vk::Image> image;
    };
    struct StagedReliefCache final {
        std::array<StagedReliefSlot, EpiFeedbackFieldDeliveryRing::kSlotCount> slots{};
        std::size_t selectedSlot = 0U;
        epi::FeedbackFieldDescriptor key{};
    };
    // Equal numeric slot indices do not imply equal ownership. External draws
    // must never replace descriptors/geometry belonging to a staged cache hit.
    std::array<ReliefDrawResources, EpiFeedbackFieldDeliveryRing::kSlotCount> externalReliefSlots_{};
    StagedReliefCache stagedRelief_{};
    std::array<std::unique_ptr<mgfx::vk::Mesh>, 3> dynamicManifoldMeshRing_;
    std::array<std::uint64_t, 3> manifoldMeshSignatures_{};
    std::size_t manifoldFrameIndex_ = 0U;
    std::vector<mgfx::vk::Vertex> manifoldVertexScratch_;
};
} // namespace epi_sculpt
