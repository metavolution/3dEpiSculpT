#pragma once

#include "SculptureProductionRoute.h"
#include "SculptureProductionTiming.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace epi_sculpt {

// Control-plane description of one frame's production opportunity. It carries
// no OpenCL/Vulkan objects and no producer-specific parameter object: resource
// ownership and cadence history remain in their owning subsystems. The schedule
// answers what is due and at which submission boundary it may be attempted.
enum class SculptureProductionSubmissionPhase : std::uint8_t {
    None,
    DispatchGpu,
    FrameRecording,
};

// Stable route metadata.  Callers choose *what* mathematical production they
// need; this table owns *where* that route may legally enter the frame. Keeping
// the mapping next to the schedule vocabulary prevents application/demo/test
// code from independently inventing submission phases.
struct SculptureProductionRouteDescriptor final {
    SculptureProductionSubmissionPhase submissionPhase =
        SculptureProductionSubmissionPhase::None;
    bool schedulable = false;
    // True when the concrete request cannot be materialized until a camera and
    // recording extent exist. This is descriptive metadata, not permission for
    // presentation code to become a scheduler.
    bool viewDependent = false;
};

[[nodiscard]] constexpr SculptureProductionRouteDescriptor describeProductionRoute(
    SculptureProductionRoute route) noexcept {
    using Phase = SculptureProductionSubmissionPhase;
    switch (route) {
    case SculptureProductionRoute::CompactInstances:
        return {Phase::DispatchGpu, true, false};
    case SculptureProductionRoute::CanonicalFeedback:
        return {Phase::FrameRecording, true, false};
    case SculptureProductionRoute::CanonicalFeedbackUnavailable:
        return {Phase::None, false, false};
    case SculptureProductionRoute::DedicatedWorld:
        return {Phase::DispatchGpu, true, false};
    case SculptureProductionRoute::ImplicitField:
        return {Phase::FrameRecording, true, true};
    }
    return {};
}

[[nodiscard]] constexpr SculptureProductionSubmissionPhase submissionPhase(
    SculptureProductionRoute route) noexcept {
    return describeProductionRoute(route).submissionPhase;
}

struct SculptureProductionSchedule final {
    SculptureProductionRoute route = SculptureProductionRoute::CompactInstances;
    SculptureProductionSubmissionPhase phase = SculptureProductionSubmissionPhase::None;
    // Nonzero only for a due submission opportunity. Together with
    // frameSequence this gives downstream receipts a stable control-plane
    // identity without introducing a GPU/resource handle into the schedule.
    std::uint64_t scheduleSequence = 0U;
    std::uint64_t frameSequence = 0U;
    SculptureProductionTiming timing{};
    std::uint64_t generationIntent = 0U;
    std::uint64_t resetEpoch = 0U;
    // Semantic/presentation lease copied from the immutable frame plan when a
    // route has one. Native transport leases remain producer-owned.
    std::uint64_t semanticLeaseEpoch = 0U;

    [[nodiscard]] bool submitDue() const noexcept {
        return timing.submit && phase != SculptureProductionSubmissionPhase::None;
    }

    [[nodiscard]] bool deferredToFrameRecording() const noexcept {
        return submitDue() && phase == SculptureProductionSubmissionPhase::FrameRecording;
    }
};

// Bounded, allocation-free collection of production opportunities attached to
// one immutable frame plan. Most routes carry one primary producer, while
// UnifiedComposite can carry compact + implicit opportunities in the same frame.
// Keeping this set in the control plane removes the
// structural assumption that a frame can describe only one producer without
// granting any ownership over those producers or their GPU resources.
class SculptureProductionScheduleSet final {
public:
    static constexpr std::size_t kCapacity = 8U;

    void clear() noexcept { count_ = 0U; }

    // Current consumers resolve an opportunity by route. Until contributions
    // carry a separate identity, one frame cannot represent two of that route.
    [[nodiscard]] bool canAppend(SculptureProductionRoute route,
                                 std::uint64_t frameSequence) const noexcept {
        if (full()) return false;
        if (count_ != 0U && schedules_[0].frameSequence != frameSequence) return false;
        return find(route) == nullptr;
    }

    [[nodiscard]] SculptureProductionSchedule* append(
        const SculptureProductionSchedule& schedule) noexcept {
        if (!canAppend(schedule.route, schedule.frameSequence)) return nullptr;
        schedules_[count_] = schedule;
        return &schedules_[count_++];
    }

    [[nodiscard]] bool add(const SculptureProductionSchedule& schedule) noexcept {
        return append(schedule) != nullptr;
    }

    // Used only to roll back a just-appended control-plane record when its
    // paired receipt could not be created. Removal is intentionally LIFO so
    // stable pointers/references to earlier schedules are never invalidated.
    [[nodiscard]] bool removeLastIf(std::uint64_t scheduleSequence) noexcept {
        if (count_ == 0U || schedules_[count_ - 1U].scheduleSequence != scheduleSequence) {
            return false;
        }
        schedules_[--count_] = {};
        return true;
    }

    [[nodiscard]] std::span<const SculptureProductionSchedule> schedules() const noexcept {
        return {schedules_.data(), count_};
    }

    [[nodiscard]] std::span<SculptureProductionSchedule> schedules() noexcept {
        return {schedules_.data(), count_};
    }

    [[nodiscard]] const SculptureProductionSchedule* find(
        SculptureProductionRoute route) const noexcept {
        for (std::size_t i = 0U; i < count_; ++i) {
            if (schedules_[i].route == route) return &schedules_[i];
        }
        return nullptr;
    }

    [[nodiscard]] SculptureProductionSchedule* find(
        SculptureProductionRoute route) noexcept {
        for (std::size_t i = 0U; i < count_; ++i) {
            if (schedules_[i].route == route) return &schedules_[i];
        }
        return nullptr;
    }

    [[nodiscard]] bool empty() const noexcept { return count_ == 0U; }
    [[nodiscard]] bool full() const noexcept { return count_ == schedules_.size(); }
    [[nodiscard]] std::size_t size() const noexcept { return count_; }

private:
    std::array<SculptureProductionSchedule, kCapacity> schedules_{};
    std::size_t count_ = 0U;
};

} // namespace epi_sculpt
