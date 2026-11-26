#include "SculptureLiveCaptureController.h"
#include "CapturePixelLayout.h"
#include <chrono>
#include <filesystem>
#include <utility>

namespace epi_sculpt {

SculptureLiveCaptureController::SculptureLiveCaptureController(std::string defaultSnapshotDir, LiveCaptureLimits limits)
    : snapshotDir_(std::move(defaultSnapshotDir)), limits_(limits) {
    try {
        std::filesystem::create_directories(snapshotDir_);
    } catch (...) {}
    
    workerThread_ = std::thread(&SculptureLiveCaptureController::workerThreadLoop, this);
}

SculptureLiveCaptureController::~SculptureLiveCaptureController() {
    stopWorker_.store(true, std::memory_order_release);
    queueCondition_.notify_all();
    if (workerThread_.joinable()) {
        workerThread_.join();
    }
}

bool SculptureLiveCaptureController::enqueueCapture(
    LiveCaptureRequest request,
    CompletionCallback onComplete
) {
    if (!capturePixelLayoutError(request.width, request.height, request.bgraPixels.size()).empty()) {
        rejected_.fetch_add(1, std::memory_order_relaxed);
        return false;
    }
    if (request.directory.empty()) {
        request.directory = snapshotDir_;
    }

    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        const auto bytes = request.bgraPixels.capacity();
        if (stopWorker_.load(std::memory_order_acquire)
            || outstanding_ >= limits_.maxOutstandingCaptures
            || bytes > limits_.maxPixelBytes - pixelBytes_) {
            rejected_.fetch_add(1, std::memory_order_relaxed);
            return false;
        }
        taskQueue_.push(InternalTask{
            .request = std::move(request),
            .callback = std::move(onComplete)
        });
        ++outstanding_;
        pixelBytes_ += bytes;
        totalEnqueued_.fetch_add(1U, std::memory_order_relaxed);
    }
    queueCondition_.notify_one();
    return true;
}

LiveCaptureReport SculptureLiveCaptureController::writeImmediate(const LiveCaptureRequest& request) {
    const auto tStart = std::chrono::high_resolution_clock::now();
    LiveCaptureReport report;
    const auto layoutError = capturePixelLayoutError(
        request.width, request.height, request.bgraPixels.size());
    if (!layoutError.empty()) {
        report.error = layoutError;
        totalCompleted_.fetch_add(1U, std::memory_order_relaxed);
        return report; // No path generation, filesystem IO or last-path mutation.
    }

    const std::string dir = request.directory.empty() ? snapshotDir_ : request.directory;
    report.paths = SculptureStateRecorder::generatePaths(dir, request.worldMode.presetName, request.customTag);
    const WorldProfileSummary* profileSummary = request.worldProfileSummary
        ? &*request.worldProfileSummary : nullptr;

    const bool ok = SculptureStateRecorder::writeSnapshot(
        report.paths.pngPath,
        report.paths.worldPath,
        report.paths.epimodePath,
        report.paths.epiPath,
        request.width,
        request.height,
        request.bgraPixels.empty() ? nullptr : request.bgraPixels.data(),
        request.worldMode,
        profileSummary
    );

    if (profileSummary == nullptr) {
        report.paths.profilePath.clear();
    }

    const auto tEnd = std::chrono::high_resolution_clock::now();
    report.writeTimeMs = std::chrono::duration<float, std::milli>(tEnd - tStart).count();
    report.success = ok;
    if (!ok) {
        report.error = "Failed to write visual PNG, state sidecar, or requested profile provenance";
    }

    {
        std::lock_guard<std::mutex> lock(lastPathsMutex_);
        lastPaths_ = report.paths;
    }
    totalCompleted_.fetch_add(1U, std::memory_order_relaxed);

    return report;
}

LiveCaptureRequest SculptureLiveCaptureController::makeSpacebarRequest(
    const SculptureWorldState& worldState,
    std::string presetName,
    std::string presetRevision,
    SculptureProgramParams params,
    std::vector<std::string> routingMap,
    std::uint32_t width,
    std::uint32_t height,
    std::vector<std::uint8_t> bgraPixels,
    std::string directory,
    std::string customTag
) {
    LiveCaptureRequest req;
    req.worldMode = SculptureStateRecorder::makeCaptureWorldMode(
        worldState,
        std::move(presetName),
        std::move(presetRevision),
        std::move(params),
        std::move(routingMap)
    );
    req.width = width;
    req.height = height;
    req.bgraPixels = std::move(bgraPixels);
    req.directory = std::move(directory);
    req.customTag = std::move(customTag);
    return req;
}

std::uint64_t SculptureLiveCaptureController::pendingCaptureCount() const noexcept {
    std::lock_guard<std::mutex> lock(queueMutex_);
    return taskQueue_.size();
}

std::uint64_t SculptureLiveCaptureController::outstandingCaptureCount() const noexcept {
    std::lock_guard<std::mutex> lock(queueMutex_);
    return outstanding_;
}

std::uint64_t SculptureLiveCaptureController::outstandingPixelBytes() const noexcept {
    std::lock_guard<std::mutex> lock(queueMutex_);
    return pixelBytes_;
}

std::optional<SnapshotPaths> SculptureLiveCaptureController::lastCompletedSnapshot() const noexcept {
    std::lock_guard<std::mutex> lock(lastPathsMutex_);
    return lastPaths_;
}

void SculptureLiveCaptureController::workerThreadLoop() {
    while (true) {
        InternalTask currentTask;
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            queueCondition_.wait(lock, [this] {
                return stopWorker_.load(std::memory_order_acquire) || !taskQueue_.empty();
            });

            if (stopWorker_.load(std::memory_order_acquire) && taskQueue_.empty()) {
                break;
            }

            if (taskQueue_.empty()) {
                continue;
            }

            currentTask = std::move(taskQueue_.front());
            taskQueue_.pop();
        }

        const auto bytes = currentTask.request.bgraPixels.capacity();
        LiveCaptureReport report;
        try { report = writeImmediate(currentTask.request); }
        catch (...) {
            // Report failure with the default success=false even if constructing
            // an error message itself fails under memory pressure.
            try { report.error = "Capture writer threw"; } catch (...) {}
            totalCompleted_.fetch_add(1U, std::memory_order_relaxed);
        }
        try {
            if (currentTask.callback) currentTask.callback(report);
        } catch (...) {
            std::fputs("[SculptureLiveCaptureController] Capture callback threw\n", stderr);
        }
        // Release the owned pixel allocation before making its budget available.
        std::vector<std::uint8_t>().swap(currentTask.request.bgraPixels);
        {
            std::lock_guard<std::mutex> lock(queueMutex_);
            --outstanding_;
            pixelBytes_ -= bytes;
        }
    }
}

} // namespace epi_sculpt
