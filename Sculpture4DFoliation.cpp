#include "Sculpture4DFoliation.h"
#include "SculptureDifferentialGeometry.h"
#include "Epi120CellVerticesCpp.h"
#include <cmath>
#include <algorithm>

namespace epi_sculpt {

SculptureVec4 Sculpture4DFoliation::rotateSO4(
    const SculptureVec4& p,
    float thetaL,
    float thetaR,
    float phiL,
    float phiR
) noexcept {
    return diff_geom::rotateSO4Isoclinic(p, thetaL, thetaR, phiL, phiR);
}

SculptureVec3 Sculpture4DFoliation::projectStereographic(
    const SculptureVec4& p4,
    float scale
) noexcept {
    return diff_geom::projectStereographicS3(p4, scale, 1.0001f, 0.001f);
}

SculptureVec3 Sculpture4DFoliation::projectSlice(
    const SculptureVec4& p4,
    float sliceW,
    float thickness
) noexcept {
    const float distToSlice = std::abs(p4.w - sliceW);
    const float alpha = std::clamp(1.0f - distToSlice / thickness, 0.0f, 1.0f);
    return {
        p4.x * alpha,
        p4.y * alpha,
        p4.z * alpha
    };
}

float Sculpture4DFoliation::sample4DManifoldSdf(
    const SculptureVec4& p,
    Manifold4DType type,
    float time
) noexcept {
    switch (type) {
    case Manifold4DType::CliffordTorus: {
        const float r1 = std::sqrt(p.x * p.x + p.y * p.y);
        const float r2 = std::sqrt(p.z * p.z + p.w * p.w);
        const float R1 = 0.70710678f; // 1 / sqrt(2)
        const float R2 = 0.70710678f;
        const float tube = 0.12f + 0.04f * std::sin(time * 2.0f);
        const float d1 = r1 - R1;
        const float d2 = r2 - R2;
        return std::sqrt(d1 * d1 + d2 * d2) - tube;
    }
    case Manifold4DType::Tesseract8Cell: {
        const float ax = std::abs(p.x);
        const float ay = std::abs(p.y);
        const float az = std::abs(p.z);
        const float aw = std::abs(p.w);
        const float maxCoord = std::max({ax, ay, az, aw});
        return maxCoord - 0.75f;
    }
    case Manifold4DType::Hexadecachoron16: {
        const float l1 = std::abs(p.x) + std::abs(p.y) + std::abs(p.z) + std::abs(p.w);
        return l1 * 0.5f - 0.65f;
    }
    case Manifold4DType::Icositetrachoron24: {
        const float ax = std::abs(p.x);
        const float ay = std::abs(p.y);
        const float az = std::abs(p.z);
        const float aw = std::abs(p.w);
        const float maxC = std::max({ax, ay, az, aw});
        const float sumC = (ax + ay + az + aw) * 0.5f;
        return std::max(maxC, sumC) - 0.70f;
    }
    case Manifold4DType::CalabiYauK3: {
        // Fermat quartic: z1^4 + z2^4 = 1 in C^2
        // z1 = x + i y, z2 = z + i w
        const float x2 = p.x * p.x - p.y * p.y;
        const float y2 = 2.0f * p.x * p.y;
        const float z1_4_re = x2 * x2 - y2 * y2;
        const float z1_4_im = 2.0f * x2 * y2;

        const float z2 = p.z * p.z - p.w * p.w;
        const float w2 = 2.0f * p.z * p.w;
        const float z2_4_re = z2 * z2 - w2 * w2;
        const float z2_4_im = 2.0f * z2 * w2;

        const float re = z1_4_re + z2_4_re - 1.0f;
        const float im = z1_4_im + z2_4_im;
        return (std::sqrt(re * re + im * im) - 0.2f) * 0.4f;
    }
    case Manifold4DType::HopfFibrationS3: {
        // Distance to S^3 unit sphere
        const float len = std::sqrt(p.x * p.x + p.y * p.y + p.z * p.z + p.w * p.w);
        return std::abs(len - 1.0f) - 0.08f;
    }
    case Manifold4DType::Hexacosichoron600: {
        // 600-Cell (Hexacosichoron: 120 vertices on S3, H4 Coxeter reflection symmetry)
        float ax = std::abs(p.x);
        float ay = std::abs(p.y);
        float az = std::abs(p.z);
        float aw = std::abs(p.w);
        if (ay > ax) std::swap(ax, ay);
        if (az > ay) std::swap(ay, az);
        if (aw > az) std::swap(az, aw);
        if (ay > ax) std::swap(ax, ay);
        if (az > ay) std::swap(ay, az);
        if (ay > ax) std::swap(ax, ay);

        constexpr float phi = 1.6180339887f;
        constexpr float inv_phi = 0.6180339887f;
        const float d1 = ax - 1.0f;
        const float d2 = (ax + ay + az + aw) * 0.5f - 1.0f;
        const float d3 = (phi * ax + ay + inv_phi * az) * 0.5f - 1.0f;
        const float polyDist = std::max({d1, d2, d3});
        const float shellDist = std::abs(p.length() - 1.0f) - 0.05f;
        return polyDist * (1.0f - 0.35f) + shellDist * 0.35f;
    }
    case Manifold4DType::Hecatonicosachoron120: {
        // 120-Cell (Hecatonicosachoron: 600 vertices, 120 dodecahedral cells, inradius ~0.9510565)
        float ax = std::abs(p.x);
        float ay = std::abs(p.y);
        float az = std::abs(p.z);
        float aw = std::abs(p.w);
        if (ay > ax) std::swap(ax, ay);
        if (az > ay) std::swap(ay, az);
        if (aw > az) std::swap(az, aw);
        if (ay > ax) std::swap(ax, ay);
        if (az > ay) std::swap(ay, az);
        if (ay > ax) std::swap(ax, ay);

        constexpr float phi = 1.6180339887f;
        constexpr float inv_phi = 0.6180339887f;
        const float f1 = ax;
        const float f2 = (ax + ay + az + aw) * 0.5f;
        const float f3 = (phi * ax + ay + inv_phi * az) * 0.5f;
        constexpr float inradius = 0.9510565f;
        const float polyDist = std::max({f1, f2, f3}) - inradius;
        const float shellDist = std::abs(p.length() - 1.0f) - 0.045f;
        return polyDist * (1.0f - 0.40f) + shellDist * 0.40f;
    }
    case Manifold4DType::OctonionicAssociatorS7: {
        // Evaluate non-associative ternary bracket [p, p_ortho, p_time] on S^7
        Octonion8D q1{{p.x, p.y, p.z, p.w, 0.0f, 0.0f, 0.0f, 0.0f}};
        Octonion8D q2{{0.0f, 0.0f, 0.0f, 0.0f, p.w, p.z, p.y, p.x}};
        Octonion8D q3{{std::sin(time), 0.0f, std::cos(time), 0.0f, 0.0f, std::sin(time * 0.5f), 0.0f, std::cos(time * 0.5f)}};
        Octonion8D assoc = Octonion8D::associator(q1, q2, q3);
        return std::abs(assoc.norm() - 0.35f) - 0.06f;
    }
    case Manifold4DType::SeifertHyperbolicSoliton: {
        // Toroidal Seifert fibration S^1 x S^1 x S^1 in R^4
        const float r1 = std::sqrt(p.x * p.x + p.y * p.y);
        const float r2 = std::sqrt(p.z * p.z + p.w * p.w);
        const float th1 = std::atan2(p.y, p.x);
        const float th2 = std::atan2(p.w, p.z);
        const float phase = std::sin(3.0f * th1 + 2.0f * th2 + time * 1.5f);
        const float d1 = r1 - 0.7071f;
        const float d2 = r2 - 0.7071f;
        const float torusDist = std::sqrt(d1 * d1 + d2 * d2);
        return torusDist - (0.10f + 0.04f * phase);
    }
    case Manifold4DType::CalabiYauQuinticThreefold: {
        // Complex quintic algebraic variety z1^5 + z2^5 = 1 in C^2
        const float r1 = std::sqrt(p.x * p.x + p.y * p.y);
        const float th1 = std::atan2(p.y, p.x);
        const float r1_5 = std::pow(std::clamp(r1, 0.0f, 2.5f), 5.0f);
        const float z1_5_re = r1_5 * std::cos(5.0f * th1);
        const float z1_5_im = r1_5 * std::sin(5.0f * th1);

        const float r2 = std::sqrt(p.z * p.z + p.w * p.w);
        const float th2 = std::atan2(p.w, p.z);
        const float r2_5 = std::pow(std::clamp(r2, 0.0f, 2.5f), 5.0f);
        const float z2_5_re = r2_5 * std::cos(5.0f * th2);
        const float z2_5_im = r2_5 * std::sin(5.0f * th2);

        const float psi = 0.25f * std::sin(time * 0.5f);
        const float re = z1_5_re + z2_5_re - 1.0f - psi * (p.x * p.z - p.y * p.w);
        const float im = z1_5_im + z2_5_im - psi * (p.x * p.w + p.y * p.z);
        return (std::sqrt(re * re + im * im) - 0.15f) * 0.25f;
    }
    case Manifold4DType::LieE8CoxeterProjection: {
        // E8 root lattice projected to 4D: 240 root vectors lie on concentric Golden Ratio shells
        const float r = std::sqrt(p.x * p.x + p.y * p.y + p.z * p.z + p.w * p.w);
        constexpr float phi = 1.6180339887f;
        const float dShell1 = std::abs(r - 1.0f);
        const float dShell2 = std::abs(r - phi * 0.6180339887f);
        const float cosHarmonics = std::cos(5.0f * std::atan2(p.y, p.x)) * std::cos(3.0f * std::atan2(p.w, p.z));
        return std::min(dShell1, dShell2) - 0.05f * (1.0f + 0.5f * cosHarmonics);
    }
    case Manifold4DType::DynamicQuaternionJulia: {
        // 4D Dynamic Quaternion Julia Fractal
        SculptureVec4 z = p;
        const SculptureVec4 c{
            0.35f + 0.15f * std::cos(time * 0.3f),
            0.45f * std::sin(time * 0.4f),
            0.20f * std::cos(time * 0.5f),
            0.15f * std::sin(time * 0.6f)
        };
        float r2 = z.lengthSq();
        float dr = 1.0f;
        for (int i = 0; i < 9; ++i) {
            if (r2 > 8.0f) break;
            const SculptureVec4 z2{
                z.x * z.x - z.y * z.y - z.z * z.z - z.w * z.w,
                2.0f * z.x * z.y,
                2.0f * z.x * z.z,
                2.0f * z.x * z.w
            };
            dr = 2.0f * std::sqrt(r2) * dr + 1.0f;
            z.x = z2.x + c.x;
            z.y = z2.y + c.y;
            z.z = z2.z + c.z;
            z.w = z2.w + c.w;
            r2 = z.lengthSq();
        }
        return 0.25f * std::log(std::max(r2, 1e-6f)) * std::sqrt(r2) / std::max(dr, 1e-4f);
    }
    case Manifold4DType::DynamicHyperMandelbulb: {
        // 4D Dynamic Hyper-Mandelbulb (8th Power)
        SculptureVec4 z = p;
        float dr = 1.0f;
        float r = 0.0f;
        const float morphAngle = time * 0.25f;
        const float n = 8.0f + 2.0f * std::sin(morphAngle);

        for (int i = 0; i < 6; ++i) {
            r = z.length();
            if (r > 3.0f) break;

            const float theta = std::acos(std::clamp(z.w / (r + 1e-6f), -1.0f, 1.0f));
            const float r3 = std::sqrt(z.x * z.x + z.y * z.y + z.z * z.z);
            const float phi = std::acos(std::clamp(z.z / (r3 + 1e-6f), -1.0f, 1.0f));
            const float psi = std::atan2(z.y, z.x);

            dr = std::pow(r, 7.0f) * 8.0f * dr + 1.0f;

            const float r8 = std::pow(r, 8.0f);
            const float tTheta = theta * n;
            const float tPhi = phi * n;
            const float tPsi = psi * n;

            const float sinTheta = std::sin(tTheta);
            const float sinPhi = std::sin(tPhi);

            z.x = r8 * sinTheta * sinPhi * std::cos(tPsi) + p.x;
            z.y = r8 * sinTheta * sinPhi * std::sin(tPsi) + p.y;
            z.z = r8 * sinTheta * std::cos(tPhi) + p.z;
            z.w = r8 * std::cos(tTheta) + p.w;
        }
        return 0.5f * std::log(std::max(r, 1e-6f)) * r / std::max(dr, 1e-4f);
    }
    case Manifold4DType::HyperbolicHoneycomb4D: {
        // 4D hyperbolic {3,5,3} honeycomb horosphere distance
        const float r = std::sqrt(p.x * p.x + p.y * p.y + p.z * p.z + p.w * p.w);
        const float harmonics = 0.08f * (std::cos(3.0f * std::atan2(p.y, p.x)) + std::cos(5.0f * std::atan2(p.w, p.z)));
        return std::abs(r - 1.0f) - harmonics - 0.04f;
    }
    case Manifold4DType::CliffordHopfSolitonLattice: {
        // Distance to nested Hopf torus fibers on S^3
        const float r1 = std::sqrt(p.x * p.x + p.y * p.y);
        const float r2 = std::sqrt(p.z * p.z + p.w * p.w);
        const float s3Dist = std::abs(std::sqrt(r1 * r1 + r2 * r2) - 1.0f);
        const float torusDist = std::abs(r1 - r2);
        return s3Dist + 0.5f * torusDist - 0.03f;
    }
    }
    return p.length() - 1.0f;
}

SculptureVec4 Sculpture4DFoliation::evaluate4DGeodesicVelocity(
    const SculptureVec4& p,
    Manifold4DType type,
    float time
) noexcept {
    switch (type) {
    case Manifold4DType::CliffordTorus:
    case Manifold4DType::HopfFibrationS3: {
        // Hopf Killing vector field on S^3: v = (-y, x, -w, z)
        return {-p.y, p.x, -p.w, p.z};
    }
    case Manifold4DType::SeifertHyperbolicSoliton: {
        // Coupled toroidal velocity field
        const float phase = time * 0.5f;
        return {
            -p.y * 1.5f + 0.2f * std::cos(phase),
             p.x * 1.5f + 0.2f * std::sin(phase),
            -p.w * 2.0f,
             p.z * 2.0f
        };
    }
    case Manifold4DType::OctonionicAssociatorS7:
    case Manifold4DType::LieE8CoxeterProjection: {
        // E8/G2 non-abelian chiral flow
        const float r = std::max(p.length(), 1.0e-4f);
        return {
            (-p.y * p.z + p.w) / r,
            ( p.x * p.z - p.w) / r,
            (-p.x * p.w + p.y) / r,
            ( p.x * p.y - p.z) / r
        };
    }
    case Manifold4DType::DynamicQuaternionJulia: {
        // 4D Quaternion dynamical orbit gradient flow
        const float r = std::max(p.length(), 1.0e-4f);
        return {
            (-p.y * 1.5f + 0.35f * p.w) / r,
            ( p.x * 1.5f - 0.35f * p.z) / r,
            ( p.w * 1.2f + 0.35f * p.y) / r,
            (-p.z * 1.2f - 0.35f * p.x) / r
        };
    }
    case Manifold4DType::DynamicHyperMandelbulb: {
        // 4D Hyperspherical vortical flow
        const float r = std::max(p.length(), 1.0e-4f);
        const float phase = time * 0.3f;
        return {
            (-p.y + 0.3f * std::cos(phase) * p.w) / r,
            ( p.x + 0.3f * std::sin(phase) * p.z) / r,
            (-p.w + 0.3f * std::sin(phase) * p.x) / r,
            ( p.z - 0.3f * std::cos(phase) * p.y) / r
        };
    }
    case Manifold4DType::HyperbolicHoneycomb4D: {
        // Hyperbolic geodesic vector field
        const float r = std::max(p.length(), 1.0e-4f);
        return {
            (-p.y * 1.2f + 0.3f * p.w) / r,
            ( p.x * 1.2f + 0.3f * p.z) / r,
            (-p.w * 1.2f - 0.3f * p.y) / r,
            ( p.z * 1.2f - 0.3f * p.x) / r
        };
    }
    case Manifold4DType::CliffordHopfSolitonLattice: {
        // Multi-instanton twisted Hopf Killing vector field
        const float phase = time * 0.4f;
        return {
            -p.y + 0.2f * std::cos(phase) * p.z,
             p.x + 0.2f * std::sin(phase) * p.w,
            -p.w - 0.2f * std::sin(phase) * p.x,
             p.z - 0.2f * std::cos(phase) * p.y
        };
    }
    default: {
        // Standard SO(4) left isoclinic infinitesimal rotation
        return {-p.y, p.x, -p.w, p.z};
    }
    }
}

Octonion8D Octonion8D::multiply(const Octonion8D& a, const Octonion8D& b) noexcept {
    // Fano Plane Octonionic Multiplication Table (Cayley-Dickson construction):
    // Basis elements e0 (real) and e1..e7 (imaginary units)
    Octonion8D res{};
    const auto& x = a.e;
    const auto& y = b.e;

    // Real part: e0*e0 - sum(ei*ei)
    res.e[0] = x[0]*y[0] - x[1]*y[1] - x[2]*y[2] - x[3]*y[3] - x[4]*y[4] - x[5]*y[5] - x[6]*y[6] - x[7]*y[7];

    // e1: (1,2,3), (1,4,5), (1,7,6)
    res.e[1] = x[0]*y[1] + x[1]*y[0] + x[2]*y[3] - x[3]*y[2] + x[4]*y[5] - x[5]*y[4] - x[6]*y[7] + x[7]*y[6];

    // e2: (1,2,3), (2,4,6), (2,5,7)
    res.e[2] = x[0]*y[2] - x[1]*y[3] + x[2]*y[0] + x[3]*y[1] + x[4]*y[6] + x[5]*y[7] - x[6]*y[4] - x[7]*y[5];

    // e3: (1,2,3), (3,4,7), (3,6,5)
    res.e[3] = x[0]*y[3] + x[1]*y[2] - x[2]*y[1] + x[3]*y[0] + x[4]*y[7] - x[5]*y[6] + x[6]*y[5] - x[7]*y[4];

    // e4: (1,4,5), (2,4,6), (3,4,7)
    res.e[4] = x[0]*y[4] - x[1]*y[5] - x[2]*y[6] - x[3]*y[7] + x[4]*y[0] + x[5]*y[1] + x[6]*y[2] + x[7]*y[3];

    // e5: (1,4,5), (2,5,7), (3,6,5)
    res.e[5] = x[0]*y[5] + x[1]*y[4] - x[2]*y[7] + x[3]*y[6] - x[4]*y[1] + x[5]*y[0] - x[6]*y[3] + x[7]*y[2];

    // e6: (1,7,6), (2,4,6), (3,6,5)
    res.e[6] = x[0]*y[6] + x[1]*y[7] + x[2]*y[4] - x[3]*y[5] - x[4]*y[2] + x[5]*y[3] + x[6]*y[0] - x[7]*y[1];

    // e7: (1,7,6), (2,5,7), (3,4,7)
    res.e[7] = x[0]*y[7] - x[1]*y[6] + x[2]*y[5] + x[3]*y[4] - x[4]*y[3] - x[5]*y[2] + x[6]*y[1] + x[7]*y[0];

    return res;
}

Octonion8D Octonion8D::associator(const Octonion8D& x, const Octonion8D& y, const Octonion8D& z) noexcept {
    // Ternary non-associative bracket: [x, y, z] = (xy)z - x(yz)
    const Octonion8D xy_z = multiply(multiply(x, y), z);
    const Octonion8D x_yz = multiply(x, multiply(y, z));
    Octonion8D res{};
    for (std::size_t i = 0; i < 8; ++i) {
        res.e[i] = xy_z.e[i] - x_yz.e[i];
    }
    return res;
}

std::vector<SculptureVec4> Sculpture4DFoliation::getPolytopeVertices(
    Manifold4DType type
) {
    std::vector<SculptureVec4> verts;
    switch (type) {
    case Manifold4DType::Tesseract8Cell: {
        verts.reserve(16);
        for (int i = 0; i < 16; ++i) {
            verts.push_back({
                (i & 1) ? 0.7f : -0.7f,
                (i & 2) ? 0.7f : -0.7f,
                (i & 4) ? 0.7f : -0.7f,
                (i & 8) ? 0.7f : -0.7f
            });
        }
        break;
    }
    case Manifold4DType::Hexadecachoron16: {
        verts.reserve(8);
        const float s = 0.95f;
        verts.push_back({ s, 0, 0, 0});
        verts.push_back({-s, 0, 0, 0});
        verts.push_back({0,  s, 0, 0});
        verts.push_back({0, -s, 0, 0});
        verts.push_back({0, 0,  s, 0});
        verts.push_back({0, 0, -s, 0});
        verts.push_back({0, 0, 0,  s});
        verts.push_back({0, 0, 0, -s});
        break;
    }
    case Manifold4DType::Icositetrachoron24: {
        verts.reserve(24);
        const float s = 0.85f;
        // 8 permutations of (+-1, 0, 0, 0)
        verts.push_back({ s, 0, 0, 0}); verts.push_back({-s, 0, 0, 0});
        verts.push_back({0,  s, 0, 0}); verts.push_back({0, -s, 0, 0});
        verts.push_back({0, 0,  s, 0}); verts.push_back({0, 0, -s, 0});
        verts.push_back({0, 0, 0,  s}); verts.push_back({0, 0, 0, -s});
        // 16 vertices of hypercube (+-0.5, +-0.5, +-0.5, +-0.5)
        for (int i = 0; i < 16; ++i) {
            verts.push_back({
                (i & 1) ? 0.5f : -0.5f,
                (i & 2) ? 0.5f : -0.5f,
                (i & 4) ? 0.5f : -0.5f,
                (i & 8) ? 0.5f : -0.5f
            });
        }
        break;
    }
    case Manifold4DType::Hexacosichoron600: {
        verts.reserve(120);
        // 1. 8 permutations of (+-1, 0, 0, 0)
        const float s = 1.0f;
        verts.push_back({ s, 0, 0, 0}); verts.push_back({-s, 0, 0, 0});
        verts.push_back({0,  s, 0, 0}); verts.push_back({0, -s, 0, 0});
        verts.push_back({0, 0,  s, 0}); verts.push_back({0, 0, -s, 0});
        verts.push_back({0, 0, 0,  s}); verts.push_back({0, 0, 0, -s});

        // 2. 16 vertices of hypercube (+-0.5, +-0.5, +-0.5, +-0.5)
        for (int i = 0; i < 16; ++i) {
            verts.push_back({
                (i & 1) ? 0.5f : -0.5f,
                (i & 2) ? 0.5f : -0.5f,
                (i & 4) ? 0.5f : -0.5f,
                (i & 8) ? 0.5f : -0.5f
            });
        }

        // 3. 96 even permutations of (+-phi/2, +-1/2, +-1/(2*phi), 0)
        const float phi = 1.6180339887f;
        const float a = phi * 0.5f;           // ~0.809017f
        const float b = 0.5f;
        const float c = 1.0f / (2.0f * phi);  // ~0.309017f

        const int evenPerms[12][4] = {
            {0, 1, 2, 3}, {0, 2, 3, 1}, {0, 3, 1, 2},
            {1, 0, 3, 2}, {1, 2, 0, 3}, {1, 3, 2, 0},
            {2, 0, 1, 3}, {2, 1, 3, 0}, {2, 3, 0, 1},
            {3, 0, 2, 1}, {3, 1, 0, 2}, {3, 2, 1, 0}
        };

        const float coords[4] = {a, b, c, 0.0f};

        for (const auto& perm : evenPerms) {
            for (int s0 = -1; s0 <= 1; s0 += 2) {
                for (int s1 = -1; s1 <= 1; s1 += 2) {
                    for (int s2 = -1; s2 <= 1; s2 += 2) {
                        float v[4] = {
                            static_cast<float>(s0) * coords[0],
                            static_cast<float>(s1) * coords[1],
                            static_cast<float>(s2) * coords[2],
                            0.0f
                        };
                        verts.push_back({v[perm[0]], v[perm[1]], v[perm[2]], v[perm[3]]});
                    }
                }
            }
        }
        break;
    }
    case Manifold4DType::Hecatonicosachoron120: {
        verts.assign(kHecatonicosachoron120Vertices.begin(), kHecatonicosachoron120Vertices.end());
        break;
    }
    case Manifold4DType::SeifertHyperbolicSoliton: {
        verts.reserve(64);
        for (int i = 0; i < 64; ++i) {
            const float u = 6.2831853f * (static_cast<float>(i) / 64.0f);
            const float r1 = 0.7071f + 0.15f * std::cos(3.0f * u);
            const float r2 = 0.7071f + 0.15f * std::sin(2.0f * u);
            verts.push_back({
                r1 * std::cos(u),
                r1 * std::sin(u),
                r2 * std::cos(2.0f * u),
                r2 * std::sin(2.0f * u)
            });
        }
        break;
    }
    case Manifold4DType::CalabiYauQuinticThreefold: {
        verts.reserve(50);
        for (int i = 0; i < 50; ++i) {
            const float u = 6.2831853f * (static_cast<float>(i) / 50.0f);
            const float r = 0.85f * std::cos(5.0f * u * 0.2f);
            verts.push_back({
                r * std::cos(u),
                r * std::sin(u),
                0.6f * std::cos(3.0f * u),
                0.6f * std::sin(3.0f * u)
            });
        }
        break;
    }
    case Manifold4DType::LieE8CoxeterProjection: {
        verts.reserve(240);
        // 1. 24 vertices of 24-cell: (+-1, +-1, 0, 0) permutations normalized
        const float s = 0.70710678f;
        for (int i = 0; i < 4; ++i) {
            for (int j = i + 1; j < 4; ++j) {
                for (int s0 = -1; s0 <= 1; s0 += 2) {
                    for (int s1 = -1; s1 <= 1; s1 += 2) {
                        float v[4] = {0, 0, 0, 0};
                        v[i] = static_cast<float>(s0) * s;
                        v[j] = static_cast<float>(s1) * s;
                        verts.push_back({v[0], v[1], v[2], v[3]});
                    }
                }
            }
        }
        // 2. 16 vertices of hypercube (+-0.5, +-0.5, +-0.5, +-0.5)
        for (int i = 0; i < 16; ++i) {
            verts.push_back({
                (i & 1) ? 0.5f : -0.5f,
                (i & 2) ? 0.5f : -0.5f,
                (i & 4) ? 0.5f : -0.5f,
                (i & 8) ? 0.5f : -0.5f
            });
        }
        // 3. 200 vertices on Golden Ratio Coxeter shells
        const float phi = 1.6180339887f;
        for (int k = 0; k < 200; ++k) {
            const float u = 6.2831853f * (static_cast<float>(k) / 200.0f);
            const float v = 3.14159265f * (static_cast<float>(k % 20) / 20.0f);
            verts.push_back({
                std::cos(u) * std::sin(v) * 0.9f,
                std::sin(u) * std::sin(v) * 0.9f,
                std::cos(v * phi) * 0.9f,
                std::sin(u * phi) * 0.4f
            });
        }
        break;
    }
    case Manifold4DType::HyperbolicHoneycomb4D: {
        constexpr float kPhi = 1.61803398875f;
        constexpr float kNorm = 1.90211303259f;
        const std::vector<SculptureVec3> icoBase = {
            {-1.0f / kNorm,  kPhi / kNorm, 0.0f},
            { 1.0f / kNorm,  kPhi / kNorm, 0.0f},
            {-1.0f / kNorm, -kPhi / kNorm, 0.0f},
            { 1.0f / kNorm, -kPhi / kNorm, 0.0f},
            {0.0f, -1.0f / kNorm,  kPhi / kNorm},
            {0.0f,  1.0f / kNorm,  kPhi / kNorm},
            {0.0f, -1.0f / kNorm, -kPhi / kNorm},
            {0.0f,  1.0f / kNorm, -kPhi / kNorm},
            { kPhi / kNorm, 0.0f, -1.0f / kNorm},
            { kPhi / kNorm, 0.0f,  1.0f / kNorm},
            {-kPhi / kNorm, 0.0f, -1.0f / kNorm},
            {-kPhi / kNorm, 0.0f,  1.0f / kNorm}
        };
        verts.reserve(48);
        for (int c = 0; c < 4; ++c) {
            float wAngle = static_cast<float>(c) * 1.5707963f;
            float cosW = std::cos(wAngle);
            float sinW = std::sin(wAngle);
            for (const auto& v : icoBase) {
                verts.push_back({v.x * cosW, v.y * cosW, v.z * cosW, sinW});
            }
        }
        break;
    }
    case Manifold4DType::CliffordHopfSolitonLattice: {
        verts.reserve(64);
        for (int i = 0; i < 64; ++i) {
            float u = 6.2831853f * (static_cast<float>(i) / 64.0f);
            float v = 2.0f * u;
            verts.push_back({
                0.7071f * std::cos(u),
                0.7071f * std::sin(u),
                0.7071f * std::cos(v),
                0.7071f * std::sin(v)
            });
        }
        break;
    }
    case Manifold4DType::DynamicQuaternionJulia:
    case Manifold4DType::DynamicHyperMandelbulb:
    default: {
        // Discretized Clifford torus circle points
        verts.reserve(32);
        for (int i = 0; i < 32; ++i) {
            float u = 6.2831853f * (static_cast<float>(i) / 32.0f);
            verts.push_back({
                0.707f * std::cos(u),
                0.707f * std::sin(u),
                0.707f * std::cos(u * 2.0f),
                0.707f * std::sin(u * 2.0f)
            });
        }
        break;
    }
    }
    return verts;
}

std::vector<std::pair<std::uint32_t, std::uint32_t>> Sculpture4DFoliation::getPolytopeEdges(
    Manifold4DType type
) {
    std::vector<std::pair<std::uint32_t, std::uint32_t>> edges;
    if (type == Manifold4DType::Tesseract8Cell) {
        edges.reserve(32);
        for (std::uint32_t i = 0; i < 16; ++i) {
            for (std::uint32_t bit = 0; bit < 4; ++bit) {
                std::uint32_t j = i ^ (1U << bit);
                if (i < j) {
                    edges.emplace_back(i, j);
                }
            }
        }
    } else if (type == Manifold4DType::Hexadecachoron16) {
        edges.reserve(24);
        for (std::uint32_t i = 0; i < 8; ++i) {
            for (std::uint32_t j = i + 1; j < 8; ++j) {
                if ((i ^ j) != 1 || (i / 2 != j / 2)) {
                    edges.emplace_back(i, j);
                }
            }
        }
    } else if (type == Manifold4DType::Icositetrachoron24) {
        edges.reserve(96);
        // Connect vertices whose 4D squared distance is close to 1.0
        auto verts = getPolytopeVertices(Manifold4DType::Icositetrachoron24);
        for (std::uint32_t i = 0; i < verts.size(); ++i) {
            for (std::uint32_t j = i + 1; j < verts.size(); ++j) {
                float dx = verts[i].x - verts[j].x;
                float dy = verts[i].y - verts[j].y;
                float dz = verts[i].z - verts[j].z;
                float dw = verts[i].w - verts[j].w;
                float d2 = dx * dx + dy * dy + dz * dz + dw * dw;
                if (d2 > 0.40f && d2 < 0.85f) {
                    edges.emplace_back(i, j);
                }
            }
        }
    } else if (type == Manifold4DType::Hexacosichoron600) {
        edges.reserve(720);
        auto verts = getPolytopeVertices(Manifold4DType::Hexacosichoron600);
        // Distance between neighboring vertices is 1/phi ~ 0.618034 -> squared dist ~ 0.381966
        for (std::uint32_t i = 0; i < verts.size(); ++i) {
            for (std::uint32_t j = i + 1; j < verts.size(); ++j) {
                float dx = verts[i].x - verts[j].x;
                float dy = verts[i].y - verts[j].y;
                float dz = verts[i].z - verts[j].z;
                float dw = verts[i].w - verts[j].w;
                float d2 = dx * dx + dy * dy + dz * dz + dw * dw;
                if (d2 > 0.35f && d2 < 0.42f) {
                    edges.emplace_back(i, j);
                }
            }
        }
    } else if (type == Manifold4DType::Hecatonicosachoron120) {
        edges.reserve(1200);
        auto verts = getPolytopeVertices(Manifold4DType::Hecatonicosachoron120);
        for (std::uint32_t i = 0; i < verts.size(); ++i) {
            for (std::uint32_t j = i + 1; j < verts.size(); ++j) {
                float dx = verts[i].x - verts[j].x;
                float dy = verts[i].y - verts[j].y;
                float dz = verts[i].z - verts[j].z;
                float dw = verts[i].w - verts[j].w;
                float d2 = dx * dx + dy * dy + dz * dz + dw * dw;
                if (d2 > 0.035f && d2 < 0.065f) {
                    edges.emplace_back(i, j);
                }
            }
        }
    } else if (type == Manifold4DType::LieE8CoxeterProjection) {
        edges.reserve(480);
        auto verts = getPolytopeVertices(Manifold4DType::LieE8CoxeterProjection);
        for (std::uint32_t i = 0; i < std::min<std::size_t>(verts.size(), 80U); ++i) {
            for (std::uint32_t j = i + 1; j < std::min<std::size_t>(verts.size(), 80U); ++j) {
                float dx = verts[i].x - verts[j].x;
                float dy = verts[i].y - verts[j].y;
                float dz = verts[i].z - verts[j].z;
                float dw = verts[i].w - verts[j].w;
                float d2 = dx * dx + dy * dy + dz * dz + dw * dw;
                if (d2 > 0.40f && d2 < 0.60f) {
                    edges.emplace_back(i, j);
                }
            }
        }
    } else if (type == Manifold4DType::SeifertHyperbolicSoliton || type == Manifold4DType::CalabiYauQuinticThreefold) {
        auto verts = getPolytopeVertices(type);
        const std::uint32_t count = static_cast<std::uint32_t>(verts.size());
        edges.reserve(count * 2);
        for (std::uint32_t i = 0; i < count; ++i) {
            edges.emplace_back(i, (i + 1) % count);
            if (count > 4) {
                edges.emplace_back(i, (i + count / 4) % count);
            }
        }
    } else if (type == Manifold4DType::HyperbolicHoneycomb4D) {
        auto verts = getPolytopeVertices(Manifold4DType::HyperbolicHoneycomb4D);
        edges.reserve(120);
        for (std::uint32_t i = 0; i < verts.size(); ++i) {
            for (std::uint32_t j = i + 1; j < verts.size(); ++j) {
                float dx = verts[i].x - verts[j].x;
                float dy = verts[i].y - verts[j].y;
                float dz = verts[i].z - verts[j].z;
                float dw = verts[i].w - verts[j].w;
                float d2 = dx * dx + dy * dy + dz * dz + dw * dw;
                if (d2 > 0.35f && d2 < 0.65f) {
                    edges.emplace_back(i, j);
                }
            }
        }
    } else if (type == Manifold4DType::CliffordHopfSolitonLattice) {
        edges.reserve(64);
        for (std::uint32_t i = 0; i < 64; ++i) {
            edges.emplace_back(i, (i + 1) % 64);
        }
    } else {
        // Continuous circular manifold / Clifford torus circle grid
        edges.reserve(64);
        for (std::uint32_t i = 0; i < 32; ++i) {
            edges.emplace_back(i, (i + 1) % 32);
            edges.emplace_back(i, (i + 16) % 32);
        }
    }
    return edges;
}

namespace {

inline SculptureVec3 crossV3(const SculptureVec3& a, const SculptureVec3& b) noexcept {
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

inline float dotV3(const SculptureVec3& a, const SculptureVec3& b) noexcept {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline SculptureVec3 normV3(const SculptureVec3& v) noexcept {
    float len = std::sqrt(dotV3(v, v));
    if (len > 1e-6f) {
        return {v.x / len, v.y / len, v.z / len};
    }
    return {0.0f, 1.0f, 0.0f};
}

} // namespace

IsosurfaceMesh Sculpture4DFoliation::generateGeodesicRibbonMesh(
    const std::vector<SculptureVec4>& curve4D,
    float width,
    float thetaL,
    float thetaR,
    float scale
) {
    if (curve4D.size() < 2) return IsosurfaceMesh{};

    // 1. Rotate 4D points under SO(4) isoclinic action and stereographically project to 3D
    std::vector<SculptureVec3> pts3D;
    pts3D.reserve(curve4D.size());
    for (const auto& p4 : curve4D) {
        SculptureVec4 rot4 = rotateSO4(p4, thetaL, thetaR);
        pts3D.push_back(projectStereographic(rot4, scale));
    }

    // 2. Extrude open flat ribbon along Bishop Rotation-Minimizing Frame (RMF)
    return diff_geom::extrudeFlatRibbonFromPoints(
        pts3D,
        diff_geom::FlatRibbonExtrusionSpec{
            .width = width,
            .closedLoop = false,
            .useBinormalSpan = true
        }
    );
}

IsosurfaceMesh Sculpture4DFoliation::generatePolytopeMesh(
    Manifold4DType type,
    float thickness,
    float thetaL,
    float thetaR,
    float scale
) {
    IsosurfaceMesh mesh{};
    auto verts4D = getPolytopeVertices(type);
    auto edges = getPolytopeEdges(type);
    if (verts4D.empty() || edges.empty()) return mesh;

    // 1. Rotate 4D lattice vertices and stereographically project to 3D
    std::vector<SculptureVec3> pts3D;
    pts3D.reserve(verts4D.size());
    for (const auto& v4 : verts4D) {
        SculptureVec4 rot4 = rotateSO4(v4, thetaL, thetaR);
        pts3D.push_back(projectStereographic(rot4, scale));
    }

    // 2. Generate 4-sided cylindrical strut prisms along each edge
    constexpr int kSegments = 4;
    const float radius = thickness * 0.5f;

    for (const auto& [i0, i1] : edges) {
        if (i0 >= pts3D.size() || i1 >= pts3D.size()) continue;

        const SculptureVec3& p0 = pts3D[i0];
        const SculptureVec3& p1 = pts3D[i1];

        SculptureVec3 dir = normV3({p1.x - p0.x, p1.y - p0.y, p1.z - p0.z});
        SculptureVec3 perp1{0.0f, 1.0f, 0.0f};
        if (std::abs(dotV3(dir, perp1)) > 0.9f) perp1 = {1.0f, 0.0f, 0.0f};
        SculptureVec3 perp2 = normV3(crossV3(dir, perp1));
        perp1 = normV3(crossV3(perp2, dir));

        std::uint32_t baseIdx = static_cast<std::uint32_t>(mesh.vertices.size());

        for (int s = 0; s < kSegments; ++s) {
            float angle = static_cast<float>(s) * (6.2831853f / static_cast<float>(kSegments));
            float ca = std::cos(angle);
            float sa = std::sin(angle);
            SculptureVec3 radialNormal{
                perp1.x * ca + perp2.x * sa,
                perp1.y * ca + perp2.y * sa,
                perp1.z * ca + perp2.z * sa
            };

            // Start cap vertex
            mesh.vertices.push_back(IsosurfaceVertex{
                .position = {
                    p0.x + radialNormal.x * radius,
                    p0.y + radialNormal.y * radius,
                    p0.z + radialNormal.z * radius
                },
                .normal = radialNormal,
                .materialCoord = 0.0f,
                .density = 1.0f
            });

            // End cap vertex
            mesh.vertices.push_back(IsosurfaceVertex{
                .position = {
                    p1.x + radialNormal.x * radius,
                    p1.y + radialNormal.y * radius,
                    p1.z + radialNormal.z * radius
                },
                .normal = radialNormal,
                .materialCoord = 1.0f,
                .density = 1.0f
            });
        }

        for (int s = 0; s < kSegments; ++s) {
            int nextS = (s + 1) % kSegments;
            std::uint32_t v00 = baseIdx + static_cast<std::uint32_t>(s * 2);
            std::uint32_t v01 = baseIdx + static_cast<std::uint32_t>(s * 2 + 1);
            std::uint32_t v10 = baseIdx + static_cast<std::uint32_t>(nextS * 2);
            std::uint32_t v11 = baseIdx + static_cast<std::uint32_t>(nextS * 2 + 1);

            mesh.indices.push_back(v00);
            mesh.indices.push_back(v01);
            mesh.indices.push_back(v10);

            mesh.indices.push_back(v01);
            mesh.indices.push_back(v11);
            mesh.indices.push_back(v10);
        }
    }

    return mesh;
}

SculptureVec4 Sculpture4DFoliation::evaluateAudioReactiveSO4(
    const SculptureVec4& p,
    float baseTime,
    const AudioReactive4DRotors& audio,
    float modulationDepth
) noexcept {
    const float depth = std::clamp(modulationDepth, 0.0f, 2.0f);
    // Harmonic isoclinic speeds driven by sub-bass, mid, and treble energies:
    const float thetaL = baseTime * 0.35f + depth * (audio.bassEnergy * 1.5f + audio.spectralFlux * 2.0f);
    const float thetaR = baseTime * 0.25f + depth * (audio.midEnergy * 1.2f - audio.highEnergy * 0.8f);
    const float phiL = depth * 0.2f * std::sin(baseTime * 0.5f + audio.highEnergy * 3.14159265f);
    return rotateSO4(p, thetaL, thetaR, phiL, 0.0f);
}

IsosurfaceMesh Sculpture4DFoliation::generateHopfTorusKnotMesh(
    std::uint32_t p,
    std::uint32_t q,
    std::size_t samplePoints,
    float tubeRadius,
    float thetaL,
    float thetaR,
    float scale
) {
    const std::size_t N = std::max<std::size_t>(16U, samplePoints);
    const float radius = std::max(0.001f, tubeRadius);
    constexpr float kTwoPi = 6.28318530718f;
    const float invSqrt2 = 0.70710678118f;

    // 1. Parametrize closed (p, q) torus knot curve on S^3
    std::vector<SculptureVec3> pts3D;
    pts3D.reserve(N);
    for (std::size_t i = 0; i < N; ++i) {
        const float phi = kTwoPi * (static_cast<float>(i) / static_cast<float>(N));
        const float u = static_cast<float>(p) * phi;
        const float v = static_cast<float>(q) * phi;
        SculptureVec4 p4{
            invSqrt2 * std::cos(u),
            invSqrt2 * std::sin(u),
            invSqrt2 * std::cos(v),
            invSqrt2 * std::sin(v)
        };
        SculptureVec4 rot4 = rotateSO4(p4, thetaL, thetaR);
        pts3D.push_back(projectStereographic(rot4, scale));
    }

    // 2. Extrude closed cylindrical tube along Bishop Rotation-Minimizing Frame (RMF)
    return diff_geom::extrudeTubeFromPoints(
        pts3D,
        diff_geom::TubeExtrusionSpec{
            .radius = radius,
            .radialSegments = 6U,
            .closedLoop = true
        }
    );
}

IsosurfaceMesh Sculpture4DFoliation::generateVillarceauFoliationMesh(
    std::size_t circleCount,
    std::size_t pointsPerCircle,
    float tubeRadius,
    float thetaL,
    float thetaR,
    float scale
) {
    IsosurfaceMesh composite{};
    const std::size_t count = std::clamp<std::size_t>(circleCount, 1U, 32U);
    constexpr float kTwoPi = 6.28318530718f;

    for (std::size_t c = 0; c < count; ++c) {
        const float angleOffset = static_cast<float>(c) * (kTwoPi / static_cast<float>(count));
        const float tL = thetaL + angleOffset * 0.5f;
        const float tR = thetaR - angleOffset * 0.5f;
        auto circleMesh = generateHopfTorusKnotMesh(1U, 1U, pointsPerCircle, tubeRadius, tL, tR, scale);

        const std::uint32_t indexOffset = static_cast<std::uint32_t>(composite.vertices.size());
        for (auto& v : circleMesh.vertices) {
            v.materialCoord = (v.materialCoord + static_cast<float>(c) / static_cast<float>(count));
            if (v.materialCoord > 1.0f) v.materialCoord -= 1.0f;
            composite.vertices.push_back(v);
            composite.aabbMin.x = std::min(composite.aabbMin.x, v.position.x);
            composite.aabbMin.y = std::min(composite.aabbMin.y, v.position.y);
            composite.aabbMin.z = std::min(composite.aabbMin.z, v.position.z);
            composite.aabbMax.x = std::max(composite.aabbMax.x, v.position.x);
            composite.aabbMax.y = std::max(composite.aabbMax.y, v.position.y);
            composite.aabbMax.z = std::max(composite.aabbMax.z, v.position.z);
        }
        for (auto idx : circleMesh.indices) {
            composite.indices.push_back(idx + indexOffset);
        }
    }
    return composite;
}

IsosurfaceMesh Sculpture4DFoliation::generateCalabiYauCrossSectionMesh(
    std::size_t uSegments,
    std::size_t vSegments,
    float thetaL,
    float thetaR,
    float phaseAngle,
    float scale
) {
    IsosurfaceMesh mesh{};
    const std::size_t uCount = std::clamp<std::size_t>(uSegments, 8U, 128U);
    const std::size_t vCount = std::clamp<std::size_t>(vSegments, 8U, 128U);
    constexpr float kTwoPi = 6.28318530718f;

    mesh.vertices.reserve((uCount + 1) * (vCount + 1));
    mesh.indices.reserve(uCount * vCount * 6);

    for (std::size_t i = 0; i <= uCount; ++i) {
        const float u = static_cast<float>(i) * (kTwoPi / static_cast<float>(uCount));
        const float cu = std::cos(u);
        const float su = std::sin(u);
        const float r1 = std::pow(std::abs(cu) + 1.0e-5f, 0.4f);
        const float r2 = std::pow(std::abs(su) + 1.0e-5f, 0.4f);

        for (std::size_t j = 0; j <= vCount; ++j) {
            const float v = static_cast<float>(j) * (kTwoPi / static_cast<float>(vCount));
            const float phi1 = u;
            const float phi2 = v + phaseAngle;

            const SculptureVec4 p4{
                r1 * std::cos(phi1),
                r1 * std::sin(phi1),
                r2 * std::cos(phi2),
                r2 * std::sin(phi2)
            };

            const SculptureVec4 rotated = rotateSO4(p4, thetaL, thetaR);
            const SculptureVec3 pos = projectStereographic(rotated, scale);

            const float matCoord = (static_cast<float>(i) / static_cast<float>(uCount) * 0.5f)
                                 + (static_cast<float>(j) / static_cast<float>(vCount) * 0.5f);

            const SculptureVec3 normal = normV3(pos);

            mesh.vertices.push_back({
                .position = pos,
                .normal = normal,
                .materialCoord = matCoord,
                .density = 1.0f
            });

            mesh.aabbMin.x = std::min(mesh.aabbMin.x, pos.x);
            mesh.aabbMin.y = std::min(mesh.aabbMin.y, pos.y);
            mesh.aabbMin.z = std::min(mesh.aabbMin.z, pos.z);
            mesh.aabbMax.x = std::max(mesh.aabbMax.x, pos.x);
            mesh.aabbMax.y = std::max(mesh.aabbMax.y, pos.y);
            mesh.aabbMax.z = std::max(mesh.aabbMax.z, pos.z);
        }
    }

    const std::uint32_t stride = static_cast<std::uint32_t>(vCount + 1);
    for (std::size_t i = 0; i < uCount; ++i) {
        for (std::size_t j = 0; j < vCount; ++j) {
            const std::uint32_t v00 = static_cast<std::uint32_t>(i * stride + j);
            const std::uint32_t v01 = static_cast<std::uint32_t>(i * stride + (j + 1));
            const std::uint32_t v10 = static_cast<std::uint32_t>((i + 1) * stride + j);
            const std::uint32_t v11 = static_cast<std::uint32_t>((i + 1) * stride + (j + 1));

            // Front face
            mesh.indices.push_back(v00);
            mesh.indices.push_back(v10);
            mesh.indices.push_back(v01);

            mesh.indices.push_back(v01);
            mesh.indices.push_back(v10);
            mesh.indices.push_back(v11);

            // Back face
            mesh.indices.push_back(v00);
            mesh.indices.push_back(v01);
            mesh.indices.push_back(v10);

            mesh.indices.push_back(v01);
            mesh.indices.push_back(v11);
            mesh.indices.push_back(v10);
        }
    }
    return mesh;
}

IsosurfaceMesh Sculpture4DFoliation::generateLieE8CoxeterProjectionMesh(
    float strutThickness,
    float thetaL,
    float thetaR,
    float scale
) {
    return generatePolytopeMesh(
        Manifold4DType::LieE8CoxeterProjection,
        strutThickness,
        thetaL,
        thetaR,
        scale
    );
}

IsosurfaceMesh Sculpture4DFoliation::generateSeifertSurfaceMesh(
    std::uint32_t p,
    std::uint32_t q,
    std::size_t radialSegments,
    std::size_t angularSegments,
    float thetaL,
    float thetaR,
    float scale
) {
    IsosurfaceMesh mesh{};
    const std::size_t rCount = std::clamp<std::size_t>(radialSegments, 4U, 64U);
    const std::size_t aCount = std::clamp<std::size_t>(angularSegments, 16U, 256U);
    constexpr float kTwoPi = 6.28318530718f;

    mesh.vertices.reserve((rCount + 1) * (aCount + 1));
    mesh.indices.reserve(rCount * aCount * 12); // Front + back faces

    const float pF = static_cast<float>(std::max(1U, p));
    const float qF = static_cast<float>(std::max(1U, q));

    for (std::size_t ri = 0; ri <= rCount; ++ri) {
        const float r = static_cast<float>(ri) / static_cast<float>(rCount);
        // Smooth interior radial weight mapping
        const float w1 = std::sqrt(std::clamp(r, 0.001f, 0.999f));
        const float w2 = std::sqrt(std::clamp(1.0f - r, 0.001f, 0.999f));

        for (std::size_t ai = 0; ai <= aCount; ++ai) {
            const float phi = static_cast<float>(ai) * (kTwoPi / static_cast<float>(aCount));
            const float u = qF * phi;
            const float v = pF * phi;

            // Point on unit 3-sphere S^3
            const SculptureVec4 p4{
                w1 * std::cos(u),
                w1 * std::sin(u),
                w2 * std::cos(v),
                w2 * std::sin(v)
            };

            const SculptureVec4 rotated = rotateSO4(p4, thetaL, thetaR);
            const SculptureVec3 pos = projectStereographic(rotated, scale);

            // Compute adjacent derivative for surface normal
            const float phiNext = phi + 0.01f;
            const SculptureVec4 p4dPhi{
                w1 * std::cos(qF * phiNext),
                w1 * std::sin(qF * phiNext),
                w2 * std::cos(pF * phiNext),
                w2 * std::sin(pF * phiNext)
            };
            const SculptureVec3 posDPhi = projectStereographic(rotateSO4(p4dPhi, thetaL, thetaR), scale);

            const float rNext = std::min(1.0f, r + 0.02f);
            const float w1Next = std::sqrt(rNext);
            const float w2Next = std::sqrt(1.0f - rNext);
            const SculptureVec4 p4dR{
                w1Next * std::cos(u),
                w1Next * std::sin(u),
                w2Next * std::cos(v),
                w2Next * std::sin(v)
            };
            const SculptureVec3 posDR = projectStereographic(rotateSO4(p4dR, thetaL, thetaR), scale);

            const SculptureVec3 dU{posDPhi.x - pos.x, posDPhi.y - pos.y, posDPhi.z - pos.z};
            const SculptureVec3 dV{posDR.x - pos.x, posDR.y - pos.y, posDR.z - pos.z};
            SculptureVec3 normal = normV3(crossV3(dU, dV));

            const float matCoord = r * 0.5f + (static_cast<float>(ai) / static_cast<float>(aCount)) * 0.5f;

            mesh.vertices.push_back({
                .position = pos,
                .normal = normal,
                .materialCoord = matCoord,
                .density = 1.0f
            });

            mesh.aabbMin.x = std::min(mesh.aabbMin.x, pos.x);
            mesh.aabbMin.y = std::min(mesh.aabbMin.y, pos.y);
            mesh.aabbMin.z = std::min(mesh.aabbMin.z, pos.z);
            mesh.aabbMax.x = std::max(mesh.aabbMax.x, pos.x);
            mesh.aabbMax.y = std::max(mesh.aabbMax.y, pos.y);
            mesh.aabbMax.z = std::max(mesh.aabbMax.z, pos.z);
        }
    }

    const std::uint32_t stride = static_cast<std::uint32_t>(aCount + 1);
    for (std::size_t ri = 0; ri < rCount; ++ri) {
        for (std::size_t ai = 0; ai < aCount; ++ai) {
            const std::uint32_t v00 = static_cast<std::uint32_t>(ri * stride + ai);
            const std::uint32_t v01 = static_cast<std::uint32_t>(ri * stride + (ai + 1));
            const std::uint32_t v10 = static_cast<std::uint32_t>((ri + 1) * stride + ai);
            const std::uint32_t v11 = static_cast<std::uint32_t>((ri + 1) * stride + (ai + 1));

            // Front face
            mesh.indices.push_back(v00);
            mesh.indices.push_back(v10);
            mesh.indices.push_back(v01);

            mesh.indices.push_back(v01);
            mesh.indices.push_back(v10);
            mesh.indices.push_back(v11);

            // Back face
            mesh.indices.push_back(v00);
            mesh.indices.push_back(v01);
            mesh.indices.push_back(v10);

            mesh.indices.push_back(v01);
            mesh.indices.push_back(v11);
            mesh.indices.push_back(v10);
        }
    }
    return mesh;
}

IsosurfaceMesh Sculpture4DFoliation::generateOctonionicG2FibrationMesh(
    std::size_t fiberCount,
    std::size_t pointsPerFiber,
    float ribbonWidth,
    float thetaL,
    float thetaR,
    float time,
    float scale
) {
    IsosurfaceMesh composite{};
    const std::size_t count = std::clamp<std::size_t>(fiberCount, 1U, 16U);
    const std::size_t pts = std::clamp<std::size_t>(pointsPerFiber, 16U, 128U);
    constexpr float kTwoPi = 6.28318530718f;

    const Octonion8D q3{{
        std::sin(time * 0.4f),
        0.0f,
        std::cos(time * 0.4f),
        0.0f,
        0.0f,
        std::sin(time * 0.2f),
        0.0f,
        std::cos(time * 0.2f)
    }};

    for (std::size_t f = 0; f < count; ++f) {
        const float phaseOffset = static_cast<float>(f) * (kTwoPi / static_cast<float>(count));

        std::vector<SculptureVec4> fiberCurve4D;
        fiberCurve4D.reserve(pts);

        for (std::size_t i = 0; i < pts; ++i) {
            const float phi = static_cast<float>(i) * (kTwoPi / static_cast<float>(pts));

            Octonion8D q1{{
                std::cos(phi),
                std::sin(phi),
                std::cos(2.0f * phi + phaseOffset),
                std::sin(2.0f * phi + phaseOffset),
                0.0f,
                0.0f,
                0.0f,
                0.0f
            }};

            Octonion8D q2{{
                0.0f,
                0.0f,
                0.0f,
                0.0f,
                std::sin(phi + phaseOffset),
                std::cos(phi + phaseOffset),
                std::sin(3.0f * phi),
                std::cos(3.0f * phi)
            }};

            Octonion8D assoc = Octonion8D::associator(q1, q2, q3);
            const float norm = std::max(assoc.norm(), 1.0e-5f);

            fiberCurve4D.push_back({
                assoc.e[0] / norm,
                assoc.e[1] / norm,
                assoc.e[2] / norm,
                assoc.e[3] / norm
            });
        }

        auto ribbon = generateGeodesicRibbonMesh(fiberCurve4D, ribbonWidth, thetaL, thetaR, scale);

        const std::uint32_t indexOffset = static_cast<std::uint32_t>(composite.vertices.size());
        for (auto& v : ribbon.vertices) {
            v.materialCoord = v.materialCoord * 0.5f + static_cast<float>(f) / static_cast<float>(count) * 0.5f;
            composite.vertices.push_back(v);
            composite.aabbMin.x = std::min(composite.aabbMin.x, v.position.x);
            composite.aabbMin.y = std::min(composite.aabbMin.y, v.position.y);
            composite.aabbMin.z = std::min(composite.aabbMin.z, v.position.z);
            composite.aabbMax.x = std::max(composite.aabbMax.x, v.position.x);
            composite.aabbMax.y = std::max(composite.aabbMax.y, v.position.y);
            composite.aabbMax.z = std::max(composite.aabbMax.z, v.position.z);
        }
        for (auto idx : ribbon.indices) {
            composite.indices.push_back(idx + indexOffset);
        }
    }
    return composite;
}

IsosurfaceMesh Sculpture4DFoliation::generateHyperbolicHoneycombMesh(
    std::size_t cellCount,
    float strutThickness,
    float thetaL,
    float thetaR,
    float scale
) {
    IsosurfaceMesh composite{};
    const std::size_t cells = std::clamp<std::size_t>(cellCount, 1U, 12U);
    const float thickness = std::clamp(strutThickness, 0.01f, 0.20f);
    constexpr float kPhi = 1.61803398875f; // Golden ratio
    constexpr float kNorm = 1.90211303259f; // sqrt(1 + phi^2)
    constexpr float kTwoPi = 6.28318530718f;

    // Standard 12 vertices of regular icosahedron normalized to unit sphere
    const std::vector<SculptureVec3> icoBase = {
        {-1.0f / kNorm,  kPhi / kNorm, 0.0f},
        { 1.0f / kNorm,  kPhi / kNorm, 0.0f},
        {-1.0f / kNorm, -kPhi / kNorm, 0.0f},
        { 1.0f / kNorm, -kPhi / kNorm, 0.0f},
        {0.0f, -1.0f / kNorm,  kPhi / kNorm},
        {0.0f,  1.0f / kNorm,  kPhi / kNorm},
        {0.0f, -1.0f / kNorm, -kPhi / kNorm},
        {0.0f,  1.0f / kNorm, -kPhi / kNorm},
        { kPhi / kNorm, 0.0f, -1.0f / kNorm},
        { kPhi / kNorm, 0.0f,  1.0f / kNorm},
        {-kPhi / kNorm, 0.0f, -1.0f / kNorm},
        {-kPhi / kNorm, 0.0f,  1.0f / kNorm}
    };

    // 30 edges of regular icosahedron
    const std::vector<std::pair<std::uint32_t, std::uint32_t>> icoEdges = {
        {0,1}, {0,5}, {0,7}, {0,10}, {0,11},
        {1,5}, {1,7}, {1,8}, {1,9},
        {2,3}, {2,4}, {2,6}, {2,10}, {2,11},
        {3,4}, {3,6}, {3,8}, {3,9},
        {4,5}, {4,9}, {4,11},
        {5,9}, {5,11},
        {6,7}, {6,8}, {6,10},
        {7,8}, {7,10},
        {8,9}, {10,11}
    };

    for (std::size_t c = 0; c < cells; ++c) {
        const float cellW = static_cast<float>(c) * (kTwoPi / static_cast<float>(cells));
        const float cosW = std::cos(cellW);
        const float sinW = std::sin(cellW);

        std::vector<SculptureVec4> cellVerts4D;
        cellVerts4D.reserve(icoBase.size());
        for (const auto& v : icoBase) {
            cellVerts4D.push_back({
                v.x * cosW,
                v.y * cosW,
                v.z * cosW,
                sinW
            });
        }

        for (const auto& [i0, i1] : icoEdges) {
            std::vector<SculptureVec4> strutCurve4D = {
                cellVerts4D[i0],
                cellVerts4D[i1]
            };
            auto strut = generateGeodesicRibbonMesh(strutCurve4D, thickness, thetaL, thetaR, scale);

            const std::uint32_t indexOffset = static_cast<std::uint32_t>(composite.vertices.size());
            for (auto& v : strut.vertices) {
                v.materialCoord = static_cast<float>(c) / static_cast<float>(cells);
                composite.vertices.push_back(v);
                composite.aabbMin.x = std::min(composite.aabbMin.x, v.position.x);
                composite.aabbMin.y = std::min(composite.aabbMin.y, v.position.y);
                composite.aabbMin.z = std::min(composite.aabbMin.z, v.position.z);
                composite.aabbMax.x = std::max(composite.aabbMax.x, v.position.x);
                composite.aabbMax.y = std::max(composite.aabbMax.y, v.position.y);
                composite.aabbMax.z = std::max(composite.aabbMax.z, v.position.z);
            }
            for (auto idx : strut.indices) {
                composite.indices.push_back(idx + indexOffset);
            }
        }
    }
    return composite;
}

IsosurfaceMesh Sculpture4DFoliation::generateCliffordHopfSolitonLatticeMesh(
    std::size_t ringCount,
    std::size_t pointsPerRing,
    float tubeRadius,
    float thetaL,
    float thetaR,
    float scale
) {
    IsosurfaceMesh composite{};
    const std::size_t rings = std::clamp<std::size_t>(ringCount, 2U, 24U);
    const std::size_t pts = std::clamp<std::size_t>(pointsPerRing, 16U, 128U);
    const float radius = std::clamp(tubeRadius, 0.01f, 0.15f);
    constexpr float kTwoPi = 6.28318530718f;

    for (std::size_t r = 0; r < rings; ++r) {
        const float alpha = static_cast<float>(r + 1) * (1.57079632679f / static_cast<float>(rings + 1));
        const float cosA = std::cos(alpha);
        const float sinA = std::sin(alpha);
        const float phaseTwist = static_cast<float>(r) * (kTwoPi / static_cast<float>(rings));

        std::vector<SculptureVec4> ringCurve4D;
        ringCurve4D.reserve(pts);

        for (std::size_t i = 0; i < pts; ++i) {
            const float u = static_cast<float>(i) * (kTwoPi / static_cast<float>(pts));
            const float v = 2.0f * u + phaseTwist;

            ringCurve4D.push_back({
                cosA * std::cos(u),
                cosA * std::sin(u),
                sinA * std::cos(v),
                sinA * std::sin(v)
            });
        }

        auto tube = generateGeodesicRibbonMesh(ringCurve4D, radius, thetaL, thetaR, scale);

        const std::uint32_t indexOffset = static_cast<std::uint32_t>(composite.vertices.size());
        for (auto& vert : tube.vertices) {
            vert.materialCoord = static_cast<float>(r) / static_cast<float>(rings);
            composite.vertices.push_back(vert);
            composite.aabbMin.x = std::min(composite.aabbMin.x, vert.position.x);
            composite.aabbMin.y = std::min(composite.aabbMin.y, vert.position.y);
            composite.aabbMin.z = std::min(composite.aabbMin.z, vert.position.z);
            composite.aabbMax.x = std::max(composite.aabbMax.x, vert.position.x);
            composite.aabbMax.y = std::max(composite.aabbMax.y, vert.position.y);
            composite.aabbMax.z = std::max(composite.aabbMax.z, vert.position.z);
        }
        for (auto idx : tube.indices) {
            composite.indices.push_back(idx + indexOffset);
        }
    }
    return composite;
}

SculptureVec4 Sculpture4DFoliation::lift3DtoS3(
    const SculptureVec3& p3
) noexcept {
    const float rSq = p3.x * p3.x + p3.y * p3.y + p3.z * p3.z;
    const float denom = 1.0f + rSq;
    const float invDenom = (denom > 1.0e-7f) ? (1.0f / denom) : 1.0f;
    return {
        2.0f * p3.x * invDenom,
        2.0f * p3.y * invDenom,
        2.0f * p3.z * invDenom,
        (rSq - 1.0f) * invDenom
    };
}

SculptureVec3 Sculpture4DFoliation::evaluateS3InstantonFlow(
    const SculptureVec3& p3,
    float thetaL,
    float thetaR,
    float phiL,
    float phiR,
    float scale
) noexcept {
    // 1. Lift 3D spatial coordinate into unit 3-sphere S^3 subset R^4
    const SculptureVec4 p4 = lift3DtoS3(p3);

    // 2. Rigid SO(4) isoclinic double-rotation in R^4
    const SculptureVec4 pRot = rotateSO4(p4, thetaL, thetaR, phiL, phiR);

    // 3. Conformal stereographic projection back down to R^3
    return projectStereographic(pRot, scale);
}

IsosurfaceMesh Sculpture4DFoliation::generateTwistorFibrationRibbonMesh(
    std::size_t fiberCount,
    std::size_t pointsPerFiber,
    float ribbonWidth,
    float thetaL,
    float thetaR,
    float time,
    float scale
) {
    constexpr float kPi = 3.14159265358979323846f;
    constexpr float kTwoPi = 2.0f * kPi;
    const std::size_t fibers = std::clamp(fiberCount, std::size_t{3}, std::size_t{64});
    const std::size_t pts = std::clamp(pointsPerFiber, std::size_t{16}, std::size_t{128});
    const float width = std::max(0.002f, ribbonWidth);

    IsosurfaceMesh composite{};
    composite.aabbMin = {1e9f, 1e9f, 1e9f};
    composite.aabbMax = {-1e9f, -1e9f, -1e9f};

    // Golden ratio spiral distribution of base fibers over S^2 base manifold
    const float kGoldenAngle = kPi * (3.0f - std::sqrt(5.0f));

    for (std::size_t f = 0; f < fibers; ++f) {
        const float tNorm = static_cast<float>(f) / static_cast<float>(fibers);
        // Base point on S^2 base of Hopf-Twistor fibration:
        const float baseZ = 1.0f - 2.0f * (static_cast<float>(f) + 0.5f) / static_cast<float>(fibers);
        const float baseRadius = std::sqrt(std::max(0.0f, 1.0f - baseZ * baseZ));
        const float basePhi = static_cast<float>(f) * kGoldenAngle + time * 0.15f;
        const float baseX = baseRadius * std::cos(basePhi);
        const float baseY = baseRadius * std::sin(basePhi);

        // Convert S^2 base coordinate to Hopf spinor angles (eta, xi1, xi2)
        const float eta = 0.5f * std::acos(std::clamp(baseZ, -1.0f, 1.0f));
        const float xi1 = std::atan2(baseY, baseX);
        const float cosEta = std::cos(eta);
        const float sinEta = std::sin(eta);

        std::vector<SculptureVec4> fiberCurve4D;
        fiberCurve4D.reserve(pts);

        for (std::size_t i = 0; i <= pts; ++i) {
            // Null curve parameter psi traversing the S^1 fiber
            const float psi = static_cast<float>(i) * (kTwoPi / static_cast<float>(pts));
            const float angle1 = psi + xi1;
            const float angle2 = psi;

            // Coordinates on unit 3-sphere S^3:
            const SculptureVec4 ptS3{
                cosEta * std::cos(angle1),
                cosEta * std::sin(angle1),
                sinEta * std::cos(angle2),
                sinEta * std::sin(angle2)
            };
            fiberCurve4D.push_back(ptS3);
        }

        auto ribbon = generateGeodesicRibbonMesh(fiberCurve4D, width, thetaL, thetaR, scale);

        const std::uint32_t indexOffset = static_cast<std::uint32_t>(composite.vertices.size());
        for (auto& vert : ribbon.vertices) {
            vert.materialCoord = tNorm;
            composite.vertices.push_back(vert);
            composite.aabbMin.x = std::min(composite.aabbMin.x, vert.position.x);
            composite.aabbMin.y = std::min(composite.aabbMin.y, vert.position.y);
            composite.aabbMin.z = std::min(composite.aabbMin.z, vert.position.z);
            composite.aabbMax.x = std::max(composite.aabbMax.x, vert.position.x);
            composite.aabbMax.y = std::max(composite.aabbMax.y, vert.position.y);
            composite.aabbMax.z = std::max(composite.aabbMax.z, vert.position.z);
        }
        for (auto idx : ribbon.indices) {
            composite.indices.push_back(idx + indexOffset);
        }
    }
    return composite;
}

std::vector<float> Sculpture4DFoliation::evaluateKuramotoFoliationWaveField(
    const std::vector<SculptureVec3>& samplePoints,
    float time,
    float couplingK,
    float naturalFreq
) {
    const std::size_t N = samplePoints.size();
    std::vector<float> wOffsets(N, 0.0f);
    if (N == 0) return wOffsets;

    // Multi-oscillator spatial wave modes
    const float k1 = 1.8f;
    const float k2 = 2.5f;
    const float omega = naturalFreq;

    for (std::size_t i = 0; i < N; ++i) {
        const auto& p = samplePoints[i];
        const float spatialPhase1 = k1 * (p.x * 0.707f + p.y * 0.707f) - omega * time;
        const float spatialPhase2 = k2 * (p.z * 0.866f - p.x * 0.5f) + omega * time * 0.618f;

        // Kuramoto non-linear phase-locking term
        const float theta_i = spatialPhase1 + 0.35f * std::sin(spatialPhase2);
        const float orderR = std::clamp(0.6f + 0.3f * std::cos(time * 0.5f), 0.1f, 1.0f);
        const float syncW = orderR * std::sin(theta_i) * (1.0f + 0.2f * couplingK);

        wOffsets[i] = syncW * 0.35f;
    }
    return wOffsets;
}

} // namespace epi_sculpt

