#include "SculptureSequenceRecorder.h"
#include "CapturePixelLayout.h"
#include "SculptureSequenceDirectory.h"
#include "SculptureFilePublication.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <exception>
#include <limits>
#include <cmath>
#include <locale>
#include <new>
#include <type_traits>

#include <epi/third_party/stb_image_write.h>

namespace epi_sculpt {

namespace {

void writeJsonString(std::ostream& out, std::string_view value) {
    static constexpr char hex[] = "0123456789abcdef";
    out << '"';
    for (unsigned char c : value) {
        if (c == '"' || c == '\\') out << '\\' << static_cast<char>(c);
        else if (c < 0x20) out << "\\u00" << hex[c >> 4] << hex[c & 15];
        else out << static_cast<char>(c);
    }
    out << '"';
}

// Unknown non-finite observations are JSON null, never invalid numeric tokens.
void writeJsonNumber(std::ostream& out, double value) {
    if (std::isfinite(value)) out << value;
    else out << "null";
}

// An unknown value must not be promoted to a claim about capture timing.
const char* metadataOriginName(SequenceMetadataOrigin origin) noexcept {
    switch (origin) {
    case SequenceMetadataOrigin::DirectCall: return "direct_call";
    case SequenceMetadataOrigin::CaptureRequest: return "capture_request";
    case SequenceMetadataOrigin::Unknown: return "unknown";
    }
    return "unknown";
}

// A sequence prefix is configurable; keep it a literal shell argument when
// the optional export script is run, including quotes and metacharacters.
void writeShellArgument(std::ostream& out, std::string_view value) {
    out << '\'';
    for (char c : value) {
        if (c == '\'') out << "'\\''";
        else out << c;
    }
    out << '\'';
}

// Failed captures hold the previous retained image through the next retained
// index, including the attempted tail. Leading failures have no image to hold.
// Non-increasing indices still fall back to one nominal interval.
template <typename Frames>
std::uint64_t nominalIntervals(const Frames& frames, std::size_t i,
                               std::uint64_t attempts) {
    const auto index = frames[i].metadata.frameIndex;
    if (i + 1 < frames.size() && frames[i + 1].metadata.frameIndex > index)
        return std::uint64_t{frames[i + 1].metadata.frameIndex} - index;
    if (i + 1 == frames.size() && attempts > index)
        return attempts - index;
    return 1;
}

template <typename Frames>
double nominalDuration(const Frames& frames, float fps, std::uint64_t attempts) {
    double seconds = 0.0;
    for (std::size_t i = 0; i < frames.size(); ++i)
        seconds += static_cast<double>(nominalIntervals(frames, i, attempts)) / fps;
    return seconds;
}

[[nodiscard]] std::string currentIsoTimestamp() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tmBuf{};
#if defined(_WIN32)
    gmtime_s(&tmBuf, &t);
#else
    gmtime_r(&t, &tmBuf);
#endif
    std::ostringstream ss;
    ss << std::put_time(&tmBuf, "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

[[nodiscard]] std::string makeTimestampDirectoryName(std::string_view prefix, std::string_view customTag) {
    const auto now = std::chrono::system_clock::now();
    const std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tmBuf{};
#if defined(_WIN32)
    localtime_s(&tmBuf, &t);
#else
    localtime_r(&t, &tmBuf);
#endif
    std::ostringstream ss;
    ss << (prefix.empty() ? "seq" : prefix) << "_"
       << std::put_time(&tmBuf, "%Y%m%d_%H%M%S");
    if (!customTag.empty()) {
        ss << "_";
        for (char c : customTag) {
            if (std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-') {
                ss << c;
            } else {
                ss << '_';
            }
        }
    }
    return ss.str();
}

} // namespace

SculptureSequenceRecorder::SculptureSequenceRecorder(SequenceRecorderConfig config)
    : config_(std::move(config)) {
    if (!std::isfinite(config_.targetFps) || config_.targetFps < 1.0f)
        config_.targetFps = 60.0f;
    sessionTargetFps_ = config_.targetFps;
    workerThread_ = std::thread(&SculptureSequenceRecorder::workerThreadLoop, this);
}

SculptureSequenceRecorder::~SculptureSequenceRecorder() {
    if (isRecording()) {
        stopRecording();
    }
    stopWorker_.store(true, std::memory_order_release);
    queueCondition_.notify_all();
    if (workerThread_.joinable()) {
        workerThread_.join();
    }
}

void SculptureSequenceRecorder::setTargetFps(float fps) noexcept {
    if (std::isfinite(fps)) config_.targetFps = std::max(fps, 1.0f);
}

bool SculptureSequenceRecorder::startRecording(std::string_view customSequenceName) {
    auto expected = Lifecycle::Idle;
    if (!lifecycle_.compare_exchange_strong(expected, Lifecycle::Starting,
                                            std::memory_order_acq_rel)) return false;
    // A failed start must not leave the recorder permanently unavailable.
    struct StartGuard {
        std::atomic<Lifecycle>& state;
        bool published = false;
        ~StartGuard() { if (!published) state.store(Lifecycle::Idle, std::memory_order_release); }
    } guard{lifecycle_};
    const auto previousSession = captureSession_.load(std::memory_order_relaxed);
    if (previousSession == std::numeric_limits<std::uint64_t>::max()) {
        return false; // Never reuse an identity that a delayed callback may hold.
    }

    std::error_code ec;
    const auto directory = detail::createSequenceDirectory(
        config_.baseDirectory, makeTimestampDirectoryName(config_.sequencePrefix, customSequenceName), ec);
    if (ec) {
        std::fprintf(stderr, "[SculptureSequenceRecorder] Failed to create sequence directory in %s: %s\n",
            config_.baseDirectory.c_str(), ec.message().c_str());
        return false;
    }
    currentSequenceName_ = directory.filename().string();
    currentDir_ = directory.string();
    startTimestampIso_ = currentIsoTimestamp();
    sessionTargetFps_ = config_.targetFps;

    frameIndexCounter_.store(0U, std::memory_order_relaxed);
    framesCompleted_.store(0U, std::memory_order_relaxed);
    framesDropped_.store(0U, std::memory_order_relaxed);
    metadataBudgetDropped_.store(0U, std::memory_order_relaxed);
    bufferBudgetRejected_.store(0U, std::memory_order_relaxed);
    frameWriteFailed_.store(0U, std::memory_order_relaxed);
    indexExhaustionRejected_.store(0U, std::memory_order_relaxed);
    metadataRetentionFailed_.store(0U, std::memory_order_relaxed);
    frameAllocationFailed_.store(0U, std::memory_order_relaxed);

    {
        std::lock_guard<std::mutex> lock(manifestMutex_);
        recordedFrameManifest_.clear();
    }
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        sessionMetadataBytes_ = 0U;
    }

