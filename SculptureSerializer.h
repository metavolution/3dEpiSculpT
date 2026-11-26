#pragma once

#include "SculptureWorldRecipe.h"
#include "SculptureStateLoadLimits.h"
#include "SculptureProgramSchema.h"
#include "SculpturePresentationTypes.h"
#include "SculpturePresets.h"
#include "SculpturePlaylistCapture.h"
#include "SculptureSequenceLoadLimits.h"
#include "SculptureMorphSpace.h"
#include "SculptureVolumePresentationState.h"
#include <cstddef>
#include <string>
#include <string_view>
#include <optional>
#include <vector>
#include <functional>

namespace epi_sculpt {

struct PresentationTuningData {
    bool hasTuning = false;
    float fogDensity = 0.002f;
    float emissionThreshold = 1.0f;
    float emissionMultiplier = 1.0f;
};

struct SculptureWorldMode {
    // v23 persists portable volume appearance intent without native resources.
    // v22 binds durable tour cues to their materialized source revisions.
    // v21 preserves the canonical producer's complete 64-bit displayed identity.
    // v20 preserves library-playlist playback policy independently of its phase.
    // v19 joins captured pixels to the exact accepted readback submission.
    // v18 namespaces presentation evidence by logical-device/surface generation
    // so native counters restarted by recovery cannot alias in persisted captures.
    // v17 extends presentation provenance to the independently produced
    // implicit contribution used by implicit-only and unified-composite frames.
    // v16 links selected presentation generations to their accepted-production
    // receipts without persisting any native resource handle.
    // v15 distinguishes selected, submitted/presented and capture provenance.
    // v14 adds displayed generation locking (compactGeneration, worldGeneration).
    // v13 adds capture-derived presentation tuning data (fog, emission).
    // v12 adds resumable library-backed playlist provenance. Older captures
    // remain readable; a v11 capture is intentionally not claimed compatible
    // with readers that cannot inspect its complete route provenance.
    static constexpr int kCurrentVersion = 23;
    static constexpr int kVolumePresentationVersion = 23;
    static constexpr int kPlaylistRevisionVersion = 22;
    static constexpr int kWideCanonicalGenerationVersion = 21;
    // Complete captured moments became mandatory in v20. Keep this floor stable
    // when a newer writer version is added; old complete captures must not fall
    // back to the permissive legacy/default-filling admission path.
    static constexpr int kCompleteCaptureVersion = 20;
    // v4 introduces explicit recipe revision provenance. These captures bind
    // by stable ID; older captures may still identify a recipe by friendly name.
    static constexpr int kPresetProvenanceVersion = 4;

