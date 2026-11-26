#include "EpimorphicReliefGenerator.h"
#include "Sculpture4DFoliation.h"
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <mutex>
#include <sstream>
#include <thread>
#include <utility>

namespace epi_sculpt {

namespace {

constexpr float kPi = 3.14159265358979323846f;

inline SculptureVec3 crossVec(const SculptureVec3& a, const SculptureVec3& b) noexcept {
    return SculptureVec3{
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

inline SculptureVec3 normalizeVec(const SculptureVec3& v) noexcept {
    const float lenSq = v.x * v.x + v.y * v.y + v.z * v.z;
    if (lenSq > 1e-12f) {
        const float invLen = 1.0f / std::sqrt(lenSq);
        return SculptureVec3{v.x * invLen, v.y * invLen, v.z * invLen};
    }
    return SculptureVec3{0.0f, 1.0f, 0.0f};
}

SculptureVec3 evaluateManifoldPosition(
    float u,
    float vCoord,
    float rawHeight,
    const EpimorphicReliefConfig& config
) noexcept {
    switch (config.projection) {
    case ReliefManifoldProjection::Planar: {
        const float posX = u * 2.0f - 1.0f;
        const float posZ = vCoord * 2.0f - 1.0f;
        return SculptureVec3{posX, rawHeight, posZ};
    }
    case ReliefManifoldProjection::PoincareDisk: {
        float posX = u * 2.0f - 1.0f;
        float posZ = vCoord * 2.0f - 1.0f;
        const float rad2 = posX * posX + posZ * posZ;
        const float rad = std::sqrt(rad2);
        const float rClamped = std::min(rad, 0.995f);
        const float poincareFactor = (1.0f - rClamped * rClamped) / (1.0f + rClamped * rClamped);
        const float h = rawHeight * poincareFactor;
        if (rad > 1e-5f) {
            const float warpedR = std::tanh(rad * 1.5f);
            posX = (posX / rad) * warpedR;
            posZ = (posZ / rad) * warpedR;
        }
        return SculptureVec3{posX, h, posZ};
    }
    case ReliefManifoldProjection::KleinBeltrami: {
        float posX = u * 2.0f - 1.0f;
        float posZ = vCoord * 2.0f - 1.0f;
        const float rad2 = posX * posX + posZ * posZ;
        const float rad = std::sqrt(rad2);
        const float rClamped = std::min(rad, 0.995f);
        const float kleinFactor = std::sqrt(std::max(0.0f, 1.0f - rClamped * rClamped));
        const float h = rawHeight * kleinFactor;
        const float chordScale = 1.0f / std::sqrt(1.0f + rad * rad);
        posX *= chordScale;
        posZ *= chordScale;
        return SculptureVec3{posX, h, posZ};
    }
    case ReliefManifoldProjection::RiemannSphere: {
        const float x = (u * 2.0f - 1.0f) * 2.0f;
        const float z = (vCoord * 2.0f - 1.0f) * 2.0f;
        const float d = 1.0f + x * x + z * z;
        const float sX = (2.0f * x) / d;
        const float sZ = (2.0f * z) / d;
        const float sY = (x * x + z * z - 1.0f) / d;
        const float elevation = 1.0f + rawHeight * 0.4f;
        return SculptureVec3{sX * elevation, sY * elevation, sZ * elevation};
    }
    case ReliefManifoldProjection::ToroidalChamber: {
        const float theta = u * 2.0f * kPi;
        const float phi = vCoord * 2.0f * kPi;
        const float rEff = config.torusMinorRadius + rawHeight * 0.3f;
        const float ringR = config.torusMajorRadius + rEff * std::cos(theta);
        return SculptureVec3{
            ringR * std::cos(phi),
            rEff * std::sin(theta),
            ringR * std::sin(phi)
        };
    }
    case ReliefManifoldProjection::HyperbolicPseudosphere: {
        const float w = (u - 0.5f) * 3.5f;
        const float theta = vCoord * 2.0f * kPi;
        const float a = std::max(0.01f, config.pseudosphereRadius);
        const float sechW = 1.0f / std::cosh(w);
        const float rho = std::max(0.001f, a * sechW + rawHeight * 0.25f);
        const float yAxis = a * (w - std::tanh(w));
        return SculptureVec3{
            rho * std::cos(theta),
            yAxis,
            rho * std::sin(theta)
        };
    }
    }
    return SculptureVec3{u * 2.0f - 1.0f, rawHeight, vCoord * 2.0f - 1.0f};
}

} // namespace

EpimorphicReliefData EpimorphicReliefGenerator::generate(
    const std::vector<float>& feedbackField,
    std::uint32_t fieldWidth,
    std::uint32_t fieldHeight,
    const EpimorphicReliefConfig& config
) {
    EpimorphicReliefData data{};
    if (config.gridResX < 2 || config.gridResY < 2) return data;
    data.projection = config.projection;

    const bool hasField = !feedbackField.empty() && fieldWidth > 0 && fieldHeight > 0;
    const bool isRgba = hasField && (feedbackField.size() >= static_cast<std::size_t>(fieldWidth) * fieldHeight * 4U);

    auto sampleField = [&](float u, float vCoord, float col[4]) -> float {
        if (!hasField) {
            col[0] = 0.85f; col[1] = 0.65f; col[2] = 1.0f; col[3] = 1.0f;
            return 0.0f;
        }
        const float cu = std::clamp(u, 0.0f, 1.0f);
        const float cv = std::clamp(vCoord, 0.0f, 1.0f);
        const float fx = cu * static_cast<float>(fieldWidth - 1);
        const float fy = cv * static_cast<float>(fieldHeight - 1);
        const std::uint32_t x0 = static_cast<std::uint32_t>(fx);
        const std::uint32_t y0 = static_cast<std::uint32_t>(fy);
        const std::uint32_t x1 = std::min(x0 + 1U, fieldWidth - 1U);
        const std::uint32_t y1 = std::min(y0 + 1U, fieldHeight - 1U);
        const float tx = fx - static_cast<float>(x0);
        const float ty = fy - static_cast<float>(y0);

        auto getPixel = [&](std::uint32_t px, std::uint32_t py, float c[4]) -> float {
            const std::size_t pIdx = static_cast<std::size_t>(px + py * fieldWidth);
            if (isRgba) {
                const std::size_t rIdx = pIdx * 4U;
                c[0] = feedbackField[rIdx + 0];
                c[1] = feedbackField[rIdx + 1];
                c[2] = feedbackField[rIdx + 2];
                c[3] = feedbackField[rIdx + 3];
                return 0.299f * c[0] + 0.587f * c[1] + 0.114f * c[2];
            }
            if (pIdx < feedbackField.size()) {
                const float val = feedbackField[pIdx];
                c[0] = val; c[1] = val; c[2] = val; c[3] = 1.0f;
                return val;
            }
            c[0] = 0.0f; c[1] = 0.0f; c[2] = 0.0f; c[3] = 1.0f;
            return 0.0f;
        };

        float c00[4], c10[4], c01[4], c11[4];
        const float v00 = getPixel(x0, y0, c00);
        const float v10 = getPixel(x1, y0, c10);
        const float v01 = getPixel(x0, y1, c01);
        const float v11 = getPixel(x1, y1, c11);

        const float top = v00 * (1.0f - tx) + v10 * tx;
        const float bot = v01 * (1.0f - tx) + v11 * tx;
        const float intensity = top * (1.0f - ty) + bot * ty;

        for (int i = 0; i < 4; ++i) {
            col[i] = (c00[i] * (1.0f - tx) + c10[i] * tx) * (1.0f - ty) + (c01[i] * (1.0f - tx) + c11[i] * tx) * ty;
        }
        return intensity;
    };

    const std::size_t vertexCount = static_cast<std::size_t>(config.gridResX) * config.gridResY;
    data.vertices.resize(vertexCount);

    const float dx = 1.0f / static_cast<float>(config.gridResX - 1);
    const float dy = 1.0f / static_cast<float>(config.gridResY - 1);
    const float epsU = 0.5f * dx;
    const float epsV = 0.5f * dy;

    const unsigned int numThreads = std::max(1u, std::thread::hardware_concurrency());
    std::vector<std::thread> threads;
    
    std::vector<float> minH_threads(numThreads, 1e9f);
    std::vector<float> maxH_threads(numThreads, -1e9f);
    std::vector<double> sumH_threads(numThreads, 0.0);

    auto worker = [&](unsigned int threadIdx) {
        const std::uint32_t startY = (config.gridResY * threadIdx) / numThreads;
        const std::uint32_t endY = (config.gridResY * (threadIdx + 1)) / numThreads;
        
        float localMinH = 1e9f;
        float localMaxH = -1e9f;
        double localSumH = 0.0;

        for (std::uint32_t gy = startY; gy < endY; ++gy) {
            for (std::uint32_t gx = 0; gx < config.gridResX; ++gx) {
                const float u = static_cast<float>(gx) * dx;
                const float vCoord = static_cast<float>(gy) * dy;
                const std::size_t vIdx = static_cast<std::size_t>(gx + gy * config.gridResX);

                float vertColor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
                const float intensity = sampleField(u, vCoord, vertColor);

                const float rawH = std::copysign(
                    std::pow(std::abs(intensity), config.curvaturePower), intensity) * config.heightScale;

                const SculptureVec3 pos = evaluateManifoldPosition(u, vCoord, rawH, config);

                localMinH = std::min(localMinH, pos.y);
                localMaxH = std::max(localMaxH, pos.y);
                localSumH += pos.y;

                // Analytical finite-difference normal on the manifold
                float tempCol[4];
                const float hR = std::copysign(std::pow(std::abs(sampleField(u + epsU, vCoord, tempCol)), config.curvaturePower), 1.0f) * config.heightScale;
                const float hL = std::copysign(std::pow(std::abs(sampleField(u - epsU, vCoord, tempCol)), config.curvaturePower), 1.0f) * config.heightScale;
                const float hU = std::copysign(std::pow(std::abs(sampleField(u, vCoord + epsV, tempCol)), config.curvaturePower), 1.0f) * config.heightScale;
                const float hD = std::copysign(std::pow(std::abs(sampleField(u, vCoord - epsV, tempCol)), config.curvaturePower), 1.0f) * config.heightScale;

                const SculptureVec3 posR = evaluateManifoldPosition(u + epsU, vCoord, hR, config);
                const SculptureVec3 posL = evaluateManifoldPosition(u - epsU, vCoord, hL, config);
                const SculptureVec3 posU = evaluateManifoldPosition(u, vCoord + epsV, hU, config);
                const SculptureVec3 posD = evaluateManifoldPosition(u, vCoord - epsV, hD, config);

                const SculptureVec3 tangentU{posR.x - posL.x, posR.y - posL.y, posR.z - posL.z};
                const SculptureVec3 tangentV{posU.x - posD.x, posU.y - posD.y, posU.z - posD.z};

                SculptureVec3 norm = normalizeVec(crossVec(tangentV, tangentU));
                if (config.projection == ReliefManifoldProjection::Planar) {
                    norm = normalizeVec(crossVec(tangentU, tangentV));
                    norm.y = std::abs(norm.y);
                }

                auto& vert = data.vertices[vIdx];
                vert.position = pos;
                vert.normal = norm;
                vert.u = u;
                vert.v = vCoord;
                vert.r = vertColor[0];
                vert.g = vertColor[1];
                vert.b = vertColor[2];
                vert.a = vertColor[3];
            }
        }
        
        minH_threads[threadIdx] = localMinH;
        maxH_threads[threadIdx] = localMaxH;
        sumH_threads[threadIdx] = localSumH;
    };

    for (unsigned int t = 0; t < numThreads; ++t) {
        threads.emplace_back(worker, t);
    }
    for (auto& t : threads) {
        t.join();
    }
    threads.clear();

    float minH = 1e9f;
    float maxH = -1e9f;
    double sumH = 0.0;
    for (unsigned int t = 0; t < numThreads; ++t) {
        minH = std::min(minH, minH_threads[t]);
        maxH = std::max(maxH, maxH_threads[t]);
        sumH += sumH_threads[t];
    }

    data.minHeight = minH;
    data.maxHeight = maxH;
    data.averageHeight = static_cast<float>(sumH / static_cast<double>(vertexCount));
    data.valid = true;

    // Optional Curvature Analysis & Discrete Metric Colorization
    if (config.colorizeCurvature && config.gridResX > 2 && config.gridResY > 2) {
        float minK = 1e9f;
        float maxK = -1e9f;
        float minHCurv = 1e9f;
        float maxHCurv = -1e9f;

        std::vector<float> gaussK(vertexCount, 0.0f);
        std::vector<float> meanH(vertexCount, 0.0f);

        for (std::uint32_t gy = 1; gy < config.gridResY - 1; ++gy) {
            for (std::uint32_t gx = 1; gx < config.gridResX - 1; ++gx) {
                const std::size_t idx = static_cast<std::size_t>(gx + gy * config.gridResX);
                const float hC = data.vertices[idx].position.y;
                const float hR = data.vertices[idx + 1].position.y;
                const float hL = data.vertices[idx - 1].position.y;
                const float hU = data.vertices[idx + config.gridResX].position.y;
                const float hD = data.vertices[idx - config.gridResX].position.y;
                const float hUR = data.vertices[idx + config.gridResX + 1].position.y;
                const float hUL = data.vertices[idx + config.gridResX - 1].position.y;
                const float hDR = data.vertices[idx - config.gridResX + 1].position.y;
                const float hDL = data.vertices[idx - config.gridResX - 1].position.y;

                const float invDx2 = 1.0f / (dx * dx);
                const float invDy2 = 1.0f / (dy * dy);
                const float hxx = (hR - 2.0f * hC + hL) * invDx2;
                const float hyy = (hU - 2.0f * hC + hD) * invDy2;
                const float hxy = ((hUR - hUL) - (hDR - hDL)) / (4.0f * dx * dy);

                const float hx = (hR - hL) / (2.0f * dx);
                const float hy = (hU - hD) / (2.0f * dy);
                const float denom = 1.0f + hx * hx + hy * hy;

                const float K = (hxx * hyy - hxy * hxy) / std::max(1e-6f, denom * denom);
                const float H = ((1.0f + hx * hx) * hyy - 2.0f * hx * hy * hxy + (1.0f + hy * hy) * hxx)
                              / (2.0f * std::pow(std::max(1e-6f, denom), 1.5f));

                gaussK[idx] = K;
                meanH[idx] = H;

                minK = std::min(minK, K);
                maxK = std::max(maxK, K);
                minHCurv = std::min(minHCurv, H);
                maxHCurv = std::max(maxHCurv, H);
            }
        }

        data.gaussianCurvatureMin = minK;
        data.gaussianCurvatureMax = maxK;
        data.meanCurvatureMin = minHCurv;
        data.meanCurvatureMax = maxHCurv;

        // Map Gaussian curvature to pseudocolor (Cyan = Saddle/K<0, Green = Flat/K=0, Orange/Red = Dome/K>0)
        for (std::size_t i = 0; i < vertexCount; ++i) {
            const float kVal = std::clamp(gaussK[i] * 0.1f, -1.0f, 1.0f);
            if (kVal < 0.0f) {
                const float t = -kVal;
                data.vertices[i].r = (1.0f - t) * 0.2f;
                data.vertices[i].g = (1.0f - t) * 0.8f + t * 0.9f;
                data.vertices[i].b = 0.95f;
            } else {
                const float t = kVal;
                data.vertices[i].r = 0.95f;
                data.vertices[i].g = (1.0f - t) * 0.8f + t * 0.2f;
                data.vertices[i].b = (1.0f - t) * 0.2f;
            }
        }
    }

    const std::size_t quadCount = static_cast<std::size_t>(config.gridResX - 1) * (config.gridResY - 1);
    data.indices.resize(quadCount * 6U);

    auto idxWorker = [&](unsigned int threadIdx) {
        const std::uint32_t startY = ((config.gridResY - 1) * threadIdx) / numThreads;
        const std::uint32_t endY = ((config.gridResY - 1) * (threadIdx + 1)) / numThreads;
        
        for (std::uint32_t gy = startY; gy < endY; ++gy) {
            for (std::uint32_t gx = 0; gx < config.gridResX - 1; ++gx) {
                const std::uint32_t i0 = gx + gy * config.gridResX;
                const std::uint32_t i1 = (gx + 1) + gy * config.gridResX;
                const std::uint32_t i2 = gx + (gy + 1) * config.gridResX;
                const std::uint32_t i3 = (gx + 1) + (gy + 1) * config.gridResX;
                
                const std::size_t quadIdx = gx + gy * (config.gridResX - 1);
                const std::size_t outBase = quadIdx * 6U;

                data.indices[outBase + 0] = i0;
                data.indices[outBase + 1] = i2;
                data.indices[outBase + 2] = i1;
                data.indices[outBase + 3] = i1;
                data.indices[outBase + 4] = i2;
                data.indices[outBase + 5] = i3;
            }
        }
    };
    
    for (unsigned int t = 0; t < numThreads; ++t) {
        threads.emplace_back(idxWorker, t);
    }
    for (auto& t : threads) {
        t.join();
    }
    threads.clear();

    // Watertight Solid Closure for 3D Printing & Digital Fabrication
    if (config.closeSolidMesh) {
        if (config.projection == ReliefManifoldProjection::Planar ||
            config.projection == ReliefManifoldProjection::PoincareDisk ||
            config.projection == ReliefManifoldProjection::KleinBeltrami) {
            
            const float baseFloorY = data.minHeight - std::max(0.02f, config.solidBaseDepth);
            const std::uint32_t topVertexCount = static_cast<std::uint32_t>(vertexCount);
            
            // Duplicate bottom vertices
            data.vertices.reserve(vertexCount * 2U);
            for (std::size_t i = 0; i < vertexCount; ++i) {
                ReliefMeshVertex bottomVert = data.vertices[i];
                bottomVert.position.y = baseFloorY;
                bottomVert.normal = SculptureVec3{0.0f, -1.0f, 0.0f};
                bottomVert.r = 0.35f;
                bottomVert.g = 0.35f;
                bottomVert.b = 0.35f;
                bottomVert.a = 1.0f;
                data.vertices.push_back(bottomVert);
            }

            // Add bottom base triangles (reverse winding so normal faces down)
            for (std::uint32_t gy = 0; gy < config.gridResY - 1; ++gy) {
                for (std::uint32_t gx = 0; gx < config.gridResX - 1; ++gx) {
                    const std::uint32_t b0 = topVertexCount + gx + gy * config.gridResX;
                    const std::uint32_t b1 = topVertexCount + (gx + 1) + gy * config.gridResX;
                    const std::uint32_t b2 = topVertexCount + gx + (gy + 1) * config.gridResX;
                    const std::uint32_t b3 = topVertexCount + (gx + 1) + (gy + 1) * config.gridResX;

                    data.indices.push_back(b0);
                    data.indices.push_back(b1);
                    data.indices.push_back(b2);

                    data.indices.push_back(b1);
                    data.indices.push_back(b3);
                    data.indices.push_back(b2);
                }
            }

            // Add 4 perimeter skirt walls with strictly consistent outward winding:
            // 1. Bottom wall (gy = 0, facing -Z)
            for (std::uint32_t gx = 0; gx < config.gridResX - 1; ++gx) {
                const std::uint32_t t0 = gx;
                const std::uint32_t t1 = gx + 1;
                const std::uint32_t b0 = topVertexCount + gx;
                const std::uint32_t b1 = topVertexCount + gx + 1;

                data.indices.push_back(t0);
                data.indices.push_back(t1);
                data.indices.push_back(b0);

                data.indices.push_back(t1);
                data.indices.push_back(b1);
                data.indices.push_back(b0);
            }

            // 2. Top wall (gy = gridResY - 1, facing +Z)
            const std::uint32_t lastRowOffset = (config.gridResY - 1) * config.gridResX;
            for (std::uint32_t gx = 0; gx < config.gridResX - 1; ++gx) {
                const std::uint32_t t0 = lastRowOffset + gx;
                const std::uint32_t t1 = lastRowOffset + gx + 1;
                const std::uint32_t b0 = topVertexCount + lastRowOffset + gx;
                const std::uint32_t b1 = topVertexCount + lastRowOffset + gx + 1;

                data.indices.push_back(t0);
                data.indices.push_back(b0);
                data.indices.push_back(t1);

                data.indices.push_back(t1);
                data.indices.push_back(b0);
                data.indices.push_back(b1);
            }

            // 3. Left wall (gx = 0, facing -X)
            for (std::uint32_t gy = 0; gy < config.gridResY - 1; ++gy) {
                const std::uint32_t t0 = gy * config.gridResX;
                const std::uint32_t t2 = (gy + 1) * config.gridResX;
                const std::uint32_t b0 = topVertexCount + gy * config.gridResX;
                const std::uint32_t b2 = topVertexCount + (gy + 1) * config.gridResX;

                data.indices.push_back(t0);
                data.indices.push_back(b0);
                data.indices.push_back(t2);

                data.indices.push_back(t2);
                data.indices.push_back(b0);
                data.indices.push_back(b2);
            }

            // 4. Right wall (gx = gridResX - 1, facing +X)
            for (std::uint32_t gy = 0; gy < config.gridResY - 1; ++gy) {
                const std::uint32_t t1 = (config.gridResX - 1) + gy * config.gridResX;
                const std::uint32_t t3 = (config.gridResX - 1) + (gy + 1) * config.gridResX;
                const std::uint32_t b1 = topVertexCount + (config.gridResX - 1) + gy * config.gridResX;
                const std::uint32_t b3 = topVertexCount + (config.gridResX - 1) + (gy + 1) * config.gridResX;

                data.indices.push_back(t1);
                data.indices.push_back(t3);
                data.indices.push_back(b1);

                data.indices.push_back(t3);
                data.indices.push_back(b3);
                data.indices.push_back(b1);
            }

            data.isSolidWatertight = true;
        } else if (config.projection == ReliefManifoldProjection::ToroidalChamber) {
            // Toroidal Chamber: wrap X seam, Y seam, and corner quad
            // 1. X seam (gx = config.gridResX - 1 connects to 0)
            for (std::uint32_t gy = 0; gy < config.gridResY - 1; ++gy) {
                const std::uint32_t i0 = (config.gridResX - 1) + gy * config.gridResX;
                const std::uint32_t i1 = 0 + gy * config.gridResX;
                const std::uint32_t i2 = (config.gridResX - 1) + (gy + 1) * config.gridResX;
                const std::uint32_t i3 = 0 + (gy + 1) * config.gridResX;

                data.indices.push_back(i0);
                data.indices.push_back(i2);
                data.indices.push_back(i1);

                data.indices.push_back(i1);
                data.indices.push_back(i2);
                data.indices.push_back(i3);
            }

            // 2. Y seam (gy = config.gridResY - 1 connects to 0)
            for (std::uint32_t gx = 0; gx < config.gridResX - 1; ++gx) {
                const std::uint32_t i0 = gx + (config.gridResY - 1) * config.gridResX;
                const std::uint32_t i1 = (gx + 1) + (config.gridResY - 1) * config.gridResX;
                const std::uint32_t i2 = gx + 0;
                const std::uint32_t i3 = (gx + 1) + 0;

                data.indices.push_back(i0);
                data.indices.push_back(i2);
                data.indices.push_back(i1);

                data.indices.push_back(i1);
                data.indices.push_back(i2);
                data.indices.push_back(i3);
            }

            // 3. Corner quad
            const std::uint32_t i0 = (config.gridResX - 1) + (config.gridResY - 1) * config.gridResX;
            const std::uint32_t i1 = 0 + (config.gridResY - 1) * config.gridResX;
            const std::uint32_t i2 = (config.gridResX - 1) + 0;
            const std::uint32_t i3 = 0 + 0;

            data.indices.push_back(i0);
            data.indices.push_back(i2);
            data.indices.push_back(i1);

            data.indices.push_back(i1);
            data.indices.push_back(i2);
            data.indices.push_back(i3);

            data.isSolidWatertight = true;
        } else if (config.projection == ReliefManifoldProjection::RiemannSphere) {
            // Riemann Sphere: wrap longitude X seam and cap North and South poles
            // 1. Longitude X seam (gx = config.gridResX - 1 connects to 0)
            for (std::uint32_t gy = 0; gy < config.gridResY - 1; ++gy) {
                const std::uint32_t i0 = (config.gridResX - 1) + gy * config.gridResX;
                const std::uint32_t i1 = 0 + gy * config.gridResX;
                const std::uint32_t i2 = (config.gridResX - 1) + (gy + 1) * config.gridResX;
                const std::uint32_t i3 = 0 + (gy + 1) * config.gridResX;

                data.indices.push_back(i0);
                data.indices.push_back(i2);
                data.indices.push_back(i1);

                data.indices.push_back(i1);
                data.indices.push_back(i2);
                data.indices.push_back(i3);
            }

            const std::uint32_t vSouth = static_cast<std::uint32_t>(data.vertices.size());
            ReliefMeshVertex southPoleVert{};
            southPoleVert.position = SculptureVec3{0.0f, -1.0f - data.averageHeight * 0.4f, 0.0f};
            southPoleVert.normal = SculptureVec3{0.0f, -1.0f, 0.0f};
            southPoleVert.r = 0.4f; southPoleVert.g = 0.4f; southPoleVert.b = 0.5f; southPoleVert.a = 1.0f;
            data.vertices.push_back(southPoleVert);

            const std::uint32_t vNorth = static_cast<std::uint32_t>(data.vertices.size());
            ReliefMeshVertex northPoleVert{};
            northPoleVert.position = SculptureVec3{0.0f, 1.0f + data.averageHeight * 0.4f, 0.0f};
            northPoleVert.normal = SculptureVec3{0.0f, 1.0f, 0.0f};
            northPoleVert.r = 0.4f; northPoleVert.g = 0.4f; northPoleVert.b = 0.5f; northPoleVert.a = 1.0f;
            data.vertices.push_back(northPoleVert);

            // South Pole fan (winding matching bottom boundary)
            for (std::uint32_t gx = 0; gx < config.gridResX - 1; ++gx) {
                data.indices.push_back(vSouth);
                data.indices.push_back(gx);
                data.indices.push_back(gx + 1);
            }
            data.indices.push_back(vSouth);
            data.indices.push_back(config.gridResX - 1);
            data.indices.push_back(0);

            // North Pole fan (winding matching top boundary)
            const std::uint32_t topOff = (config.gridResY - 1) * config.gridResX;
            for (std::uint32_t gx = 0; gx < config.gridResX - 1; ++gx) {
                data.indices.push_back(vNorth);
                data.indices.push_back(topOff + gx + 1);
                data.indices.push_back(topOff + gx);
            }
            data.indices.push_back(vNorth);
            data.indices.push_back(topOff + 0);
            data.indices.push_back(topOff + config.gridResX - 1);

            data.isSolidWatertight = true;
        } else if (config.projection == ReliefManifoldProjection::HyperbolicPseudosphere) {
            // Pseudosphere: wrap angle Y seam and cap W_min and W_max boundaries
            // 1. Angle Y seam (theta around circular cross-section, gy = config.gridResY - 1 connects to 0)
            for (std::uint32_t gx = 0; gx < config.gridResX - 1; ++gx) {
                const std::uint32_t i0 = gx + (config.gridResY - 1) * config.gridResX;
                const std::uint32_t i1 = (gx + 1) + (config.gridResY - 1) * config.gridResX;
                const std::uint32_t i2 = gx + 0;
                const std::uint32_t i3 = (gx + 1) + 0;

                data.indices.push_back(i0);
                data.indices.push_back(i2);
                data.indices.push_back(i1);

                data.indices.push_back(i1);
                data.indices.push_back(i2);
                data.indices.push_back(i3);
            }

            float yStart = 0.0f;
            float yEnd = 0.0f;
            for (std::uint32_t gy = 0; gy < config.gridResY; ++gy) {
                yStart += data.vertices[gy * config.gridResX].position.y;
                yEnd += data.vertices[(config.gridResX - 1) + gy * config.gridResX].position.y;
            }
            yStart /= static_cast<float>(config.gridResY);
            yEnd /= static_cast<float>(config.gridResY);

            const std::uint32_t vStart = static_cast<std::uint32_t>(data.vertices.size());
            ReliefMeshVertex startCapVert{};
            startCapVert.position = SculptureVec3{0.0f, yStart, 0.0f};
            startCapVert.normal = SculptureVec3{0.0f, -1.0f, 0.0f};
            startCapVert.r = 0.45f; startCapVert.g = 0.45f; startCapVert.b = 0.45f; startCapVert.a = 1.0f;
            data.vertices.push_back(startCapVert);

            const std::uint32_t vEnd = static_cast<std::uint32_t>(data.vertices.size());
            ReliefMeshVertex endCapVert{};
            endCapVert.position = SculptureVec3{0.0f, yEnd, 0.0f};
            endCapVert.normal = SculptureVec3{0.0f, 1.0f, 0.0f};
            endCapVert.r = 0.45f; endCapVert.g = 0.45f; endCapVert.b = 0.45f; endCapVert.a = 1.0f;
            data.vertices.push_back(endCapVert);

            // Start cap fan (gx = 0)
            for (std::uint32_t gy = 0; gy < config.gridResY - 1; ++gy) {
                const std::uint32_t i0 = 0 + gy * config.gridResX;
                const std::uint32_t i2 = 0 + (gy + 1) * config.gridResX;
                data.indices.push_back(vStart);
                data.indices.push_back(i2);
                data.indices.push_back(i0);
            }
            data.indices.push_back(vStart);
            data.indices.push_back(0);
            data.indices.push_back((config.gridResY - 1) * config.gridResX);

            // End cap fan (gx = config.gridResX - 1)
            for (std::uint32_t gy = 0; gy < config.gridResY - 1; ++gy) {
                const std::uint32_t i1 = (config.gridResX - 1) + gy * config.gridResX;
                const std::uint32_t i3 = (config.gridResX - 1) + (gy + 1) * config.gridResX;
                data.indices.push_back(vEnd);
                data.indices.push_back(i1);
                data.indices.push_back(i3);
            }
            data.indices.push_back(vEnd);
            data.indices.push_back((config.gridResX - 1) + (config.gridResY - 1) * config.gridResX);
            data.indices.push_back(config.gridResX - 1);

            data.isSolidWatertight = true;
        }
    }

    return data;
}

EpimorphicReliefData EpimorphicReliefGenerator::generate(
    const std::vector<float>& feedbackField,
    std::uint32_t fieldWidth,
    std::uint32_t fieldHeight,
    std::uint32_t gridResX,
    std::uint32_t gridResY,
    float heightScale,
    float curvaturePower,
    bool applyPoincareDisk
) {
    EpimorphicReliefConfig config{};
    config.gridResX = gridResX;
    config.gridResY = gridResY;
    config.heightScale = heightScale;
    config.curvaturePower = curvaturePower;
    config.projection = applyPoincareDisk ? ReliefManifoldProjection::PoincareDisk : ReliefManifoldProjection::Planar;
    return generate(feedbackField, fieldWidth, fieldHeight, config);
}

// -----------------------------------------------------------------------------
// 3D Printing & Digital Fabrication Mesh Exporters
// -----------------------------------------------------------------------------

std::string EpimorphicReliefGenerator::exportObjToString(
    const EpimorphicReliefData& data,
    std::string_view objectName
) {
    if (!data.valid || data.vertices.empty() || data.indices.empty()) {
        return {};
    }

    std::ostringstream ss;
    ss << "# Wavefront OBJ generated by epiC EpimorphicReliefGenerator\n";
    ss << "# Manifold Projection: " << name(data.projection) << "\n";
    ss << "# Vertices: " << data.vertices.size() << " | Triangles: " << data.triangleCount() << "\n";
    ss << "# Solid Watertight: " << (data.isSolidWatertight ? "YES" : "NO") << "\n\n";

    ss << "o " << objectName << "\n\n";

    // Write vertices with vertex colors
    ss << std::fixed << std::setprecision(6);
    for (const auto& v : data.vertices) {
        ss << "v " << v.position.x << " " << v.position.y << " " << v.position.z
           << " " << v.r << " " << v.g << " " << v.b << "\n";
    }
    ss << "\n";

    // Write texture coordinates
    for (const auto& v : data.vertices) {
        ss << "vt " << v.u << " " << v.v << "\n";
    }
    ss << "\n";

    // Write vertex normals
    for (const auto& v : data.vertices) {
        ss << "vn " << v.normal.x << " " << v.normal.y << " " << v.normal.z << "\n";
    }
    ss << "\n";

    // Write triangle faces (1-indexed)
    ss << "s 1\n";
    const std::size_t triCount = data.triangleCount();
    for (std::size_t i = 0; i < triCount; ++i) {
        const std::uint32_t i0 = data.indices[i * 3U + 0] + 1U;
        const std::uint32_t i1 = data.indices[i * 3U + 1] + 1U;
        const std::uint32_t i2 = data.indices[i * 3U + 2] + 1U;
        ss << "f " << i0 << "/" << i0 << "/" << i0 << " "
           << i1 << "/" << i1 << "/" << i1 << " "
           << i2 << "/" << i2 << "/" << i2 << "\n";
    }

    return ss.str();
}

bool EpimorphicReliefGenerator::exportObj(
    const std::string& filepath,
    const EpimorphicReliefData& data,
    std::string_view objectName
) {
    const std::string objContent = exportObjToString(data, objectName);
    if (objContent.empty()) return false;

    std::ofstream out(filepath, std::ios::out | std::ios::trunc);
    if (!out.is_open()) return false;
    out << objContent;
    return out.good();
}

std::vector<std::uint8_t> EpimorphicReliefGenerator::exportStlBinaryToBuffer(
    const EpimorphicReliefData& data,
    std::string_view headerText
) {
    if (!data.valid || data.vertices.empty() || data.indices.empty()) {
        return {};
    }

    const std::uint32_t triCount = static_cast<std::uint32_t>(data.triangleCount());
    const std::size_t totalBytes = 80U + 4U + static_cast<std::size_t>(triCount) * 50U;
    std::vector<std::uint8_t> buffer(totalBytes, 0U);

    // 80-byte header
    const std::size_t copyLen = std::min(headerText.size(), std::size_t{79U});
    std::memcpy(buffer.data(), headerText.data(), copyLen);

    // 4-byte triangle count (little-endian uint32)
    std::memcpy(buffer.data() + 80U, &triCount, sizeof(triCount));

    std::size_t offset = 84U;
    for (std::size_t i = 0; i < triCount; ++i) {
        const auto& v0 = data.vertices[data.indices[i * 3U + 0]];
        const auto& v1 = data.vertices[data.indices[i * 3U + 1]];
        const auto& v2 = data.vertices[data.indices[i * 3U + 2]];

        // Calculate face normal
        const SculptureVec3 edge1{v1.position.x - v0.position.x, v1.position.y - v0.position.y, v1.position.z - v0.position.z};
        const SculptureVec3 edge2{v2.position.x - v0.position.x, v2.position.y - v0.position.y, v2.position.z - v0.position.z};
        const SculptureVec3 faceNorm = normalizeVec(crossVec(edge1, edge2));

        const float triangleFloats[12] = {
            faceNorm.x, faceNorm.y, faceNorm.z,
            v0.position.x, v0.position.y, v0.position.z,
            v1.position.x, v1.position.y, v1.position.z,
            v2.position.x, v2.position.y, v2.position.z
        };

        std::memcpy(buffer.data() + offset, triangleFloats, sizeof(triangleFloats));
        offset += sizeof(triangleFloats);

        const std::uint16_t attributeByteCount = 0U;
        std::memcpy(buffer.data() + offset, &attributeByteCount, sizeof(attributeByteCount));
        offset += sizeof(attributeByteCount);
    }

    return buffer;
}

bool EpimorphicReliefGenerator::exportStlBinary(
    const std::string& filepath,
    const EpimorphicReliefData& data,
    std::string_view headerText
) {
    const auto buffer = exportStlBinaryToBuffer(data, headerText);
    if (buffer.empty()) return false;

    std::ofstream out(filepath, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) return false;
    out.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
    return out.good();
}

bool EpimorphicReliefGenerator::exportStlAscii(
    const std::string& filepath,
    const EpimorphicReliefData& data,
    std::string_view solidName
) {
    if (!data.valid || data.vertices.empty() || data.indices.empty()) {
        return false;
    }

    std::ofstream out(filepath, std::ios::out | std::ios::trunc);
    if (!out.is_open()) return false;

    out << "solid " << solidName << "\n";
    out << std::scientific << std::setprecision(6);

    const std::size_t triCount = data.triangleCount();
    for (std::size_t i = 0; i < triCount; ++i) {
        const auto& v0 = data.vertices[data.indices[i * 3U + 0]];
        const auto& v1 = data.vertices[data.indices[i * 3U + 1]];
        const auto& v2 = data.vertices[data.indices[i * 3U + 2]];

        const SculptureVec3 edge1{v1.position.x - v0.position.x, v1.position.y - v0.position.y, v1.position.z - v0.position.z};
        const SculptureVec3 edge2{v2.position.x - v0.position.x, v2.position.y - v0.position.y, v2.position.z - v0.position.z};
        const SculptureVec3 faceNorm = normalizeVec(crossVec(edge1, edge2));

        out << "  facet normal " << faceNorm.x << " " << faceNorm.y << " " << faceNorm.z << "\n";
        out << "    outer loop\n";
        out << "      vertex " << v0.position.x << " " << v0.position.y << " " << v0.position.z << "\n";
        out << "      vertex " << v1.position.x << " " << v1.position.y << " " << v1.position.z << "\n";
        out << "      vertex " << v2.position.x << " " << v2.position.y << " " << v2.position.z << "\n";
        out << "    endloop\n";
        out << "  endfacet\n";
    }

    out << "endsolid " << solidName << "\n";
    return out.good();
}

EpimorphicReliefGenerator::MeshTopologyReport EpimorphicReliefGenerator::analyzeTopology(
    const EpimorphicReliefData& data
) {
    MeshTopologyReport report{};
    report.vertexCount = data.vertices.size();
    report.triangleCount = data.triangleCount();

    if (report.vertexCount == 0 || report.triangleCount == 0) {
        return report;
    }

    report.boundingBoxMin = data.vertices[0].position;
    report.boundingBoxMax = data.vertices[0].position;
    for (const auto& vert : data.vertices) {
        report.boundingBoxMin.x = std::min(report.boundingBoxMin.x, vert.position.x);
        report.boundingBoxMin.y = std::min(report.boundingBoxMin.y, vert.position.y);
        report.boundingBoxMin.z = std::min(report.boundingBoxMin.z, vert.position.z);
        report.boundingBoxMax.x = std::max(report.boundingBoxMax.x, vert.position.x);
        report.boundingBoxMax.y = std::max(report.boundingBoxMax.y, vert.position.y);
        report.boundingBoxMax.z = std::max(report.boundingBoxMax.z, vert.position.z);
    }

    std::map<std::pair<std::uint32_t, std::uint32_t>, std::size_t> undirectedEdges;
    std::map<std::pair<std::uint32_t, std::uint32_t>, std::size_t> halfEdges;

    for (std::size_t i = 0; i < report.triangleCount; ++i) {
        const std::uint32_t i0 = data.indices[i * 3U + 0];
        const std::uint32_t i1 = data.indices[i * 3U + 1];
        const std::uint32_t i2 = data.indices[i * 3U + 2];

        if (i0 == i1 || i1 == i2 || i2 == i0) {
            ++report.degenerateTriangleCount;
            continue;
        }

        const std::uint32_t tri[3] = {i0, i1, i2};
        for (int e = 0; e < 3; ++e) {
            const std::uint32_t u = tri[e];
            const std::uint32_t v = tri[(e + 1) % 3];

            halfEdges[{u, v}]++;
            undirectedEdges[{std::min(u, v), std::max(u, v)}]++;
        }
    }

    report.edgeCount = undirectedEdges.size();

    for (const auto& [edge, count] : undirectedEdges) {
        if (count == 1) {
            ++report.boundaryEdgeCount;
        } else if (count != 2) {
            ++report.nonManifoldEdgeCount;
        }
    }

    for (const auto& [halfEdge, count] : halfEdges) {
        const auto revKey = std::make_pair(halfEdge.second, halfEdge.first);
        auto it = halfEdges.find(revKey);
        if (it != halfEdges.end() && it->second != count) {
            ++report.inconsistentOrientationCount;
        } else if (count > 1) {
            ++report.inconsistentOrientationCount;
        }
    }

    report.eulerCharacteristic = static_cast<int>(report.vertexCount) -
                                 static_cast<int>(report.edgeCount) +
                                 static_cast<int>(report.triangleCount);

    report.isClosedManifold = (report.boundaryEdgeCount == 0 &&
                               report.nonManifoldEdgeCount == 0 &&
                               report.degenerateTriangleCount == 0);

    return report;
}

bool EpimorphicReliefGenerator::isWatertightClosedSolid(const EpimorphicReliefData& data) {
    const auto report = analyzeTopology(data);
    return report.isClosedManifold && report.inconsistentOrientationCount == 0;
}

ReliefManifoldProjection EpimorphicReliefGenerator::resolveReliefProjection(
    const SculptureProgramParams& params
) noexcept {
    if (params.enable4D > 0U) {
        const auto manifoldType = static_cast<Manifold4DType>((params.enable4D - 1U) % kManifold4DTypeCount);
        switch (manifoldType) {
        case Manifold4DType::CliffordTorus:
        case Manifold4DType::SeifertHyperbolicSoliton:
            return ReliefManifoldProjection::ToroidalChamber;
        case Manifold4DType::HopfFibrationS3:
        case Manifold4DType::CalabiYauK3:
        case Manifold4DType::CalabiYauQuinticThreefold:
        case Manifold4DType::DynamicQuaternionJulia:
            return ReliefManifoldProjection::RiemannSphere;
        case Manifold4DType::HyperbolicHoneycomb4D:
            return ReliefManifoldProjection::HyperbolicPseudosphere;
        case Manifold4DType::DynamicHyperMandelbulb:
            return ReliefManifoldProjection::PoincareDisk;
        default:
            break;
        }
    }
    if (params.outputMode == 2 || params.outputMode == 4) {
        return ReliefManifoldProjection::ToroidalChamber;
    }
    if (params.outputMode == 3 || params.outputMode == 10 || params.outputMode == 14) {
        return ReliefManifoldProjection::RiemannSphere;
    }
    if (params.outputMode == 5) {
        return ReliefManifoldProjection::HyperbolicPseudosphere;
    }
    if (params.outputMode == 15) {
        return ReliefManifoldProjection::PoincareDisk;
    }
    if (params.reduction == ReductionMode::PoincareHyperbolic) {
        return ReliefManifoldProjection::PoincareDisk;
    }
    if (params.reduction == ReductionMode::CliffordFoliation || params.reduction == ReductionMode::Torus) {
        return ReliefManifoldProjection::ToroidalChamber;
    }
    if (params.inner == InnerMap::KleinProjective) {
        return ReliefManifoldProjection::KleinBeltrami;
    }
    if (params.inner == InnerMap::HyperbolicHoneycomb || params.inner == InnerMap::HyperbolicMandelboxFold) {
        return ReliefManifoldProjection::HyperbolicPseudosphere;
    }
    return ReliefManifoldProjection::Planar;
}

} // namespace epi_sculpt
