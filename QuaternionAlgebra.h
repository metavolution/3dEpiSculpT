#pragma once

#include <cmath>
#include <array>
#include <algorithm>

namespace epi_sculpt {

// -----------------------------------------------------------------------------
// Educational Annotation: 4D Hypercomplex Quaternion Algebra (H)
// Quaternions extend complex numbers into 4D: q = w + x*i + y*j + z*k
// where i^2 = j^2 = k^2 = i*j*k = -1, and multiplication is non-commutative:
// i*j = k, j*i = -k, j*k = i, k*j = -i, k*i = j, i*k = -j.
// -----------------------------------------------------------------------------
struct Quaternion {
    float w = 0.0f; // Real part
    float x = 0.0f; // i
    float y = 0.0f; // j
    float z = 0.0f; // k

    constexpr Quaternion() noexcept = default;
    constexpr Quaternion(float w_, float x_, float y_, float z_) noexcept
        : w(w_), x(x_), y(y_), z(z_) {}

    [[nodiscard]] constexpr Quaternion operator+(const Quaternion& rhs) const noexcept {
        return Quaternion(w + rhs.w, x + rhs.x, y + rhs.y, z + rhs.z);
    }

    [[nodiscard]] constexpr Quaternion operator-(const Quaternion& rhs) const noexcept {
        return Quaternion(w - rhs.w, x - rhs.x, y - rhs.y, z - rhs.z);
    }

    [[nodiscard]] constexpr Quaternion operator*(float s) const noexcept {
        return Quaternion(w * s, x * s, y * s, z * s);
    }

    // Hamilton Product (q1 * q2)
    [[nodiscard]] constexpr Quaternion operator*(const Quaternion& rhs) const noexcept {
        return Quaternion(
            w * rhs.w - x * rhs.x - y * rhs.y - z * rhs.z,
            w * rhs.x + x * rhs.w + y * rhs.z - z * rhs.y,
            w * rhs.y - x * rhs.z + y * rhs.w + z * rhs.x,
            w * rhs.z + x * rhs.y - y * rhs.x + z * rhs.w
        );
    }

    [[nodiscard]] constexpr Quaternion conjugate() const noexcept {
        return Quaternion(w, -x, -y, -z);
    }

    [[nodiscard]] constexpr float normSq() const noexcept {
        return w * w + x * x + y * y + z * z;
    }

    [[nodiscard]] float norm() const noexcept {
        return std::sqrt(normSq());
    }

    [[nodiscard]] Quaternion normalized() const noexcept {
        const float n = norm();
        if (n <= 1.0e-7f) return Quaternion(1.0f, 0.0f, 0.0f, 0.0f);
        return *this * (1.0f / n);
    }

    [[nodiscard]] Quaternion inverse() const noexcept {
        const float nsq = normSq();
        if (nsq <= 1.0e-9f) return Quaternion(0.0f, 0.0f, 0.0f, 0.0f);
        return conjugate() * (1.0f / nsq);
    }

    // Quaternion Square: q^2 = (w^2 - |v|^2, 2*w*v)
    [[nodiscard]] constexpr Quaternion square() const noexcept {
        const float vSq = x * x + y * y + z * z;
        return Quaternion(w * w - vSq, 2.0f * w * x, 2.0f * w * y, 2.0f * w * z);
    }

    // Quaternion Exponential: exp(w + v) = exp(w) * (cos(|v|) + (v/|v|) * sin(|v|))
    [[nodiscard]] static Quaternion exp(const Quaternion& q) noexcept {
        const float expW = std::exp(q.w);
        const float vNorm = std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z);
        if (vNorm < 1.0e-7f) {
            return Quaternion(expW, 0.0f, 0.0f, 0.0f);
        }
        const float coeff = expW * std::sin(vNorm) / vNorm;
        return Quaternion(expW * std::cos(vNorm), q.x * coeff, q.y * coeff, q.z * coeff);
    }

    // Quaternion Sine
    [[nodiscard]] static Quaternion sin(const Quaternion& q) noexcept {
        const float vNorm = std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z);
        if (vNorm < 1.0e-7f) {
            return Quaternion(std::sin(q.w), 0.0f, 0.0f, 0.0f);
        }
        const float coshV = std::cosh(vNorm);
        const float sinhV = std::sinh(vNorm);
        const float coeff = std::cos(q.w) * sinhV / vNorm;
        return Quaternion(std::sin(q.w) * coshV, q.x * coeff, q.y * coeff, q.z * coeff);
    }

    // 4D Quaternion Julia step with running derivative:
    // q_{n+1} = q_n^2 + c
    // q'_{n+1} = 2 * q_n * q'_n
    static void juliaStep(Quaternion& q, Quaternion& qPrime, const Quaternion& c) noexcept {
        qPrime = (q * 2.0f) * qPrime;
        q = q.square() + c;
    }

    // Analytical Distance Estimator for 4D Quaternion Julia:
    // dist(p) = 0.5 * |q_k| * ln(|q_k|) / |q'_k|
    [[nodiscard]] static float distanceEstimate(const Quaternion& q, const Quaternion& qPrime) noexcept {
        const float qNorm = q.norm();
        const float qpNorm = qPrime.norm();
        if (qpNorm <= 1.0e-7f || qNorm <= 1.0f) return 0.0f;
        return 0.5f * qNorm * std::log(qNorm) / qpNorm;
    }
};

} // namespace epi_sculpt