    // Decoding retains the source version for compatibility decisions. Writing
    // always emits kCurrentVersion, independently of this source-version field.
    int version = kCurrentVersion;
    SculptureWorldState state{};
    // Stable library ID and immutable source revision describe the starting
    // recipe; params/state below preserve the exact captured moment.
    std::string presetName;
    std::string presetRevision;
    SculpturePresentationOwnership presentationOwnership =
        SculpturePresentationOwnership::CompactInstances;
    // Capture provenance, not a second route-selection authority. It records
    // the exact bounded OpenCL/Vulkan recipe selected for this moment even if
    // a future catalogue binding changes its preferred world.
    bool hasResolvedWorldRecipe = false;
    WorldRecipe resolvedWorldRecipe{};
    SculpturePlaylistCaptureState playlist{};
    PresentationTuningData presentationTuning{};
    SculptureVolumePresentationState volumePresentation{};
    // Observational runtime provenance. These values never influence the
    // mathematical program; replay can choose to use the authored recipe.
    std::uint32_t requestedWorldBudget = 0U;
    std::uint32_t effectiveWorldBudget = 0U;
    float effectiveWorldUpdateHz = 0.0f;
    // Canonical generations and reset epochs have the same width as their
    // producer/publication identities; compact/world/implicit counters below
    // retain their own native contracts.
    std::uint64_t canonicalFeedbackGeneration = 0U;
    std::uint64_t canonicalFeedbackResetEpoch = 0ULL;
    std::uint32_t compactGeneration = 0U;
    std::uint32_t worldGeneration = 0U;
    std::uint32_t implicitGeneration = 0U;
    // Receipt provenance: selected values describe the frame recorded for a
    // capture; displayed values above are nonzero only after stable desktop
    // presentation confirmation. Numeric outcome mirrors mGFX FrameOutcome
    // without coupling this persisted format to renderer headers.
    std::uint64_t presentationFrameSequence = 0U;
    // Zero means no native presentation observation was attached (for example
    // a manually saved semantic WorldMode rather than a captured frame).
    std::uint64_t presentationDeviceGeneration = 0U;
    std::uint64_t presentationSurfaceGeneration = 0U;
    // Zero means unknown/no joined capture (including ordinary SaveState).
    std::uint64_t captureSubmissionSerial = 0U;
    std::uint32_t selectedCompactGeneration = 0U;
    std::uint64_t selectedCompactProductionReceiptSequence = 0U;
    std::uint32_t selectedWorldGeneration = 0U;
    std::uint64_t selectedWorldProductionReceiptSequence = 0U;
    std::uint64_t selectedCanonicalFeedbackGeneration = 0U;
    std::uint64_t selectedCanonicalFeedbackResetEpoch = 0ULL;
    std::uint64_t selectedCanonicalFeedbackProductionReceiptSequence = 0U;
    std::uint32_t selectedImplicitGeneration = 0U;
    std::uint64_t selectedImplicitProductionReceiptSequence = 0U;
    std::uint32_t presentationOutcome = 0U;
    bool presentationOutcomeKnown = false;
    bool presentationConfirmed = false;
    bool implicitExternalActive = false;
    std::string implicitDiagnostic{};
    std::uint64_t implicitQuarantineCount = 0ULL;
    SculptureProgramParams params{};
    std::vector<std::string> routingMap{};
    std::string macroAutomationJson{};
    SculptureMorphSpaceState morphSpace{};
};

// Owned codec output, not application state or a GPU checkpoint. Keeping the
// optional recipe distinguishes authored intent from catalog-derived defaults.
struct SculptureDecodedPreset {
    SculptureWorldState state{};
    std::string presetName;
    SculptureProgramParams params{};
    std::vector<std::string> routingMap;
    SculpturePresentationOwnership presentationOwnership = SculpturePresentationOwnership::CompactInstances;
    std::optional<WorldRecipe> declaredWorldRecipe;
    std::string macroAutomationJson;
};

enum class SculpturePresetDecodeError {
    None,
    InvalidJson,
    InvalidWorldMode,
    InvalidProjection,
    InvalidMacroAutomation,
    InvalidString,
    InvalidNumber,
    InvalidVector,
    UnknownSelector,
    UnknownPresentation,
    InvalidRoute
};

[[nodiscard]] constexpr std::string_view presetDecodeErrorName(SculpturePresetDecodeError error) noexcept {
    switch (error) {
        case SculpturePresetDecodeError::None: return "none";
        case SculpturePresetDecodeError::InvalidJson: return "invalid-json";
        case SculpturePresetDecodeError::InvalidWorldMode: return "invalid-worldmode";
        case SculpturePresetDecodeError::InvalidProjection: return "invalid-projection";
        case SculpturePresetDecodeError::InvalidMacroAutomation: return "invalid-macro-automation";
        case SculpturePresetDecodeError::InvalidString: return "invalid-string";
        case SculpturePresetDecodeError::InvalidNumber: return "invalid-number";
        case SculpturePresetDecodeError::InvalidVector: return "invalid-vector";
        case SculpturePresetDecodeError::UnknownSelector: return "unknown-selector";
        case SculpturePresetDecodeError::UnknownPresentation: return "unknown-presentation";
        case SculpturePresetDecodeError::InvalidRoute: return "invalid-route";
    }
    return "unknown";
}

// Own the field name so rejection context survives the source byte snapshot.
// Reasons describe codec admission, not downstream route/macro semantics or GPU readiness.
struct SculpturePresetDecodeDiagnostic {
    SculpturePresetDecodeError error = SculpturePresetDecodeError::None;
    std::string field;
};

struct SculpturePresetDecodeResult {
    std::optional<SculptureDecodedPreset> preset;
    SculpturePresetDecodeDiagnostic diagnostic;
};

// -----------------------------------------------------------------------------
// State Parameter Diffing & Delta Patching Model
// -----------------------------------------------------------------------------
struct ParameterDelta {
    std::string key;
    std::string oldValue;
    std::string newValue;
    float numericDelta = 0.0f;
    bool isNumeric = false;
};

struct SculptureStateDiff {
    std::vector<ParameterDelta> deltas{};

