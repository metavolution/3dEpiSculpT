/**
 * ============================================================================
 * EPI2 / 3dEpiSculpTMorph - SCULPTURE 4D POLYCHORON HOMOTOPY ENGINE
 * ============================================================================
 */

#include "Sculpture4DPolychoronHomotopy.h"
#include "Sculpture4DFoliation.h"
#include "SculptureDifferentialGeometry.h"
#include <cmath>
#include <algorithm>

namespace epi_sculpt {

namespace {

inline constexpr float kGoldenRatio = 1.618033988749895f;      // phi = (1 + sqrt(5)) / 2
inline constexpr float kInvGoldenRatio = 0.618033988749895f;   // 1 / phi = phi - 1

inline SculptureVec4 normalize4(const SculptureVec4& v) noexcept {
    return diff_geom::normalize4(v);
}

inline float dot4(const SculptureVec4& a, const SculptureVec4& b) noexcept {
    return diff_geom::dot4(a, b);
}

} // namespace

Sculpture4DPolychoronHomotopy::Sculpture4DPolychoronHomotopy(const PolychoronHomotopyConfig& config)
    : config_(config) {}

SculptureVec4 Sculpture4DPolychoronHomotopy::slerpS3(
    const SculptureVec4& a,
    const SculptureVec4& b,
    float t
) noexcept {
    return diff_geom::slerpS3(a, b, t);
}

SculptureVec3 Sculpture4DPolychoronHomotopy::projectStereographic(
    const SculptureVec4& p,
    float poleDistance
) noexcept {
    return diff_geom::projectStereographicS3(p, poleDistance, poleDistance, 0.08f);
}

std::vector<PolychoronVertex4D> Sculpture4DPolychoronHomotopy::generateCanonicalVertices(PolychoronKind kind) {
    std::vector<PolychoronVertex4D> vertices;

    switch (kind) {
    case PolychoronKind::Tesseract8Cell: {
        // 16 vertices: (+-0.5, +-0.5, +-0.5, +-0.5) on unit 3-sphere S^3
        vertices.reserve(16U);
        for (int i = 0; i < 16; ++i) {
            const float x = (i & 1) ? 0.5f : -0.5f;
            const float y = (i & 2) ? 0.5f : -0.5f;
            const float z = (i & 4) ? 0.5f : -0.5f;
            const float w = (i & 8) ? 0.5f : -0.5f;
            const auto band = static_cast<std::uint32_t>((i % 3));
            vertices.push_back(PolychoronVertex4D{
                .position = SculptureVec4{x, y, z, w},
                .semanticBand = band,
                .energyPhase = static_cast<float>(i) / 16.0f
            });
        }
        break;
    }

    case PolychoronKind::Hexadecachoron16: {
        // 8 vertices: (+-1, 0, 0, 0) and orthogonal permutations
        vertices.reserve(8U);
        const float coords[4][4] = {
            {1.0f, 0.0f, 0.0f, 0.0f},
            {0.0f, 1.0f, 0.0f, 0.0f},
            {0.0f, 0.0f, 1.0f, 0.0f},
            {0.0f, 0.0f, 0.0f, 1.0f}
        };
        for (int axis = 0; axis < 4; ++axis) {
            for (float sign : {1.0f, -1.0f}) {
                vertices.push_back(PolychoronVertex4D{
                    .position = SculptureVec4{
                        coords[axis][0] * sign,
                        coords[axis][1] * sign,
                        coords[axis][2] * sign,
                        coords[axis][3] * sign
                    },
                    .semanticBand = static_cast<std::uint32_t>(axis % 3),
                    .energyPhase = static_cast<float>(axis) / 4.0f
                });
            }
        }
        break;
    }

    case PolychoronKind::Icositetrachoron24: {
        // 24-Cell: Self-dual regular 4-polytope.
        // 8 vertices permutations of (+-1, 0, 0, 0) + 16 vertices of (+-0.5, +-0.5, +-0.5, +-0.5)
        vertices.reserve(24U);
        // Cross-polytope basis
        for (int axis = 0; axis < 4; ++axis) {
            for (float sign : {1.0f, -1.0f}) {
                SculptureVec4 v{0.0f, 0.0f, 0.0f, 0.0f};
                if (axis == 0) v.x = sign;
                if (axis == 1) v.y = sign;
                if (axis == 2) v.z = sign;
                if (axis == 3) v.w = sign;
                vertices.push_back(PolychoronVertex4D{
                    .position = v,
                    .semanticBand = 0U,
                    .energyPhase = static_cast<float>(axis) / 12.0f
                });
            }
        }
        // Hypercube basis
        for (int i = 0; i < 16; ++i) {
            const float x = (i & 1) ? 0.5f : -0.5f;
            const float y = (i & 2) ? 0.5f : -0.5f;
            const float z = (i & 4) ? 0.5f : -0.5f;
            const float w = (i & 8) ? 0.5f : -0.5f;
            vertices.push_back(PolychoronVertex4D{
                .position = SculptureVec4{x, y, z, w},
                .semanticBand = (i % 2 == 0) ? 1U : 2U,
                .energyPhase = static_cast<float>(8 + i) / 24.0f
            });
        }
        break;
    }

    case PolychoronKind::Hexacosichoron600: {
        // 120 vertices on unit S^3
        vertices.reserve(120U);
        // 8 vertices (+-1, 0, 0, 0)
        for (int axis = 0; axis < 4; ++axis) {
            for (float sign : {1.0f, -1.0f}) {
                SculptureVec4 v{0.0f, 0.0f, 0.0f, 0.0f};
                if (axis == 0) v.x = sign;
                if (axis == 1) v.y = sign;
                if (axis == 2) v.z = sign;
                if (axis == 3) v.w = sign;
                vertices.push_back(PolychoronVertex4D{
                    .position = v,
                    .semanticBand = 0U,
                    .energyPhase = static_cast<float>(vertices.size()) / 120.0f
                });
            }
        }
        // 16 vertices (+-0.5, +-0.5, +-0.5, +-0.5)
        for (int i = 0; i < 16; ++i) {
            const float x = (i & 1) ? 0.5f : -0.5f;
            const float y = (i & 2) ? 0.5f : -0.5f;
            const float z = (i & 4) ? 0.5f : -0.5f;
            const float w = (i & 8) ? 0.5f : -0.5f;
            vertices.push_back(PolychoronVertex4D{
                .position = SculptureVec4{x, y, z, w},
                .semanticBand = 1U,
                .energyPhase = static_cast<float>(vertices.size()) / 120.0f
            });
        }
        // 96 vertices: Even permutations of (0, +-phi/2, +-1/2, +-1/(2*phi))
        const float phiHalf = kGoldenRatio * 0.5f;
        const float half = 0.5f;
        const float invPhiHalf = kInvGoldenRatio * 0.5f;

        // 12 even permutation patterns of (0, 1, 2, 3)
        static constexpr int perms[12][4] = {
            {0, 1, 2, 3}, {0, 2, 3, 1}, {0, 3, 1, 2},
            {1, 0, 3, 2}, {1, 2, 0, 3}, {1, 3, 2, 0},
            {2, 0, 1, 3}, {2, 1, 3, 0}, {2, 3, 0, 1},
            {3, 0, 2, 1}, {3, 1, 0, 2}, {3, 2, 1, 0}
        };

        for (int p = 0; p < 12 && vertices.size() < 120U; ++p) {
            for (float s1 : {1.0f, -1.0f}) {
                for (float s2 : {1.0f, -1.0f}) {
                    for (float s3 : {1.0f, -1.0f}) {
                        if (vertices.size() >= 120U) break;
                        float raw[4] = {0.0f, phiHalf * s1, half * s2, invPhiHalf * s3};
                        SculptureVec4 v{
                            raw[perms[p][0]],
                            raw[perms[p][1]],
                            raw[perms[p][2]],
                            raw[perms[p][3]]
                        };
                        vertices.push_back(PolychoronVertex4D{
                            .position = normalize4(v),
                            .semanticBand = 2U,
                            .energyPhase = static_cast<float>(vertices.size()) / 120.0f
                        });
                    }
                }
            }
        }
        break;
    }

    case PolychoronKind::Hecatonicosachoron120: {
        // 120-Cell representative 120 primary cell centers on S^3
        // Uses the dual vertex configuration of the 600-cell
        auto dual600 = generateCanonicalVertices(PolychoronKind::Hexacosichoron600);
        vertices.reserve(dual600.size());
        for (std::size_t i = 0; i < dual600.size(); ++i) {
            auto v = dual600[i];
            // Rotate dual configuration by pi/4 in dual coordinate planes
            v.position = Sculpture4DFoliation::rotateSO4(v.position, 0.785398f, 0.785398f);
            v.semanticBand = static_cast<std::uint32_t>((i % 3));
            vertices.push_back(v);
        }
        break;
    }

    case PolychoronKind::CliffordTorus:
    default: {
        // 64 sample vertices on the Clifford Flat Torus S^1 x S^1 subset S^3
        vertices.reserve(64U);
        constexpr float kInvSqrt2 = 0.7071067811865475f;
        for (int u = 0; u < 8; ++u) {
            const float phiU = static_cast<float>(u) * (6.283185307f / 8.0f);
            const float cosU = std::cos(phiU) * kInvSqrt2;
            const float sinU = std::sin(phiU) * kInvSqrt2;
            for (int v = 0; v < 8; ++v) {
                const float phiV = static_cast<float>(v) * (6.283185307f / 8.0f);
                const float cosV = std::cos(phiV) * kInvSqrt2;
                const float sinV = std::sin(phiV) * kInvSqrt2;
                vertices.push_back(PolychoronVertex4D{
                    .position = SculptureVec4{cosU, sinU, cosV, sinV},
                    .semanticBand = static_cast<std::uint32_t>((u + v) % 3),
                    .energyPhase = static_cast<float>(u * 8 + v) / 64.0f
                });
            }
        }
        break;
    }
    }

    return vertices;
}

std::vector<ProjectedVertex3D> Sculpture4DPolychoronHomotopy::evaluateHomotopy(float simulatedTime) const {
    const auto vertsA = generateCanonicalVertices(config_.sourcePolytope);
    const auto vertsB = generateCanonicalVertices(config_.targetPolytope);

    const std::size_t count = std::min(std::max(vertsA.size(), vertsB.size()), kMaxHomotopyVertices);
    std::vector<ProjectedVertex3D> projected;
    projected.reserve(count);

    const float t = std::clamp(config_.morphProgress, 0.0f, 1.0f);

    // Continuous dynamic SO(4) rotor angles
    const float alpha = config_.rotorAlpha + simulatedTime * 0.25f;
    const float beta = config_.rotorBeta + simulatedTime * 0.18f;

    for (std::size_t i = 0; i < count; ++i) {
        const auto& vA = vertsA[i % vertsA.size()];
        const auto& vB = vertsB[i % vertsB.size()];

        // 1. Geodesic Slerp homotopy on S^3
        SculptureVec4 p4 = slerpS3(vA.position, vB.position, t);

        // 2. SO(4) double-isoclinic Lie rotation
        p4 = Sculpture4DFoliation::rotateSO4(p4, alpha, beta);

        // 3. Planar Z-W rotor if configured
        if (std::abs(config_.rotorTheta4D) > 1e-4f) {
            const float cosT = std::cos(config_.rotorTheta4D);
            const float sinT = std::sin(config_.rotorTheta4D);
            const float newZ = p4.z * cosT - p4.w * sinT;
            const float newW = p4.z * sinT + p4.w * cosT;
            p4.z = newZ;
            p4.w = newW;
        }

        // 4. Projection to 3D visual space
        ProjectedVertex3D out{};
        out.wCoordinate = p4.w;
        out.semanticBand = (t < 0.5f) ? vA.semanticBand : vB.semanticBand;
        out.hueShift = (1.0f - t) * vA.energyPhase + t * vB.energyPhase;

        if (config_.useStereographic) {
            out.position = projectStereographic(p4, config_.projectionPoleR);
            out.position.x *= config_.scale;
            out.position.y *= config_.scale;
            out.position.z *= config_.scale;

            // Perspective scale & depth-of-field opacity based on 4D distance
            const float depth = std::max(config_.projectionPoleR - p4.w, 0.1f);
            out.uniformScale = (0.18f * config_.scale) / depth;
            out.opacity = std::clamp(1.2f - std::abs(p4.w) * 0.4f, 0.2f, 1.0f);
        } else {
            // Hyperplane slicing: extract points within [latentW - sliceThickness, latentW + sliceThickness]
            const float distToSlice = std::abs(p4.w - config_.latentW);
            if (distToSlice > config_.sliceThickness) {
                out.opacity = 0.0f; // Culled by hyperplane slice
            } else {
                out.opacity = 1.0f - (distToSlice / config_.sliceThickness);
            }
            out.position = SculptureVec3{p4.x * config_.scale, p4.y * config_.scale, p4.z * config_.scale};
            out.uniformScale = 0.15f * config_.scale * (0.5f + 0.5f * out.opacity);
        }

        // 4D anisotropic tidal elongation along coordinate velocity
        out.axisScale = SculptureVec3{
            1.0f + 0.35f * std::abs(p4.x * p4.w),
            1.0f + 0.35f * std::abs(p4.y * p4.w),
            1.0f + 0.35f * std::abs(p4.z * p4.w)
        };

        projected.push_back(out);
    }

    return projected;
}

std::vector<PolychoronEdge> Sculpture4DPolychoronHomotopy::generateCanonicalEdges(PolychoronKind kind) {
    std::vector<PolychoronEdge> edges;

    switch (kind) {
    case PolychoronKind::Tesseract8Cell: {
        // 16 vertices, 32 edges. Two vertices share an edge iff their binary indices differ by exactly 1 bit.
        edges.reserve(32U);
        for (std::uint32_t i = 0; i < 16; ++i) {
            for (std::uint32_t bit = 0; bit < 4; ++bit) {
                const std::uint32_t j = i ^ (1U << bit);
                if (i < j) {
                    edges.push_back(PolychoronEdge{
                        .v0 = i,
                        .v1 = j,
                        .semanticBand = bit % 3U
                    });
                }
            }
        }
        break;
    }

    case PolychoronKind::Hexadecachoron16: {
        // 8 vertices, 24 edges. Every vertex connects to all vertices except itself and its antipodal opposite.
        edges.reserve(24U);
        for (std::uint32_t i = 0; i < 8; ++i) {
            for (std::uint32_t j = i + 1; j < 8; ++j) {
                // In generateCanonicalVertices, pairs (0,1), (2,3), (4,5), (6,7) are antipodal
                if ((i / 2) != (j / 2)) {
                    edges.push_back(PolychoronEdge{
                        .v0 = i,
                        .v1 = j,
                        .semanticBand = (i + j) % 3U
                    });
                }
            }
        }
        break;
    }

    case PolychoronKind::Icositetrachoron24: {
        // 24-Cell: Self-dual 24 vertices, 96 edges.
        // Two vertices form an edge iff their 4D dot product is 0.5 (distance 1.0).
        const auto vertices = generateCanonicalVertices(PolychoronKind::Icositetrachoron24);
        edges.reserve(96U);
        for (std::uint32_t i = 0; i < vertices.size(); ++i) {
            for (std::uint32_t j = i + 1; j < vertices.size(); ++j) {
                const float d = dot4(vertices[i].position, vertices[j].position);
                if (std::abs(d - 0.5f) < 0.05f) {
                    edges.push_back(PolychoronEdge{
                        .v0 = i,
                        .v1 = j,
                        .semanticBand = (vertices[i].semanticBand + vertices[j].semanticBand) / 2U
                    });
                }
            }
        }
        break;
    }

    case PolychoronKind::Hexacosichoron600: {
        // 600-Cell: 120 vertices, 720 edges.
        // Edge length on S^3 has geodesic angular separation 36 deg, dot product = phi / 2 = 0.809017f.
        const auto vertices = generateCanonicalVertices(PolychoronKind::Hexacosichoron600);
        edges.reserve(720U);
        for (std::uint32_t i = 0; i < vertices.size(); ++i) {
            for (std::uint32_t j = i + 1; j < vertices.size(); ++j) {
                const float d = dot4(vertices[i].position, vertices[j].position);
                if (std::abs(d - 0.809017f) < 0.035f) {
                    edges.push_back(PolychoronEdge{
                        .v0 = i,
                        .v1 = j,
                        .semanticBand = vertices[i].semanticBand
                    });
                }
            }
        }
        break;
    }

    case PolychoronKind::Hecatonicosachoron120: {
        // 120-Cell: 120 cell centers, dual edges connected when adjacent.
        const auto vertices = generateCanonicalVertices(PolychoronKind::Hecatonicosachoron120);
        edges.reserve(720U);
        for (std::uint32_t i = 0; i < vertices.size(); ++i) {
            for (std::uint32_t j = i + 1; j < vertices.size(); ++j) {
                const float d = dot4(vertices[i].position, vertices[j].position);
                if (std::abs(d - 0.809017f) < 0.035f) {
                    edges.push_back(PolychoronEdge{
                        .v0 = i,
                        .v1 = j,
                        .semanticBand = vertices[i].semanticBand
                    });
                }
            }
        }
        break;
    }

    case PolychoronKind::CliffordTorus:
    default: {
        // 64 sample nodes on an 8x8 periodic toroidal grid.
        // Edges along u and v directions: 64 * 2 = 128 edges.
        edges.reserve(128U);
        for (std::uint32_t u = 0; u < 8; ++u) {
            for (std::uint32_t v = 0; v < 8; ++v) {
                const std::uint32_t cur = u * 8 + v;
                const std::uint32_t nextU = ((u + 1) % 8) * 8 + v;
                const std::uint32_t nextV = u * 8 + ((v + 1) % 8);

                edges.push_back(PolychoronEdge{
                    .v0 = cur,
                    .v1 = nextU,
                    .semanticBand = (u % 3U)
                });
                edges.push_back(PolychoronEdge{
                    .v0 = cur,
                    .v1 = nextV,
                    .semanticBand = (v % 3U)
                });
            }
        }
        break;
    }
    }

    return edges;
}

std::vector<std::vector<SculptureVec3>> Sculpture4DPolychoronHomotopy::generateHomotopyEdgeCurves(
    float simulatedTime,
    std::size_t samplesPerEdge
) const {
    const auto vertsA = generateCanonicalVertices(config_.sourcePolytope);
    const auto vertsB = generateCanonicalVertices(config_.targetPolytope);
    const std::size_t vertexCount = std::min(std::max(vertsA.size(), vertsB.size()), kMaxHomotopyVertices);

    const float t = std::clamp(config_.morphProgress, 0.0f, 1.0f);

    // Compute S^3 geodesic interpolated vertices
    std::vector<SculptureVec4> blended4D;
    blended4D.reserve(vertexCount);
    for (std::size_t i = 0; i < vertexCount; ++i) {
        const auto& vA = vertsA[i % vertsA.size()];
        const auto& vB = vertsB[i % vertsB.size()];
        blended4D.push_back(slerpS3(vA.position, vB.position, t));
    }

    // Select canonical edge connectivity corresponding to dominant homotopy endpoint
    const auto edges = generateCanonicalEdges(t < 0.5f ? config_.sourcePolytope : config_.targetPolytope);

    // Continuous dynamic SO(4) rotor angles
    const float alpha = config_.rotorAlpha + simulatedTime * 0.25f;
    const float beta = config_.rotorBeta + simulatedTime * 0.18f;

    std::vector<std::vector<SculptureVec3>> edgeCurves;
    edgeCurves.reserve(edges.size());

    const std::size_t steps = std::max<std::size_t>(2U, samplesPerEdge);

    for (const auto& edge : edges) {
        const std::uint32_t idx0 = edge.v0 % vertexCount;
        const std::uint32_t idx1 = edge.v1 % vertexCount;
        if (idx0 == idx1) continue;

        const SculptureVec4& p0 = blended4D[idx0];
        const SculptureVec4& p1 = blended4D[idx1];

        std::vector<SculptureVec3> curve;
        curve.reserve(steps);

        bool edgeValid = true;

        for (std::size_t s = 0; s < steps; ++s) {
            const float lambda = static_cast<float>(s) / static_cast<float>(steps - 1);
            // Great-circle arc on S^3 connecting the two vertices
            SculptureVec4 p4 = slerpS3(p0, p1, lambda);

            // Double-isoclinic SO(4) rotation
            p4 = Sculpture4DFoliation::rotateSO4(p4, alpha, beta);

            // Planar Z-W rotor if configured
            if (std::abs(config_.rotorTheta4D) > 1e-4f) {
                const float cosT = std::cos(config_.rotorTheta4D);
                const float sinT = std::sin(config_.rotorTheta4D);
                const float newZ = p4.z * cosT - p4.w * sinT;
                const float newW = p4.z * sinT + p4.w * cosT;
                p4.z = newZ;
                p4.w = newW;
            }

            SculptureVec3 p3{};
            if (config_.useStereographic) {
                p3 = projectStereographic(p4, config_.projectionPoleR);
                p3.x *= config_.scale;
                p3.y *= config_.scale;
                p3.z *= config_.scale;
            } else {
                const float distToSlice = std::abs(p4.w - config_.latentW);
                if (distToSlice > config_.sliceThickness) {
                    edgeValid = false;
                    break;
                }
                p3 = SculptureVec3{p4.x * config_.scale, p4.y * config_.scale, p4.z * config_.scale};
            }

            if (!std::isfinite(p3.x) || !std::isfinite(p3.y) || !std::isfinite(p3.z)) {
                edgeValid = false;
                break;
            }

            curve.push_back(p3);
        }

        if (edgeValid && curve.size() >= 2) {
            edgeCurves.push_back(std::move(curve));
        }
    }

    return edgeCurves;
}

IsosurfaceMesh Sculpture4DPolychoronHomotopy::generateHomotopyMesh(
    float simulatedTime,
    const StreamlineTubeConfig& tubeConfig
) const {
    const auto edgeCurves = generateHomotopyEdgeCurves(simulatedTime, 4U);

    StreamlineTubeConfig cfg = tubeConfig;
    if (cfg.radius <= 0.0f) {
        cfg.radius = 0.035f * config_.scale;
    }
    cfg.radialSegments = std::clamp(cfg.radialSegments, 3U, 16U);
    cfg.generateCaps = true;
    cfg.smoothNormals = true;

    IsosurfaceMesh mesh = SculptureGeometryExporter::generateStreamlineTubesMesh(edgeCurves, cfg);

    if (!mesh.vertices.empty()) {
        mesh.aabbMin = mesh.vertices[0].position;
        mesh.aabbMax = mesh.vertices[0].position;
        for (const auto& v : mesh.vertices) {
            mesh.aabbMin.x = std::min(mesh.aabbMin.x, v.position.x);
            mesh.aabbMin.y = std::min(mesh.aabbMin.y, v.position.y);
            mesh.aabbMin.z = std::min(mesh.aabbMin.z, v.position.z);
            mesh.aabbMax.x = std::max(mesh.aabbMax.x, v.position.x);
            mesh.aabbMax.y = std::max(mesh.aabbMax.y, v.position.y);
            mesh.aabbMax.z = std::max(mesh.aabbMax.z, v.position.z);
        }
    }

    mesh.isoLevel = evaluateCoxeterOrderParameter(config_.morphProgress);
    return mesh;
}

namespace {

inline std::array<float, 4> quaternionFromZAxis(const SculptureVec3& dir) noexcept {
    const float dz = std::clamp(dir.z, -1.0f, 1.0f);
    if (dz > 0.99999f) {
        return {0.0f, 0.0f, 0.0f, 1.0f}; // Identity
    }
    if (dz < -0.99999f) {
        return {1.0f, 0.0f, 0.0f, 0.0f}; // 180 deg around X
    }
    // Cross product (0, 0, 1) x dir = (-dir.y, dir.x, 0)
    float qx = -dir.y;
    float qy = dir.x;
    float qz = 0.0f;
    float qw = 1.0f + dz;
    const float len = std::sqrt(qx * qx + qy * qy + qz * qz + qw * qw);
    const float invLen = 1.0f / len;
    return {qx * invLen, qy * invLen, qz * invLen, qw * invLen};
}

} // namespace

std::vector<world_gpu_abi::WorldInstanceGpu> Sculpture4DPolychoronHomotopy::generateWorldInstances(
    float simulatedTime
) const {
    const auto projectedVerts = evaluateHomotopy(simulatedTime);
    if (projectedVerts.empty()) return {};

    const float t = std::clamp(config_.morphProgress, 0.0f, 1.0f);
    const float orderParam = evaluateCoxeterOrderParameter(t);

    const auto edges = generateCanonicalEdges(t < 0.5f ? config_.sourcePolytope : config_.targetPolytope);

    std::vector<world_gpu_abi::WorldInstanceGpu> instances;
    instances.reserve(projectedVerts.size() + edges.size());

    // 1. Emit vertex node instances (spheres)
    for (std::size_t i = 0; i < projectedVerts.size(); ++i) {
        const auto& v = projectedVerts[i];
        if (v.opacity <= 0.01f) continue;

        world_gpu_abi::WorldInstanceGpu inst{};
        inst.positionScale = {v.position.x, v.position.y, v.position.z, v.uniformScale};
        inst.axisScale = {v.axisScale.x, v.axisScale.y, v.axisScale.z, 1.0f};

        // Align orientation to position direction or identity
        const float posLen = std::sqrt(v.position.x * v.position.x + v.position.y * v.position.y + v.position.z * v.position.z);
        if (posLen > 1e-4f) {
            inst.orientation = quaternionFromZAxis(SculptureVec3{v.position.x / posLen, v.position.y / posLen, v.position.z / posLen});
        } else {
            inst.orientation = {0.0f, 0.0f, 0.0f, 1.0f};
        }

        // Semantic band color tinting
        if (v.semanticBand == 0U) {
            // Core / Form: Electric Cyan
            inst.tintEmission = {0.2f, 0.75f, 1.0f, 2.0f * v.opacity};
        } else if (v.semanticBand == 1U) {
            // Lattice / Structure: Vivid Magenta
            inst.tintEmission = {1.0f, 0.25f, 0.85f, 1.6f * v.opacity};
        } else {
            // Apex / Accent: Brilliant Gold
            inst.tintEmission = {1.0f, 0.85f, 0.25f, 2.4f * v.opacity};
        }

        // Modulate PBR metallic / roughness based on order parameter
        inst.pbrOverride = {0.90f, 0.15f + 0.35f * (1.0f - v.opacity), 0.0f, 0.0f};
        inst.primitiveId = static_cast<std::uint32_t>(world_gpu_abi::PrimitiveId::Sphere);
        inst.materialBand = v.semanticBand;
        inst.flags = world_gpu_abi::kAnisotropicScaleFlag | (config_.useStereographic ? world_gpu_abi::kUses4DProjectionFlag : 0U);
        inst.sourceIndex = static_cast<std::uint32_t>(i);

        instances.push_back(inst);
    }

    // 2. Emit strut instances along edges (truss struts)
    const float strutRadius = 0.025f * config_.scale;
    for (std::size_t e = 0; e < edges.size(); ++e) {
        const auto& edge = edges[e];
        const std::uint32_t i0 = edge.v0 % projectedVerts.size();
        const std::uint32_t i1 = edge.v1 % projectedVerts.size();
        if (i0 == i1) continue;

        const auto& v0 = projectedVerts[i0];
        const auto& v1 = projectedVerts[i1];

        const float minOpacity = std::min(v0.opacity, v1.opacity);
        if (minOpacity <= 0.05f) continue;

        const float dx = v1.position.x - v0.position.x;
        const float dy = v1.position.y - v0.position.y;
        const float dz = v1.position.z - v0.position.z;
        const float len = std::sqrt(dx * dx + dy * dy + dz * dz);

        if (len < 1e-4f || len > 15.0f) continue;

        const SculptureVec3 dir{dx / len, dy / len, dz / len};
        const SculptureVec3 mid{
            (v0.position.x + v1.position.x) * 0.5f,
            (v0.position.y + v1.position.y) * 0.5f,
            (v0.position.z + v1.position.z) * 0.5f
        };

        world_gpu_abi::WorldInstanceGpu strut{};
        strut.positionScale = {mid.x, mid.y, mid.z, 1.0f};
        strut.axisScale = {strutRadius, strutRadius, len * 0.5f, 1.0f};
        strut.orientation = quaternionFromZAxis(dir);
        strut.tintEmission = {0.8f, 0.9f, 1.0f, 1.2f * minOpacity * (1.0f + 0.5f * orderParam)};
        strut.pbrOverride = {0.95f, 0.12f, 0.0f, 0.0f};
        strut.primitiveId = static_cast<std::uint32_t>(world_gpu_abi::PrimitiveId::TrussStrut);
        strut.materialBand = 1U;
        strut.flags = world_gpu_abi::kAnisotropicScaleFlag | world_gpu_abi::kUses4DProjectionFlag;
        strut.sourceIndex = static_cast<std::uint32_t>(projectedVerts.size() + e);

        instances.push_back(strut);
    }

    return instances;
}

float Sculpture4DPolychoronHomotopy::evaluateCoxeterOrderParameter(float t) const noexcept {
    const float blend = std::clamp(t, 0.0f, 1.0f);
    // Vanishes at pure polytope endpoints (t = 0 or t = 1), peaks at mid-homotopy maximal chiral mixing
    return 4.0f * blend * (1.0f - blend);
}

} // namespace epi_sculpt
