#pragma once

#include "ExperimentMistVolumeRenderBridge.h"
#include "SculptureAudioModulation.h"
#include "ExperimentMistFieldAbi.h"

#include <mgfx/Engine.h>
#include <vector>

namespace epi_sculpt {

// -----------------------------------------------------------------------------
// Educational Annotation: ACOUSTIC MIST INTEGRATION BRIDGE
// -----------------------------------------------------------------------------
// This class acts as the mediator between the mathematical acoustic standing
// wave cavity (SculptureAudioResonance) and the Vulkan volumetric rendering 
// engine (ExperimentMistVolumeRenderBridge).
// It samples the pure mathematical acoustic grid into the memory-aligned 
// ExperimentMistVoxelGpu format so the existing Mist shaders can raymarch it 
// as glowing physical density without requiring any modifications to Vulkan.
// -----------------------------------------------------------------------------

class SculptureAcousticMistBridge {
public:
    SculptureAcousticMistBridge() = default;

    // Bakes the current 3D acoustic resonance field into a volumetric mist texture
    // and binds it to the Vulkan render graph for procedural rendering.
    // An empty observation explicitly retires the optional effect.
    void apply(mgfx::Engine& engine,
               const std::optional<SculptureAcousticMistObservation>& observation);

    // Clears the bound procedural volume pass
    void clear(mgfx::Engine& engine) const;

private:
    ExperimentMistVolumeRenderBridge mistRenderer_;
    std::vector<float> pressureGrid_;
    std::vector<experiment_mist_field_gpu_abi::MistVoxelGpu> mistVoxels_;
    std::uint64_t simulatedGeneration_ = 0U;
};

} // namespace epi_sculpt
