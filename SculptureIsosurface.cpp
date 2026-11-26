#include "SculptureIsosurface.h"
#include "SculptureThreadPool.h"
#include "SculptureFutureDrain.h"
#include <future>
#include <thread>
#include <cmath>
#include <algorithm>
#include <unordered_map>
#include <limits>

namespace epi_sculpt {

namespace {

// -----------------------------------------------------------------------------
// Marching Cubes Canonical Lookup Tables
// -----------------------------------------------------------------------------

// Corner offsets for a unit cube
constexpr int kCornerOffsets[8][3] = {
    {0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0},
    {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}
};

// Edge endpoint pairs (cornerA, cornerB)
constexpr int kEdgeCorners[12][2] = {
    {0, 1}, {1, 2}, {2, 3}, {3, 0},
    {4, 5}, {5, 6}, {6, 7}, {7, 4},
    {0, 4}, {1, 5}, {2, 6}, {3, 7}
};

// 256-entry edge table indicating which edges are cut by the isosurface
#include "SculptureIsosurfaceTables.inl"

SculptureVec3 interpolateVertex(const SculptureVec3& p1, const SculptureVec3& p2, float val1, float val2, float isoLevel) noexcept {
    const float diff = val2 - val1;
    if (std::abs(diff) < 1e-6f) {
        return {
            0.5f * (p1.x + p2.x),
            0.5f * (p1.y + p2.y),
            0.5f * (p1.z + p2.z)
        };
    }
    const float t = std::clamp((isoLevel - val1) / diff, 0.0f, 1.0f);
    return {
        p1.x + t * (p2.x - p1.x),
        p1.y + t * (p2.y - p1.y),
        p1.z + t * (p2.z - p1.z)
    };
}

SculptureVec3 normalizeVec(const SculptureVec3& v) noexcept {
    const float lenSq = v.x * v.x + v.y * v.y + v.z * v.z;
    if (lenSq < 1e-8f) {
        return {0.0f, 1.0f, 0.0f};
    }
    const float invLen = 1.0f / std::sqrt(lenSq);
    return {v.x * invLen, v.y * invLen, v.z * invLen};
}

} // namespace

IsosurfaceMesh SculptureIsosurface::extractMesh(
    const SculptureProgram& program,
    float time,
    float morph,
    const std::vector<float>& spatialBuffer,
    const IsosurfaceConfig& config
) {
    IsosurfaceMesh mesh;
    mesh.isoLevel = config.isoLevel;

    const std::uint32_t res = std::clamp(config.resolution, 4U, 128U);
    const float stepX = (config.boundsMax.x - config.boundsMin.x) / static_cast<float>(res);
    const float stepY = (config.boundsMax.y - config.boundsMin.y) / static_cast<float>(res);
    const float stepZ = (config.boundsMax.z - config.boundsMin.z) / static_cast<float>(res);

    // Pre-evaluate 3D scalar grid to avoid redundant evaluations
    const std::size_t gridPointsX = res + 1;
    const std::size_t gridPointsY = res + 1;
    const std::size_t gridPointsZ = res + 1;
    const std::size_t totalPoints = gridPointsX * gridPointsY * gridPointsZ;

    std::vector<float> gridValues(totalPoints, 0.0f);
    auto pointIndex = [gridPointsX, gridPointsY](std::size_t x, std::size_t y, std::size_t z) -> std::size_t {
        return x + y * gridPointsX + z * (gridPointsX * gridPointsY);
    };

    static SculptureThreadPool sharedWorkerPool(std::thread::hardware_concurrency());
    const std::uint32_t numThreads = std::max<std::uint32_t>(1U, std::thread::hardware_concurrency());
    std::vector<std::future<void>> evalFutures;
    evalFutures.reserve(numThreads);
    detail::SculptureFutureDrain<void> settleEvaluation(evalFutures);
    
    const std::size_t zEvalPerThread = (gridPointsZ + numThreads - 1) / numThreads;
    for (std::uint32_t threadId = 0; threadId < numThreads; ++threadId) {
        const std::size_t startZ = threadId * zEvalPerThread;
        const std::size_t endZ = std::min(startZ + zEvalPerThread, gridPointsZ);
        if (startZ >= endZ) continue;
        
        evalFutures.push_back(sharedWorkerPool.enqueue([&, startZ, endZ]() {
            for (std::size_t z = startZ; z < endZ; ++z) {
                float pz = config.boundsMin.z + static_cast<float>(z) * stepZ;
                for (std::size_t y = 0; y < gridPointsY; ++y) {
                    float py = config.boundsMin.y + static_cast<float>(y) * stepY;
                    for (std::size_t x = 0; x < gridPointsX; ++x) {
                        float px = config.boundsMin.x + static_cast<float>(x) * stepX;
                        SculptureVec3 pos{px, py, pz};
                        gridValues[pointIndex(x, y, z)] = program.sampleDensityField(pos, time, morph, spatialBuffer);
                    }
                }
            }
        }));
    }
    for (auto& f : evalFutures) f.get();

    // March across voxels in parallel
    std::vector<std::future<IsosurfaceMesh>> marchFutures;
    marchFutures.reserve(numThreads);
    detail::SculptureFutureDrain<IsosurfaceMesh> settleMarching(marchFutures);
    const std::size_t zMarchPerThread = (res + numThreads - 1) / numThreads;
    
    for (std::uint32_t threadId = 0; threadId < numThreads; ++threadId) {
        const std::size_t startZ = threadId * zMarchPerThread;
        const std::size_t endZ = std::min(startZ + zMarchPerThread, static_cast<std::size_t>(res));
        if (startZ >= endZ) continue;
        
        marchFutures.push_back(sharedWorkerPool.enqueue([&, startZ, endZ]() -> IsosurfaceMesh {
            IsosurfaceMesh localMesh;
            for (std::size_t z = startZ; z < endZ; ++z) {
                for (std::size_t y = 0; y < res; ++y) {
                    for (std::size_t x = 0; x < res; ++x) {
                        SculptureVec3 cornerPos[8];
                        float cornerVal[8];

                        for (int i = 0; i < 8; ++i) {
                            std::size_t cx = x + kCornerOffsets[i][0];
                            std::size_t cy = y + kCornerOffsets[i][1];
                            std::size_t cz = z + kCornerOffsets[i][2];
                            cornerPos[i] = {
                                config.boundsMin.x + static_cast<float>(cx) * stepX,
                                config.boundsMin.y + static_cast<float>(cy) * stepY,
                                config.boundsMin.z + static_cast<float>(cz) * stepZ
                            };
                            cornerVal[i] = gridValues[pointIndex(cx, cy, cz)];
                        }

                        int cubeIndex = 0;
                        for (int i = 0; i < 8; ++i) {
                            if (cornerVal[i] >= config.isoLevel) {
                                cubeIndex |= (1 << i);
                            }
                        }

                        int edgeFlags = kEdgeTable[cubeIndex];
                        if (edgeFlags == 0) continue;

                        SculptureVec3 edgeVertices[12];
                        for (int e = 0; e < 12; ++e) {
                            if (edgeFlags & (1 << e)) {
                                int c1 = kEdgeCorners[e][0];
                                int c2 = kEdgeCorners[e][1];
                                edgeVertices[e] = interpolateVertex(
                                    cornerPos[c1], cornerPos[c2],
                                    cornerVal[c1], cornerVal[c2],
                                    config.isoLevel
                                );
                            }
                        }

                        for (int t = 0; t < 16 && kTriTable[cubeIndex][t] != -1; t += 3) {
                            if (t + 2 >= 16 || kTriTable[cubeIndex][t + 1] == -1 || kTriTable[cubeIndex][t + 2] == -1) break;
                            for (int j = 0; j < 3; ++j) {
                                int edge = kTriTable[cubeIndex][t + j];
                                if (edge < 0 || edge >= 12) continue;
                                SculptureVec3 pos = edgeVertices[edge];

                                IsosurfaceVertex v{};
                                v.position = pos;
                                v.density = config.isoLevel;

                                if (config.calculateNormals) {
                                    SculptureVec3 grad = program.evaluateFieldGradient(
                                        pos, time, morph, config.normalEpsilon, spatialBuffer
                                    );
                                    v.normal = normalizeVec({-grad.x, -grad.y, -grad.z});
                                } else {
                                    v.normal = {0.0f, 1.0f, 0.0f};
                                }

                                float azimuth = std::atan2(pos.y, pos.x);
                                float radius = std::sqrt(pos.x * pos.x + pos.y * pos.y + pos.z * pos.z);
                                v.materialCoord = std::fmod(azimuth * 0.15915494f + radius * 0.25f + time * 0.05f, 1.0f);
                                if (v.materialCoord < 0.0f) v.materialCoord += 1.0f;

                                std::uint32_t vIndex = static_cast<std::uint32_t>(localMesh.vertices.size());
                                localMesh.vertices.push_back(v);
                                localMesh.indices.push_back(vIndex);
                            }
                        }
                    }
                }
            }
            return localMesh;
        }));
    }
    
    for (auto& f : marchFutures) {
        auto localMesh = f.get();
        std::uint32_t offset = static_cast<std::uint32_t>(mesh.vertices.size());
        mesh.vertices.insert(mesh.vertices.end(), localMesh.vertices.begin(), localMesh.vertices.end());
        for (auto idx : localMesh.indices) {
            mesh.indices.push_back(idx + offset);
        }
    }
    
    computeMeshBounds(mesh);
    return mesh;
}

IsosurfaceMesh SculptureIsosurface::extractFromVoxelBuffer(
    const std::vector<float>& voxelBuffer,
    std::uint32_t gridDim,
    const IsosurfaceConfig& config
) {
    IsosurfaceMesh mesh;
    mesh.isoLevel = config.isoLevel;
    if (voxelBuffer.empty() || gridDim < 2) return mesh;

    const std::size_t N = gridDim;
    // Prove N^3 fits the supplied buffer before multiplying any strides. N is
    // nonzero here; division also rejects cubes unrepresentable in size_t.
    if (N > voxelBuffer.size() / N / N) return mesh;
    const std::size_t planeStride = N * N;

    // A two-sample axis has no interior point: use a forward difference across
    // its sole cell. Larger grids retain the existing centered interior stencil.
    const std::size_t normalBackstep = N > 2 ? 1U : 0U;
    const std::size_t maxNormalCoord = N - 2;

    const float stepX = (config.boundsMax.x - config.boundsMin.x) / static_cast<float>(N - 1);
    const float stepY = (config.boundsMax.y - config.boundsMin.y) / static_cast<float>(N - 1);
    const float stepZ = (config.boundsMax.z - config.boundsMin.z) / static_cast<float>(N - 1);

    auto getVoxel = [&voxelBuffer, N, planeStride](std::size_t x, std::size_t y, std::size_t z) -> float {
        return voxelBuffer[x + y * N + z * planeStride];
    };

    const std::uint32_t numThreads = std::max<std::uint32_t>(1U, std::thread::hardware_concurrency());
    static SculptureThreadPool sharedWorkerPool(std::thread::hardware_concurrency());
    std::vector<std::future<IsosurfaceMesh>> marchFutures;
    marchFutures.reserve(numThreads);
    detail::SculptureFutureDrain<IsosurfaceMesh> settleMarching(marchFutures);
    
    const std::size_t cellsZ = N - 1;
    const std::size_t zMarchPerThread = (cellsZ + numThreads - 1) / numThreads;
    
    for (std::uint32_t threadId = 0; threadId < numThreads; ++threadId) {
        const std::size_t startZ = threadId * zMarchPerThread;
        const std::size_t endZ = std::min(startZ + zMarchPerThread, cellsZ);
        if (startZ >= endZ) continue;
        
        marchFutures.push_back(sharedWorkerPool.enqueue([&, startZ, endZ]() -> IsosurfaceMesh {
            IsosurfaceMesh localMesh;
            for (std::size_t z = startZ; z < endZ; ++z) {
                for (std::size_t y = 0; y < N - 1; ++y) {
                    for (std::size_t x = 0; x < N - 1; ++x) {
                        SculptureVec3 cornerPos[8];
                        float cornerVal[8];

                        for (int i = 0; i < 8; ++i) {
                            std::size_t cx = x + kCornerOffsets[i][0];
                            std::size_t cy = y + kCornerOffsets[i][1];
                            std::size_t cz = z + kCornerOffsets[i][2];
                            cornerPos[i] = {
                                config.boundsMin.x + static_cast<float>(cx) * stepX,
                                config.boundsMin.y + static_cast<float>(cy) * stepY,
                                config.boundsMin.z + static_cast<float>(cz) * stepZ
                            };
                            cornerVal[i] = getVoxel(cx, cy, cz);
                        }

                        int cubeIndex = 0;
                        for (int i = 0; i < 8; ++i) {
                            if (cornerVal[i] >= config.isoLevel) {
                                cubeIndex |= (1 << i);
                            }
                        }

                        int edgeFlags = kEdgeTable[cubeIndex];
                        if (edgeFlags == 0) continue;

                        SculptureVec3 edgeVertices[12];
                        for (int e = 0; e < 12; ++e) {
                            if (edgeFlags & (1 << e)) {
                                int c1 = kEdgeCorners[e][0];
                                int c2 = kEdgeCorners[e][1];
                                edgeVertices[e] = interpolateVertex(
                                    cornerPos[c1], cornerPos[c2],
                                    cornerVal[c1], cornerVal[c2],
                                    config.isoLevel
                                );
                            }
                        }

                        for (int t = 0; t < 16 && kTriTable[cubeIndex][t] != -1; t += 3) {
                            if (t + 2 >= 16 || kTriTable[cubeIndex][t + 1] == -1 || kTriTable[cubeIndex][t + 2] == -1) break;
                            for (int j = 0; j < 3; ++j) {
                                int edge = kTriTable[cubeIndex][t + j];
                                if (edge < 0 || edge >= 12) continue;
                                SculptureVec3 pos = edgeVertices[edge];

                                IsosurfaceVertex v{};
                                v.position = pos;
                                v.density = config.isoLevel;

                                // Both stencil endpoints stay inside [0, N-1].
                                const std::size_t vx = std::clamp(x, normalBackstep, maxNormalCoord);
                                const std::size_t vy = std::clamp(y, normalBackstep, maxNormalCoord);
                                const std::size_t vz = std::clamp(z, normalBackstep, maxNormalCoord);

                                float dx = getVoxel(vx + 1, vy, vz) - getVoxel(vx - normalBackstep, vy, vz);
                                float dy = getVoxel(vx, vy + 1, vz) - getVoxel(vx, vy - normalBackstep, vz);
                                float dz = getVoxel(vx, vy, vz + 1) - getVoxel(vx, vy, vz - normalBackstep);
                                v.normal = normalizeVec({-dx, -dy, -dz});

                                float r = std::sqrt(pos.x * pos.x + pos.y * pos.y + pos.z * pos.z);
                                v.materialCoord = std::clamp(r * 0.5f, 0.0f, 1.0f);

                                std::uint32_t vIndex = static_cast<std::uint32_t>(localMesh.vertices.size());
                                localMesh.vertices.push_back(v);
                                localMesh.indices.push_back(vIndex);
                            }
                        }
                    }
                }
            }
            return localMesh;
        }));
    }
    
    for (auto& f : marchFutures) {
        auto localMesh = f.get();
        std::uint32_t offset = static_cast<std::uint32_t>(mesh.vertices.size());
        mesh.vertices.insert(mesh.vertices.end(), localMesh.vertices.begin(), localMesh.vertices.end());
        for (auto idx : localMesh.indices) {
            mesh.indices.push_back(idx + offset);
        }
    }

    computeMeshBounds(mesh);
    return mesh;
}


void SculptureIsosurface::computeMeshBounds(IsosurfaceMesh& mesh) noexcept {
    if (mesh.vertices.empty()) {
        mesh.aabbMin = {0.0f, 0.0f, 0.0f};
        mesh.aabbMax = {0.0f, 0.0f, 0.0f};
        return;
    }
    SculptureVec3 bMin = mesh.vertices[0].position;
    SculptureVec3 bMax = mesh.vertices[0].position;
    for (const auto& v : mesh.vertices) {
        if (v.position.x < bMin.x) bMin.x = v.position.x;
        if (v.position.y < bMin.y) bMin.y = v.position.y;
        if (v.position.z < bMin.z) bMin.z = v.position.z;
        if (v.position.x > bMax.x) bMax.x = v.position.x;
        if (v.position.y > bMax.y) bMax.y = v.position.y;
        if (v.position.z > bMax.z) bMax.z = v.position.z;
    }
    mesh.aabbMin = bMin;
    mesh.aabbMax = bMax;
}

} // namespace epi_sculpt
