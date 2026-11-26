#pragma once

#include "Sculpture4DFoliation.h"
#include "SculptureIsosurface.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace epi_sculpt {

struct SculptureSnapshot;

namespace field_geometry {

/**
 * CPU-side observational geometry derived from already-produced sculpture or
 * field state.
 *
 * These helpers do not own simulation cadence, canonical VisualMode state, or
 * GPU publication. They are pure(ish) data transformations used for educational
 * inspection, auxiliary coupling and derived presentation geometry.
 */

void rasterizeSampleDensityTo2DSeed(
    const SculptureSnapshot& snapshot,
    std::vector<float>& outSeedGrid,
    std::uint32_t width,
    std::uint32_t height);

void sample2DTextureTo3DVoxelGrid(
    const std::vector<float>& inTextureRgba,
    std::uint32_t width,
    std::uint32_t height,
    std::vector<float>& outVoxelGrid,
    std::uint32_t voxelDim);

void displaceSamplesWith2DTexture(
    SculptureSnapshot& snapshot,
    const std::vector<float>& inTextureRgba,
    std::uint32_t width,
    std::uint32_t height,
    float displacementScale = 0.25f);

struct ReliefVertex {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float nx = 0.0f;
    float ny = 0.0f;
    float nz = 1.0f;
    float u = 0.0f;
    float v = 0.0f;
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    float a = 1.0f;
};

struct ReliefMesh {
    std::vector<ReliefVertex> vertices;
    std::vector<std::uint32_t> indices;
    float minHeight = 0.0f;
    float maxHeight = 0.0f;
    float meanCurvature = 0.0f;
};

[[nodiscard]] ReliefMesh generateEpimorphicReliefMesh(
    const std::vector<float>& inFeedbackField,
    std::uint32_t fieldWidth,
    std::uint32_t fieldHeight,
    std::uint32_t gridResX,
    std::uint32_t gridResY,
    float heightScale = 0.35f,
    float curvaturePower = 1.2f,
    bool applyPoincareDisk = false);

[[nodiscard]] ReliefMesh convertIsosurfaceToReliefMesh(
    const IsosurfaceMesh& isoMesh);

[[nodiscard]] IsosurfaceMesh generateFoliationSurfaceMesh(
    Manifold4DType type,
    float thetaL,
    float thetaR,
    float scale = 1.0f,
    std::size_t uDivisions = 32,
    std::size_t vDivisions = 32);

} // namespace field_geometry
} // namespace epi_sculpt
