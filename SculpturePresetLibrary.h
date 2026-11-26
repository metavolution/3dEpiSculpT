#pragma once

#include "SculpturePresetAsset.h"
#include "SculpturePresetLoadLimits.h"
#include "SculptureSequenceLoadLimits.h"

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace epi_sculpt {


enum class SculptureTourEntryStatus {
    Ready,
    MissingAsset,
    UnsupportedRoute,
    AmbiguousAsset,
};

struct SculptureTourEntry {
    std::string presetId;
    std::string title;
    SculptureTourEntryStatus status = SculptureTourEntryStatus::MissingAsset;
};

// An owned snapshot of library/route availability, not native capability or
// performance qualification. Reloading the library cannot invalidate its text.
struct SculptureTourInspection {
    bool recognized = false;
    std::vector<SculptureTourEntry> entries;
    [[nodiscard]] std::size_t readyCount() const noexcept;
};

class SculpturePresetLibrary {
public:
    // Rebuilds a deterministic unified catalogue: built-ins and connected root
    // pattern worlds, lexically sorted presets/**/*.epimode assets from the
    // supplied roots, then strictly admitted canonical feedback assets. The
    // loader does not guess checkout casing or walk cwd. Invalid files and
    // unsupported canonical feedback candidates are diagnosed and skipped. Discovery/budget failure
    // returns false and preserves existing entries (including their addresses);
    // a fresh library instead receives the built-in/root-pattern/feedback fallback.
    // Diagnostics always describe the latest completed reload attempt.
    // Successful reload replaces entries and invalidates pointers into them.
    bool reload(const std::filesystem::path& root,
                const std::filesystem::path& secondaryRoot = {});
    // Host acquisition budgets; the compatibility overload uses bounded defaults.
    // maxBytes == 0 suppresses content reads but retains built-in/root-pattern/feedback
    // assets and complete discovery within the other budgets. Individual oversized
    // files skip; directory/file-count/aggregate exhaustion rejects the candidate.
    // Other zero budgets admit only an empty corresponding resource (depth zero
    // still admits root children). Failed startup exposes no partial authored set.
    bool reload(const std::filesystem::path& root,
                const std::filesystem::path& secondaryRoot,
                const PresetLoadLimits& limits);

    // Publish an already admitted candidate without allocation or asset moves.
    // Element borrows follow their storage to the other owner; borrows into the
    // retired catalog expire when that owner destroys or replaces its storage.
    void swap(SculpturePresetLibrary& other) noexcept {
        entries_.swap(other.entries_);
        discoveredSourcePaths_.swap(other.discoveredSourcePaths_);
        diagnostics_.swap(other.diagnostics_);
    }

    [[nodiscard]] const std::vector<SculpturePresetAsset>& entries() const noexcept {
        return entries_;
    }
    // All regular .epimode candidates from the published discovery, including
    // rejected, unreadable/oversized and duplicate-ID inputs. Their authored
    // files remain protected even when no selectable asset could be decoded.
    // Published/swapped with entries; failed reload preserves both. This is a
    // borrowed path inventory, not a filesystem reservation or content snapshot.
    [[nodiscard]] const std::vector<std::filesystem::path>& discoveredSourcePaths() const noexcept {
        return discoveredSourcePaths_;
    }
    // Durable identities are stricter than interactive names: no title/alias
    // fallback and no discovery-order choice among duplicates. Borrow only after
    // the full scan proves uniqueness; successful reload invalidates the pointer.
    [[nodiscard]] SculpturePresetIdResolution resolveExactId(std::string_view id) const noexcept {
        if (id.empty()) return {};
        const SculpturePresetAsset* selected = nullptr;
        for (const auto& entry : entries_) {
            if (entry.id != id) continue;
            if (selected) return {SculpturePresetIdStatus::Ambiguous, nullptr};
            selected = &entry;
        }
        return selected ? SculpturePresetIdResolution{SculpturePresetIdStatus::Found, selected}
                        : SculpturePresetIdResolution{};
    }
    // Index projection for application selection; share exact uniqueness policy.
    // Consume synchronously: a successful reload invalidates indices and pointers.
    [[nodiscard]] std::optional<std::size_t> findExactIndex(std::string_view id) const noexcept {
        const auto resolved = resolveExactId(id);
        if (!resolved.asset) return std::nullopt;
        // The resolver returns an element of this catalog's contiguous storage.
        return static_cast<std::size_t>(resolved.asset - entries_.data());
    }
    [[nodiscard]] const SculpturePresetAsset* at(std::size_t index) const noexcept;
    [[nodiscard]] std::optional<std::size_t> findIndex(
        std::string_view idTitleOrAlias) const noexcept;
    [[nodiscard]] const SculpturePresetAsset* find(std::string_view idOrTitle) const noexcept;
    [[nodiscard]] const std::vector<std::string>& diagnostics() const noexcept {
        return diagnostics_;
    }
    [[nodiscard]] bool loadSequenceFromFile(const std::filesystem::path& path,
                                            SculptureSequencer& sequencer) const;
    // Explicit host budgets; the compatibility overload uses bounded defaults.
    [[nodiscard]] bool loadSequenceFromFile(const std::filesystem::path& path,
                                            SculptureSequencer& sequencer,
                                            const SequenceLoadLimits& limits) const;

    // Category and Curated Tour Queries & Sequencer Population
    [[nodiscard]] std::vector<std::string> categories() const;
    [[nodiscard]] std::vector<const SculpturePresetAsset*> findByCategory(std::string_view category) const;
    // Definition names; inspect each against the currently loaded assets.
    [[nodiscard]] std::vector<std::string> availableTours() const;
    // Ready entries carry exact unique IDs. Friendly tour tokens may resolve
    // first, but titles cannot substitute for missing known namespaced IDs.
    [[nodiscard]] SculptureTourInspection inspectCategoryTour(std::string_view category) const;
    // Unrecognized names retain the existing category-name fallback.
    [[nodiscard]] SculptureTourInspection inspectCuratedTour(std::string_view tourName) const;
    // Failed/empty/non-finite timing requests or invalid authored modulation in
    // any included cue leave all live choreography intact. Route admission uses
    // the installation parser without installing a graph or changing history;
    // empty authored maps retain application-default policy. Preparation/allocation
    // exceptions propagate before replacement, preserving the old choreography.
    // Partial tours retain ready entries in authored order, as reported above;
    // malformed modulation rejects the candidate rather than dropping that cue.
    bool buildCategoryTour(class SculptureMorphChoreographer& choreographer,
                           std::string_view category,
                           float durationSeconds = 3.0f,
                           float dwellSeconds = 6.0f,
                           MorphEasing easing = MorphEasing::Smoothstep) const;
    bool buildCuratedTour(class SculptureMorphChoreographer& choreographer,
                          std::string_view tourName,
                          float durationSeconds = 3.0f,
                          float dwellSeconds = 6.0f,
                          MorphEasing easing = MorphEasing::Smoothstep) const;

private:
    [[nodiscard]] SculptureTourEntry inspectTourEntry(std::string_view presetId) const;
    bool buildInspectedTour(class SculptureMorphChoreographer& choreographer,
                            const SculptureTourInspection& inspection,
                            float durationSeconds, float dwellSeconds,
                            MorphEasing easing) const;
    // Called only on a fresh candidate, never on the published library.
    bool loadCandidate(const std::filesystem::path& root,
                       const std::filesystem::path& secondaryRoot,
                       const PresetLoadLimits& limits);
    void appendBuiltIns();
    void appendRootPatternWorlds();
    void appendCanonicalFeedback();
    std::vector<SculpturePresetAsset> entries_;
    std::vector<std::filesystem::path> discoveredSourcePaths_;
    std::vector<std::string> diagnostics_;
};

} // namespace epi_sculpt
