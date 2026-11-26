#include "SculptureGeometryExporter.h"
#include <fstream>
#include <sstream>
#include <cmath>
#include <numbers>
#include <cstring>
#include <algorithm>

namespace epi_sculpt {

namespace {

SculptureVec3 normalize(const SculptureVec3& v) noexcept {
    const float lenSq = v.x * v.x + v.y * v.y + v.z * v.z;
    if (lenSq < 1e-8f) {
        return {0.0f, 1.0f, 0.0f};
    }
    const float invLen = 1.0f / std::sqrt(lenSq);
    return {v.x * invLen, v.y * invLen, v.z * invLen};
}

SculptureVec3 cross(const SculptureVec3& a, const SculptureVec3& b) noexcept {
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

float dot(const SculptureVec3& a, const SculptureVec3& b) noexcept {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

SculptureVec3 add(const SculptureVec3& a, const SculptureVec3& b) noexcept {
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}

SculptureVec3 sub(const SculptureVec3& a, const SculptureVec3& b) noexcept {
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}

SculptureVec3 scale(const SculptureVec3& a, float s) noexcept {
    return {a.x * s, a.y * s, a.z * s};
}

} // namespace

bool SculptureGeometryExporter::exportWavefrontObj(
    const IsosurfaceMesh& mesh,
    const std::string& filepath,
    const std::string& objectName
) {
    if (mesh.vertices.empty() || mesh.indices.empty()) {
        return false;
    }

    std::ofstream out(filepath);
    if (!out.is_open()) {
        return false;
    }

    out << "# epiC 3D Epimorphic Sculpture Wavefront OBJ Export\n";
    out << "# Vertices: " << mesh.vertices.size() << "\n";
    out << "# Triangles: " << mesh.triangleCount() << "\n";
    out << "o " << objectName << "\n\n";

    // Write vertex positions
    for (const auto& v : mesh.vertices) {
        out << "v " << v.position.x << " " << v.position.y << " " << v.position.z << "\n";
    }

    // Write vertex texture coordinates
    for (const auto& v : mesh.vertices) {
        out << "vt " << v.materialCoord << " 0.0\n";
    }

    // Write vertex normals
    for (const auto& v : mesh.vertices) {
        out << "vn " << v.normal.x << " " << v.normal.y << " " << v.normal.z << "\n";
    }

    out << "\ns 1\n";

    // Write faces (1-indexed in OBJ format)
    const std::size_t numTriangles = mesh.triangleCount();
    for (std::size_t t = 0; t < numTriangles; ++t) {
        std::uint32_t i0 = mesh.indices[t * 3 + 0] + 1;
        std::uint32_t i1 = mesh.indices[t * 3 + 1] + 1;
        std::uint32_t i2 = mesh.indices[t * 3 + 2] + 1;
        out << "f " << i0 << "/" << i0 << "/" << i0 << " "
                   << i1 << "/" << i1 << "/" << i1 << " "
                   << i2 << "/" << i2 << "/" << i2 << "\n";
    }

    return true;
}

bool SculptureGeometryExporter::exportBinaryStl(
    const IsosurfaceMesh& mesh,
    const std::string& filepath
) {
    if (mesh.vertices.empty() || mesh.indices.empty()) {
        return false;
    }

    std::ofstream out(filepath, std::ios::binary);
    if (!out.is_open()) {
        return false;
    }

    // 80-byte header
    char header[80] = {0};
    const char* title = "epiC 3D Epimorphic Sculpture Binary STL Export (Watertight Manifold)";
    std::strncpy(header, title, sizeof(header) - 1);
    out.write(header, 80);

    // 4-byte triangle count
    std::uint32_t numTriangles = static_cast<std::uint32_t>(mesh.triangleCount());
    out.write(reinterpret_cast<const char*>(&numTriangles), sizeof(numTriangles));

    // 50 bytes per triangle
    for (std::size_t t = 0; t < numTriangles; ++t) {
        std::uint32_t i0 = mesh.indices[t * 3 + 0];
        std::uint32_t i1 = mesh.indices[t * 3 + 1];
        std::uint32_t i2 = mesh.indices[t * 3 + 2];

        const auto& v0 = mesh.vertices[i0];
        const auto& v1 = mesh.vertices[i1];
        const auto& v2 = mesh.vertices[i2];

        // Geometric normal
        SculptureVec3 edge1 = sub(v1.position, v0.position);
        SculptureVec3 edge2 = sub(v2.position, v0.position);
        SculptureVec3 facetNormal = normalize(cross(edge1, edge2));

        float triData[12] = {
            facetNormal.x, facetNormal.y, facetNormal.z,
            v0.position.x, v0.position.y, v0.position.z,
            v1.position.x, v1.position.y, v1.position.z,
            v2.position.x, v2.position.y, v2.position.z
        };
        out.write(reinterpret_cast<const char*>(triData), sizeof(triData));

        std::uint16_t attributeByteCount = 0;
        out.write(reinterpret_cast<const char*>(&attributeByteCount), sizeof(attributeByteCount));
    }

    return true;
}

bool SculptureGeometryExporter::exportPointCloudPly(
    const SculptureSnapshot& snapshot,
    const std::string& filepath
) {
    if (snapshot.samples.empty()) {
        return false;
    }

    std::ofstream out(filepath);
    if (!out.is_open()) {
        return false;
    }

    // Count active samples
    std::size_t activeCount = 0;
    for (const auto& s : snapshot.samples) {
        if (s.radius > 0.0f) {
            activeCount++;
        }
    }

    out << "ply\n";
    out << "format ascii 1.0\n";
    out << "comment Generated by epiC 3dEpiSculpTMorph\n";
    out << "element vertex " << activeCount << "\n";
    out << "property float x\n";
    out << "property float y\n";
    out << "property float z\n";
    out << "property uchar red\n";
    out << "property uchar green\n";
    out << "property uchar blue\n";
    out << "property float radius\n";
    out << "end_header\n";

    for (const auto& s : snapshot.samples) {
        if (s.radius <= 0.0f) continue;

        int r = static_cast<int>(std::clamp(s.r * 255.0f, 0.0f, 255.0f));
        int g = static_cast<int>(std::clamp(s.g * 255.0f, 0.0f, 255.0f));
        int b = static_cast<int>(std::clamp(s.b * 255.0f, 0.0f, 255.0f));

        out << s.x << " " << s.y << " " << s.z << " "
            << r << " " << g << " " << b << " "
            << s.radius << "\n";
    }

    return true;
}

IsosurfaceMesh SculptureGeometryExporter::generateStreamlineTubesMesh(
    const std::vector<std::vector<SculptureVec3>>& streamlines,
    const StreamlineTubeConfig& config
) {
    IsosurfaceMesh mesh;
    const std::uint32_t S = std::clamp(config.radialSegments, 3U, 32U);
    const float twoPi = 2.0f * static_cast<float>(std::numbers::pi);

    for (const auto& curve : streamlines) {
        const std::size_t M = curve.size();
        if (M < 2) continue;

        const std::uint32_t baseVertexIndex = static_cast<std::uint32_t>(mesh.vertices.size());

        // Initialize parallel transport reference frame
        SculptureVec3 t0 = normalize(sub(curve[1], curve[0]));
        SculptureVec3 ref = (std::abs(t0.y) < 0.9f) ? SculptureVec3{0.0f, 1.0f, 0.0f} : SculptureVec3{1.0f, 0.0f, 0.0f};
        SculptureVec3 n0 = normalize(cross(t0, ref));
        SculptureVec3 b0 = normalize(cross(t0, n0));

        std::vector<SculptureVec3> normals(M);
        std::vector<SculptureVec3> binormals(M);
        normals[0] = n0;
        binormals[0] = b0;

        // Propagate parallel transport frames along streamline
        for (std::size_t k = 1; k < M; ++k) {
            SculptureVec3 prevT = normalize(sub(curve[k], curve[k - 1]));
            SculptureVec3 curT = (k + 1 < M) ? normalize(sub(curve[k + 1], curve[k])) : prevT;

            SculptureVec3 v = cross(prevT, curT);
            float c = dot(prevT, curT);

            if (std::abs(c + 1.0f) < 1e-5f) {
                // Opposing direction
                normals[k] = scale(normals[k - 1], -1.0f);
            } else {
                // Rotation-minimizing update
                SculptureVec3 prevN = normals[k - 1];
                SculptureVec3 term = cross(v, prevN);
                SculptureVec3 crossCross = cross(v, term);
                float denom = 1.0f + c;
                if (denom > 1e-6f) {
                    normals[k] = normalize(add(prevN, add(term, scale(crossCross, 1.0f / denom))));
                } else {
                    normals[k] = prevN;
                }
            }
            binormals[k] = normalize(cross(curT, normals[k]));
        }

        // Generate tube vertices
        for (std::size_t k = 0; k < M; ++k) {
            const float vCoord = static_cast<float>(k) / static_cast<float>(M - 1);
            const auto& P = curve[k];
            const auto& N = normals[k];
            const auto& B = binormals[k];

            for (std::uint32_t j = 0; j < S; ++j) {
                float angle = twoPi * (static_cast<float>(j) / static_cast<float>(S));
                float cosA = std::cos(angle);
                float sinA = std::sin(angle);

                SculptureVec3 radialDir = add(scale(N, cosA), scale(B, sinA));
                SculptureVec3 pos = add(P, scale(radialDir, config.radius));

                IsosurfaceVertex vert{};
                vert.position = pos;
                vert.normal = radialDir;
                vert.materialCoord = vCoord;
                vert.density = 1.0f;

                mesh.vertices.push_back(vert);
            }
        }

        // Generate cylinder quad strip indices
        for (std::size_t k = 0; k < M - 1; ++k) {
            std::uint32_t row0 = baseVertexIndex + static_cast<std::uint32_t>(k * S);
            std::uint32_t row1 = baseVertexIndex + static_cast<std::uint32_t>((k + 1) * S);

            for (std::uint32_t j = 0; j < S; ++j) {
                std::uint32_t nextJ = (j + 1) % S;

                std::uint32_t i0 = row0 + j;
                std::uint32_t i1 = row0 + nextJ;
                std::uint32_t i2 = row1 + nextJ;
                std::uint32_t i3 = row1 + j;

                // Triangle 1
                mesh.indices.push_back(i0);
                mesh.indices.push_back(i1);
                mesh.indices.push_back(i2);

                // Triangle 2
                mesh.indices.push_back(i0);
                mesh.indices.push_back(i2);
                mesh.indices.push_back(i3);
            }
        }

        // Generate end caps if requested
        if (config.generateCaps) {
            // Start cap
            std::uint32_t startCenterIdx = static_cast<std::uint32_t>(mesh.vertices.size());
            SculptureVec3 startNormal = scale(t0, -1.0f);
            IsosurfaceVertex startCapVert{};
            startCapVert.position = curve[0];
            startCapVert.normal = startNormal;
            startCapVert.materialCoord = 0.0f;
            mesh.vertices.push_back(startCapVert);

            for (std::uint32_t j = 0; j < S; ++j) {
                std::uint32_t nextJ = (j + 1) % S;
                mesh.indices.push_back(startCenterIdx);
                mesh.indices.push_back(baseVertexIndex + nextJ);
                mesh.indices.push_back(baseVertexIndex + j);
            }

            // End cap
            std::uint32_t endCenterIdx = static_cast<std::uint32_t>(mesh.vertices.size());
            SculptureVec3 endT = normalize(sub(curve[M - 1], curve[M - 2]));
            IsosurfaceVertex endCapVert{};
            endCapVert.position = curve[M - 1];
            endCapVert.normal = endT;
            endCapVert.materialCoord = 1.0f;
            mesh.vertices.push_back(endCapVert);

            std::uint32_t lastRow = baseVertexIndex + static_cast<std::uint32_t>((M - 1) * S);
            for (std::uint32_t j = 0; j < S; ++j) {
                std::uint32_t nextJ = (j + 1) % S;
                mesh.indices.push_back(endCenterIdx);
                mesh.indices.push_back(lastRow + j);
                mesh.indices.push_back(lastRow + nextJ);
            }
        }
    }

    SculptureIsosurface::computeMeshBounds(mesh);
    return mesh;
}

std::vector<std::vector<SculptureVec3>> SculptureGeometryExporter::traceAttractorStreamlines(
    const SculptureProgram& program,
    float time,
    float morph,
    std::size_t numStreamlines,
    std::size_t stepsPerStreamline,
    float stepSize
) {
    std::vector<std::vector<SculptureVec3>> streamlines;
    streamlines.reserve(numStreamlines);

    const float twoPi = 2.0f * static_cast<float>(std::numbers::pi);

    for (std::size_t s = 0; s < numStreamlines; ++s) {
        float angle = twoPi * (static_cast<float>(s) / static_cast<float>(numStreamlines));
        float rad = 0.35f + 0.15f * std::sin(angle * 3.0f);

        SculptureVec3 curPos{
            rad * std::cos(angle),
            0.1f * std::sin(angle * 2.0f),
            rad * std::sin(angle)
        };

        std::vector<SculptureVec3> curve;
        curve.reserve(stepsPerStreamline);
        curve.push_back(curPos);

        for (std::size_t step = 0; step < stepsPerStreamline; ++step) {
            // Runge-Kutta 2nd order (midpoint) advection using field gradient
            SculptureVec3 grad1 = program.evaluateFieldGradient(curPos, time, morph, 0.01f);
            // Swirl vector field: curl = grad x (0, 1, 0) + slight attractor contraction
            SculptureVec3 v1{
                grad1.z * 1.5f - curPos.x * 0.1f,
                -grad1.y * 0.5f + 0.05f * std::sin(curPos.x * 4.0f),
                -grad1.x * 1.5f - curPos.z * 0.1f
            };

            SculptureVec3 midPos = add(curPos, scale(v1, 0.5f * stepSize));
            SculptureVec3 grad2 = program.evaluateFieldGradient(midPos, time, morph, 0.01f);
            SculptureVec3 v2{
                grad2.z * 1.5f - midPos.x * 0.1f,
                -grad2.y * 0.5f + 0.05f * std::sin(midPos.x * 4.0f),
                -grad2.x * 1.5f - midPos.z * 0.1f
            };

            curPos = add(curPos, scale(v2, stepSize));
            curve.push_back(curPos);
        }

        streamlines.push_back(std::move(curve));
    }

    return streamlines;
}

} // namespace epi_sculpt
