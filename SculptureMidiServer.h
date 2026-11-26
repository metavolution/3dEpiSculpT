#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <atomic>
#include <thread>
#include "SculptureEventBus.h"
#include "SculptureControlSurfaceMapper.h"

namespace epi_sculpt {

// -----------------------------------------------------------------------------
// Educational Annotation: Asynchronous MIDI Control Surface Server
// -----------------------------------------------------------------------------
// Enables live interactive control via hardware MIDI keyboards and fader banks.
// The semantic mapper is backend-independent; the Linux device capability uses
// ALSA Sequencer when compiled, while a no-device implementation preserves the
// same direct/event-injection surface without a platform dependency.
// Translates MIDI Notes and Control Change (CC) messages into type-safe
// semantic events. Worker-thread events are queued by SculptureEventBus and
// committed on the application thread at the next frame input boundary.
// -----------------------------------------------------------------------------

class SculptureMidiServer {
public:
    explicit SculptureMidiServer(SculptureEventBus& eventBus);
    ~SculptureMidiServer();

    SculptureMidiServer(const SculptureMidiServer&) = delete;
    SculptureMidiServer& operator=(const SculptureMidiServer&) = delete;

    [[nodiscard]] static bool backendCompiled() noexcept;

    bool start();
    void stop();

    /// Process an individual MIDI Control Change (CC) message (0-127).
    /// Dispatches DJ/VJ deck controls, master FX, cue banking, and legacy mutations.
    /// Safe to call directly from tests, simulations, or external input layers.
    void processMidiController(uint8_t channel, uint8_t cc, uint8_t value);

    /// Process an individual MIDI Note-On message (0-127).
    /// Translates drum pads to Hot Cue triggers and functional performance keys.
    void processMidiNoteOn(uint8_t channel, uint8_t note, uint8_t velocity);

    /// Process an individual MIDI Note-Off message (0-127).
    void processMidiNoteOff(uint8_t channel, uint8_t note, uint8_t velocity);

    [[nodiscard]] bool isRunning() const noexcept { return isRunning_.load(); }
    [[nodiscard]] uint64_t messageCount() const noexcept { return messageCount_.load(); }

    [[nodiscard]] SculptureControlSurfaceMapper& mapper() noexcept { return mapper_; }
    [[nodiscard]] const SculptureControlSurfaceMapper& mapper() const noexcept { return mapper_; }

private:
    void serverLoop();

    SculptureEventBus& eventBus_;
    SculptureControlSurfaceMapper mapper_;
    std::atomic<bool> isRunning_{false};
    std::atomic<uint64_t> messageCount_{0};
    std::thread workerThread_;

    // ALSA sequencer handle (opaque pointer to hide <alsa/asoundlib.h> from the header)
    void* seqHandle_ = nullptr;
    int portId_ = -1;
};

} // namespace epi_sculpt
