#include "VulkanOpenCLSyncHelper.h"
#include <thread>
#include "vk/RenderGraph.h"
#include "SculptureRenderBridge.h"
#include "SculptureCompactObservation.h"
#include "SculptureVolumePresentation.h"
#include "EpimorphicReliefGenerator.h"
#include "SculptureReliefContourSettings.h"
#include "SculptureExternalDeliveryPolicy.h"
#include "EpiFeedbackFieldExternalTransport.h"
#include "ExperimentMistVolumeGpuAbi.h"

#include "mgfx/Engine.h"
#include "mgfx/MaterialLibrary.h"
#include "mgfx/scene/AssetValidation.h"
#include "mgfx/scene/ProceduralRealizationContract.h"
#include "mgfx/vk/Buffer.h"
#include "mgfx/vk/Descriptor.h"
#include "mgfx/vk/Pipeline.h"
#include "mgfx/vk/Mesh.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <stdexcept>

namespace epi_sculpt {

namespace {

// This allocator covers the sculpture bridge publishers in this process.
// Future procedural-field publishers must coordinate a unique namespace.
// Saturation is permanent: wrapping would falsely identify unrelated fields.
std::uint64_t allocateFieldPublisherNamespace() {
    static std::atomic<std::uint64_t> next{1U};
    auto value = next.load(std::memory_order_relaxed);
    while (value != UINT64_MAX) {
        if (next.compare_exchange_weak(value, value + 1U, std::memory_order_relaxed))
            return value;
    }
    throw std::overflow_error("sculpture field publisher namespaces exhausted");
}

// Graph epochs do not carry prior state automatically. The same derived slot
// can be read by later frames before another queued compute overwrite.
constexpr mgfx::vk::ResourceState kDerivedInstanceEntry{
    VK_IMAGE_LAYOUT_UNDEFINED,
    VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT |
        VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT,
    VK_ACCESS_2_SHADER_WRITE_BIT | VK_ACCESS_2_SHADER_READ_BIT |
        VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT, 0};
constexpr mgfx::vk::ResourceState kDerivedVolumeEntry{
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
    VK_ACCESS_2_SHADER_WRITE_BIT | VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
    VK_IMAGE_ASPECT_COLOR_BIT};

// A canonical feedback delivery is one immutable image observed in three PBR
// roles: geometry displacement, albedo, and emission. Keeping that descriptor
// fan-out in one helper prevents staged and external-memory presentation paths
// from silently sampling different generations or texture bindings.
void bindCanonicalReliefField(
    mgfx::Engine& engine,
    mgfx::MaterialHandle material,
    const VkDescriptorImageInfo& imageInfo
) {
    const std::array<mgfx::MaterialImageBinding, 3> bindings{{
        {mgfx::pbr::kHeightBinding, imageInfo},
        {mgfx::pbr::kAlbedoBinding, imageInfo},
        {mgfx::pbr::kEmissiveBinding, imageInfo},
    }};
    engine.materials()->updateMaterialImageBindings(material, bindings);
}

// The transfer shader writes RGBA32F and the geometry shader samples it with
// trilinear spatial filtering. Verify that exact recipe before image
// allocation; changing to nearest silently would change the rendered field.
std::unique_ptr<mgfx::vk::Image> makeSculptureVolumeImage(mgfx::Engine& engine) {
    constexpr VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT;
    constexpr VkImageUsageFlags usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    constexpr VkFormatFeatureFlags required = VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT
        | VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;
    const VkPhysicalDevice physical = engine.context().device().physical();
    VkFormatProperties formatProperties{};
    vkGetPhysicalDeviceFormatProperties(physical, format, &formatProperties);
    if ((formatProperties.optimalTilingFeatures & required) != required)
        throw std::runtime_error("sculpture RGBA32F volume requires storage, sampling and linear filtering support");
    VkImageFormatProperties imageProperties{};
    const VkResult result = vkGetPhysicalDeviceImageFormatProperties(
        physical, format, VK_IMAGE_TYPE_3D, VK_IMAGE_TILING_OPTIMAL, usage, 0, &imageProperties);
    if (result != VK_SUCCESS || imageProperties.maxExtent.width < gpu_abi::kVolumeGridSize
        || imageProperties.maxExtent.height < gpu_abi::kVolumeGridSize
        || imageProperties.maxExtent.depth < gpu_abi::kVolumeGridSize
        || imageProperties.maxMipLevels < 1U || imageProperties.maxArrayLayers < 1U
        || !(imageProperties.sampleCounts & VK_SAMPLE_COUNT_1_BIT))
        throw std::runtime_error("sculpture RGBA32F 3D volume image recipe is unsupported");
    mgfx::vk::Image::CreateInfo image{};
    image.width = gpu_abi::kVolumeGridSize;
    image.height = gpu_abi::kVolumeGridSize;
    image.depth = gpu_abi::kVolumeGridSize;
    image.format = format;
    image.usage = usage;
    image.explicitMipLevels = 1;
    image.maxLod = 0.0f;
    image.addressU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    image.addressV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    image.addressW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    auto resultImage = std::make_unique<mgfx::vk::Image>(engine.context(), image);
    resultImage->primeLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    return resultImage;
}

} // namespace


SculptureRenderBridge::SculptureRenderBridge(SculptureReliefQuality reliefQuality)
    : reliefQuality_(reliefQuality) {
    if (!validSculptureReliefQuality(reliefQuality_))
        throw std::invalid_argument("unsupported sculpture relief quality");
}

SculptureRenderBridge::~SculptureRenderBridge() {
    gpu_.reset();
}


// -----------------------------------------------------------------------------
// Educational Annotation: EXTERNAL MEMORY HANDOFF
// -----------------------------------------------------------------------------
// This function initializes the Vulkan side of the Zero-Copy Interop.
// It allocates Vulkan buffers with exportFd capabilities so that OpenCL 
// can map the same VRAM pages, bypassing the PCIe bus entirely.
// -----------------------------------------------------------------------------
void SculptureRenderBridge::initializeGpuIngest(mgfx::Engine& engine, SculptureFrameProducer& producer) {
    if (!gpu_) gpu_ = std::make_shared<GpuResources>();
    auto& gpu = *gpu_;
    if (gpu.boundProducer && gpu.boundProducer != &producer)
        throw std::logic_error("sculpture GPU ingest requires a fresh bridge for a different producer");
    if (!gpu.boundProducer) {
        gpu.fieldPublisherNamespace = allocateFieldPublisherNamespace();
        gpu.fieldActivationEpoch = 1U;
        gpu.boundProducer = &producer;
    }
    if (!gpu.ingestBuffers.empty()) return;

    // GPU ingestion is always needed. External memory is only an optional
    // optimization after the producer has selected the same physical device
    // and established a real cross-API synchronization protocol.
    const bool supportsExternalMemory = engine.context().device().supportsExternalMemoryFd()
        && engine.context().device().supportsExternalSemaphoreFd()
        && producer.externalMemoryInteropReady();
    if (supportsExternalMemory) {
        gpu.externalSemaphoreDevice = VK_NULL_HANDLE;
        externalProducer_ = &producer;
    }
    
    // A host-only configuration needs no dormant export cohort. Its retired
    // frame slots below are the complete renderer-side ingest allocation.
    const std::size_t externalSlotCount = supportsExternalMemory
        ? SculptureFrameProducer::kZeroCopySlotCount : 0U;
    for (std::size_t i = 0; i < externalSlotCount; ++i) {
        gpu.ingestBuffers.push_back(std::make_unique<mgfx::vk::Buffer>(
            engine.context(),
            sizeof(epi_sculpt::gpu_abi::PackedSculptureSnapshot),
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            mgfx::vk::BufferLocation::HostVisible,
            supportsExternalMemory // exportFd
        ));
        gpu.ingestSpatialBuffers.push_back(std::make_unique<mgfx::vk::Buffer>(
            engine.context(),
            sizeof(float) * 4 * gpu_abi::kVolumeVoxelCount,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            mgfx::vk::BufferLocation::DeviceLocal, // It's only needed on the GPU!
            supportsExternalMemory // exportFd
        ));
        SculptureFrameProducer::ZeroCopyBuffer slot{
            supportsExternalMemory ? gpu.ingestBuffers[i]->exportFd() : -1,
            sizeof(epi_sculpt::gpu_abi::PackedSculptureSnapshot),
            gpu.ingestBuffers[i]->mapped(),
            supportsExternalMemory ? gpu.ingestSpatialBuffers[i]->exportFd() : -1,
            sizeof(float) * 4 * gpu_abi::kVolumeVoxelCount
        };
        if (supportsExternalMemory) {
            ExternalSemaphorePair pair{
                std::make_unique<mgfx::vk::Semaphore>(engine.context(), true),
                std::make_unique<mgfx::vk::Semaphore>(engine.context(), true)};
            slot.vulkanReleaseSemaphoreFd = pair.vulkanRelease->exportFd();
            slot.openClReleaseSemaphoreFd = pair.openClRelease->exportFd();
            if (pair.vulkanRelease == nullptr || pair.openClRelease == nullptr
                || slot.vulkanReleaseSemaphoreFd < 0 || slot.openClReleaseSemaphoreFd < 0) {
                // handled by unique_ptr
                gpu.externalSemaphores.clear();
                externalProducer_ = nullptr;
                slot.vulkanReleaseSemaphoreFd = -1;
                slot.openClReleaseSemaphoreFd = -1;
            } else {
                gpu.externalSemaphores.push_back(std::move(pair));
            }
        }
        gpu.ingestSlots.push_back(slot);
        
        engine.renderGraph().registerBuffer("SculptureVolume." + std::to_string(i), gpu.ingestSpatialBuffers[i]->handle());
        
        gpu.ingestVolumeImages.push_back(makeSculptureVolumeImage(engine));
        engine.renderGraph().registerImage("SculptureVolumeTex." + std::to_string(i), gpu.ingestVolumeImages[i]->handle(), VK_IMAGE_ASPECT_COLOR_BIT, mgfx::vk::ResourceState{VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_2_NONE, 0, 0});
    }
    // Host deliveries never write exported/imported OpenCL storage. Engine
    // collection runs after the active frame slot's fence wait, so each of
    // these separate slots can be rewritten for that frame only.
    gpu.hostSlotBase = gpu.ingestBuffers.size();
    for (std::uint32_t frame = 0; frame < engine.frameLoop().framesInFlight(); ++frame) {
        gpu.ingestBuffers.push_back(std::make_unique<mgfx::vk::Buffer>(
            engine.context(), sizeof(gpu_abi::PackedSculptureSnapshot),
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, mgfx::vk::BufferLocation::HostVisible));
        gpu.ingestSpatialBuffers.push_back(std::make_unique<mgfx::vk::Buffer>(
            engine.context(), sizeof(float) * 4 * gpu_abi::kVolumeVoxelCount,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, mgfx::vk::BufferLocation::HostVisible));
        gpu.ingestVolumeImages.push_back(makeSculptureVolumeImage(engine));
    }
    if (supportsExternalMemory
        && gpu.externalSemaphores.size() == SculptureFrameProducer::kZeroCopySlotCount) {
        producer.importZeroCopyBuffers(gpu.ingestSlots);
    }
}

void SculptureRenderBridge::initCompute(mgfx::Engine& engine) {
    if (!gpu_) gpu_ = std::make_shared<GpuResources>();
    auto& gpu = *gpu_;
    if (gpu.computeInitialized) return;
    if (gpu.computeInitializationFailed)
        throw std::runtime_error("sculpture compute initialization previously failed");
    // A partial candidate is never reused after an exception. Its resources
    // stay owned until the presentation island is retired.
    gpu.computeInitializationFailed = true;
    gpu.computeDevice = engine.context().device().logical();
    
    // Allocate descriptor allocator
    const std::size_t ringSize = gpu.ingestBuffers.size();
    if (ringSize == 0U) throw std::logic_error("sculpture ingest is not initialized");
    std::vector<VkDescriptorPoolSize> poolSizes = {
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, static_cast<std::uint32_t>(3 * ringSize + 4) },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, static_cast<std::uint32_t>(ringSize) },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, static_cast<std::uint32_t>(ringSize + 4) },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, static_cast<std::uint32_t>(ringSize + 4) }
    };
    gpu.descriptorAllocator = std::make_unique<mgfx::vk::DescriptorAllocator>(engine.context(), 2 * ringSize + 4, poolSizes);
    
    // Create descriptor set layout
    mgfx::vk::DescriptorSetLayoutBuilder layoutBuilder;
    layoutBuilder.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 1);
    layoutBuilder.addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 1);
    // Bind 2 is now a combined image sampler, but we need to change procedural_world.frag!
    layoutBuilder.addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 1);
    // Binding 3 is a versioned experiment-optics UBO when the MIST stage is
    // active. Ordinary sculpture payloads bind a zeroed compatible record.
    layoutBuilder.addBinding(3, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 1);
    VkDescriptorSetLayout setLayout = layoutBuilder.build(gpu.computeDevice);
    gpu.computeSetLayout = setLayout;
    
    mgfx::vk::DescriptorSetLayoutBuilder volLayoutBuilder;
    volLayoutBuilder.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 1);
    volLayoutBuilder.addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 1);
    gpu.volumeSetLayout = volLayoutBuilder.build(gpu.computeDevice);

    // Create compute pipeline
    mgfx::vk::Pipeline::ComputeCreateInfo cci;
    cci.computeShaderPath = "shaders/expand_instances.comp.spv";
    cci.pushConstantSize = sizeof(ProceduralPushConstants);
    cci.pushConstantStages = VK_SHADER_STAGE_COMPUTE_BIT;
    cci.setLayouts = { setLayout };
    gpu.computePipeline = std::make_unique<mgfx::vk::Pipeline>(engine.context(), cci);
    
    mgfx::vk::Pipeline::ComputeCreateInfo vci;
    vci.computeShaderPath = "shaders/volume_transfer.comp.spv";
    vci.pushConstantSize = 0;
    vci.setLayouts = { gpu.volumeSetLayout };
    gpu.volumeTransferPipeline = std::make_unique<mgfx::vk::Pipeline>(engine.context(), vci);
    
    gpu.instanceBuffers.resize(ringSize);
    gpu.proceduralUniformBuffers.resize(ringSize);
    gpu.descriptorSets.resize(ringSize);
    gpu.volumeDescriptorSets.resize(ringSize);
    gpu.preparedIngest.resize(ringSize);
    
    gpu.instanceResourceNames.resize(ringSize);
    gpu.volumeBufferResourceNames.resize(ringSize);
    gpu.volumeTextureResourceNames.resize(ringSize);
    gpu.computePassNames.resize(ringSize);
    gpu.vertexReadyPassNames.resize(ringSize);
    
    for (std::size_t i = 0; i < ringSize; ++i) {
        gpu.descriptorSets[i] = gpu.descriptorAllocator->allocate(setLayout);
        gpu.volumeDescriptorSets[i] = gpu.descriptorAllocator->allocate(gpu.volumeSetLayout);
        
        gpu.instanceBuffers[i] = std::make_unique<mgfx::vk::Buffer>(
            engine.context(), 
            sizeof(mgfx::vk::InstanceData) * kDirectInstanceCapacity,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
            mgfx::vk::BufferLocation::DeviceLocal);
        gpu.proceduralUniformBuffers[i] = std::make_unique<mgfx::vk::Buffer>(
            engine.context(), experiment_mist_volume_gpu_abi::kOpticsBytes, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            mgfx::vk::BufferLocation::HostVisible);
        const std::array<std::byte, experiment_mist_volume_gpu_abi::kOpticsBytes> zeroOptics{};
        gpu.proceduralUniformBuffers[i]->write(zeroOptics.data(), sizeof(zeroOptics));
        gpu.instanceResourceNames[i] = "SculptureInstanceOutput." + std::to_string(i);
        gpu.volumeBufferResourceNames[i] = "SculptureVolume." + std::to_string(i);
        gpu.volumeTextureResourceNames[i] = "SculptureVolumeTex." + std::to_string(i);
        gpu.computePassNames[i] = "SculptureCompute." + std::to_string(i);
        gpu.vertexReadyPassNames[i] = "SculptureComputeToVertex." + std::to_string(i);
        // The graph is reset after draw collection. Register this buffer in
        // the deferred declaration for the current frame, not during setup.
        VkDescriptorBufferInfo inInfo{};
        inInfo.buffer = gpu.ingestBuffers.empty() ? VK_NULL_HANDLE : gpu.ingestBuffers[i]->handle();
        inInfo.offset = 0;
        inInfo.range = VK_WHOLE_SIZE;
        
        VkDescriptorBufferInfo outInfo{};
        outInfo.buffer = gpu.instanceBuffers[i]->handle();
        outInfo.offset = 0;
        outInfo.range = VK_WHOLE_SIZE;
        
        VkDescriptorImageInfo imageInfo = gpu.ingestVolumeImages[i]->descriptorInfo();
        
        if (inInfo.buffer != VK_NULL_HANDLE) {
            VkDescriptorBufferInfo uniformInfo{};
            uniformInfo.buffer = gpu.proceduralUniformBuffers[i]->handle();
            uniformInfo.range = experiment_mist_volume_gpu_abi::kOpticsBytes;
            VkWriteDescriptorSet writes[4] = {};
            writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[0].dstSet = gpu.descriptorSets[i];
            writes[0].dstBinding = 0;
            writes[0].descriptorCount = 1;
            writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            writes[0].pBufferInfo = &inInfo;
            
            writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[1].dstSet = gpu.descriptorSets[i];
            writes[1].dstBinding = 1;
            writes[1].descriptorCount = 1;
            writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            writes[1].pBufferInfo = &outInfo;
            
            writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[2].dstSet = gpu.descriptorSets[i];
            writes[2].dstBinding = 2;
            writes[2].descriptorCount = 1;
            writes[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            writes[2].pImageInfo = &imageInfo;
            writes[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[3].dstSet = gpu.descriptorSets[i];
            writes[3].dstBinding = 3;
            writes[3].descriptorCount = 1;
            writes[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            writes[3].pBufferInfo = &uniformInfo;
            
            vkUpdateDescriptorSets(gpu.computeDevice, 4, writes, 0, nullptr);
        }
        
        VkDescriptorBufferInfo spatialInfo{};
        spatialInfo.buffer = gpu.ingestSpatialBuffers.empty() ? VK_NULL_HANDLE : gpu.ingestSpatialBuffers[i]->handle();
        spatialInfo.offset = 0;
        spatialInfo.range = VK_WHOLE_SIZE;
        
        VkDescriptorImageInfo volImageInfo = gpu.ingestVolumeImages[i]->descriptorInfo();
        volImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        
        if (spatialInfo.buffer != VK_NULL_HANDLE) {
            VkWriteDescriptorSet writes[2] = {};
            writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[0].dstSet = gpu.volumeDescriptorSets[i];
            writes[0].dstBinding = 0;
            writes[0].descriptorCount = 1;
            writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            writes[0].pBufferInfo = &spatialInfo;
            
            writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[1].dstSet = gpu.volumeDescriptorSets[i];
            writes[1].dstBinding = 1;
            writes[1].descriptorCount = 1;
            writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            writes[1].pImageInfo = &volImageInfo;
            
            vkUpdateDescriptorSets(gpu.computeDevice, 2, writes, 0, nullptr);
        }
    }
    gpu.computeInitialized = true;
    gpu.computeInitializationFailed = false;
}

void SculptureRenderBridge::setVolumeRecipe(mgfx::ProceduralGeometryRecipe recipe) {
    if (!mgfx::validProceduralGeometryRecipe(recipe))
        throw std::invalid_argument("unsupported sculpture volume recipe");
    volumeRecipe_ = recipe;
}

void SculptureRenderBridge::setFieldAppearance(const mgfx::ProceduralFieldAppearance& appearance) {
    if (!appearance.valid())
        throw std::invalid_argument("unsupported sculpture field appearance");
    fieldAppearance_ = appearance;
}

bool SculptureRenderBridge::fieldCandidateAccepted(
    const SculptureFieldCaptureCandidate& candidate, std::uint64_t serial) const noexcept {
    if (!gpu_ || !candidate.valid() || !fieldCandidate_ || serial == 0U ||
        candidate.attempt != collectedAttempt_ || candidate.attempt != gpu_->preparedAttempt ||
        fieldCandidate_->identity != candidate.identity || fieldCandidate_->mapping != candidate.mapping ||
        fieldCandidate_->appearance != candidate.appearance || fieldCandidate_->recipe != candidate.recipe)
        return false;
    const auto& accepted = gpu_->acceptedVolume;
    return accepted.submissionSerial == serial && accepted.identity == candidate.identity &&
        accepted.mapping == candidate.mapping;
}

mgfx::ProceduralPayload SculptureRenderBridge::makeVolumePayload(
        const std::shared_ptr<GpuResources>& gpu, std::size_t index,
        const ProceduralPushConstants& push, const mgfx::ProceduralFieldMapping& mapping,
        mgfx::ProceduralFieldContentIdentity identity) {
    if (!gpu || index >= gpu->descriptorSets.size() ||
        index >= gpu->ingestVolumeImages.size() || !gpu->ingestVolumeImages[index])
        throw std::logic_error("sculpture volume image or descriptor is unavailable");
    const auto& image = *gpu->ingestVolumeImages[index];
    mgfx::ProceduralPayload payload{};
    payload.descriptorSet = gpu->descriptorSets[index];
    payload.resourceOwner = gpu;
    payload.retainedResourceBytes = gpu->approxBytes();
    payload.setPushConstants(push);
    payload.fieldMapping = mapping;
    payload.fieldIdentity = identity;
    payload.geometryRecipe = volumeRecipe_;
    payload.fieldAppearance = fieldAppearance_;
    payload.sampledImage = {
        .graphResourceName = gpu->volumeTextureResourceNames[index],
        .image = image.handle(), .imageView = image.view(), .sampler = image.sampler(),
        .format = image.format(), .extent = {image.width(), image.height(), image.depth()},
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .sampledLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .sampledStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
        .sampledAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
    };
    return payload;
}

SculptureRenderBridge::VolumePublicationCounters
SculptureRenderBridge::volumePublicationCounters() const noexcept {
    return gpu_ ? gpu_->volumeCounters : VolumePublicationCounters{};
}

CullingFrustum SculptureRenderBridge::extractFrustum(const mgfx::math::mat4& vp) noexcept {
    CullingFrustum frustum;
    const mgfx::math::vec4 r0(vp[0][0], vp[1][0], vp[2][0], vp[3][0]);
    const mgfx::math::vec4 r1(vp[0][1], vp[1][1], vp[2][1], vp[3][1]);
    const mgfx::math::vec4 r2(vp[0][2], vp[1][2], vp[2][2], vp[3][2]);
    const mgfx::math::vec4 r3(vp[0][3], vp[1][3], vp[2][3], vp[3][3]);

    const std::array<mgfx::math::vec4, 6> rawPlanes = {
        r3 + r0, // Left
        r3 - r0, // Right
        r3 + r1, // Bottom
        r3 - r1, // Top
        r2,      // Near (Vulkan 0..1)
        r3 - r2  // Far
    };

    for (std::size_t i = 0; i < 6; ++i) {
        const auto& raw = rawPlanes[i];
        const float len = std::sqrt(raw.x * raw.x + raw.y * raw.y + raw.z * raw.z);
        if (len > 1.0e-6f) {
            frustum.planes[i].normal = mgfx::math::vec3(raw.x / len, raw.y / len, raw.z / len);
            frustum.planes[i].distance = raw.w / len;
        } else {
            frustum.planes[i].normal = mgfx::math::vec3(0.0f);
            frustum.planes[i].distance = 0.0f;
        }
    }
    return frustum;
}

void SculptureRenderBridge::rebuild(
    const gpu_abi::PackedSculptureSnapshot* snapshot) {
    rebuild(snapshot, {});
}

SculptureLodTier SculptureRenderBridge::calculateLodTier(
    float distanceToCam, const SculptureLodConfig& config) noexcept {
    const float bias = std::max(0.01f, config.lodBias);
    const float nearD = config.nearDistance * bias;
    const float farD = config.farDistance * bias;
    const float cullD = config.cullDistance * bias;

    if (distanceToCam >= cullD) {
        return SculptureLodTier::LOD_Culled;
    }
    if (distanceToCam >= farD) {
        return SculptureLodTier::LOD2_Low;
    }
    if (distanceToCam >= nearD) {
        return SculptureLodTier::LOD1_Medium;
    }
    return SculptureLodTier::LOD0_Full;
}

void SculptureRenderBridge::rebuild(
    const gpu_abi::PackedSculptureSnapshot* snapshot,
    const InstanceFilterOptions& filterOptions) {
    instanceCount_ = 0U;
    activeBatchCount_ = 0U;
    droppedSampleCount_ = 0U;
    prunedSampleCount_ = 0U;
    culledSampleCount_ = 0U;
    lodStats_ = {};
    boundingVolume_ = {};
    motionKinematics_ = {};
    generation_ = snapshot ? snapshot->frame.generation : 0U;
    renderPath_ = SculptureRenderPath::NoRenderableSnapshot;
    for (auto& batch : cpuBatches_) batch.clear();
    if (!snapshot) return;

    CullingFrustum activeFrustum;
    bool useFrustum = false;
    if (filterOptions.frustum) {
        activeFrustum = *filterOptions.frustum;
        useFrustum = true;
    } else if (filterOptions.viewProjMatrix) {
        activeFrustum = extractFrustum(*filterOptions.viewProjMatrix);
        useFrustum = true;
    }

    const bool useDynamicLod = filterOptions.lodConfig.enableDynamicLod && (filterOptions.cameraPosition != nullptr);

    mgfx::math::vec3 minB(std::numeric_limits<float>::infinity());
    mgfx::math::vec3 maxB(-std::numeric_limits<float>::infinity());
    mgfx::math::vec3 totalVel(0.0f);
    float maxSpd = 0.0f;
    float totalKinetic = 0.0f;
    std::size_t validSampleCount = 0;

    const std::size_t sampleCount = std::min<std::size_t>(
        snapshot->activeInstanceCount, snapshot->samples.size());
    for (std::size_t index = 0; index < sampleCount; ++index) {
        const auto& sample = snapshot->samples[index];
        const auto& position = sample.positionRadius;
        const auto& color = sample.colorTint;
        const bool valid = std::isfinite(position[0]) && std::isfinite(position[1])
            && std::isfinite(position[2]) && std::isfinite(position[3])
            && std::isfinite(color[0]) && std::isfinite(color[1])
            && std::isfinite(color[2]) && std::isfinite(color[3])
            && position[3] > 0.0f;
        if (!valid) {
            ++droppedSampleCount_;
            continue;
        }

        if (filterOptions.minDensityThreshold > 0.0f && position[3] < filterOptions.minDensityThreshold) {
            ++prunedSampleCount_;
            continue;
        }

        const mgfx::math::vec3 center(position[0], position[1], position[2]);
        if (useFrustum) {
            if (!activeFrustum.isSphereInside(center, position[3])) {
                ++culledSampleCount_;
                continue;
            }
        }

        float radiusScale = 1.0f;
        if (useDynamicLod) {
            const float distToCam = glm::distance(center, *filterOptions.cameraPosition);
            const auto tier = calculateLodTier(distToCam, filterOptions.lodConfig);
            std::size_t stride = 1U;
            switch (tier) {
            case SculptureLodTier::LOD0_Full:
                stride = 1U;
                radiusScale = 1.0f;
                ++lodStats_.lod0Count;
                break;
            case SculptureLodTier::LOD1_Medium:
                stride = 2U;
                radiusScale = 1.41421356f;
                ++lodStats_.lod1Count;
                break;
            case SculptureLodTier::LOD2_Low:
                stride = 4U;
                radiusScale = 2.0f;
                ++lodStats_.lod2Count;
                break;
            case SculptureLodTier::LOD_Culled:
                stride = 0U;
                radiusScale = 0.0f;
                ++culledSampleCount_;
                ++lodStats_.lodCulledCount;
                break;
            }
            if (stride == 0U || (index % stride != 0U)) {
                continue;
            }
        } else {
            ++lodStats_.lod0Count;
        }

        const auto band = std::min<std::size_t>(
            static_cast<std::size_t>(std::max(0.0f, color[3])),
            kMaterialBandCount - 1U);
        auto& instance = cpuBatches_[band].emplace_back();
        const float effectiveRadius = position[3] * radiusScale;
        instance.posRadius = mgfx::math::vec4(position[0], position[1], position[2], effectiveRadius);
        instance.baseColorTint = mgfx::math::vec4(color[0], color[1], color[2], 1.0f);
        instance.pbrOverride = mgfx::math::vec4(
            sample.normalVector[0], sample.normalVector[1], sample.normalVector[2], sample.normalVector[3]);
        instance.velocity = mgfx::math::vec4(
            sample.velocity[0], sample.velocity[1], sample.velocity[2], sample.velocity[3]);
        ++instanceCount_;

        // Dynamic Bounding Volume & Kinematics Accumulation
        const float r = effectiveRadius;
        minB = glm::min(minB, center - mgfx::math::vec3(r));
        maxB = glm::max(maxB, center + mgfx::math::vec3(r));

        const mgfx::math::vec3 vel(sample.velocity[0], sample.velocity[1], sample.velocity[2]);
        const float spd = glm::length(vel);
        totalVel += vel;
        maxSpd = std::max(maxSpd, spd);
        totalKinetic += 0.5f * spd * spd;
        ++validSampleCount;
    }

    if (validSampleCount > 0) {
        boundingVolume_.minBounds = {minB.x, minB.y, minB.z};
        boundingVolume_.maxBounds = {maxB.x, maxB.y, maxB.z};
        boundingVolume_.center = {0.5f * (minB.x + maxB.x), 0.5f * (minB.y + maxB.y), 0.5f * (minB.z + maxB.z)};
        boundingVolume_.boundingRadius = glm::length(maxB - mgfx::math::vec3(boundingVolume_.center.x, boundingVolume_.center.y, boundingVolume_.center.z));
        boundingVolume_.valid = true;

        motionKinematics_.averageVelocity = {
            totalVel.x / static_cast<float>(validSampleCount),
            totalVel.y / static_cast<float>(validSampleCount),
            totalVel.z / static_cast<float>(validSampleCount)
        };
        motionKinematics_.maxSpeed = maxSpd;
        motionKinematics_.kineticEnergy = totalKinetic;
        motionKinematics_.activeCount = validSampleCount;
        motionKinematics_.valid = true;
    }

    for (const auto& batch : cpuBatches_) {
        activeBatchCount_ += !batch.empty();
    }
    if (instanceCount_ != 0U) {
        renderPath_ = SculptureRenderPath::StagedCpuInstances;
    }
}


// -----------------------------------------------------------------------------
// Educational Annotation: FAULT-TOLERANT RENDERING PIPELINE
// -----------------------------------------------------------------------------
// This method orchestrates the rendering path. It dynamically chooses between:
// 1. Zero-Copy External Memory (Direct GPU ingestion)
// 2. CPU Fallback (Staged CPU instances)
//
// By isolating this decision here, the renderer remains robust even if the 
// OpenCL kernel panics or the GPU driver disconnects the shared memory.
// -----------------------------------------------------------------------------
bool SculptureRenderBridge::canAcquireExternalDelivery(
    const SculptureSnapshotDelivery& delivery) const noexcept {
    return gpu_ != nullptr
        && delivery.usesExternalMemory()
        && delivery.matchesSnapshot()
        && delivery.slotIndex < gpu_->externalSemaphores.size()
        && externalProducer_ != nullptr
        && gpu_->externalSemaphores[delivery.slotIndex].openClRelease != nullptr
        && gpu_->externalSemaphores[delivery.slotIndex].vulkanRelease != nullptr;
}

void SculptureRenderBridge::enqueueAcquiredExternalDeliveryHandoff(
    mgfx::Drawable& draw,
    const SculptureSnapshotDelivery& delivery) {
    // The producer already moved this exact delivery to VulkanReading. First
    // register its terminal transition: FrameLoop resolves it false for every
    // recording/submission failure, or true immediately after queue submit.
    // Only then append the external semaphore transaction, so an exception
    // during handoff setup cannot strand a ticket in VulkanReading.
    bool commitQueued = false;
    try {
        commitQueued = draw.engine().frameLoop().enqueueSubmitCommit(
            [producer = externalProducer_, delivery](bool success) noexcept {
                if (success) {
                    static_cast<void>(producer->releaseExternalDelivery(delivery));
                } else {
                    producer->cancelExternalDelivery(delivery);
                }
            });
    } catch (...) {
        // A throwing registration leaves this acquisition locally owned.
        // Normal refusal instead invokes the closure inline with false.
        externalProducer_->cancelExternalDelivery(delivery);
        throw;
    }
    if (!commitQueued) {
        // The closure already cancelled the ticket. Abort outside the local
        // cleanup catch so this refusal cannot cancel the same epoch twice.
        throw std::logic_error(
            "compact external commit capacity changed after preflight");
    }

    // The queue wait makes the OpenCL release visible before any Vulkan work
    // reads the imported slot; the signal returns ownership after submission.
    if (!gpu_ || !VulkanOpenCLSyncHelper::enqueueHandoff(
            draw.engine(),
            gpu_->externalSemaphores[delivery.slotIndex].openClRelease.get(),
            gpu_->externalSemaphores[delivery.slotIndex].vulkanRelease.get())) {
        // The caller preflights the complete transaction before acquiring the
        // delivery. Reaching this path after commit registration would leave a
        // terminal callback without its semaphore pair, so abort recording.
        throw std::logic_error(
            "compact external handoff capacity changed after preflight");
    }
}

bool SculptureRenderBridge::retireExternalDelivery(
    mgfx::Drawable& draw,
    const SculptureSnapshotDelivery& delivery) {
    if (!canAcquireExternalDelivery(delivery)) return false;

    // Admission is checked before moving the producer ticket to VulkanReading.
    // A full frame-local synchronization channel is ordinary backpressure: the
    // immutable delivery stays available and can be retried next frame.
    if (!draw.engine().frameLoop().canEnqueueSubmission({
            .externalWaits = 1U,
            .externalSignals = 1U,
            .commitCallbacks = 1U,
        })) {
        return false;
    }

    if (!externalProducer_->acquireExternalDelivery(delivery)) {
        return false;
    }

    // Once registered, FrameLoop is the sole terminalizer: it calls the
    // callback false for any later recording/submission failure. This avoids
    // a second cancellation and preserves exact-ticket telemetry.
    enqueueAcquiredExternalDeliveryHandoff(draw, delivery);
    return true;
}

bool SculptureRenderBridge::drawDirectly(
    mgfx::Drawable& draw,
    const mgfx::vk::Mesh& mesh,
    std::span<const mgfx::MaterialHandle> materials,
    const mgfx::math::mat4& rootTransform,
    const SculptureProgramParams& params,
    const SculptureSnapshotDelivery& delivery,
    const InstanceFilterOptions& filterOptions) {
    
    // -------------------------------------------------------------------------
    // Educational Annotation: ZERO-COPY HOST INGESTION (SINGLE BATCH)
    // Now that OpenCL fully computes the continuous HSV material color, we no
    // longer need to split draws by discrete material bands. We dispatch a
    // single bulk instanced draw call and tint every instance dynamically.
    // -------------------------------------------------------------------------

    collectedAttempt_ = 0U;
    collectedGeneration_ = 0U;
    fieldCandidate_.reset();
    // Every presentation attempt starts without observations. This also covers
    // replay and early deferral, which need not read a new analytics sidecar.
    rebuild(nullptr, filterOptions);
    const bool isVolumeMode = params.outputMode == gpu_abi::kOutputModeVolume;
    if (isVolumeMode) draw.engine().setProceduralPayload({});
    if (materials.empty()) {
        // A completed external frame is still a synchronization obligation
        // when presentation has no material. Drain its OpenCL signal through
        // Vulkan and return the slot instead of eventually filling the ring
        // with READY frames that can never be reused.
        static_cast<void>(retireExternalDelivery(draw, delivery));
        rebuild(nullptr, filterOptions);
        renderPath_ = SculptureRenderPath::NoRenderableSnapshot;
        return false;
    }

    if (!gpu_ || gpu_->ingestBuffers.empty()) {
        static_cast<void>(retireExternalDelivery(draw, delivery));
        rebuild(nullptr, filterOptions);
        return false;
    }
    const std::shared_ptr<GpuResources> gpu = gpu_;
    const auto attempt = draw.engine().activeFrameAttempt();
    if (!draw.engine().frameLoop().frameActive() || attempt == 0U || gpu->preparedAttempt == attempt)
        throw std::logic_error("sculpture ingest requires one preparation per active frame attempt");
    gpu->preparedAttempt = attempt;
    if (delivery.matchesSnapshot()) {
        if (gpu->lastFieldGeneration > 0U && delivery.generation < gpu->lastFieldGeneration) {
            ++gpu->fieldActivationEpoch;
        }
        gpu->lastFieldGeneration = delivery.generation;
    }
    const mgfx::ProceduralFieldContentIdentity fieldIdentity{
        gpu->fieldPublisherNamespace, gpu->fieldActivationEpoch, delivery.generation};
    const bool replayEligible = fieldIdentity.known() && gpu->acceptedVolume.identity == fieldIdentity
        && gpu->acceptedVolume.matches(isVolumeMode, delivery.matchesSnapshot(), delivery.generation);
    std::optional<mgfx::ProceduralFieldMapping> newVolumeMapping;
    if (isVolumeMode && !replayEligible) {
        newVolumeMapping = makeSculptureVolumeFieldMapping(rootTransform);
        if (!newVolumeMapping) {
            static_cast<void>(retireExternalDelivery(draw, delivery));
            rebuild(nullptr, filterOptions);
            renderPath_ = SculptureRenderPath::NoRenderableSnapshot;
            return false;
        }
    }
    const std::size_t hostIndex = gpu->hostSlotBase + draw.engine().activeFrameIndex();
    if (hostIndex >= gpu->ingestBuffers.size() || hostIndex >= gpu->ingestSpatialBuffers.size())
        throw std::logic_error("sculpture host ingest frame slot is unavailable");
    // Complete all native allocations and reserve lifetime before moving an
    // external producer ticket into Vulkan ownership.
    initCompute(draw.engine());
    if (!draw.engine().retainFramePayloadResources(gpu, gpu->approxBytes())) {
        static_cast<void>(retireExternalDelivery(draw, delivery));
        rebuild(nullptr, filterOptions);
        return false;
    }

    auto& frameLoop = draw.engine().frameLoop();
    if (!frameLoop.canEnqueueSubmission({.commitCallbacks = 1U})) return false;
    const auto queueSettlement = [&frameLoop, owner = gpu](std::size_t index) {
        return frameLoop.enqueueSubmitCommit(
            [owner, loop = &frameLoop, index](bool accepted) noexcept {
                owner->settlePrepared(index, accepted, loop->queueSubmissionAttempted(),
                                      accepted ? loop->lastSubmittedSerial() : 0U);
            });
    };

    // A previously accepted renderer-owned derivative needs no second CL
    // acquisition or source read. The application has already qualified the
    // current delivery against its semantic route/lease and generation floor.
    if (replayEligible) {
        const auto index = gpu->acceptedVolume.index;
        if (index >= gpu->preparedIngest.size())
            throw std::logic_error("cached sculpture volume slot is unavailable");
        gpu->preparedIngest[index] = {gpu->acceptedVolume.push, true, true, delivery.generation,
                                      gpu->acceptedVolume.mapping, gpu->acceptedVolume.identity};
        if (!queueSettlement(index))
            throw std::logic_error("sculpture replay settlement capacity changed after preflight");
        draw.engine().deferGraphPass([owner = gpu.get(), index](mgfx::vk::RenderGraph& graph) {
            const auto instances = graph.registerBuffer(
                owner->instanceResourceNames[index], owner->instanceBuffers[index]->handle(),
                kDerivedInstanceEntry);
            const auto volume = graph.registerImage(
                owner->volumeTextureResourceNames[index], owner->ingestVolumeImages[index]->handle(),
                VK_IMAGE_ASPECT_COLOR_BIT, kDerivedVolumeEntry);
            graph.addPass(owner->vertexReadyPassNames[index])
                .readBuffer(instances, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT)
                .read(volume, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                      VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT)
                .execute([](VkCommandBuffer) {});
        });
        draw.engine().setProceduralPayload(makeVolumePayload(
            gpu, index, gpu->acceptedVolume.push, gpu->acceptedVolume.mapping, gpu->acceptedVolume.identity));
        fieldCandidate_ = SculptureFieldCaptureCandidate{attempt, gpu->preparedIngest[index].identity,
            gpu->preparedIngest[index].mapping, fieldAppearance_, volumeRecipe_};
        generation_ = delivery.generation;
        instanceCount_ = gpu->acceptedVolume.push.activeCount;
        lodStats_.lod0Count = instanceCount_;
        publishMappedCompactObservation(instanceCount_, boundingVolume_, motionKinematics_);
        renderPath_ = SculptureRenderPath::MappedCompute;
        collectedAttempt_ = attempt;
        collectedGeneration_ = delivery.generation;
        return true;
    }

    const bool externalStorage =
        delivery.storage == SculptureSnapshotStorage::ExternalMemory;
    const bool externalMetadataValid = delivery.usesExternalMemory()
        && delivery.matchesSnapshot();
    const bool externalSubmissionCapacity =
        draw.engine().frameLoop().canEnqueueSubmission({
            .externalWaits = 1U,
            .externalSignals = 1U,
            .commitCallbacks = 2U,
        });
    const bool externalResourcesReady = canAcquireExternalDelivery(delivery)
        && (!isVolumeMode || delivery.hasExternalSpatial())
        && externalSubmissionCapacity;
    const bool externalAcquireAccepted = externalResourcesReady
        && externalProducer_->acquireExternalDelivery(delivery);

    // If acquisition fails before submission, never read the shared mapping.
    // The producer keeps a generation-matched private copy specifically for
    // this path. A missing private copy drops one frame instead of inventing
    // unsafe ownership.
    const SculptureSnapshotDelivery stagedDelivery = externalStorage
        ? (externalProducer_ != nullptr
               ? externalProducer_->safeStagedDelivery()
               : SculptureSnapshotDelivery{})
        : delivery;
    const bool safeHostSnapshotAvailable = stagedDelivery.matchesSnapshot()
        && stagedDelivery.storage == SculptureSnapshotStorage::HostStaged
        && (!isVolumeMode || stagedDelivery.hasHostSpatial())
        && (!externalStorage
            || stagedDelivery.generation == delivery.generation);
    const SculptureDeliveryAction preSubmitAction = decidePreSubmitDelivery({
        externalMetadataValid,
        externalResourcesReady,
        externalAcquireAccepted,
        safeHostSnapshotAvailable,
    });

    const bool externalDelivery =
        preSubmitAction == SculptureDeliveryAction::UseExternal;
    const auto* snapshot = externalDelivery
        ? delivery.snapshot
        : preSubmitAction == SculptureDeliveryAction::UseHostStaged
            ? stagedDelivery.snapshot
            : nullptr;
    if (!snapshot) {
        if (externalStorage && !externalAcquireAccepted
            && isVolumeMode && !delivery.hasExternalSpatial())
            static_cast<void>(retireExternalDelivery(draw, delivery));
        rebuild(nullptr, filterOptions);
        renderPath_ = SculptureRenderPath::NoRenderableSnapshot;
        return false;
    }

    instanceCount_ = 0;
    droppedSampleCount_ = 0;
    activeBatchCount_ = 0;
    generation_ = snapshot->frame.generation;

    // Delivery metadata makes ownership explicit: external output selects its
    // producer slot; staged output is copied into a bridge-owned ring slot.
    std::size_t ringIndex = 0U;
    // An external slot is usable only after the producer has attached its
    // OpenCL-release epoch. Metadata without that ownership proof follows the
    // ordinary staged path instead of exposing shared storage prematurely.
    if (externalDelivery) {
        ringIndex = delivery.slotIndex;
        // The queue wait makes the OpenCL release visible before any compute
        // reads this imported slot; the signal transfers it back after this
        // frame's graph and draw commands complete.
        enqueueAcquiredExternalDeliveryHandoff(draw, delivery);

        // Queue-commit registration makes the delivery durable until submit.
        const bool ownershipReleaseAccepted = true; 

        const SculptureDeliveryAction postSubmitAction =
            decidePostSubmitDelivery(ownershipReleaseAccepted);
        if (postSubmitAction
            == SculptureDeliveryAction::RetainCommittedExternal) {
            // The Vulkan commands already reference this imported buffer. Do
            // not fall through to a host copy of shared memory: that would
            // bypass the queue ownership contract. A failed release leaves
            // the slot unavailable rather than risking an unsynchronised read.
        }
    } else {
        ringIndex = hostIndex;
        // If an external field is not eligible, its ticket still needs the
        // ordinary handoff, even when a matching private host copy is usable.
        if (externalStorage && isVolumeMode && !delivery.hasExternalSpatial())
            static_cast<void>(retireExternalDelivery(draw, delivery));
    }
    VkBuffer inputVkBuffer = VK_NULL_HANDLE;
    if (externalDelivery && ringIndex < gpu->ingestSlots.size()) {
        inputVkBuffer = gpu->ingestBuffers[ringIndex]->handle();
    } else {
        gpu->ingestBuffers[ringIndex]->write(snapshot, sizeof(gpu_abi::PackedSculptureSnapshot));
        if (isVolumeMode) {
            const auto field = stagedDelivery.hostSpatial();
            gpu->ingestSpatialBuffers[ringIndex]->write(field.data(), field.size_bytes());
        }
        inputVkBuffer = gpu->ingestBuffers[ringIndex]->handle();
    }

    if (inputVkBuffer != VK_NULL_HANDLE && ringIndex < gpu->descriptorSets.size()) {
        
        // Push constants
        ProceduralPushConstants pc;
        pc.groupModel = rootTransform;
        pc.activeCount = std::min<std::uint32_t>(
            snapshot->activeInstanceCount,
            static_cast<std::uint32_t>(kDirectInstanceCapacity));
        pc.colorEffect = snapshot->reserved0 & 0xFF;
        pc.stageMask = (snapshot->reserved0 >> 8);
        pc.time = snapshot->frame.time;
        pc.chromaBreath = params.chromaBreath;
        pc.lumaDrift = params.lumaDrift;
        pc.paletteBlend = params.paletteBlend;
        pc.hueRotation = params.hueRotation;
        pc.audioSpectrum = mgfx::math::vec4(params.audioSpectrum[0], params.audioSpectrum[1], params.audioSpectrum[2], params.audioSpectrum[3]);
        pc.temporalFeedbackBlend = params.temporalFeedbackBlend;
        pc.temporalReset = params.temporalReset ? 1U : 0U;
        // -------------------------------------------------------------------------
        // Educational Annotation: GPU INSTANCE EXPANSION & CONDITIONAL VOLUME TRANSFER
        // OpenCL writes 80-byte SculptureSampleGpu records into ingestBuffers[ringIndex].
        // Both rendering modes require expand_instances.comp.spv (gpu->computePipeline)
        // to expand those 80-byte samples into 112-byte mgfx::vk::InstanceData records
        // in gpu->instanceBuffers[ringIndex] (applying rootTransform, audio-reactive
        // velocity stretching, and all 16 OKLCH palettes). The expensive 50x50x50 3D
        // volume transfer pass is conditionally queued only when outputMode == 1U.
        // -------------------------------------------------------------------------
        gpu->preparedIngest[ringIndex] = {pc, isVolumeMode, false, snapshot->frame.generation,
            isVolumeMode ? *newVolumeMapping : mgfx::ProceduralFieldMapping{},
            isVolumeMode ? fieldIdentity : mgfx::ProceduralFieldContentIdentity{}};
        if (!queueSettlement(ringIndex))
            throw std::logic_error("sculpture ingest settlement capacity changed after preflight");
        // Frame retention above owns the resources. Recording closures borrow
        // only that owner and its bounded slot, keeping graph callbacks small.
        draw.engine().deferGraphPass([gpu = gpu.get(), ringIndex](mgfx::vk::RenderGraph& graph) {
            const auto& pc = gpu->preparedIngest[ringIndex].push;
            const bool isVolumeMode = gpu->preparedIngest[ringIndex].volume;
            const VkBuffer inputVkBuffer = gpu->ingestBuffers[ringIndex]->handle();
            const auto ingestBufId = graph.registerBuffer(
                "SculptureIngest." + std::to_string(ringIndex),
                inputVkBuffer);
            const auto instBufId = graph.registerBuffer(
                gpu->instanceResourceNames[ringIndex],
                gpu->instanceBuffers[ringIndex]->handle(), kDerivedInstanceEntry);

            if (pc.activeCount > 0U && gpu->computePipeline) {
                graph.addPass(gpu->computePassNames[ringIndex])
                    .readBuffer(ingestBufId,
                                VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                                VK_ACCESS_2_SHADER_READ_BIT)
                    .writeBuffer(instBufId,
                                 VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                                 VK_ACCESS_2_SHADER_WRITE_BIT)
                    .execute([gpu, ringIndex](VkCommandBuffer cmd) {
                        const auto& pc = gpu->preparedIngest[ringIndex].push;
                        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
                                          gpu->computePipeline->handle());
                        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
                                                gpu->computePipeline->layout(),
                                                0, 1, &gpu->descriptorSets[ringIndex],
                                                0, nullptr);
                        vkCmdPushConstants(cmd, gpu->computePipeline->layout(),
                                           VK_SHADER_STAGE_COMPUTE_BIT,
                                           0, sizeof(ProceduralPushConstants), &pc);
                        const std::uint32_t groupCount = (pc.activeCount + 255U) / 256U;
                        vkCmdDispatch(cmd, groupCount, 1U, 1U);
                    });
            }

            if (isVolumeMode) {
                const auto volBufId = graph.registerBuffer(
                    gpu->volumeBufferResourceNames[ringIndex],
                    gpu->ingestSpatialBuffers[ringIndex]->handle());
                const auto volTexId = graph.registerImage(
                    gpu->volumeTextureResourceNames[ringIndex],
                    gpu->ingestVolumeImages[ringIndex]->handle(),
                    VK_IMAGE_ASPECT_COLOR_BIT, kDerivedVolumeEntry);

                graph.addPass("SculptureVolumeTransfer." + std::to_string(ringIndex))
                    .readBuffer(volBufId,
                                VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                                VK_ACCESS_2_SHADER_READ_BIT)
                    .write(volTexId,
                           VK_IMAGE_LAYOUT_GENERAL,
                           VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                           VK_ACCESS_2_SHADER_WRITE_BIT)
                    .execute([gpu, ringIndex](VkCommandBuffer cmd) {
                        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, gpu->volumeTransferPipeline->handle());
                        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, gpu->volumeTransferPipeline->layout(), 0, 1, &gpu->volumeDescriptorSets[ringIndex], 0, nullptr);
                        
                        // -------------------------------------------------------------------------
                        // Educational Annotation: DYNAMIC COMPUTE DISPATCH
                        // The Vulkan local workgroup size (defined in the shader as 8x8x8) divides
                        // the total required domain. By computing (kVolumeGridSize + 7) / 8 dynamically,
                        // we guarantee fault-tolerant boundary coverage even if the underlying
                        // OpenCL grid dimensions scale up in future hardware configurations.
                        // -------------------------------------------------------------------------
                        const std::uint32_t groupCount = (gpu_abi::kVolumeGridSize + 7U) / 8U;
                        vkCmdDispatch(cmd, groupCount, groupCount, groupCount);
                    });

                // Transition the 3D volume texture and expanded instance buffer for the graphics passes
                graph.addPass(gpu->vertexReadyPassNames[ringIndex])
                    .read(volTexId,
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                          VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                          VK_ACCESS_2_SHADER_SAMPLED_READ_BIT)
                    .readBuffer(instBufId,
                                VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                                VK_ACCESS_2_SHADER_READ_BIT)
                    .execute([](VkCommandBuffer) {});
            } else if (pc.activeCount > 0U) {
                graph.addPass(gpu->vertexReadyPassNames[ringIndex])
                    .readBuffer(instBufId,
                                VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT,
                                VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT)
                    .execute([](VkCommandBuffer) {});
            }
        });
            
        // -------------------------------------------------------------------------
        // Educational Annotation: DECOUPLED PROCEDURAL INJECTION
        // We package the fractal's dynamic state (active count, time) into a 
        // generic payload. The renderer doesn't need to know what these bytes 
        // represent; it simply routes them to the procedural world shader.
        // -------------------------------------------------------------------------
        if (isVolumeMode) {
            // Volume Mode: Present the implicit field / SDF volume, skipping diagnostic instances.
            draw.engine().setProceduralPayload(makeVolumePayload(gpu, ringIndex, pc, *newVolumeMapping, fieldIdentity));
            fieldCandidate_ = SculptureFieldCaptureCandidate{attempt, gpu->preparedIngest[ringIndex].identity,
                gpu->preparedIngest[ringIndex].mapping, fieldAppearance_, volumeRecipe_};
        } else {
            // Explicit instanced mode: Render GPU-expanded 112-byte InstanceData records
            // (with rootTransform, velocity stretching, and OKLCH palette evaluation).
            draw.engine().setProceduralPayload({});
            draw.drawExternalInstanced(mesh, materials[0],
                gpu->instanceBuffers[ringIndex]->handle(),
                0U, pc.activeCount,
                mgfx::math::mat4(1.0f), mgfx::math::vec4(1.0f),
                mgfx::pbr::RenderMode::Full, 0U,
                false);
        }
        
        // The sidecar contains center samples and speed sums, not conservative
        // radius-expanded bounds, maximum speed or kinetic energy. Only the
        // capacity-admitted draw count is known on this path.
        instanceCount_ = pc.activeCount;
        lodStats_.lod0Count = instanceCount_;
        publishMappedCompactObservation(instanceCount_, boundingVolume_, motionKinematics_);

        renderPath_ = SculptureRenderPath::MappedCompute;
        collectedAttempt_ = draw.engine().activeFrameAttempt();
        collectedGeneration_ = snapshot->frame.generation;
        return true;
    }

    // This is an internal candidate invariant, not permission to reinterpret
    // an already acquired external buffer as a CPU fallback.
    throw std::logic_error("sculpture ingest candidate lacks its descriptor or input buffer");
}

