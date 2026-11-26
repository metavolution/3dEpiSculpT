#include "SculptureFieldGeometry.h"

#include "Sculpture4DFoliation.h"
#include "SculptureIsosurface.h"
#include "SculptureProgram.h"

#include <algorithm>
#include <cmath>

namespace epi_sculpt::field_geometry {

void rasterizeSampleDensityTo2DSeed(
    const SculptureSnapshot& snapshot,
    std::vector<float>& outSeedGrid,
    std::uint32_t width,
    std::uint32_t height
) {
    if (width == 0 || height == 0) return;
    const std::size_t totalPixels = static_cast<std::size_t>(width) * height;
    outSeedGrid.assign(totalPixels, 0.0f);

    for (const auto& sample : snapshot.samples) {
        if (sample.radius <= 0.0f) continue;

        // Orthographic/stereographic projection from 3D space [-2.5, 2.5]^3 to 2D UV [0, 1]^2
        const float u = (sample.x + 2.5f) / 5.0f;
        const float v = (sample.y + 2.5f) / 5.0f;

        if (u < 0.0f || u >= 1.0f || v < 0.0f || v >= 1.0f) continue;

        const float px = u * static_cast<float>(width);
        const float py = v * static_cast<float>(height);
        const int ix = static_cast<int>(px);
        const int iy = static_cast<int>(py);

        // Splat radius in pixel domain (bounded to [1, 8] for instant rasterization)
        const int splatRadius = std::clamp(static_cast<int>(sample.radius * static_cast<float>(width) * 0.05f), 1, 8);
        const float densityVal = (sample.a > 0.0f) ? sample.a : std::max(0.5f, 0.299f * sample.r + 0.587f * sample.g + 0.114f * sample.b);

        for (int dy = -splatRadius; dy <= splatRadius; ++dy) {
            const int cy = iy + dy;
            if (cy < 0 || cy >= static_cast<int>(height)) continue;

            for (int dx = -splatRadius; dx <= splatRadius; ++dx) {
                const int cx = ix + dx;
                if (cx < 0 || cx >= static_cast<int>(width)) continue;

                const float distSq = static_cast<float>(dx * dx + dy * dy);
                const float maxDistSq = static_cast<float>(splatRadius * splatRadius);
                if (distSq > maxDistSq) continue;

                const float weight = std::exp(-distSq / std::max(1.0f, maxDistSq * 0.5f));
                const std::size_t pIdx = static_cast<std::size_t>(cx) + static_cast<std::size_t>(cy) * width;
                outSeedGrid[pIdx] = std::clamp(outSeedGrid[pIdx] + densityVal * weight * 0.35f, 0.0f, 1.0f);
            }
        }
    }
}

void sample2DTextureTo3DVoxelGrid(
    const std::vector<float>& inTextureRgba,
    std::uint32_t width,
    std::uint32_t height,
    std::vector<float>& outVoxelGrid,
    std::uint32_t voxelDim
) {
    // -------------------------------------------------------------------------
    // Educational Annotation: Fast 2D-to-3D Cylindrical-Toroidal Projection
    // -------------------------------------------------------------------------
    // Maps a 2D RGB/RGBA texture field into an N^3 voxel grid using precomputed
    // coordinate projections and radial Gaussian falloff:
    //   P(x,y,z) = Intensity(tx, ty) * max(0, 1 - (px^2 + py^2 + pz^2) / 3)
    // Precomputing 1D coordinate vectors eliminates redundant ALU ops inside
    // the O(N^3) innermost loop.
    // -------------------------------------------------------------------------
    if (width == 0 || height == 0 || voxelDim < 2 || inTextureRgba.empty()) return;
    const std::size_t totalVoxels = static_cast<std::size_t>(voxelDim) * voxelDim * voxelDim;
    outVoxelGrid.assign(totalVoxels, 0.0f);

    const float invDim = 1.0f / static_cast<float>(voxelDim - 1U);
    const bool isRgba = (inTextureRgba.size() >= static_cast<std::size_t>(width) * height * 4U);

    // Precompute 1D coordinate projections
    std::vector<float> pX2(voxelDim);
    std::vector<std::uint32_t> tX(voxelDim);
    for (std::uint32_t x = 0; x < voxelDim; ++x) {
        const float px = (static_cast<float>(x) * invDim) * 2.0f - 1.0f;
        pX2[x] = px * px;
        const float u = std::clamp(0.5f + 0.5f * px, 0.0f, 1.0f);
        tX[x] = std::min(static_cast<std::uint32_t>(u * (width - 1U)), width - 1U);
    }

    std::vector<float> pY2(voxelDim);
    std::vector<std::uint32_t> tY(voxelDim);
    for (std::uint32_t y = 0; y < voxelDim; ++y) {
        const float py = (static_cast<float>(y) * invDim) * 2.0f - 1.0f;
        pY2[y] = py * py;
        const float v = std::clamp(0.5f + 0.5f * py, 0.0f, 1.0f);
        tY[y] = std::min(static_cast<std::uint32_t>(v * (height - 1U)), height - 1U);
    }

    std::vector<float> pZ2(voxelDim);
    for (std::uint32_t z = 0; z < voxelDim; ++z) {
        const float pz = (static_cast<float>(z) * invDim) * 2.0f - 1.0f;
        pZ2[z] = pz * pz;
    }

    const std::size_t sliceSize = static_cast<std::size_t>(voxelDim) * voxelDim;

    for (std::uint32_t z = 0; z < voxelDim; ++z) {
        const float pz2 = pZ2[z];
        const std::size_t zOffset = static_cast<std::size_t>(z) * sliceSize;

        for (std::uint32_t y = 0; y < voxelDim; ++y) {
            const float py2pz2 = pY2[y] + pz2;
            const std::uint32_t tyRow = tY[y] * width;
            const std::size_t yzOffset = zOffset + static_cast<std::size_t>(y) * voxelDim;

            for (std::uint32_t x = 0; x < voxelDim; ++x) {
                const std::size_t texIdx = static_cast<std::size_t>(tX[x] + tyRow);

                float intensity = 0.0f;
                if (isRgba) {
                    const std::size_t rgbaIdx = texIdx * 4U;
                    const float rVal = inTextureRgba[rgbaIdx + 0];
                    const float gVal = inTextureRgba[rgbaIdx + 1];
                    const float bVal = inTextureRgba[rgbaIdx + 2];
                    intensity = 0.299f * rVal + 0.587f * gVal + 0.114f * bVal;
                } else if (texIdx < inTextureRgba.size()) {
                    intensity = inTextureRgba[texIdx];
                }

                // Modulate by radial falloff in 3D volume
                const float radialFalloff = std::clamp(1.0f - (pX2[x] + py2pz2) * (1.0f / 3.0f), 0.0f, 1.0f);
                outVoxelGrid[yzOffset + x] = intensity * radialFalloff;
            }
        }
    }
}

void displaceSamplesWith2DTexture(
    SculptureSnapshot& snapshot,
    const std::vector<float>& inTextureRgba,
    std::uint32_t width,
    std::uint32_t height,
    float displacementScale
) {
    if (width == 0 || height == 0 || inTextureRgba.empty() || snapshot.samples.empty()) return;
    const bool isRgba = (inTextureRgba.size() >= static_cast<std::size_t>(width) * height * 4U);

    for (auto& sample : snapshot.samples) {
        if (sample.radius <= 0.0f) continue;

        // Project sample (x, y, z) to UV space [0, 1]^2
        const float u = std::clamp(0.5f + sample.x * 0.25f, 0.0f, 1.0f);
        const float v = std::clamp(0.5f + sample.y * 0.25f, 0.0f, 1.0f);

        const std::uint32_t tx = std::min(static_cast<std::uint32_t>(u * (width - 1U)), width - 1U);
        const std::uint32_t ty = std::min(static_cast<std::uint32_t>(v * (height - 1U)), height - 1U);
        const std::size_t texIdx = static_cast<std::size_t>(tx + ty * width);

        float intensity = 0.0f;
        if (isRgba) {
            const std::size_t rgbaIdx = texIdx * 4U;
            const float rVal = inTextureRgba[rgbaIdx + 0];
            const float gVal = inTextureRgba[rgbaIdx + 1];
            const float bVal = inTextureRgba[rgbaIdx + 2];
            intensity = 0.299f * rVal + 0.587f * gVal + 0.114f * bVal;
        } else if (texIdx < inTextureRgba.size()) {
            intensity = inTextureRgba[texIdx];
        }

        // Apply normal/radial scalar displacement
        const float r = std::sqrt(sample.x * sample.x + sample.y * sample.y + sample.z * sample.z);
        if (r > 1e-4f) {
            const float dirX = sample.x / r;
            const float dirY = sample.y / r;
            const float dirZ = sample.z / r;
            const float disp = (intensity - 0.5f) * displacementScale;
            sample.x += dirX * disp;
            sample.y += dirY * disp;
            sample.z += dirZ * disp;
            sample.radius = std::clamp(sample.radius * (1.0f + 0.5f * intensity), 0.001f, 0.25f);
        }
    }
}


ReliefMesh generateEpimorphicReliefMesh(
    const std::vector<float>& inFeedbackField,
    std::uint32_t fieldWidth,
    std::uint32_t fieldHeight,
    std::uint32_t gridResX,
    std::uint32_t gridResY,
    float heightScale,
    float curvaturePower,
    bool applyPoincareDisk
) {
    ReliefMesh mesh;
    if (gridResX < 2 || gridResY < 2) return mesh;

    const bool hasField = !inFeedbackField.empty() && fieldWidth > 0 && fieldHeight > 0;
    const bool isRgba = hasField && (inFeedbackField.size() >= static_cast<std::size_t>(fieldWidth) * fieldHeight * 4U);

    auto sampleField = [&](float u, float v, float& r, float& g, float& b, float& a) -> float {
        if (!hasField) {
            r = 0.8f; g = 0.5f; b = 1.0f; a = 1.0f;
            return 0.0f;
        }
        const float cu = std::clamp(u, 0.0f, 1.0f);
        const float cv = std::clamp(v, 0.0f, 1.0f);
        const float fx = cu * static_cast<float>(fieldWidth - 1);
        const float fy = cv * static_cast<float>(fieldHeight - 1);
        const std::uint32_t x0 = static_cast<std::uint32_t>(fx);
        const std::uint32_t y0 = static_cast<std::uint32_t>(fy);
        const std::uint32_t x1 = std::min(x0 + 1U, fieldWidth - 1U);
        const std::uint32_t y1 = std::min(y0 + 1U, fieldHeight - 1U);
        const float tx = fx - static_cast<float>(x0);
        const float ty = fy - static_cast<float>(y0);

        auto getPixel = [&](std::uint32_t x, std::uint32_t y, float& pr, float& pg, float& pb, float& pa) -> float {
            const std::size_t idx = static_cast<std::size_t>(x + y * fieldWidth);
            if (isRgba) {
                const std::size_t rIdx = idx * 4U;
                pr = inFeedbackField[rIdx + 0];
                pg = inFeedbackField[rIdx + 1];
                pb = inFeedbackField[rIdx + 2];
                pa = inFeedbackField[rIdx + 3];
                return 0.299f * pr + 0.587f * pg + 0.114f * pb;
            }
            if (idx < inFeedbackField.size()) {
                const float val = inFeedbackField[idx];
                pr = val; pg = val; pb = val; pa = 1.0f;
                return val;
            }
            pr = 0.0f; pg = 0.0f; pb = 0.0f; pa = 1.0f;
            return 0.0f;
        };

        float r00, g00, b00, a00, r10, g10, b10, a10;
        float r01, g01, b01, a01, r11, g11, b11, a11;
        const float v00 = getPixel(x0, y0, r00, g00, b00, a00);
        const float v10 = getPixel(x1, y0, r10, g10, b10, a10);
        const float v01 = getPixel(x0, y1, r01, g01, b01, a01);
        const float v11 = getPixel(x1, y1, r11, g11, b11, a11);

        const float top = v00 * (1.0f - tx) + v10 * tx;
        const float bot = v01 * (1.0f - tx) + v11 * tx;
        const float intensity = top * (1.0f - ty) + bot * ty;

        r = (r00 * (1.0f - tx) + r10 * tx) * (1.0f - ty) + (r01 * (1.0f - tx) + r11 * tx) * ty;
        g = (g00 * (1.0f - tx) + g10 * tx) * (1.0f - ty) + (g01 * (1.0f - tx) + g11 * tx) * ty;
        b = (b00 * (1.0f - tx) + b10 * tx) * (1.0f - ty) + (b01 * (1.0f - tx) + b11 * tx) * ty;
        a = (a00 * (1.0f - tx) + a10 * tx) * (1.0f - ty) + (a01 * (1.0f - tx) + a11 * tx) * ty;

        return intensity;
    };

    // Height is a signed nonlinear lift of field intensity. When the optional
    // Poincare presentation warp is enabled, use the same attenuation for the
    // finite-difference samples used by normals so geometry and shading describe
    // one surface rather than two subtly different ones.
    const auto reliefHeight = [&](float intensity, float u, float v) noexcept {
        float height = std::copysign(
            std::pow(std::abs(intensity), curvaturePower), intensity) * heightScale;
        if (!applyPoincareDisk) return height;

        const float x = u * 2.0f - 1.0f;
        const float y = v * 2.0f - 1.0f;
        const float radiusSquared = std::min(x * x + y * y, 0.995f * 0.995f);
        const float factor = (1.0f - radiusSquared) / (1.0f + radiusSquared);
        return height * factor;
    };

    const std::size_t vertexCount = static_cast<std::size_t>(gridResX) * gridResY;
    mesh.vertices.resize(vertexCount);
    mesh.minHeight = 1e9f;
    mesh.maxHeight = -1e9f;

    const float dx = 1.0f / static_cast<float>(gridResX - 1);
    const float dy = 1.0f / static_cast<float>(gridResY - 1);
    const float epsU = 0.5f * dx;
    const float epsV = 0.5f * dy;

    float totalCurvature = 0.0f;

    for (std::uint32_t gy = 0; gy < gridResY; ++gy) {
        for (std::uint32_t gx = 0; gx < gridResX; ++gx) {
            const float u = static_cast<float>(gx) * dx;
            const float v = static_cast<float>(gy) * dy;
            const std::size_t vIdx = static_cast<std::size_t>(gx + gy * gridResX);

            float pr, pg, pb, pa;
            const float intensity = sampleField(u, v, pr, pg, pb, pa);

            float posX = u * 2.0f - 1.0f;
            float posY = v * 2.0f - 1.0f;
            const float rad2 = posX * posX + posY * posY;
            const float rad = std::sqrt(rad2);

            const float h = reliefHeight(intensity, u, v);

            if (applyPoincareDisk && rad > 1e-5f) {
                // Presentation geometry is warped in XY only. The sampled field
                // remains canonical and is never rewritten by this operation.
                const float warpedR = std::tanh(rad * 1.5f);
                posX = (posX / rad) * warpedR;
                posY = (posY / rad) * warpedR;
            }

            // Estimate the normal from exactly the same signed height function
            // as the vertex. Negative lobes and Poincare attenuation therefore
            // contribute consistently to both displacement and shading.
            float dr, dg, db, da;
            const float rightSample = sampleField(u + epsU, v, dr, dg, db, da);
            const float leftSample  = sampleField(u - epsU, v, dr, dg, db, da);
            const float upSample    = sampleField(u, v + epsV, dr, dg, db, da);
            const float downSample  = sampleField(u, v - epsV, dr, dg, db, da);
            const float hRight = reliefHeight(rightSample, u + epsU, v);
            const float hLeft  = reliefHeight(leftSample, u - epsU, v);
            const float hUp    = reliefHeight(upSample, u, v + epsV);
            const float hDown  = reliefHeight(downSample, u, v - epsV);

            const float dh_dx = (hRight - hLeft) / (2.0f * epsU * 2.0f);
            const float dh_dy = (hUp - hDown) / (2.0f * epsV * 2.0f);

            // Normal = normalize(-dh/dx, -dh/dy, 1.0)
            const float len = std::sqrt(dh_dx * dh_dx + dh_dy * dh_dy + 1.0f);
            const float nx = -dh_dx / len;
            const float ny = -dh_dy / len;
            const float nz = 1.0f / len;

            auto& vert = mesh.vertices[vIdx];
            vert.x = posX;
            vert.y = posY;
            vert.z = h;
            vert.nx = nx;
            vert.ny = ny;
            vert.nz = nz;
            vert.u = u;
            vert.v = v;
            vert.r = pr;
            vert.g = pg;
            vert.b = pb;
            vert.a = pa;

            mesh.minHeight = std::min(mesh.minHeight, h);
            mesh.maxHeight = std::max(mesh.maxHeight, h);
            totalCurvature += std::abs(dh_dx) + std::abs(dh_dy);
        }
    }

    mesh.meanCurvature = (vertexCount > 0) ? (totalCurvature / static_cast<float>(vertexCount)) : 0.0f;

    // Generate triangle indices
    const std::size_t quadCount = static_cast<std::size_t>(gridResX - 1) * (gridResY - 1);
    mesh.indices.reserve(quadCount * 6U);

    for (std::uint32_t gy = 0; gy < gridResY - 1; ++gy) {
        for (std::uint32_t gx = 0; gx < gridResX - 1; ++gx) {
            const std::uint32_t i0 = gx + gy * gridResX;
            const std::uint32_t i1 = (gx + 1) + gy * gridResX;
            const std::uint32_t i2 = gx + (gy + 1) * gridResX;
            const std::uint32_t i3 = (gx + 1) + (gy + 1) * gridResX;

            // Triangle 1
            mesh.indices.push_back(i0);
            mesh.indices.push_back(i2);
            mesh.indices.push_back(i1);

            // Triangle 2
            mesh.indices.push_back(i1);
            mesh.indices.push_back(i2);
            mesh.indices.push_back(i3);
        }
    }

    return mesh;
}

ReliefMesh convertIsosurfaceToReliefMesh(const IsosurfaceMesh& isoMesh) {
    ReliefMesh relief;
    relief.vertices.reserve(isoMesh.vertices.size());
    relief.indices = isoMesh.indices;
    if (!isoMesh.vertices.empty()) {
        relief.minHeight = isoMesh.vertices.front().position.z;
        relief.maxHeight = isoMesh.vertices.front().position.z;
    }

    float totalCurvature = 0.0f;
    for (const auto& v : isoMesh.vertices) {
        ReliefVertex rv;
        rv.x = v.position.x;
        rv.y = v.position.y;
        rv.z = v.position.z;
        rv.nx = v.normal.x;
        rv.ny = v.normal.y;
        rv.nz = v.normal.z;
        rv.u = v.materialCoord;
        rv.v = v.density;
        rv.r = std::clamp(v.normal.x * 0.5f + 0.5f, 0.0f, 1.0f);
        rv.g = std::clamp(v.normal.y * 0.5f + 0.5f, 0.0f, 1.0f);
        rv.b = std::clamp(v.normal.z * 0.5f + 0.5f, 0.0f, 1.0f);
        rv.a = std::clamp(v.density, 0.0f, 1.0f);

        relief.vertices.push_back(rv);
        relief.minHeight = std::min(relief.minHeight, v.position.z);
        relief.maxHeight = std::max(relief.maxHeight, v.position.z);
        totalCurvature += std::abs(v.normal.x) + std::abs(v.normal.y);
    }
    relief.meanCurvature = isoMesh.vertices.empty() ? 0.0f : (totalCurvature / static_cast<float>(isoMesh.vertices.size()));
    return relief;
}

IsosurfaceMesh generateFoliationSurfaceMesh(
    Manifold4DType type,
    float thetaL,
    float thetaR,
    float scale,
    std::size_t uDivisions,
    std::size_t vDivisions
) {
    switch (type) {
    case Manifold4DType::CliffordTorus:
        return Sculpture4DFoliation::generateVillarceauFoliationMesh(
            std::clamp<std::size_t>(uDivisions, 4U, 32U),
            std::clamp<std::size_t>(vDivisions, 16U, 64U),
            0.04f, thetaL, thetaR, scale);
    case Manifold4DType::CalabiYauK3:
    case Manifold4DType::CalabiYauQuinticThreefold:
        return Sculpture4DFoliation::generateCalabiYauCrossSectionMesh(
            std::clamp<std::size_t>(uDivisions, 8U, 48U),
            std::clamp<std::size_t>(vDivisions, 8U, 48U),
            thetaL, thetaR, 0.0f, scale);
    case Manifold4DType::HopfFibrationS3:
        return Sculpture4DFoliation::generateHopfTorusKnotMesh(
            3, 2, std::clamp<std::size_t>(uDivisions * 4, 32U, 128U),
            0.05f, thetaL, thetaR, scale);
    case Manifold4DType::LieE8CoxeterProjection:
        return Sculpture4DFoliation::generateLieE8CoxeterProjectionMesh(
            0.04f, thetaL, thetaR, scale);
    case Manifold4DType::SeifertHyperbolicSoliton:
        return Sculpture4DFoliation::generateSeifertSurfaceMesh(
            3, 2,
            std::clamp<std::size_t>(uDivisions, 8U, 32U),
            std::clamp<std::size_t>(vDivisions, 16U, 64U),
            thetaL, thetaR, scale);
    case Manifold4DType::OctonionicAssociatorS7:
        return Sculpture4DFoliation::generateOctonionicG2FibrationMesh(
            std::clamp<std::size_t>(uDivisions / 2, 4U, 16U),
            std::clamp<std::size_t>(vDivisions, 16U, 64U),
            0.05f, thetaL, thetaR, 0.0f, scale);
    case Manifold4DType::HyperbolicHoneycomb4D:
        return Sculpture4DFoliation::generateHyperbolicHoneycombMesh(
            std::clamp<std::size_t>(uDivisions / 4, 2U, 8U),
            0.04f, thetaL, thetaR, scale);
    case Manifold4DType::CliffordHopfSolitonLattice:
        return Sculpture4DFoliation::generateCliffordHopfSolitonLatticeMesh(
            std::clamp<std::size_t>(uDivisions / 3, 3U, 12U),
            std::clamp<std::size_t>(vDivisions, 16U, 64U),
            0.04f, thetaL, thetaR, scale);
    default:
        return Sculpture4DFoliation::generatePolytopeMesh(
            type, 0.05f, thetaL, thetaR, scale);
    }
}


} // namespace epi_sculpt::field_geometry
