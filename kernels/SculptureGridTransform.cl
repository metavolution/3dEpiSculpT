#include "SculptureGpuAbi.clh"
#include "MgfxVertex.clh"
#include "EpiLearnMode.clh"
#include "EpiMath.clh"
#include "EpiColor.clh"
#include "EpiTopology.clh"
#include "EpiFractal.clh"
#include "EpiVectorFields.clh"
#include "EpiAtomic.clh"
#include "EpiMorphogen.clh"
#include "EpiFluid.clh"
#include "SculptureGridTransform.clh"

__kernel void compute_grid_displacement(
    __constant const SculptureProgramParamsGpu* params,
    __constant const SculptureFrameGpu* frame,
    const uint gridResolution,
    __global MgfxVertex* outVertexBuffer)
{
    const uint index = get_global_id(0);
    if (index >= gridResolution * gridResolution) return;
    
    epi_sculpt_evaluate_grid_transform(index, gridResolution, params, frame, outVertexBuffer);
}
