#pragma once

#include <string>
#include <cstddef>
#include <vector>
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>
#include <queue>
#include <condition_variable>
#include <atomic>
#include <string_view>

namespace epi_sculpt {

// -----------------------------------------------------------------------------
// Educational Annotation: DETERMINISTIC FRAME SEQUENCE RECORDER
// -----------------------------------------------------------------------------
// Provides professional, frame-accurate image sequence export for live VJing,
// animation tours, and high-resolution rendering as mandated by docu/project-vision.txt.
//
// In contrast to single-frame snapshots, sequence recording:
// 1. Manages a stateful session in a timestamped folder: captures/seq_YYYYMMDD_HHMMSS/
// 2. Encodes sequentially numbered, zero-padded PNGs: frame_000000.png, frame_000001.png, ...
// 3. Asynchronously offloads color swizzling and PNG compression to a dedicated worker thread,
//    preventing render thread hitches during live 60/120 FPS performance.
// 4. Emits an archival JSON manifest (sequence_manifest.json) recording resolution,
//    target framerate, total duration, camera trajectories, and per-frame preset lineage.
// -----------------------------------------------------------------------------

// Request metadata may precede the actual readback when capture is deferred.
enum class SequenceMetadataOrigin : std::uint8_t {
    Unknown,
    DirectCall,
    CaptureRequest
};

struct SequenceFrameMetadata {
    std::uint32_t frameIndex = 0U;
    float timestampSeconds = 0.0f;
    std::string presetName{};
    float morph = 0.0f;
    float cameraEye[3] = {0.0f, 0.0f, 0.0f};
    float cameraTarget[3] = {0.0f, 0.0f, 0.0f};
    std::string customTag{};
    // Preserve an explicitly observed zero time instead of deriving a timestamp.
    bool timestampExplicit = false;
    SequenceMetadataOrigin origin = SequenceMetadataOrigin::Unknown;
};

struct SequenceRecorderConfig {
    std::string baseDirectory = "captures";
    std::string sequencePrefix = "seq";
    float targetFps = 60.0f;
    std::size_t maxBufferedFrames = 180; // Bounded memory queue (~3 seconds at 60 FPS)
    bool writeManifest = true;
    // Logical pixel payload across queued frames and the active encoder.
    // Zero disables frame admission; allocator and PNG scratch are separate.
    std::size_t maxBufferedPixelBytes = 256U * 1024U * 1024U;
    // Logical charge for queued, active and retained frame metadata. Zero
    // disables admission; vector slack and allocator overhead are separate.
    std::size_t maxSessionMetadataBytes = 32U * 1024U * 1024U;
};

struct SequenceRecorderStats {
    bool isRecording = false;
    std::string activeDirectory{};
    std::uint32_t recordedFrameCount = 0U;
    std::uint32_t queuedFrameCount = 0U;
    std::uint32_t droppedFrameCount = 0U;
    float targetFps = 60.0f;
    float elapsedSeconds = 0.0f;
    float averageWriteTimeMs = 0.0f;
    std::size_t bufferedPixelBytes = 0U;
    std::size_t sessionMetadataBytes = 0U;
    std::uint32_t metadataBudgetDroppedFrameCount = 0U;
};

// Facts from one completed stop; optional script failure does not fail the stop.
struct SequenceStopOutcome {
    std::uint32_t recordedFrameCount = 0;
    bool manifestPublished = false;
    bool scriptPublished = false;
};

class SculptureSequenceRecorder {
public:
    explicit SculptureSequenceRecorder(SequenceRecorderConfig config = {});
    ~SculptureSequenceRecorder();

    SculptureSequenceRecorder(const SculptureSequenceRecorder&) = delete;
    SculptureSequenceRecorder& operator=(const SculptureSequenceRecorder&) = delete;

    // Start a session only after any earlier stop has finished draining and
    // publishing. A concurrent start/stop transition returns false.
    bool startRecording(std::string_view customSequenceName = "");

    // Stop recording, wait for queued frames to flush to disk, and write manifest
    bool stopRecording(SequenceStopOutcome* outcome = nullptr);

    // Toggle recording mode
    bool toggleRecording(std::string_view customSequenceName = "");

