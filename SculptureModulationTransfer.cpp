// Shared pure transfer mathematics for runtime modulation and authoring study.
// This translation unit owns no oscillator clocks, routes or native resources.
#include "SculptureModulationEngine.h"

namespace epi_sculpt {

float ModulationOscillator::applyTransferCurve(float in, TransferCurve curve) noexcept {
    switch (curve) {
    case TransferCurve::Linear:
        return in;
    case TransferCurve::QuadraticEaseIn: {
        const float sign = (in >= 0.0f) ? 1.0f : -1.0f;
        return sign * in * in;
    }
    case TransferCurve::QuadraticEaseOut: {
        const float c = std::clamp(in, -1.0f, 1.0f);
        const float sign = (c >= 0.0f) ? 1.0f : -1.0f;
        const float absC = std::abs(c);
        return sign * (1.0f - (1.0f - absC) * (1.0f - absC));
    }
    case TransferCurve::Smoothstep: {
        // Maps [-1, 1] to smoothstep [-1, 1]
        const float u = std::clamp(in * 0.5f + 0.5f, 0.0f, 1.0f);
        const float s = u * u * (3.0f - 2.0f * u);
        return s * 2.0f - 1.0f;
    }
    case TransferCurve::Smootherstep: {
        const float u = std::clamp(in * 0.5f + 0.5f, 0.0f, 1.0f);
        const float s = u * u * u * (u * (u * 6.0f - 15.0f) + 10.0f);
        return s * 2.0f - 1.0f;
    }
    case TransferCurve::BipolarCubic: {
        const float c = std::clamp(in, -1.0f, 1.0f);
        return c * c * c;
    }
    case TransferCurve::Quantize8Step: {
        return std::round(in * 8.0f) / 8.0f;
    }
    case TransferCurve::Invert:
        return -in;
    case TransferCurve::Sigmoid:
        return std::tanh(in * 2.0f);
    case TransferCurve::SinusoidalFold:
        return std::sin(in * 3.14159265f);
    case TransferCurve::ChebyshevT3: {
        const float c = std::clamp(in, -1.0f, 1.0f);
        return 4.0f * c * c * c - 3.0f * c;
    }
    }
    return in;
}

} // namespace epi_sculpt
