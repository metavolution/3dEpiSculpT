#include "SculptureControlSession.h"

#include "SculptureMorphChoreographer.h"
#include "SculpturePresetLibrary.h"
#include "SculpturePresetRouteAdmission.h"
#include "SculptureRotorKinematics.h"
#include "SculptureNumericEdit.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace epi_sculpt {
namespace {

// Cold reload admission: retained cue values must not borrow discrete metadata
// from a different source revision. Parameter-only cues have no catalog binding.
bool playlistMatchesCatalog(const SculptureMorphChoreographer& choreographer,
    const SculpturePresetLibrary& catalog) noexcept {
    for (const auto& target : choreographer.playlist()) {
        if (target.presetId.empty()) continue;
        const auto* asset = catalog.resolveExactId(target.presetId).asset;
        if (asset == nullptr || target.presetRevision.empty()
            || target.presetRevision != asset->revision) return false;
    }
    return true;
}

bool selectableForCycling(const SculpturePresetAsset& asset,
    const SculptureMacroAutomationEngine& automation) noexcept {
    if (!hasImplementedPresetRoute(asset)) return false;
    // This cold query uses the installation parser without publishing a graph
    // or consuming live history. Empty authored maps select known defaults.
    try {
        for (const auto& route : asset.routingMap) {
            if (!SculptureModulationEngine::inspectRouteResult(route).route) return false;
        }
        if (asset.macroAutomationJson.empty()) return true;
        auto candidate = automation;
        return candidate.deserializeJson(asset.macroAutomationJson);
    } catch (...) {
        return false;
    }
}

} // namespace

std::optional<SculptureAcousticMistObservation> SculptureControlSession::composePostBase(
    SculptureProgramParams& params, float timeSeconds, float deltaSeconds,
    const SculptureAudioObservation& audio, bool temporalActive,
    SculptureCompositionObservation& composition) {
    // A held manual/rotor stage preserves its input/history; it does not cancel
    // later audio/routing stages or imply rollback of this whole evaluation.
    composition.manualRotors = compositionAdmission(applyManualAndRotorOverrides(
        params, timeSeconds, deltaSeconds, audio.active));

    auto acousticMist = audioModulation_.apply(params, audio, timeSeconds, deltaSeconds);
    // Optional audio contributes values, never a fallback beat or device owner.
    // Visual LFO/chaos routes continue through the same graph when it is absent.
    const ModulationSourceContext context{
        .audio = audio.active ? &audio.ballistic : nullptr,
        .kinematics = nullptr,
        .gestures = nullptr,
        .resonance = audio.active ? &audioModulation_.metrics() : nullptr,
        .timeSeconds = timeSeconds,
    };
    modulation_.updateAndApplyWithContext(params, context, deltaSeconds);

    if (temporalActive) params.temporalFeedbackBlend = 0.8f;
    return acousticMist;
}

modulation_routing::ReconcileOutcome SculptureControlSession::reconcileModulationGraph(
    const std::vector<std::string>& routingMap, bool audioEnabled,
    ModulationRouteDiagnostic& diagnostic) {
    // Existing reconciliation prepares text before engine publication and uses
    // nonthrowing swaps afterward. Its rejection/exception policy is unchanged.
    return modulation_routing::reconcile(routingMap, audioEnabled, modulation_,
        installedRoutingMap_, lastRequestedRoutingMap_, &diagnostic);
}

std::optional<SculptureControlSession::PreparedModulationGraph>
SculptureControlSession::prepareModulationGraph(
    const std::vector<std::string>& routingMap, bool audioEnabled,
    ModulationRouteDiagnostic& diagnostic) {
    diagnostic = {};
    std::vector<std::string> resolved = modulation_routing::resolve(routingMap, audioEnabled);
    auto routes = SculptureModulationEngine::prepareRoutes(resolved, diagnostic);
    if (!routes) return std::nullopt;
    auto requested = resolved;
    return PreparedModulationGraph(std::move(*routes), std::move(resolved), std::move(requested));
}

modulation_routing::ReconcileOutcome SculptureControlSession::publishModulationGraph(
    PreparedModulationGraph prepared) noexcept {
    const bool changed = prepared.installed_ != installedRoutingMap_;
    if (changed) {
        modulation_.publishRoutes(std::move(prepared.routes_));
        installedRoutingMap_.swap(prepared.installed_);
    }
    lastRequestedRoutingMap_.swap(prepared.requested_);
    return changed ? modulation_routing::ReconcileOutcome::Installed
                   : modulation_routing::ReconcileOutcome::Unchanged;
}

