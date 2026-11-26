#include "SculptureWorldTourChoreographer.h"
#include "SculptureWorldNavigator.h"
#include "SculptureWorldRecipe.h"

#include <cmath>
#include <algorithm>
#include <cstdio>

namespace epi_sculpt {

namespace {

inline float quinticSmootherstep(float x) noexcept {
    const float c = std::clamp(x, 0.0f, 1.0f);
    return c * c * c * (c * (c * 6.0f - 15.0f) + 10.0f);
}

inline mgfx::math::vec3 safeNormalize(
    const mgfx::math::vec3& v,
    const mgfx::math::vec3& fallback = {0.0f, 0.0f, 1.0f}
) noexcept {
    const float lenSq = v.x * v.x + v.y * v.y + v.z * v.z;
    if (lenSq < 1e-6f) return fallback;
    return v / std::sqrt(lenSq);
}

} // namespace

SculptureWorldTourChoreographer::SculptureWorldTourChoreographer(WorldTourConfig config)
    : config_(std::move(config)) {}

void SculptureWorldTourChoreographer::setPlaylist(
    std::vector<std::string> presetIds,
    std::string_view tourName
) {
    playlist_ = std::move(presetIds);
    config_.tourName = tourName;
    config_.mode = WorldTourMode::CuratedPlaylist;
    currentWaypointIndex_ = 0U;
    reset();
}

void SculptureWorldTourChoreographer::setTourMode(WorldTourMode mode) noexcept {
    config_.mode = mode;
    reset();
}

void SculptureWorldTourChoreographer::cycleTourMode() noexcept {
    switch (config_.mode) {
    case WorldTourMode::SequentialCatalog:
        if (!playlist_.empty()) {
            config_.mode = WorldTourMode::CuratedPlaylist;
        } else {
            config_.mode = WorldTourMode::ShuffleCatalog;
        }
        break;
    case WorldTourMode::CuratedPlaylist:
        config_.mode = WorldTourMode::ShuffleCatalog;
        break;
    case WorldTourMode::ShuffleCatalog:
    default:
        config_.mode = WorldTourMode::SequentialCatalog;
        break;
    }
    reset();
}

void SculptureWorldTourChoreographer::reset() noexcept {
    dwellTimer_ = 0.0f;
    transitionTimer_ = 0.0f;
    userInactivityTimer_ = 0.0f;
    isTransitioning_ = false;
    userIntervened_ = false;
}

void SculptureWorldTourChoreographer::enable(bool active) noexcept {
    config_.enabled = active;
    if (active) {
        reset();
    }
}

void SculptureWorldTourChoreographer::notifyUserInteraction() noexcept {
    userIntervened_ = true;
    userInactivityTimer_ = 0.0f;
}

std::size_t SculptureWorldTourChoreographer::computeNextIndex(const SculptureWorldNavigator& navigator) const noexcept {
    if (config_.mode == WorldTourMode::CuratedPlaylist && !playlist_.empty()) {
        return (currentWaypointIndex_ + 1U) % playlist_.size();
    }
    const std::size_t total = navigator.count();
    if (total == 0U) return 0U;

    if (config_.mode == WorldTourMode::ShuffleCatalog && total > 2U) {
        // Fast pseudo-random hop avoiding immediately repeated index
        const std::size_t current = navigator.activeIndex();
        std::size_t step = (current * 7U + 5U) % (total - 1U) + 1U;
        return (current + step) % total;
    }
    return (navigator.activeIndex() + 1U) % total;
}

std::optional<WorldGeneratorKind> SculptureWorldTourChoreographer::peekNextGenerator(
    const SculptureWorldNavigator& navigator
) const noexcept {
    if (!config_.enabled || isTransitioning_ || userIntervened_) {
        return std::nullopt;
    }
    const std::size_t nextIdx = computeNextIndex(navigator);
    if (config_.mode == WorldTourMode::CuratedPlaylist && !playlist_.empty()) {
        const std::string& nextPreset = playlist_[nextIdx % playlist_.size()];
        if (const auto* entry = findWorldCatalogEntry(nextPreset)) {
            return entry->generator;
        }
        return std::nullopt;
    }
    if (const auto* entry = navigator.catalogEntryAt(nextIdx)) {
        return entry->generator;
    }
    return std::nullopt;
}

void SculptureWorldTourChoreographer::startTransition(
    SculptureWorldNavigator& navigator,
    std::size_t targetIndex
) {
    catalogSize_ = navigator.count();
    fromCamera_ = navigator.computeFrameCamera();
    fromHint_ = navigator.activeFrameHint();
    currentPresetId_ = navigator.activePresetName();

    const bool curated = config_.mode == WorldTourMode::CuratedPlaylist && !playlist_.empty();
    const std::size_t waypoint = curated ? targetIndex % playlist_.size() : targetIndex;
    const bool selected = curated
        ? navigator.selectWorld(playlist_[waypoint], config_.transitionSeconds)
        : navigator.selectWorld(waypoint, config_.transitionSeconds);
    if (!selected) {
        // Do not fly toward a rejected world or retry it invisibly every tick.
        reset();
        enable(false);
        nextPresetId_.clear();
        std::fprintf(stderr, "[SculptureWorldTourChoreographer] Stopped: unsupported waypoint %zu\n",
            waypoint);
        return;
    }
    currentWaypointIndex_ = waypoint;
    if (navigator.isTransitioning()) {
        nextPresetId_ = navigator.transitionState().toPresetName;
        toHint_ = navigator.transitionState().toHint;
    } else {
        nextPresetId_ = navigator.activePresetName();
        toHint_ = navigator.activeFrameHint();
    }

    toCamera_ = mgfx::Camera{
        .eye = mgfx::math::vec3(toHint_.eyeX, toHint_.eyeY, toHint_.eyeZ),
        .target = mgfx::math::vec3(toHint_.targetX, toHint_.targetY, toHint_.targetZ),
        .up = mgfx::math::vec3(0.0f, 1.0f, 0.0f),
        .fovYRad = fromCamera_.fovYRad,
    };

    isTransitioning_ = true;
    transitionTimer_ = 0.0f;

    std::fprintf(stderr,
        "[SculptureWorldTourChoreographer] Autopilot waypoint transition: %s -> %s (flight: %.1fs)\n",
        currentPresetId_.c_str(), nextPresetId_.c_str(), config_.transitionSeconds);
}

void SculptureWorldTourChoreographer::finishTransition(SculptureWorldNavigator& /*navigator*/) {
    isTransitioning_ = false;
    transitionTimer_ = 0.0f;
    dwellTimer_ = 0.0f;
    currentPresetId_ = nextPresetId_;
}

void SculptureWorldTourChoreographer::tick(SculptureWorldNavigator& navigator, float deltaSeconds) {
    catalogSize_ = navigator.count();
    if (!config_.enabled) {
        return;
    }

    // Inactivity timeout logic: yield autopilot control to human user
    if (userIntervened_) {
        userInactivityTimer_ += deltaSeconds;
        if (userInactivityTimer_ >= config_.inactivityResumeSeconds) {
            userIntervened_ = false;
            userInactivityTimer_ = 0.0f;
            dwellTimer_ = 0.0f;
            std::fprintf(stderr,
                "[SculptureWorldTourChoreographer] Inactivity timeout reached (%.1fs). Resuming tour autopilot.\n",
                config_.inactivityResumeSeconds);
        } else {
            return;
        }
    }

    if (isTransitioning_) {
        transitionTimer_ += deltaSeconds;
        if (transitionTimer_ >= config_.transitionSeconds) {
            finishTransition(navigator);
        }
    } else {
        dwellTimer_ += deltaSeconds;
        if (dwellTimer_ >= config_.dwellSeconds) {
            dwellTimer_ = 0.0f;
            const std::size_t nextIdx = computeNextIndex(navigator);
            startTransition(navigator, nextIdx);
        }
    }
}

std::optional<mgfx::Camera> SculptureWorldTourChoreographer::activeFlightCamera(float /*baseFovYRad*/) const noexcept {
    if (!isTransitioning_ || !config_.enableAutoCameraGlide) {
        return std::nullopt;
    }

    const float rawT = config_.transitionSeconds > 0.0f
        ? (transitionTimer_ / config_.transitionSeconds)
        : 1.0f;
    const float s = quinticSmootherstep(rawT);

    // 1. Smooth target interpolation
    const mgfx::math::vec3 target = (1.0f - s) * fromCamera_.target + s * toCamera_.target;

    // 2. Geodesic radial orbit interpolation with clearance elevation dome
    mgfx::math::vec3 fromDir = fromCamera_.eye - fromCamera_.target;
    mgfx::math::vec3 toDir = toCamera_.eye - toCamera_.target;
    const float fromRadius = std::sqrt(fromDir.x * fromDir.x + fromDir.y * fromDir.y + fromDir.z * fromDir.z);
    const float toRadius = std::sqrt(toDir.x * toDir.x + toDir.y * toDir.y + toDir.z * toDir.z);

    fromDir = safeNormalize(fromDir);
    toDir = safeNormalize(toDir);

    const mgfx::math::vec3 flightDir = safeNormalize((1.0f - s) * fromDir + s * toDir, fromDir);
    const float clearanceDome = config_.orbitalArcHeight * std::sin(s * 3.1415926535f);
    const float flightRadius = (1.0f - s) * fromRadius + s * toRadius + clearanceDome;

    const mgfx::math::vec3 eye = target + flightDir * flightRadius;
    const float fov = (1.0f - s) * fromCamera_.fovYRad + s * toCamera_.fovYRad;

    return mgfx::Camera{
        .eye = eye,
        .target = target,
        .up = mgfx::math::vec3(0.0f, 1.0f, 0.0f),
        .fovYRad = fov,
    };
}

WorldTourStatus SculptureWorldTourChoreographer::status() const noexcept {
    const float progress = config_.transitionSeconds > 0.0f
        ? std::clamp(transitionTimer_ / config_.transitionSeconds, 0.0f, 1.0f)
        : 0.0f;

    const std::size_t total = (config_.mode == WorldTourMode::CuratedPlaylist && !playlist_.empty())
        ? playlist_.size()
        : catalogSize_;

    const std::size_t wpIdx = (config_.mode == WorldTourMode::CuratedPlaylist && !playlist_.empty())
        ? (currentWaypointIndex_ % playlist_.size())
        : currentWaypointIndex_;

    return WorldTourStatus{
        .active = config_.enabled,
        .inTransition = isTransitioning_,
        .userIntervention = userIntervened_,
        .waypointIndex = wpIdx,
        .totalWaypoints = total,
        .timeRemainingInDwell = std::max(0.0f, config_.dwellSeconds - dwellTimer_),
        .transitionProgress = progress,
        .currentPresetId = currentPresetId_,
        .nextPresetId = nextPresetId_,
        .tourName = config_.tourName,
    };
}

void SculptureWorldTourChoreographer::advanceNext(SculptureWorldNavigator& navigator) {
    dwellTimer_ = config_.dwellSeconds;
    const std::size_t nextIdx = computeNextIndex(navigator);
    startTransition(navigator, nextIdx);
}

void SculptureWorldTourChoreographer::advancePrev(SculptureWorldNavigator& navigator) {
    dwellTimer_ = config_.dwellSeconds;
    if (config_.mode == WorldTourMode::CuratedPlaylist && !playlist_.empty()) {
        const std::size_t prevIdx = (currentWaypointIndex_ + playlist_.size() - 1U) % playlist_.size();
        startTransition(navigator, prevIdx);
    } else {
        const std::size_t total = navigator.count();
        if (total > 0U) {
            const std::size_t prevIdx = (navigator.activeIndex() + total - 1U) % total;
            startTransition(navigator, prevIdx);
        }
    }
}

} // namespace epi_sculpt
