#include "SculptureGpuAbi.clh"
#include "SculptureWorldGpuAbi.clh"
#include "EpiMath.clh"
#include "SculptureWorldCommon.clh"

// [EDUCATIONAL ANNOTATION]
// This file serves as the minimal AOT fallback kernel.
// The execution program flow has been aggressively optimized and refactored.
// All 31 distinct generative worlds have been fully decoupled into modular
// components located in `worlds/X.clh`.
// The host C++ dispatcher (SculptureWorldOpenClDispatcher) now completely
// bypasses this monolith and utilizes Hybrid Component JIT Assembly to 
// compile only the active world formula in complete isolation.

__kernel void generate_world_instances(
    __constant const SculptureProgramParamsGpu* params,
    __constant const SculptureFrameGpu* sculptureFrame,
    __constant const WorldRecipeGpu* recipe,
    __global WorldSnapshotGpu* output) {
    
    const uint index = get_global_id(0);
    if (!epi_sculpt_program_abi_valid(params) || 
        !epi_sculpt_frame_abi_valid(sculptureFrame) || 
        !epi_sculpt_world_recipe_abi_valid(recipe)) {
        return;
    }

    // A fully modularized architecture leaves this fallback empty.
    if (index == 0u) {
        output->frame.abiVersion = EPI_SCULPT_WORLD_ABI_VERSION;
        output->frame.byteSize = EPI_SCULPT_WORLD_FRAME_BYTES;
        output->frame.generation = sculptureFrame->generation;
        output->frame.activeInstanceCount = 0u; // Fallback generates nothing.
        output->frame.time = sculptureFrame->time;
        output->frame.deltaSeconds = sculptureFrame->delta_seconds;
        output->frame.morph = sculptureFrame->morph;
        output->frame.reserved = 0.0f;
        output->recipe = *recipe;
    }
}