std::optional<SculpturePresetTransition> SculptureControlSession::selectPreset(
    const SculpturePresetLibrary& library,
    std::size_t index,
    float durationSeconds,
    SculpturePresetTakeover takeover,
    bool audioEnabled) noexcept {
    const auto* asset = library.at(index);
    if (asset == nullptr || !std::isfinite(durationSeconds) || durationSeconds < 0.0f)
        return std::nullopt;

    // All graph allocation/parsing must precede even automation's transactional
    // commit. After that decoder accepts, only nonthrowing semantic writes remain.
    std::optional<PreparedModulationGraph> graph;
    try {
        ModulationRouteDiagnostic diagnostic;
        graph = prepareModulationGraph(asset->routingMap, audioEnabled, diagnostic);
        if (!graph) return std::nullopt;
        if (!asset->macroAutomationJson.empty()
            && !sources_.deck().macroEngine().deserializeJson(asset->macroAutomationJson))
            return std::nullopt;
    } catch (...) {
        return std::nullopt;
    }
    if (takeover == SculpturePresetTakeover::ReplaceSource
        || takeover == SculpturePresetTakeover::ResetBase) {
        sources_.presetMorph().stopPlaylist();
    }
    if (takeover == SculpturePresetTakeover::ReplaceSource) {
        runtime_.releaseCapturedWorld();
    }
    if (takeover == SculpturePresetTakeover::ResetBase) {
        resetManualOverrides();
        resetContinuousRotors();
    }
    // setTargetPreset only projects params. Passing them directly avoids copying
    // asset strings/routing vectors inside a no-throw semantic transition.
    sources_.presetMorph().setTarget(asset->params, durationSeconds);
    presetIndex_ = index;
    static_cast<void>(publishModulationGraph(std::move(*graph)));
    return SculpturePresetTransition{index, *asset, true};
}

std::optional<SculpturePresetTransition> SculptureControlSession::replacePresetLibrary(
    SculpturePresetLibrary& library,
    SculpturePresetLibrary& candidate,
    float durationSeconds, bool audioEnabled) noexcept {
    if (&library == &candidate) return std::nullopt;
    const auto* selected = library.at(presetIndex_);
    if (selected == nullptr || library.resolveExactId(selected->id).asset != selected)
        return std::nullopt;
    const auto index = candidate.findExactIndex(selected->id);
    if (!index) return std::nullopt;

    // Check even stopped tours: a later start would otherwise revive old cue
    // values under the replacement catalog's current recipes/routes.
    const bool retirePlaylist = !playlistMatchesCatalog(sources_.presetMorph(), candidate);
    auto transition = selectPreset(candidate, *index, durationSeconds,
        SculpturePresetTakeover::Retarget, audioEnabled);
    if (!transition) return std::nullopt;
    // Admission has committed the semantic target/index. Nothing may fail or
    // call out before its matching catalog is published. Vector swap transfers
    // storage, so the transition's asset borrow now belongs to library.
    library.swap(candidate);
    if (retirePlaylist) {
        // clearPlaylist retires cue ownership and clocks, but leaves selectPreset's
        // newly admitted interpolation target intact. stopPlaylist would snap it.
        sources_.presetMorph().clearPlaylist();
        transition->playlistRetired = true;
    }
    return transition;
}

SculptureSourceAdvance SculptureControlSession::advanceSources(float wallDeltaSeconds) noexcept {
    const float delta = runtime_.advance(sources_.cinematic(), wallDeltaSeconds);
    SculptureSourceAdvance advanced{};
    advanced.simulationDelta = delta;
    advanced.playlistWasActive = sources_.advanceControllers(delta, &advanced.composition);
    return advanced;
}

float* SculptureControlSession::manualOffset(SculptureManualParameter parameter) noexcept {
    switch (parameter) {
    case SculptureManualParameter::LatentW: return &manual_.latentW;
    case SculptureManualParameter::RotorAlpha: return &manual_.rotorAlpha;
    case SculptureManualParameter::RotorBeta: return &manual_.rotorBeta;
    case SculptureManualParameter::RotorTheta: return &manual_.rotorTheta;
    case SculptureManualParameter::TopologyBlend: return &manual_.topologyBlend;
    case SculptureManualParameter::Hue: return &manual_.hue;
    case SculptureManualParameter::SeedAlpha: return &manual_.seedAlpha;
    }
    return nullptr;
}

