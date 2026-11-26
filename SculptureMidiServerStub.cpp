#include "SculptureMidiServer.h"

#include <cstdio>

namespace epi_sculpt {

SculptureMidiServer::SculptureMidiServer(SculptureEventBus& eventBus)
    : eventBus_(eventBus), mapper_(eventBus) {}

SculptureMidiServer::~SculptureMidiServer() {
    stop();
}

bool SculptureMidiServer::backendCompiled() noexcept {
    return false;
}

bool SculptureMidiServer::start() {
    std::fprintf(stderr,
        "[SculptureMidiServer] MIDI device backend is not compiled; semantic MIDI mapping remains available for direct/event injection.\n");
    return false;
}

void SculptureMidiServer::stop() {
    isRunning_.store(false, std::memory_order_relaxed);
    if (workerThread_.joinable()) {
        workerThread_.join();
    }
    seqHandle_ = nullptr;
    portId_ = -1;
}

void SculptureMidiServer::serverLoop() {}

void SculptureMidiServer::processMidiNoteOn(
    std::uint8_t channel, std::uint8_t note, std::uint8_t velocity) {
    messageCount_.fetch_add(1, std::memory_order_relaxed);
    mapper_.handleMidiNoteOn(channel, note, velocity);
}

void SculptureMidiServer::processMidiNoteOff(
    std::uint8_t channel, std::uint8_t note, std::uint8_t velocity) {
    messageCount_.fetch_add(1, std::memory_order_relaxed);
    mapper_.handleMidiNoteOff(channel, note, velocity);
}

void SculptureMidiServer::processMidiController(
    std::uint8_t channel, std::uint8_t cc, std::uint8_t value) {
    messageCount_.fetch_add(1, std::memory_order_relaxed);
    mapper_.handleMidiController(channel, cc, value);
}

} // namespace epi_sculpt