std::span<const mgfx::vk::CompactInstanceData> SculptureRenderBridge::band(
    std::size_t materialBand) const noexcept {
    if (materialBand >= kMaterialBandCount) return {};
    return cpuBatches_[materialBand];
}

std::size_t SculptureRenderBridge::instanceCount() const noexcept {
    return instanceCount_;
}

std::size_t SculptureRenderBridge::batchCount() const noexcept {
    return activeBatchCount_;
}

std::size_t SculptureRenderBridge::droppedSampleCount() const noexcept {
    return droppedSampleCount_;
}

std::size_t SculptureRenderBridge::prunedSampleCount() const noexcept {
    return prunedSampleCount_;
}

std::size_t SculptureRenderBridge::culledSampleCount() const noexcept {
    return culledSampleCount_;
}

const SculptureLodStats& SculptureRenderBridge::lodStats() const noexcept {
    return lodStats_;
}

std::uint32_t SculptureRenderBridge::generation() const noexcept {
    return generation_;
}

SculptureRenderPath SculptureRenderBridge::renderPath() const noexcept {
    return renderPath_;
}

void SculptureRenderBridge::prepareReliefDrawResources(
    mgfx::Engine& engine, ReliefDrawResources& resources) {
    if (!resources.mesh) {
        const auto resolution = sculptureReliefGridResolution(reliefQuality_);
        resources.mesh = std::make_unique<mgfx::vk::Mesh>(
            mgfx::vk::Mesh::makeDenseGrid(engine.context(), resolution, resolution));
    }
    if (resources.material == mgfx::kInvalidMaterial) {
        mgfx::pbr::Material material{};
        material.baseColorFactor = mgfx::math::vec4(0.8f, 0.8f, 0.8f, 1.0f);
        material.metallic = 0.5f;
        material.roughness = 0.2f;
        material.emissiveFactor = mgfx::math::vec3(1.0f);
        resources.material = engine.materials()->createMaterial(material);
    }
}

