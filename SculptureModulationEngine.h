#pragma once

#include "SculptureProgramSchema.h"
#include "SculptureAudioSignals.h"
#include "SculptureModulationCurves.h"

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <optional>
#include <string_view>
#include <utility>
#include <cmath>
#include <algorithm>
#include <array>

namespace epi_sculpt {

// -----------------------------------------------------------------------------
// Educational Annotation: UNIVERSAL MODULATION SIGNAL ENGINE
// -----------------------------------------------------------------------------
// Visual parameters in the epimorphism loop (such as scale, offset, rotation,
// topology blend, and density) are driven dynamically through modular sources.
// This engine provides:
// 1. Multi-Waveform Continuous Oscillators (Sine, Triangle, Saw, Square, Chaos, Perlin)
// 2. Non-Linear Transfer & Easing Functions (Smoothstep, Bezier, Quantized)
// 3. Host-supplied audio envelope/resonance values (no device acquisition)
// 4. Data-Driven Dynamic Signal Matrix (Named string parser & zero-alloc dispatch)
// -----------------------------------------------------------------------------

enum class ModulationWaveform : std::uint8_t {
    Sine = 0,
    Cosine,
    Triangle,
    SawtoothUp,
    SawtoothDown,
    Square,
    PulseWidth,
    PerlinNoise,
    LorenzChaosX,
    LorenzChaosZ,
    SampleAndHold
};

struct OscillatorConfig {
    ModulationWaveform waveform = ModulationWaveform::Sine;
    float frequencyHz = 0.1f;
    float amplitude = 1.0f;
    float phaseOffsetRad = 0.0f;
    float pulseWidth = 0.5f; // For PulseWidth waveform [0.01, 0.99]
    float bias = 0.0f;
};

// -----------------------------------------------------------------------------
// Continuous Multi-Waveform Signal Generator
// -----------------------------------------------------------------------------
class ModulationOscillator {
public:
    explicit ModulationOscillator(const OscillatorConfig& config = {})
        : config_(config) {}

    void setConfig(const OscillatorConfig& config) noexcept { config_ = config; }
    [[nodiscard]] const OscillatorConfig& config() const noexcept { return config_; }

    [[nodiscard]] float evaluate(float timeSeconds) const noexcept;

    // Evaluates shaping curve on normalized [0, 1] or bipolar [-1, 1] inputs
    [[nodiscard]] static float applyTransferCurve(float in, TransferCurve curve) noexcept;

private:
    OscillatorConfig config_;
};

// -----------------------------------------------------------------------------
// 3D Strange Attractor Chaos Signal Generator (Lorenz system)
// -----------------------------------------------------------------------------
class ChaosOscillator {
public:
    struct State {
        float x = 0.1f;
        float y = 0.0f;
        float z = 0.0f;
    };

    ChaosOscillator(float sigma = 10.0f, float rho = 28.0f, float beta = 8.0f / 3.0f, float stepSize = 0.01f)
        : sigma_(sigma), rho_(rho), beta_(beta), dt_(stepSize) {}

    void step(float simDeltaTime) noexcept;
    void reset() noexcept { state_ = {0.1f, 0.0f, 0.0f}; }

