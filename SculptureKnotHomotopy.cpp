#include "SculptureKnotHomotopy.h"
#include "SculptureDifferentialGeometry.h"

#include <cmath>
#include <algorithm>
#include <numbers>

namespace epi_sculpt {

namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kTwoPi = 2.0 * kPi;
constexpr double kInvSqrt2 = 0.7071067811865475244;

struct Vec3d {
    double x = 0.0, y = 0.0, z = 0.0;
};

inline Vec3d add(const Vec3d& a, const Vec3d& b) noexcept {
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}

inline Vec3d sub(const Vec3d& a, const Vec3d& b) noexcept {
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}

inline Vec3d mul(const Vec3d& a, double s) noexcept {
    return {a.x * s, a.y * s, a.z * s};
}

inline double dot(const Vec3d& a, const Vec3d& b) noexcept {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline Vec3d cross(const Vec3d& a, const Vec3d& b) noexcept {
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

inline double length(const Vec3d& v) noexcept {
    return std::sqrt(dot(v, v));
}

inline Vec3d normalize(const Vec3d& v) noexcept {
    const double len = length(v);
    if (len <= 1e-9) return {0.0, 0.0, 1.0};
    return mul(v, 1.0 / len);
}

inline SculptureVec3 hsvToRgb(float h, float s, float v) noexcept {
    h = std::fmod(h, 1.0f);
    if (h < 0.0f) h += 1.0f;
    const float c = v * s;
    const float x = c * (1.0f - std::abs(std::fmod(h * 6.0f, 2.0f) - 1.0f));
    const float m = v - c;

    float r = 0.0f, g = 0.0f, b = 0.0f;
    if (h < 1.0f / 6.0f) {
        r = c; g = x; b = 0.0f;
    } else if (h < 2.0f / 6.0f) {
        r = x; g = c; b = 0.0f;
    } else if (h < 3.0f / 6.0f) {
        r = 0.0f; g = c; b = x;
    } else if (h < 4.0f / 6.0f) {
        r = 0.0f; g = x; b = c;
    } else if (h < 5.0f / 6.0f) {
        r = x; g = 0.0f; b = c;
    } else {
        r = c; g = 0.0f; b = x;
    }
    return {r + m, g + m, b + m};
}

} // namespace

Quaternion SculptureKnotHomotopy::evaluateKnot4D(
    float t, float p, float q, float phi) noexcept {
    const double pt = static_cast<double>(p) * static_cast<double>(t);
    const double qt = static_cast<double>(q) * static_cast<double>(t) + static_cast<double>(phi);
    
    return Quaternion(
        static_cast<float>(kInvSqrt2 * std::cos(pt)),
        static_cast<float>(kInvSqrt2 * std::sin(pt)),
        static_cast<float>(kInvSqrt2 * std::cos(qt)),
        static_cast<float>(kInvSqrt2 * std::sin(qt))
    ).normalized();
}

Quaternion SculptureKnotHomotopy::advectKnotPoint4D(
    const Quaternion& q,
    Manifold4DType manifold,
    float time,
    float flowMagnitude
) noexcept {
    const SculptureVec4 p4{q.x, q.y, q.z, q.w};
    const SculptureVec4 vel = Sculpture4DFoliation::evaluate4DGeodesicVelocity(p4, manifold, time);

    Quaternion advected(
        q.w + flowMagnitude * vel.w,
        q.x + flowMagnitude * vel.x,
        q.y + flowMagnitude * vel.y,
        q.z + flowMagnitude * vel.z
    );
    return advected.normalized();
}

SculptureVec3 SculptureKnotHomotopy::stereographicProject(
    const Quaternion& q, float scale) noexcept {
    float denom = 1.0f - q.w;
    if (std::abs(denom) < 1e-4f) {
        denom = (denom >= 0.0f ? 1.0f : -1.0f) * 1e-4f;
    }
    const float factor = scale / denom;
    return {q.x * factor, q.y * factor, q.z * factor};
}

Quaternion SculptureKnotHomotopy::inverseStereographicProject(
    const SculptureVec3& p, float scale) noexcept {
    if (std::abs(scale) <= 1e-7f) {
        return Quaternion(1.0f, 0.0f, 0.0f, 0.0f);
    }
    const SculptureVec4 s3 = diff_geom::inverseStereographicS3(p, std::abs(scale));
    if (scale < 0.0f) {
        return Quaternion(s3.w, -s3.x, -s3.y, -s3.z).normalized();
    }
    return Quaternion(s3.w, s3.x, s3.y, s3.z).normalized();
}

KnotFramePoint SculptureKnotHomotopy::evaluateKnotFrame(
    float t,
    const KnotHomotopyConfig& config,
    float time
) noexcept {
    const double thetaL = static_cast<double>(config.leftIsoclinicSpeed * time);
    const double thetaR = static_cast<double>(config.rightIsoclinicSpeed * time);

    const double cosL = std::cos(thetaL);
    const double sinL = std::sin(thetaL);
    const double cosR = std::cos(thetaR);
    const double sinR = std::sin(thetaR);

    auto evalPosD = [&](double paramT) -> Vec3d {
        const double pt = static_cast<double>(config.p) * paramT;
        const double qt = static_cast<double>(config.q) * paramT + static_cast<double>(config.phaseOffset);

        // Raw 4D knot quaternion in S^3
        const double qw = kInvSqrt2 * std::cos(pt);
        const double qx = kInvSqrt2 * std::sin(pt);
        const double qy = kInvSqrt2 * std::cos(qt);
        const double qz = kInvSqrt2 * std::sin(qt);

        // Left isoclinic multiply: qL * qBase where qL = (cosL, 0, sinL, 0)
        const double l_w = cosL * qw - sinL * qy;
        const double l_x = cosL * qx - sinL * qz;
        const double l_y = cosL * qy + sinL * qw;
        const double l_z = cosL * qz + sinL * qx;

        // Right isoclinic multiply: q_L * qR where qR = (cosR, 0, 0, sinR)
        const double r_w = l_w * cosR - l_z * sinR;
        const double r_x = l_x * cosR + l_y * sinR;
        const double r_y = l_y * cosR - l_x * sinR;
        const double r_z = l_z * cosR + l_w * sinR;

        // Normalization
        const double nsq = r_w * r_w + r_x * r_x + r_y * r_y + r_z * r_z;
        const double invN = 1.0 / std::max(std::sqrt(nsq), 1e-9);

        double w_norm = r_w * invN;
        double x_norm = r_x * invN;
        double y_norm = r_y * invN;
        double z_norm = r_z * invN;

        // Apply 4D Geodesic Flow Field Advection if active
        if (std::abs(config.geodesicFlowSpeed) > 1e-6f) {
            const SculptureVec4 p4{
                static_cast<float>(x_norm),
                static_cast<float>(y_norm),
                static_cast<float>(z_norm),
                static_cast<float>(w_norm)
            };
            const SculptureVec4 vel = Sculpture4DFoliation::evaluate4DGeodesicVelocity(
                p4, config.foliationManifold, time);

            w_norm += static_cast<double>(config.geodesicFlowSpeed) * static_cast<double>(vel.w);
            x_norm += static_cast<double>(config.geodesicFlowSpeed) * static_cast<double>(vel.x);
            y_norm += static_cast<double>(config.geodesicFlowSpeed) * static_cast<double>(vel.y);
            z_norm += static_cast<double>(config.geodesicFlowSpeed) * static_cast<double>(vel.z);

            const double advNsq = w_norm * w_norm + x_norm * x_norm + y_norm * y_norm + z_norm * z_norm;
            const double invAdvN = 1.0 / std::max(std::sqrt(advNsq), 1e-9);
            w_norm *= invAdvN;
            x_norm *= invAdvN;
            y_norm *= invAdvN;
            z_norm *= invAdvN;
        }

        // Stereographic projection: sigma(q) = scale / (1 - w) * (x, y, z)
        double denom = 1.0 - w_norm;
        if (std::abs(denom) < 1e-5) {
            denom = (denom >= 0.0 ? 1.0 : -1.0) * 1e-5;
        }
        const double factor = static_cast<double>(config.stereographicScale) / denom;
        return Vec3d{x_norm * factor, y_norm * factor, z_norm * factor};
    };

    constexpr double dt = 1e-4;
    constexpr double inv2Dt = 1.0 / (2.0 * dt);
    constexpr double invDt2 = 1.0 / (dt * dt);
    constexpr double inv2Dt3 = 1.0 / (2.0 * dt * dt * dt);

    const double tD = static_cast<double>(t);
    const Vec3d pMid  = evalPosD(tD);
    const Vec3d pFwd1 = evalPosD(tD + dt);
    const Vec3d pBwd1 = evalPosD(tD - dt);
    const Vec3d pFwd2 = evalPosD(tD + 2.0 * dt);
    const Vec3d pBwd2 = evalPosD(tD - 2.0 * dt);

    // First derivative r'(t)
    const Vec3d dr = mul(sub(pFwd1, pBwd1), inv2Dt);
    const double speed = length(dr);
    const Vec3d tangent = (speed > 1e-6) ? mul(dr, 1.0 / speed) : Vec3d{1.0, 0.0, 0.0};

    // Second derivative r''(t)
    const Vec3d d2r = mul(add(sub(pFwd1, mul(pMid, 2.0)), pBwd1), invDt2);

    // Third derivative r'''(t)
    const Vec3d d3r = mul(
        sub(add(pFwd2, mul(pBwd1, 2.0)), add(mul(pFwd1, 2.0), pBwd2)),
        inv2Dt3
    );

    // Curvature vector & Binormal B = (r' x r'') / ||r' x r''||
    const Vec3d drCrossD2r = cross(dr, d2r);
    const double crossNorm = length(drCrossD2r);
    const double speed3 = std::max(speed * speed * speed, 1e-9);
    const double curvature = crossNorm / speed3;

    Vec3d binormal;
    if (crossNorm > 1e-6) {
        binormal = mul(drCrossD2r, 1.0 / crossNorm);
    } else {
        const Vec3d arbitraryUp = (std::abs(tangent.y) < 0.9)
            ? Vec3d{0.0, 1.0, 0.0}
            : Vec3d{1.0, 0.0, 0.0};
        binormal = normalize(cross(tangent, arbitraryUp));
    }

    // Normal vector N = B x T
    const Vec3d normal = normalize(cross(binormal, tangent));

    // Ensure Gram-Schmidt orthonormalization
    const Vec3d binormalOrtho = normalize(cross(tangent, normal));

    // Torsion tau = (r' x r'' . r''') / ||r' x r''||^2
    const double crossNorm2 = std::max(crossNorm * crossNorm, 1e-9);
    const double torsion = dot(drCrossD2r, d3r) / crossNorm2;

    return KnotFramePoint{
        .position = {static_cast<float>(pMid.x), static_cast<float>(pMid.y), static_cast<float>(pMid.z)},
        .tangent = {static_cast<float>(tangent.x), static_cast<float>(tangent.y), static_cast<float>(tangent.z)},
        .normal = {static_cast<float>(normal.x), static_cast<float>(normal.y), static_cast<float>(normal.z)},
        .binormal = {static_cast<float>(binormalOrtho.x), static_cast<float>(binormalOrtho.y), static_cast<float>(binormalOrtho.z)},
        .curvature = static_cast<float>(curvature),
        .torsion = static_cast<float>(torsion),
        .arcLength = 0.0f,
        .parameterT = t
    };
}

std::vector<KnotFramePoint> SculptureKnotHomotopy::generateTrajectory(
    const KnotHomotopyConfig& config,
    float time
) {
    const std::size_t N = std::max(config.sampleCount, 16UL);
    std::vector<KnotFramePoint> frames;
    frames.reserve(N);

    float cumulativeArcLength = 0.0f;
    SculptureVec3 prevPos{};

    for (std::size_t i = 0; i < N; ++i) {
        const float t = (static_cast<float>(i) / static_cast<float>(N)) * static_cast<float>(kTwoPi);
        auto frame = evaluateKnotFrame(t, config, time);

        if (i > 0) {
            const float dx = frame.position.x - prevPos.x;
            const float dy = frame.position.y - prevPos.y;
            const float dz = frame.position.z - prevPos.z;
            cumulativeArcLength += std::sqrt(dx * dx + dy * dy + dz * dz);
        }
        frame.arcLength = cumulativeArcLength;
        prevPos = frame.position;
        frames.push_back(frame);
    }

    return frames;
}

float SculptureKnotHomotopy::computeGaussLinkingNumber(
    const std::vector<KnotFramePoint>& curveA,
    const std::vector<KnotFramePoint>& curveB
) noexcept {
    if (curveA.size() < 4 || curveB.size() < 4) return 0.0f;

    double doubleIntegral = 0.0;
    const std::size_t NA = curveA.size();
    const std::size_t NB = curveB.size();

    for (std::size_t i = 0; i < NA; ++i) {
        const std::size_t nextI = (i + 1) % NA;
        const Vec3d drA{
            curveA[nextI].position.x - curveA[i].position.x,
            curveA[nextI].position.y - curveA[i].position.y,
            curveA[nextI].position.z - curveA[i].position.z
        };
        const Vec3d rA{
            0.5 * (curveA[i].position.x + curveA[nextI].position.x),
            0.5 * (curveA[i].position.y + curveA[nextI].position.y),
            0.5 * (curveA[i].position.z + curveA[nextI].position.z)
        };

        for (std::size_t j = 0; j < NB; ++j) {
            const std::size_t nextJ = (j + 1) % NB;
            const Vec3d drB{
                curveB[nextJ].position.x - curveB[j].position.x,
                curveB[nextJ].position.y - curveB[j].position.y,
                curveB[nextJ].position.z - curveB[j].position.z
            };
            const Vec3d rB{
                0.5 * (curveB[j].position.x + curveB[nextJ].position.x),
                0.5 * (curveB[j].position.y + curveB[nextJ].position.y),
                0.5 * (curveB[j].position.z + curveB[nextJ].position.z)
            };

            const Vec3d r12 = sub(rA, rB);
            const double dist = length(r12);
            if (dist < 1e-4) continue;

            const double dist3 = dist * dist * dist;
            const Vec3d drCross = cross(drA, drB);
            doubleIntegral += dot(r12, drCross) / dist3;
        }
    }

    return static_cast<float>(doubleIntegral / (4.0 * kPi));
}

std::vector<KnotFramePoint> SculptureKnotHomotopy::generateHomotopyMorph(
    const KnotHomotopyConfig& startConfig,
    const KnotHomotopyConfig& targetConfig,
    float morphProgress,
    float time
) {
    const float u = std::clamp(morphProgress, 0.0f, 1.0f);
    KnotHomotopyConfig blended = startConfig;
    blended.p = (1.0f - u) * startConfig.p + u * targetConfig.p;
    blended.q = (1.0f - u) * startConfig.q + u * targetConfig.q;
    blended.phaseOffset = (1.0f - u) * startConfig.phaseOffset + u * targetConfig.phaseOffset;
    blended.stereographicScale = (1.0f - u) * startConfig.stereographicScale + u * targetConfig.stereographicScale;
    blended.tubeRadius = (1.0f - u) * startConfig.tubeRadius + u * targetConfig.tubeRadius;
    blended.foliationManifold = (u < 0.5f) ? startConfig.foliationManifold : targetConfig.foliationManifold;
    blended.geodesicFlowSpeed = (1.0f - u) * startConfig.geodesicFlowSpeed + u * targetConfig.geodesicFlowSpeed;

    return generateTrajectory(blended, time);
}

SculptureSnapshot SculptureKnotHomotopy::generateSnapshot(
    const KnotHomotopyConfig& config,
    float time,
    float morph
) {
    SculptureSnapshot snapshot{};
    snapshot.time = time;
    snapshot.morph = morph;

    const auto trajectory = generateTrajectory(config, time);
    if (trajectory.empty()) return snapshot;

    const float totalLen = std::max(trajectory.back().arcLength, 1.0f);
    const std::size_t maxCount = std::min(trajectory.size(), kSampleCount);

    for (std::size_t i = 0; i < maxCount; ++i) {
        const auto& f = trajectory[i];
        const float phase = f.arcLength / totalLen;
        const float hue = std::fmod(phase + morph * 0.5f, 1.0f);
        const float sat = std::clamp(0.7f + 0.3f * std::sin(f.curvature), 0.0f, 1.0f);
        const float val = std::clamp(0.85f + 0.15f * std::cos(f.torsion), 0.0f, 1.0f);

        const auto rgb = hsvToRgb(hue, sat, val);

        auto& s = snapshot.samples[i];
        s.x = f.position.x;
        s.y = f.position.y;
        s.z = f.position.z;
        s.radius = config.tubeRadius * (1.0f + 0.25f * std::sin(f.parameterT * config.q));
        s.r = rgb.x;
        s.g = rgb.y;
        s.b = rgb.z;
        s.a = 1.0f;
        s.nx = f.normal.x;
        s.ny = f.normal.y;
        s.nz = f.normal.z;
        s.nw = f.curvature;
        s.vx = f.tangent.x;
        s.vy = f.tangent.y;
        s.vz = f.tangent.z;
        s.vw = f.torsion;
    }

    return snapshot;
}

IsosurfaceMesh SculptureKnotHomotopy::generateTubeMesh(
    const KnotHomotopyConfig& config,
    float time
) {
    IsosurfaceMesh mesh{};
    const auto frames = generateTrajectory(config, time);
    if (frames.size() < 3) return mesh;

    const std::size_t numRings = frames.size();
    const std::size_t radialSegs = std::max(config.radialSegments, 6UL);
    const float dPhi = static_cast<float>(kTwoPi) / static_cast<float>(radialSegs);

    mesh.vertices.reserve(numRings * radialSegs);

    // Generate vertex rings
    for (std::size_t i = 0; i < numRings; ++i) {
        const auto& f = frames[i];
        for (std::size_t j = 0; j < radialSegs; ++j) {
            const float phi = static_cast<float>(j) * dPhi;
            const float cosP = std::cos(phi);
            const float sinP = std::sin(phi);

            // Radial normal in Frenet plane: n_tube = cos(phi) * N + sin(phi) * B
            const float nx = f.normal.x * cosP + f.binormal.x * sinP;
            const float ny = f.normal.y * cosP + f.binormal.y * sinP;
            const float nz = f.normal.z * cosP + f.binormal.z * sinP;
            const float nLen = std::max(std::sqrt(nx * nx + ny * ny + nz * nz), 1e-6f);

            const SculptureVec3 normalTube{nx / nLen, ny / nLen, nz / nLen};
            const SculptureVec3 pos{
                f.position.x + normalTube.x * config.tubeRadius,
                f.position.y + normalTube.y * config.tubeRadius,
                f.position.z + normalTube.z * config.tubeRadius
            };

            IsosurfaceVertex v{};
            v.position = pos;
            v.normal = normalTube;
            v.materialCoord = static_cast<float>(i) / static_cast<float>(numRings);
            v.density = 1.0f;
            mesh.vertices.push_back(v);
        }
    }

    // Generate triangle quad strips between consecutive rings
    for (std::size_t i = 0; i < numRings; ++i) {
        const std::size_t nextRing = (i + 1) % numRings;
        for (std::size_t j = 0; j < radialSegs; ++j) {
            const std::size_t nextJ = (j + 1) % radialSegs;

            const std::uint32_t i00 = static_cast<std::uint32_t>(i * radialSegs + j);
            const std::uint32_t i01 = static_cast<std::uint32_t>(i * radialSegs + nextJ);
            const std::uint32_t i10 = static_cast<std::uint32_t>(nextRing * radialSegs + j);
            const std::uint32_t i11 = static_cast<std::uint32_t>(nextRing * radialSegs + nextJ);

            // Triangle 1
            mesh.indices.push_back(i00);
            mesh.indices.push_back(i10);
            mesh.indices.push_back(i01);

            // Triangle 2
            mesh.indices.push_back(i01);
            mesh.indices.push_back(i10);
            mesh.indices.push_back(i11);
        }
    }

    return mesh;
}

KnotTopologicalInvariants SculptureKnotHomotopy::computeTopologicalInvariants(
    const std::vector<KnotFramePoint>& trajectory
) noexcept {
    KnotTopologicalInvariants inv{};
    if (trajectory.size() < 2) return inv;

    double totalTwistInt = 0.0;
    double totalCurvatureInt = 0.0;
    double totalLen = 0.0;
    float maxK = 0.0f;
    float maxT = 0.0f;

    const std::size_t n = trajectory.size();
    for (std::size_t i = 0; i < n; ++i) {
        const auto& f0 = trajectory[i];
        const auto& f1 = trajectory[(i + 1) % n];

        const double dx = static_cast<double>(f1.position.x - f0.position.x);
        const double dy = static_cast<double>(f1.position.y - f0.position.y);
        const double dz = static_cast<double>(f1.position.z - f0.position.z);
        const double ds = std::sqrt(dx * dx + dy * dy + dz * dz);

        const double avgK = 0.5 * (static_cast<double>(f0.curvature) + static_cast<double>(f1.curvature));
        const double avgT = 0.5 * (static_cast<double>(f0.torsion) + static_cast<double>(f1.torsion));

        totalCurvatureInt += avgK * ds;
        totalTwistInt += avgT * ds;
        totalLen += ds;

        maxK = std::max(maxK, f0.curvature);
        maxT = std::max(maxT, std::abs(f0.torsion));
    }

    inv.totalTwist = static_cast<float>(totalTwistInt / (2.0 * kPi));
    inv.totalCurvature = static_cast<float>(totalCurvatureInt);
    inv.totalArcLength = static_cast<float>(totalLen);
    inv.maxCurvature = maxK;
    inv.maxTorsion = maxT;
    inv.isFenchelSatisfied = (inv.totalCurvature >= static_cast<float>(2.0 * kPi) - 1e-3f);
    return inv;
}

std::vector<std::vector<KnotFramePoint>> SculptureKnotHomotopy::generateMultiStrandBraid(
    const KnotHomotopyConfig& baseConfig,
    std::size_t numStrands,
    float time,
    float strandTwistSpeed
) {
    std::vector<std::vector<KnotFramePoint>> braid;
    const std::size_t nStrands = std::clamp(numStrands, 1UL, 32UL);
    braid.reserve(nStrands);

    const float dPhi = static_cast<float>(kTwoPi) / static_cast<float>(nStrands);
    for (std::size_t k = 0; k < nStrands; ++k) {
        KnotHomotopyConfig strandCfg = baseConfig;
        strandCfg.phaseOffset = baseConfig.phaseOffset + static_cast<float>(k) * dPhi + strandTwistSpeed * time;
        braid.push_back(generateTrajectory(strandCfg, time));
    }
    return braid;
}

SculptureSnapshot SculptureKnotHomotopy::generateMultiStrandSnapshot(
    const KnotHomotopyConfig& baseConfig,
    std::size_t numStrands,
    float time,
    float morph,
    float strandTwistSpeed
) {
    SculptureSnapshot snapshot{};
    snapshot.time = time;
    snapshot.morph = morph;

    const auto braid = generateMultiStrandBraid(baseConfig, numStrands, time, strandTwistSpeed);
    if (braid.empty()) return snapshot;

    const std::size_t nStrands = braid.size();
    std::size_t writeIdx = 0;

    for (std::size_t k = 0; k < nStrands && writeIdx < kSampleCount; ++k) {
        const auto& strand = braid[k];
        if (strand.empty()) continue;

        const float totalLen = std::max(strand.back().arcLength, 1.0f);
        const std::size_t remainingStrands = (nStrands - k);
        const std::size_t samplesPerStrand = std::min(strand.size(), (kSampleCount - writeIdx) / remainingStrands);

        for (std::size_t i = 0; i < samplesPerStrand && writeIdx < kSampleCount; ++i) {
            const auto& f = strand[i];
            const float phase = f.arcLength / totalLen;
            const float strandBaseHue = static_cast<float>(k) / static_cast<float>(nStrands);
            const float hue = std::fmod(strandBaseHue + phase * 0.35f + morph * 0.5f, 1.0f);
            const float sat = std::clamp(0.75f + 0.25f * std::sin(f.curvature), 0.0f, 1.0f);
            const float val = std::clamp(0.85f + 0.15f * std::cos(f.torsion), 0.0f, 1.0f);

            const auto rgb = hsvToRgb(hue, sat, val);

            auto& s = snapshot.samples[writeIdx++];
            s.x = f.position.x;
            s.y = f.position.y;
            s.z = f.position.z;
            s.radius = baseConfig.tubeRadius * (1.0f + 0.2f * std::sin(f.parameterT * baseConfig.q));
            s.r = rgb.x;
            s.g = rgb.y;
            s.b = rgb.z;
            s.a = 1.0f;
            s.nx = f.normal.x;
            s.ny = f.normal.y;
            s.nz = f.normal.z;
            s.nw = f.curvature;
            s.vx = f.tangent.x;
            s.vy = f.tangent.y;
            s.vz = f.tangent.z;
            s.vw = f.torsion;
        }
    }
    return snapshot;
}

IsosurfaceMesh SculptureKnotHomotopy::generateRibbonMesh(
    const KnotHomotopyConfig& config,
    float ribbonWidth,
    float ribbonTwistRate,
    float time
) {
    IsosurfaceMesh mesh{};
    const auto frames = generateTrajectory(config, time);
    if (frames.size() < 3) return mesh;

    const std::size_t numSlices = frames.size();
    mesh.vertices.reserve(numSlices * 2);

    const float halfWidth = ribbonWidth * 0.5f;

    for (std::size_t i = 0; i < numSlices; ++i) {
        const auto& f = frames[i];
        const float theta = ribbonTwistRate * f.parameterT;
        const float cosT = std::cos(theta);
        const float sinT = std::sin(theta);

        // Vector spanning width of the ribbon in normal-binormal plane
        const SculptureVec3 span{
            f.normal.x * cosT + f.binormal.x * sinT,
            f.normal.y * cosT + f.binormal.y * sinT,
            f.normal.z * cosT + f.binormal.z * sinT
        };

        // Surface normal perpendicular to span and tangent: N_surf = Tangent x Span
        SculptureVec3 surfNorm{
            f.tangent.y * span.z - f.tangent.z * span.y,
            f.tangent.z * span.x - f.tangent.x * span.z,
            f.tangent.x * span.y - f.tangent.y * span.x
        };
        const float normLen = std::max(std::sqrt(surfNorm.x * surfNorm.x + surfNorm.y * surfNorm.y + surfNorm.z * surfNorm.z), 1e-6f);
        surfNorm.x /= normLen;
        surfNorm.y /= normLen;
        surfNorm.z /= normLen;

        const float uCoord = static_cast<float>(i) / static_cast<float>(numSlices);

        // Left vertex
        IsosurfaceVertex vLeft{};
        vLeft.position = {
            f.position.x - span.x * halfWidth,
            f.position.y - span.y * halfWidth,
            f.position.z - span.z * halfWidth
        };
        vLeft.normal = surfNorm;
        vLeft.materialCoord = uCoord;
        vLeft.density = 1.0f;
        mesh.vertices.push_back(vLeft);

        // Right vertex
        IsosurfaceVertex vRight{};
        vRight.position = {
            f.position.x + span.x * halfWidth,
            f.position.y + span.y * halfWidth,
            f.position.z + span.z * halfWidth
        };
        vRight.normal = surfNorm;
        vRight.materialCoord = uCoord;
        vRight.density = 1.0f;
        mesh.vertices.push_back(vRight);
    }

    // Connect ribbon quad strips
    for (std::size_t i = 0; i < numSlices; ++i) {
        const std::size_t nextSlice = (i + 1) % numSlices;
        const std::uint32_t i0 = static_cast<std::uint32_t>(i * 2);
        const std::uint32_t i1 = static_cast<std::uint32_t>(i * 2 + 1);
        const std::uint32_t i2 = static_cast<std::uint32_t>(nextSlice * 2);
        const std::uint32_t i3 = static_cast<std::uint32_t>(nextSlice * 2 + 1);

        // Triangle 1 (Front)
        mesh.indices.push_back(i0);
        mesh.indices.push_back(i2);
        mesh.indices.push_back(i1);

        // Triangle 2 (Front)
        mesh.indices.push_back(i1);
        mesh.indices.push_back(i2);
        mesh.indices.push_back(i3);
    }

    return mesh;
}

std::vector<float> SculptureKnotHomotopy::rasterizeBraidTo2DField(
    const KnotHomotopyConfig& config,
    std::size_t numStrands,
    std::uint32_t width,
    std::uint32_t height,
    float time,
    float splatRadius
) {
    std::vector<float> field(static_cast<std::size_t>(width) * height, 0.0f);
    if (width == 0 || height == 0) return field;

    const auto strands = generateMultiStrandBraid(config, numStrands, time);
    const float radiusSq = std::max(1e-5f, splatRadius * splatRadius);
    const int pixelRadius = std::max(1, static_cast<int>(std::ceil(splatRadius * std::min(width, height) * 0.5f)));

    for (const auto& strand : strands) {
        for (const auto& pt : strand) {
            // Normalize projected 3D position (x, y) from [-scale, scale] to UV [0, 1]
            const float u = (pt.position.x / (config.stereographicScale * 1.5f)) * 0.5f + 0.5f;
            const float v = (pt.position.y / (config.stereographicScale * 1.5f)) * 0.5f + 0.5f;

            if (u < 0.0f || u > 1.0f || v < 0.0f || v > 1.0f) continue;

            const int cx = static_cast<int>(u * (width - 1));
            const int cy = static_cast<int>(v * (height - 1));

            // Gaussian splat with curvature / torsion weighting
            const float weight = 1.0f + std::min(5.0f, pt.curvature * 0.5f);

            for (int dy = -pixelRadius; dy <= pixelRadius; ++dy) {
                const int py = cy + dy;
                if (py < 0 || py >= static_cast<int>(height)) continue;

                for (int dx = -pixelRadius; dx <= pixelRadius; ++dx) {
                    const int px = cx + dx;
                    if (px < 0 || px >= static_cast<int>(width)) continue;

                    const float nx = static_cast<float>(dx) / (width * 0.5f);
                    const float ny = static_cast<float>(dy) / (height * 0.5f);
                    const float distSq = nx * nx + ny * ny;

                    if (distSq <= radiusSq * 4.0f) {
                        const float falloff = std::exp(-distSq / (2.0f * radiusSq));
                        const std::size_t idx = static_cast<std::size_t>(py) * width + px;
                        field[idx] = std::clamp(field[idx] + falloff * weight * 0.25f, 0.0f, 2.0f);
                    }
                }
            }
        }
    }

    return field;
}

} // namespace epi_sculpt