    [[nodiscard]] bool empty() const noexcept { return deltas.empty(); }
    [[nodiscard]] std::size_t size() const noexcept { return deltas.size(); }

    [[nodiscard]] bool hasChange(std::string_view key) const noexcept {
        for (const auto& d : deltas) {
            if (d.key == key) return true;
        }
        return false;
    }

    [[nodiscard]] const ParameterDelta* getDelta(std::string_view key) const noexcept {
        for (const auto& d : deltas) {
            if (d.key == key) return &d;
        }
        return nullptr;
    }
};

// -----------------------------------------------------------------------------
// Educational Annotation: State Serialization
// This module provides deterministic serialization of the visual program state.
// It allows users to save and share their interactive parameter discoveries,
// preventing the loss of complex 4D structural states upon application exit.
// -----------------------------------------------------------------------------
// File publication contract for saveToFile, saveDiffToFile, saveWorldModeToFile
// and saveEpimodeToFile (POSIX): encode completely, exclusively stage a sibling,
// then rename only after write/permission/sync/close succeed. Ordinary IO failure
// preserves prior destination bytes; encoding/allocation exceptions may propagate.
// Reject empty/NUL paths and existing symlink/nonregular destinations. New files
// use 0600; replacements retain ordinary rwx bits. Single-writer policy: inode
// ownership/ACLs, concurrent destination edits, capture-group atomicity and durable
// directory rename after power loss are outside this contract. Saves may block on
// file sync. This contract does not extend to gesture or other export writers.
class SculptureSerializer {
public:
    // Sequence parsing stays format-oriented; application catalogues may
    // resolve stable symbolic IDs without teaching the serializer about paths.
    using SequencePresetResolver = std::function<bool(
        std::string_view id,
        SculptureProgramParams& params,
        SculptureWorldState& state,
        std::vector<std::string>& routingMap)>;
    // Serializes the parameters into a human-readable, version-controlled text format.
    [[nodiscard]] static std::string serialize(const SculptureProgramParams& params);

    // Deserializes a text payload back into parameters. 
    // Returns std::nullopt if the format is invalid or fundamentally corrupted.
    [[nodiscard]] static std::optional<SculptureProgramParams> deserialize(const std::string& payload);

    // State parameter diffing, delta patching, and drift inspection
    [[nodiscard]] static SculptureStateDiff diff(
        const SculptureProgramParams& base,
        const SculptureProgramParams& target,
        float epsilon = 1e-5f);

    static bool applyDiff(
        SculptureProgramParams& target,
        const SculptureStateDiff& diff);

    [[nodiscard]] static std::string formatDiff(const SculptureStateDiff& diff);

    // Epimorphic Micro-Patch Differential Presets (.epidiff)
    [[nodiscard]] static std::string serializeDiff(const SculptureStateDiff& diff);
    [[nodiscard]] static std::optional<SculptureStateDiff> deserializeDiff(const std::string& payload);
    static bool saveDiffToFile(const std::string& filepath, const SculptureStateDiff& diff);
    [[nodiscard]] static std::optional<SculptureStateDiff> loadDiffFromFile(const std::string& filepath);
    
