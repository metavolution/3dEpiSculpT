// -----------------------------------------------------------------------------
// Educational Annotation: The Sculpture Math Oracle
// -----------------------------------------------------------------------------
// This modular header encapsulates the entire strict, deterministic CPU 
// reference implementation of the Epimorphic Math engine (The Oracle). 
// It implements the full 6-stage pipeline (Transform, Fold, Orbit, Seed, Color, Blend)
// and guarantees bit-exact structural parity with the OpenCL GPU kernels.
// By extracting this, we guarantee core architectural separation of the math
// from the async rendering and memory management systems.
// -----------------------------------------------------------------------------
#ifndef EPI_SCULPTURE_MATH_ORACLE_H
#define EPI_SCULPTURE_MATH_ORACLE_H

#include "epi/core/complex.hpp"
#include "epi/core/vec.hpp"
#include "epi/seed/sdf.hpp"
#include "QuaternionAlgebra.h"
#include "OctonionAlgebra.h"
#include "SculptureGpuAbi.h"
#include "SculptureParameterRegistry.h"
#include "SculptureFractalCatalog.h"
#include <algorithm>
#include <cmath>
#include <numbers>

namespace epi_sculpt {
namespace oracle {

using epi::Complexf;
using epi::Vec3f;
struct Vec4f { float x, y, z, w; };

constexpr float kPi = std::numbers::pi_v<float>;

[[nodiscard]] inline float smoothstep(float edge0, float edge1, float x) noexcept {
    if (edge0 == edge1) return x < edge0 ? 0.0f : 1.0f;
    const float t = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

[[nodiscard]] inline float fract(float value) noexcept {
    return value - std::floor(value);
}

[[nodiscard]] inline Vec3f oklchToRgb(float L, float C, float h_degrees) {
    float h = h_degrees * kPi / 180.0f;
    float a = C * std::cos(h); float b = C * std::sin(h);
    float l_ = L + 0.3963377774f * a + 0.2158037573f * b;
    float m_ = L - 0.1055613458f * a - 0.0638541728f * b;
    float s_ = L - 0.0894841775f * a - 1.2914855480f * b;
    l_ = l_*l_*l_; m_ = m_*m_*m_; s_ = s_*s_*s_;
    float r =  4.0767416621f * l_ - 3.3077115913f * m_ + 0.2309699292f * s_;
    float g = -1.2684380046f * l_ + 2.6097574011f * m_ - 0.3413193965f * s_;
    float bl = -0.0041960863f * l_ - 0.7034186147f * m_ + 1.7076147010f * s_;
    float mv = std::max({r, g, bl});
    if (mv > 1.0f) { r /= mv; g /= mv; bl /= mv; }
    return {std::clamp(r, 0.0f, 1.0f), std::clamp(g, 0.0f, 1.0f), std::clamp(bl, 0.0f, 1.0f)};
}

[[nodiscard]] inline Vec3f paletteEvaluate(ColorEffectType effect, float materialCoord, float orbitEnergy, float magnitude, float time, float chromaBreath, float lumaDrift, float hueRotation) {
    float h = 0.0f, c = 0.15f, l = 0.70f;
    float u = std::clamp(materialCoord, 0.0f, 1.0f);
    float breath = 1.0f + chromaBreath * std::sin(time * 3.14159f * 1.5f + u * 6.28318f);
    float driftLuma = lumaDrift * 0.15f * std::cos(time * 2.0f + u * 3.14159f);

    switch (effect) {
        case ColorEffectType::DistanceHSV:
            h = std::fmod(u * 360.0f + magnitude * 80.0f + time * 15.0f, 360.0f);
            c = 0.22f;
            l = 0.65f + 0.15f * std::sin(orbitEnergy * 3.14159f);
            break;
        case ColorEffectType::ChannelSwap: {
            float baseH = std::fmod(time * 25.0f + u * 180.0f, 360.0f);
            h = (u > 0.5f) ? std::fmod(baseH + 180.0f, 360.0f) : baseH;
            c = 0.18f;
            l = 0.72f;
            break;
        }
        case ColorEffectType::Glitch:
            h = std::fmod(u * 720.0f + orbitEnergy * 120.0f + time * 45.0f, 360.0f);
            c = 0.25f + 0.1f * std::sin(time * 20.0f);
            l = (std::sin(u * 50.0f + time * 30.0f) > 0.5f) ? 0.90f : 0.40f;
            break;
        case ColorEffectType::ThermalHologram:
            h = std::lerp(15.0f, 290.0f, u);
            c = std::lerp(0.24f, 0.14f, u);
            l = std::lerp(0.35f, 0.92f, u);
            break;
        case ColorEffectType::Vaporwave:
            h = (u < 0.5f) ? std::lerp(190.0f, 320.0f, u * 2.0f) : std::lerp(320.0f, 270.0f, (u - 0.5f) * 2.0f);
            c = 0.20f;
            l = 0.75f;
            break;
        case ColorEffectType::GoldenRatio:
            h = std::fmod(u * 137.507764f * 8.0f + time * 20.0f, 360.0f);
            c = 0.16f;
            l = 0.70f + 0.12f * std::sin(u * 6.28318f);
            break;
        case ColorEffectType::HopfFibration:
            h = std::fmod(u * 360.0f + time * 30.0f, 360.0f);
            c = 0.22f;
            l = 0.68f + 0.18f * std::cos(u * 12.56637f);
            break;
        case ColorEffectType::QuaternionCurvature:
            h = std::fmod(u * 360.0f + orbitEnergy * 90.0f + magnitude * 45.0f + time * 20.0f, 360.0f);
            c = 0.25f;
            l = 0.72f + 0.15f * std::sin(orbitEnergy * 6.28318f);
            break;
        case ColorEffectType::PhenomenologicalPalette: {
            float wave1 = 0.6f * std::sin(6.28318f * (0.18f * u + 0.013f * time));
            float wave2 = 0.4f * std::sin(6.28318f * (0.31f * u + 0.008f * time + 1.7f));
            float hue_x = 0.20f * u * 360.0f + wave1 * 40.0f + wave2 * 30.0f;
            float hue_dyn = orbitEnergy * 60.0f; 
            h = std::fmod(hue_x + hue_dyn + time * 10.0f, 360.0f);
            float sat_breath = 0.85f + 0.25f * std::sin(time * 0.12f * 6.28318f);
            c = 0.22f * sat_breath * (0.9f + 0.2f * u);
            l = 0.70f + 0.1f * std::cos(time * 0.05f * 6.28318f);
            break;
        }
        case ColorEffectType::OpArtIsoluminance: {
            // Isoluminance und Farbvibration (Moiré-Effekt)
            // Constant lightness and high chroma.
            l = 0.70f;
            c = 0.25f;
            
            // High frequency Moiré grid
            float grid1 = std::sin(u * 150.0f) * std::cos(magnitude * 150.0f);
            float grid2 = std::sin((u + 0.01f) * 148.0f) * std::cos((magnitude + orbitEnergy * 0.05f) * 148.0f);
            float moire = grid1 + grid2;
            
            // Hue oscillates sharply between two complementary colors (e.g. Red and Cyan) based on Moiré
            float hueBase = std::fmod(time * 15.0f, 360.0f);
            h = (moire > 0.0f) ? hueBase : std::fmod(hueBase + 180.0f, 360.0f);
            break;
        }
        case ColorEffectType::ThinFilmIridescence: {
            // Optical interference of thin dielectric film: 100-600nm thickness variation
            const float filmD = 350.0f + 250.0f * std::sin(u * 6.28318f + time * 0.5f);
            const float interference = std::sin((filmD / 550.0f) * 6.28318f + magnitude * 4.0f);
            h = std::fmod(u * 180.0f + interference * 120.0f + time * 20.0f, 360.0f);
            c = 0.28f * (0.7f + 0.3f * std::abs(interference));
            l = 0.68f + 0.12f * interference;
            break;
        }
        case ColorEffectType::ChromaticDispersion: {
            // Prism dispersion and refractive spectral splitting
            const float dispersion = std::sin(magnitude * 8.0f - time * 2.0f);
            h = std::fmod(u * 360.0f + dispersion * 60.0f + orbitEnergy * 45.0f, 360.0f);
            c = 0.26f;
            l = 0.70f + 0.10f * std::cos(u * 12.56637f);
            break;
        }
        case ColorEffectType::KineticVelocity: {
            // Relativistic Doppler redshift / blueshift gradient
            const float velRatio = std::clamp(magnitude / 2.5f, 0.0f, 1.0f);
            h = std::lerp(10.0f, 240.0f, velRatio) + orbitEnergy * 30.0f;
            c = 0.22f + 0.08f * velRatio;
            l = 0.60f + 0.25f * velRatio;
            break;
        }
        case ColorEffectType::Bioluminescence: {
            // Oceanic enzymatic luciferin cyan-emerald glow pulses
            const float rawPulse = std::max(0.0f, std::sin(time * 2.5f + u * 12.0f + orbitEnergy * 4.0f));
            const float pulse = rawPulse * rawPulse * rawPulse;
            h = std::lerp(175.0f, 150.0f, pulse);
            c = 0.20f + 0.15f * pulse;
            l = 0.35f + 0.55f * pulse;
            break;
        }
        case ColorEffectType::QuantumInterference: {
            // Artistic scalar palette; this does not alter the simulated field.
            const float phase = 10.0f * u + 20.0f * magnitude;
            const float interference = std::sin(phase) * std::cos(phase * 1.618f);
            h = std::lerp(210.0f, 330.0f, u) + hueRotation * 360.0f;
            c = 0.22f;
            l = 0.25f + 0.65f * (0.5f + 0.5f * interference);
            break;
        }
        case ColorEffectType::ChladniAcoustic: {
            // Nodal-style color from compact scalar observations, not geometry.
            const float nodal = std::abs(std::cos(3.0f * kPi * u) * std::cos(2.0f * kPi * magnitude)
                                       - std::sin(2.0f * kPi * u) * std::sin(3.0f * kPi * magnitude));
            const float w = 1.0f - smoothstep(0.0f, 0.25f, nodal);
            h = std::lerp(220.0f, 50.0f, w) + hueRotation * 360.0f;
            c = std::lerp(0.12f, 0.20f, w);
            l = std::lerp(0.25f, 0.85f, w);
            break;
        }
        case ColorEffectType::LivingPaletteAgent: {
            const float theta = u * 6.28318f;
            const float rLog = std::log1p(std::max(magnitude, 0.0f));
            const float wave1 = 0.6f * std::sin(6.28318f * (0.18f * u + 0.013f * time));
            const float wave2 = 0.4f * std::sin(6.28318f * (0.31f * u + 0.008f * time + 1.7f));
            const float spiral = 40.0f * theta * theta + 120.0f * rLog * rLog * orbitEnergy;
            const float hueX = 0.20f * u * 360.0f + wave1 * 40.0f + wave2 * 30.0f;
            h = std::fmod(time * 15.0f + hueX + spiral + hueRotation * 360.0f, 360.0f);
            const float breath2 = 0.5f + 0.5f * std::sin(time * 0.12f * 6.28318f);
            const float satSpiral = 0.22f * (0.85f + 0.25f * breath2) * (0.9f + 0.2f * std::sin(theta + rLog));
            c = std::clamp(satSpiral + orbitEnergy * 0.06f, 0.0f, 0.38f);
            l = 0.65f + 0.15f * std::sin(time * 0.05f * 6.28318f + 2.0f * rLog);
            break;
        }
        case ColorEffectType::StandardHSV:
        default: {
            float t = smoothstep(0.0f, 1.0f, u);
            if (t < 0.5f) {
                float f = t * 2.0f;
                h = std::lerp(45.0f, 320.0f, f);
                c = std::lerp(0.12f, 0.18f, f);
                l = std::lerp(0.75f, 0.65f, f);
            } else {
                float f = (t - 0.5f) * 2.0f;
                h = std::lerp(320.0f, 220.0f, f);
                c = std::lerp(0.18f, 0.14f, f);
                l = std::lerp(0.65f, 0.70f, f);
            }
            h = std::fmod(h + orbitEnergy * 45.0f + time * 12.0f, 360.0f);
            break;
        }
    }
    if (h < 0.0f) h += 360.0f;
    c = std::clamp(c * breath + orbitEnergy * 0.06f, 0.0f, 0.38f);
    l = std::clamp(l + driftLuma, 0.05f, 0.98f);
    return oklchToRgb(l, c, h);
}

struct JacobiSnCnResult {
    float sn = 0.0f;
    float cn = 1.0f;
    float dn = 1.0f;
};

[[nodiscard]] inline JacobiSnCnResult evaluateJacobiElliptic(float u, float k) noexcept {
    k = std::clamp(k, 0.0f, 0.9999f);
    if (k < 1.0e-5f) {
        return {.sn = std::sin(u), .cn = std::cos(u), .dn = 1.0f};
    }
    if (k > 0.999f) {
        const float t = std::tanh(u);
        const float s = 1.0f / std::cosh(u);
        return {.sn = t, .cn = s, .dn = s};
    }

    constexpr int kMaxSteps = 6;
    float a[kMaxSteps + 1];
    float g[kMaxSteps + 1];
    float c[kMaxSteps + 1];

    a[0] = 1.0f;
    g[0] = std::sqrt(1.0f - k * k);
    c[0] = k;

    int n = 0;
    while (n < kMaxSteps && c[n] > 1.0e-6f) {
        a[n + 1] = 0.5f * (a[n] + g[n]);
        g[n + 1] = std::sqrt(a[n] * g[n]);
        c[n + 1] = 0.5f * (a[n] - g[n]);
        ++n;
    }

    float phi = std::pow(2.0f, static_cast<float>(n)) * a[n] * u;
    for (int i = n; i >= 1; --i) {
        const float sinPhi = std::sin(phi);
        const float ratio = std::clamp((c[i] / a[i]) * sinPhi, -1.0f, 1.0f);
        phi = 0.5f * (phi + std::asin(ratio));
    }

    const float sn = std::sin(phi);
    const float cn = std::cos(phi);
    const float dn = std::sqrt(std::max(0.0f, 1.0f - k * k * sn * sn));
    return {.sn = sn, .cn = cn, .dn = dn};
}

[[nodiscard]] inline Complexf applyAffine(Complexf z, const ComplexAffine& affine) {
    return Complexf{affine.scaleReal, affine.scaleImag} * z
         + Complexf{affine.offsetReal, affine.offsetImag};
}

[[nodiscard]] inline Complexf applyInner(InnerMap inner, Complexf z) {
    switch (inner) {
    case InnerMap::Identity: return z;
    case InnerMap::Square: return z * z;
    case InnerMap::Cube: return z * z * z;
    case InnerMap::Sin: return epi::sin(z);
    case InnerMap::Cos: return epi::cos(z);
    case InnerMap::Sinh: return epi::sinh(z);
    case InnerMap::Cosh: return epi::cosh(z);
    case InnerMap::Tanh: return epi::tanh(z);
    case InnerMap::Exp: return epi::exp(z);
    case InnerMap::Invert: {
        // Preserve epiC's conformal 1/z concept while making the CPU oracle
        // deterministic at the singularity. Reduction will fold large finite
        // values back into the visible domain.
        const float magnitudeSquared = std::max(z.magnitude_sq(), 1.0e-5f);
        return {z.re / magnitudeSquared, -z.im / magnitudeSquared};
    }
    case InnerMap::Kaleidoscope: {
        const float r = z.magnitude();
        const float theta = std::atan2(z.im, z.re);
        const float folds = 6.0f;
        const float wedge = 2.0f * kPi / folds;
        float wrapped = std::fmod(theta + kPi, wedge);
        if (wrapped < 0.0f) wrapped += wedge;
        const float foldedTheta = std::abs(wrapped - wedge * 0.5f) - kPi;
        return Complexf::from_polar(r, foldedTheta);
    }
    case InnerMap::Log: {
        const float r = std::max(z.magnitude(), 1.0e-5f);
        const float theta = std::atan2(z.im, z.re);
        return {std::log(r), theta};
    }
    case InnerMap::Henon: {
        return {1.0f - 1.4f * z.re * z.re + z.im, 0.3f * z.re};
    }
    case InnerMap::Clifford: {
        return {std::sin(-1.4f * z.im) + 1.0f * std::cos(-1.4f * z.re),
                std::sin(1.6f * z.re) + 0.7f * std::cos(1.6f * z.im)};
    }
    case InnerMap::Lissajous: {
        return {std::sin(1.5f * z.re + 0.5f), std::sin(2.0f * z.im)};
    }
    case InnerMap::Mandelbox: {
        float x = z.re;
        float y = z.im;
        if (x > 1.0f) x = 2.0f - x; else if (x < -1.0f) x = -2.0f - x;
        if (y > 1.0f) y = 2.0f - y; else if (y < -1.0f) y = -2.0f - y;
        float mag = x * x + y * y;
        if (mag < 0.25f) { x *= 4.0f; y *= 4.0f; }
        else if (mag < 1.0f && mag > 0.0001f) { x /= mag; y /= mag; }
        return {x * 2.0f, y * 2.0f};
    }
    case InnerMap::Gyroid: {
        return {std::sin(z.re) * std::cos(z.im) + std::sin(z.im) * std::cos(z.re),
                std::sin(z.re) * std::sin(z.im) - std::cos(z.re) * std::cos(z.im)};
    }
    case InnerMap::Drop: {
        float r = z.magnitude();
        float theta = std::atan2(z.im, z.re);
        float d = 1.0f + r * r;
        return {r * std::cos(theta) / d, r * std::sin(theta) / d};
    }
    case InnerMap::Mobius: {
        float denMag = std::max((z.re + 1.0f)*(z.re + 1.0f) + z.im*z.im, 1e-5f);
        return {((z.re - 1.0f)*(z.re + 1.0f) + z.im*z.im) / denMag,
                (z.im*(z.re + 1.0f) - (z.re - 1.0f)*z.im) / denMag};
    }
    case InnerMap::QuaternionJulia: {
        Quaternion q(z.re, z.im, 0.0f, 0.0f);
        Quaternion c(-0.4f, 0.6f, 0.0f, 0.0f);
        Quaternion qPrime(1.0f, 0.0f, 0.0f, 0.0f);
        Quaternion::juliaStep(q, qPrime, c);
        return {q.w, q.x};
    }
    case InnerMap::QuaternionMandelbrot: {
        Quaternion q(z.re, z.im, 0.0f, 0.0f);
        Quaternion qExp = Quaternion::exp(q);
        return {qExp.w, qExp.x};
    }
    case InnerMap::OctonionFeedback: {
        // 8D Non-Associative Octonionic Mandelbrot/Julia Dynamic Section with G2 Automorphism
        Octonion oct(z.re, z.im, 0.25f * z.re, -0.15f * z.im,
                     0.1f * z.im, 0.05f * z.re, -0.08f * z.im, 0.12f * z.re);
        Octonion octRot = Octonion::rotateG2(oct, 0.785398f);
        Octonion octSq = octRot.square();
        Octonion octExp = Octonion::exp(octRot * 0.35f);
        Octonion octOut = octSq + octExp;
        auto p3 = octOut.projectTo3D();
        return {p3[0], p3[1]};
    }
    case InnerMap::PoincareHyperbolic: {
        // Poincaré Disk Conformal Hyperbolic Inversion & Geodesic Dilation:
        // Points outside unit disk (|z| >= 1) are conformally inverted back inside;
        // points inside are dilated along hyperbolic geodesics artanh(|z|).
        float r2 = z.re * z.re + z.im * z.im;
        if (r2 >= 1.0f) {
            float invR2 = 1.0f / std::max(r2, 1e-5f);
            z.re *= invR2;
            z.im *= -invR2;
            r2 = z.re * z.re + z.im * z.im;
        }
        float r = std::sqrt(std::max(r2, 1e-8f));
        float rClamped = std::clamp(r, 1e-4f, 0.999f);
        float theta = std::atan2(z.im, z.re);
        float hypDist = 0.5f * std::log((1.0f + rClamped) / (1.0f - rClamped));
        float rOut = std::tanh(1.5f * hypDist);
        return {rOut * std::cos(theta), rOut * std::sin(theta)};
    }
    case InnerMap::KleinProjective: {
        // Beltrami-Klein Projective Disk Mapping:
        // Straightens hyperbolic geodesics into Euclidean chords: z_K = 2z / (1 + |z|^2)
        float r2 = z.re * z.re + z.im * z.im;
        float denom = 1.0f + r2;
        float kx = 2.0f * z.re / denom;
        float ky = 2.0f * z.im / denom;
        float kr2 = std::min(kx * kx + ky * ky, 0.98f);
        float factor = 1.0f / (1.0f + std::sqrt(std::max(1.0f - kr2, 1e-5f)));
        return {kx * factor, ky * factor};
    }
    case InnerMap::CalabiYauQuintic: {
        // Fermat Quintic Threefold Section: z -> (1 - z^5)^(1/5)
        float r = std::sqrt(z.re * z.re + z.im * z.im);
        float theta = std::atan2(z.im, z.re);
        float r5 = std::pow(std::clamp(r, 0.0f, 2.5f), 5.0f);
        float z5_re = r5 * std::cos(5.0f * theta);
        float z5_im = r5 * std::sin(5.0f * theta);
        float w_re = 1.0f - z5_re;
        float w_im = -z5_im;
        float rw = std::sqrt(w_re * w_re + w_im * w_im);
        float phi_w = std::atan2(w_im, w_re);
        float out_r = std::pow(std::max(rw, 1e-6f), 0.2f);
        return {out_r * std::cos(phi_w * 0.2f), out_r * std::sin(phi_w * 0.2f)};
    }
    case InnerMap::OctonionCayley: {
        // 8D Cayley-Dickson Non-Associative Cubic Iteration with Non-Zero Associator
        Octonion a(z.re, z.im, 0.1f, -0.1f, 0.05f, -0.05f, 0.02f, -0.02f);
        Octonion b(0.2f, -0.1f, z.re * 0.5f, z.im * 0.5f, 0.1f, 0.0f, -0.1f, 0.0f);
        Octonion c(0.1f, 0.2f, -0.1f, 0.0f, z.im * 0.3f, z.re * 0.3f, 0.05f, -0.05f);
        
        Octonion prod = a * (b * c);
        Octonion assoc = Octonion::associator(a, b, c);
        Octonion octCombined = prod + assoc * 0.5f;
        auto p3 = octCombined.projectTo3D();
        return {p3[0], p3[1]};
    }
    case InnerMap::CliffordHopf: {
        // Clifford Flat Torus Hopf Fibration Projection (S^3 -> S^2)
        float r1_sq = z.re * z.re + z.im * z.im;
        float r2_sq = 1.0f - std::clamp(r1_sq, 0.0f, 0.95f);
        float r2 = std::sqrt(r2_sq);
        Complexf z2 = {r2 * std::cos(1.5f * z.re), r2 * std::sin(1.5f * z.im)};
        float den = std::max(r1_sq + r2_sq, 1e-5f);
        float hopf_x = 2.0f * (z.re * z2.re + z.im * z2.im) / den;
        float hopf_y = 2.0f * (z.im * z2.re - z.re * z2.im) / den;
        return {hopf_x, hopf_y};
    }
    case InnerMap::LorenzAttractor: {
        // Lorenz attractor dynamics: dx/dt = sigma*(y-x), dy/dt = x*(rho-z)-y
        float x = z.re;
        float y = z.im;
        float dx = 10.0f * (y - x);
        float dy = x * (28.0f - (x * x + y * y) * 0.04f) - y;
        float nx = x + 0.02f * dx;
        float ny = y + 0.02f * dy;
        float mag = std::sqrt(nx * nx + ny * ny);
        if (mag > 4.0f && mag > 1e-5f) { nx *= 4.0f / mag; ny *= 4.0f / mag; }
        return {nx, ny};
    }
    case InnerMap::RosslerAttractor: {
        // Rössler attractor dynamics: dx/dt = -y - (x^2+y^2)*0.08, dy/dt = x + 0.2*y
        float x = z.re;
        float y = z.im;
        float dx = -y - (x * x + y * y) * 0.08f;
        float dy = x + 0.2f * y;
        float nx = x + 0.025f * dx;
        float ny = y + 0.025f * dy;
        float mag = std::sqrt(nx * nx + ny * ny);
        if (mag > 4.0f && mag > 1e-5f) { nx *= 4.0f / mag; ny *= 4.0f / mag; }
        return {nx, ny};
    }
    case InnerMap::AizawaAttractor: {
        // Aizawa spherical attractor dynamics
        float x = z.re;
        float y = z.im;
        float r2 = x * x + y * y;
        float dx = (y - 0.7f) * x - 3.5f * y;
        float dy = 3.5f * x + (y - 0.7f) * y - r2 * 0.15f;
        float nx = x + 0.015f * dx;
        float ny = y + 0.015f * dy;
        float mag = std::sqrt(nx * nx + ny * ny);
        if (mag > 4.0f && mag > 1e-5f) { nx *= 4.0f / mag; ny *= 4.0f / mag; }
        return {nx, ny};
    }
    case InnerMap::ChenAttractor: {
        // Chen dual-scroll chaotic attractor: dx/dt = a*(y-x), dy/dt = (c-a)*x - x*r2 + c*y
        float x = z.re;
        float y = z.im;
        float dx = 3.5f * (y - x);
        float dy = -0.7f * x - x * (x * x + y * y) * 0.003f + 2.8f * y;
        float nx = x + 0.015f * dx;
        float ny = y + 0.015f * dy;
        float mag = std::sqrt(nx * nx + ny * ny);
        if (mag > 4.0f && mag > 1e-5f) { nx *= 4.0f / mag; ny *= 4.0f / mag; }
        return {nx, ny};
    }
    case InnerMap::ReactionDiffusionTuring: {
        // 2D/3D Gray-Scott Reaction-Diffusion Turing Morphogenesis:
        // Solves discrete reaction-diffusion kinetics on domain z:
        // Du = 0.18, Dv = 0.09, F = 0.035, k = 0.065
        float x = z.re;
        float y = z.im;
        float r2 = x * x + y * y;
        float u = std::clamp(1.0f - 0.5f * std::exp(-r2 * 2.0f) * std::cos(4.0f * x) * std::cos(4.0f * y), 0.0f, 1.0f);
        float v = std::clamp(0.25f * std::exp(-r2 * 2.0f) * (1.0f + std::sin(3.0f * x + y)), 0.0f, 1.0f);
        
        float lapU = -16.0f * (u - 0.5f) * (std::cos(3.0f * x) * std::cos(3.0f * y));
        float lapV = -16.0f * v * (std::cos(3.0f * x) * std::cos(3.0f * y));
        
        float Du = 0.18f;
        float Dv = 0.09f;
        float F = 0.035f;
        float k = 0.065f;
        float uvv = u * v * v;
        
        float du = Du * lapU - uvv + F * (1.0f - u);
        float dv = Dv * lapV + uvv - (F + k) * v;
        
        float u_next = std::clamp(u + 0.8f * du, 0.0f, 1.0f);
        float v_next = std::clamp(v + 0.8f * dv, 0.0f, 1.0f);
        
        float angle = std::atan2(y, x) + (v_next - u_next) * 2.5f;
        float radius = std::sqrt(r2) * (0.85f + 0.35f * v_next);
        return {radius * std::cos(angle), radius * std::sin(angle)};
    }
    case InnerMap::HyperbolicMandelboxFold: {
        // Multi-Scale Hyperbolic Mandelbox Folding
        float px = std::clamp(z.re, -1.0f, 1.0f) * 2.0f - z.re;
        float py = std::clamp(z.im, -1.0f, 1.0f) * 2.0f - z.im;
        float r2 = px * px + py * py;
        float minR2 = 0.25f;
        float fixedR2 = 1.0f;
        if (r2 < minR2) {
            float factor = fixedR2 / minR2;
            px *= factor;
            py *= factor;
        } else if (r2 < fixedR2 && r2 > 1e-6f) {
            float factor = fixedR2 / r2;
            px *= factor;
            py *= factor;
        }
        float scale = 1.85f;
        px = px * scale + 0.35f;
        py = py * scale - 0.25f;
        float mag = std::sqrt(px * px + py * py);
        if (mag > 5.0f && mag > 1e-5f) {
            px *= 5.0f / mag;
            py *= 5.0f / mag;
        }
        return {px, py};
    }
    case InnerMap::HyperbolicExponentialHomotopy: {
        // Conformal Geodesic Homotopy: H(z, lambda) = (1 - lambda)*sinh(z) + lambda*exp(z)
        // Bridges tensioned hyperbolic sheets and radial exponential cascades.
        Complexf z_sinh = epi::sinh(z);
        Complexf z_exp = epi::exp(z);
        Complexf z_homotopy = z_sinh * 0.5f + z_exp * 0.5f;
        float mag = z_homotopy.magnitude();
        if (mag > 6.0f && mag > 1e-5f) {
            return z_homotopy * (6.0f / mag);
        }
        return z_homotopy;
    }
    case InnerMap::OctonionAssociatorManifold: {
        // 8D Non-Associative Octonionic Lie Algebra g2 Flow & Non-Zero Associator Manifold:
        // Computes triple non-associative curvature [X, Y, Z] = (XY)Z - X(YZ) combined with
        // continuous G2 exceptional automorphism rotation along Fano plane triad circles.
        Octonion x(z.re, z.im, 0.35f * z.re, -0.25f * z.im, 0.15f * z.im, 0.10f * z.re, -0.05f * z.im, 0.08f * z.re);
        Octonion y(0.25f, -0.15f, z.re * 0.4f, z.im * 0.4f, 0.05f, -0.05f, 0.12f, -0.08f);
        Octonion zOct(0.15f, 0.20f, -0.10f, 0.05f, z.im * 0.25f, z.re * 0.25f, 0.04f, -0.04f);

        Octonion rotX = Octonion::rotateG2(x, z.re * 0.5f + z.im * 0.5f);
        Octonion assoc = Octonion::associator(rotX, y, zOct);
        Octonion quad = (rotX * y) + assoc * 0.75f;
        auto p3 = quad.projectTo3D();
        return {p3[0], p3[1]};
    }
    case InnerMap::HolomorphicPotential: {
        // Quantum Holomorphic Complex Potential Field with Multipole Singularities
        Complexf c1 = {0.55f, 0.40f};
        Complexf c2 = {-0.55f, -0.40f};
        Complexf c3 = {0.0f, 0.65f};
        Complexf d1 = z - c1;
        Complexf d2 = z - c2;
        Complexf d3 = z - c3;
        float r1 = std::max(d1.magnitude(), 1e-4f);
        float r2 = std::max(d2.magnitude(), 1e-4f);
        float r3 = std::max(d3.magnitude(), 1e-4f);
        float theta1 = std::atan2(d1.im, d1.re);
        float theta2 = std::atan2(d2.im, d2.re);
        float theta3 = std::atan2(d3.im, d3.re);
        float phi = 0.5f * std::log(r1) + 1.8f * theta1 - 0.5f * std::log(r2) - 1.8f * theta2 + 1.2f * theta3;
        float psi = -1.8f * std::log(r1) + 0.5f * theta1 + 1.8f * std::log(r2) - 0.5f * theta2 - 1.2f * std::log(r3);
        Complexf z2 = z * z;
        Complexf z3 = z2 * z;
        Complexf pot = {phi * 0.45f + z3.re * 0.15f, psi * 0.45f + z3.im * 0.15f};
        float mag = pot.magnitude();
        if (mag > 6.0f && mag > 1e-5f) {
            return pot * (6.0f / mag);
        }
        return pot;
    }
    case InnerMap::HyperbolicHoneycomb: {
        // 4D Non-Euclidean Hyperbolic Honeycomb & Seifert Surface Mapping
        float r2 = z.re * z.re + z.im * z.im;
        if (r2 >= 1.0f) {
            float invR2 = 1.0f / r2;
            z = Complexf(z.re * invR2, -z.im * invR2);
            r2 = z.re * z.re + z.im * z.im;
        }
        float r = std::sqrt(std::max(r2, 1e-8f));
        float th = std::atan2(z.im, z.re);
        constexpr float kSector = 2.0f * kPi / 5.0f;
        float thFold = std::fmod(th + kPi / 5.0f, kSector);
        if (thFold < 0.0f) thFold += kSector;
        thFold -= kPi / 5.0f;
        float d = 0.5f * std::log((1.0f + std::clamp(r, 1e-4f, 0.999f)) / (1.0f - std::clamp(r, 1e-4f, 0.999f)));
        float rOut = std::tanh(1.85f * d);
        float seifert = 0.35f * (1.0f - rOut * rOut);
        return Complexf(rOut * std::cos(thFold) + seifert * std::cos(3.0f * thFold),
                        rOut * std::sin(thFold) + seifert * std::sin(2.0f * thFold));
    }
    case InnerMap::KuramotoLattice: {
        // Non-linear Kuramoto Phase Synchronization Lattice
        const float theta = std::atan2(z.im, z.re);
        const float r = z.magnitude();
        const float coupling = 0.65f * std::sin(3.0f * theta) + 0.35f * std::sin(5.0f * theta);
        const float theta_next = theta + 0.45f * coupling;
        const float r_next = std::tanh(1.2f * r);
        return Complexf(r_next * std::cos(theta_next), r_next * std::sin(theta_next));
    }
    case InnerMap::QuantumWave: {
        // Quantum Harmonic Wave Packet Projection
        const float r2 = z.re * z.re + z.im * z.im;
        const float envelope = std::exp(-0.35f * r2);
        const float waveRe = (2.0f * z.re * z.re - 1.0f) * std::cos(2.5f * z.im);
        const float waveIm = (2.0f * z.im * z.im - 1.0f) * std::sin(2.5f * z.re);
        return Complexf(waveRe * envelope, waveIm * envelope);
    }
    case InnerMap::JacobiEllipticSnCn: {
        // Jacobi Elliptic Doubly Periodic Modular Soliton Mapping
        const float kMod = 0.785f;
        const auto jx = evaluateJacobiElliptic(z.re * 2.0f, kMod);
        const auto jy = evaluateJacobiElliptic(z.im * 2.0f, kMod);
        const float outRe = jx.sn * jy.dn;
        const float outIm = jy.cn * jx.dn;
        return Complexf(outRe, outIm);
    }
        case InnerMap::PenroseTiling4D: {
        float th1 = 3.14159265f * 0.4f;
        float th2 = 3.14159265f * 0.8f;
        float v1_re = std::cos(th1) * z.re + std::sin(th1) * z.im;
        float v1_im = std::cos(th2) * z.im - std::sin(th2) * z.re;
        float v2_re = std::cos(th2) * z.re + std::sin(th2) * z.im;
        float v2_im = std::cos(th1) * z.im - std::sin(th1) * z.re;
        float p_re = std::sin(v1_re) * std::cosh(v2_im);
        float p_im = std::cos(v2_re) * std::sinh(v1_im);
        Complexf p(p_re, p_im);
        float m = p.magnitude();
        if (m > 3.0f && m > 1e-5f) {
            p = p * (3.0f / m);
        }
        return p;
    }
    case InnerMap::ChebyshevPolynomial: {
        // Multi-Order Chebyshev Polynomial Fold: T3(x) and T5(y)
        const float cx = std::clamp(z.re, -1.0f, 1.0f);
        const float cy = std::clamp(z.im, -1.0f, 1.0f);
        const float t3 = 4.0f * cx * cx * cx - 3.0f * cx;
        const float y3 = cy * cy * cy;
        const float y5 = y3 * cy * cy;
        const float t5 = 16.0f * y5 - 20.0f * y3 + 5.0f * cy;
        Complexf result(t3, t5);
        const float mag = result.magnitude();
        if (mag > 2.5f && mag > 1e-5f) {
            result = result * (2.5f / mag);
        }
        return result;
    }
    case InnerMap::SiphonophoreAttractor: {
        // Siphonophore Hydrodynamic Marine Attractor:
        // Twisting vortex log-spiral logarithmic potential
        const float r2 = z.re * z.re + z.im * z.im;
        const float ang = std::atan2(z.im, z.re);
        const float twist = ang + 1.5f * std::sin(r2 * 4.0f);
        const float rad = 0.5f * std::log1p(r2) + 0.15f * std::cos(3.0f * ang);
        return {rad * std::cos(twist), rad * std::sin(twist)};
    }
    case InnerMap::KaliSetOrbit: {
        // Kali Set Fractal Orbit Folding:
        // Inversion and affine reflection loop
        float px = std::abs(z.re);
        float py = std::abs(z.im);
        float m = px * px + py * py;
        if (m < 0.25f) {
            px *= 4.0f;
            py *= 4.0f;
        } else if (m < 1.0f && m > 1e-6f) {
            float invM = 1.0f / m;
            px *= invM;
            py *= invM;
        }
        return {px * 1.5f - 0.5f, py * 1.5f - 0.5f};
    }
    }
    return z;
}

[[nodiscard]] inline Complexf transformPlane(Complexf z,
                                      const SculptureProgramParams& params,
                                      float phase) {
    if (!learn_mode::enabled(params.stageMask, learn_mode::Stage::Transform)) return z;
    // The small animated unit-complex multiplier is epiC's pattern-time drift:
    // multiplication rotates and scales in one operation.
    const Complexf drift = Complexf::from_polar(1.0f, phase);
    z = applyAffine(drift * z, params.pre);
    Complexf zA = applyInner(params.inner, z);
    Complexf zB = applyInner(params.innerB, z);
    z = SculptureFractalCatalog::evaluateHomotopy(zA, zB, z, params.topologyBlend);
    return applyAffine(z, params.post);
}


[[nodiscard]] inline float gridReduce(float value);
[[nodiscard]] inline Vec4f liftFourDimensions(Vec4f v,
                                       const SculptureProgramParams& params,
                                       float time,
                                       std::uint32_t iteration) {
    if (!learn_mode::enabled(params.stageMask, learn_mode::Stage::Transform)) return v;
    const float drift = params.patternSpeed * time
                      + 0.11f * static_cast<float>(iteration);
                      
    const float w = params.latentW;
    
    // SO(4) Dual-Isoclinic Rotation (Left + Right commuting isoclinic angles)
    const float alpha = drift + w + params.rotorAlpha;
    const float beta = drift * 0.61803398875f - w * 0.5f + params.rotorBeta;
    const float thetaL = alpha + beta;
    const float thetaR = alpha - beta;
    
    const float cosL = std::cos(thetaL), sinL = std::sin(thetaL);
    const float cosR = std::cos(thetaR), sinR = std::sin(thetaR);
    
    Vec4f r4 = {
        v.x * cosL - v.y * sinL,
        v.x * sinL + v.y * cosL,
        v.z * cosR - v.w * sinR,
        v.z * sinR + v.w * cosR
    };

    if (std::abs(params.rotorTheta4D) > 1e-5f) {
        const float cosT = std::cos(params.rotorTheta4D);
        const float sinT = std::sin(params.rotorTheta4D);
        const float zw_z = r4.z * cosT - r4.w * sinT;
        const float zw_w = r4.z * sinT + r4.w * cosT;
        r4.z = zw_z;
        r4.w = zw_w;
    }
    
    // Evaluate the 6 principal 4D planes with distributed phases
    const Complexf xy = transformPlane({r4.x, r4.y}, params, drift + w);
    const Complexf xz = transformPlane({r4.x, r4.z}, params, drift + 1.0f * kPi / 3.0f + w * 1.5f);
    const Complexf xw = transformPlane({r4.x, r4.w}, params, drift + 2.0f * kPi / 3.0f - w);
    const Complexf yz = transformPlane({r4.y, r4.z}, params, drift + 3.0f * kPi / 3.0f + w * 0.5f);
    const Complexf yw = transformPlane({r4.y, r4.w}, params, drift + 4.0f * kPi / 3.0f - w * 1.5f);
    const Complexf zw = transformPlane({r4.z, r4.w}, params, drift + 5.0f * kPi / 3.0f + w * 2.0f);

    return {
        (xy.re + xz.re + xw.re) / 3.0f,
        (xy.im + yz.re + yw.re) / 3.0f,
        (xz.im + yz.im + zw.re) / 3.0f,
        (xw.im + yw.im + zw.im) / 3.0f
    };
}

[[nodiscard]] inline Vec4f reduceDomain4(Vec4f v, ReductionMode mode, std::uint32_t stageMask) {
    if (!learn_mode::enabled(stageMask, learn_mode::Stage::Reduce)) return v;
    auto sanitize = [](float val) { return std::isnan(val) || std::isinf(val) ? 0.0f : val; };
    v.x = sanitize(v.x); v.y = sanitize(v.y); v.z = sanitize(v.z); v.w = sanitize(v.w);
    
    if (mode == ReductionMode::Grid) {
        return {gridReduce(v.x), gridReduce(v.y), gridReduce(v.z), gridReduce(v.w)};
    } else if (mode == ReductionMode::Clamp) {
        return {
            std::clamp(v.x, -1.0f, 1.0f),
            std::clamp(v.y, -1.0f, 1.0f),
            std::clamp(v.z, -1.0f, 1.0f),
            std::clamp(v.w, -1.0f, 1.0f),
        };
    } else if (mode == ReductionMode::PareidoliaCathedral) {
        auto fold = [](float val) {
            float wrapped = std::fmod(val + 1.0f, 4.0f);
            if (wrapped < 0.0f) wrapped += 4.0f;
            return 1.0f - std::abs(wrapped - 2.0f);
        };
        Vec4f p = {fold(v.x), fold(v.y), fold(v.z), fold(v.w)};
        if (p.x + p.y < 0.0f) { float tmp = -p.y; p.y = -p.x; p.x = tmp; }
        if (p.z + p.w < 0.0f) { float tmp = -p.w; p.w = -p.z; p.z = tmp; }
        return p;
    } else if (mode == ReductionMode::AmmannBeenker) {
        auto fold = [](float val) {
            float wrapped = std::fmod(val + 1.0f, 4.0f);
            if (wrapped < 0.0f) wrapped += 4.0f;
            return 1.0f - std::abs(wrapped - 2.0f);
        };
        Vec4f p = {fold(v.x), fold(v.y), fold(v.z), fold(v.w)};
        float r1 = std::sqrt(p.x * p.x + p.y * p.y);
        float phi1 = std::atan2(p.y, p.x);
        float fold1 = std::fmod(phi1 + 3.14159265f / 8.0f, 3.14159265f / 4.0f);
        if (fold1 < 0.0f) fold1 += 3.14159265f / 4.0f;
        fold1 -= 3.14159265f / 8.0f;
        p.x = r1 * std::cos(fold1);
        p.y = r1 * std::sin(fold1);
        return p;
    } else if (mode == ReductionMode::PoincareHyperbolic) {
        float r2 = v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w;
        float r = std::sqrt(std::max(r2, 1e-6f));
        if (r >= 1.0f) {
            float factor = std::tanh(r) / (r * r);
            return {v.x * factor, v.y * factor, v.z * factor, v.w * factor};
        }
        return v;
    } else if (mode == ReductionMode::CliffordFoliation) {
        float foldX = std::cos(3.14159265f * v.x);
        float foldY = std::sin(3.14159265f * v.x);
        float foldZ = std::cos(3.14159265f * v.y);
        float foldW = std::sin(3.14159265f * v.y);
        float den = std::max(1.0f - 0.5f * foldW, 0.25f);
        return {foldX / den, foldY / den, foldZ / den, (v.z + v.w) * 0.5f};
    }
    
    // Torus (Triangle wave)
    auto fold = [](float val) {
        float wrapped = std::fmod(val + 1.0f, 4.0f);
        if (wrapped < 0.0f) wrapped += 4.0f;
        return 1.0f - std::abs(wrapped - 2.0f);
    };
    return {fold(v.x), fold(v.y), fold(v.z), fold(v.w)};
}

[[nodiscard]] inline Vec3f liftThreePlanes(Vec3f v,
                                    const SculptureProgramParams& params,
                                    float time,
                                    std::uint32_t iteration) {
    if (!learn_mode::enabled(params.stageMask, learn_mode::Stage::Transform)) return v;
    const float drift = params.patternSpeed * time
                      + 0.11f * static_cast<float>(iteration);
                      
    // 4D Latent Coordinate: breaks the rigid 120-degree slice symmetry,
    // exploring a continuous hyper-dimensional subspace of the topology.
    const float w = params.latentW;
    
    const Complexf xy = transformPlane({v.x, v.y}, params, drift + w);
    const Complexf yz = transformPlane({v.y, v.z}, params, drift + 2.0f * kPi / 3.0f - w * 0.5f);
    const Complexf zx = transformPlane({v.z, v.x}, params, drift + 4.0f * kPi / 3.0f + w * 2.0f);

    // Every coordinate participates in two complex slices.  For Identity with
    // zero drift this averaging reconstructs the original vector exactly;
    // nonlinear maps couple all three planes into one 3D epimorphism.
    return {
        0.5f * (xy.re + zx.im),
        0.5f * (xy.im + yz.re),
        0.5f * (yz.im + zx.re),
    };
}

[[nodiscard]] inline float gridReduce(float value) {
    float wrapped = std::fmod(value + 1.0f, 2.0f);
    if (wrapped < 0.0f) {
        wrapped += 2.0f;
    }
    return wrapped - 1.0f;
}

[[nodiscard]] inline Vec3f sanitize(Vec3f v) {
    auto component = [](float value) {
        return std::isfinite(value) ? std::clamp(value, -32.0f, 32.0f) : 0.0f;
    };
    return {component(v.x), component(v.y), component(v.z)};
}

[[nodiscard]] inline Vec3f reduceDomain(Vec3f v, ReductionMode mode, std::uint32_t stageMask) {
    if (!learn_mode::enabled(stageMask, learn_mode::Stage::Reduce)) return v;
    v = sanitize(v);
    switch (mode) {
    case ReductionMode::Torus:
        // This is epiC's existing 3D extension of the canonical 2D
        // triangle-wave fold, not a demo-local approximation.
        return epi::torus_reduce3(v);
    case ReductionMode::Grid:
        return {gridReduce(v.x), gridReduce(v.y), gridReduce(v.z)};
    case ReductionMode::Clamp:
        return {
            std::clamp(v.x, -1.0f, 1.0f),
            std::clamp(v.y, -1.0f, 1.0f),
            std::clamp(v.z, -1.0f, 1.0f),
        };
    case ReductionMode::PareidoliaCathedral: {
        Vec3f p = epi::torus_reduce3(v);
        if (p.x + p.y < 0.0f) {
            float tmp = -p.y;
            p.y = -p.x;
            p.x = tmp;
        }
        if (p.x + p.z < 0.0f) {
            float tmp = -p.z;
            p.z = -p.x;
            p.x = tmp;
        }
        p.z = p.z * (1.0f - 0.25f * (p.x * p.x + p.y * p.y));
        return p;
    }
    case ReductionMode::AmmannBeenker: {
        Vec3f p = epi::torus_reduce3(v);
        constexpr float kTanPi8 = 0.41421356237f;
        if (std::abs(p.y) > kTanPi8 * std::abs(p.x)) {
            float r = std::sqrt(p.x * p.x + p.y * p.y);
            float phi = std::atan2(p.y, p.x);
            float phiFold = std::fmod(phi + 3.14159265f / 8.0f, 3.14159265f / 4.0f);
            if (phiFold < 0.0f) phiFold += 3.14159265f / 4.0f;
            phiFold -= 3.14159265f / 8.0f;
            p.x = r * std::cos(phiFold);
            p.y = r * std::sin(phiFold);
        }
        return p;
    }
    case ReductionMode::PoincareHyperbolic: {
        float r2 = v.x * v.x + v.y * v.y + v.z * v.z;
        float r = std::sqrt(std::max(r2, 1e-6f));
        if (r >= 1.0f) {
            float factor = std::tanh(r) / (r * r);
            return {v.x * factor, v.y * factor, v.z * factor};
        }
        return v;
    }
    case ReductionMode::CliffordFoliation: {
        Vec3f p = epi::torus_reduce3(v);
        float aX = std::cos(3.14159265f * p.x);
        float aY = std::sin(3.14159265f * p.x);
        float bX = std::cos(3.14159265f * p.y);
        float bY = std::sin(3.14159265f * p.y);
        float den = std::max(1.0f - 0.5f * bY, 0.25f);
        return {aX / den, aY / den, bX / den};
    }
    }
    return v;
}

[[nodiscard]] inline Vec3f seedPointShape(std::size_t index, float time, std::uint32_t outputMode, StageResolutionScale auxiliaryScale) {
    std::size_t scale = static_cast<std::size_t>(auxiliaryScale);
    if (scale == 0) scale = 1;
    if (outputMode == 1) {
        const std::size_t mod_idx = index % 15625;
        const std::size_t x_f = mod_idx % 25;
        const std::size_t y_f = (mod_idx / 25) % 25;
        const std::size_t z_f = mod_idx / 625;
        const std::size_t x = (x_f / scale) * scale;
        const std::size_t y = (y_f / scale) * scale;
        const std::size_t z = (z_f / scale) * scale;
        return {
            ((static_cast<float>(x) / 24.0f) * 2.0f - 1.0f) * 1.5f,
            ((static_cast<float>(y) / 24.0f) * 2.0f - 1.0f) * 1.5f,
            ((static_cast<float>(z) / 24.0f) * 2.0f - 1.0f) * 1.5f,
        };
    } else if (outputMode == 2) {
        const std::size_t u_idx = ((index % 128) / scale) * scale;
        const std::size_t v_idx = ((index / 128) / scale) * scale;
        const float u_val = static_cast<float>(u_idx) / 127.0f;
        const float v_val = static_cast<float>(v_idx) / 127.0f;
        const float theta = 2.0f * 3.14159265f * u_val;
        const float phi = 2.0f * 3.14159265f * v_val;
        const float r1 = 1.0f;
        const float r2 = 0.35f;
        return {
            (r1 + r2 * std::cos(phi)) * std::cos(theta),
            r2 * std::sin(phi),
            (r1 + r2 * std::cos(phi)) * std::sin(theta)
        };
    } else if (outputMode == 3) {
        const float golden_ratio = (1.0f + std::sqrt(5.0f)) / 2.0f;
        const float i_f = static_cast<float>((index / scale) * scale);
        const float n_f = static_cast<float>(kSampleCount);
        const float y = std::clamp(1.0f - (i_f / std::max(1.0f, n_f - 1.0f)) * 2.0f, -1.0f, 1.0f);
        const float r = std::sqrt(std::max(0.0f, 1.0f - y * y));
        const float theta = 2.0f * 3.14159265f * i_f / golden_ratio;
        return {
            1.1f * r * std::cos(theta),
            1.1f * y,
            1.1f * r * std::sin(theta)
        };
    } else if (outputMode == 4) {
        // 4D Clifford Torus & Hopf Soliton projection from S^3
        const float u_val = static_cast<float>((index / scale) * scale) / static_cast<float>(kSampleCount);
        const float th = 2.0f * 3.14159265f * u_val * 3.0f + 0.05f * time;
        const float ph = 2.0f * 3.14159265f * u_val * 2.0f + 0.03f * time;
        const float qx = 0.70710678f * std::cos(th);
        const float qy = 0.70710678f * std::sin(th);
        const float qz = 0.70710678f * std::cos(ph);
        const float qw = 0.70710678f * std::sin(ph);
        const float denom = std::max(1.0f - qw * 0.5f, 0.2f);
        return {
            (1.35f / denom) * qx,
            (1.35f / denom) * qy,
            (1.35f / denom) * qz
        };
    } else if (outputMode == 5) {
        // Hyperbolic Pseudosphere (Tractricoid) with constant negative Gaussian curvature
        const float u_val = static_cast<float>((index / scale) * scale) / static_cast<float>(kSampleCount);
        const float th = 2.0f * 3.14159265f * u_val * 4.0f + 0.02f * time;
        const float v_val = std::clamp(u_val, 0.05f, 0.95f) * 3.0f - 1.5f;
        const float sech_v = 1.0f / std::cosh(v_val);
        const float tanh_v = std::tanh(v_val);
        return {
            sech_v * std::cos(th) * 1.2f,
            (v_val - tanh_v) * 0.8f,
            sech_v * std::sin(th) * 1.2f
        };
    } else if (outputMode == 6) {
        // Exponential Logarithmic Starburst Cascade
        const float u_val = static_cast<float>((index / scale) * scale) / static_cast<float>(kSampleCount);
        const float th = 2.0f * 3.14159265f * u_val * 6.0f + 0.04f * time;
        const float r = 0.25f + 0.85f * std::sqrt(std::max(0.0f, u_val));
        const float branch = std::sin(th * 5.0f);
        return {
            r * std::cos(th),
            0.35f * branch * r,
            r * std::sin(th)
        };
    } else if (outputMode == 7) {
        // 2.5D / 3D Non-Orientable Boy's Surface (RP^2 smooth immersion in R^3)
        const float u_val = static_cast<float>((index / scale) * scale) / static_cast<float>(kSampleCount);
        const float u = 3.14159265f * (u_val * 2.0f - 1.0f);
        const float v = 0.5f * 3.14159265f * std::sin(2.0f * 3.14159265f * u_val + 0.03f * time);
        const float cos_u = std::cos(u);
        const float sin_u = std::sin(u);
        const float cos_v = std::cos(v);
        const float sin_v = std::sin(v);
        const float denom = std::max(2.0f - 1.41421356f * std::sin(3.0f * u) * std::sin(2.0f * v), 0.35f);
        const float x = (1.41421356f * cos_v * cos_v * std::cos(2.0f * u) + cos_u * std::sin(2.0f * v)) / denom;
        const float y = (1.41421356f * cos_v * cos_v * std::sin(2.0f * u) - sin_u * std::sin(2.0f * v)) / denom;
        const float z = (3.0f * cos_v * cos_v) / denom - 1.5f;
        return { x * 0.75f, z * 0.75f, y * 0.75f };
    } else if (outputMode == 8) {
        // Costa Complete Minimal Surface of Genus 1 (Weierstrass P-function foliation)
        const float u_val = static_cast<float>((index / scale) * scale) / static_cast<float>(kSampleCount);
        const float theta = 2.0f * 3.14159265f * u_val * 4.0f + 0.025f * time;
        const float r = 0.3f + 0.9f * std::sqrt(std::clamp(u_val, 0.05f, 0.95f));
        const float x = r * std::cos(theta);
        const float y = r * std::sin(theta);
        const float z = 0.45f * (std::log(std::max(r, 0.2f)) - 0.5f * std::cos(2.0f * theta) / (r * r + 0.1f));
        return { x * 1.1f, z, y * 1.1f };
    } else if (outputMode == 9) {
        // 3.5D Octonionic Cayley-Dickson G2 Associative 3-Form Fibration Ribbon
        const float u_val = static_cast<float>((index / scale) * scale) / static_cast<float>(kSampleCount);
        const float t1 = 2.0f * 3.14159265f * u_val * 3.0f + 0.035f * time;
        const float t2 = 2.0f * 3.14159265f * u_val * 5.0f + 0.02f * time;
        const float q1 = std::cos(t1);
        const float q2 = std::sin(t1);
        const float q3 = std::cos(t2);
        const float q4 = std::sin(t2);
        const float phi_x = q1 * q3 - q2 * q4;
        const float phi_y = q1 * q4 + q2 * q3;
        const float phi_z = 0.5f * (q1 * q1 - q2 * q2 + q3 * q3 - q4 * q4);
        return { phi_x * 1.25f, phi_z * 1.25f, phi_y * 1.25f };
    } else if (outputMode == 10) {
        // Twistor Kerr-Robinson Null Congruence Ribbon (S3 -> R3 stereographic projection)
        const float u_val = static_cast<float>((index / scale) * scale) / static_cast<float>(kSampleCount);
        const float eta = 0.5f * std::acos(std::clamp(1.0f - 2.0f * u_val, -1.0f, 1.0f));
        const float psi = u_val * 16.0f * 3.14159265f + 0.08f * time;
        const float ptS3_x = std::cos(eta) * std::cos(psi + u_val * 6.28f);
        const float ptS3_y = std::cos(eta) * std::sin(psi + u_val * 6.28f);
        const float ptS3_z = std::sin(eta) * std::cos(psi);
        const float ptS3_w = std::sin(eta) * std::sin(psi);
        const float denom = std::max(1.0001f - ptS3_w, 0.001f);
        return { (1.25f / denom) * ptS3_x, (1.25f / denom) * ptS3_y, (1.25f / denom) * ptS3_z };
    } else if (outputMode == 11) {
        // Kuramoto Phase-Synchronized Dynamic Hypersurface Lattice
        const float u_val = static_cast<float>((index / scale) * scale) / static_cast<float>(kSampleCount);
        const float th = 2.0f * 3.14159265f * u_val * 5.0f + 0.05f * time;
        const float r = 0.8f + 0.25f * std::sin(th * 3.0f + 0.35f * std::cos(time * 2.0f));
        const float w_sync = 0.35f * std::sin(th * 2.0f - time * 1.5f);
        return { r * std::cos(th), w_sync, r * std::sin(th) };
    } else if (outputMode == 12) {
        // Dedekind Modular Eta & Jacobi Conformal Hypermanifold
        const float u_val = static_cast<float>((index / scale) * scale) / static_cast<float>(kSampleCount);
        const float th = 2.0f * 3.14159265f * u_val * 6.0f + 0.03f * time;
        const float q_mod = 0.65f * (1.0f + 0.3f * std::cos(th * 4.0f));
        const float z_re = q_mod * std::cos(th);
        const float z_im = q_mod * std::sin(th);
        const float eta_approx = 0.5f * std::sin(3.0f * th) * std::cos(2.0f * th);
        return { z_re * 1.3f, eta_approx * 0.7f, z_im * 1.3f };
    } else if (outputMode == 13) {
        // Lin-Shu Galactic Density Wave Dispersion Manifold
        const float u_val = static_cast<float>((index / scale) * scale) / static_cast<float>(kSampleCount);
        const float r = 0.2f + 1.1f * std::sqrt(u_val);
        const float theta_arm = 4.0f * std::log(std::max(r, 0.1f)) + 0.04f * time;
        const float z_wave = 0.25f * std::sin(2.0f * theta_arm) * std::exp(-r * 0.8f);
        return { r * std::cos(theta_arm), z_wave, r * std::sin(theta_arm) };
    } else if (outputMode == 14) {
        // Schwarzschild Photon Sphere Conformal Lensing Manifold
        const float u_val = static_cast<float>((index / scale) * scale) / static_cast<float>(kSampleCount);
        const float th = 2.0f * 3.14159265f * u_val * 8.0f + 0.06f * time;
        const float r_sphere = 1.0f / (1.0f + 0.5f * std::exp(-4.0f * std::abs(u_val - 0.5f)));
        const float deflection = 0.35f * std::sin(th * 2.0f);
        return { r_sphere * std::cos(th), deflection, r_sphere * std::sin(th) };
    } else if (outputMode == 15) {
        // Poincare Hyperbolic Vortex Chamber
        const float u_val = static_cast<float>((index / scale) * scale) / static_cast<float>(kSampleCount);
        const float th = 2.0f * 3.14159265f * u_val * 5.0f + 0.05f * time;
        const float r_hyp = std::tanh(1.5f * u_val);
        const float vortex_z = 0.4f * std::sin(th * 3.0f) / (1.0f - r_hyp * r_hyp + 0.05f);
        return { r_hyp * std::cos(th) * 1.2f, std::clamp(vortex_z * 0.25f, -1.0f, 1.0f), r_hyp * std::sin(th) * 1.2f };
    } else {
        const float u = static_cast<float>((index / scale) * scale) / static_cast<float>(kSampleCount);
        const float phase = 2.0f * 3.14159265f * u + 0.018f * time;
        const float ring = 0.60f + 0.23f * std::cos(3.0f * phase);
        return {
            ring * std::cos(2.0f * phase),
            0.30f * std::sin(3.0f * phase),
            ring * std::sin(2.0f * phase)
        };
    }
}

[[nodiscard]] inline Vec3f seedPoint(std::size_t index, float time, const SculptureProgramParams& params) {
    Vec3f pA = seedPointShape(index, time, params.outputMode, params.auxiliaryResolutionScale);
    Vec3f pB = seedPointShape(index, time, params.outputModeB, params.auxiliaryResolutionScale);
    return epi::lerp(pA, pB, params.topologyBlend);
}

[[nodiscard]] inline Vec3f paletteMorph(float u, float time, float orbitEnergy, float magnitude, float extraHueShift) {
    float h = 0.0f, c = 0.0f, l = 0.0f;
    const float t = smoothstep(0.0f, 1.0f, u);

    if (t < 0.5f) {
        const float f = t * 2.0f;
        h = std::lerp(45.0f, 320.0f, f);
        c = std::lerp(0.12f, 0.18f, f);
        l = std::lerp(0.75f, 0.65f, f);
    } else {
        const float f = (t - 0.5f) * 2.0f;
        h = std::lerp(320.0f, 220.0f, f);
        c = std::lerp(0.18f, 0.14f, f);
        l = std::lerp(0.65f, 0.70f, f);
    }

    h = std::fmod(h + orbitEnergy * 45.0f + magnitude * 15.0f + time * 12.0f + extraHueShift, 360.0f);
    if (h < 0.0f) h += 360.0f;

    c = std::clamp(c + orbitEnergy * 0.08f, 0.0f, 0.35f);
    return oklchToRgb(l, c, h);
}


// -----------------------------------------------------------------------------
// Educational Annotation: The Core Epimorphism Loop
// This function executes the primary mathematical feedback loop.
// 1. TRANSFORM: Maps the 3D/4D coordinate through a complex-plane mapping.
// 2. REDUCE: Folds infinite mathematical space back into a bounded domain.
// 3. SEED INJECTION: Computes an alpha mask based on spatial distance to inject the original coordinate back into the flow.
// 4. BLEND: Linearly interpolates between the reduced topology and the original seed.
// -----------------------------------------------------------------------------
struct OrbitResult {
    Vec3f feedback;
    float finalSeedWeight;
    float orbitEnergy;
    float orbitTrap;
};

// -----------------------------------------------------------------------------
// Educational Annotation: Seed Injection Field
// Computes the spatial alpha mask determining where the base seed coordinate
// overpowers the flowing mathematical feedback. This anchors the chaos.
// -----------------------------------------------------------------------------
[[nodiscard]] inline float evaluateSingleSeedWindow(const Vec3f& reduced, const Vec3f& seed, float time, const SculptureProgramParams& params, SeedType seedType) {
    if (!learn_mode::enabled(params.stageMask, learn_mode::Stage::Seed)) return 0.0f;
    const float width = std::max(params.seedWidth, 0.001f);
    const float distance = (reduced - seed).length();
    
    if (seedType == SeedType::Border) {
        float wx = (std::abs(reduced.x) + width - 1.0f) / width;
        float wy = (std::abs(reduced.y) + width - 1.0f) / width;
        float wz = (std::abs(reduced.z) + width - 1.0f) / width;
        float w = std::max(0.0f, std::max(wx, std::max(wy, wz)));
        return (1.0f - smoothstep(width, 2.0f * width, distance)) * (1.0f - smoothstep(width, 2.0f * width, distance));
    } 
    if (seedType == SeedType::WaveRing) {
        float ring = smoothstep(0.7f, 1.0f, std::cos(25.0f * distance - time * 5.0f));
        float fade = 1.0f - smoothstep(0.0f, 3.0f * width, distance);
        return ring * fade;
    } 
    if (seedType == SeedType::Cross) {
        const float distX = std::abs(reduced.x - seed.x);
        const float distY = std::abs(reduced.y - seed.y);
        const float distZ = std::abs(reduced.z - seed.z);
        const float crossDist = std::min(std::min(distX, distY), distZ);
        return 1.0f - smoothstep(width * 0.1f, width * 0.5f, crossDist);
    } 
    if (seedType == SeedType::Dipole) {
        const Vec3f diff = reduced - seed;
        const float r = std::max(diff.length(), 0.001f);
        const float cosTheta = diff.y / r;
        const float sin2Theta = 1.0f - cosTheta * cosTheta;
        const float phase = 15.0f * r - time * 6.0f;
        return std::clamp(sin2Theta * std::cos(phase) / (1.0f + r * 2.0f), 0.0f, 1.0f);
    } 
    if (seedType == SeedType::Airy) {
        const float r = std::max((reduced - seed).length(), 0.0001f);
        const float x = 15.0f * r;
        const float j1 = std::sin(x) / (x * x) - std::cos(x) / x; 
        const float airy = (2.0f * j1 / x);
        return std::clamp(airy * airy, 0.0f, 1.0f);
    } 
    if (seedType == SeedType::Lissajous) {
        const float lx = std::sin(3.0f * time + reduced.x * 5.0f);
        const float ly = std::sin(2.0f * time + reduced.y * 5.0f + kPi / 2.0f);
        const float lz = std::sin(4.0f * time + reduced.z * 5.0f);
        const float dist = (reduced - Vec3f{lx * 0.5f, ly * 0.5f, lz * 0.5f}).length();
        return 1.0f - smoothstep(0.0f, width, dist);
    }
    if (seedType == SeedType::Attractor) {
        const float sigma = 10.0f;
        const float rho = 28.0f;
        const float beta = 8.0f / 3.0f;
        const float dx = sigma * (reduced.y - reduced.x);
        const float dy = reduced.x * (rho - reduced.z) - reduced.y;
        const float dz = reduced.x * reduced.y - beta * reduced.z;
        const float flow = std::sqrt(dx*dx + dy*dy + dz*dz);
        const float dist = std::abs(flow * 0.05f - 1.0f);
        return 1.0f - smoothstep(0.0f, width * 2.0f, dist);
    }
    if (seedType == SeedType::Hopf) {
        const float r_xy = std::sqrt(reduced.x * reduced.x + reduced.y * reduced.y);
        const float d_core = std::abs(r_xy - 1.0f);
        const float d_hopf = std::sqrt(d_core * d_core + reduced.z * reduced.z);
        return 1.0f - smoothstep(0.0f, width * 2.5f, std::abs(d_hopf - 0.35f));
    }
    if (seedType == SeedType::HyperSphere4D) {
        const float r4 = std::sqrt(reduced.x * reduced.x + reduced.y * reduced.y + reduced.z * reduced.z + 0.04f);
        return 1.0f - smoothstep(0.0f, width * 2.0f, std::abs(r4 - 1.0f));
    }
    if (seedType == SeedType::CrossPolytope4D) {
        const float m4 = std::abs(reduced.x) + std::abs(reduced.y) + std::abs(reduced.z);
        return 1.0f - smoothstep(0.0f, width * 2.0f, std::abs(m4 - 1.2f));
    }
    if (seedType == SeedType::TorusKnotPQ) {
        // (p, q) = (3, 2) Trefoil / Torus Knot Fiber Streamline
        const float phi = std::atan2(reduced.y, reduced.x);
        const float r_xy = std::sqrt(reduced.x * reduced.x + reduced.y * reduced.y);
        const float knot_r = 1.0f + 0.35f * std::cos(3.0f * phi + time * 0.5f);
        const float knot_z = 0.35f * std::sin(2.0f * phi + time * 0.5f);
        const float dr = r_xy - knot_r;
        const float dz = reduced.z - knot_z;
        const float dist = std::sqrt(dr * dr + dz * dz);
        return 1.0f - smoothstep(0.0f, width * 2.2f, dist);
    }
    if (seedType == SeedType::ChladniCymatics) {
        // 3D Acoustic Standing-Wave Cymatic Nodal Field
        // Helmholtz Eigenmode: psi = cos(n*pi*x)*cos(m*pi*y)*cos(l*pi*z) - sin(m*pi*x)*sin(n*pi*y)*sin(l*pi*z)
        const float pi = 3.14159265358979323846f;
        const float n = 3.0f;
        const float m = 2.0f;
        const float l = 4.0f;
        const float drift = time * 0.25f;
        float psi = std::cos(n * pi * reduced.x + drift) * std::cos(m * pi * reduced.y - drift * 0.5f) * std::cos(l * pi * reduced.z)
                  - std::sin(m * pi * reduced.x - drift) * std::sin(n * pi * reduced.y + drift * 0.5f) * std::sin(l * pi * reduced.z);
        float nodalDist = std::abs(psi);
        return 1.0f - smoothstep(0.0f, width * 2.0f, nodalDist);
    }
    if (seedType == SeedType::TuringPerturbation) {
        // Turing Morphogenetic Seed Injection Field
        // Localized Gray-Scott spot perturbation: V-species concentration thresholding
        const float dist = (reduced - seed).length();
        const float spotWave = std::cos(8.0f * reduced.x) * std::cos(8.0f * reduced.y) * std::cos(8.0f * reduced.z);
        const float vConc = std::clamp(0.5f + 0.5f * spotWave - dist * 0.5f, 0.0f, 1.0f);
        const float turingMask = 1.0f - smoothstep(0.2f, 0.7f, std::abs(vConc - 0.45f));
        return std::clamp(turingMask * (1.0f - smoothstep(0.0f, width * 2.5f, dist)), 0.0f, 1.0f);
    }
    if (seedType == SeedType::JuliaQuaternionicSlice) {
        // 4D Quaternionic Julia slice: q_{n+1} = q_n^2 + C, sliced at w = 0.35
        float qx = reduced.x;
        float qy = reduced.y;
        float qz = reduced.z;
        float qw = 0.35f;
        float cx = -0.4f;
        float cy = 0.6f;
        float cz = 0.2f;
        float cw = -0.2f;
        float q2x = (qx*qx - qy*qy - qz*qz - qw*qw) + cx;
        float q2y = (2.0f*qx*qy) + cy;
        float q2z = (2.0f*qx*qz) + cz;
        float q2w = (2.0f*qx*qw) + cw;
        float lenQ2 = std::sqrt(q2x*q2x + q2y*q2y + q2z*q2z + q2w*q2w);
        float distJulia = std::abs(lenQ2 - 1.2f);
        return 1.0f - smoothstep(0.0f, width * 2.2f, distJulia);
    }
    if (seedType == SeedType::OctonionFibrationS7) {
        // 8D Unit Hypersphere S^7 Octonionic Seed with Hopf Fibration Projection
        float r = reduced.length();
        float th = std::atan2(reduced.y, reduced.x);
        float phi = std::acos(std::clamp(reduced.z / std::max(r, 1e-4f), -1.0f, 1.0f));
        Octonion s8(
            std::cos(th) * std::sin(phi), std::sin(th) * std::sin(phi),
            std::cos(phi), std::sin(2.0f * th) * 0.5f,
            std::cos(2.0f * phi) * 0.5f, std::sin(3.0f * th) * 0.25f,
            std::cos(3.0f * phi) * 0.25f, std::sin(time * 0.5f) * 0.35f
        );
        auto p3 = s8.normalized().projectTo3D();
        float p3Dist = std::sqrt(p3[0]*p3[0] + p3[1]*p3[1] + p3[2]*p3[2]);
        float seedDist = std::abs(p3Dist - 0.85f);
        return 1.0f - smoothstep(0.0f, width * 2.0f, seedDist);
    }
    if (seedType == SeedType::KuramotoSynchronization) {
        const float r = std::max(reduced.length(), 1e-4f);
        const float theta = std::atan2(reduced.y, reduced.x);
        const float phi = std::acos(std::clamp(reduced.z / r, -1.0f, 1.0f));
        const float globalMeanPhase = 2.5f; // Static mock for oracle
        const float sync = std::sin(3.0f * theta - globalMeanPhase) + std::sin(5.0f * phi - globalMeanPhase * 1.618f);
        const float seedDist = std::abs(sync - 0.5f);
        return 1.0f - smoothstep(0.0f, width * 2.0f, seedDist);
    }
    if (seedType == SeedType::FibonacciPhyllotaxis) {
        // Golden Angle Fibonacci Spiral Phyllotaxis Lattice: 13 / 21 harmonic ratio
        const float r = std::max(reduced.length(), 1e-4f);
        const float theta = std::atan2(reduced.y, reduced.x);
        const float phi = std::acos(std::clamp(reduced.z / r, -1.0f, 1.0f));
        const float phyllotaxis = std::sin(13.0f * theta + 21.0f * phi);
        const float dist = std::abs(phyllotaxis - 0.5f);
        return 1.0f - smoothstep(0.0f, width * 1.5f, dist);
    }
    if (seedType == SeedType::NavierStokesVortex) {
        float r2 = reduced.x*reduced.x + reduced.y*reduced.y;
        float u_x = -reduced.y / (r2 + 0.1f);
        float u_y = reduced.x / (r2 + 0.1f);
        float vort_mag = std::sqrt(u_x*u_x + u_y*u_y);
        float seedDist = std::abs(vort_mag - 1.5f - 0.2f*std::sin(time*2.0f));
        return 1.0f - smoothstep(0.0f, width * 2.5f, seedDist);
    }
    if (seedType == SeedType::HyperbolicTessellation) {
        float r = std::sqrt(reduced.x*reduced.x + reduced.y*reduced.y);
        float phi = std::atan2(reduced.y, reduced.x);
        float kr = std::tanh(r*2.0f);
        float poincare = kr * std::cos(7.0f*phi + time);
        float seedDist = std::abs(poincare - 0.5f);
        return 1.0f - smoothstep(0.0f, width * 2.0f, seedDist);
    }
    if (seedType == SeedType::CalabiYauManifold) {
        float z_c = reduced.x*reduced.x + reduced.y*reduced.y - reduced.z*reduced.z;
        float w_c = 2.0f*reduced.x*reduced.y;
        float manifold = std::pow(z_c*z_c + w_c*w_c, 0.2f);
        float seedDist = std::abs(manifold - 0.85f);
        return 1.0f - smoothstep(0.0f, width * 3.0f, seedDist);
    }
    if (seedType == SeedType::QuantumProbabilityCloud) {
        float r = reduced.length();
        float rho = r * std::exp(-r*0.5f);
        float Y_lm = std::abs(0.5f * std::sqrt(15.0f/3.14159265f) * reduced.x * reduced.y / (r*r+1e-5f));
        float psi2 = rho * rho * Y_lm * Y_lm;
        return std::clamp(psi2 * 10.0f, 0.0f, 1.0f) * (1.0f - smoothstep(width*1.5f, width*3.0f, r));
    }
    return 0.0f;
}

[[nodiscard]] inline float evaluateSeedWindow(const Vec3f& reduced, const Vec3f& seed, float time, const SculptureProgramParams& params) {
    float wA = evaluateSingleSeedWindow(reduced, seed, time, params, params.seedType);
    float wB = evaluateSingleSeedWindow(reduced, seed, time, params, params.seedTypeB);
    return wA * (1.0f - params.topologyBlend) + wB * params.topologyBlend;
}
inline float evaluatePareidoliaNoise(const Vec3f& p, float time) {
    // Simple pseudo-random organic noise for "Flesh / Cloud" textures
    float s = 2.5f;
    float t = time * 0.5f;
    return std::sin(p.x * s + t) * std::cos(p.y * s - t * 0.8f) * std::sin(p.z * s + t * 1.2f);
}

// -----------------------------------------------------------------------------
// Educational Annotation: WABI-SABI SYMMETRY GLITCH
// -----------------------------------------------------------------------------
// Introduces intentional Pareidolia and symmetry-breaking into the perfect math.
// Overloads the visual cortex (Salience Network) by destroying exact predictive coding.
// -----------------------------------------------------------------------------
inline Vec3f applyWabiSabiGlitch(const Vec3f& p, float u, float time, float orbitEnergy) {
    // Break the symmetry on one specific axis based on the orbit energy and time
    float glitchTrigger = std::sin(u * 15.0f + time * 2.0f);
    if (glitchTrigger > 0.85f) { // Only break symmetry locally
        float noise = evaluatePareidoliaNoise(p, time);
        return {
            p.x + noise * 0.15f * orbitEnergy,
            p.y - noise * 0.05f * orbitEnergy,
            p.z + noise * 0.10f * orbitEnergy
        };
    }
    return p;
}

[[nodiscard]] inline OrbitResult evaluateOrbit(const Vec3f& seed, float u, float time, const SculptureProgramParams& params, const Vec3f& prevFeedback) {
    const Vec3f decayedPrev = prevFeedback * std::clamp(params.temporalFade, 0.0f, 1.0f);
    Vec3f feedback = epi::lerp(seed, decayedPrev, params.temporalFeedbackBlend);
    if (!learn_mode::enabled(params.stageMask, learn_mode::Stage::FeedbackSample)) feedback = seed;
    if (params.temporalReset) feedback = seed;
    float finalSeedWeight = 0.0f;
    float orbitEnergy = 0.0f;
    float minTrapDist = 1000.0f;
    
    const auto evalStage = learn_mode::diagnostic(params.stageMask);
    const std::uint32_t iterationCount = std::clamp(params.feedbackIterations, 1U, 12U);
    for (std::uint32_t iteration = 0; iteration < iterationCount; ++iteration) {
        Vec3f reduced;
        if (params.enable4D != 0U || params.enable4DB != 0U) {
            Vec4f fb4 = {feedback.x, feedback.y, feedback.z, params.latentW};
            Vec4f t4 = liftFourDimensions(fb4, params, time, iteration);
            if (evalStage == learn_mode::DiagnosticStage::Transform) return { {t4.x, t4.y, t4.z}, 0.0f, 0.0f, 0.0f };
            Vec4f r4 = reduceDomain4(t4, params.reduction, params.stageMask);
            if (params.topologyBlend > 0.0f && params.reduction != params.reductionB) {
                const Vec4f r4_b = reduceDomain4(t4, params.reductionB, params.stageMask);
                const float tb = std::clamp(params.topologyBlend, 0.0f, 1.0f);
                r4 = {
                    r4.x + (r4_b.x - r4.x) * tb,
                    r4.y + (r4_b.y - r4.y) * tb,
                    r4.z + (r4_b.z - r4.z) * tb,
                    r4.w + (r4_b.w - r4.w) * tb
                };
            }
            const float stereoScale = 1.0f / (1.0f + std::clamp(r4.w * 0.35f, -0.7f, 0.7f));
            Vec3f red4 = {r4.x * stereoScale, r4.y * stereoScale, r4.z * stereoScale};

            if ((params.enable4D == 0U || params.enable4DB == 0U) && params.topologyBlend > 0.0f) {
                const Vec3f transformed3 = liftThreePlanes(feedback, params, time, iteration);
                Vec3f red3 = reduceDomain(transformed3, params.reduction, params.stageMask);
                const float tb = std::clamp(params.topologyBlend, 0.0f, 1.0f);
                reduced = (params.enable4D > 0U)
                    ? Vec3f{red4.x * (1.0f - tb) + red3.x * tb, red4.y * (1.0f - tb) + red3.y * tb, red4.z * (1.0f - tb) + red3.z * tb}
                    : Vec3f{red3.x * (1.0f - tb) + red4.x * tb, red3.y * (1.0f - tb) + red4.y * tb, red3.z * (1.0f - tb) + red4.z * tb};
            } else {
                reduced = red4;
            }
        } else {
            const Vec3f transformed = liftThreePlanes(feedback, params, time, iteration);
            if (evalStage == learn_mode::DiagnosticStage::Transform) return { transformed, 0.0f, 0.0f, 0.0f };
            reduced = reduceDomain(transformed, params.reduction, params.stageMask);
            if (params.topologyBlend > 0.0f && params.reduction != params.reductionB) {
                const Vec3f reduced_b = reduceDomain(transformed, params.reductionB, params.stageMask);
                const float tb = std::clamp(params.topologyBlend, 0.0f, 1.0f);
                reduced = {
                    reduced.x + (reduced_b.x - reduced.x) * tb,
                    reduced.y + (reduced_b.y - reduced.y) * tb,
                    reduced.z + (reduced_b.z - reduced.z) * tb
                };
            }
        }
        if (evalStage == learn_mode::DiagnosticStage::Reduce) return { reduced, 0.0f, 0.0f, 0.0f };
        orbitEnergy += (reduced - feedback).length() / static_cast<float>(iterationCount);

        // Hyper-Complex Orbit Trap: tracks proximity to Torus core & Axis plane
        const float r_xy = std::sqrt(reduced.x * reduced.x + reduced.y * reduced.y);
        const float trapTorus = std::abs(r_xy - 1.0f);
        const float trapCross = std::min(std::abs(reduced.x), std::abs(reduced.y));
        const float currentTrap = std::min(trapTorus, trapCross);
        minTrapDist = std::min(minTrapDist, currentTrap);

        float window = learn_mode::enabled(params.stageMask, learn_mode::Stage::Seed)
            ? evaluateSeedWindow(reduced, seed, time, params)
            : 0.0f;
        if (evalStage == learn_mode::DiagnosticStage::SeedWindow) return { {window, window, window}, 0.0f, 0.0f, 0.0f };

        finalSeedWeight = window;
        const float pulse = 1.0f + params.seedPulseAmplitude * std::sin(
            2.0f * kPi * (params.seedPulseFrequency * time + u
                          + 0.17f * static_cast<float>(iteration)));
        const float alpha = learn_mode::enabled(params.stageMask, learn_mode::Stage::Seed)
            ? std::clamp(params.seedBaseline + params.seedAlpha * finalSeedWeight * pulse, 0.0f, 0.95f)
            : 0.0f;
        if (learn_mode::enabled(params.stageMask, learn_mode::Stage::Blend)) {
            feedback = epi::lerp(reduced, seed, alpha);
        } else {
            feedback = reduced;
        }
        
        // Apply Wabi-Sabi Symmetry Glitch to force Pareidolia on high energy orbits
        feedback = applyWabiSabiGlitch(feedback, u, time, orbitEnergy);
    }
    return {feedback, finalSeedWeight, orbitEnergy, minTrapDist};
}


// -----------------------------------------------------------------------------
// Educational Annotation: KINETIC FLUID ADVECTION (RK2 Midpoint)
// -----------------------------------------------------------------------------
// Evaluates an analytical divergence-free curl vector field combined with 
// the fractal's own feedback displacement. Uses a 2nd-order Runge-Kutta 
// integration to transport the spatial coordinate along the flow field.
// -----------------------------------------------------------------------------
inline Vec3f applyKineticAdvection(
    const Vec3f& domain, 
    const Vec3f& feedback, 
    const Vec3f& seed, 
    const SculptureProgramParams& params,
    Vec3f& outFlowVelocity) noexcept 
{
    float audioAdvect = params.advectionSpeed * (1.0f + params.audioSpectrum[0] * 3.0f);
    float audioCurl = params.curlStrength * (1.0f + params.audioSpectrum[2] * 2.0f);

    if (audioCurl <= 0.0f && audioAdvect <= 0.0f) {
        outFlowVelocity = {0.0f, 0.0f, 0.0f};
        return domain;
    }

    auto evaluateCurlField = [](const Vec3f& p) -> Vec3f {
        const float pi = 3.14159265358979323846f;
        const float sx = std::sin(pi * p.x);
        const float cx = std::cos(pi * p.x);
        const float sy = std::sin(pi * p.y);
        const float cy = std::cos(pi * p.y);
        const float sz = std::sin(pi * p.z);
        const float cz = std::cos(pi * p.z);
        
        // Analytical curl components of potential Ψ
        float vx = -pi * (sx * sy + cx * cz);
        float vy = -pi * (sy * sz + cx * cy) + pi * std::sin(2.0f * pi * p.z);
        float vz = -pi * (sx * sz + cy * cz) - pi * std::sin(2.0f * pi * p.y);
        return {vx, vy, vz};
    };

    const Vec3f fbDisplacement = feedback - seed;
    const Vec3f curlV = evaluateCurlField(domain);
    outFlowVelocity = curlV * audioCurl + fbDisplacement * audioAdvect;

    // RK2 Midpoint Integration
    const float dt = 0.05f;
    const Vec3f midP = domain + outFlowVelocity * (0.5f * dt);
    const Vec3f midV = evaluateCurlField(midP) * audioCurl + (feedback - seed) * audioAdvect;
    
    outFlowVelocity = midV; // Update velocity to midpoint sample for accurate trailing
    return domain + midV * dt;
}

// -----------------------------------------------------------------------------
// Educational Annotation: ANALYTICAL SDF NORMALS
// -----------------------------------------------------------------------------
// Computes 3D surface normals via Central Finite Differences of the 
// fractal Signed Distance Field (SDF).
// -----------------------------------------------------------------------------
inline Vec3f computeAnalyticalNormal(
    const Vec3f& p_center, 
    float u, 
    float time, 
    float shapedMorph,
    const SculptureProgramParams& params) noexcept 
{
    auto evaluateSdf = [&](const Vec3f& p) -> float {
        OrbitResult orbit = evaluateOrbit(p, u, time, params, p);
        return epi::lerp(p, orbit.feedback, shapedMorph).length() - 1.0f;
    };
    
    const float eps = 0.005f;
    const float nx = evaluateSdf(p_center + Vec3f(eps, 0.0f, 0.0f)) - evaluateSdf(p_center - Vec3f(eps, 0.0f, 0.0f));
    const float ny = evaluateSdf(p_center + Vec3f(0.0f, eps, 0.0f)) - evaluateSdf(p_center - Vec3f(0.0f, eps, 0.0f));
    const float nz = evaluateSdf(p_center + Vec3f(0.0f, 0.0f, eps)) - evaluateSdf(p_center - Vec3f(0.0f, 0.0f, eps));
    
    Vec3f normal{nx, ny, nz};
    const float nLen = normal.length();
    if (nLen > 0.0f) {
        normal.x /= nLen; normal.y /= nLen; normal.z /= nLen;
    } else {
        normal = {0.0f, 1.0f, 0.0f};
    }
    return normal;
}

// -----------------------------------------------------------------------------
// Educational Annotation: EPIMORPHISM MASTER PIPELINE
// -----------------------------------------------------------------------------
// Core oracle sequence for spatial evaluation:
// 1. SEED INJECTION & TEMPORAL FEEDBACK
// 2. ORBIT EVALUATION (Pre-affine -> Inner map -> Post-affine -> Fold)
// 3. MORPHOLOGY & KINETIC ADVECTION
// 4. COLOR DOMAIN MAP & SDF NORMALS
// -----------------------------------------------------------------------------
[[nodiscard]] inline SculptureSample evaluateSample(std::size_t index, const SculptureSample* prevSample,
                               float time,
                               float morph,
                               const SculptureProgramParams& params,
                               const std::array<bool, gpu_abi::kVolumeMacroBlockCount>& activeBlocks,
                               std::vector<float>& spatialBuffer,
                               float deltaSeconds = 0.0166667f) {
    
    if (params.outputMode == 1 && params.outputModeB == 1 && index < 15625) {
        const std::size_t bx = (index % 25) / 5;
        const std::size_t by = ((index / 25) % 25) / 5;
        const std::size_t bz = (index / 625) / 5;
        const std::size_t b = bx + by * 5 + bz * 25;
        if (!activeBlocks[b]) return SculptureSample{};
    }

    const float u = static_cast<float>(index) / static_cast<float>(kSampleCount);

    // [STAGE 1: Seed Injection & Temporal Feedback]
    const Vec3f seed = seedPoint(index, time, params);
    Vec3f prevFeedback = seed;
    if (prevSample && params.temporalFeedbackBlend > 0.0f && !params.temporalReset) {
        prevFeedback = {prevSample->x, prevSample->y, prevSample->z};
    }

    // [STAGE 2: Orbit Evaluation (Math Core)]
    OrbitResult orbit = evaluateOrbit(seed, u, time, params, prevFeedback);
    Vec3f feedback = orbit.feedback;
    float finalSeedWeight = orbit.finalSeedWeight;
    float orbitEnergy = orbit.orbitEnergy;

    // [STAGE 3: Morphology & Kinetic Advection]
    const float shapedMorph = smoothstep(0.0f, 1.0f, std::clamp(morph, 0.0f, 1.0f));
    const Vec3f domain = epi::lerp(seed, feedback, shapedMorph);
    const float magnitude = domain.length();

    Vec3f flowVelocity{0.0f, 0.0f, 0.0f};
    Vec3f advectedDomain = applyKineticAdvection(domain, feedback, seed, params, flowVelocity);

    // [STAGE 4: Color Domain Map]
    const float angle = std::atan2(domain.z, domain.x) / (2.0f * kPi);
    float materialCoordinate = fract(
        angle + params.hueRotation + params.hueSpeed * time
        + 0.12f * std::log1p(magnitude) + 0.08f * orbitEnergy);
        
    if (!learn_mode::enabled(params.stageMask, learn_mode::Stage::Color)) materialCoordinate = 0.0f;

    // Fluid Injection
    float fluid = 0.0f;
    float emitScale = 1.0f;
    const auto evalStage = learn_mode::diagnostic(params.stageMask);
    if (evalStage == learn_mode::DiagnosticStage::FullPipeline && params.outputMode == 1 && params.outputModeB == 1) {
        if (std::abs(magnitude - 1.0f) > 0.12f) emitScale = 0.0f;
        if (!spatialBuffer.empty() && index >= spatialBuffer.size()) emitScale = 0.0f;
    }

    if (!spatialBuffer.empty() && index < spatialBuffer.size()) {
        float injection = (1.0f - smoothstep(0.0f, 0.15f, std::abs(magnitude - 1.0f))) * finalSeedWeight;
        spatialBuffer[index] = std::clamp(spatialBuffer[index] + injection * 0.35f, 0.0f, 2.0f);
        fluid = spatialBuffer[index];
    }

    // Populate Sample Data
    SculptureSample sample{};
    const float worldScale = std::clamp(params.worldScale, 0.25f, 2.0f) * emitScale;
    sample.x = worldScale * 2.45f * advectedDomain.x;
    sample.y = worldScale * 2.15f * advectedDomain.y;
    sample.z = worldScale * 2.45f * advectedDomain.z;
    
    const float trapDensity = std::clamp(1.0f - orbit.orbitTrap * 1.5f, 0.0f, 1.0f);
    const float audioRadiusBoost = params.audioSpectrum[3] * 1.5f;
    sample.radius = std::clamp(params.radiusScale, 0.25f, 2.0f) * std::clamp(
        0.58f + 0.34f * finalSeedWeight + 0.18f * orbitEnergy + fluid * 0.45f + trapDensity * 0.22f + audioRadiusBoost,
        0.48f, 3.5f) * emitScale;

    // SDF Normals
    if (params.outputMode == 1 && params.outputModeB == 1) {
        Vec3f normal = computeAnalyticalNormal(seed, u, time, shapedMorph, params);
        sample.nx = normal.x; sample.ny = normal.y; sample.nz = normal.z;
    } else {
        sample.nx = 0.0f; sample.ny = 1.0f; sample.nz = 0.0f;
    }

    // Packing Latent Coordinates for the Shader Exporter and Learn Mode Diagnostic Probes
    if (evalStage == learn_mode::DiagnosticStage::Transform) {
        // Stage 1: Raw Transform Space Coordinate Mapping
        sample.r = std::clamp((domain.x + 1.0f) * 0.5f, 0.0f, 1.0f);
        sample.g = std::clamp((domain.y + 1.0f) * 0.5f, 0.0f, 1.0f);
        sample.b = std::clamp((domain.z + 1.0f) * 0.5f, 0.0f, 1.0f);
        sample.a = 1.0f;
    } else if (evalStage == learn_mode::DiagnosticStage::Reduce) {
        // Stage 2: Domain Reduction Folded Space Mapping
        sample.r = std::clamp((feedback.x + 1.0f) * 0.5f, 0.0f, 1.0f);
        sample.g = std::clamp((feedback.y + 1.0f) * 0.5f, 0.0f, 1.0f);
        sample.b = std::clamp((feedback.z + 1.0f) * 0.5f, 0.0f, 1.0f);
        sample.a = 1.0f;
    } else if (evalStage == learn_mode::DiagnosticStage::SeedWindow) {
        // Stage 3: Seed Window Injection Density Mask
        sample.r = std::clamp(finalSeedWeight, 0.0f, 1.0f);
        sample.g = std::clamp(finalSeedWeight, 0.0f, 1.0f);
        sample.b = std::clamp(finalSeedWeight, 0.0f, 1.0f);
        sample.a = 1.0f;
    } else if (evalStage == learn_mode::DiagnosticStage::MaterialCoordinate) {
        sample.r = materialCoordinate; sample.g = materialCoordinate;
        sample.b = materialCoordinate; sample.a = 0.0f;
    } else if (evalStage == learn_mode::DiagnosticStage::OrbitEnergy) {
        sample.r = std::clamp(orbitEnergy * 0.5f, 0.0f, 1.0f);
        sample.g = std::clamp(orbitEnergy * 0.5f, 0.0f, 1.0f);
        sample.b = std::clamp(orbitEnergy * 0.5f, 0.0f, 1.0f);
        sample.a = 1.0f;
    } else if (evalStage == learn_mode::DiagnosticStage::OrbitTrap) {
        sample.r = trapDensity; sample.g = trapDensity;
        sample.b = trapDensity; sample.a = 1.0f;
    } else if (learn_mode::enabled(params.stageMask, learn_mode::Stage::Color)) {
        if (params.colorEffect == ColorEffectType::KineticVelocity && flowVelocity.length() > 0.0f) {
            Vec3f velN = flowVelocity.normalized();
            Vec3f rgb = velN * 0.5f + Vec3f{0.5f, 0.5f, 0.5f};
            float vMag = std::clamp(flowVelocity.length() * 2.0f, 0.0f, 1.0f);
            rgb = epi::lerp(Vec3f{0.1f, 0.1f, 0.1f}, rgb, vMag);
            sample.r = rgb.x; sample.g = rgb.y; sample.b = rgb.z; sample.a = 1.0f;
        } else if (params.colorEffect != ColorEffectType::StandardHSV) {
            Vec3f rgb = paletteEvaluate(
                params.colorEffect,
                materialCoordinate,
                orbitEnergy,
                magnitude,
                time,
                params.chromaBreath,
                params.lumaDrift,
                params.hueRotation
            );
            sample.r = rgb.x; sample.g = rgb.y; sample.b = rgb.z; sample.a = materialCoordinate;
        } else {
            sample.r = materialCoordinate;
            sample.g = orbitEnergy;
            sample.b = magnitude;
            sample.a = std::clamp(materialCoordinate, 0.0f, 1.0f);
        }
    } else {
        sample.r = materialCoordinate;
        sample.g = orbitEnergy;
        sample.b = magnitude;
        sample.a = std::clamp(materialCoordinate, 0.0f, 1.0f);
    }
    sample.nw = finalSeedWeight;

    // Velocity / Fluid decay
    if (prevSample && !params.temporalReset) {
        float rawVx = 0.0f, rawVy = 0.0f, rawVz = 0.0f;
        if (deltaSeconds > 1e-5f) {
            rawVx = (sample.x - prevSample->x) / deltaSeconds;
            rawVy = (sample.y - prevSample->y) / deltaSeconds;
            rawVz = (sample.z - prevSample->z) / deltaSeconds;
        }
        if (flowVelocity.length() > 0.0f) {
            float visc = std::clamp(params.kineticViscosity, 0.0f, 1.0f);
            sample.vx = (1.0f - visc) * flowVelocity.x + visc * prevSample->vx * params.trailDecay;
            sample.vy = (1.0f - visc) * flowVelocity.y + visc * prevSample->vy * params.trailDecay;
            sample.vz = (1.0f - visc) * flowVelocity.z + visc * prevSample->vz * params.trailDecay;
        } else {
            sample.vx = rawVx; sample.vy = rawVy; sample.vz = rawVz;
        }
    } else if (flowVelocity.length() > 0.0f) {
        sample.vx = flowVelocity.x; sample.vy = flowVelocity.y; sample.vz = flowVelocity.z;
    }

    float flowSpeed = std::sqrt(sample.vx * sample.vx + sample.vy * sample.vy + sample.vz * sample.vz);
    sample.vw = (finalSeedWeight > 0.8f) ? 2.0f : flowSpeed;
    sample.materialRole = (flowSpeed > 0.5f) ? 2U : 1U;

    if (!std::isfinite(sample.x) || !std::isfinite(sample.y) || !std::isfinite(sample.z)) {
        sample.x = 0.0f;
        sample.y = 0.0f;
        sample.z = 0.0f;
    }
    if (!std::isfinite(sample.r) || !std::isfinite(sample.g) || !std::isfinite(sample.b) || !std::isfinite(sample.a)) {
        sample.r = 0.0f;
        sample.g = 0.0f;
        sample.b = 0.0f;
        sample.a = 1.0f;
    }

    return sample;
}



} // namespace oracle
} // namespace epi_sculpt
#endif // EPI_SCULPTURE_MATH_ORACLE_H