bool SculptureControlSession::adjustManualOffset(SculptureManualParameter parameter, float delta) noexcept {
    float* const target = manualOffset(parameter);
    if (!target) return false;
    switch (parameter) {
    case SculptureManualParameter::LatentW:
        return numeric_edit::addFiniteClamped(*target, delta, -4.0f, 4.0f);
    case SculptureManualParameter::TopologyBlend:
    case SculptureManualParameter::SeedAlpha:
        return numeric_edit::addFiniteClamped(*target, delta, -1.0f, 1.0f);
    default:
        return numeric_edit::addFinite(*target, delta);
    }
}

bool SculptureControlSession::setManualOffset(SculptureManualParameter parameter, float value) noexcept {
    float* const target = manualOffset(parameter);
    return target && numeric_edit::setFinite(*target, value);
}

bool SculptureControlSession::steerManual(float deltaLatentW, float deltaTime) noexcept {
    const auto latentW = numeric_edit::prepareFiniteSum(manual_.latentW, deltaLatentW);
    if (!latentW || !runtime_.steer(deltaTime)) return false;
    manual_.latentW = std::clamp(*latentW, -4.0f, 4.0f);
    return true;
}

bool SculptureControlSession::nudgeManualRotors(SculptureProgramParams& params,
    float deltaAlpha, float deltaBeta, float deltaTheta) noexcept {
    const std::array deltas{deltaAlpha, deltaBeta, deltaTheta};
    const auto manual = numeric_edit::prepareFiniteSums(
        std::array{manual_.rotorAlpha, manual_.rotorBeta, manual_.rotorTheta}, deltas);
    const auto active = numeric_edit::prepareFiniteSums(
        std::array{params.rotorAlpha, params.rotorBeta, params.rotorTheta4D}, deltas);
    if (!manual || !active) return false;
    manual_.rotorAlpha = (*manual)[0];
    manual_.rotorBeta = (*manual)[1];
    manual_.rotorTheta = (*manual)[2];
    params.rotorAlpha = (*active)[0];
    params.rotorBeta = (*active)[1];
    params.rotorTheta4D = (*active)[2];
    sources_.presetMorph().setSource(params);
    return true;
}

void SculptureControlSession::toggleManual4D(bool effectiveEnabled) noexcept {
    manual_.enable4D = manual_.enable4D.has_value() ? !*manual_.enable4D : !effectiveEnabled;
}

bool SculptureControlSession::applyManualAndRotorOverrides(SculptureProgramParams& params,
    float timeSeconds, float deltaSeconds, bool audioReactive) noexcept {
    auto enable4D = params.enable4D;
    if (manual_.enable4D.has_value()) {
        if (*manual_.enable4D) {
            if (enable4D == 0U) enable4D = 1U;
        } else {
            enable4D = 0U;
        }
    }
    const auto rotors = prepareContinuousRotors(
        params.latentW, enable4D != 0U, timeSeconds, deltaSeconds, audioReactive);
    if (!rotors) return false;

    // Preserve rotor grouping as base + (manual + continuous).
    const auto offsets = numeric_edit::prepareFiniteSums(
        std::array{manual_.rotorAlpha, manual_.rotorBeta, manual_.rotorTheta},
        std::array{rotors->alpha, rotors->beta, rotors->theta});
    if (!offsets) return false;
    const auto values = numeric_edit::prepareFiniteSums(
        std::array{rotors->latentW, params.rotorAlpha, params.rotorBeta,
            params.rotorTheta4D, params.topologyBlend, params.hueRotation, params.seedAlpha},
        std::array{manual_.latentW, (*offsets)[0], (*offsets)[1], (*offsets)[2],
            manual_.topologyBlend, manual_.hue, manual_.seedAlpha});
    // A clamp is output policy, not admission of an overflowing pre-clamp sum.
    if (!values) return false;

    params.enable4D = enable4D;
    params.latentW = (*values)[0];
    params.rotorAlpha = (*values)[1];
    params.rotorBeta = (*values)[2];
    params.rotorTheta4D = (*values)[3];
    params.topologyBlend = std::clamp((*values)[4], 0.0f, 1.0f);
    params.hueRotation = (*values)[5];
    params.seedAlpha = std::clamp((*values)[6], 0.0f, 1.0f);
    continuousRotorAlpha_ = rotors->alpha;
    continuousRotorBeta_ = rotors->beta;
    continuousRotorTheta_ = rotors->theta;
    return true;
}