    std::fprintf(stderr, "[SculptureSequenceRecorder] Started recording sequence: %s\n",
        currentDir_.c_str());
    captureSession_.store(previousSession + 1U, std::memory_order_relaxed);
    lifecycle_.store(Lifecycle::Recording, std::memory_order_release);
    guard.published = true;
    return true;
}

bool SculptureSequenceRecorder::stopRecording(SequenceStopOutcome* outcome) {
    if (outcome) *outcome = {};
    auto expected = Lifecycle::Recording;
    if (!lifecycle_.compare_exchange_strong(expected, Lifecycle::Stopping,
                                            std::memory_order_acq_rel)) return false;
    // Keep the session unavailable until the worker and manifest are settled.
    // An unexpected exception before settlement leaves it closed, rather than
    // allowing a new session to reuse data still owned by the previous one.
    struct StopGuard {
        std::atomic<Lifecycle>& state;
        bool settled = false;
        ~StopGuard() { if (settled) state.store(Lifecycle::Idle, std::memory_order_release); }
    } guard{lifecycle_};

    flush();

    SequenceStopOutcome result{};
    result.recordedFrameCount = framesCompleted_.load(std::memory_order_relaxed);
    bool published = true;
    if (config_.writeManifest) {
        try { published = writeManifestOnDisk(result.scriptPublished); }
        catch (const std::exception& error) {
            std::fprintf(stderr, "[SculptureSequenceRecorder] Manifest error: %s\n", error.what());
            published = false;
        } catch (...) {
            std::fprintf(stderr, "[SculptureSequenceRecorder] Manifest error: unknown exception\n");
            published = false;
        }
    }

    result.manifestPublished = config_.writeManifest && published;
    if (outcome) *outcome = result;

    std::fprintf(stderr,
        "[SculptureSequenceRecorder] Stopped sequence recording: %s (Total: %u frames, Dropped: %u)\n",
        currentDir_.c_str(),
        framesCompleted_.load(std::memory_order_relaxed),
        framesDropped_.load(std::memory_order_relaxed));
    guard.settled = true;
    return published;
}

bool SculptureSequenceRecorder::toggleRecording(std::string_view customSequenceName) {
    if (isRecording()) {
        return stopRecording();
    }
    return startRecording(customSequenceName);
}

bool SculptureSequenceRecorder::recordFrame(
    std::uint32_t width,
    std::uint32_t height,
    const std::uint8_t* pixels,
    const SequenceFrameMetadata& metadata,
    bool isBgra
) {
    return recordFrameForSession(captureSession(), width, height, pixels,
                                 metadata, isBgra);
}

bool SculptureSequenceRecorder::recordFrameForSession(
    std::uint64_t expectedSession,
    std::uint32_t width,
    std::uint32_t height,
    const std::uint8_t* pixels,
    const SequenceFrameMetadata& metadata,
    bool isBgra
) {
    if (expectedSession == 0U) return false;
    // A borrowed pointer carries no length. Validate the dimensions and PNG
    // arithmetic before indexing the frame, allocating its copy, or queuing it.
    if (width == 0U || height == 0U || pixels == nullptr
        || !capturePixelLayoutError(width, height, std::numeric_limits<std::size_t>::max()).empty()) {
        return false;
    }

    // Dimensions were admitted above, so this multiplication fits size_t.
    const std::size_t byteCount = static_cast<std::size_t>(width) * height * 4U;
    // The byte ledger includes the worker's frame until its buffer is freed.
    {
        std::unique_lock<std::mutex> lock(queueMutex_);
        // A frame prepared before a stop cannot enter a later session. Indexing
        // is inside this gate so a rejected stale frame cannot alter its timeline.
        if (lifecycle_.load(std::memory_order_acquire) != Lifecycle::Recording
            || captureSession_.load(std::memory_order_relaxed) != expectedSession) return false;
        const auto nextIndex = frameIndexCounter_.load(std::memory_order_relaxed);
        if (nextIndex == std::numeric_limits<std::uint32_t>::max()) {
            framesDropped_.fetch_add(1U, std::memory_order_relaxed);
            indexExhaustionRejected_.fetch_add(1U, std::memory_order_relaxed);
            return false; // Never wrap and overwrite an earlier frame filename.
        }
        frameIndexCounter_.store(nextIndex + 1U, std::memory_order_relaxed);
        // Reject impossible metadata sizes before making a private copy.
        // Subtractions prevent overflow even for adversarial string lengths.
        const auto limit = config_.maxSessionMetadataBytes;
        std::size_t remaining = limit;
        const auto charge = [&](std::size_t bytes) {
            if (bytes > remaining) return false;
            remaining -= bytes;
            return true;
        };
        if (!charge(sizeof(ManifestFrame)) || !charge(metadata.presetName.size())
            || !charge(metadata.customTag.size()) || !charge(sessionMetadataBytes_)) {
            metadataBudgetDropped_.fetch_add(1U, std::memory_order_relaxed);
            framesDropped_.fetch_add(1U, std::memory_order_relaxed);
            return false;
        }
        std::size_t metadataCharge = 0U;
        if (taskQueue_.size() >= config_.maxBufferedFrames
            || byteCount > config_.maxBufferedPixelBytes
            || bufferedPixelBytes_ > config_.maxBufferedPixelBytes - byteCount) {
            framesDropped_.fetch_add(1U, std::memory_order_relaxed);
            bufferBudgetRejected_.fetch_add(1U, std::memory_order_relaxed);
            return false;
        }

        // Copy admitted metadata and pixels before charging the ledger.
        // Allocation failure leaves the queue and accounting unchanged; the
        // attempt retains its index so export timing represents the gap.
        static_assert(std::is_nothrow_move_constructible_v<FrameTask>);
        try {
            FrameTask task;
            task.width = width;
            task.height = height;
            task.metadata = metadata;
            // A string copy may have a different capacity from its source.
            // Recheck and charge the actual owned record before publishing it.
            remaining = limit;
            if (!charge(sizeof(ManifestFrame)) || !charge(task.metadata.presetName.capacity())
                || !charge(task.metadata.customTag.capacity()) || !charge(sessionMetadataBytes_)) {
                metadataBudgetDropped_.fetch_add(1U, std::memory_order_relaxed);
                framesDropped_.fetch_add(1U, std::memory_order_relaxed);
                return false;
            }
            metadataCharge = limit - remaining - sessionMetadataBytes_;
            task.metadata.frameIndex = nextIndex;
            if (!task.metadata.timestampExplicit && task.metadata.timestampSeconds <= 0.0f) {
                task.metadata.timestampSeconds = static_cast<float>(nextIndex) / sessionTargetFps_;
            }
            task.metadataCharge = metadataCharge;
            task.isBgra = isBgra;
            task.pixelData.resize(byteCount);
            std::memcpy(task.pixelData.data(), pixels, byteCount);
            taskQueue_.push(std::move(task));
        } catch (const std::bad_alloc&) {
            framesDropped_.fetch_add(1U, std::memory_order_relaxed);
            frameAllocationFailed_.fetch_add(1U, std::memory_order_relaxed);
            return false;
        }
        bufferedPixelBytes_ += byteCount;
        sessionMetadataBytes_ += metadataCharge;
        activeWorkCount_.fetch_add(1U, std::memory_order_relaxed);
    }

    queueCondition_.notify_one();
    return true;
}

void SculptureSequenceRecorder::flush() {
    std::unique_lock<std::mutex> lock(queueMutex_);
    flushCondition_.wait(lock, [this]() {
        return activeWorkCount_.load(std::memory_order_acquire) == 0U;
    });
}

bool SculptureSequenceRecorder::isRecording() const noexcept {
    return lifecycle_.load(std::memory_order_acquire) == Lifecycle::Recording;
}

std::uint64_t SculptureSequenceRecorder::captureSession() const noexcept {
    if (!isRecording()) return 0U;
    return captureSession_.load(std::memory_order_relaxed);
}

const std::string& SculptureSequenceRecorder::currentSequenceDirectory() const noexcept {
    return currentDir_;
}

SequenceRecorderStats SculptureSequenceRecorder::stats() const {
    SequenceRecorderStats s;
    s.isRecording = isRecording();
    s.activeDirectory = currentDir_;
    s.recordedFrameCount = framesCompleted_.load(std::memory_order_relaxed);
    s.droppedFrameCount = framesDropped_.load(std::memory_order_relaxed);
    s.metadataBudgetDroppedFrameCount = metadataBudgetDropped_.load(std::memory_order_relaxed);
    s.targetFps = sessionTargetFps_;
    s.elapsedSeconds = static_cast<float>(s.recordedFrameCount) / sessionTargetFps_;

    std::lock_guard<std::mutex> lock(queueMutex_);
    s.queuedFrameCount = static_cast<std::uint32_t>(taskQueue_.size());
    s.bufferedPixelBytes = bufferedPixelBytes_;
    s.sessionMetadataBytes = sessionMetadataBytes_;
    return s;
}

std::string SculptureSequenceRecorder::formatHudIndicator() const {
    if (!isRecording()) {
        return {};
    }
    const std::uint32_t count = framesCompleted_.load(std::memory_order_relaxed);
    char buf[128];
    std::snprintf(buf, sizeof(buf), "[REC ● frame: %06u | %.1f fps]", count, sessionTargetFps_);
    return std::string(buf);
}

void SculptureSequenceRecorder::workerThreadLoop() {
    while (true) {
        FrameTask task;
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            queueCondition_.wait(lock, [this]() {
                return stopWorker_.load(std::memory_order_relaxed) || !taskQueue_.empty();
            });

            if (stopWorker_.load(std::memory_order_relaxed) && taskQueue_.empty()) {
                break;
            }

            if (taskQueue_.empty()) {
                continue;
            }

            task = std::move(taskQueue_.front());
            taskQueue_.pop();
        }

        // Color swizzling: BGRA -> RGBA if needed
        if (task.isBgra) {
            std::uint8_t* p = task.pixelData.data();
            const std::size_t total = static_cast<std::size_t>(task.width) * task.height * 4U;
            for (std::size_t i = 0; i < total; i += 4U) {
                std::swap(p[i], p[i + 2]);
            }
        }

        // Only the bounded basename uses a fixed buffer; the directory can
        // exceed 512 bytes without silently changing the output path.
        char basename[32];
        std::snprintf(basename, sizeof(basename), "frame_%06u.png", task.metadata.frameIndex);
        int result = 0;
        try {
            const std::string filename = (std::filesystem::path(currentDir_) / basename).string();
            result = stbi_write_png(filename.c_str(), static_cast<int>(task.width),
                                    static_cast<int>(task.height), 4, task.pixelData.data(),
                                    static_cast<int>(task.width * 4U));
            if (!result)
                std::fprintf(stderr, "[SculptureSequenceRecorder] Failed writing frame: %s\n", filename.c_str());
        } catch (const std::exception& error) {
            std::fprintf(stderr, "[SculptureSequenceRecorder] Failed preparing frame path: %s\n", error.what());
        } catch (...) {
            // A sink exception cannot bypass the common frame settlement below.
            std::fprintf(stderr, "[SculptureSequenceRecorder] Frame write failed: unknown exception\n");
        }

        bool retainedMetadata = false;
        if (result != 0) {
            framesCompleted_.fetch_add(1U, std::memory_order_relaxed);
            try {
                std::lock_guard<std::mutex> lock(manifestMutex_);
                recordedFrameManifest_.push_back({std::move(task.metadata), task.width, task.height});
                retainedMetadata = true;
            } catch (const std::exception& error) {
                // The PNG exists even if its optional metadata could not be retained.
                std::fprintf(stderr, "[SculptureSequenceRecorder] Frame metadata not retained: %s\n", error.what());
            } catch (...) {
                std::fprintf(stderr, "[SculptureSequenceRecorder] Frame metadata not retained: unknown exception\n");
            }
            if (!retainedMetadata)
                metadataRetentionFailed_.fetch_add(1U, std::memory_order_relaxed);
        } else {
            framesDropped_.fetch_add(1U, std::memory_order_relaxed);
            frameWriteFailed_.fetch_add(1U, std::memory_order_relaxed);
        }

        // Release the buffer before making its budget available to producers.
        const auto completedBytes = task.pixelData.size();
        std::vector<std::uint8_t>().swap(task.pixelData);
        {
            std::lock_guard<std::mutex> lock(queueMutex_);
            bufferedPixelBytes_ -= completedBytes;
            if (!retainedMetadata) sessionMetadataBytes_ -= task.metadataCharge;
            const auto remaining = activeWorkCount_.fetch_sub(1U, std::memory_order_acq_rel);
            if (remaining <= 1U) flushCondition_.notify_all();
        }
    }
}

