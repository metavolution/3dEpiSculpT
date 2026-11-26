#pragma once

#include "QuaternionAlgebra.h"

#include <array>
#include <cmath>
#include <algorithm>

namespace epi_sculpt {

// -----------------------------------------------------------------------------
// Educational Annotation: 8D Hypercomplex Octonion Algebra (O) & Fano Plane
// -----------------------------------------------------------------------------
// The octonions O form the largest normed division algebra over the reals (R, C, H, O).
// An octonion is defined by 8 real coordinates:
//   x = e0 + e1*i1 + e2*i2 + e3*i3 + e4*i4 + e5*i5 + e6*i6 + e7*i7
// where e0 is the real unit, and e1..e7 are imaginary units (ei^2 = -1).
//
// Octonionic multiplication is non-commutative AND non-associative.
// Via the Cayley-Dickson construction over quaternions:
//   X = (A, B), Y = (C, D) where A, B, C, D in H
//   X * Y = (A * C - D.conjugate() * B, D * A + B * C.conjugate())
//
// The 14-dimensional exceptional Lie group G2 is the automorphism group of O:
//   G2 = Aut(O) = { g in SO(8) : g(x * y) = g(x) * g(y) for all x, y in O }
// -----------------------------------------------------------------------------
struct Octonion {
    float e0 = 0.0f; // Real part
    float e1 = 0.0f; // i1
    float e2 = 0.0f; // i2
    float e3 = 0.0f; // i3
    float e4 = 0.0f; // i4
    float e5 = 0.0f; // i5
    float e6 = 0.0f; // i6
    float e7 = 0.0f; // i7

    constexpr Octonion() noexcept = default;
    constexpr Octonion(float e0_, float e1_, float e2_, float e3_,
                       float e4_, float e5_, float e6_, float e7_) noexcept
        : e0(e0_), e1(e1_), e2(e2_), e3(e3_),
          e4(e4_), e5(e5_), e6(e6_), e7(e7_) {}

    constexpr Octonion(const Quaternion& a, const Quaternion& b) noexcept
        : e0(a.w), e1(a.x), e2(a.y), e3(a.z),
          e4(b.w), e5(b.x), e6(b.y), e7(b.z) {}

    [[nodiscard]] constexpr Quaternion lo() const noexcept {
        return Quaternion(e0, e1, e2, e3);
    }

    [[nodiscard]] constexpr Quaternion hi() const noexcept {
        return Quaternion(e4, e5, e6, e7);
    }

    [[nodiscard]] constexpr Octonion operator+(const Octonion& rhs) const noexcept {
        return Octonion(
            e0 + rhs.e0, e1 + rhs.e1, e2 + rhs.e2, e3 + rhs.e3,
            e4 + rhs.e4, e5 + rhs.e5, e6 + rhs.e6, e7 + rhs.e7
        );
    }

    [[nodiscard]] constexpr Octonion operator-(const Octonion& rhs) const noexcept {
        return Octonion(
            e0 - rhs.e0, e1 - rhs.e1, e2 - rhs.e2, e3 - rhs.e3,
            e4 - rhs.e4, e5 - rhs.e5, e6 - rhs.e6, e7 - rhs.e7
        );
    }

    [[nodiscard]] constexpr Octonion operator*(float s) const noexcept {
        return Octonion(
            e0 * s, e1 * s, e2 * s, e3 * s,
            e4 * s, e5 * s, e6 * s, e7 * s
        );
    }

    // Cayley-Dickson Multiplication: (A, B) * (C, D) = (AC - D* B, DA + B C*)
    [[nodiscard]] constexpr Octonion operator*(const Octonion& rhs) const noexcept {
        const Quaternion A = lo();
        const Quaternion B = hi();
        const Quaternion C = rhs.lo();
        const Quaternion D = rhs.hi();

        const Quaternion resLo = (A * C) - (D.conjugate() * B);
        const Quaternion resHi = (D * A) + (B * C.conjugate());

        return Octonion(resLo, resHi);
    }

    [[nodiscard]] constexpr Octonion conjugate() const noexcept {
        return Octonion(e0, -e1, -e2, -e3, -e4, -e5, -e6, -e7);
    }

    [[nodiscard]] constexpr float normSq() const noexcept {
        return e0 * e0 + e1 * e1 + e2 * e2 + e3 * e3 +
               e4 * e4 + e5 * e5 + e6 * e6 + e7 * e7;
    }

