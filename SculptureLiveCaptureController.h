#pragma once

#include "SculptureProgramSchema.h"
#include "SculptureSerializer.h"
#include "SculptureStateRecorder.h"

#include <string>
#include <vector>
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>
#include <queue>
#include <condition_variable>
#include <atomic>
#include <functional>
#include <optional>

namespace epi_sculpt {

// -----------------------------------------------------------------------------
// Educational Annotation: UNIFIED LIVE CAPTURE CONTROLLER
// -----------------------------------------------------------------------------
// Encodes captures on one worker. Admission is bounded by retained pixel storage
// and outstanding task count, including the active writer/callback. Encoding and
// IO stay off the caller thread; allocation/copy costs are not a zero-stall promise.
// -----------------------------------------------------------------------------

struct LiveCaptureRequest {
    SculptureWorldMode worldMode{};
    std::vector<std::uint8_t> bgraPixels{};
    std::uint32_t width = 0U;
    std::uint32_t height = 0U;
    std::string directory = "snapshots";
    std::string customTag{};
    std::uint64_t sequenceNumber = 0U;
    // Present only when a bounded world profile has completed. This is copied
    // into the async task so later profiling cannot alter capture provenance.
    std::optional<WorldProfileSummary> worldProfileSummary{};
};

struct LiveCaptureReport {
    SnapshotPaths paths{};
    bool success = false;
    float writeTimeMs = 0.0f;
    std::string error{};
};

struct LiveCaptureLimits {
    std::uint64_t maxOutstandingCaptures = 4;
    std::uint64_t maxPixelBytes = 512ULL * 1024ULL * 1024ULL;
};

class SculptureLiveCaptureController {
public:
    using CompletionCallback = std::function<void(const LiveCaptureReport&)>;

    explicit SculptureLiveCaptureController(std::string defaultSnapshotDir = "snapshots",
        LiveCaptureLimits limits = {});
    ~SculptureLiveCaptureController();

    SculptureLiveCaptureController(const SculptureLiveCaptureController&) = delete;
    SculptureLiveCaptureController& operator=(const SculptureLiveCaptureController&) = delete;

    // Sets default target directory on disk (auto-created if missing)
    void setSnapshotDirectory(std::string dir) noexcept { snapshotDir_ = std::move(dir); }
    [[nodiscard]] const std::string& snapshotDirectory() const noexcept { return snapshotDir_; }

    // Returns false on invalid pixel layout or count/retained-pixel budget exhaustion.
    // Rejected requests get no callback. Accepted callbacks run on the worker;
    // they must not destroy this controller or wait for another capture callback.
    bool enqueueCapture(
        LiveCaptureRequest request,
        CompletionCallback onComplete = nullptr
    );

    // Synchronous immediate capture; invalid layout returns failure before IO.
    // State-only requests use width=height=0 and an empty pixel vector.
    [[nodiscard]] LiveCaptureReport writeImmediate(const LiveCaptureRequest& request);

    // Helpers for Spacebar live performance capture
    [[nodiscard]] static LiveCaptureRequest makeSpacebarRequest(
        const SculptureWorldState& worldState,
        std::string presetName,
        std::string presetRevision,
        SculptureProgramParams params,
        std::vector<std::string> routingMap,
        std::uint32_t width,
        std::uint32_t height,
        std::vector<std::uint8_t> bgraPixels,
        std::string directory = "snapshots",
        std::string customTag = ""
    );

    // Statistics & Diagnostics
    [[nodiscard]] std::uint64_t totalCapturesEnqueued() const noexcept { return totalEnqueued_.load(std::memory_order_relaxed); }
    [[nodiscard]] std::uint64_t totalCapturesCompleted() const noexcept { return totalCompleted_.load(std::memory_order_relaxed); }
    [[nodiscard]] std::uint64_t pendingCaptureCount() const noexcept;
    // Outstanding includes active encoding/callback; pendingCaptureCount keeps
    // its existing queued-only meaning. Pixel budget excludes state/encoder scratch.
    [[nodiscard]] std::uint64_t outstandingCaptureCount() const noexcept;
    [[nodiscard]] std::uint64_t outstandingPixelBytes() const noexcept;
    [[nodiscard]] std::uint64_t rejectedCaptureCount() const noexcept { return rejected_.load(); }
    [[nodiscard]] std::optional<SnapshotPaths> lastCompletedSnapshot() const noexcept;

private:
    struct InternalTask {
        LiveCaptureRequest request;
        CompletionCallback callback;
    };

    void workerThreadLoop();

    std::string snapshotDir_ = "snapshots";
    const LiveCaptureLimits limits_;
    std::uint64_t outstanding_ = 0, pixelBytes_ = 0; // guarded by queueMutex_
    std::atomic<std::uint64_t> rejected_{0};
    std::atomic<std::uint64_t> totalEnqueued_{0U};
    std::atomic<std::uint64_t> totalCompleted_{0U};
    std::atomic<bool> stopWorker_{false};

    mutable std::mutex queueMutex_{};
    std::condition_variable queueCondition_{};
    std::queue<InternalTask> taskQueue_{};

    mutable std::mutex lastPathsMutex_{};
    std::optional<SnapshotPaths> lastPaths_{};

    std::thread workerThread_{};
};

} // namespace epi_sculpt