bool SculptureSequenceRecorder::writeManifestOnDisk(bool& scriptPublished) {
    const std::string manifestPath = currentDir_ + "/sequence_manifest.json";
    // stopRecording has closed admission and flush() has settled the worker.
    // Freeze one attempted-frame boundary for both manifest and export script.
    const auto attemptedFrames = frameIndexCounter_.load(std::memory_order_relaxed);
    std::ostringstream out;
    out.imbue(std::locale::classic());

    // flush() has settled all worker tasks. Serialize the owned entries under
    // their lock instead of duplicating the entire per-frame metadata ledger.
    {
        std::lock_guard<std::mutex> lock(manifestMutex_);
        const auto& frames = recordedFrameManifest_;

        const std::uint32_t totalFrames = static_cast<std::uint32_t>(frames.size());
        const double durationSec = nominalDuration(frames, sessionTargetFps_, attemptedFrames);

        // Diagnostics describe only recorder-observed outcomes: invalid/stale
        // requests and capture attempts that never arrive here are not counted.
        // A PNG can be written even when its metadata cannot be retained, so
        // written_frame_count may differ from the manifest's frame_count.
        out << "{\n  \"sequence_name\": ";
        writeJsonString(out, currentSequenceName_);
        out << ",\n  \"directory\": ";
        writeJsonString(out, currentDir_);
        out << ",\n"
            << "  \"frame_count\": " << totalFrames << ",\n"
            << "  \"attempted_frame_count\": " << attemptedFrames << ",\n"
            << "  \"recording_diagnostics\": {\n"
            << "    \"written_frame_count\": " << framesCompleted_.load(std::memory_order_relaxed) << ",\n"
            << "    \"dropped_frame_count\": " << framesDropped_.load(std::memory_order_relaxed) << ",\n"
            << "    \"metadata_budget_rejected_count\": " << metadataBudgetDropped_.load(std::memory_order_relaxed) << ",\n"
            << "    \"buffer_budget_rejected_count\": " << bufferBudgetRejected_.load(std::memory_order_relaxed) << ",\n"
            << "    \"frame_write_failure_count\": " << frameWriteFailed_.load(std::memory_order_relaxed) << ",\n"
            << "    \"index_exhaustion_rejected_count\": " << indexExhaustionRejected_.load(std::memory_order_relaxed) << ",\n"
            << "    \"frame_allocation_failure_count\": " << frameAllocationFailed_.load(std::memory_order_relaxed) << ",\n"
            << "    \"metadata_retention_failure_count\": " << metadataRetentionFailed_.load(std::memory_order_relaxed) << "\n"
            << "  },\n"
            << "  \"target_fps\": " << std::setprecision(17) << sessionTargetFps_ << ",\n"
            << "  \"duration_seconds\": " << std::setprecision(17) << durationSec << std::setprecision(6) << ",\n"
            << "  \"start_time_iso\": ";
        writeJsonString(out, startTimestampIso_);
        out << ",\n  \"end_time_iso\": ";
        writeJsonString(out, currentIsoTimestamp());
        out << ",\n  \"frames\": [\n";

        for (std::size_t i = 0; i < frames.size(); ++i) {
            const auto& frame = frames[i];
            const auto& f = frame.metadata;
            char frameFilename[64];
            std::snprintf(frameFilename, sizeof(frameFilename), "frame_%06u.png", f.frameIndex);

            out << "    {\n"
                << "      \"index\": " << f.frameIndex << ",\n"
                << "      \"filename\": \"" << frameFilename << "\",\n"
                << "      \"width\": " << frame.width << ",\n"
                << "      \"height\": " << frame.height << ",\n"
                << "      \"timestamp_sec\": ";
            writeJsonNumber(out, f.timestampSeconds);
            out << ",\n      \"preset\": ";
            writeJsonString(out, f.presetName);
            out << ",\n      \"metadata_origin\": \"" << metadataOriginName(f.origin) << "\"";
            out << ",\n      \"custom_tag\": ";
            writeJsonString(out, f.customTag);
            out << ",\n      \"morph\": ";
            writeJsonNumber(out, f.morph);
            out << ",\n      \"camera_eye\": [";
            for (int j = 0; j < 3; ++j) { if (j) out << ", "; writeJsonNumber(out, f.cameraEye[j]); }
            out << "],\n      \"camera_target\": [";
            for (int j = 0; j < 3; ++j) { if (j) out << ", "; writeJsonNumber(out, f.cameraTarget[j]); }
            out << "]\n    }" << (i + 1 < frames.size() ? "," : "") << "\n";
        }

        out << "  ]\n}\n";
    }
    // view() borrows the encoded buffer for this synchronous transaction and
    // avoids allocating a second full manifest payload.
    if (!out || !file_publication_detail::publishFile(manifestPath, [&](int fd) {
            const auto payload = out.view();
            return file_publication_detail::writeAll(fd, payload.data(), payload.size());
        })) {
        std::fprintf(stderr, "[SculptureSequenceRecorder] Failed writing manifest to %s\n", manifestPath.c_str());
        return false;
    }
    std::fprintf(stderr, "[SculptureSequenceRecorder] Wrote sequence manifest: %s\n", manifestPath.c_str());

    // The script is optional: its failure must not undo a published manifest.
    try {
        const std::string scriptPath = currentDir_ + "/export_video.sh";
        std::ostringstream scriptOut;
        scriptOut.imbue(std::locale::classic());
        scriptOut << "#!/usr/bin/env bash\n"
                  << "# Export recorded frames; gaps hold the preceding frame at nominal FPS.\n"
                  << "cd -- \"$(dirname -- \"$0\")\" || exit 1\n";
        {
            std::lock_guard<std::mutex> lock(manifestMutex_);
            const auto& frames = recordedFrameManifest_;
            if (frames.empty()) {
                scriptOut << "printf '%s\\n' 'No recorded frames to export.' >&2\nexit 1\n";
            } else {
                // The generated basenames contain only ASCII letters, digits and
                // punctuation. Check all inputs before invoking ffmpeg.
                for (const auto& frame : frames) {
                    char name[32];
                    std::snprintf(name, sizeof(name), "frame_%06u.png", frame.metadata.frameIndex);
                    scriptOut << "[ -f '" << name << "' ] || { printf '%s\\n' 'Missing "
                              << name << "' >&2; exit 1; }\n";
                }
                scriptOut << "list=$(mktemp './.epic-frames-XXXXXX') || exit 1\n"
                          << "trap 'rm -f -- \"$list\"' EXIT\n"
                          << "cat > \"$list\" <<'EPIC_FRAME_LIST' || exit 1\n"
                          << "ffconcat version 1.0\n" << std::setprecision(17);
                double duration = 0.0;
                double previousBoundary = 0.0;
                char lastName[32]{};
                for (std::size_t i = 0; i < frames.size(); ++i) {
                    const auto index = frames[i].metadata.frameIndex;
                    std::snprintf(lastName, sizeof(lastName), "frame_%06u.png", index);
                    // Failed or rejected captures leave index gaps. Hold the
                    // preceding image for those nominal frame intervals.
                    const double seconds = static_cast<double>(nominalIntervals(frames, i, attemptedFrames)) / sessionTargetFps_;
                    duration += seconds;
                    // Concat parses durations at microsecond precision. Round the
                    // cumulative boundary, not each interval, to avoid drift.
                    const double boundary = std::round(duration * 1000000.0) / 1000000.0;
                    const double quantizedInterval = boundary - previousBoundary;
                    previousBoundary = boundary;
                    // image2 otherwise timestamps still images at its default 25 Hz,
                    // quantizing the concat timeline even for 60/120 Hz exports.
                    scriptOut << "file '" << lastName << "'\noption framerate 1000000\nduration " << quantizedInterval << '\n';
                }
                // The concat demuxer needs a final sentinel to honor the last
                // image duration; -t prevents the sentinel extending output.
                // The list contains only recorder-generated basenames. FFmpeg
                // requires safe=0 to apply the per-file image demuxer option.
                // Quantized boundaries are nearest to the intended frame times;
                // map them back to the nearest output frame rather than ceiling.
                scriptOut << "file '" << lastName << "'\noption framerate 1000000\nEPIC_FRAME_LIST\n"
                          << "ffmpeg -y -f concat -safe 0 -i \"$list\" -vf fps=fps=" << sessionTargetFps_ << ":round=near -r " << sessionTargetFps_
                          << " -t " << duration << " -c:v libx264 -crf 18 -pix_fmt yuv420p ";
                writeShellArgument(scriptOut, "./" + currentSequenceName_ + ".mp4");
                scriptOut << '\n';
            }
        }
        const bool written = scriptOut && file_publication_detail::publishFile(scriptPath, [&](int fd) {
            const auto payload = scriptOut.view();
            return file_publication_detail::writeAll(fd, payload.data(), payload.size());
        });
        if (written) {
            std::error_code ec;
            std::filesystem::permissions(scriptPath,
                std::filesystem::perms::owner_read | std::filesystem::perms::owner_write | std::filesystem::perms::owner_exec,
                std::filesystem::perm_options::add, ec);
            scriptPublished = !ec;
        }
    } catch (const std::exception& error) {
        std::fprintf(stderr, "[SculptureSequenceRecorder] Optional script error: %s\n", error.what());
    } catch (...) {
        // The manifest is already published; script failure must not erase it.
        std::fprintf(stderr, "[SculptureSequenceRecorder] Optional script error: unknown exception\n");
    }
    return true;
}

} // namespace epi_sculpt
