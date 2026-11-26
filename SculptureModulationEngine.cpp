#include "SculptureModulationEngine.h"
#include "SculptureModulator.h"
#include "SculptureSpatialGestureEngine.h"
#include "SculptureAudioResonance.h"
#include <numbers>
#include <utility>

namespace epi_sculpt {

namespace {

constexpr float kTwoPi = 6.28318530717958647692f;

float hash11(float p) {
    p = std::sin(p * 127.1f + 311.7f) * 43758.5453123f;
    return (p - std::floor(p)) * 2.0f - 1.0f;
}

float perlin1D(float x) {
    float i = std::floor(x);
    float f = x - i;
    float u = f * f * (3.0f - 2.0f * f); // Cubic Hermite
    return std::lerp(hash11(i), hash11(i + 1.0f), u);
}

} // namespace

float ModulationOscillator::evaluate(float timeSeconds) const noexcept {
    const float t = timeSeconds * config_.frequencyHz + (config_.phaseOffsetRad / kTwoPi);
    const float phase = t - std::floor(t); // Normalized [0, 1)
    float raw = 0.0f;

    switch (config_.waveform) {
    case ModulationWaveform::Sine:
        raw = std::sin(phase * kTwoPi);
        break;
    case ModulationWaveform::Cosine:
        raw = std::cos(phase * kTwoPi);
        break;
    case ModulationWaveform::Triangle:
        raw = 1.0f - 4.0f * std::abs(phase - 0.5f);
        break;
    case ModulationWaveform::SawtoothUp:
        raw = 2.0f * phase - 1.0f;
        break;
    case ModulationWaveform::SawtoothDown:
        raw = 1.0f - 2.0f * phase;
        break;
    case ModulationWaveform::Square:
        raw = (phase < 0.5f) ? 1.0f : -1.0f;
        break;
    case ModulationWaveform::PulseWidth:
        raw = (phase < std::clamp(config_.pulseWidth, 0.01f, 0.99f)) ? 1.0f : -1.0f;
        break;
    case ModulationWaveform::PerlinNoise:
        raw = perlin1D(timeSeconds * config_.frequencyHz * 4.0f);
        break;
    case ModulationWaveform::SampleAndHold:
        raw = hash11(std::floor(t));
        break;
    case ModulationWaveform::LorenzChaosX:
    case ModulationWaveform::LorenzChaosZ:
        raw = std::sin(phase * kTwoPi);
        break;
    }

    return raw * config_.amplitude + config_.bias;
}

void ChaosOscillator::step(float simDeltaTime) noexcept {
    const float dt = std::clamp(simDeltaTime, 0.001f, 0.05f);
    
    // RK4 integration for Lorenz attractor:
    auto lorenzDeriv = [this](const State& s) -> State {
        return State{
            sigma_ * (s.y - s.x),
            s.x * (rho_ - s.z) - s.y,
            s.x * s.y - beta_ * s.z
        };
    };

    const State k1 = lorenzDeriv(state_);
    const State s2{state_.x + 0.5f * dt * k1.x, state_.y + 0.5f * dt * k1.y, state_.z + 0.5f * dt * k1.z};
    const State k2 = lorenzDeriv(s2);
    const State s3{state_.x + 0.5f * dt * k2.x, state_.y + 0.5f * dt * k2.y, state_.z + 0.5f * dt * k2.z};
    const State k3 = lorenzDeriv(s3);
    const State s4{state_.x + dt * k3.x, state_.y + dt * k3.y, state_.z + dt * k3.z};
    const State k4 = lorenzDeriv(s4);

    state_.x += (dt / 6.0f) * (k1.x + 2.0f * k2.x + 2.0f * k3.x + k4.x);
    state_.y += (dt / 6.0f) * (k1.y + 2.0f * k2.y + 2.0f * k3.y + k4.y);
    state_.z += (dt / 6.0f) * (k1.z + 2.0f * k2.z + 2.0f * k3.z + k4.z);

    // Bounded attractor containment
    if (!std::isfinite(state_.x) || !std::isfinite(state_.y) || !std::isfinite(state_.z) ||
        std::abs(state_.x) > 100.0f || std::abs(state_.y) > 100.0f || state_.z > 100.0f) {
        reset();
    }
}

SculptureModulationEngine::SculptureModulationEngine() {
    // Initialize default LFO banks
    lfos_[0] = ModulationOscillator({ModulationWaveform::Sine, 0.05f, 1.0f, 0.0f, 0.5f, 0.0f});
    lfos_[1] = ModulationOscillator({ModulationWaveform::Triangle, 0.08f, 1.0f, 1.57f, 0.5f, 0.0f});
    lfos_[2] = ModulationOscillator({ModulationWaveform::PerlinNoise, 0.12f, 1.0f, 0.0f, 0.5f, 0.0f});
    lfos_[3] = ModulationOscillator({ModulationWaveform::SampleAndHold, 0.25f, 1.0f, 0.0f, 0.5f, 0.0f});
}

void SculptureModulationEngine::setLfo(std::size_t index, const OscillatorConfig& config) {
    if (index < lfos_.size()) {
        lfos_[index].setConfig(config);
    }
}

const ModulationOscillator& SculptureModulationEngine::lfo(std::size_t index) const {
    const std::size_t idx = std::min(index, lfos_.size() - 1);
    return lfos_[idx];
}

void SculptureModulationEngine::setMacro(std::size_t index, float value) noexcept {
    if (index < macros_.size()) {
        macros_[index] = value;
    }
}

void SculptureModulationEngine::setMacro(std::string_view name, float value) noexcept {
    if (name == "macro0" || name == "macro") setMacro(0, value);
    else if (name == "macro1") setMacro(1, value);
    else if (name == "macro2") setMacro(2, value);
    else if (name == "macro3") setMacro(3, value);
    else if (name == "macro:warp" || name == "macroSpaceWarp" || name == "macroWarp") setMacro(4, value);
    else if (name == "macro:chaos" || name == "macroColorChaos" || name == "macroChaos") setMacro(5, value);
    else if (name == "macro:decay" || name == "macroTemporalDecay" || name == "macroDecay") setMacro(6, value);
    else if (name == "macro:lift" || name == "macroDimensionalLift" || name == "macroLift") setMacro(7, value);
    else if (name == "psychoFlow" || name == "macroPsychoFlow") setMacro(8, value);
    else if (name == "psychoTension" || name == "macroPsychoTension") setMacro(9, value);
    else if (name == "psychoAura" || name == "macroPsychoAura") setMacro(10, value);
}

float SculptureModulationEngine::macro(std::size_t index) const noexcept {
    if (index < macros_.size()) return macros_[index];
    return 0.0f;
}

float SculptureModulationEngine::macro(std::string_view name) const noexcept {
    if (name == "macro0" || name == "macro") return macro(0);
    if (name == "macro1") return macro(1);
    if (name == "macro2") return macro(2);
    if (name == "macro3") return macro(3);
    if (name == "macro:warp" || name == "macroSpaceWarp" || name == "macroWarp") return macro(4);
    if (name == "macro:chaos" || name == "macroColorChaos" || name == "macroChaos") return macro(5);
    if (name == "macro:decay" || name == "macroTemporalDecay" || name == "macroDecay") return macro(6);
    if (name == "macro:lift" || name == "macroDimensionalLift" || name == "macroLift") return macro(7);
    if (name == "psychoFlow" || name == "macroPsychoFlow") return macro(8);
    if (name == "psychoTension" || name == "macroPsychoTension") return macro(9);
    if (name == "psychoAura" || name == "macroPsychoAura") return macro(10);
    return 0.0f;
}

void SculptureModulationEngine::resetSmoothing() noexcept {
    for (auto& route : routes_) {
        route.initialized = false;
        route.currentSmoothedValue = 0.0f;
    }
}

void SculptureModulationEngine::addRoute(const CompiledModulationRoute& route) {
    routes_.push_back(route);
}

void SculptureModulationEngine::clearRoutes() noexcept {
    routes_.clear();
}

void SculptureModulationEngine::loadPresetRoutes(const std::vector<std::string>& routingMap) {
    clearRoutes();
    for (const auto& routeStr : routingMap) {
        parseAndAddRoute(routeStr);
    }
}

bool SculptureModulationEngine::parseAndAddRoute(std::string_view routeString) {
    auto route = compileRoute(routeString);
    if (!route) return false;
    routes_.push_back(std::move(*route));
    return true;
}

bool SculptureModulationEngine::replaceRoutes(const std::vector<std::string>& routingMap) {
    ModulationRouteDiagnostic diagnostic;
    return replaceRoutes(routingMap, diagnostic);
}

bool SculptureModulationEngine::replaceRoutes(
    const std::vector<std::string>& routingMap, ModulationRouteDiagnostic& diagnostic) {
    auto prepared = prepareRoutes(routingMap, diagnostic);
    if (!prepared) return false;
    publishRoutes(std::move(*prepared));
    return true;
}

std::optional<SculptureModulationEngine::PreparedRoutes> SculptureModulationEngine::prepareRoutes(
    const std::vector<std::string>& routingMap, ModulationRouteDiagnostic& diagnostic) {
    diagnostic = {};
    // Prepare only the route vector, preserving the live engine's oscillators,
    // macros and chaos state. Nothing observable changes until the final swap.
    std::vector<CompiledModulationRoute> prepared;
    prepared.reserve(routingMap.size());
    for (std::size_t index = 0; index < routingMap.size(); ++index) {
        auto result = inspectRouteResult(routingMap[index]);
        if (!result.route) {
            diagnostic = {index, result.error};
            return std::nullopt;
        }
        prepared.push_back(std::move(*result.route));
    }
    return PreparedRoutes(std::move(prepared));
}

void SculptureModulationEngine::publishRoutes(PreparedRoutes prepared) noexcept {
    routes_.swap(prepared.routes_);
}

float SculptureModulationEngine::evaluateSource(
    ModulationSourceId sourceId,
    const ModulationSourceContext& ctx
) const noexcept {
    switch (sourceId) {
        case ModulationSourceId::Lfo0: return lfos_[0].evaluate(ctx.timeSeconds);
        case ModulationSourceId::Lfo1: return lfos_[1].evaluate(ctx.timeSeconds);
        case ModulationSourceId::Lfo2: return lfos_[2].evaluate(ctx.timeSeconds);
        case ModulationSourceId::Lfo3: return lfos_[3].evaluate(ctx.timeSeconds);
        case ModulationSourceId::ChaosX: return chaos_.valueX();
        case ModulationSourceId::ChaosY: return chaos_.valueY();
        case ModulationSourceId::ChaosZ: return chaos_.valueZ();
        case ModulationSourceId::Rosecurve: return SculptureParametricMotionOrchestrator::evaluateRoseCurve(ctx.timeSeconds).first;
        case ModulationSourceId::Spirograph: return SculptureParametricMotionOrchestrator::evaluateSpirograph(ctx.timeSeconds).first;
        case ModulationSourceId::Lissajous: return SculptureParametricMotionOrchestrator::evaluateLissajous(ctx.timeSeconds).first;
        case ModulationSourceId::Hopforbit: return SculptureParametricMotionOrchestrator::evaluateHopfOrbit(ctx.timeSeconds);
        case ModulationSourceId::Breathelfo: return SculptureParametricMotionOrchestrator::evaluateBreathingLFO(ctx.timeSeconds);
        case ModulationSourceId::AudioSub: return ctx.audio ? ctx.audio->subBass : 0.0f;
        case ModulationSourceId::AudioBass: return ctx.audio ? ctx.audio->bass : 0.0f;
        case ModulationSourceId::AudioMid: return ctx.audio ? ctx.audio->mid : 0.0f;
        case ModulationSourceId::AudioTreble: return ctx.audio ? ctx.audio->treble : 0.0f;
        case ModulationSourceId::AudioFlux: return ctx.audio ? ctx.audio->spectralFlux : 0.0f;
        case ModulationSourceId::GestureDist: return ctx.gestures ? ctx.gestures->gestureScaleDelta() : 0.0f;
        case ModulationSourceId::GestureRot: return ctx.gestures ? ctx.gestures->gestureRotationDelta() : 0.0f;
        case ModulationSourceId::GestureVortex: return ctx.gestures ? ctx.gestures->gestureVortexIntensity() : 0.0f;
        case ModulationSourceId::GestureLefttrigger: return ctx.gestures ? ctx.gestures->leftHand().trigger : 0.0f;
        case ModulationSourceId::GestureRighttrigger: return ctx.gestures ? ctx.gestures->rightHand().trigger : 0.0f;
        case ModulationSourceId::GestureLeftgrip: return ctx.gestures ? ctx.gestures->leftHand().grip : 0.0f;
        case ModulationSourceId::GestureRightgrip: return ctx.gestures ? ctx.gestures->rightHand().grip : 0.0f;
        case ModulationSourceId::KinematicsSpeed: return ctx.kinematics ? ctx.kinematics->averageVelocity.x : 0.0f;
        case ModulationSourceId::KinematicsMaxspeed: return ctx.kinematics ? ctx.kinematics->maxSpeed : 0.0f;
        case ModulationSourceId::KinematicsEnergy: return ctx.kinematics ? ctx.kinematics->kineticEnergy : 0.0f;
        case ModulationSourceId::ResonanceModal0: return ctx.resonance ? ctx.resonance->modalFrequenciesHz[0] : 0.0f;
        case ModulationSourceId::ResonanceModal1: return ctx.resonance ? ctx.resonance->modalFrequenciesHz[1] : 0.0f;
        case ModulationSourceId::ResonancePressure: return ctx.resonance ? ctx.resonance->instantaneousPressure : 0.0f;
        case ModulationSourceId::ResonanceEnergy: return ctx.resonance ? ctx.resonance->acousticEnergy : 0.0f;
        case ModulationSourceId::ResonanceCentroid: return ctx.resonance ? ctx.resonance->spectralCentroidHz : 0.0f;
        case ModulationSourceId::ResonanceQfactor: return ctx.resonance ? ctx.resonance->resonanceQFactor : 0.0f;
        case ModulationSourceId::ResonanceDamping: return ctx.resonance ? ctx.resonance->bioAdaptiveDamping : 0.0f;
        case ModulationSourceId::KuramotoOrderR: return ctx.resonance ? ctx.resonance->kuramotoOrderR : 0.0f;
        case ModulationSourceId::KuramotoMeanPhase: return ctx.resonance ? ctx.resonance->kuramotoMeanPhase : 0.0f;
        case ModulationSourceId::KuramotoEntrainment: return ctx.resonance ? ctx.resonance->spectralEntrainment : 0.0f;
        case ModulationSourceId::Macro0: return macros_[0];
        case ModulationSourceId::Macro1: return macros_[1];
        case ModulationSourceId::Macro2: return macros_[2];
        case ModulationSourceId::Macro3: return macros_[3];
        case ModulationSourceId::MacroSpaceWarp: return macros_[4];
        case ModulationSourceId::MacroColorChaos: return macros_[5];
        case ModulationSourceId::MacroTemporalDecay: return macros_[6];
        case ModulationSourceId::MacroDimensionalLift: return macros_[7];
        case ModulationSourceId::MacroPsychoFlow: return macros_[8];
        case ModulationSourceId::MacroPsychoTension: return macros_[9];
        case ModulationSourceId::MacroPsychoAura: return macros_[10];
        case ModulationSourceId::Time: return ctx.timeSeconds;
        default: return 0.0f;
    }
}



void SculptureModulationEngine::applyTarget(
    SculptureProgramParams& params,
    ModulationTargetId targetId,
    float value
) const noexcept {
    switch (targetId) {
        case ModulationTargetId::Prescale: params.pre.scaleReal += value; params.pre.scaleImag += value * 0.5f; break;
        case ModulationTargetId::Postscale: params.post.scaleReal += value; params.post.scaleImag += value * 0.5f; break;
        case ModulationTargetId::Preoffset: params.pre.offsetReal += value; break;
        case ModulationTargetId::Postoffset: params.post.offsetReal += value; break;
        case ModulationTargetId::Topologyblend: params.topologyBlend = std::clamp(params.topologyBlend + value, 0.0f, 1.0f); break;
        case ModulationTargetId::Latentw: params.latentW += value; break;
        case ModulationTargetId::Seedwidth: params.seedWidth = std::clamp(params.seedWidth + value, 0.05f, 2.0f); break;
        case ModulationTargetId::Seedalpha: params.seedAlpha = std::clamp(params.seedAlpha + value, 0.0f, 1.0f); break;
        case ModulationTargetId::Seedpulsefreq: params.seedPulseFrequency = std::clamp(params.seedPulseFrequency + value, 0.01f, 1.0f); break;
        case ModulationTargetId::Seedpulseamp: params.seedPulseAmplitude = std::clamp(params.seedPulseAmplitude + value, 0.0f, 1.0f); break;
        case ModulationTargetId::Huerotation: params.hueRotation = std::fmod(params.hueRotation + value + 1.0f, 1.0f); break;
        case ModulationTargetId::Huespeed: params.hueSpeed += value; break;
        case ModulationTargetId::Chromabreath: params.chromaBreath = std::clamp(params.chromaBreath + value, 0.0f, 1.0f); break;
        case ModulationTargetId::Lumadrift: params.lumaDrift = std::clamp(params.lumaDrift + value, 0.0f, 1.0f); break;
        case ModulationTargetId::Paletteblend: params.paletteBlend = std::clamp(params.paletteBlend + value, 0.0f, 1.0f); break;
        case ModulationTargetId::Filmthickness: params.filmThickness = std::clamp(params.filmThickness + value, 50.0f, 1500.0f); break;
        case ModulationTargetId::Filmior: params.filmIOR = std::clamp(params.filmIOR + value, 1.05f, 2.80f); break;
        case ModulationTargetId::Volumetricdensity: params.volumetricDensityScale = std::clamp(params.volumetricDensityScale + value, 0.0f, 3.0f); break;
        case ModulationTargetId::Foganisotropy: params.fogAnisotropy = std::clamp(params.fogAnisotropy + value, -0.95f, 0.95f); break;
        case ModulationTargetId::Advectionspeed: params.advectionSpeed = std::clamp(params.advectionSpeed + value, 0.0f, 2.0f); break;
        case ModulationTargetId::Curlstrength: params.curlStrength = std::clamp(params.curlStrength + value, 0.0f, 2.0f); break;
        case ModulationTargetId::Temporalfeedbackblend: params.temporalFeedbackBlend = std::clamp(params.temporalFeedbackBlend + value, 0.0f, 1.0f); break;
        case ModulationTargetId::Temporalfade: params.temporalFade = std::clamp(params.temporalFade + value, 0.0f, 1.0f); break;
        case ModulationTargetId::Worldscale: params.worldScale = std::clamp(params.worldScale + value, 0.1f, 5.0f); break;
        case ModulationTargetId::Radiusscale: params.radiusScale = std::clamp(params.radiusScale + value, 0.1f, 5.0f); break;
        case ModulationTargetId::Traildecay: params.trailDecay = std::clamp(params.trailDecay + value, 0.1f, 0.99f); break;
        case ModulationTargetId::Kineticviscosity: params.kineticViscosity = std::clamp(params.kineticViscosity + value, 0.001f, 1.0f); break;
        case ModulationTargetId::RotorAlpha: params.rotorAlpha += value; break;
        case ModulationTargetId::RotorBeta: params.rotorBeta += value; break;
        case ModulationTargetId::RotorTheta4D: params.rotorTheta4D += value; break;
        case ModulationTargetId::Patternspeed: params.patternSpeed = std::clamp(params.patternSpeed + value, 0.001f, 2.0f); break;
        default: break;
    }
}

void SculptureModulationEngine::updateAndApply(
    SculptureProgramParams& params,
    const AudioSignalContext* audio,
    float timeSeconds,
    float deltaSeconds
) {
    const ModulationSourceContext ctx{
        .audio = audio,
        .kinematics = nullptr,
        .gestures = nullptr,
        .resonance = nullptr,
        .timeSeconds = timeSeconds
    };
    updateAndApplyWithContext(params, ctx, deltaSeconds);
}

void SculptureModulationEngine::updateAndApplyWithContext(
    SculptureProgramParams& params,
    const ModulationSourceContext& context,
    float deltaSeconds
) {
    // Step internal dynamical systems
    chaos_.step(deltaSeconds);

    // Apply all active routing links
    for (const auto& route : routes_) {
        if (!route.enabled) continue;
        if (context.kinematics && !context.kinematics->valid
            && (route.sourceId == ModulationSourceId::KinematicsSpeed
                || route.sourceId == ModulationSourceId::KinematicsMaxspeed
                || route.sourceId == ModulationSourceId::KinematicsEnergy)) continue;

        const float rawSource = evaluateSource(route.sourceId, context);
        const float shaped = ModulationOscillator::applyTransferCurve(rawSource, route.transferCurve);
        const float targetVal = shaped * route.scale + route.bias;

        float finalVal = targetVal;
        if (route.slewTimeSeconds > 0.0f && deltaSeconds > 0.0f) {
            if (!route.initialized) {
                route.currentSmoothedValue = targetVal;
                route.initialized = true;
            } else {
                const float alpha = 1.0f - std::exp(-deltaSeconds / route.slewTimeSeconds);
                route.currentSmoothedValue += alpha * (targetVal - route.currentSmoothedValue);
            }
            finalVal = route.currentSmoothedValue;
        } else {
            route.currentSmoothedValue = targetVal;
            route.initialized = true;
        }

        applyTarget(params, route.targetId, finalVal);
    }
}

} // namespace epi_sculpt