    [[nodiscard]] float valueX() const noexcept { return state_.x * 0.05f; } // Scaled to ~[-1, 1]
    [[nodiscard]] float valueY() const noexcept { return state_.y * 0.05f; }
    [[nodiscard]] float valueZ() const noexcept { return (state_.z - 25.0f) * 0.05f; }

private:
    float sigma_;
    float rho_;
    float beta_;
    float dt_;
    State state_;
};

// -----------------------------------------------------------------------------
// Modulation Matrix Link Definition
// -----------------------------------------------------------------------------

enum class ModulationSourceId : uint8_t {
    Lfo0, Lfo1, Lfo2, Lfo3,
    ChaosX, ChaosY, ChaosZ,
    Rosecurve, Spirograph, Lissajous, Hopforbit, Breathelfo,
    AudioSub, AudioBass, AudioMid, AudioTreble, AudioFlux,
    GestureDist, GestureRot, GestureVortex,
    GestureLefttrigger, GestureRighttrigger, GestureLeftgrip, GestureRightgrip,
    KinematicsSpeed, KinematicsMaxspeed, KinematicsEnergy,
    ResonanceModal0, ResonanceModal1, ResonancePressure, ResonanceEnergy,
    ResonanceCentroid, ResonanceQfactor, ResonanceDamping,
    KuramotoOrderR, KuramotoMeanPhase, KuramotoEntrainment,
    Macro0, Macro1, Macro2, Macro3,
    MacroSpaceWarp, MacroColorChaos, MacroTemporalDecay, MacroDimensionalLift,
    MacroPsychoFlow, MacroPsychoTension, MacroPsychoAura,
    Time,
    None
};

enum class ModulationTargetId : uint8_t {
    Prescale, Postscale, Preoffset, Postoffset,
    Topologyblend, Latentw, Seedwidth, Seedalpha,
    Seedpulsefreq, Seedpulseamp, Huerotation, Huespeed,
    Chromabreath, Lumadrift, Paletteblend, Filmthickness, Filmior,
    Volumetricdensity, Foganisotropy, Advectionspeed, Curlstrength,
    Temporalfeedbackblend, Temporalfade, Worldscale, Radiusscale,
    Traildecay, Kineticviscosity,
    RotorAlpha, RotorBeta, RotorTheta4D,
    Patternspeed,
    None
};

struct CompiledModulationRoute {
    ModulationSourceId sourceId = ModulationSourceId::None;
    ModulationTargetId targetId = ModulationTargetId::None;
    TransferCurve transferCurve = TransferCurve::Linear;
    float scale = 1.0f;
    float bias = 0.0f;
    float slewTimeSeconds = 0.0f;
    mutable float currentSmoothedValue = 0.0f;
    mutable bool initialized = false;
    bool enabled = true;
};

enum class ModulationRouteError : std::uint8_t {
    None,
    MissingArrow,
    UnknownSource,
    UnknownTarget,
    NonfiniteScale,
    NonfiniteBias,
    NonfiniteSlew,
    InvalidScale,
    InvalidBias,
    InvalidSlew,
    UnknownCurve,
    UnknownModifier
};

// Preparation facts only: no installed-state or runtime-history authority.
struct ModulationRouteParseResult {
    std::optional<CompiledModulationRoute> route;
    ModulationRouteError error = ModulationRouteError::None;
};

struct ModulationRouteDiagnostic {
    std::size_t routeIndex = 0; // Zero-based; meaningful only when error != None.
    ModulationRouteError error = ModulationRouteError::None;
};


struct ModulationSourceContext {
    const AudioSignalContext* audio = nullptr;
    const SculptureMotionKinematics* kinematics = nullptr;
    const class SculptureSpatialGestureEngine* gestures = nullptr;
    const struct AudioResonanceMetrics* resonance = nullptr;
    float timeSeconds = 0.0f;
};

// -----------------------------------------------------------------------------
// Universal Multi-Source Modulation Matrix Coordinator
// -----------------------------------------------------------------------------
class SculptureModulationEngine {
public:
    SculptureModulationEngine();

    // An owned, parser-admitted replacement. Carry routes across a cold load
    // boundary without copying the live engine or freezing its signal history.
    class PreparedRoutes {
    public:
        PreparedRoutes(PreparedRoutes&&) noexcept = default;
        PreparedRoutes& operator=(PreparedRoutes&&) noexcept = default;
    private:
        friend class SculptureModulationEngine;
        explicit PreparedRoutes(std::vector<CompiledModulationRoute> routes) noexcept
            : routes_(std::move(routes)) {}
        std::vector<CompiledModulationRoute> routes_;
    };

    // Register oscillators & modulation slots
    void setLfo(std::size_t index, const OscillatorConfig& config);
    [[nodiscard]] const ModulationOscillator& lfo(std::size_t index) const;

