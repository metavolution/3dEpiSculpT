#pragma once

#include "SculptureProgram.h"
#include <bit>
#include <vector>
#include <array>
#include <cstdint>
#include <cstddef>
#include <string_view>

namespace epi_sculpt {

// -----------------------------------------------------------------------------
// Educational Annotation: Implicit Surface Marching Cubes & Polygonization
// -----------------------------------------------------------------------------
// Translates continuous 3D mathematical density fields and 3D fluid memory
// grids into explicit indexed triangle meshes (Vulkan-ready vertex/index arrays).
// Evaluates 8 corner density values per voxel cube, uses standard lookup tables
// to determine topology, linearly interpolates vertex edge intersections, and
// calculates analytical surface normal gradients:
//   n_hat = normalize(grad(rho(x, y, z)))
// -----------------------------------------------------------------------------

struct IsosurfaceVertex {
    SculptureVec3 position{};
    SculptureVec3 normal{};
    float materialCoord = 0.0f;
    float density = 0.0f;
};

struct IsosurfaceMesh {
    std::vector<IsosurfaceVertex> vertices;
    std::vector<std::uint32_t> indices;
    SculptureVec3 aabbMin{-1.0f, -1.0f, -1.0f};
    SculptureVec3 aabbMax{ 1.0f,  1.0f,  1.0f};
    float isoLevel = 0.25f;
    std::uint64_t generation = 0U;

    [[nodiscard]] std::size_t triangleCount() const noexcept {
        return indices.size() / 3;
    }

    [[nodiscard]] bool empty() const noexcept {
        return indices.empty();
    }

    // Fast 64-bit FNV-1a content & generation signature used by the presentation
    // ring cache to skip redundant CPU vertex conversion and GPU uploads when
    // consecutive frames observe the same polygonized isosurface snapshot.
    [[nodiscard]] std::uint64_t contentSignature(std::uint64_t externalGeneration = 0U) const noexcept {
        if (vertices.empty() || indices.empty()) {
            return 0U;
        }
        std::uint64_t h = 14695981039346656037ULL;
        const auto mixU64 = [&h](std::uint64_t v) noexcept {
            h ^= v;
            h *= 1099511628211ULL;
        };
        const auto mixF32 = [&mixU64](float f) noexcept {
            mixU64(static_cast<std::uint64_t>(std::bit_cast<std::uint32_t>(f)));
        };
        mixU64(generation);
        mixU64(externalGeneration);
        mixU64(static_cast<std::uint64_t>(vertices.size()));
        mixU64(static_cast<std::uint64_t>(indices.size()));
        mixF32(isoLevel);

        const std::size_t vCount = vertices.size();
        const std::size_t vStride = std::max<std::size_t>(1U, vCount / 8U);
        for (std::size_t i = 0U; i < vCount; i += vStride) {
            const auto& v = vertices[i];
            mixF32(v.position.x);
            mixF32(v.position.y);
            mixF32(v.position.z);
            mixF32(v.normal.x);
            mixF32(v.density);
        }
        const auto& vBack = vertices.back();
        mixF32(vBack.position.x);
        mixF32(vBack.position.y);
        mixF32(vBack.position.z);

        const std::size_t iCount = indices.size();
        const std::size_t iStride = std::max<std::size_t>(1U, iCount / 8U);
        for (std::size_t i = 0U; i < iCount; i += iStride) {
            mixU64(static_cast<std::uint64_t>(indices[i]));
        }
        mixU64(static_cast<std::uint64_t>(indices.back()));
        return h == 0U ? 1U : h;
    }
};

struct IsosurfaceConfig {
    float isoLevel = 0.25f;
    SculptureVec3 boundsMin{-2.0f, -2.0f, -2.0f};
    SculptureVec3 boundsMax{ 2.0f,  2.0f,  2.0f};
    std::uint32_t resolution = 32; // Grid divisions per axis (e.g. 16, 32, 50)
    bool calculateNormals = true;
    bool weldVertices = true;
    float normalEpsilon = 0.015f;
};

class SculptureIsosurface {
public:
    // Extraction is synchronous: all submitted tasks settle before return or
    // unwind, including enqueue/result-assembly failures. A failed extraction
    // throws instead of returning a partially assembled mesh. Keep input owners
    // alive and unchanged until completion; no retained snapshot/lock is taken.
    // Extract polygon surface mesh from continuous SculptureProgram scalar field
    [[nodiscard]] static IsosurfaceMesh extractMesh(
        const SculptureProgram& program,
        float time,
        float morph,
        const std::vector<float>& spatialBuffer = {},
        const IsosurfaceConfig& config = {}
    );

    // Extract polygon surface mesh directly from an x-fastest cubic voxel grid.
    // Requires gridDim >= 2 and at least gridDim^3 samples. Unrepresentable or
    // truncated extents return an empty mesh before worker startup; trailing
    // samples are ignored. Normal sampling uses centered interior differences
    // for gridDim >= 3 and forward differences for the single-cell gridDim == 2.
    [[nodiscard]] static IsosurfaceMesh extractFromVoxelBuffer(
        const std::vector<float>& voxelBuffer,
        std::uint32_t gridDim,
        const IsosurfaceConfig& config = {}
    );

    // Compute bounding box for extracted mesh
    static void computeMeshBounds(IsosurfaceMesh& mesh) noexcept;
};

} // namespace epi_sculpt
