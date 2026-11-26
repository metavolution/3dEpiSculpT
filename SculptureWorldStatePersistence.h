#pragma once

#include "SculptureWorldNavigator.h"

namespace epi_sculpt {

// -----------------------------------------------------------------------------
// SculptureWorldStatePersistence
// -----------------------------------------------------------------------------
// [EDUCATIONAL NOTE]
// Domain-Focused Snapshot, State Diff, Undo/Redo History & Sequence Recording
// Service for SculptureWorldNavigator:
// Encapsulates all persistent state capture (`.world`, `.epimode`, `.volmem`,
// `.manifest` bundles), parameter differential patching (`SculptureStateDiff`),
// bounded undo/redo history stacks, and frame-accurate video sequence recording,
// keeping `SculptureWorldNavigator` focused on world navigation and live frame
// orchestration.
// -----------------------------------------------------------------------------
class SculptureWorldStatePersistence {
public:
    [[nodiscard]] static SculptureWorldMode makeCaptureWorldMode(
        const SculptureWorldNavigator& navigator,
        const mgfx::Camera& cam,
        std::uint32_t generation
    );

    static SnapshotPaths saveSnapshotBundle(
        SculptureWorldNavigator& navigator,
        const std::string& directory,
        const std::string& baseName,
        const mgfx::Camera* cameraOverride
    );

    // Complete bounded manifest admission precedes any referenced state load.
    [[nodiscard]] static bool loadSnapshotBundle(
        SculptureWorldNavigator& navigator,
        const std::string& filepathOrManifest
    );

    static void requestSnapshot(SculptureWorldNavigator& navigator) noexcept;

    static void requestSnapshot(
        SculptureWorldNavigator& navigator,
        mgfx::vk::FrameLoop& frameLoop,
        const mgfx::Camera& cam,
        std::uint32_t generation,
        const std::string& directory
    );

    [[nodiscard]] static bool loadSnapshot(
        SculptureWorldNavigator& navigator,
        const std::string& filepath
    );

    [[nodiscard]] static bool applyWorldMode(
        SculptureWorldNavigator& navigator,
        const SculptureWorldMode& worldMode
    );

    [[nodiscard]] static SculptureStateDiff diffFromBaseline(
        const SculptureWorldNavigator& navigator,
        float epsilon
    );

    [[nodiscard]] static bool applyPatch(
        SculptureWorldNavigator& navigator,
        const SculptureStateDiff& diff,
        std::string_view tag
    );

    [[nodiscard]] static bool exportBaselineDiff(
        const SculptureWorldNavigator& navigator,
        const std::string& filepath
    );

    [[nodiscard]] static bool importDiffPatch(
        SculptureWorldNavigator& navigator,
        const std::string& filepath
    );

    static void pushHistory(
        SculptureWorldNavigator& navigator,
        std::string_view tag
    );

    [[nodiscard]] static bool undo(SculptureWorldNavigator& navigator);
    [[nodiscard]] static bool redo(SculptureWorldNavigator& navigator);
    static void clearHistory(SculptureWorldNavigator& navigator) noexcept;

    [[nodiscard]] static bool startSequenceRecording(
        SculptureWorldNavigator& navigator,
        std::string_view sessionName
    );
    [[nodiscard]] static bool stopSequenceRecording(
        SculptureWorldNavigator& navigator
    );
    [[nodiscard]] static SequenceFrameMetadata makeSequenceFrameMetadata(
        const SculptureWorldNavigator& navigator,
        const mgfx::Camera& cam,
        std::string_view customTag
    );
    [[nodiscard]] static bool recordFrame(
        SculptureWorldNavigator& navigator,
        const mgfx::vk::CaptureResult& res,
        const mgfx::Camera& cam,
        std::string_view customTag
    );
    [[nodiscard]] static bool recordFrame(
        SculptureWorldNavigator& navigator,
        std::uint32_t width,
        std::uint32_t height,
        const std::uint8_t* pixels,
        const mgfx::Camera& cam,
        bool isBgra,
        std::string_view customTag
    );

private:
    // The reload source may be the enclosing manifest rather than the decoded
    // file. Manifest loads suppress implicit siblings and apply only named volume.
    [[nodiscard]] static bool loadSnapshotState(
        SculptureWorldNavigator& navigator,
        const std::string& filepath,
        bool restoreSiblingVolume,
        const std::string& reloadSource
    );
};

} // namespace epi_sculpt
