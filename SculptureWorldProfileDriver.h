// Application-control-plane driver for one bounded dedicated-world profile.
// It observes immutable records only: it cannot change OpenCL generation,
// renderer batching, or quality policy.
#pragma once

#include "SculptureWorldProducer.h"
#include "SculptureWorldProfileSession.h"
#include "SculptureWorldProfileSidecar.h"

#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace epi_sculpt {

struct WorldProfileCompletion {
    bool sidecarWritten = false;
    std::string path;
    WorldProfileSummary summary;
};

class SculptureWorldProfileDriver {
public:
    void start(WorldProfileIdentity identity, WorldProfileSessionConfig config,
               std::string outputPath) {
        session_.emplace(std::move(identity), config);
        outputPath_ = std::move(outputPath);
        completionPublished_ = false;
    }

    [[nodiscard]] bool active() const noexcept {
        return session_.has_value() && !completionPublished_;
    }

    [[nodiscard]] const WorldProfileSummary* summary() const noexcept {
        return session_ ? &session_->summary() : nullptr;
    }

    [[nodiscard]] const WorldProfileSummary* completedSummary() const noexcept {
        return session_ && session_->complete() ? &session_->summary() : nullptr;
    }

    [[nodiscard]] std::string_view stateName() const noexcept {
        if (!session_) return "inactive";
        if (session_->complete()) return "complete";
        if (session_->state() == WorldProfileSessionState::Aborted) return "aborted";
        if (session_->state() == WorldProfileSessionState::Sampling) return "sampling";
        return "settling";
    }

    // Returns exactly one completion result. Missing producer readiness is an
    // explicit interrupted profile, while missing optional timing remains an
    // unavailable field in the otherwise valid sidecar.
    [[nodiscard]] std::optional<WorldProfileCompletion> observe(
        float deltaSeconds, bool producerReady,
        const WorldSnapshotDelivery& delivery,
        const WorldBridgeMetrics& bridge,
        const WorldPresentationMetrics* presentation = nullptr) {
        if (!active()) return std::nullopt;
        if (!producerReady) session_->abort();
        session_->advance(deltaSeconds);
        if (delivery.metrics != nullptr && bridge.matches(delivery.generation)) {
            static_cast<void>(session_->observe(
                *delivery.metrics, bridge, presentation));
        }
        if (!session_->finished()) return std::nullopt;

        completionPublished_ = true;
        WorldProfileCompletion result{
            .sidecarWritten = SculptureWorldProfileSidecar::writeToFile(
                outputPath_, session_->summary()),
            .path = outputPath_,
            .summary = session_->summary(),
        };
        return result;
    }

private:
    std::optional<SculptureWorldProfileSession> session_;
    std::string outputPath_;
    bool completionPublished_ = false;
};

} // namespace epi_sculpt
