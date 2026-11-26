#pragma once

#include "SculptureProgramSchema.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <functional>
#include <numbers>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace epi_sculpt {

// =============================================================================
// Macro Automation Targets
// =============================================================================
enum class MacroTarget : std::uint8_t {
    CrossfadePosition = 0,
    RotorAlpha = 1,
    RotorBeta = 2,
    RotorTheta4D = 3,
    TopologyBlend = 4,
    PatternSpeed = 5,
    LatentW = 6,
    SeedPulseAmplitude = 7,
    HueRotation = 8,
    MasterFilter = 9,
    ChaosIntensity = 10,
    VolumetricDensity = 11,
    Count = 12
};

[[nodiscard]] std::string_view macroTargetName(MacroTarget target) noexcept;

enum class AutomationCurve : std::uint8_t {
    Linear = 0,
    Smoothstep = 1,    // Hermite cubic 3x^2 - 2x^3
    Step = 2,          // Sample-and-hold instant switch
    Exponential = 3    // Smooth exponential rise/decay
};

enum class AutomationLfoShape : std::uint8_t {
    None = 0,          // Use authored/recorded keyframe nodes
    Sine = 1,
    Triangle = 2,
    SawUp = 3,
    SawDown = 4,
    Square = 5,
    EuclideanPulse = 6
};

enum class MacroModulationMode : std::uint8_t {
    Absolute = 0,      // Replaces parameter with evaluated macro value
    Additive = 1,      // Adds macro offset to base parameter value
    Multiplicative = 2 // Multiplies base parameter by macro value
};

// =============================================================================
// Automation Node (Phase Keyframe)
// =============================================================================
struct AutomationNode {
    float phase = 0.0f; // [0.0, 1.0) normalized position within loop
    float value = 0.0f; // Target parameter value
    AutomationCurve curve = AutomationCurve::Smoothstep;
};

// =============================================================================
// Macro Automation Lane
// =============================================================================
class MacroAutomationLane {
public:
    MacroAutomationLane() noexcept = default;
    explicit MacroAutomationLane(MacroTarget target,
                                 MacroModulationMode mode = MacroModulationMode::Absolute,
                                 std::uint32_t loopLengthBars = 4) noexcept
        : target_(target), mode_(mode), loopLengthBars_(std::max(1U, loopLengthBars)) {}

    [[nodiscard]] MacroTarget target() const noexcept { return target_; }
    void setTarget(MacroTarget target) noexcept { target_ = target; }

    [[nodiscard]] MacroModulationMode mode() const noexcept { return mode_; }
    void setMode(MacroModulationMode mode) noexcept { mode_ = mode; }

    [[nodiscard]] std::uint32_t loopLengthBars() const noexcept { return loopLengthBars_; }
    void setLoopLengthBars(std::uint32_t bars) noexcept { loopLengthBars_ = std::max(1U, bars); }

    [[nodiscard]] bool isEnabled() const noexcept { return enabled_; }
    void setEnabled(bool enabled) noexcept { enabled_ = enabled; }

    [[nodiscard]] bool isMuted() const noexcept { return muted_; }
    void setMuted(bool muted) noexcept { muted_ = muted; }

    [[nodiscard]] float amount() const noexcept { return amount_; }
    void setAmount(float amount) noexcept { amount_ = std::clamp(amount, 0.0f, 1.0f); }

    // LFO generator configuration
    [[nodiscard]] AutomationLfoShape lfoShape() const noexcept { return lfoShape_; }
    void setLfoShape(AutomationLfoShape shape) noexcept {
        lfoShape_ = shape;
        if (shape != AutomationLfoShape::None) enabled_ = true;
    }

    [[nodiscard]] float lfoCyclesPerLoop() const noexcept { return lfoCycles_; }
    void setLfoCyclesPerLoop(float cycles) noexcept { lfoCycles_ = cycles; }

    [[nodiscard]] float lfoDepth() const noexcept { return lfoDepth_; }
    void setLfoDepth(float depth) noexcept { lfoDepth_ = depth; }

    [[nodiscard]] float lfoPhaseOffset() const noexcept { return lfoPhaseOffset_; }
    void setLfoPhaseOffset(float offset) noexcept { lfoPhaseOffset_ = offset; }