ReliefDrawOutcome SculptureRenderBridge::drawRelief(
    mgfx::Drawable& draw,
    mgfx::MaterialHandle material,
    const mgfx::math::mat4& rootTransform,
    const SculptureProgramParams& params,
    const EpiFeedbackFieldDelivery& field,
    bool applyPoincareDisk
) {
    if (!field.isConsumable()) return ReliefDrawOutcome::Rejected;

    const float heightScale = params.radiusScale * 0.35f;
    const auto contours = resolveReliefContourSettings(
        {params.reliefContourDensity, params.reliefContourStyle});
    const float contourDensity = contours.density;
    const std::uint32_t contourStyle = contours.style;
    const std::uint32_t reliefFlags = 1024u | (contourDensity > 0.0f
        ? mgfx::pbr::kFeatureFlagReliefContours
            | (contourStyle == 1U
                ? mgfx::pbr::kFeatureFlagReliefSignedLuminance : 0u)
            | (contourStyle == 2U ? mgfx::pbr::kFeatureFlagReliefMajorContours : 0u)
        : 0u);
    const auto& descriptor = field.descriptor;
    const mgfx::math::vec4 extraData(
        heightScale, 1.2f, applyPoincareDisk ? 1.0f : 0.0f, contourDensity);
    // Acquisition remains transport-specific; the selected slot reaches one
    // presentation command with identical observation intent and push layout.
    const auto drawSelectedRelief = [&](const ReliefDrawResources& resources) {
        draw.draw(*resources.mesh, resources.material, rootTransform,
                  mgfx::math::vec4(1.0f), mgfx::pbr::RenderMode::Full,
                  reliefFlags, false, false, extraData);
        return ReliefDrawOutcome::Collected;
    };

    if (field.transport == EpiFeedbackFieldTransport::ExternalMemory) {
        auto* transport = canonicalFeedbackTransport_;
        if (!transport) return ReliefDrawOutcome::Deferred;

        auto& frameLoop = draw.engine().frameLoop();
        const auto readState = transport->probeVulkanRead(field);
        if (readState == EpiFeedbackFieldExternalTransport::VulkanReadState::Rejected)
            return ReliefDrawOutcome::Rejected;
        const bool needsReadyWait =
            readState == EpiFeedbackFieldExternalTransport::VulkanReadState::Ready;
        if (needsReadyWait
            && !frameLoop.canEnqueueSubmission({.externalWaits = 1U, .commitCallbacks = 1U})) {
            // The delivery remains OpenClReleased and can be acquired later.
            return ReliefDrawOutcome::Deferred;
        }

        // Prepare fallible native resources before taking ownership. Allocation
        // failure leaves a ready delivery OpenClReleased, with its wait unconsumed.
        const std::size_t ringIndex = field.externalLease->slotIndex;
        auto& resources = externalReliefSlots_[ringIndex];

        prepareReliefDrawResources(draw.engine(), resources);

        if (transport->beginVulkanRead(field) != readState)
            return ReliefDrawOutcome::Rejected;

        if (needsReadyWait) {
            // Install settlement before binding or collecting draws can throw.
            // Queue acceptance leaves the image resident; failure quarantines
            // the exact ticket instead of replaying an uncertain binary wait.
            bool commitQueued = false;
            try {
                commitQueued = frameLoop.enqueueSubmitCommit(
                    [transport, field](bool success) noexcept {
                        if (!success) static_cast<void>(transport->quarantine(field));
                    });
            } catch (...) {
                // Only registration exceptions retain local cleanup. Normal
                // refusal is already quarantined by the closure adapter.
                static_cast<void>(transport->quarantine(field));
                throw;
            }
            if (!commitQueued) {
                throw std::logic_error(
                    "canonical feedback commit capacity changed after preflight");
            }
            if (!frameLoop.enqueueExternalWait(
                    transport->openClReadySemaphore(ringIndex),
                    VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT)) {
                throw std::logic_error(
                    "canonical feedback wait capacity changed after preflight");
            }
        }

        const auto imageInfo = transport->descriptorFor(field);
        if (!imageInfo) {
            static_cast<void>(transport->quarantine(field));
            return ReliefDrawOutcome::Rejected;
        }
        bindCanonicalReliefField(draw.engine(), resources.material, imageInfo.value());

        // The first ready wait is now represented by this submission. Resident
        // frames reuse the image; only a later production submission releases it.
        return drawSelectedRelief(resources);

    } else if (field.transport == EpiFeedbackFieldTransport::HostStaged) {
        // Staged transport of OpenCL output; the CPU does no field evaluation.
        const auto& selected = stagedRelief_.slots[stagedRelief_.selectedSlot];
        const bool cacheMatches = selected.image && selected.draw.mesh
            && selected.draw.material != mgfx::kInvalidMaterial
            && epi::sameFeedbackFieldIdentity(stagedRelief_.key, descriptor);

        if (!cacheMatches) {
            // Source identity normally survives many uploaded generations.
            // Prepare text only on a source change, before native side effects.
            const bool sourceChanged = stagedRelief_.key.sourceStateId != descriptor.sourceStateId;
            std::string sourceStateId;
            if (sourceChanged) sourceStateId = descriptor.sourceStateId;
            const std::size_t next =
                (stagedRelief_.selectedSlot + 1U) % stagedRelief_.slots.size();
            auto& candidate = stagedRelief_.slots[next];

            if (!candidate.image
                || candidate.image->width() != descriptor.width
                || candidate.image->height() != descriptor.height) {
                mgfx::vk::Image::CreateInfo imageInfo{};
                imageInfo.width = descriptor.width;
                imageInfo.height = descriptor.height;
                imageInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
                imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
                imageInfo.addressU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                imageInfo.addressV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                candidate.image = std::make_unique<mgfx::vk::Image>(
                    draw.engine().context(), imageInfo);
            }
            
            candidate.image->uploadStaged(
                field.hostLinearRgba.data(),
                field.hostLinearRgba.size() * sizeof(float));

            prepareReliefDrawResources(draw.engine(), candidate.draw);

            bindCanonicalReliefField(
                draw.engine(), candidate.draw.material, candidate.image->descriptorInfo());

            // Publish only a complete image/material binding. If preparation or
            // binding throws, selection and identity still describe the previous
            // ready slot; retry cannot mistake the partial candidate for a hit.
            if (sourceChanged) stagedRelief_.key.sourceStateId.swap(sourceStateId);
            stagedRelief_.selectedSlot = next;
            auto& key = stagedRelief_.key;
            key.generation = descriptor.generation;
            key.resetEpoch = descriptor.resetEpoch;
            key.width = descriptor.width;
            key.height = descriptor.height;
            key.format = descriptor.format;
            key.simulationTime = descriptor.simulationTime;
            key.producerSession = descriptor.producerSession;
        }

        return drawSelectedRelief(stagedRelief_.slots[stagedRelief_.selectedSlot].draw);
    }
    return ReliefDrawOutcome::Rejected;
}