    [[nodiscard]] float norm() const noexcept {
        return std::sqrt(normSq());
    }

    [[nodiscard]] Octonion normalized() const noexcept {
        const float n = norm();
        if (n <= 1.0e-7f) return Octonion(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        return *this * (1.0f / n);
    }

    [[nodiscard]] Octonion inverse() const noexcept {
        const float nsq = normSq();
        if (nsq <= 1.0e-9f) return Octonion();
        return conjugate() * (1.0f / nsq);
    }

    // Octonionic Square: X^2 = (e0^2 - |v|^2, 2 * e0 * v)
    [[nodiscard]] constexpr Octonion square() const noexcept {
        const float vSq = e1 * e1 + e2 * e2 + e3 * e3 +
                          e4 * e4 + e5 * e5 + e6 * e6 + e7 * e7;
        return Octonion(
            e0 * e0 - vSq,
            2.0f * e0 * e1,
            2.0f * e0 * e2,
            2.0f * e0 * e3,
            2.0f * e0 * e4,
            2.0f * e0 * e5,
            2.0f * e0 * e6,
            2.0f * e0 * e7
        );
    }

    // Octonionic Exponential: exp(e0 + v) = exp(e0) * (cos(|v|) + (v/|v|) * sin(|v|))
    [[nodiscard]] static Octonion exp(const Octonion& x) noexcept {
        const float expE0 = std::exp(x.e0);
        const float vNorm = std::sqrt(
            x.e1 * x.e1 + x.e2 * x.e2 + x.e3 * x.e3 +
            x.e4 * x.e4 + x.e5 * x.e5 + x.e6 * x.e6 + x.e7 * x.e7
        );
        if (vNorm < 1.0e-7f) {
            return Octonion(expE0, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        }
        const float coeff = expE0 * std::sin(vNorm) / vNorm;
        return Octonion(
            expE0 * std::cos(vNorm),
            x.e1 * coeff, x.e2 * coeff, x.e3 * coeff,
            x.e4 * coeff, x.e5 * coeff, x.e6 * coeff, x.e7 * coeff
        );
    }

    // Non-Associative Associator: [X, Y, Z] = (X * Y) * Z - X * (Y * Z)
    [[nodiscard]] static constexpr Octonion associator(
        const Octonion& x, const Octonion& y, const Octonion& z
    ) noexcept {
        return (x * y) * z - x * (y * z);
    }

    // 1-Parameter G2 Automorphism Generator:
    // Exactly preserves the octonionic multiplication table for any theta:
    //   g(X * Y) == g(X) * g(Y)
    [[nodiscard]] static Octonion rotateG2(const Octonion& in, float theta) noexcept {
        const float c1 = std::cos(2.0f * theta);
        const float s1 = std::sin(2.0f * theta);
        const float c2 = std::cos(theta);
        const float s2 = std::sin(theta);
        
        Octonion out = in;
        // Primary Fano circle (e1, e2) rotation by 2*theta
        out.e1 = in.e1 * c1 + in.e2 * s1;
        out.e2 = -in.e1 * s1 + in.e2 * c1;
        out.e3 = in.e3;
        // Associated triad (e4, e7) rotation by theta
        out.e4 = in.e4 * c2 - in.e7 * s2;
        out.e7 = in.e4 * s2 + in.e7 * c2;
        // Associated triad (e5, e6) rotation by theta
        out.e5 = in.e5 * c2 + in.e6 * s2;
        out.e6 = -in.e5 * s2 + in.e6 * c2;
        return out;
    }

    // 8D -> 3D Octonionic Hopf-like Fibration S^7 -> S^4 -> S^2 Projection
    [[nodiscard]] std::array<float, 3> projectTo3D() const noexcept {
        const float nsq = normSq();
        const float denom = std::max(nsq + 0.1f, 1e-5f);
        
        // Hopf S^7 -> S^4 -> R^3 projection
        const float pX = 2.0f * (e0 * e1 + e2 * e3 + e4 * e5 + e6 * e7) / denom;
        const float pY = 2.0f * (e0 * e2 - e1 * e3 + e4 * e6 - e5 * e7) / denom;
        const float pZ = ((e0 * e0 + e1 * e1 + e2 * e2 + e3 * e3) -
                          (e4 * e4 + e5 * e5 + e6 * e6 + e7 * e7)) / denom;
        return {pX, pY, pZ};
    }
};

} // namespace epi_sculpt