    // Macro controls (0..7 or named aliases)
    void setMacro(std::size_t index, float value) noexcept;
    void setMacro(std::string_view name, float value) noexcept;
    [[nodiscard]] float macro(std::size_t index) const noexcept;
    [[nodiscard]] float macro(std::string_view name) const noexcept;
    void resetSmoothing() noexcept;

    // Routing rules
    void addRoute(const CompiledModulationRoute& route);
    void clearRoutes() noexcept;
    [[nodiscard]] const std::vector<CompiledModulationRoute>& routes() const noexcept { return routes_; }

    // Parses human-readable route string: "lfo1 -> smoothstep * 0.5 + 0.1 -> preScale" or "macro0 -> preScale : 0.4"
    // Curve coefficients allow signed/scientific floats and +/- bias; / slew
    // modifiers are separate from that coefficient expression.
    // A slash must introduce slew at its own boundary; unknown modifiers reject
    // rather than being ignored or matched as substrings. Colon/equal/space and
    // legacy delimiterless numeric slew forms retain their existing behavior.
    // Explicit curve names resolve exactly through the shared canonical/alias
    // catalog; unknown or empty names reject instead of selecting linear.
    // Omitted coefficients use defaults; explicit markers require readable finite
    // numbers. Each number must consume its complete field (apart from whitespace
    // or the curve's following bias operator); suffix junk rejects as InvalidScale,
    // InvalidBias or InvalidSlew. Failed conversions never substitute defaults.
    bool parseAndAddRoute(std::string_view routeString);
    // Cold authored-route inspection uses exactly the installation parser,
    // without constructing an engine or changing oscillator/smoothing state.
    [[nodiscard]] static std::optional<CompiledModulationRoute> inspectRoute(std::string_view text) {
        return compileRoute(text);
    }
    [[nodiscard]] static ModulationRouteParseResult inspectRouteResult(std::string_view text);
    static ModulationSourceId mapSourceNameToId(std::string_view name);
    static ModulationTargetId mapTargetNameToId(std::string_view name);

    // All-or-nothing replacement: rejection or preparation exceptions retain
    // existing routes/history. An empty vector explicitly clears engine routes.
    [[nodiscard]] bool replaceRoutes(const std::vector<std::string>& routingMap);
    // Clears the transient diagnostic, then reports the first rejected member.
    // Exceptions retain the existing propagation policy; they are not parse errors.
    [[nodiscard]] bool replaceRoutes(const std::vector<std::string>& routingMap,
                                     ModulationRouteDiagnostic& diagnostic);

    // Preparation has no engine effects; allocation exceptions propagate.
    [[nodiscard]] static std::optional<PreparedRoutes> prepareRoutes(
        const std::vector<std::string>& routingMap, ModulationRouteDiagnostic& diagnostic);
    // Consumes only prepared routes; oscillators, macros and chaos stay live.
    void publishRoutes(PreparedRoutes prepared) noexcept;

    // Legacy partial loading: clears first, then appends each accepted route.
    void loadPresetRoutes(const std::vector<std::string>& routingMap);

    // Apply entire modulation graph to parameters in-place
    void updateAndApply(
        SculptureProgramParams& params,
        const AudioSignalContext* audio,
        float timeSeconds,
        float deltaSeconds
    );

    void updateAndApplyWithContext(
        SculptureProgramParams& params,
        const ModulationSourceContext& context,
        float deltaSeconds
    );

    [[nodiscard]] ChaosOscillator& chaos() noexcept { return chaos_; }
    [[nodiscard]] const ChaosOscillator& chaos() const noexcept { return chaos_; }

private:
    [[nodiscard]] static std::optional<CompiledModulationRoute> compileRoute(std::string_view text) {
        return inspectRouteResult(text).route;
    }
    [[nodiscard]] float evaluateSource(ModulationSourceId sourceId, const ModulationSourceContext& ctx) const noexcept;
    void applyTarget(SculptureProgramParams& params, ModulationTargetId targetId, float value) const noexcept;

    std::array<ModulationOscillator, 4> lfos_;
    ChaosOscillator chaos_;
    std::array<float, 16> macros_{};
    std::vector<CompiledModulationRoute> routes_;
};

} // namespace epi_sculpt