    // Helper IO functions
    static bool saveToFile(const std::string& filepath, const SculptureProgramParams& params);
    // Acquire one owned byte snapshot for all state-format probes. Zero disables
    // input; ordinary IO/byte-budget errors return nullopt before decoding. No
    // seek-sized allocation. Memory/allocation exceptions may propagate.
    [[nodiscard]] static std::optional<std::string> readStateFile(
        const std::string& filepath, const StateLoadLimits& limits = {});
    // Versioned WorldMode input is validated completely before extracting params.
    // This content-only operation lets hosts reuse already acquired bytes.
    [[nodiscard]] static std::optional<SculptureProgramParams> deserializeParameterState(
        const std::string& content);
    // Compatibility signature uses StateLoadLimits defaults; explicit host policy
    // is available without changing the content decoder's mathematical semantics.
    [[nodiscard]] static std::optional<SculptureProgramParams> loadFromFile(const std::string& filepath);
    [[nodiscard]] static std::optional<SculptureProgramParams> loadFromFile(
        const std::string& filepath, const StateLoadLimits& limits);

    // The capture format owns complete application-side world state. It stays
    // independent of OpenCL and mGFX so saved worlds remain portable inputs.
    [[nodiscard]] static std::string serializeWorldMode(const SculptureWorldMode& mode);
    [[nodiscard]] static std::optional<SculptureWorldMode> deserializeWorldMode(const std::string& payload);
    // Uses the shared state-file publication contract above.
    static bool saveWorldModeToFile(const std::string& filepath, const SculptureWorldMode& mode);
    [[nodiscard]] static std::optional<SculptureWorldMode> loadWorldModeFromFile(const std::string& filepath);
    [[nodiscard]] static std::optional<SculptureWorldMode> loadWorldModeFromFile(
        const std::string& filepath, const StateLoadLimits& limits);

    // Canonical .epimode JSON sidecar serialization
    [[nodiscard]] static std::string serializeEpimode(const SculptureWorldMode& mode);
    static bool saveEpimodeToFile(const std::string& filepath, const SculptureWorldMode& mode);

    // Loads a plain-text playlist (.episeq) of presets and .epimode files into the sequencer.
    // Prepare privately and replace only when at least one keyframe resolves.
    // False results or propagated parsing/resolver exceptions leave the live
    // sequencer unchanged. Resolver side effects remain the caller's concern.
    // Declared durations must be complete finite numbers at least the sequencer's
    // minimum duration; accepted keyframes must keep float timeline time finite
    // and strictly advancing. Omitted durations keep the 3-second default.
    // Numeric target indices must fit size_t. LOOP configures wrapping regardless
    // of declaration position; the last declaration wins. Omission inherits the
    // prior policy. Successful replacement is stopped at its initial sample.
    // Partial-loading/easing and accepted boolean spellings retain their behavior.
    // Legacy signatures use bounded defaults. Explicit limits apply equally to
    // file and content input; exceeding either budget rejects the whole candidate.
    static bool loadSequenceFromFile(const std::string& filepath, SculptureSequencer& sequencer);
    static bool deserializeSequence(const std::string& content, SculptureSequencer& sequencer);
    static bool loadSequenceFromFile(const std::string& filepath,
                                     SculptureSequencer& sequencer,
                                     const SequencePresetResolver& resolver);
    static bool deserializeSequence(const std::string& content,
                                    SculptureSequencer& sequencer,
                                    const SequencePresetResolver& resolver);
    static bool loadSequenceFromFile(const std::string& filepath,
                                     SculptureSequencer& sequencer,
                                     const SequencePresetResolver& resolver,
                                     const SequenceLoadLimits& limits);
    static bool deserializeSequence(const std::string& content,
                                    SculptureSequencer& sequencer,
                                    const SequencePresetResolver& resolver,
                                    const SequenceLoadLimits& limits);

    // Gesture Trajectory Serialization
    [[nodiscard]] static std::string serializeGestureTrajectory(const struct GestureTrajectory& trajectory);
    [[nodiscard]] static std::optional<struct GestureTrajectory> deserializeGestureTrajectory(const std::string& payload);
    static bool saveGestureTrajectoryToFile(const std::string& filepath, const struct GestureTrajectory& trajectory);
    [[nodiscard]] static std::optional<struct GestureTrajectory> loadGestureTrajectoryFromFile(const std::string& filepath);

    // Macro Automation Engine Serialization
    [[nodiscard]] static std::string serializeMacroEngine(const class SculptureMacroAutomationEngine& engine);
    static bool deserializeMacroEngine(const std::string& content, class SculptureMacroAutomationEngine& engine);

