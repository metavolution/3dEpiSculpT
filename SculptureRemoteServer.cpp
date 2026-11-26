#include "SculptureLearnMode.h"
#include "SculptureRemoteServer.h"

#include <iostream>
#include <vector>
#include <cstring>

#if defined(__unix__) || defined(__linux__) || defined(__APPLE__)
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>
#include <fcntl.h>
#endif

namespace epi_sculpt {

SculptureRemoteServer::SculptureRemoteServer(SculptureEventBus& eventBus, uint16_t port)
    : eventBus_(eventBus), port_(port) {
    setTargetEndpoint("127.0.0.1", port + 1);
}

SculptureRemoteServer::~SculptureRemoteServer() {
    stop();
}

bool SculptureRemoteServer::start() {
    if (isRunning_.load()) return true;

#if defined(__unix__) || defined(__linux__) || defined(__APPLE__)
    socketFd_ = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (socketFd_ < 0) {
        std::fprintf(stderr, "[SculptureRemoteServer] Failed to create UDP socket on port %u\n", port_);
        return false;
    }

    // Set non-blocking / reuseaddr
    int opt = 1;
    ::setsockopt(socketFd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port_);

    if (::bind(socketFd_, reinterpret_cast<const sockaddr*>(&serverAddr), sizeof(serverAddr)) < 0) {
        std::fprintf(stderr, "[SculptureRemoteServer] Failed to bind UDP socket to port %u\n", port_);
        ::close(socketFd_);
        socketFd_ = -1;
        return false;
    }

    isRunning_.store(true);
    workerThread_ = std::thread(&SculptureRemoteServer::serverLoop, this);
    std::fprintf(stderr, "[SculptureRemoteServer] OSC Server listening on UDP port %u\n", port_);
    return true;
#else
    return false;
#endif
}

void SculptureRemoteServer::stop() {
    if (!isRunning_.exchange(false)) return;

#if defined(__unix__) || defined(__linux__) || defined(__APPLE__)
    if (socketFd_ >= 0) {
        ::close(socketFd_);
        socketFd_ = -1;
    }
#endif

    if (workerThread_.joinable()) {
        workerThread_.join();
    }
}

void SculptureRemoteServer::serverLoop() {
#if defined(__unix__) || defined(__linux__) || defined(__APPLE__)
    std::vector<uint8_t> buffer(4096);
    pollfd pfd{};
    pfd.fd = socketFd_;
    pfd.events = POLLIN;

    while (isRunning_.load()) {
        int pollRes = ::poll(&pfd, 1, 100); // 100ms timeout
        if (pollRes > 0 && (pfd.revents & POLLIN)) {
            sockaddr_in clientAddr{};
            socklen_t clientLen = sizeof(clientAddr);
            ssize_t bytesRead = ::recvfrom(socketFd_, buffer.data(), buffer.size(), 0,
                                          reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);
            if (bytesRead > 0) {
                {
                    std::lock_guard<std::mutex> lock(sendMutex_);
                    if (!hasTargetAddr_) {
                        targetAddr_ = clientAddr;
                        targetAddr_.sin_port = htons(targetPort_);
                        hasTargetAddr_ = true;
                    }
                }
                processDatagram(std::span<const uint8_t>(buffer.data(), static_cast<std::size_t>(bytesRead)));
            }
        }
    }
#endif
}

void SculptureRemoteServer::processDatagram(std::span<const uint8_t> packet) {
    packetCount_++;
    OscMessage msg;
    if (SculptureOscDecoder::parseMessage(packet, msg)) {
        dispatchMessage(msg);
    }
}

void SculptureRemoteServer::dispatchMessage(const OscMessage& msg) {
    const std::string_view addr = msg.address;

    if (addr == "/epi/preset/select" || addr == "/epi/preset/morph") {
        const auto idx = static_cast<std::size_t>(std::max(0, msg.getInt(0)));
        const float duration = msg.getFloat(1, 3.0f);
        eventBus_.post(EventPresetSelect{idx, duration});
    } else if (addr == "/epi/preset/cycle") {
        eventBus_.post(EventPresetCycle{msg.getInt(0, 1)});
    } else if (addr == "/epi/morph/easing/cycle") {
        eventBus_.post(EventMorphEasingCycle{});
    } else if (addr == "/epi/learn/stage") {
        eventBus_.post(EventLearnStage{static_cast<uint32_t>(msg.getInt(0))});
    } else if (addr == "/epi/learn/solo") {
        const int s = msg.getInt(0);
        if (s >= 0 && s < 6) {
            eventBus_.post(EventLearnStage{learn_mode::commandSoloStage(static_cast<learn_mode::Stage>(s))});
        }
    } else if (addr == "/epi/learn/accumulative") {
        const int s = msg.getInt(0);
        if (s >= 0 && s < 6) {
            eventBus_.post(EventLearnStage{learn_mode::commandAccumulativeStage(static_cast<learn_mode::Stage>(s))});
        }
    } else if (addr == "/epi/learn/cycle") {
        eventBus_.post(EventLearnStage{0xFFFFFFFFU});
    } else if (addr == "/epi/learn/reset") {
        eventBus_.post(EventLearnStage{0x8000003FU});
    } else if (addr == "/epi/mutate/tour") {
        eventBus_.post(EventMutation{EventMutation::Action::ToggleTour});
    } else if (addr == "/epi/mutate/step") {
        eventBus_.post(EventMutation{EventMutation::Action::StepMutation, msg.getFloat(0, 0.25f)});
    } else if (addr == "/epi/mutate/reset") {
        eventBus_.post(EventMutation{EventMutation::Action::ResetBase});
    } else if (addr == "/epi/camera/mode" || addr == "/epi/camera/cycle") {
        eventBus_.post(EventCameraModeCycle{});
    } else if (addr == "/epi/steer") {
        eventBus_.post(EventSteering{msg.getFloat(0, 0.0f), msg.getFloat(1, 0.0f)});
    } else if (addr == "/epi/shader/reload") {
        eventBus_.post(EventReloadShaders{});
    } else if (addr == "/epi/pause") {
        eventBus_.post(EventToggleState{EventToggleState::Target::Pause});
    } else if (addr == "/epi/cinematic") {
        eventBus_.post(EventToggleState{EventToggleState::Target::Cinematic});
    } else if (addr == "/epi/temporal") {
        eventBus_.post(EventToggleState{EventToggleState::Target::Temporal});
    } else if (addr == "/epi/temporal/reset") {
        eventBus_.post(EventTemporalReset{});
    } else if (addr == "/epi/capture" || addr == "/epi/snapshot") {
        eventBus_.post(EventCapture{});
    } else if (addr == "/epi/sonify/cycle" || addr == "/epi/sonify/mode") {
        eventBus_.post(EventSonificationModeCycle{});
    } else if (addr == "/epi/sonify/volume") {
        eventBus_.post(EventSonificationVolume{msg.getFloat(0, 0.0f)});
    } else if (addr == "/epi/strategy/cycle" || addr == "/epi/render/strategy") {
        eventBus_.post(EventRenderStrategyCycle{});
    } else if (addr == "/epi/scale/cycle" || addr == "/epi/resolution/scale") {
        eventBus_.post(EventResolutionScaleCycle{});
    } else if (addr == "/epi/state/save") {
        eventBus_.post(EventFileAction{EventFileAction::Action::SaveState, std::string(msg.getString(0, "remote_state.epi"))});
    } else if (addr == "/epi/state/load") {
        eventBus_.post(EventFileAction{EventFileAction::Action::LoadState, std::string(msg.getString(0, "remote_state.epi"))});
    } else if (addr == "/epi/gesture/left" || addr == "/epi/gesture/right") {
        const bool isLeft = (addr == "/epi/gesture/left");
        EventSpatialGesture evt{};
        evt.isLeftHand = isLeft;
        evt.posX = msg.getFloat(0, 0.0f);
        evt.posY = msg.getFloat(1, 0.0f);
        evt.posZ = msg.getFloat(2, 0.0f);
        evt.trigger = msg.getFloat(3, 0.0f);
        evt.grip = msg.getFloat(4, 0.0f);
        evt.valid = true;
        eventBus_.post(evt);
    } else if (addr == "/epi/vr/stereo" || addr == "/epi/vr/ipd") {
        EventStereoRigConfig evt{};
        evt.ipd = msg.getFloat(0, 0.064f);
        evt.convergenceDist = msg.getFloat(1, 2.5f);
        eventBus_.post(evt);
    } else if (addr == "/epi/foliation/manifold") {
        EventFoliationManifold evt{};
        evt.manifoldType = static_cast<std::uint8_t>(msg.getInt(0, 0));
        eventBus_.post(evt);
    } else if (addr == "/epi/foliation/rotor" || addr == "/epi/foliation/nudge") {
        EventRotorNudge evt{};
        evt.deltaAlpha = msg.getFloat(0, 0.0f);
        evt.deltaBeta = msg.getFloat(1, 0.0f);
        evt.deltaTheta4D = msg.getFloat(2, 0.0f);
        eventBus_.post(evt);
    } else if (addr == "/epi/deck/crossfade") {
        eventBus_.post(EventPerformanceDeckControl{
            .action = EventPerformanceDeckControl::Action::SetCrossfadePosition,
            .value = msg.getFloat(0, 0.5f)
        });
    } else if (addr == "/epi/deck/autocrossfade") {
        eventBus_.post(EventPerformanceDeckControl{
            .action = EventPerformanceDeckControl::Action::TriggerAutoCrossfade,
            .value = msg.getFloat(0, 4.0f)
        });
    } else if (addr == "/epi/deck/beatsync") {
        eventBus_.post(EventPerformanceDeckControl{
            .action = EventPerformanceDeckControl::Action::TriggerBeatSyncedCrossfade,
            .value = msg.getFloat(0, 2.0f)
        });
    } else if (addr == "/epi/deck/bpm") {
        eventBus_.post(EventPerformanceDeckControl{
            .action = EventPerformanceDeckControl::Action::SetBpm,
            .value = msg.getFloat(0, 120.0f)
        });
    } else if (addr == "/epi/deck/tap") {
        eventBus_.post(EventPerformanceDeckControl{
            .action = EventPerformanceDeckControl::Action::TapTempo
        });
    } else if (addr == "/epi/deck/swap") {
        eventBus_.post(EventPerformanceDeckControl{
            .action = EventPerformanceDeckControl::Action::SwapDecks
        });
    } else if (addr == "/epi/deck/tapestop") {
        eventBus_.post(EventPerformanceDeckControl{
            .action = EventPerformanceDeckControl::Action::TriggerTapeStop,
            .value = msg.getFloat(0, 1.2f)
        });
    } else if (addr == "/epi/deck/beatroll") {
        eventBus_.post(EventPerformanceDeckControl{
            .action = EventPerformanceDeckControl::Action::TriggerBeatRoll,
            .value = msg.getFloat(0, 0.25f)
        });
    } else if (addr == "/epi/deck/chaos") {
        eventBus_.post(EventPerformanceDeckControl{
            .action = EventPerformanceDeckControl::Action::TriggerChaosBurst,
            .value = msg.getFloat(0, 1.0f)
        });
    } else if (addr == "/epi/deck/filter") {
        eventBus_.post(EventPerformanceDeckControl{
            .action = EventPerformanceDeckControl::Action::SetMasterFilter,
            .value = msg.getFloat(0, 0.0f)
        });
    } else if (addr == "/epi/deck/cue/recall") {
        const auto bank = static_cast<std::size_t>(std::clamp(msg.getInt(0, 0), 0, 9));
        eventBus_.post(EventPerformanceDeckControl{
            .action = EventPerformanceDeckControl::Action::RecallCueToOpposite,
            .bankIndex = bank
        });
    } else if (addr == "/epi/deck/cue/store") {
        const auto bank = static_cast<std::size_t>(std::clamp(msg.getInt(0, 0), 0, 9));
        eventBus_.post(EventPerformanceDeckControl{
            .action = EventPerformanceDeckControl::Action::StoreCue,
            .bankIndex = bank
        });
    } else if (addr == "/epi/deck/nudge") {
        eventBus_.post(EventPerformanceDeckControl{
            .action = EventPerformanceDeckControl::Action::NudgeCrossfade,
            .value = msg.getFloat(0, 0.05f)
        });
    } else if (addr == "/epi/deck/toursync") {
        eventBus_.post(EventPerformanceDeckControl{
            .action = EventPerformanceDeckControl::Action::ToggleTourBeatSync
        });
    } else if (addr == "/epi/midi/profile") {
        eventBus_.post(EventMidiProfile{
            .profileName = std::string(msg.getString(0, "dj"))
        });
    } else if (addr == "/epi/timeline/play") {
        eventBus_.post(EventTimelineControl{EventTimelineControl::Action::Play});
    } else if (addr == "/epi/timeline/pause") {
        eventBus_.post(EventTimelineControl{EventTimelineControl::Action::Pause});
    } else if (addr == "/epi/timeline/stop") {
        eventBus_.post(EventTimelineControl{EventTimelineControl::Action::Stop});
    } else if (addr == "/epi/timeline/toggle") {
        eventBus_.post(EventTimelineControl{EventTimelineControl::Action::TogglePlayPause});
    } else if (addr == "/epi/timeline/next") {
        eventBus_.post(EventTimelineControl{EventTimelineControl::Action::NextCue});
    } else if (addr == "/epi/timeline/prev") {
        eventBus_.post(EventTimelineControl{EventTimelineControl::Action::PreviousCue});
    } else if (addr == "/epi/timeline/cue") {
        eventBus_.post(EventTimelineControl{
            .action = EventTimelineControl::Action::SkipToCue,
            .cueIndex = static_cast<std::size_t>(std::max(0, msg.getInt(0, 0)))
        });
    } else if (addr == "/epi/timeline/load") {
        eventBus_.post(EventTimelineControl{
            .action = EventTimelineControl::Action::LoadFile,
            .argument = std::string(msg.getString(0))
        });
    } else if (addr == "/epi/timeline/tour") {
        eventBus_.post(EventTimelineControl{
            .action = EventTimelineControl::Action::BuildTour,
            .argument = std::string(msg.getString(0)),
            .dwellSec = msg.getFloat(1, 4.0f),
            .transSec = msg.getFloat(2, 2.0f)
        });
    } else if (addr == "/epi/sequence/start") {
        eventBus_.post(EventSequenceRecorderControl{
            .action = EventSequenceRecorderControl::Action::Start,
            .sessionName = std::string(msg.getString(0))
        });
    } else if (addr == "/epi/sequence/stop") {
        eventBus_.post(EventSequenceRecorderControl{
            .action = EventSequenceRecorderControl::Action::Stop
        });
    } else if (addr == "/epi/sequence/toggle") {
        eventBus_.post(EventSequenceRecorderControl{
            .action = EventSequenceRecorderControl::Action::Toggle,
            .sessionName = std::string(msg.getString(0))
        });
    } else if (addr == "/epi/palette/toggle") {
        eventBus_.post(EventPaletteAgentControl{
            .action = EventPaletteAgentControl::Action::Toggle
        });
    } else if (addr == "/epi/palette/mode") {
        eventBus_.post(EventPaletteAgentControl{
            .action = EventPaletteAgentControl::Action::SetMode,
            .mode = static_cast<std::uint8_t>(std::clamp(msg.getInt(0, 0), 0, 3))
        });
    } else if (addr == "/epi/palette/stimulate") {
        eventBus_.post(EventPaletteAgentControl{
            .action = EventPaletteAgentControl::Action::Stimulate,
            .intensity = msg.getFloat(0, 0.5f)
        });
    } else if (addr == "/epi/palette/reseed") {
        eventBus_.post(EventPaletteAgentControl{
            .action = EventPaletteAgentControl::Action::Reseed,
            .seed = static_cast<std::uint32_t>(msg.getInt(0, 0xE91C0DEu))
        });
    } else if (addr == "/epi/morphspace/cursor") {
        eventBus_.post(EventMorphSpaceControl{
            .action = EventMorphSpaceControl::Action::SetCursor,
            .cursorX = msg.getFloat(0, 0.5f),
            .cursorY = msg.getFloat(1, 0.5f)
        });
    } else if (addr == "/epi/morphspace/nudge") {
        eventBus_.post(EventMorphSpaceControl{
            .action = EventMorphSpaceControl::Action::NudgeCursor,
            .cursorX = msg.getFloat(0, 0.0f),
            .cursorY = msg.getFloat(1, 0.0f)
        });
    } else if (addr == "/epi/morphspace/toggle") {
        eventBus_.post(EventMorphSpaceControl{
            .action = EventMorphSpaceControl::Action::ToggleActive
        });
    } else if (addr == "/epi/morphspace/motion") {
        eventBus_.post(EventMorphSpaceControl{
            .action = EventMorphSpaceControl::Action::SetMotionMode,
            .mode = static_cast<std::uint8_t>(std::clamp(msg.getInt(0, 0), 0, 5))
        });
    } else if (addr == "/epi/morphspace/motion/cycle") {
        eventBus_.post(EventMorphSpaceControl{
            .action = EventMorphSpaceControl::Action::CycleMotionMode
        });
    } else if (addr == "/epi/morphspace/speed") {
        eventBus_.post(EventMorphSpaceControl{
            .action = EventMorphSpaceControl::Action::SetMotionSpeed,
            .speed = msg.getFloat(0, 0.5f)
        });
    } else if (addr == "/epi/morphspace/power") {
        eventBus_.post(EventMorphSpaceControl{
            .action = EventMorphSpaceControl::Action::SetShepardPower,
            .shepardPower = msg.getFloat(0, 2.0f)
        });
    } else if (addr == "/epi/morphspace/bank") {
        if (!msg.getString(0).empty()) {
            eventBus_.post(EventMorphSpaceControl{
                .action = EventMorphSpaceControl::Action::LoadBank,
                .bankName = std::string(msg.getString(0))
            });
        } else {
            eventBus_.post(EventMorphSpaceControl{
                .action = EventMorphSpaceControl::Action::LoadBank,
                .bankIndex = static_cast<std::size_t>(std::max(0, msg.getInt(0, 0)))
            });
        }
    } else if (addr == "/epi/morphspace/bank/cycle") {
        eventBus_.post(EventMorphSpaceControl{
            .action = EventMorphSpaceControl::Action::CycleBank
        });
    } else if (addr == "/epi/morphspace/path/toggle") {
        eventBus_.post(EventMorphSpaceControl{
            .action = EventMorphSpaceControl::Action::TogglePath
        });
    } else if (addr == "/epi/morphspace/path/clear") {
        eventBus_.post(EventMorphSpaceControl{
            .action = EventMorphSpaceControl::Action::ClearPath
        });
    } else if (addr == "/epi/morphspace/path/waypoint") {
        eventBus_.post(EventMorphSpaceControl{
            .action = EventMorphSpaceControl::Action::AddWaypoint,
            .cursorX = msg.getFloat(0, 0.5f),
            .cursorY = msg.getFloat(1, 0.5f),
            .dwellSec = msg.getFloat(2, 2.0f),
            .travelSec = msg.getFloat(3, 2.0f)
        });
    } else if (addr == "/epi/morphspace/sync") {
        eventBus_.post(EventMorphSpaceControl{
            .action = EventMorphSpaceControl::Action::SyncCategory
        });
    } else if (addr == "/epi/morphspace/bind") {
        eventBus_.post(EventMorphSpaceControl{
            .action = EventMorphSpaceControl::Action::BindCategory,
            .category = std::string(msg.getString(0, "dimensional"))
        });
    }
}

void SculptureRemoteServer::setTargetEndpoint(std::string_view host, uint16_t port) {
    std::lock_guard<std::mutex> lock(sendMutex_);
    targetHost_ = std::string(host);
    targetPort_ = port;
#if defined(__unix__) || defined(__linux__) || defined(__APPLE__)
    std::memset(&targetAddr_, 0, sizeof(targetAddr_));
    targetAddr_.sin_family = AF_INET;
    targetAddr_.sin_port = htons(targetPort_);
    if (::inet_pton(AF_INET, targetHost_.c_str(), &targetAddr_.sin_addr) > 0) {
        hasTargetAddr_ = true;
    } else {
        hasTargetAddr_ = false;
    }
#endif
}

void SculptureRemoteServer::sendOscMessage(std::string_view address, const std::vector<OscArg>& args) {
#if defined(__unix__) || defined(__linux__) || defined(__APPLE__)
    if (socketFd_ < 0) return;
    const auto packet = SculptureOscEncoder::encodeMessage(address, args);
    std::lock_guard<std::mutex> lock(sendMutex_);
    if (hasTargetAddr_) {
        ::sendto(socketFd_, packet.data(), packet.size(), 0,
                 reinterpret_cast<const sockaddr*>(&targetAddr_), sizeof(targetAddr_));
    }
#endif
}

void SculptureRemoteServer::sendOscFloat(std::string_view address, float value) {
#if defined(__unix__) || defined(__linux__) || defined(__APPLE__)
    if (socketFd_ < 0) return;
    const auto packet = SculptureOscEncoder::encodeFloat(address, value);
    std::lock_guard<std::mutex> lock(sendMutex_);
    if (hasTargetAddr_) {
        ::sendto(socketFd_, packet.data(), packet.size(), 0,
                 reinterpret_cast<const sockaddr*>(&targetAddr_), sizeof(targetAddr_));
    }
#endif
}

void SculptureRemoteServer::sendOscInt(std::string_view address, int32_t value) {
#if defined(__unix__) || defined(__linux__) || defined(__APPLE__)
    if (socketFd_ < 0) return;
    const auto packet = SculptureOscEncoder::encodeInt(address, value);
    std::lock_guard<std::mutex> lock(sendMutex_);
    if (hasTargetAddr_) {
        ::sendto(socketFd_, packet.data(), packet.size(), 0,
                 reinterpret_cast<const sockaddr*>(&targetAddr_), sizeof(targetAddr_));
    }
#endif
}

void SculptureRemoteServer::sendOscString(std::string_view address, std::string_view value) {
#if defined(__unix__) || defined(__linux__) || defined(__APPLE__)
    if (socketFd_ < 0) return;
    const auto packet = SculptureOscEncoder::encodeString(address, value);
    std::lock_guard<std::mutex> lock(sendMutex_);
    if (hasTargetAddr_) {
        ::sendto(socketFd_, packet.data(), packet.size(), 0,
                 reinterpret_cast<const sockaddr*>(&targetAddr_), sizeof(targetAddr_));
    }
#endif
}

void SculptureRemoteServer::broadcastTelemetry(
    float fps,
    float morphProgress,
    float simulatedTime,
    float crossfadePosition,
    float bpm,
    bool isRecording,
    std::string_view activeCueTitle,
    bool morphSpaceActive,
    float morphSpaceX,
    float morphSpaceY,
    std::string_view morphSpaceBank
) {
    sendOscFloat("/epi/hud/fps", fps);
    sendOscFloat("/epi/hud/morph", morphProgress);
    sendOscFloat("/epi/hud/time", simulatedTime);
    sendOscFloat("/epi/deck/crossfade", crossfadePosition);
    sendOscFloat("/epi/deck/bpm", bpm);
    sendOscInt("/epi/sequence/recording", isRecording ? 1 : 0);
    if (!activeCueTitle.empty()) {
        sendOscString("/epi/hud/cue", activeCueTitle);
    }
    if (morphSpaceActive) {
        sendOscInt("/epi/morphspace/active", 1);
        sendOscFloat("/epi/morphspace/x", morphSpaceX);
        sendOscFloat("/epi/morphspace/y", morphSpaceY);
        if (!morphSpaceBank.empty()) {
            sendOscString("/epi/morphspace/bank", morphSpaceBank);
        }
    }
}

} // namespace epi_sculpt