namespace {

void populateManifoldVertices(const IsosurfaceMesh& isoMesh,
                              std::vector<mgfx::vk::Vertex>& outVertices) {
    outVertices.resize(isoMesh.vertices.size());
    for (std::size_t i = 0; i < isoMesh.vertices.size(); ++i) {
        const auto& v = isoMesh.vertices[i];
        mgfx::vk::Vertex vert{};
        vert.position = mgfx::math::vec3(v.position.x, v.position.y, v.position.z);
        vert.normal = mgfx::math::vec3(v.normal.x, v.normal.y, v.normal.z);
        vert.uv = mgfx::math::vec2(v.materialCoord, v.density);
        vert.color = mgfx::math::vec4(
            std::clamp(v.normal.x * 0.5f + 0.5f, 0.0f, 1.0f),
            std::clamp(v.normal.y * 0.5f + 0.5f, 0.0f, 1.0f),
            std::clamp(v.normal.z * 0.5f + 0.5f, 0.0f, 1.0f),
            1.0f
        );
        vert.tangent = mgfx::math::vec4(1.0f, 0.0f, 0.0f, 1.0f);
        outVertices[i] = vert;
    }
}

} // namespace

std::unique_ptr<mgfx::vk::Mesh> SculptureRenderBridge::createManifoldVkMesh(
    const mgfx::vk::Context& context,
    const IsosurfaceMesh& isoMesh
) {
    if (isoMesh.vertices.empty() || isoMesh.indices.empty()) {
        return nullptr;
    }

    std::vector<mgfx::vk::Vertex> vertices;
    populateManifoldVertices(isoMesh, vertices);

    auto& mutableCtx = const_cast<mgfx::vk::Context&>(context);
    return mgfx::vk::Mesh::makeDynamic(mutableCtx, vertices, isoMesh.indices);
}

