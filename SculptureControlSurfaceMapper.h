#pragma once

#include "SculptureEventBus.h"
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace epi_sculpt {

// -----------------------------------------------------------------------------
// Educational Annotation: HARDWARE CONTROL SURFACE MAPPER
// -----------------------------------------------------------------------------
// Encapsulates physical MIDI and OSC controller mappings into hot-swappable
// performance profiles. Decouples raw hardware protocols (ALSA MIDI, UDP OSC)
// from high-level visual performance logic.
//
// Supports standard DJ/VJ controllers, grid controllers (Launchpad/APC),
// and rotary fader banks with bidirectional LED status telemetry.
// -----------------------------------------------------------------------------

enum class ControlSurfaceProfile : std::uint8_t {
    StandardDJ,          // Two decks, center crossfader, filter knobs, cue pads
    LaunchControlXL,     // 8 faders, 24 knobs, 16 track focus/cue buttons
    AkaiAPC,             // 8x5 clip matrix, master fader, device controls
    KorgNano,            // Compact 8-channel fader/knob layout with transport keys
    GenericKeyboard      // Standard keyboard with pitch bend, mod wheel & pads
};

enum class ControlSurfaceAction : std::uint16_t {
    None = 0,
    Crossfader,
    CrossfadeNudge,
    MasterFilter,
    BpmTempo,
    TapTempo,
    DeckSwap,
    TriggerTapeStop,
    TriggerBeatRoll,
    TriggerChaosBurst,
    TriggerBeatSyncedCrossfade,
    TriggerAutoCrossfade,
    ToggleTourBeatSync,
    RecallCue0, RecallCue1, RecallCue2, RecallCue3, RecallCue4,
    RecallCue5, RecallCue6, RecallCue7, RecallCue8, RecallCue9,
    StoreCue0, StoreCue1, StoreCue2, StoreCue3, StoreCue4,
    StoreCue5, StoreCue6, StoreCue7, StoreCue8, StoreCue9,
    ModWheelLatentW,
    SonificationVolume,
    RotorTheta4D,
    TopologyBlend,
    RotorAlpha,
    RotorBeta,
    HueRotation,
    SeedAlpha,
    TimelinePlayPause,
    TimelineNextCue,
    TimelinePrevCue,
    SequenceRecordToggle,
    PaletteAgentToggle,
    PaletteAgentStimulate,
    MorphSpaceX,
    MorphSpaceY,
    MorphSpaceToggle,
    MorphSpaceBankCycle,
    MorphSpaceMotionCycle
};

struct ControlSurfaceLedStatus {
    uint8_t noteOrCc = 0;
    uint8_t velocityOrValue = 0;
    bool isNote = true;
};

class SculptureControlSurfaceMapper {
public:
    explicit SculptureControlSurfaceMapper(SculptureEventBus& bus);

    void setProfile(ControlSurfaceProfile profile);
    [[nodiscard]] ControlSurfaceProfile profile() const noexcept { return currentProfile_; }
    [[nodiscard]] std::string_view profileName() const noexcept;

    // Dispatch raw MIDI messages through active mapping table
    void handleMidiController(uint8_t channel, uint8_t cc, uint8_t value);
    void handleMidiNoteOn(uint8_t channel, uint8_t note, uint8_t velocity);
    void handleMidiNoteOff(uint8_t channel, uint8_t note, uint8_t velocity);

    // Dynamic Hardware MIDI Learn
    void bindCc(uint8_t cc, ControlSurfaceAction action);
    void bindNote(uint8_t note, ControlSurfaceAction action);
    void clearCustomBindings();

    // Query feedback LED states to send back to controller hardware
    [[nodiscard]] std::vector<ControlSurfaceLedStatus> computeFeedbackLeds(
        float crossfadePos,
        bool isDownbeat,
        uint16_t occupiedCueBankMask,
        bool tapeStopActive,
        bool beatRollActive,
        bool chaosActive) const;

private:
    void initProfileTables();
    void dispatchAction(ControlSurfaceAction action, float normalizedValue, uint8_t rawValue);

    SculptureEventBus& eventBus_;
    ControlSurfaceProfile currentProfile_ = ControlSurfaceProfile::StandardDJ;

    // CC -> Action lookup table (CC 0-127)
    std::unordered_map<uint8_t, ControlSurfaceAction> ccMap_;
    // Note -> Action lookup table (Note 0-127)
    std::unordered_map<uint8_t, ControlSurfaceAction> noteMap_;
};

} // namespace epi_sculpt