    // Enqueue a frame for asynchronous encoding and disk persistence.
    // Metadata is borrowed only during this call and copied after admission;
    // the queued task owns the resulting snapshot. Pixels may be BGRA or RGBA.
    bool recordFrame(
        std::uint32_t width,
        std::uint32_t height,
        const std::uint8_t* pixels,
        const SequenceFrameMetadata& metadata = {},
        bool isBgra = true
    );
    // Delayed capture must carry its original session to queue admission.
    // Zero and stale identities are rejected before indexing or charging work.
    bool recordFrameForSession(
        std::uint64_t expectedSession,
        std::uint32_t width,
        std::uint32_t height,
        const std::uint8_t* pixels,
        const SequenceFrameMetadata& metadata = {},
        bool isBgra = true
    );

    // Synchronously flush all pending write tasks
    void flush();

    // State queries
    [[nodiscard]] bool isRecording() const noexcept;
    // Capture callbacks and borrowed directory/config access remain owner-thread
    // operations. Zero means no recording; each successful start gets a new ID.
    [[nodiscard]] std::uint64_t captureSession() const noexcept;
    // The owned directory snapshot and mutex acquisition may throw.
    [[nodiscard]] SequenceRecorderStats stats() const;
    [[nodiscard]] const std::string& currentSequenceDirectory() const noexcept;
    [[nodiscard]] std::string formatHudIndicator() const;

    // Owner-thread configuration for the next session. An active session keeps
    // the rate it started with so its timestamps and export timeline agree.
    void setTargetFps(float fps) noexcept;
    [[nodiscard]] float targetFps() const noexcept { return config_.targetFps; }

private:
    struct FrameTask {
        std::uint32_t width = 0U;
        std::uint32_t height = 0U;
        std::vector<std::uint8_t> pixelData{};
        SequenceFrameMetadata metadata{};
        std::size_t metadataCharge = 0U;
        bool isBgra = true;
    };
    struct ManifestFrame {
        SequenceFrameMetadata metadata;
        std::uint32_t width;
        std::uint32_t height;
    };

    void workerThreadLoop();
    bool writeManifestOnDisk(bool& scriptPublished);

    SequenceRecorderConfig config_;
    // Frozen from config_ at successful session start, retained after stop for
    // stats and manifest/export. Never altered by an in-session config edit.
    float sessionTargetFps_ = 60.0f;

    enum class Lifecycle : std::uint8_t { Idle, Starting, Recording, Stopping };
    std::atomic<Lifecycle> lifecycle_{Lifecycle::Idle};
    std::atomic<std::uint64_t> captureSession_{0U};
    std::atomic<bool> stopWorker_{false};
    std::atomic<std::uint32_t> frameIndexCounter_{0U};
    std::atomic<std::uint32_t> framesCompleted_{0U};
    std::atomic<std::uint32_t> framesDropped_{0U};
    std::atomic<std::uint32_t> metadataBudgetDropped_{0U};
    // Session diagnostics are serialized only after admission closes and the worker drains.
    std::atomic<std::uint64_t> bufferBudgetRejected_{0U};
    std::atomic<std::uint64_t> frameWriteFailed_{0U};
    std::atomic<std::uint64_t> indexExhaustionRejected_{0U};
    std::atomic<std::uint64_t> metadataRetentionFailed_{0U};
    std::atomic<std::uint64_t> frameAllocationFailed_{0U};
    std::atomic<std::uint32_t> activeWorkCount_{0U};

    std::string currentDir_{};
    std::string currentSequenceName_{};
    std::string startTimestampIso_{};

    mutable std::mutex queueMutex_{};
    std::condition_variable queueCondition_{};
    std::condition_variable flushCondition_{};
    std::queue<FrameTask> taskQueue_{};
    // Protected by queueMutex_; includes the frame held by the worker.
    std::size_t bufferedPixelBytes_ = 0U;
    // Protected by queueMutex_; retained entries remain charged until restart.
    std::size_t sessionMetadataBytes_ = 0U;

    mutable std::mutex manifestMutex_{};
    std::vector<ManifestFrame> recordedFrameManifest_{};

    std::thread workerThread_{};
};

} // namespace epi_sculpt