    // Decode one owned semantic value from legacy JSON or WorldMode, without IO.
    // Failure publishes no candidate. Macro text is structurally admitted here;
    // its semantic validation remains with the existing automation owner.
    [[nodiscard]] static std::optional<SculptureDecodedPreset> decodePresetContent(
        const std::string& content, std::string_view fallbackPresetName);

    // Same admission and single decode; the first failing codec boundary supplies
    // a typed reason and, when known, its raw selected JSON member name.
    // Syntax and WorldMode failures do not claim an individual offending field.
    // Success returns a complete preset with None/empty diagnostics; rejection
    // returns no preset. Allocation exceptions retain their existing behavior.
    [[nodiscard]] static SculpturePresetDecodeResult decodePresetContentDetailed(
        const std::string& content, std::string_view fallbackPresetName);

    // Compatibility adapter over the same decoder, retaining its existing
    // omitted-field and optional-output behavior for persistence callers.
    // Decode already-owned legacy JSON or typed WorldMode text without file IO.
    // The caller owns fallback naming; library provenance can hash these same bytes.
    // Missing legacy scalar fields keep defaults; invalid present numeric values
    // reject rather than silently substituting a different mathematical input.
    // Present legacy camera vectors require exactly three finite numeric values;
    // each vector is assigned only after its complete value is admitted.
    // Legacy scalar strings accept whitespace around colons and require complete
    // quoted values. Standard short JSON escapes are decoded; unsupported escapes
    // (including Unicode escapes), raw control characters and malformed values reject.
    // Present mathematical selectors must resolve through the existing canonical/
    // compatibility matching rules; unmatched values reject instead of selecting
    // a default algorithm. Missing selectors retain the existing omission policy.
    // Macro metadata, when present, selects one top-level object; duplicate,
    // wrong-type or malformed declarations reject even without outMacroJson.
    // Nested extension members are not macro sources. Semantic macro validation
    // remains with SculptureMacroAutomationEngine before application takeover.
    static bool deserializeEpimode(
        const std::string& content, std::string_view fallbackPresetName,
        SculptureWorldState& outMode, std::string& outPresetName,
        SculptureProgramParams& outParams, std::vector<std::string>& outRoutingMap,
        SculpturePresentationOwnership* outOwnership = nullptr,
        std::string* outMacroJson = nullptr);

    // File adapters retain the existing API and use StateLoadLimits defaults.
    // Optional output defaults on acquisition failure retain their legacy behavior.
    static bool loadEpimodeFromFile(const std::string& path, SculptureWorldState& outMode, std::string& outPresetName, SculptureProgramParams& outParams, std::vector<std::string>& outRoutingMap, SculpturePresentationOwnership* outOwnership, std::string* outMacroJson);
    static bool loadEpimodeFromFile(const std::string& path, SculptureWorldState& outMode, std::string& outPresetName, SculptureProgramParams& outParams, std::vector<std::string>& outRoutingMap, SculpturePresentationOwnership* outOwnership = nullptr);
    static bool loadEpimodeFromFile(
        const std::string& path, SculptureWorldState& outMode, std::string& outPresetName,
        SculptureProgramParams& outParams, std::vector<std::string>& outRoutingMap,
        SculpturePresentationOwnership* outOwnership, std::string* outMacroJson,
        const StateLoadLimits& limits);

private:
    // One format decision/parser serves both API shapes. Internal borrowed
    // destinations preserve legacy partial-output semantics without exposing a
    // partial typed candidate to new consumers.
    static bool decodeEpimodeInto(
        const std::string& content, std::string_view fallbackPresetName,
        SculptureWorldState& outMode, std::string& outPresetName,
        SculptureProgramParams& outParams, std::vector<std::string>& outRoutingMap,
        SculpturePresentationOwnership* outOwnership, std::string* outMacroJson,
        std::optional<WorldRecipe>* outRecipe,
        SculpturePresetDecodeDiagnostic* diagnostic);
};

} // namespace epi_sculpt