    // Keyframe nodes management
    void addNode(float phase, float value, AutomationCurve curve = AutomationCurve::Smoothstep);
    void clearNodes() noexcept;
    [[nodiscard]] const std::vector<AutomationNode>& nodes() const noexcept { return nodes_; }
    [[nodiscard]] std::size_t nodeCount() const noexcept { return nodes_.size(); }

    // Live punch-in recording (inserts/replaces node with minimum phase separation)
    void recordValue(float loopPhase, float value, float minDistance = 0.02f);

    // Evaluates lane value at normalized loop phase [0.0, 1.0)
    [[nodiscard]] float evaluate(float loopPhase) const noexcept;

    // Evaluates LFO waveform at normalized phase [0.0, 1.0)
    [[nodiscard]] static float evaluateLfo(AutomationLfoShape shape, float phase) noexcept;

    // Bidirectional JSON persistence. Import patches a private candidate;
    // malformed values or allocation failure return false without mutation.
    // Numeric fields must be finite; integer/enums require integral tokens.
    [[nodiscard]] std::string serializeJson() const;
    bool deserializeJson(std::string_view json);

private:
    MacroTarget target_ = MacroTarget::CrossfadePosition;
    MacroModulationMode mode_ = MacroModulationMode::Absolute;
    std::uint32_t loopLengthBars_ = 4;
    bool enabled_ = true;
    bool muted_ = false;
    float amount_ = 1.0f;

    AutomationLfoShape lfoShape_ = AutomationLfoShape::None;
    float lfoCycles_ = 1.0f;
    float lfoDepth_ = 1.0f;
    float lfoPhaseOffset_ = 0.0f;

    std::vector<AutomationNode> nodes_{};
};

// =============================================================================
// Sculpture Macro Automation Engine
// =============================================================================
class SculptureMacroAutomationEngine {
public:
    SculptureMacroAutomationEngine() noexcept;

    // Access or create lane for a target
    [[nodiscard]] MacroAutomationLane& lane(MacroTarget target);
    [[nodiscard]] const MacroAutomationLane* findLane(MacroTarget target) const noexcept;
    [[nodiscard]] bool hasActiveLane(MacroTarget target) const noexcept;

    // Master intensity / bypass
    void setMasterAmount(float amount) noexcept {
        // A clamp is output policy; invalid edits must retain the live program.
        if (!std::isfinite(amount)) return;
        masterAmount_ = std::clamp(amount, 0.0f, 1.0f);
    }
    [[nodiscard]] float masterAmount() const noexcept { return masterAmount_; }
    void setMasterBypass(bool bypass) noexcept { masterBypass_ = bypass; }
    [[nodiscard]] bool isMasterBypassed() const noexcept { return masterBypass_; }

    void clearAll() noexcept;

    // Real-time musical clock tick
    void update(std::uint32_t currentBar, float barPhase, float deltaSeconds) noexcept;

    // Prepares all active program-target writes before committing. Refusal
    // leaves params unchanged; lane/clock advancement is owned separately.
    [[nodiscard]] bool tryApplyToParams(SculptureProgramParams& params) const noexcept;
    // Existing consumers retain a local hold on refusal, with no clock rollback.
    void applyToParams(SculptureProgramParams& params) const noexcept;

    // Same local admission policy for deck controls; unsupported multiplication
    // remains a no-op. Output references may alias and retain lane-order meaning.
    [[nodiscard]] bool tryApplyToDeck(float& crossfadePos, float& masterFilter, float& chaosIntensity) const noexcept;
    void applyToDeck(float& crossfadePos, float& masterFilter, float& chaosIntensity) const noexcept;

    // Diagnostic status summary
    [[nodiscard]] std::string formatStatus() const;

    // Full Engine Bidirectional JSON persistence. Import commits all recognized
    // lanes together; any rejected lane preserves the complete previous engine.
    [[nodiscard]] std::string serializeJson() const;
    bool deserializeJson(std::string_view json);

private:
    std::array<MacroAutomationLane, static_cast<std::size_t>(MacroTarget::Count)> lanes_{};
    float masterAmount_ = 1.0f;
    bool masterBypass_ = false;
    std::uint32_t currentBar_ = 1;
    float barPhase_ = 0.0f;
    float accumulatedTime_ = 0.0f;
};

} // namespace epi_sculpt