std::optional<SculptureRotorComposition> SculptureControlSession::prepareContinuousRotors(
    float latentW, bool enabled, float timeSeconds, float deltaSeconds,
    bool audioReactive) const noexcept {
    SculptureRotorComposition candidate{
        continuousRotorAlpha_, continuousRotorBeta_, continuousRotorTheta_, latentW};
    if (enabled && !runtime_.paused()) {
        // Use the prior audio evaluation's entrainment and this source owner's
        // tape speed. Updating audio here would change composition ordering.
        const float entrainment = audioModulation_.rotorEntrainment(audioReactive);
        const float tapeFactor = sources_.deckActive() && sources_.deck().isTapeStopActive()
            ? sources_.deck().tapeStopSpeedFactor() : 1.0f;
        if (!tryEvaluateContinuousRotorKinematics(
            deltaSeconds, 0.35f, 0.25f, entrainment, tapeFactor,
            candidate.alpha, candidate.beta, candidate.theta,
            candidate.latentW, timeSeconds)) return std::nullopt;
    }
    return candidate;
}

void SculptureControlSession::resetContinuousRotors() noexcept {
    continuousRotorAlpha_ = 0.0f;
    continuousRotorBeta_ = 0.0f;
    continuousRotorTheta_ = 0.0f;
}

std::optional<SculpturePresetTransition> SculptureControlSession::bindPreset(
    const SculpturePresetLibrary& library, std::size_t index) noexcept {
    const auto* asset = library.at(index);
    if (asset == nullptr) return std::nullopt;
    presetIndex_ = index;
    return SculpturePresetTransition{index, *asset};
}

std::optional<std::size_t> SculptureControlSession::cyclePresetIndex(
    const SculpturePresetLibrary& library, int offset) const noexcept {
    const auto& entries = library.entries();
    if (offset == 0 || presetIndex_ >= entries.size()) return std::nullopt;

    std::size_t eligible = 0U;
    for (const auto& asset : entries) {
        if (selectableForCycling(asset, sources_.deck().macroEngine())) ++eligible;
    }
    if (eligible == 0U) return std::nullopt;

    // Widen before negation: abs(INT_MIN) is not representable as an int.
    const auto magnitude = static_cast<std::uint64_t>(
        offset < 0 ? -static_cast<std::int64_t>(offset) : offset);
    auto remaining = magnitude % eligible;
    if (remaining == 0U) remaining = eligible;
    auto next = presetIndex_;
    for (std::size_t visited = 0U; visited < entries.size(); ++visited) {
        next = offset > 0 ? (next + 1U == entries.size() ? 0U : next + 1U)
                          : (next == 0U ? entries.size() - 1U : next - 1U);
        if (selectableForCycling(entries[next], sources_.deck().macroEngine()) && --remaining == 0U) return next;
    }
    return std::nullopt;
}

bool SculptureControlSession::admitSequenceRoutes(
    const SculptureSequencer& sequence,
    ModulationRouteDiagnostic* diagnostic) noexcept {
    if (diagnostic != nullptr) *diagnostic = {};
    try {
        for (const SequenceKeyframe& keyframe : sequence.keyframes()) {
            for (std::size_t routeIndex = 0U; routeIndex < keyframe.routingMap.size(); ++routeIndex) {
                const auto parsed = SculptureModulationEngine::inspectRouteResult(
                    keyframe.routingMap[routeIndex]);
                if (!parsed.route) {
                    if (diagnostic != nullptr) {
                        *diagnostic = {routeIndex, parsed.error};
                    }
                    return false;
                }
            }
        }
        return true;
    } catch (...) {
        return false;
    }
}

bool SculptureControlSession::loadSequence(
    const SculpturePresetLibrary& library,
    const std::filesystem::path& path,
    const SequenceLoadLimits& limits,
    ModulationRouteDiagnostic* diagnostic) noexcept {
    if (diagnostic != nullptr) *diagnostic = {};
    try {
        auto candidate = sources_.sequence().makeReplacementCandidate();
        if (!library.loadSequenceFromFile(path, candidate, limits)) return false;
        if (!admitSequenceRoutes(candidate, diagnostic)) return false;
        sources_.sequence() = std::move(candidate);
        return sources_.acceptSequenceLoad(true);
    } catch (...) {
        return false;
    }
}

} // namespace epi_sculpt
