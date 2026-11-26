#pragma once

#include <vector>
#include <functional>
#include <unordered_map>
#include <string>
#include <algorithm>
#include <random>
#include <cmath>

#include "SculptureGpuAbi.h"
#include "SculptureMutator.h"
#include "SculptureParameterRegistry.h"
#include "SculptureAudio.h"
#include "SculptureAudioColorSync.h"

namespace epi_sculpt {

// -----------------------------------------------------------------------------
// Educational Annotation: Functional Signal Routing Architecture
// -----------------------------------------------------------------------------
// Hardcoding LFOs and Audio inputs to specific shader parameters limits the 
// visualizer's flexibility. The Modulator Graph decouples signal generation 
// (Sources) from parameter assignment (Targets) via a routing matrix.
// By using std::function, we can bind any class method, lambda, or static
// function without forcing tight coupling or inheritance.

using SignalSource = std::function<float(float /* time */)>;
using SignalTarget = std::function<void(SculptureProgramParams&, float /* signal */)>;

struct SignalLink {
    SignalSource source;
    SignalTarget target;
    float amount;
};

// -----------------------------------------------------------------------------
// Educational Annotation: Modulation Router (Data-Driven Signal Graph)
// -----------------------------------------------------------------------------
// This decouples audio/MIDI analysis from parameter updates. Instead of hardcoding 
// "if (kick) then increase width", the engine exposes a routing matrix. 
// A JSON string like "audioBass -> preScale : 0.5" is parsed dynamically, 
// allowing technical artists to build extremely complex interconnected 
// reactive behaviors without touching C++.


class SculptureParametricMotionOrchestrator {
public:
    struct RoseParams {
        float speed = 0.1f;
        float amplitude = 0.5f;
        int petals = 4;
        float phase = 0.0f;
    };

    struct SpirographParams {
        float speed = 0.05f;
        float R = 1.0f;
        float r = 0.3f;
        float d = 0.4f;
    };

    struct LissajousParams {
        float speedX = 0.1f;
        float speedY = 0.2f;
        float ampX = 0.4f;
        float ampY = 0.4f;
        float phaseDelta = 1.57079632679f;
    };

    struct HopfParams {
        float speed = 0.1f;
        float radius = 0.35f;
    };

    struct BreathingParams {
        float speed = 0.25f;
        float minAmp = 0.2f;
        float maxAmp = 0.8f;
    };

    static std::pair<float, float> evaluateRoseCurve(float t, const RoseParams& p) noexcept {
        const float theta = p.speed * t * 6.28318530718f + p.phase;
        const float r = p.amplitude * std::cos(static_cast<float>(p.petals) * theta);
        return {r * std::cos(theta), r * std::sin(theta)};
    }

    static std::pair<float, float> evaluateRoseCurve(float t) noexcept {
        return evaluateRoseCurve(t, RoseParams{.speed = 0.1f, .amplitude = 0.35f, .petals = 4, .phase = 0.0f});
    }

    static std::pair<float, float> evaluateSpirograph(float t, const SpirographParams& p) noexcept {
        const float theta = p.speed * t * 6.28318530718f;
        const float diff = p.R - p.r;
        const float ratio = (p.r != 0.0f) ? (diff / p.r) : 1.0f;
        const float x = diff * std::cos(theta) + p.d * std::cos(ratio * theta);
        const float y = diff * std::sin(theta) - p.d * std::sin(ratio * theta);
        return {x, y};
    }

    static std::pair<float, float> evaluateSpirograph(float t) noexcept {
        return evaluateSpirograph(t, SpirographParams{});
    }

    static std::pair<float, float> evaluateLissajous(float t, const LissajousParams& p) noexcept {
        const float x = p.ampX * std::sin(p.speedX * t * 6.28318530718f + p.phaseDelta);
        const float y = p.ampY * std::sin(p.speedY * t * 6.28318530718f);
        return {x, y};
    }

    static std::pair<float, float> evaluateLissajous(float t) noexcept {
        return evaluateLissajous(t, LissajousParams{});
    }

    static float evaluateHopfOrbit(float t, const HopfParams& p) noexcept {
        return p.radius * std::sin(p.speed * t * 6.28318530718f);
    }

    static float evaluateHopfOrbit(float t) noexcept {
        return evaluateHopfOrbit(t, HopfParams{});
    }

    static float evaluateBreathingLFO(float t, const BreathingParams& p) noexcept {
        const float mid = 0.5f * (p.minAmp + p.maxAmp);
        const float halfRange = 0.5f * (p.maxAmp - p.minAmp);
        return mid + halfRange * std::sin(p.speed * t * 6.28318530718f);
    }

    static float evaluateBreathingLFO(float t) noexcept {
        return evaluateBreathingLFO(t, BreathingParams{});
    }
};

class ModulationRouter {
public:
    void addLink(SignalSource source, SignalTarget target, float amount = 1.0f) {
        links_.push_back({std::move(source), std::move(target), amount});
    }

    void clearLinks() noexcept {
        links_.clear();
    }

    [[nodiscard]] std::size_t linkCount() const noexcept {
        return links_.size();
    }

    [[nodiscard]] static SignalTarget resolveTarget(std::string_view name) {
        if (name == "preOffset" || name == "pre.offsetReal") {
            return [](SculptureProgramParams& p, float v) { p.pre.offsetReal += v; };
        }
        if (name == "latentW") {
            return [](SculptureProgramParams& p, float v) { p.latentW += v; };
        }
        if (name == "preScale" || name == "pre.scaleReal") {
            return [](SculptureProgramParams& p, float v) { p.pre.scaleReal += v; };
        }
        if (name == "topologyBlend") {
            return [](SculptureProgramParams& p, float v) { p.topologyBlend = std::clamp(p.topologyBlend + v, 0.0f, 1.0f); };
        }
        if (name == "hueRotation") {
            return [](SculptureProgramParams& p, float v) { p.hueRotation = std::fmod(p.hueRotation + v, 1.0f); };
        }
        if (name == "advectionSpeed") {
            return [](SculptureProgramParams& p, float v) { p.advectionSpeed += v; };
        }
        if (name == "curlStrength") {
            return [](SculptureProgramParams& p, float v) { p.curlStrength += v; };
        }
        if (name == "seedWidth") {
            return [](SculptureProgramParams& p, float v) { p.seedWidth += v; };
        }
        if (name == "seedAlpha") {
            return [](SculptureProgramParams& p, float v) { p.seedAlpha += v; };
        }
        if (name == "seedPulseAmplitude" || name == "pulseAmplitude") {
            return [](SculptureProgramParams& p, float v) { p.seedPulseAmplitude += v; };
        }
        if (name == "volumetricDensityScale" || name == "volumetricDensity") {
            return [](SculptureProgramParams& p, float v) { p.volumetricDensityScale += v; };
        }
        if (name == "filmThickness") {
            return [](SculptureProgramParams& p, float v) { p.filmThickness += v; };
        }
        return [](SculptureProgramParams&, float) {};
    }

    void apply(SculptureProgramParams& params, float time) const {
        for (const auto& link : links_) {
            if (link.source && link.target) {
                const float val = link.source(time) * link.amount;
                link.target(params, val);
            }
        }
    }

private:
    std::vector<SignalLink> links_;
};




} // namespace epi_sculpt
