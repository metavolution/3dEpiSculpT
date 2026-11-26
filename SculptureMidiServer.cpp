#include "SculptureMidiServer.h"
#include <cstdio>
#include <vector>

#if defined(__linux__)
#include <poll.h>
#endif

#if defined(__linux__)
#include <alsa/asoundlib.h>
#endif

namespace epi_sculpt {

SculptureMidiServer::SculptureMidiServer(SculptureEventBus& eventBus)
    : eventBus_(eventBus), mapper_(eventBus) {}

SculptureMidiServer::~SculptureMidiServer() {
    stop();
}

bool SculptureMidiServer::backendCompiled() noexcept {
#if defined(__linux__)
    return true;
#else
    return false;
#endif
}

bool SculptureMidiServer::start() {
    if (isRunning_.load()) return true;

#if defined(__linux__)
    snd_seq_t* seq = nullptr;
    int err = snd_seq_open(&seq, "default", SND_SEQ_OPEN_INPUT, SND_SEQ_NONBLOCK);
    if (err < 0) {
        std::fprintf(stderr, "[SculptureMidiServer] Failed to open ALSA sequencer: %s\n", snd_strerror(err));
        return false;
    }

    snd_seq_set_client_name(seq, "epiC_Sculpture");

    int port = snd_seq_create_simple_port(seq, "epiC_Midi_In",
                                          SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE,
                                          SND_SEQ_PORT_TYPE_MIDI_GENERIC | SND_SEQ_PORT_TYPE_APPLICATION);
    if (port < 0) {
        std::fprintf(stderr, "[SculptureMidiServer] Failed to create ALSA port: %s\n", snd_strerror(port));
        snd_seq_close(seq);
        return false;
    }

    const int descriptorCount = snd_seq_poll_descriptors_count(seq, POLLIN);
    if (descriptorCount <= 0) {
        std::fprintf(stderr,
            "[SculptureMidiServer] ALSA sequencer exposed no readable poll descriptors; MIDI device input remains disabled.\n");
        snd_seq_close(seq);
        return false;
    }

    seqHandle_ = seq;
    portId_ = port;
    isRunning_.store(true, std::memory_order_release);
    try {
        workerThread_ = std::thread(&SculptureMidiServer::serverLoop, this);
    } catch (...) {
        isRunning_.store(false, std::memory_order_release);
        snd_seq_close(seq);
        seqHandle_ = nullptr;
        portId_ = -1;
        std::fprintf(stderr,
            "[SculptureMidiServer] Failed to start MIDI worker thread; device input remains disabled.\n");
        return false;
    }
    std::fprintf(stderr, "[SculptureMidiServer] ALSA MIDI Server listening on port %d\n", port);
    return true;
#else
    std::fprintf(stderr, "[SculptureMidiServer] MIDI Server is only supported on Linux via ALSA.\n");
    return false;
#endif
}

void SculptureMidiServer::stop() {
    isRunning_.store(false, std::memory_order_release);

    if (workerThread_.joinable()) {
        workerThread_.join();
    }

#if defined(__linux__)
    if (seqHandle_) {
        snd_seq_t* seq = static_cast<snd_seq_t*>(seqHandle_);
        snd_seq_close(seq);
        seqHandle_ = nullptr;
    }
#endif
    portId_ = -1;
}

void SculptureMidiServer::serverLoop() {
#if defined(__linux__)
    snd_seq_t* seq = static_cast<snd_seq_t*>(seqHandle_);
    int npfd = snd_seq_poll_descriptors_count(seq, POLLIN);
    if (npfd <= 0) {
        isRunning_.store(false, std::memory_order_release);
        return;
    }

    std::vector<struct pollfd> pfd(npfd);
    snd_seq_poll_descriptors(seq, pfd.data(), npfd, POLLIN);

    while (isRunning_.load(std::memory_order_relaxed)) {
        if (poll(pfd.data(), npfd, 100) > 0) {
            snd_seq_event_t* ev = nullptr;
            while (snd_seq_event_input(seq, &ev) > 0 && ev != nullptr) {
                if (ev->type == SND_SEQ_EVENT_NOTEON) {
                    processMidiNoteOn(ev->data.note.channel, ev->data.note.note, ev->data.note.velocity);
                } else if (ev->type == SND_SEQ_EVENT_NOTEOFF) {
                    processMidiNoteOff(ev->data.note.channel, ev->data.note.note, ev->data.note.velocity);
                } else if (ev->type == SND_SEQ_EVENT_CONTROLLER) {
                    processMidiController(ev->data.control.channel, ev->data.control.param, ev->data.control.value);
                }
            }
        }
    }
#endif
}

void SculptureMidiServer::processMidiNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
    messageCount_.fetch_add(1, std::memory_order_relaxed);
    mapper_.handleMidiNoteOn(channel, note, velocity);
}

void SculptureMidiServer::processMidiNoteOff(uint8_t channel, uint8_t note, uint8_t velocity) {
    messageCount_.fetch_add(1, std::memory_order_relaxed);
    mapper_.handleMidiNoteOff(channel, note, velocity);
}

void SculptureMidiServer::processMidiController(uint8_t channel, uint8_t cc, uint8_t value) {
    messageCount_.fetch_add(1, std::memory_order_relaxed);
    mapper_.handleMidiController(channel, cc, value);
}

} // namespace epi_sculpt
