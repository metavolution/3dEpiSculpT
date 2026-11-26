// Renderer/device-free authoring persistence and capture provenance serializer
// for the Recursive Counter-Winding Hopf Torus.
//
// Provides:
// 1. Human-readable key-value (.epihopf) recipe serialization and parsing.
// 2. Canonical JSON grammar serialization (used by both CLI inspector and captures).
// 3. Atomic sibling-directory capture bundle writer joining accepted domain
//    generation, traversal telemetry, and native presentation outcome facts.

#pragma once

#include "RecursiveHopfTorusMath.h"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace epi_sculpt::recursive_hopf {

struct CaptureObservationMetadata {
    std::uint32_t generation = 0U;
    std::string producerDevice = "CPU-reference";
    bool producedByOpenCl = false;
    float evaluationMilliseconds = 0.0f;
    bool stereoSideBySide = false;
    bool presentationConfirmed = false;
    std::uint64_t submissionSerial = 0U;
    std::string presetName{};
    std::string paletteName{};
    // Additive observation facts; CPU traversal below remains a reference
    // calculation, not evidence that a GPU-authored stream was read back.
    std::string instanceStream = "unspecified";
    std::optional<std::uint32_t> submittedInstanceCount{};
    std::optional<std::uint32_t> submittedNodeCount{};
    bool geometrySubmissionDeferred = false;
    // Unspecified for headless/legacy writers; a device selection fact, not
    // evidence that external-memory imports or synchronization were qualified.
    std::optional<bool> openClMatchesPresentationDevice{};
    std::optional<std::uint64_t> externalGrammarAllocation{};
    std::optional<std::uint64_t> externalGrammarHandoff{};
};

[[nodiscard]] PlacementMode parsePlacementMode(std::string_view text);

[[nodiscard]] std::string serializeRecipeText(
    const Recipe& recipe,
    std::string_view title = "");

[[nodiscard]] Recipe parseRecipeText(std::string_view text);

[[nodiscard]] Recipe loadRecipeFile(const std::filesystem::path& path);

void saveRecipeFile(
    const std::filesystem::path& path,
    const Recipe& recipe,
    std::string_view title = "");

[[nodiscard]] std::string serializeGrammarJson(const Grammar& grammar);

[[nodiscard]] std::string serializeObservationReport(
    const Grammar& grammar,
    const TraversalBudget& budget,
    const TraversalResult& traversal,
    const CaptureObservationMetadata& metadata);

// Publishes recipe.epihopf, grammar.json, and observation.txt as one complete
// directory. An existing destination (including an empty directory or symlink)
// is never replaced. Uses exclusive attempt-owned staging and Linux renameat2
// with RENAME_NOREPLACE; unsupported platforms/filesystems fail closed.
// Failures clean up only this attempt's staging. Atomic visibility does not
// imply power-loss durability; this metadata bundle does not contain pixels.
void writeCaptureBundle(
    const std::filesystem::path& bundleDirectory,
    const Grammar& grammar,
    const TraversalBudget& budget,
    const TraversalResult& traversal,
    const CaptureObservationMetadata& metadata);

} // namespace epi_sculpt::recursive_hopf
