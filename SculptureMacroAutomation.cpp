#include "SculptureMacroAutomation.h"
#include "SculptureJsonView.h"
#include "SculptureNumericEdit.h"

#include <cmath>
#include <charconv>
#include <iomanip>
#include <new>
#include <sstream>
#include <stdexcept>
#include <type_traits>

namespace epi_sculpt {

namespace {

constexpr float kPi = 3.14159265358979323846f;
constexpr float kTwoPi = 6.28318530717958647692f;

enum class ParameterClamp { None, Unit, Nonnegative, SignedUnit };
struct WriteBinding {
    MacroTarget target;
    float* destination;
    ParameterClamp clamp = ParameterClamp::None;
    bool supportsMultiplication = true;
};

// A synchronous, non-owning write set: consumers retain state/clock authority.
// N is three or nine here; scratch is fixed-size and never copies a full program.
template <std::size_t N>
bool tryApplyMacroWrites(
    const std::array<MacroAutomationLane, static_cast<std::size_t>(MacroTarget::Count)>& lanes,
    bool bypass, float masterAmount, std::uint32_t currentBar, float barPhase,
    const std::array<WriteBinding, N>& bindings) noexcept {
    if (bypass || masterAmount <= 1e-4f) return true;
    std::array<float, N> proposed{};
    std::array<bool, N> written{};
    for (const auto& lane : lanes) {
        if (!lane.isEnabled() || lane.isMuted() || lane.amount() <= 1e-4f) continue;
        const auto binding = std::find_if(bindings.begin(), bindings.end(),
            [&](const auto& value) { return value.target == lane.target(); });
        if (binding == bindings.end()) continue;
        if (lane.mode() == MacroModulationMode::Multiplicative && !binding->supportsMultiplication) continue;
        if (!std::isfinite(masterAmount) || !std::isfinite(lane.amount())) return false;

        const std::uint32_t bars = std::max(1U, lane.loopLengthBars());
        const std::uint32_t barInLoop = (currentBar - 1U) % bars;
        const float phase = (static_cast<float>(barInLoop) + barPhase) / static_cast<float>(bars);
        if (!std::isfinite(phase)) return false;
        if (lane.lfoShape() != AutomationLfoShape::None
            && (!std::isfinite(lane.lfoCyclesPerLoop()) || !std::isfinite(lane.lfoDepth())
                || !std::isfinite(lane.lfoPhaseOffset()))) return false;
        const float value = lane.evaluate(phase) * masterAmount;
        if (!std::isfinite(value)) return false;

        auto index = static_cast<std::size_t>(binding - bindings.begin());
        // Aliased output references share one candidate. The current target's
        // clamp still applies at each write, just as in ordinary lane execution.
        for (std::size_t prior = 0; prior < index; ++prior) {
            if (bindings[prior].destination == binding->destination) {
                index = prior;
                break;
            }
        }
        float candidate = value;
        switch (lane.mode()) {
        case MacroModulationMode::Absolute:
            break;
        case MacroModulationMode::Additive: {
            const auto sum = numeric_edit::prepareFiniteSum(
                written[index] ? proposed[index] : *binding->destination, value);
            if (!sum) return false;
            candidate = *sum;
            break;
        }
        case MacroModulationMode::Multiplicative: {
            const float before = written[index] ? proposed[index] : *binding->destination;
            if (!std::isfinite(before)) return false;
            candidate = before * value;
            break;
        }
        default:
            return false;
        }
        // Output clamps do not admit an overflowing arithmetic proposal.
        if (!std::isfinite(candidate)) return false;
        switch (binding->clamp) {
        case ParameterClamp::Unit: candidate = std::clamp(candidate, 0.0f, 1.0f); break;
        case ParameterClamp::SignedUnit: candidate = std::clamp(candidate, -1.0f, 1.0f); break;
        case ParameterClamp::Nonnegative: candidate = std::max(0.0f, candidate); break;
        case ParameterClamp::None: break;
        }
        proposed[index] = candidate;
        written[index] = true;
    }
    for (std::size_t index = 0; index < N; ++index)
        if (written[index]) *bindings[index].destination = proposed[index];
    return true;
}

inline float smoothstep(float edge0, float edge1, float x) noexcept {
    const float t = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

using MacroJsonReader = json_view::Reader;

template <typename Field>
bool macroObject(std::string_view text, Field&& field) {
    return json_view::object(text, [&](std::string_view key, std::string_view value) {
        // Macro schema keys retain their existing unescaped-spelling contract.
        return key.find('\\') == std::string_view::npos && field(key, value);
    });
}

template <typename Element>
bool macroArray(std::string_view text, Element&& element) {
    return json_view::array(text, element);
}

bool macroFloat(std::string_view text, float& value) noexcept {
    float candidate = 0.0f;
    const auto parsed = std::from_chars(text.data(), text.data() + text.size(), candidate);
    if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size()
        || !std::isfinite(candidate)) return false;
    value = candidate;
    return true;
}

bool macroUnsigned(std::string_view text, std::uint32_t& value) noexcept {
    const auto parsed = std::from_chars(text.data(), text.data() + text.size(), value);
    return parsed.ec == std::errc{} && parsed.ptr == text.data() + text.size();
}

bool macroBool(std::string_view text, bool& value) noexcept {
    if (text == "true" || text == "1") { value = true; return true; }
    if (text == "false" || text == "0") { value = false; return true; }
    return false;
}

// Recognized fields may appear once. Unknown fields remain ignorable extensions.
template <std::size_t N>
bool macroField(std::string_view key, const std::array<std::string_view, N>& names,
                std::uint32_t& seen, std::size_t& index) noexcept {
    for (index = 0U; index < N; ++index) {
        if (key != names[index]) continue;
        const auto bit = std::uint32_t{1} << index;
        if ((seen & bit) != 0U) return false;
        seen |= bit;
        break;
    }
    return true;
}

} // namespace

std::string_view macroTargetName(MacroTarget target) noexcept {
    switch (target) {
    case MacroTarget::CrossfadePosition:  return "CrossfadePosition";
    case MacroTarget::RotorAlpha:         return "RotorAlpha";
    case MacroTarget::RotorBeta:          return "RotorBeta";
    case MacroTarget::RotorTheta4D:       return "RotorTheta4D";
    case MacroTarget::TopologyBlend:      return "TopologyBlend";
    case MacroTarget::PatternSpeed:       return "PatternSpeed";
    case MacroTarget::LatentW:            return "LatentW";
    case MacroTarget::SeedPulseAmplitude: return "SeedPulseAmplitude";
    case MacroTarget::HueRotation:        return "HueRotation";
    case MacroTarget::MasterFilter:       return "MasterFilter";
    case MacroTarget::ChaosIntensity:     return "ChaosIntensity";
    case MacroTarget::VolumetricDensity:  return "VolumetricDensity";
    default:                              return "Unknown";
    }
}

// =============================================================================
// MacroAutomationLane Implementation
// =============================================================================

void MacroAutomationLane::addNode(float phase, float value, AutomationCurve curve) {
    enabled_ = true;
    const float p = std::clamp(phase, 0.0f, 0.999999f);
    AutomationNode newNode{p, value, curve};

    auto it = std::lower_bound(nodes_.begin(), nodes_.end(), newNode,
        [](const AutomationNode& a, const AutomationNode& b) {
            return a.phase < b.phase;
        });

    if (it != nodes_.end() && std::abs(it->phase - p) < 1e-5f) {
        *it = newNode; // Replace exact duplicate phase
    } else {
        nodes_.insert(it, newNode);
    }
}

void MacroAutomationLane::clearNodes() noexcept {
    nodes_.clear();
}

void MacroAutomationLane::recordValue(float loopPhase, float value, float minDistance) {
    enabled_ = true;
    const float p = std::clamp(loopPhase, 0.0f, 0.999999f);
    const float dist = std::max(0.001f, minDistance);

    for (auto& node : nodes_) {
        if (std::abs(node.phase - p) <= dist) {
            node.value = value;
            return;
        }
    }
    addNode(p, value, AutomationCurve::Smoothstep);
}

float MacroAutomationLane::evaluateLfo(AutomationLfoShape shape, float phase) noexcept {
    const float p = std::fmod(phase, 1.0f);
    const float normP = (p < 0.0f) ? (p + 1.0f) : p;

    switch (shape) {
    case AutomationLfoShape::Sine:
        return std::sin(normP * kTwoPi);
    case AutomationLfoShape::Triangle:
        return (normP < 0.5f) ? (4.0f * normP - 1.0f) : (3.0f - 4.0f * normP);
    case AutomationLfoShape::SawUp:
        return 2.0f * normP - 1.0f;
    case AutomationLfoShape::SawDown:
        return 1.0f - 2.0f * normP;
    case AutomationLfoShape::Square:
        return (normP < 0.5f) ? 1.0f : -1.0f;
    case AutomationLfoShape::EuclideanPulse: {
        // Exponential decay per beat onset
        const float decay = std::exp(-normP * 8.0f);
        return decay * 2.0f - 1.0f;
    }
    case AutomationLfoShape::None:
    default:
        return 0.0f;
    }
}

float MacroAutomationLane::evaluate(float loopPhase) const noexcept {
    if (!enabled_ || muted_) return 0.0f;

    float rawValue = 0.0f;

    if (lfoShape_ != AutomationLfoShape::None) {
        const float lfoPhase = std::fmod(loopPhase * lfoCycles_ + lfoPhaseOffset_, 1.0f);
        rawValue = evaluateLfo(lfoShape_, lfoPhase) * lfoDepth_;
    } else {
        if (nodes_.empty()) return 0.0f;
        if (nodes_.size() == 1U) return nodes_[0].value;

        const float p = std::clamp(loopPhase, 0.0f, 0.999999f);

        // Find bounding nodes
        std::size_t idx0 = 0U;
        std::size_t idx1 = 0U;
        bool found = false;

        for (std::size_t i = 0; i < nodes_.size() - 1U; ++i) {
            if (p >= nodes_[i].phase && p < nodes_[i + 1U].phase) {
                idx0 = i;
                idx1 = i + 1U;
                found = true;
                break;
            }
        }

        float t = 0.0f;
        float val0 = 0.0f;
        float val1 = 0.0f;
        AutomationCurve curve = AutomationCurve::Smoothstep;

        if (found) {
            val0 = nodes_[idx0].value;
            val1 = nodes_[idx1].value;
            curve = nodes_[idx0].curve;
            const float span = nodes_[idx1].phase - nodes_[idx0].phase;
            t = (span > 1e-6f) ? ((p - nodes_[idx0].phase) / span) : 0.0f;
        } else {
            // Wrap-around between last node and first node
            idx0 = nodes_.size() - 1U;
            idx1 = 0U;
            val0 = nodes_[idx0].value;
            val1 = nodes_[idx1].value;
            curve = nodes_[idx0].curve;
            const float span = (1.0f - nodes_[idx0].phase) + nodes_[idx1].phase;
            const float offset = (p >= nodes_[idx0].phase) ? (p - nodes_[idx0].phase) : ((1.0f - nodes_[idx0].phase) + p);
            t = (span > 1e-6f) ? (offset / span) : 0.0f;
        }

        switch (curve) {
        case AutomationCurve::Linear:
            rawValue = val0 + t * (val1 - val0);
            break;
        case AutomationCurve::Smoothstep:
            rawValue = val0 + smoothstep(0.0f, 1.0f, t) * (val1 - val0);
            break;
        case AutomationCurve::Step:
            rawValue = val0;
            break;
        case AutomationCurve::Exponential: {
            const float expT = (std::exp(3.0f * t) - 1.0f) / (std::exp(3.0f) - 1.0f);
            rawValue = val0 + expT * (val1 - val0);
            break;
        }
        }
    }

    return rawValue * amount_;
}

// =============================================================================
// SculptureMacroAutomationEngine Implementation
// =============================================================================

SculptureMacroAutomationEngine::SculptureMacroAutomationEngine() noexcept {
    for (std::size_t i = 0; i < static_cast<std::size_t>(MacroTarget::Count); ++i) {
        const auto target = static_cast<MacroTarget>(i);
        lanes_[i].setTarget(target);
        lanes_[i].setEnabled(false); // Inactive until explicitly configured or recorded
    }
}

MacroAutomationLane& SculptureMacroAutomationEngine::lane(MacroTarget target) {
    const auto idx = static_cast<std::size_t>(target);
    if (idx < lanes_.size()) {
        return lanes_[idx];
    }
    return lanes_[0];
}

const MacroAutomationLane* SculptureMacroAutomationEngine::findLane(MacroTarget target) const noexcept {
    const auto idx = static_cast<std::size_t>(target);
    if (idx < lanes_.size()) {
        return &lanes_[idx];
    }
    return nullptr;
}

bool SculptureMacroAutomationEngine::hasActiveLane(MacroTarget target) const noexcept {
    const auto* l = findLane(target);
    return l && l->isEnabled() && !l->isMuted();
}

void SculptureMacroAutomationEngine::clearAll() noexcept {
    for (auto& l : lanes_) {
        l.clearNodes();
        l.setLfoShape(AutomationLfoShape::None);
        l.setEnabled(false);
    }
}

void SculptureMacroAutomationEngine::update(
    std::uint32_t currentBar,
    float barPhase,
    float deltaSeconds
) noexcept {
    currentBar_ = currentBar;
    barPhase_ = barPhase;
    accumulatedTime_ += std::max(0.0f, deltaSeconds);
}

bool SculptureMacroAutomationEngine::tryApplyToParams(SculptureProgramParams& params) const noexcept {
    const std::array bindings{
        WriteBinding{MacroTarget::RotorAlpha, &params.rotorAlpha},
        WriteBinding{MacroTarget::RotorBeta, &params.rotorBeta},
        WriteBinding{MacroTarget::RotorTheta4D, &params.rotorTheta4D},
        WriteBinding{MacroTarget::TopologyBlend, &params.topologyBlend, ParameterClamp::Unit},
        WriteBinding{MacroTarget::PatternSpeed, &params.patternSpeed},
        WriteBinding{MacroTarget::LatentW, &params.latentW},
        WriteBinding{MacroTarget::SeedPulseAmplitude, &params.seedPulseAmplitude, ParameterClamp::Unit},
        WriteBinding{MacroTarget::HueRotation, &params.hueRotation},
        WriteBinding{MacroTarget::VolumetricDensity, &params.volumetricDensityScale, ParameterClamp::Nonnegative},
    };
    return tryApplyMacroWrites(lanes_, masterBypass_, masterAmount_, currentBar_, barPhase_, bindings);
}

void SculptureMacroAutomationEngine::applyToParams(SculptureProgramParams& params) const noexcept {
    static_cast<void>(tryApplyToParams(params));
}

bool SculptureMacroAutomationEngine::tryApplyToDeck(
    float& crossfadePos, float& masterFilter, float& chaosIntensity) const noexcept {
    const std::array bindings{
        WriteBinding{MacroTarget::CrossfadePosition, &crossfadePos, ParameterClamp::Unit, false},
        WriteBinding{MacroTarget::MasterFilter, &masterFilter, ParameterClamp::SignedUnit, false},
        WriteBinding{MacroTarget::ChaosIntensity, &chaosIntensity, ParameterClamp::Unit, false},
    };
    return tryApplyMacroWrites(lanes_, masterBypass_, masterAmount_, currentBar_, barPhase_, bindings);
}

void SculptureMacroAutomationEngine::applyToDeck(
    float& crossfadePos, float& masterFilter, float& chaosIntensity) const noexcept {
    static_cast<void>(tryApplyToDeck(crossfadePos, masterFilter, chaosIntensity));
}

std::string SculptureMacroAutomationEngine::formatStatus() const {
    std::ostringstream ss;
    ss << "[Macro Automation Engine] Master: "
       << (masterBypass_ ? "BYPASSED" : "ACTIVE")
       << " | Amount: " << std::fixed << std::setprecision(2) << masterAmount_ * 100.0f << "%\n";

    std::size_t activeCount = 0;
    for (const auto& l : lanes_) {
        if (l.isEnabled() && !l.isMuted()) {
            ++activeCount;
            ss << "  • " << macroTargetName(l.target())
               << " [" << l.loopLengthBars() << " bars]: ";
            if (l.lfoShape() != AutomationLfoShape::None) {
                ss << "LFO (" << static_cast<int>(l.lfoShape()) << ") freq=" << l.lfoCyclesPerLoop();
            } else {
                ss << l.nodeCount() << " nodes";
            }
            ss << " (amount=" << l.amount() << ")\n";
        }
    }

    if (activeCount == 0) {
        ss << "  (no active automation lanes)\n";
    }

    return ss.str();
}

std::string MacroAutomationLane::serializeJson() const {
    std::ostringstream ss;
    ss << "{\n"
       << "  \"target\": \"" << macroTargetName(target_) << "\",\n"
       << "  \"mode\": " << static_cast<int>(mode_) << ",\n"
       << "  \"loopBars\": " << loopLengthBars_ << ",\n"
       << "  \"enabled\": " << (enabled_ ? "true" : "false") << ",\n"
       << "  \"muted\": " << (muted_ ? "true" : "false") << ",\n"
       << "  \"amount\": " << amount_ << ",\n"
       << "  \"lfoShape\": " << static_cast<int>(lfoShape_) << ",\n"
       << "  \"lfoCycles\": " << lfoCycles_ << ",\n"
       << "  \"lfoDepth\": " << lfoDepth_ << ",\n"
       << "  \"lfoPhaseOffset\": " << lfoPhaseOffset_ << ",\n"
       << "  \"nodes\": [";
    for (std::size_t i = 0; i < nodes_.size(); ++i) {
        ss << "{\"p\":" << nodes_[i].phase << ",\"v\":" << nodes_[i].value << ",\"c\":" << static_cast<int>(nodes_[i].curve) << "}";
        if (i + 1 < nodes_.size()) ss << ",";
    }
    ss << "]\n}";
    return ss.str();
}

bool MacroAutomationLane::deserializeJson(std::string_view json) {
    try {
        auto candidate = *this;
        constexpr std::array<std::string_view, 11> fields{
            "target", "mode", "loopBars", "enabled", "muted", "amount", "lfoShape",
            "lfoCycles", "lfoDepth", "lfoPhaseOffset", "nodes"};
        std::uint32_t seen = 0U;
        std::optional<bool> declaredEnabled;
        const bool valid = macroObject(json, [&](std::string_view key, std::string_view value) {
            std::size_t field = 0U;
            if (!macroField(key, fields, seen, field)) return false;
            std::uint32_t integer = 0U;
            float number = 0.0f;
            switch (field) {
            case 0: {
                // A standalone lane keeps its existing target identity. The
                // engine selects a lane from this structural metadata member.
                MacroJsonReader reader(value);
                std::string_view target;
                return reader.string(target) && reader.done();
            }
            case 1:
                if (!macroUnsigned(value, integer) || integer > 2U) return false;
                candidate.mode_ = static_cast<MacroModulationMode>(integer);
                return true;
            case 2:
                if (!macroUnsigned(value, integer)) return false;
                candidate.loopLengthBars_ = std::max(1U, integer);
                return true;
            case 3: {
                bool enabled = false;
                if (!macroBool(value, enabled)) return false;
                declaredEnabled = enabled;
                return true;
            }
            case 4: return macroBool(value, candidate.muted_);
            case 5:
                if (!macroFloat(value, number)) return false;
                candidate.amount_ = std::clamp(number, 0.0f, 1.0f);
                return true;
            case 6:
                if (!macroUnsigned(value, integer) || integer > 6U) return false;
                candidate.lfoShape_ = static_cast<AutomationLfoShape>(integer);
                return true;
            case 7: return macroFloat(value, candidate.lfoCycles_);
            case 8: return macroFloat(value, candidate.lfoDepth_);
            case 9: return macroFloat(value, candidate.lfoPhaseOffset_);
            case 10:
                candidate.clearNodes();
                return macroArray(value, [&](std::string_view node) {
                    float phase = 0.0f, amplitude = 0.0f;
                    std::uint32_t curve = 1U, nodeSeen = 0U;
                    constexpr std::array<std::string_view, 3> nodeFields{"p", "v", "c"};
                    if (!macroObject(node, [&](std::string_view name, std::string_view token) {
                            std::size_t index = 0U;
                            if (!macroField(name, nodeFields, nodeSeen, index)) return false;
                            switch (index) {
                            case 0: return macroFloat(token, phase);
                            case 1: return macroFloat(token, amplitude);
                            case 2: return macroUnsigned(token, curve) && curve <= 3U;
                            default: return true;
                            }
                        })) return false;
                    // Conversion is now integral and range-checked, never a
                    // float-to-int cast of untrusted persisted data.
                    candidate.addNode(phase, amplitude, static_cast<AutomationCurve>(curve));
                    return true;
                });
            default: return true;
            }
        });
        if (!valid) return false;
        // addNode enables live recording; an explicitly persisted enable flag
        // describes the final imported state, independent of member ordering.
        if (declaredEnabled) candidate.enabled_ = *declaredEnabled;
        static_assert(std::is_nothrow_move_assignable_v<MacroAutomationLane>);
        *this = std::move(candidate);
        return true;
    } catch (const std::bad_alloc&) {
        return false;
    } catch (const std::length_error&) {
        return false;
    }
}

std::string SculptureMacroAutomationEngine::serializeJson() const {
    std::ostringstream ss;
    ss << "{\n"
       << "  \"masterAmount\": " << masterAmount_ << ",\n"
       << "  \"masterBypass\": " << (masterBypass_ ? "true" : "false") << ",\n"
       << "  \"lanes\": [\n";

    bool first = true;
    for (const auto& l : lanes_) {
        if (l.nodeCount() > 0 || l.lfoShape() != AutomationLfoShape::None) {
            if (!first) ss << ",\n";
            first = false;
            ss << l.serializeJson();
        }
    }
    ss << "\n  ]\n}";
    return ss.str();
}

bool SculptureMacroAutomationEngine::deserializeJson(std::string_view json) {
    try {
        auto candidate = *this;
        constexpr std::array<std::string_view, 3> fields{"masterAmount", "masterBypass", "lanes"};
        std::uint32_t seen = 0U, laneSeen = 0U;
        const bool valid = macroObject(json, [&](std::string_view key, std::string_view value) {
            std::size_t field = 0U;
            if (!macroField(key, fields, seen, field)) return false;
            if (field == 0U) {
                float amount = 0.0f;
                if (!macroFloat(value, amount)) return false;
                candidate.masterAmount_ = std::clamp(amount, 0.0f, 1.0f);
            } else if (field == 1U) {
                return macroBool(value, candidate.masterBypass_);
            } else if (field == 2U) {
                return macroArray(value, [&](std::string_view laneJson) {
                    std::optional<MacroTarget> selected;
                    bool targetSeen = false;
                    if (!macroObject(laneJson, [&](std::string_view name, std::string_view token) {
                            if (name != "target") return true;
                            if (targetSeen) return false;
                            targetSeen = true;
                            MacroJsonReader reader(token);
                            std::string_view target;
                            if (!reader.string(target) || !reader.done()) return false;
                            for (std::size_t t = 0; t < static_cast<std::size_t>(MacroTarget::Count); ++t) {
                                const auto identity = static_cast<MacroTarget>(t);
                                if (target == macroTargetName(identity)) selected = identity;
                            }
                            return true;
                        })) return false;
                    // Preserve forward-compatible omission of unknown targets;
                    // target-like strings in unrelated fields never select a lane.
                    if (!selected) return true;
                    const auto bit = std::uint32_t{1} << static_cast<unsigned>(*selected);
                    if ((laneSeen & bit) != 0U) return false;
                    laneSeen |= bit;
                    return candidate.lane(*selected).deserializeJson(laneJson);
                });
            }
            return true;
        });
        if (!valid) return false;
        static_assert(std::is_nothrow_move_assignable_v<SculptureMacroAutomationEngine>);
        *this = std::move(candidate);
        return true;
    } catch (const std::bad_alloc&) {
        return false;
    } catch (const std::length_error&) {
        return false;
    }
}

} // namespace epi_sculpt