std::optional<mgfx::scene::Scene> SculptureRenderBridge::makeManifoldScene(
    const IsosurfaceMesh& isoMesh, mgfx::scene::MaterialDef material) {
    if (isoMesh.vertices.empty() || isoMesh.indices.empty() || isoMesh.indices.size() % 3u != 0u) {
        return std::nullopt;
    }

    mgfx::scene::Scene scene;
    scene.sourceFormat = "PROCEDURAL";
    auto& mesh = scene.meshes.emplace_back();
    mesh.name = "Sculpture isosurface";
    populateManifoldVertices(isoMesh, mesh.vertices);
    mesh.indices = isoMesh.indices;
    mesh.sourceHasNormals = true;
    mesh.sourceHasUv0 = true;
    mesh.sourceHasColors = true;
    scene.materials.push_back(std::move(material));
    auto& object = scene.objects.emplace_back();
    object.name = mesh.name;
    object.mesh = 0u;
    object.material = 0u;
    if (!mgfx::scene::validateProceduralSceneBounds(scene) ||
        mgfx::scene::validateScene(scene).hasErrors()) {
        return std::nullopt;
    }
    return scene;
}

void SculptureRenderBridge::drawManifoldMesh(
    mgfx::Drawable& draw,
    mgfx::MaterialHandle material,
    const mgfx::math::mat4& rootTransform,
    const IsosurfaceMesh& isoMesh
) {
    if (isoMesh.empty()) return;
    auto& ctx = draw.engine().context();
    const std::uint64_t sig = isoMesh.contentSignature(static_cast<std::uint64_t>(generation_));
    if (sig != 0U &&
        dynamicManifoldMeshRing_[manifoldFrameIndex_] &&
        manifoldMeshSignatures_[manifoldFrameIndex_] == sig) {
        draw.draw(*dynamicManifoldMeshRing_[manifoldFrameIndex_], material, rootTransform);
        return;
    }

    manifoldFrameIndex_ = (manifoldFrameIndex_ + 1U) % dynamicManifoldMeshRing_.size();
    populateManifoldVertices(isoMesh, manifoldVertexScratch_);
    auto& slotMesh = dynamicManifoldMeshRing_[manifoldFrameIndex_];
    if (slotMesh) {
        slotMesh->updateDynamic(ctx, manifoldVertexScratch_, isoMesh.indices);
    } else {
        slotMesh = mgfx::vk::Mesh::makeDynamic(ctx, manifoldVertexScratch_, isoMesh.indices);
    }
    manifoldMeshSignatures_[manifoldFrameIndex_] = sig;
    if (slotMesh) {
        draw.draw(*slotMesh, material, rootTransform);
    }
}

} // namespace epi_sculpt

// -----------------------------------------------------------------------------
