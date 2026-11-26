#pragma once

#include "SculptureEventBus.h"
#include "mgfx/vk/CaptureLifecycle.h"
#include <cstdio>
#include <memory>
#include <optional>
#include <utility>

namespace epi_sculpt {

// App intent only: no pixels, GPU resources, or completion ownership. Busy
// admission preserves intent; successful admission transfers it to FrameLoop.
class SculptureCaptureAdmission {
public:
    enum class Kind { Snapshot, Sequence };
    struct Request {
        Kind kind;
        std::uint32_t sequenceIndex;
        std::uint64_t sequenceSession;
    };

    void onEvent(const EventCapture&) noexcept { snapshot_ = true; }
    void onEvent(const EventSnapshotControl& event) noexcept {
        if (event.action == EventSnapshotControl::Action::Request) snapshot_ = true;
    }
    void onEvent(const EventSequenceRecorderControl& event) noexcept {
        switch (event.action) {
        case EventSequenceRecorderControl::Action::Start: start(); break;
        case EventSequenceRecorderControl::Action::Stop: recording_ = false; break;
        case EventSequenceRecorderControl::Action::Toggle: toggle(); break;
        }
    }
    void onEvent(const EventToggleState& event) noexcept {
        if (event.target == EventToggleState::Target::RecordSequence) toggle();
    }
    [[nodiscard]] bool snapshotPending() const noexcept { return snapshot_; }
    [[nodiscard]] bool recording() const noexcept { return recording_; }
    [[nodiscard]] std::uint32_t nextSequenceIndex() const noexcept { return index_; }

    // Called once at the app's frame boundary. Submit may construct immutable
    // capture metadata; if it throws, no intent/index is consumed.
    template<class Submit>
    std::optional<mgfx::vk::CaptureAdmission> admit(Submit&& submit) {
        if (!snapshot_ && !recording_) return std::nullopt;
        const Request request{snapshot_ ? Kind::Snapshot : Kind::Sequence, index_, session_};
        const auto result = std::forward<Submit>(submit)(request);
        switch (result) {
        case mgfx::vk::CaptureAdmission::Accepted:
            if (request.kind == Kind::Snapshot) snapshot_ = false;
            else ++index_;
            break;
        case mgfx::vk::CaptureAdmission::Busy:
            break;
        case mgfx::vk::CaptureAdmission::Closed:
        case mgfx::vk::CaptureAdmission::Invalid:
            snapshot_ = false;
            recording_ = false;
            break;
        }
        return result;
    }

    // Callback captures own their metadata; only successful readbacks reach IO.
    template<class Save>
    static mgfx::vk::CaptureOutcomeFn completedOnly(Save save) {
        return [save = std::move(save)](const mgfx::vk::CaptureOutcome& outcome) {
            if (outcome.status == mgfx::vk::CaptureStatus::Completed) save(outcome.image);
            else std::fprintf(stderr, "[SculptureApplication] capture ended (%d)\n",
                              static_cast<int>(outcome.status));
        };
    }

    // Snapshot callbacks retain their originating observation envelope and full
    // readback outcome. FrameLoop cancels them before device replacement; a
    // serial from another FrameLoop must never be joined to this namespace.
    template<class Observation, class Save>
    static mgfx::vk::CaptureOutcomeFn joinedSnapshotOnly(
        std::shared_ptr<Observation> observation, Save save) {
        return [observation = std::move(observation), save = std::move(save)]
               (const mgfx::vk::CaptureOutcome& outcome) mutable {
            if (outcome.status == mgfx::vk::CaptureStatus::Completed && observation
                && observation->identity.matchesReadback(outcome.submissionSerial, outcome.submitted)) {
                save(outcome, *observation);
            } else {
                std::fprintf(stderr,
                    "[SculptureApplication] snapshot rejected: no completed, joined submission "
                    "(status %d, serial %llu)\n", static_cast<int>(outcome.status),
                    static_cast<unsigned long long>(outcome.submissionSerial));
            }
        };
    }

private:
    void start() noexcept {
        if (!recording_) { index_ = 0; ++session_; }
        recording_ = true;
    }
    void toggle() noexcept {
        if (recording_) recording_ = false;
        else start();
    }
    bool snapshot_ = false;
    bool recording_ = false;
    std::uint32_t index_ = 0;
    std::uint64_t session_ = 0;
};

} // namespace epi_sculpt
