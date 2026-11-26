#include "SculptureControlSurfaceMapper.h"

#include <algorithm>
#include <cmath>

namespace epi_sculpt {

SculptureControlSurfaceMapper::SculptureControlSurfaceMapper(SculptureEventBus& bus)
    : eventBus_(bus) {
    setProfile(ControlSurfaceProfile::StandardDJ);
}

void SculptureControlSurfaceMapper::setProfile(ControlSurfaceProfile profile) {
    currentProfile_ = profile;
    initProfileTables();
}

std::string_view SculptureControlSurfaceMapper::profileName() const noexcept {
    switch (currentProfile_) {
    case ControlSurfaceProfile::StandardDJ: return "Standard DJ/VJ Controller (DDJ/Traktor)";
    case ControlSurfaceProfile::LaunchControlXL: return "Novation Launch Control XL";
    case ControlSurfaceProfile::AkaiAPC: return "Akai APC Matrix Controller";
    case ControlSurfaceProfile::KorgNano: return "Korg nanoKONTROL2";
    case ControlSurfaceProfile::GenericKeyboard: return "Generic MIDI Keyboard / Synthesizer";
    }
    return "Unknown Profile";
}

void SculptureControlSurfaceMapper::initProfileTables() {
    ccMap_.clear();
    noteMap_.clear();

    switch (currentProfile_) {
    case ControlSurfaceProfile::StandardDJ:
        // Crossfader & Tempo
        ccMap_[8] = ControlSurfaceAction::Crossfader;
        ccMap_[14] = ControlSurfaceAction::Crossfader;
        ccMap_[16] = ControlSurfaceAction::MasterFilter;
        ccMap_[17] = ControlSurfaceAction::BpmTempo;
        ccMap_[18] = ControlSurfaceAction::CrossfadeNudge;

        // Hot Cue Recall & Store Pads (10 banks)
        for (uint8_t i = 0; i < 10; ++i) {
            ccMap_[20 + i] = static_cast<ControlSurfaceAction>(static_cast<uint16_t>(ControlSurfaceAction::RecallCue0) + i);
            ccMap_[30 + i] = static_cast<ControlSurfaceAction>(static_cast<uint16_t>(ControlSurfaceAction::StoreCue0) + i);
            noteMap_[36 + i] = static_cast<ControlSurfaceAction>(static_cast<uint16_t>(ControlSurfaceAction::RecallCue0) + i);
        }

        // Functional Performance Triggers & Decks
        ccMap_[45] = ControlSurfaceAction::DeckSwap;
        ccMap_[46] = ControlSurfaceAction::TriggerTapeStop;
        ccMap_[47] = ControlSurfaceAction::TriggerBeatRoll;
        ccMap_[48] = ControlSurfaceAction::TriggerChaosBurst;
        ccMap_[49] = ControlSurfaceAction::TriggerBeatSyncedCrossfade;
        ccMap_[50] = ControlSurfaceAction::TriggerAutoCrossfade;
        ccMap_[51] = ControlSurfaceAction::ToggleTourBeatSync;
        ccMap_[64] = ControlSurfaceAction::TapTempo;

        noteMap_[48] = ControlSurfaceAction::DeckSwap;
        noteMap_[49] = ControlSurfaceAction::TriggerTapeStop;
        noteMap_[50] = ControlSurfaceAction::TriggerBeatRoll;
        noteMap_[51] = ControlSurfaceAction::TriggerChaosBurst;
        noteMap_[52] = ControlSurfaceAction::TapTempo;

        // Timeline, Sequence Archiving & Living Palette
        ccMap_[53] = ControlSurfaceAction::TimelinePlayPause;
        ccMap_[54] = ControlSurfaceAction::TimelineNextCue;
        ccMap_[55] = ControlSurfaceAction::TimelinePrevCue;
        ccMap_[56] = ControlSurfaceAction::SequenceRecordToggle;
        ccMap_[57] = ControlSurfaceAction::PaletteAgentToggle;
        ccMap_[58] = ControlSurfaceAction::PaletteAgentStimulate;

        noteMap_[53] = ControlSurfaceAction::TimelinePlayPause;
        noteMap_[54] = ControlSurfaceAction::TimelineNextCue;
        noteMap_[55] = ControlSurfaceAction::TimelinePrevCue;
        noteMap_[56] = ControlSurfaceAction::SequenceRecordToggle;
        noteMap_[57] = ControlSurfaceAction::PaletteAgentToggle;
        noteMap_[58] = ControlSurfaceAction::PaletteAgentStimulate;

        // Legacy Expression & Mathematical Morphisms
        ccMap_[1] = ControlSurfaceAction::ModWheelLatentW;
        ccMap_[2] = ControlSurfaceAction::SonificationVolume;
        ccMap_[71] = ControlSurfaceAction::RotorTheta4D;
        ccMap_[72] = ControlSurfaceAction::TopologyBlend;
        ccMap_[73] = ControlSurfaceAction::RotorAlpha;
        ccMap_[74] = ControlSurfaceAction::RotorBeta;
        ccMap_[75] = ControlSurfaceAction::HueRotation;
        ccMap_[76] = ControlSurfaceAction::SeedAlpha;

        // 2D Morph Space Vector Synthesizer
        ccMap_[68] = ControlSurfaceAction::MorphSpaceX;
        ccMap_[69] = ControlSurfaceAction::MorphSpaceY;
        ccMap_[70] = ControlSurfaceAction::MorphSpaceToggle;
        ccMap_[77] = ControlSurfaceAction::MorphSpaceBankCycle;
        ccMap_[78] = ControlSurfaceAction::MorphSpaceMotionCycle;
        break;

    case ControlSurfaceProfile::LaunchControlXL:
        // Launch Control XL: Fader 8 is Crossfader, Send A row is Filter
        ccMap_[77] = ControlSurfaceAction::Crossfader;
        ccMap_[13] = ControlSurfaceAction::MasterFilter;
        ccMap_[14] = ControlSurfaceAction::TopologyBlend;
        ccMap_[15] = ControlSurfaceAction::ModWheelLatentW;
        ccMap_[16] = ControlSurfaceAction::MorphSpaceX;
        ccMap_[17] = ControlSurfaceAction::MorphSpaceY;
        ccMap_[18] = ControlSurfaceAction::MorphSpaceToggle;

        for (uint8_t i = 0; i < 8; ++i) {
            noteMap_[41 + i] = static_cast<ControlSurfaceAction>(static_cast<uint16_t>(ControlSurfaceAction::RecallCue0) + i);
            noteMap_[73 + i] = static_cast<ControlSurfaceAction>(static_cast<uint16_t>(ControlSurfaceAction::StoreCue0) + i);
        }
        noteMap_[24] = ControlSurfaceAction::DeckSwap;
        noteMap_[25] = ControlSurfaceAction::TriggerTapeStop;
        noteMap_[26] = ControlSurfaceAction::TriggerBeatRoll;
        noteMap_[27] = ControlSurfaceAction::TriggerChaosBurst;
        break;

    case ControlSurfaceProfile::KorgNano:
        // Korg nanoKONTROL2: Slider 0 is Crossfader, Knob 0 is Filter
        ccMap_[0] = ControlSurfaceAction::Crossfader;
        ccMap_[16] = ControlSurfaceAction::MasterFilter;
        ccMap_[1] = ControlSurfaceAction::MorphSpaceX;
        ccMap_[2] = ControlSurfaceAction::MorphSpaceY;
        ccMap_[32] = ControlSurfaceAction::MorphSpaceToggle;
        ccMap_[46] = ControlSurfaceAction::DeckSwap;
        ccMap_[60] = ControlSurfaceAction::TapTempo;
        ccMap_[63] = ControlSurfaceAction::TriggerTapeStop;
        ccMap_[62] = ControlSurfaceAction::TriggerBeatRoll;
        ccMap_[61] = ControlSurfaceAction::TriggerChaosBurst;
        break;

    case ControlSurfaceProfile::AkaiAPC:
    case ControlSurfaceProfile::GenericKeyboard:
        // Standard General MIDI Keyboard controls
        ccMap_[1] = ControlSurfaceAction::ModWheelLatentW;
        ccMap_[2] = ControlSurfaceAction::SonificationVolume;
        ccMap_[11] = ControlSurfaceAction::MasterFilter;
        ccMap_[64] = ControlSurfaceAction::TapTempo;
        for (uint8_t i = 0; i < 10; ++i) {
            noteMap_[36 + i] = static_cast<ControlSurfaceAction>(static_cast<uint16_t>(ControlSurfaceAction::RecallCue0) + i);
        }
        noteMap_[48] = ControlSurfaceAction::DeckSwap;
        noteMap_[49] = ControlSurfaceAction::TriggerTapeStop;
        noteMap_[50] = ControlSurfaceAction::TriggerBeatRoll;
        noteMap_[51] = ControlSurfaceAction::TriggerChaosBurst;
        break;
    }
}

void SculptureControlSurfaceMapper::bindCc(uint8_t cc, ControlSurfaceAction action) {
    ccMap_[cc] = action;
}

void SculptureControlSurfaceMapper::bindNote(uint8_t note, ControlSurfaceAction action) {
    noteMap_[note] = action;
}

void SculptureControlSurfaceMapper::clearCustomBindings() {
    initProfileTables();
}

void SculptureControlSurfaceMapper::handleMidiController(uint8_t /*channel*/, uint8_t cc, uint8_t value) {
    auto it = ccMap_.find(cc);
    if (it == ccMap_.end()) return;

    const float norm = static_cast<float>(value) / 127.0f;
    dispatchAction(it->second, norm, value);
}

void SculptureControlSurfaceMapper::handleMidiNoteOn(uint8_t /*channel*/, uint8_t note, uint8_t velocity) {
    if (velocity == 0) return;

    auto it = noteMap_.find(note);
    if (it == noteMap_.end()) {
        if (note >= 60) {
            EventCapture e{};
            eventBus_.post(e);
        }
        return;
    }

    const float norm = static_cast<float>(velocity) / 127.0f;
    dispatchAction(it->second, norm, velocity);
}

void SculptureControlSurfaceMapper::handleMidiNoteOff(uint8_t /*channel*/, uint8_t /*note*/, uint8_t /*velocity*/) {
    // Note-off can be used in the future for gate/momentary release
}

void SculptureControlSurfaceMapper::dispatchAction(ControlSurfaceAction action, float norm, uint8_t rawValue) {
    // Range-based Hot Cue checks
    if (action >= ControlSurfaceAction::RecallCue0 && action <= ControlSurfaceAction::RecallCue9) {
        if (rawValue > 0) {
            const std::size_t bank = static_cast<std::size_t>(action) - static_cast<std::size_t>(ControlSurfaceAction::RecallCue0);
            eventBus_.post(EventPerformanceDeckControl{
                .action = EventPerformanceDeckControl::Action::RecallCueToOpposite,
                .bankIndex = bank
            });
        }
        return;
    }

    if (action >= ControlSurfaceAction::StoreCue0 && action <= ControlSurfaceAction::StoreCue9) {
        if (rawValue > 0) {
            const std::size_t bank = static_cast<std::size_t>(action) - static_cast<std::size_t>(ControlSurfaceAction::StoreCue0);
            eventBus_.post(EventPerformanceDeckControl{
                .action = EventPerformanceDeckControl::Action::StoreCue,
                .bankIndex = bank
            });
        }
        return;
    }

    switch (action) {
    case ControlSurfaceAction::Crossfader:
        eventBus_.post(EventPerformanceDeckControl{
            .action = EventPerformanceDeckControl::Action::SetCrossfadePosition,
            .value = norm
        });
        break;
    case ControlSurfaceAction::CrossfadeNudge:
        eventBus_.post(EventPerformanceDeckControl{
            .action = EventPerformanceDeckControl::Action::NudgeCrossfade,
            .value = (norm - 0.5f) * 0.2f
        });
        break;
    case ControlSurfaceAction::MasterFilter:
        eventBus_.post(EventPerformanceDeckControl{
            .action = EventPerformanceDeckControl::Action::SetMasterFilter,
            .value = std::clamp((norm * 2.0f) - 1.0f, -1.0f, 1.0f)
        });
        break;
    case ControlSurfaceAction::BpmTempo:
        eventBus_.post(EventPerformanceDeckControl{
            .action = EventPerformanceDeckControl::Action::SetBpm,
            .value = 60.0f + norm * 120.0f
        });
        break;
    case ControlSurfaceAction::TapTempo:
        if (rawValue >= 64) {
            eventBus_.post(EventPerformanceDeckControl{
                .action = EventPerformanceDeckControl::Action::TapTempo
            });
        }
        break;
    case ControlSurfaceAction::DeckSwap:
        if (rawValue > 0) {
            eventBus_.post(EventPerformanceDeckControl{
                .action = EventPerformanceDeckControl::Action::SwapDecks
            });
        }
        break;
    case ControlSurfaceAction::TriggerTapeStop:
        if (rawValue > 0) {
            eventBus_.post(EventPerformanceDeckControl{
                .action = EventPerformanceDeckControl::Action::TriggerTapeStop,
                .value = 1.2f
            });
        }
        break;
    case ControlSurfaceAction::TriggerBeatRoll:
        if (rawValue > 0) {
            const float subdiv = (norm > 0.75f) ? 0.125f : ((norm > 0.45f) ? 0.25f : 0.5f);
            eventBus_.post(EventPerformanceDeckControl{
                .action = EventPerformanceDeckControl::Action::TriggerBeatRoll,
                .value = subdiv
            });
        }
        break;
    case ControlSurfaceAction::TriggerChaosBurst:
        if (rawValue > 0) {
            eventBus_.post(EventPerformanceDeckControl{
                .action = EventPerformanceDeckControl::Action::TriggerChaosBurst,
                .value = norm
            });
        }
        break;
    case ControlSurfaceAction::TriggerBeatSyncedCrossfade:
        if (rawValue > 0) {
            eventBus_.post(EventPerformanceDeckControl{
                .action = EventPerformanceDeckControl::Action::TriggerBeatSyncedCrossfade,
                .value = 2.0f
            });
        }
        break;
    case ControlSurfaceAction::TriggerAutoCrossfade:
        if (rawValue > 0) {
            eventBus_.post(EventPerformanceDeckControl{
                .action = EventPerformanceDeckControl::Action::TriggerAutoCrossfade,
                .value = 4.0f
            });
        }
        break;
    case ControlSurfaceAction::ToggleTourBeatSync:
        if (rawValue > 0) {
            eventBus_.post(EventPerformanceDeckControl{
                .action = EventPerformanceDeckControl::Action::ToggleTourBeatSync
            });
        }
        break;

    case ControlSurfaceAction::ModWheelLatentW: {
        EventMutation mut;
        mut.action = EventMutation::Action::SetLatentWOffset;
        mut.strength = (norm - 0.5f) * 4.0f;
        eventBus_.post(mut);
        break;
    }
    case ControlSurfaceAction::SonificationVolume: {
        EventSonificationVolume vol;
        vol.targetVolume = norm;
        eventBus_.post(vol);
        break;
    }
    case ControlSurfaceAction::RotorTheta4D: {
        EventMutation mut;
        mut.action = EventMutation::Action::SetRotorThetaOffset;
        mut.strength = (norm - 0.5f) * 6.28f;
        eventBus_.post(mut);
        break;
    }
    case ControlSurfaceAction::TopologyBlend: {
        EventMutation mut;
        mut.action = EventMutation::Action::SetTopologyBlendOffset;
        mut.strength = (norm - 0.5f) * 2.0f;
        eventBus_.post(mut);
        break;
    }
    case ControlSurfaceAction::RotorAlpha: {
        EventMutation mut;
        mut.action = EventMutation::Action::SetRotorAlphaOffset;
        mut.strength = (norm - 0.5f) * 6.28f;
        eventBus_.post(mut);
        break;
    }
    case ControlSurfaceAction::RotorBeta: {
        EventMutation mut;
        mut.action = EventMutation::Action::SetRotorBetaOffset;
        mut.strength = (norm - 0.5f) * 6.28f;
        eventBus_.post(mut);
        break;
    }
    case ControlSurfaceAction::HueRotation: {
        EventMutation mut;
        mut.action = EventMutation::Action::SetHueOffset;
        mut.strength = (norm - 0.5f) * 6.28f;
        eventBus_.post(mut);
        break;
    }
    case ControlSurfaceAction::SeedAlpha: {
        EventMutation mut;
        mut.action = EventMutation::Action::SetSeedAlphaOffset;
        mut.strength = (norm - 0.5f) * 2.0f;
        eventBus_.post(mut);
        break;
    }
    case ControlSurfaceAction::TimelinePlayPause:
        if (rawValue > 0) {
            eventBus_.post(EventTimelineControl{EventTimelineControl::Action::TogglePlayPause});
        }
        break;
    case ControlSurfaceAction::TimelineNextCue:
        if (rawValue > 0) {
            eventBus_.post(EventTimelineControl{EventTimelineControl::Action::NextCue});
        }
        break;
    case ControlSurfaceAction::TimelinePrevCue:
        if (rawValue > 0) {
            eventBus_.post(EventTimelineControl{EventTimelineControl::Action::PreviousCue});
        }
        break;
    case ControlSurfaceAction::SequenceRecordToggle:
        if (rawValue > 0) {
            eventBus_.post(EventSequenceRecorderControl{EventSequenceRecorderControl::Action::Toggle});
        }
        break;
    case ControlSurfaceAction::PaletteAgentToggle:
        if (rawValue > 0) {
            eventBus_.post(EventPaletteAgentControl{EventPaletteAgentControl::Action::Toggle});
        }
        break;
    case ControlSurfaceAction::PaletteAgentStimulate:
        if (rawValue > 0) {
            eventBus_.post(EventPaletteAgentControl{
                .action = EventPaletteAgentControl::Action::Stimulate,
                .intensity = norm
            });
        }
        break;
    case ControlSurfaceAction::MorphSpaceX:
        eventBus_.post(EventMorphSpaceControl{
            .action = EventMorphSpaceControl::Action::SetCursor,
            .cursorX = norm,
            .cursorY = -1.0f
        });
        break;
    case ControlSurfaceAction::MorphSpaceY:
        eventBus_.post(EventMorphSpaceControl{
            .action = EventMorphSpaceControl::Action::SetCursor,
            .cursorX = -1.0f,
            .cursorY = norm
        });
        break;
    case ControlSurfaceAction::MorphSpaceToggle:
        if (rawValue > 0) {
            eventBus_.post(EventMorphSpaceControl{
                .action = EventMorphSpaceControl::Action::ToggleActive
            });
        }
        break;
    case ControlSurfaceAction::MorphSpaceBankCycle:
        if (rawValue > 0) {
            eventBus_.post(EventMorphSpaceControl{
                .action = EventMorphSpaceControl::Action::CycleBank
            });
        }
        break;
    case ControlSurfaceAction::MorphSpaceMotionCycle:
        if (rawValue > 0) {
            eventBus_.post(EventMorphSpaceControl{
                .action = EventMorphSpaceControl::Action::CycleMotionMode
            });
        }
        break;
    default:
        break;
    }
}

std::vector<ControlSurfaceLedStatus> SculptureControlSurfaceMapper::computeFeedbackLeds(
    float crossfadePos,
    bool isDownbeat,
    uint16_t occupiedCueBankMask,
    bool tapeStopActive,
    bool beatRollActive,
    bool chaosActive) const {

    std::vector<ControlSurfaceLedStatus> leds;

    // Deck status LEDs
    leds.push_back({.noteOrCc = 45, .velocityOrValue = static_cast<uint8_t>(crossfadePos < 0.5f ? 127 : 32), .isNote = false});

    // Master FX LEDs
    leds.push_back({.noteOrCc = 46, .velocityOrValue = static_cast<uint8_t>(tapeStopActive ? 127 : 0), .isNote = false});
    leds.push_back({.noteOrCc = 47, .velocityOrValue = static_cast<uint8_t>(beatRollActive ? 127 : 0), .isNote = false});
    leds.push_back({.noteOrCc = 48, .velocityOrValue = static_cast<uint8_t>(chaosActive ? 127 : 0), .isNote = false});

    // Beat flash LED
    leds.push_back({.noteOrCc = 52, .velocityOrValue = static_cast<uint8_t>(isDownbeat ? 127 : 16), .isNote = true});

    // Hot Cue Pads LED state (occupied = bright 127, empty = off 0)
    for (uint8_t bank = 0; bank < 10; ++bank) {
        const bool occupied = (occupiedCueBankMask & (1U << bank)) != 0;
        const uint8_t brightness = occupied ? 127 : 0;
        leds.push_back({.noteOrCc = static_cast<uint8_t>(36 + bank), .velocityOrValue = brightness, .isNote = true});
        leds.push_back({.noteOrCc = static_cast<uint8_t>(20 + bank), .velocityOrValue = brightness, .isNote = false});
    }

    return leds;
}

} // namespace epi_sculpt
