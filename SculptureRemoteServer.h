#pragma once

#include "SculptureEventBus.h"
#include "SculptureOscDecoder.h"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <atomic>
#include <thread>
#include <mutex>
#include <span>

#if defined(__unix__) || defined(__linux__) || defined(__APPLE__)
#include <netinet/in.h>
#endif

namespace epi_sculpt {

// -----------------------------------------------------------------------------
// Educational Annotation: BIDIRECTIONAL REMOTE CONTROL SURFACE & OSC STREAMER
// -----------------------------------------------------------------------------
// Enables live interactive control and real-time state telemetry streaming over
// network UDP / OSC packets.
//
// 1. Inbound: Incoming network datagrams are parsed on the server worker thread and
//    translated into type-safe semantic events. Off-thread EventBus posts are queued
//    and committed by SculptureApplication at the next frame input boundary, so the
//    authoritative semantic state remains single-writer.
// 2. Outbound: Broadcasts high-frequency visual/musical engine telemetry
//    (FPS, morph progress, simulation time, crossfade position, BPM, recording status)
//    back to connected TouchOSC tablets, Resolume, or remote monitors.
// -----------------------------------------------------------------------------

class SculptureRemoteServer {
public:
    explicit SculptureRemoteServer(SculptureEventBus& eventBus, uint16_t port = 9000);
    ~SculptureRemoteServer();

    SculptureRemoteServer(const SculptureRemoteServer&) = delete;
    SculptureRemoteServer& operator=(const SculptureRemoteServer&) = delete;

    bool start();
    void stop();

    [[nodiscard]] bool isRunning() const noexcept { return isRunning_.load(); }
    [[nodiscard]] uint16_t port() const noexcept { return port_; }
    [[nodiscard]] uint64_t packetCount() const noexcept { return packetCount_.load(); }

    // Direct packet injection for unit tests and local simulation
    void processDatagram(std::span<const uint8_t> packet);

    // Outbound OSC transmission & remote client configuration
    void setTargetEndpoint(std::string_view host, uint16_t port);
    [[nodiscard]] const std::string& targetHost() const noexcept { return targetHost_; }
    [[nodiscard]] uint16_t targetPort() const noexcept { return targetPort_; }

    void sendOscMessage(std::string_view address, const std::vector<OscArg>& args);
    void sendOscFloat(std::string_view address, float value);
    void sendOscInt(std::string_view address, int32_t value);
    void sendOscString(std::string_view address, std::string_view value);

    // Bidirectional live telemetry broadcast for TouchOSC / remote VJ monitors
    void broadcastTelemetry(
        float fps,
        float morphProgress,
        float simulatedTime,
        float crossfadePosition,
        float bpm,
        bool isRecording,
        std::string_view activeCueTitle = "",
        bool morphSpaceActive = false,
        float morphSpaceX = 0.5f,
        float morphSpaceY = 0.5f,
        std::string_view morphSpaceBank = ""
    );

private:
    void dispatchMessage(const OscMessage& msg);
    void serverLoop();

    SculptureEventBus& eventBus_;
    uint16_t port_ = 9000;
    int socketFd_ = -1;
    std::atomic<bool> isRunning_{false};
    std::atomic<uint64_t> packetCount_{0};
    std::thread workerThread_;

    std::string targetHost_{"127.0.0.1"};
    uint16_t targetPort_ = 9001; // Default TouchOSC receive port
    std::mutex sendMutex_{};

#if defined(__unix__) || defined(__linux__) || defined(__APPLE__)
    sockaddr_in targetAddr_{};
    bool hasTargetAddr_ = false;
#endif
};

} // namespace epi_sculpt
