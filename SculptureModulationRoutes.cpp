// Cold authored-route preparation shared by inspection and runtime installation.
// Execution remains in SculptureModulationEngine.cpp; this codec owns no state.
#include "SculptureModulationEngine.h"
#include "SculptureModulationCurves.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <string>
#include <string_view>

namespace epi_sculpt {
namespace {

struct ParsedNumber {
    float value = 0.0f;
    std::size_t next = 0U;
    bool valid = false;
};

// Conversion identifies the numeric prefix; the caller owns its field boundary.
// This keeps unary/exponent signs inside the number instead of mistaking them
// for a bias operator, and never accepts trailing text by skipping over it.
ParsedNumber parseNumber(const std::string& text, std::size_t begin) noexcept {
    if (begin > text.size()) return {};
    const char* base = text.c_str();
    char* end = nullptr;
    const float value = std::strtof(base + begin, &end);
    return {value, static_cast<std::size_t>(end - base), end != base + begin};
}

std::size_t skipSpace(std::string_view text, std::size_t begin) noexcept {
    const auto next = text.find_first_not_of(" \t\r\n\f\v", begin);
    return next == std::string_view::npos ? text.size() : next;
}

bool onlySpace(std::string_view text, std::size_t begin, std::size_t end) noexcept {
    return begin <= end && end <= text.size()
        && skipSpace(text.substr(0, end), begin) == end;
}

} // namespace

ModulationSourceId SculptureModulationEngine::mapSourceNameToId(std::string_view name) {
    if (name == "lfo0" || name == "latentLFO") return ModulationSourceId::Lfo0;
    if (name == "lfo1" || name == "hueLFO") return ModulationSourceId::Lfo1;
    if (name == "lfo2") return ModulationSourceId::Lfo2;
    if (name == "lfo3") return ModulationSourceId::Lfo3;
    if (name == "chaos:x") return ModulationSourceId::ChaosX;
    if (name == "chaos:y") return ModulationSourceId::ChaosY;
    if (name == "chaos:z") return ModulationSourceId::ChaosZ;
    if (name == "roseCurve" || name == "roseReal") return ModulationSourceId::Rosecurve;
    if (name == "spirograph" || name == "spiroReal") return ModulationSourceId::Spirograph;
    if (name == "lissajous" || name == "lissajousReal") return ModulationSourceId::Lissajous;
    if (name == "hopfOrbit" || name == "orbitLFO" || name == "poincareOrbit" || name == "calabiFlow") return ModulationSourceId::Hopforbit;
    if (name == "breatheLFO") return ModulationSourceId::Breathelfo;
    if (name == "audio:sub" || name == "audioSub" || name == "audioSubBass" || name == "subBass") return ModulationSourceId::AudioSub;
    if (name == "audio:bass" || name == "audioBass" || name == "bass") return ModulationSourceId::AudioBass;
    if (name == "audio:mid" || name == "audioMid" || name == "mid") return ModulationSourceId::AudioMid;
    if (name == "audio:treble" || name == "audioTreble" || name == "treble" || name == "audioHigh" || name == "audio:high") return ModulationSourceId::AudioTreble;
    if (name == "audio:flux" || name == "audioFlux" || name == "flux" || name == "transientFlux" || name == "audioEnergy") return ModulationSourceId::AudioFlux;
    if (name == "gesture:dist" || name == "gestureScale") return ModulationSourceId::GestureDist;
    if (name == "gesture:rot" || name == "gestureRotation") return ModulationSourceId::GestureRot;
    if (name == "gesture:vortex") return ModulationSourceId::GestureVortex;
    if (name == "gesture:leftTrigger") return ModulationSourceId::GestureLefttrigger;
    if (name == "gesture:rightTrigger") return ModulationSourceId::GestureRighttrigger;
    if (name == "gesture:leftGrip") return ModulationSourceId::GestureLeftgrip;
    if (name == "gesture:rightGrip") return ModulationSourceId::GestureRightgrip;
    if (name == "kinematics:speed" || name == "averageSpeed") return ModulationSourceId::KinematicsSpeed;
    if (name == "kinematics:maxSpeed") return ModulationSourceId::KinematicsMaxspeed;
    if (name == "kinematics:energy") return ModulationSourceId::KinematicsEnergy;
    if (name == "resonance:modal0" || name == "modal0") return ModulationSourceId::ResonanceModal0;
    if (name == "resonance:modal1" || name == "modal1") return ModulationSourceId::ResonanceModal1;
    if (name == "resonance:pressure" || name == "acousticPressure" || name == "resonance_pressure") return ModulationSourceId::ResonancePressure;
    if (name == "resonance:energy" || name == "acousticEnergy" || name == "resonance_energy") return ModulationSourceId::ResonanceEnergy;
    if (name == "resonance:centroid" || name == "spectralCentroid" || name == "resonance_centroid") return ModulationSourceId::ResonanceCentroid;
    if (name == "resonance:qFactor" || name == "qFactor" || name == "resonance_qfactor") return ModulationSourceId::ResonanceQfactor;
    if (name == "resonance:damping" || name == "resonance_damping") return ModulationSourceId::ResonanceDamping;
    if (name == "kuramotoOrder" || name == "kuramotoR" || name == "kuramoto:orderR" || name == "kuramoto_r") return ModulationSourceId::KuramotoOrderR;
    if (name == "kuramotoPhase" || name == "kuramotoMeanPhase" || name == "kuramoto:phase" || name == "kuramoto_phase") return ModulationSourceId::KuramotoMeanPhase;
    if (name == "kuramotoEntrainment" || name == "entrainment" || name == "kuramoto:entrainment" || name == "kuramoto_entrainment") return ModulationSourceId::KuramotoEntrainment;
    if (name == "macro0" || name == "macro") return ModulationSourceId::Macro0;
    if (name == "macro1") return ModulationSourceId::Macro1;
    if (name == "macro2") return ModulationSourceId::Macro2;
    if (name == "macro3") return ModulationSourceId::Macro3;
    if (name == "macro:warp" || name == "macroSpaceWarp" || name == "macroWarp") return ModulationSourceId::MacroSpaceWarp;
    if (name == "macro:chaos" || name == "macroColorChaos" || name == "macroChaos") return ModulationSourceId::MacroColorChaos;
    if (name == "macro:decay" || name == "macroTemporalDecay" || name == "macroDecay") return ModulationSourceId::MacroTemporalDecay;
    if (name == "macro:lift" || name == "macroDimensionalLift" || name == "macroLift") return ModulationSourceId::MacroDimensionalLift;
    if (name == "psychoFlow" || name == "macroPsychoFlow") return ModulationSourceId::MacroPsychoFlow;
    if (name == "psychoTension" || name == "macroPsychoTension") return ModulationSourceId::MacroPsychoTension;
    if (name == "psychoAura" || name == "macroPsychoAura") return ModulationSourceId::MacroPsychoAura;
    if (name == "time" || name == "simTime") return ModulationSourceId::Time;
    return ModulationSourceId::None;
}

ModulationTargetId SculptureModulationEngine::mapTargetNameToId(std::string_view name) {
    if (name == "preScale") return ModulationTargetId::Prescale;
    if (name == "postScale") return ModulationTargetId::Postscale;
    if (name == "preOffset") return ModulationTargetId::Preoffset;
    if (name == "postOffset") return ModulationTargetId::Postoffset;
    if (name == "topologyBlend") return ModulationTargetId::Topologyblend;
    if (name == "latentW") return ModulationTargetId::Latentw;
    if (name == "seedWidth") return ModulationTargetId::Seedwidth;
    if (name == "seedAlpha") return ModulationTargetId::Seedalpha;
    if (name == "seedPulseFreq" || name == "pulseFrequency" || name == "seedPulseFrequency") return ModulationTargetId::Seedpulsefreq;
    if (name == "seedPulseAmp" || name == "pulseAmplitude" || name == "seedPulseAmplitude" || name == "scaleAmplitude" || name == "scaleAmp") return ModulationTargetId::Seedpulseamp;
    if (name == "patternSpeed" || name == "speed") return ModulationTargetId::Patternspeed;
    if (name == "hueRotation") return ModulationTargetId::Huerotation;
    if (name == "hueSpeed" || name == "colorSpeed") return ModulationTargetId::Huespeed;
    if (name == "chromaBreath") return ModulationTargetId::Chromabreath;
    if (name == "lumaDrift") return ModulationTargetId::Lumadrift;
    if (name == "paletteBlend") return ModulationTargetId::Paletteblend;
    if (name == "filmThickness") return ModulationTargetId::Filmthickness;
    if (name == "filmIOR") return ModulationTargetId::Filmior;
    if (name == "volumetricDensity" || name == "volumetricDensityScale") return ModulationTargetId::Volumetricdensity;
    if (name == "fogAnisotropy") return ModulationTargetId::Foganisotropy;
    if (name == "advectionSpeed") return ModulationTargetId::Advectionspeed;
    if (name == "curlStrength") return ModulationTargetId::Curlstrength;
    if (name == "temporalFeedbackBlend" || name == "temporalFeedback") return ModulationTargetId::Temporalfeedbackblend;
    if (name == "temporalFade") return ModulationTargetId::Temporalfade;
    if (name == "worldScale") return ModulationTargetId::Worldscale;
    if (name == "radiusScale") return ModulationTargetId::Radiusscale;
    if (name == "trailDecay") return ModulationTargetId::Traildecay;
    if (name == "kineticViscosity") return ModulationTargetId::Kineticviscosity;
    if (name == "rotorAlpha" || name == "rotorA") return ModulationTargetId::RotorAlpha;
    if (name == "rotorBeta" || name == "rotorB") return ModulationTargetId::RotorBeta;
    if (name == "rotorTheta4D" || name == "rotorTheta" || name == "rotorZW") return ModulationTargetId::RotorTheta4D;
    return ModulationTargetId::None;
}

ModulationRouteParseResult SculptureModulationEngine::inspectRouteResult(std::string_view routeString) {
    // Expected formats:
    // 1. "source -> target"
    // 2. "source -> target : scale"
    // 3. "source -> target * scale"
    // 4. "source -> curve * scale +/- bias -> target" (signed/scientific floats)
    std::string str(routeString);
    std::size_t arrow1 = str.find("->");
    if (arrow1 == std::string::npos) return {std::nullopt, ModulationRouteError::MissingArrow};

    std::string source = str.substr(0, arrow1);
    // Trim whitespace
    source.erase(0, source.find_first_not_of(" \t"));
    source.erase(source.find_last_not_of(" \t") + 1);

    std::string rest = str.substr(arrow1 + 2);
    std::size_t arrow2 = rest.find("->");

    CompiledModulationRoute route{};
    route.sourceId = mapSourceNameToId(source);
    route.transferCurve = TransferCurve::Linear;
    // Defaults express omitted controls. An explicit numeric marker below
    // must decode successfully rather than silently changing authored intent.
    route.scale = 1.0f;
    route.bias = 0.0f;
    route.slewTimeSeconds = 0.0f;

    // A slash declares a modifier; it cannot silently discard authored text.
    // Recognize slew only at that boundary, never inside an unrelated name.
    const auto modifierSlash = str.find('/', arrow1 + 2);
    if (modifierSlash != std::string::npos) {
        const auto slewPos = skipSpace(str, modifierSlash + 1);
        if (!std::string_view{str}.substr(slewPos).starts_with("slew"))
            return {std::nullopt, ModulationRouteError::UnknownModifier};
        auto valueBegin = slewPos + 4;
        if (valueBegin < str.size() && (str[valueBegin] == ':' || str[valueBegin] == '=')) ++valueBegin;
        const auto number = parseNumber(str, valueBegin);
        const auto arrow = str.find("->", valueBegin);
        const auto end = arrow == std::string::npos ? str.size() : arrow;
        if (!number.valid || !onlySpace(str, number.next, end))
            return {std::nullopt, ModulationRouteError::InvalidSlew};
        // Reject before clamping: max(0, NaN/-Inf) can hide invalid input.
        if (!std::isfinite(number.value)) return {std::nullopt, ModulationRouteError::NonfiniteSlew};
        route.slewTimeSeconds = std::max(0.0f, number.value);
    }

    if (arrow2 == std::string::npos) {
        // Form: "source -> target" or "source -> target : scale" or "source -> target * scale"
        std::string targetPart = rest;
        std::size_t slash = targetPart.find('/');
        if (slash != std::string::npos) {
            targetPart = targetPart.substr(0, slash);
        }
        targetPart.erase(0, targetPart.find_first_not_of(" \t"));
        targetPart.erase(targetPart.find_last_not_of(" \t") + 1);

        std::size_t colon = targetPart.find(':');
        std::size_t star = targetPart.find('*');

        if (colon != std::string::npos) {
            std::string targetName = targetPart.substr(0, colon);
            targetName.erase(0, targetName.find_first_not_of(" \t"));
            targetName.erase(targetName.find_last_not_of(" \t") + 1);
            route.targetId = mapTargetNameToId(targetName);

            const auto number = parseNumber(targetPart, colon + 1);
            if (!number.valid || !onlySpace(targetPart, number.next, targetPart.size()))
                return {std::nullopt, ModulationRouteError::InvalidScale};
            if (!std::isfinite(number.value)) return {std::nullopt, ModulationRouteError::NonfiniteScale};
            route.scale = number.value;
        } else if (star != std::string::npos) {
            std::string targetName = targetPart.substr(0, star);
            targetName.erase(0, targetName.find_first_not_of(" \t"));
            targetName.erase(targetName.find_last_not_of(" \t") + 1);
            route.targetId = mapTargetNameToId(targetName);

            const auto number = parseNumber(targetPart, star + 1);
            if (!number.valid || !onlySpace(targetPart, number.next, targetPart.size()))
                return {std::nullopt, ModulationRouteError::InvalidScale};
            if (!std::isfinite(number.value)) return {std::nullopt, ModulationRouteError::NonfiniteScale};
            route.scale = number.value;
        } else {
            route.targetId = mapTargetNameToId(targetPart);
        }
    } else {
        // Form: "source -> curveExpr -> target"
        std::string curveExpr = rest.substr(0, arrow2);
        // Slew is decoded from the full route above. Its numeric signs belong
        // to that modifier, never to the scale/bias expression.
        if (const auto slash = curveExpr.find('/'); slash != std::string::npos) {
            curveExpr.resize(slash);
        }
        std::string target = rest.substr(arrow2 + 2);
        target.erase(0, target.find_first_not_of(" \t"));
        target.erase(target.find_last_not_of(" \t") + 1);
        route.targetId = mapTargetNameToId(target);

        // Resolve the complete declared name, then consume arithmetic in order.
        // Searching ahead for a '*' could otherwise discard an earlier bias.
        const auto expressionOperator = curveExpr.find_first_of("*+-");
        auto curveName = std::string_view{curveExpr}.substr(0, expressionOperator);
        const auto nameBegin = skipSpace(curveName, 0);
        curveName.remove_prefix(nameBegin);
        const auto nameEnd = curveName.find_last_not_of(" \t\r\n\f\v");
        curveName = curveName.substr(0, nameEnd == std::string_view::npos ? 0 : nameEnd + 1);
        const auto curve = modulation_curves::find(curveName);
        if (!curve) return {std::nullopt, ModulationRouteError::UnknownCurve};
        route.transferCurve = *curve;

        const bool hasScale = expressionOperator != std::string::npos && curveExpr[expressionOperator] == '*';
        const auto star = hasScale ? expressionOperator : std::string::npos;
        auto biasOperator = hasScale ? std::string::npos : expressionOperator;
        if (star != std::string::npos) {
            const auto number = parseNumber(curveExpr, star + 1);
            if (!number.valid)
                return {std::nullopt, ModulationRouteError::InvalidScale};
            const auto next = skipSpace(curveExpr, number.next);
            if (next != curveExpr.size() && curveExpr[next] != '+' && curveExpr[next] != '-')
                return {std::nullopt, ModulationRouteError::InvalidScale};
            if (!std::isfinite(number.value)) return {std::nullopt, ModulationRouteError::NonfiniteScale};
            route.scale = number.value;
            biasOperator = next == curveExpr.size() ? std::string::npos : next;
        }
        if (biasOperator != std::string::npos) {
            const auto number = parseNumber(curveExpr, biasOperator + 1);
            if (!number.valid || !onlySpace(curveExpr, number.next, curveExpr.size()))
                return {std::nullopt, ModulationRouteError::InvalidBias};
            if (!std::isfinite(number.value)) return {std::nullopt, ModulationRouteError::NonfiniteBias};
            route.bias = curveExpr[biasOperator] == '-' ? -number.value : number.value;
        }
    }

    if (route.sourceId == ModulationSourceId::None) return {std::nullopt, ModulationRouteError::UnknownSource};
    if (route.targetId == ModulationTargetId::None) return {std::nullopt, ModulationRouteError::UnknownTarget};
    return {route, ModulationRouteError::None};
}

} // namespace epi_sculpt
