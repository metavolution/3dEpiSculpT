#include "SculpturePerformanceDeck.h"
#include "SculptureNumericEdit.h"

#include <algorithm>
#include <cstdio>
#include <limits>
#include <numeric>
#include <sstream>

namespace epi_sculpt {

SculpturePerformanceDeck::SculpturePerformanceDeck() noexcept {
    tapTimestamps_.reserve(kMaxTapHistory);
}

void SculpturePerformanceDeck::loadDeck(DeckId deck, const SculptureProgramParams& params,
                                        std::string_view presetId, std::string_view title) {
    DeckSlot& slot = (deck == DeckId::DeckA) ? deckA_ : deckB_;
    slot.params = params;
    slot.presetId = std::string(presetId);
    slot.presetTitle = title.empty() ? (deck == DeckId::DeckA ? "Deck A" : "Deck B") : std::string(title);
    if (prewarmCallback_) {
        prewarmCallback_(deck, params);
    }
}

void SculpturePerformanceDeck::loadDeck(DeckId deck, const SculpturePresetAsset& asset) {
    loadDeck(deck, asset.params, asset.id, asset.title);
}

void SculpturePerformanceDeck::swapDecks() noexcept {
    std::swap(deckA_, deckB_);
    crossfadePos_ = 1.0f - crossfadePos_;
}

void SculpturePerformanceDeck::setCrossfadePosition(float position) noexcept {
    if (!std::isfinite(position)) return;
    crossfadePos_ = std::clamp(position, 0.0f, 1.0f);
}

void SculpturePerformanceDeck::nudgeCrossfade(float delta) noexcept {
    numeric_edit::addFiniteClamped(crossfadePos_, delta, 0.0f, 1.0f);
}

std::pair<float, float> SculpturePerformanceDeck::computeDeckGains(
    float pos, CrossfadeCurve curve) noexcept {
    const float x = std::clamp(pos, 0.0f, 1.0f);
    switch (curve) {
    case CrossfadeCurve::Linear:
        return {1.0f - x, x};

    case CrossfadeCurve::EqualPower: {
        // Equal power curve ensures cos^2 + sin^2 = 1.0, eliminating perceived energy dips
        constexpr float kHalfPi = 1.57079632679f;
        const float angle = x * kHalfPi;
        return {std::clamp(std::cos(angle), 0.0f, 1.0f), std::clamp(std::sin(angle), 0.0f, 1.0f)};
    }

    case CrossfadeCurve::Smoothstep: {
        const float s = x * x * (3.0f - 2.0f * x);
        return {1.0f - s, s};
    }

    case CrossfadeCurve::SharpCut: {
        if (x < 0.48f) return {1.0f, 0.0f};
        if (x > 0.52f) return {0.0f, 1.0f};
        return {0.7071f, 0.7071f};
    }
    }
    return {1.0f - x, x};
}

SculptureProgramParams SculpturePerformanceDeck::composeMasterParams(
    SculptureCompositionAdmission* macroAdmission) const {
    const auto [gainA, gainB] = computeDeckGains(crossfadePos_, crossfadeCurve_);

    SculptureProgramParams params;
    if (gainB <= 1e-4f) {
        params = deckA_.params;
    } else if (gainA <= 1e-4f) {
        params = deckB_.params;
    } else {
        // Morph interpolation weight normalized between active deck weights
        const float sum = gainA + gainB;
        const float t = (sum > 1e-6f) ? std::clamp(gainB / sum, 0.0f, 1.0f) : crossfadePos_;
        params = SculptureMorphChoreographer::interpolateParams(deckA_.params, deckB_.params, t);
    }

    // -------------------------------------------------------------------------
    // VJ Master Bus Performance FX Processing
    // -------------------------------------------------------------------------
    // 1. Tape Stop / Vinyl Brake
    if (masterFx_.tapeStopActive) {
        params.patternSpeed *= masterFx_.tapeStopSpeedFactor;
    }

    // 2. Rhythmic Beat Roll / Stutter Loop
    if (masterFx_.beatRollActive && masterFx_.beatRollSubdivision > 1e-4f) {
        params.patternSpeed = 0.0f;
        const float sub = masterFx_.beatRollSubdivision;
        const float loopPhase = std::fmod(beatPhase_, sub) / sub;
        params.latentW += std::sin(loopPhase * 6.2831853f) * 0.15f;
        params.seedPulseAmplitude = std::max(params.seedPulseAmplitude, 0.6f);
    }

    // 3. Chaos / Rotor Turbulence Burst
    if (masterFx_.chaosIntensity > 1e-4f) {
        const float c = masterFx_.chaosIntensity;
        params.rotorAlpha += std::sin(accumulatedTime_ * 18.0f) * c * 0.45f;
        params.rotorBeta += std::cos(accumulatedTime_ * 14.0f) * c * 0.35f;
        params.rotorTheta4D += std::sin(accumulatedTime_ * 22.0f) * c * 0.50f;
        params.lumaDrift += c * 0.25f;
        params.chromaBreath = std::min(1.0f, params.chromaBreath + c * 0.4f);
    }

    // 4. Master Isolator / Bipolar Filter (-1.0 LowPass, 0.0 Bypass, +1.0 HighPass)
    if (masterFx_.masterFilter != 0.0f) {
        if (masterFx_.masterFilter < 0.0f) {
            const float lp = -masterFx_.masterFilter; // [0, 1]
            params.fogAnisotropy = std::clamp(params.fogAnisotropy + lp * 0.4f, -0.9f, 0.9f);
            params.volumetricDensityScale *= (1.0f + lp * 0.8f);
            params.hueSpeed *= (1.0f - lp * 0.7f);
        } else {
            const float hp = masterFx_.masterFilter; // [0, 1]
            params.seedWidth *= (1.0f - hp * 0.5f);
            params.filmThickness = std::clamp(params.filmThickness + hp * 250.0f, 100.0f, 1200.0f);
            params.volumetricDensityScale *= (1.0f - hp * 0.5f);
        }
    }

    // 5. Musical Macro Automation Processing
    const bool admitted = macroEngine_.tryApplyToParams(params);
    if (macroAdmission) *macroAdmission = compositionAdmission(admitted);

    return params;
}

void SculpturePerformanceDeck::triggerAutoCrossfade(float durationSeconds, std::optional<DeckId> targetDeck) {
    if (!std::isfinite(durationSeconds)) return;
    // Only an admitted request can replace the existing time/beat transition.
    isBeatSyncCrossfade_ = false;
    pendingDownbeat_ = false;
    autoCrossfading_ = true;
    autoCrossfadeStartPos_ = crossfadePos_;
    if (targetDeck.has_value()) {
        autoCrossfadeTargetPos_ = (*targetDeck == DeckId::DeckA) ? 0.0f : 1.0f;
    } else {
        // Toggle to the opposite deck
        autoCrossfadeTargetPos_ = (crossfadePos_ < 0.5f) ? 1.0f : 0.0f;
    }
    autoCrossfadeDuration_ = std::max(durationSeconds, 0.1f);
    autoCrossfadeElapsed_ = 0.0f;
    autoCrossfadeProgress_ = 0.0f;
}

void SculpturePerformanceDeck::cancelAutoCrossfade() noexcept {
    autoCrossfading_ = false;
    isBeatSyncCrossfade_ = false;
    pendingDownbeat_ = false;
    autoCrossfadeProgress_ = 0.0f;
}

void SculpturePerformanceDeck::triggerBeatSyncedCrossfade(
    std::uint32_t barCount,
    std::optional<DeckId> targetDeck,
    bool snapToNextDownbeat
) {
    if (barCount == 0) barCount = 1;
    beatSyncTargetBars_ = barCount;

    if (snapToNextDownbeat && !isDownbeat_) {
        pendingDownbeat_ = true;
        pendingTargetDeck_ = targetDeck;
        return;
    }

    pendingDownbeat_ = false;
    autoCrossfading_ = true;
    isBeatSyncCrossfade_ = true;
    autoCrossfadeStartPos_ = crossfadePos_;

    if (targetDeck.has_value()) {
        autoCrossfadeTargetPos_ = (*targetDeck == DeckId::DeckA) ? 0.0f : 1.0f;
    } else {
        autoCrossfadeTargetPos_ = (crossfadePos_ <= 0.5f) ? 1.0f : 0.0f;
    }

    // Convert before scaling: every admitted uint32 bar count must stay positive.
    beatSyncTotalBeats_ = static_cast<float>(barCount) * 4.0f;
    beatSyncElapsedBeats_ = 0.0f;
    autoCrossfadeProgress_ = 0.0f;
}

float SculpturePerformanceDeck::beatSine(float cyclesPerBar) const noexcept {
    constexpr float kTwoPi = 2.0f * std::numbers::pi_v<float>;
    return std::sin(barPhase() * cyclesPerBar * kTwoPi);
}

float SculpturePerformanceDeck::beatTri(float cyclesPerBar) const noexcept {
    const float phase = std::fmod(barPhase() * cyclesPerBar, 1.0f);
    return 1.0f - 4.0f * std::abs(phase - 0.5f);
}

void SculpturePerformanceDeck::tapTempo(float explicitTimestamp) noexcept {
    if (!std::isfinite(explicitTimestamp)) return;
    const float now = (explicitTimestamp >= 0.0f) ? explicitTimestamp : accumulatedTime_;
    if (!std::isfinite(now)) return;

    if (!tapTimestamps_.empty()) {
        const float delta = now - tapTimestamps_.back();
        // Reject rapid bounces (< 150ms)
        if (delta < 0.15f) {
            return;
        }
        // Reset tap history if interval exceeds 2.5s (new phrase)
        if (delta > 2.5f) {
            tapTimestamps_.clear();
        }
    }

    tapTimestamps_.push_back(now);
    if (tapTimestamps_.size() > kMaxTapHistory) {
        tapTimestamps_.erase(tapTimestamps_.begin());
    }

    if (tapTimestamps_.size() >= 2) {
        float totalDelta = 0.0f;
        for (std::size_t i = 1; i < tapTimestamps_.size(); ++i) {
            totalDelta += (tapTimestamps_[i] - tapTimestamps_[i - 1]);
        }
        const float avgInterval = totalDelta / static_cast<float>(tapTimestamps_.size() - 1);
        if (avgInterval > 0.05f) {
            const float computedBpm = 60.0f / avgInterval;
            setBpm(computedBpm);
        }
    }
}

void SculpturePerformanceDeck::setBpm(float bpm) noexcept {
    if (std::isfinite(bpm)) bpm_ = std::clamp(bpm, 30.0f, 300.0f);
}

void SculpturePerformanceDeck::resetBeatPhase() noexcept {
    beatPhase_ = 0.0f;
    totalBeats_ = 0U;
    isDownbeat_ = true;
}

void SculpturePerformanceDeck::storeCue(std::size_t bankIndex, const SculptureProgramParams& params,
                                        std::string_view name, std::string_view presetId) {
    if (bankIndex >= kMaxCueBanks) return;
    cueBanks_[bankIndex] = PerformanceCuePoint{
        .params = params,
        .name = name.empty() ? ("Cue " + std::to_string(bankIndex + 1)) : std::string(name),
        .presetId = std::string(presetId),
        .active = true
    };
}

bool SculpturePerformanceDeck::recallCue(std::size_t bankIndex, DeckId targetDeck) {
    const auto* cuePoint = cue(bankIndex);
    if (cuePoint == nullptr) return false;
    loadDeck(targetDeck, cuePoint->params, cuePoint->presetId, cuePoint->name);
    return true;
}

bool SculpturePerformanceDeck::recallCueToOppositeDeck(std::size_t bankIndex) {
    // If crossfader is biased to Deck A (<= 0.5), load cue into Deck B for upcoming transition
    const DeckId target = (crossfadePos_ <= 0.5f) ? DeckId::DeckB : DeckId::DeckA;
    return recallCue(bankIndex, target);
}

const PerformanceCuePoint* SculpturePerformanceDeck::cue(std::size_t bankIndex) const noexcept {
    if (bankIndex >= kMaxCueBanks || !cueBanks_[bankIndex].active) {
        return nullptr;
    }
    return &cueBanks_[bankIndex];
}

// -----------------------------------------------------------------------------
// VJ Master Bus Performance FX Implementations
// -----------------------------------------------------------------------------
void SculpturePerformanceDeck::triggerTapeStop(float durationSeconds) noexcept {
    if (!std::isfinite(durationSeconds)) return;
    masterFx_.tapeStopActive = true;
    masterFx_.tapeStopDuration = std::max(durationSeconds, 0.1f);
    masterFx_.tapeStopElapsed = 0.0f;
    masterFx_.tapeStopSpeedFactor = 1.0f;
}

void SculpturePerformanceDeck::releaseTapeStop() noexcept {
    masterFx_.tapeStopActive = false;
    masterFx_.tapeStopElapsed = 0.0f;
    masterFx_.tapeStopSpeedFactor = 1.0f;
}

bool SculpturePerformanceDeck::isTapeStopActive() const noexcept {
    return masterFx_.tapeStopActive;
}

float SculpturePerformanceDeck::tapeStopSpeedFactor() const noexcept {
    return masterFx_.tapeStopSpeedFactor;
}

void SculpturePerformanceDeck::triggerBeatRoll(float subdivision, float durationBeats) noexcept {
    if (!std::isfinite(subdivision) || !std::isfinite(durationBeats)) return;
    masterFx_.beatRollActive = true;
    masterFx_.beatRollSubdivision = std::clamp(subdivision, 0.0625f, 1.0f);
    masterFx_.beatRollDurationBeats = std::max(durationBeats, 0.25f);
    masterFx_.beatRollElapsedBeats = 0.0f;
}

void SculpturePerformanceDeck::releaseBeatRoll() noexcept {
    masterFx_.beatRollActive = false;
    masterFx_.beatRollElapsedBeats = 0.0f;
}

bool SculpturePerformanceDeck::isBeatRollActive() const noexcept {
    return masterFx_.beatRollActive;
}

void SculpturePerformanceDeck::triggerChaosBurst(float intensity, float decayRate) noexcept {
    if (!std::isfinite(intensity) || !std::isfinite(decayRate)) return;
    masterFx_.chaosIntensity = std::clamp(intensity, 0.0f, 2.0f);
    masterFx_.chaosDecayRate = std::max(decayRate, 0.1f);
}

float SculpturePerformanceDeck::chaosIntensity() const noexcept {
    return masterFx_.chaosIntensity;
}

void SculpturePerformanceDeck::setMasterFilter(float filterValue) noexcept {
    if (!std::isfinite(filterValue)) return;
    masterFx_.masterFilter = std::clamp(filterValue, -1.0f, 1.0f);
}

float SculpturePerformanceDeck::masterFilter() const noexcept {
    return masterFx_.masterFilter;
}

void SculpturePerformanceDeck::applyMasterFx(SculptureProgramParams& params, float time, float& deltaSeconds) const noexcept {
    // 1. Tape Stop: Spin-down decrescendo & temporal freeze
    if (masterFx_.tapeStopActive) {
        deltaSeconds *= masterFx_.tapeStopSpeedFactor;
        params.patternSpeed *= masterFx_.tapeStopSpeedFactor;
    }

    // 2. Beat Roll: Fast sub-beat phase loop & high-energy stutter
    if (masterFx_.beatRollActive && masterFx_.beatRollSubdivision > 1e-4f) {
        const float rollPhase = std::fmod(beatPhase_, masterFx_.beatRollSubdivision) / masterFx_.beatRollSubdivision;
        params.seedPulseAmplitude += (1.0f - rollPhase) * 0.45f;
        params.curlStrength += 0.25f * std::sin(rollPhase * 6.2831853f);
    }

    // 3. Chaos Burst: Deterministic high-entropy perturbation of orbit and 4D coordinates
    if (masterFx_.chaosIntensity > 0.001f) {
        params.seedPulseAmplitude += masterFx_.chaosIntensity * 0.65f;
        params.worldScale *= (1.0f + 0.12f * masterFx_.chaosIntensity * std::sin(time * 35.0f));
        params.latentW += 0.22f * masterFx_.chaosIntensity * std::cos(time * 45.0f);
        params.curlStrength += masterFx_.chaosIntensity * 0.45f;
        params.hueRotation += masterFx_.chaosIntensity * 0.5f;
    }

    // 4. Master Isolator Filter: Low-Pass (<0) / High-Pass (>0)
    if (std::abs(masterFx_.masterFilter) > 0.001f) {
        if (masterFx_.masterFilter < 0.0f) {
            // Low-Pass (-1.0 to 0.0): Dampen high frequencies, pull hue back
            const float damp = 1.0f + masterFx_.masterFilter * 0.75f;
            params.seedPulseFrequency *= damp;
            params.seedAlpha *= damp;
            params.radiusScale *= damp;
        } else {
            // High-Pass (0.0 to +1.0): Boost seed sharpness, edge luminance
            const float boost = 1.0f + masterFx_.masterFilter * 0.85f;
            params.seedAlpha = std::clamp(params.seedAlpha * boost, 0.0f, 1.0f);
            params.seedPulseAmplitude *= boost;
            params.hueRotation += masterFx_.masterFilter * 0.4f * time;
        }
    }
}

bool SculpturePerformanceDeck::tryUpdate(float deltaSeconds,
    SculptureCompositionAdmission* macroAdmission) noexcept {
    if (macroAdmission) *macroAdmission = SculptureCompositionAdmission::NotEvaluated;
    if (!std::isfinite(deltaSeconds)) return false;
    deltaSeconds = std::max(0.0f, deltaSeconds);
    if (!std::isfinite(bpm_) || bpm_ <= 0.0f || !std::isfinite(beatPhase_)
        || beatPhase_ < 0.0f || beatPhase_ >= 1.0f) return false;

    // Preserve ordinary float arithmetic, but prepare the complete clock before
    // advancing any effect or callback. Rejection belongs to this owner only.
    const float nextTime = accumulatedTime_ + deltaSeconds;
    const float beatsThisTick = deltaSeconds * (bpm_ / 60.0f);
    const float nextPhase = beatPhase_ + beatsThisTick;
    if (!std::isfinite(nextTime) || !std::isfinite(beatsThisTick) || !std::isfinite(nextPhase)) return false;

    // currentBar() is 1-based uint32. Keeping total/4 below UINT32_MAX also
    // bounds the float-to-integer conversion and prevents total-beat overflow.
    constexpr std::uint64_t maxTotalBeats =
        static_cast<std::uint64_t>(std::numeric_limits<std::uint32_t>::max()) * 4U - 1U;
    if (totalBeats_ > maxTotalBeats) return false;
    const double exclusiveBeatLimit = static_cast<double>(maxTotalBeats - totalBeats_) + 1.0;
    if (static_cast<double>(nextPhase) >= exclusiveBeatLimit) return false;
    const bool crossedBeat = nextPhase >= 1.0f;
    const auto wholeBeats = crossedBeat ? static_cast<std::uint64_t>(nextPhase) : 0U;
    const auto nextTotalBeats = totalBeats_ + wholeBeats;
    const float nextBeatPhase = crossedBeat ? std::fmod(nextPhase, 1.0f) : nextPhase;
    if (!std::isfinite(nextBeatPhase)) return false;

    accumulatedTime_ = nextTime;
    totalBeats_ = nextTotalBeats;
    beatPhase_ = nextBeatPhase;
    isDownbeat_ = crossedBeat && (nextTotalBeats % 4U == 0U);
    // Existing end-of-step downbeat semantics and envelope order remain intact.
    downbeatEnvelope_ = std::max(0.0f, downbeatEnvelope_ - deltaSeconds * 7.0f);
    if (isDownbeat_) downbeatEnvelope_ = 1.0f;

    // Advance Master Performance FX
    if (masterFx_.tapeStopActive) {
        masterFx_.tapeStopElapsed += deltaSeconds;
        if (masterFx_.tapeStopElapsed >= masterFx_.tapeStopDuration) {
            releaseTapeStop();
        } else {
            const float tNorm = masterFx_.tapeStopElapsed / masterFx_.tapeStopDuration;
            masterFx_.tapeStopSpeedFactor = (1.0f - tNorm) * (1.0f - tNorm);
        }
    }

    if (masterFx_.beatRollActive) {
        masterFx_.beatRollElapsedBeats += beatsThisTick;
        if (masterFx_.beatRollElapsedBeats >= masterFx_.beatRollDurationBeats) {
            releaseBeatRoll();
        }
    }

    if (masterFx_.chaosIntensity > 0.0f) {
        masterFx_.chaosIntensity = std::max(0.0f, masterFx_.chaosIntensity - deltaSeconds * masterFx_.chaosDecayRate);
    }

    bool crossfadeStartedThisTick = false;
    // Check queued downbeat transition trigger
    if (pendingDownbeat_ && isDownbeat_) {
        triggerBeatSyncedCrossfade(beatSyncTargetBars_, pendingTargetDeck_, false);
        crossfadeStartedThisTick = true;
    }

    // Advance crossfade transition
    if (autoCrossfading_ && !crossfadeStartedThisTick) {
        if (isBeatSyncCrossfade_) {
            beatSyncElapsedBeats_ += beatsThisTick;
            autoCrossfadeProgress_ = std::clamp(beatSyncElapsedBeats_ / beatSyncTotalBeats_, 0.0f, 1.0f);
            crossfadePos_ = autoCrossfadeStartPos_ +
                (autoCrossfadeTargetPos_ - autoCrossfadeStartPos_) * autoCrossfadeProgress_;
            if (autoCrossfadeProgress_ >= 1.0f) {
                autoCrossfading_ = false;
                isBeatSyncCrossfade_ = false;
                crossfadePos_ = autoCrossfadeTargetPos_;
            }
        } else {
            autoCrossfadeElapsed_ += deltaSeconds;
            autoCrossfadeProgress_ = std::clamp(autoCrossfadeElapsed_ / autoCrossfadeDuration_, 0.0f, 1.0f);
            crossfadePos_ = autoCrossfadeStartPos_ +
                (autoCrossfadeTargetPos_ - autoCrossfadeStartPos_) * autoCrossfadeProgress_;
            if (autoCrossfadeProgress_ >= 1.0f) {
                autoCrossfading_ = false;
                crossfadePos_ = autoCrossfadeTargetPos_;
            }
        }
    }

    // Autonomous musical tour progression
    if (tourSyncConfig_.enabled && isDownbeat_) {
        const std::uint32_t bar = currentBar();
        if (bar != lastTourTriggerBar_ && (bar % tourSyncConfig_.barsPerPreset == 0)) {
            lastTourTriggerBar_ = bar;
            const DeckId targetDeck = (crossfadePos_ <= 0.5f) ? DeckId::DeckB : DeckId::DeckA;
            if (tourAdvanceCallback_) {
                tourAdvanceCallback_(targetDeck);
            }
            triggerBeatSyncedCrossfade(tourSyncConfig_.transitionBars, targetDeck, false);
        }
    }

    // Musical Macro Automation Engine Tick
    macroEngine_.update(currentBar(), barPhase(), deltaSeconds);
    if (!autoCrossfading_) {
        const bool admitted = macroEngine_.tryApplyToDeck(
            crossfadePos_, masterFx_.masterFilter, masterFx_.chaosIntensity);
        if (macroAdmission) *macroAdmission = compositionAdmission(admitted);
    }
    return true;
}

void SculpturePerformanceDeck::update(float deltaSeconds) noexcept {
    static_cast<void>(tryUpdate(deltaSeconds));
}

std::string SculpturePerformanceDeck::formatDeckStatus() const {
    const auto [gainA, gainB] = computeDeckGains(crossfadePos_, crossfadeCurve_);
    std::ostringstream ss;
    ss << "DECK [A: " << static_cast<int>(gainA * 100.0f) << "% \"" << deckA_.presetTitle << "\""
       << " <";

    constexpr int kSliderWidth = 10;
    const int markerPos = std::clamp(static_cast<int>(crossfadePos_ * kSliderWidth), 0, kSliderWidth);
    for (int i = 0; i <= kSliderWidth; ++i) {
        if (i == markerPos) ss << "|";
        else ss << "=";
    }
    ss << "> B: " << static_cast<int>(gainB * 100.0f) << "% \"" << deckB_.presetTitle << "\"]"
       << "  |  BPM: " << static_cast<int>(bpm_ + 0.5f)
       << " [Bar " << currentBar() << "." << beatInBar() << "]"
       << (isDownbeat_ ? " *" : "");

    if (pendingDownbeat_) {
        ss << "  |  QUEUED (WAIT DOWNBEAT)";
    } else if (autoCrossfading_) {
        ss << "  |  " << (isBeatSyncCrossfade_ ? "BEAT-FADE: " : "AUTO-FADE: ")
           << static_cast<int>(autoCrossfadeProgress_ * 100.0f) << "%";
    }
    if (tourSyncConfig_.enabled) {
        ss << "  |  TOUR-BARS: " << tourSyncConfig_.barsPerPreset;
    }

    // Master FX status tags
    if (masterFx_.tapeStopActive) {
        ss << "  |  [FX: TAPE-STOP " << static_cast<int>(masterFx_.tapeStopSpeedFactor * 100.0f) << "%]";
    }
    if (masterFx_.beatRollActive) {
        ss << "  |  [FX: ROLL " << masterFx_.beatRollSubdivision << "]";
    }
    if (masterFx_.chaosIntensity > 1e-3f) {
        ss << "  |  [FX: CHAOS " << static_cast<int>(masterFx_.chaosIntensity * 100.0f) << "%]";
    }
    if (std::abs(masterFx_.masterFilter) > 1e-3f) {
        ss << "  |  [FLTR: " << (masterFx_.masterFilter < 0.0f ? "LP " : "HP ")
           << masterFx_.masterFilter << "]";
    }

    return ss.str();
}

} // namespace epi_sculpt
