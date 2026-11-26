#include "SculptureAcousticMistBridge.h"
#include <cmath>
#include <algorithm>

namespace epi_sculpt {

void SculptureAcousticMistBridge::apply(
    mgfx::Engine& engine,
    const std::optional<SculptureAcousticMistObservation>& observation) {
    if (!observation) {
        clear(engine);
        return;
    }
    const auto& resonance = observation->resonance;
    const auto& bounds = observation->bounds;
    const float timeSeconds = observation->timeSeconds;
    
    // We choose a relatively dense 32x32x32 voxel grid for the acoustic field
    constexpr std::uint32_t kVoxelDim = 32U;
    
    // 1. Evaluate the mathematical cavity pressure wave into a 1D scalar vector
    resonance.rasterizePressureField3D(pressureGrid_, kVoxelDim, bounds, timeSeconds);
    
    // 2. Map the scalar pressure into the Vulkan-aligned MistVoxelGpu ABI format
    if (mistVoxels_.size() != pressureGrid_.size()) {
        mistVoxels_.resize(pressureGrid_.size());
    }

    // A standing wave pressure p oscillates in [-1, 1].
    // We rectify it into absolute density (LWC) and inject Kuramoto phase data.
    const float qFactor = std::max(resonance.metrics().resonanceQFactor, 1.0f);
    const float globalEnergy = resonance.metrics().acousticEnergy * 0.1f;

    for (std::size_t i = 0; i < pressureGrid_.size(); ++i) {
        const float rawPressure = pressureGrid_[i];
        
        // Map absolute pressure to Liquid Water Content [kg/m^3]
        const float lwc = std::abs(rawPressure) * globalEnergy * 100.0f;
        
        // Enhance number density where pressure gradients are steepest (edges of standing waves)
        const float edgeDensity = (1.0f - std::abs(rawPressure)) * 10.0f;
        
        mistVoxels_[i].waterMoments[0] = edgeDensity;                 // Number density
        mistVoxels_[i].waterMoments[1] = lwc;                         // LWC (Primary visual density)
        mistVoxels_[i].waterMoments[2] = 0.05f;                       // Mean droplet radius (fixed)
        mistVoxels_[i].waterMoments[3] = 0.01f;                       // Variance
        
        // Encode acoustic phase velocity mapping
        mistVoxels_[i].velocity[0] = 0.0f;
        mistVoxels_[i].velocity[1] = rawPressure * 5.0f; // Y-oscillation
        mistVoxels_[i].velocity[2] = 0.0f;
        mistVoxels_[i].velocity[3] = 0.0f;
    }

    // 3. Construct the fake Experiment Manifest so the Vulkan renderer accepts it
    ExperimentResourceManifest manifest{};
    manifest.version = ExperimentResourceManifest::kVersion;
    manifest.generation = ++simulatedGeneration_;
    manifest.simulationTime = timeSeconds;
    manifest.resourceCount = 1U;
    
    // The generic mist bridge consumes the DenseMedium role; this acoustic
    // producer keeps a local numeric id without coupling that renderer to it.
    manifest.resources[0].descriptor.id = ExperimentResourceId{2U};
    manifest.resources[0].descriptor.role = ExperimentResourceRole::DenseMedium;
    manifest.resources[0].descriptor.kind = ExperimentResourceKind::ScalarField3D;
    manifest.resources[0].descriptor.shape = ExperimentResourceShape::Volume3D;
    manifest.resources[0].descriptor.transport = ExperimentResourceTransport::HostStaged;
    manifest.resources[0].descriptor.schemaVersion = experiment_mist_field_gpu_abi::kVersion;
    manifest.resources[0].descriptor.elementStrideBytes = experiment_mist_field_gpu_abi::kVoxelBytes;
    manifest.resources[0].descriptor.elementCount = mistVoxels_.size();
    manifest.resources[0].descriptor.elementCapacity = mistVoxels_.size();
    manifest.resources[0].descriptor.extent = {kVoxelDim, kVoxelDim, kVoxelDim};
    manifest.resources[0].descriptor.generation = manifest.generation;
    // Host-staged resources carry their immutable payload identity in the
    // manifest.  The renderer may still receive the typed span directly, but
    // there is no second out-of-band ownership convention for this adapter.
    manifest.resources[0].hostStagedPayload = mistVoxels_.data();

    experiment_mist_field_gpu_abi::MistFieldDescriptorGpu fieldDesc{};
    fieldDesc.identity[0] = experiment_mist_field_gpu_abi::kVersion;
    fieldDesc.identity[1] = experiment_mist_field_gpu_abi::kDescriptorBytes;
    fieldDesc.identity[2] = kVoxelDim; // Width
    fieldDesc.identity[3] = kVoxelDim; // Height
    fieldDesc.extentCadence[0] = kVoxelDim; // Depth
    
    ExperimentMistOpticsConfig optics{};
    float ext = 2.5f * (1.0f + resonance.metrics().spectralEntrainment);
    SculptureVec3 albedo{0.1f, 0.4f, 0.9f};
    optics.scatteringPerLiquidWater = {albedo.x * ext, albedo.y * ext, albedo.z * ext};
    optics.absorptionPerLiquidWater = {(1.0f - albedo.x) * ext, (1.0f - albedo.y) * ext, (1.0f - albedo.z) * ext};
    
    // 4. Submit to the Vulkan Volume Texture Bridge. Candidate construction and
    // host publication are transactional, so a failed replacement leaves the last
    // compatible descriptor bundle available. Explicit clear() remains the route/
    // feature retirement boundary.
    if (mistRenderer_.rebuild(engine, manifest, fieldDesc, mistVoxels_, optics)) {
        static_cast<void>(mistRenderer_.bind(engine));
    }
}

void SculptureAcousticMistBridge::clear(mgfx::Engine& engine) const {
    mistRenderer_.clear(engine);
}

} // namespace epi_sculpt
