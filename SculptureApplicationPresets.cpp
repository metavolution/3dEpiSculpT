// Preset identity, playlist provenance and curated morph-space navigation.
// These translation units share one SculptureApplication owner; none is a second scheduler.

#include "SculptureApplication.h"
#include "SculptureSerializer.h"
#include "SculpturePlaylistRestore.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace epi_sculpt {

const SculpturePresetAsset& SculptureApplication::presetAt(std::size_t index) const noexcept {
    static const SculpturePresetAsset fallback{
        .id = "builtin/fallback", .title = "Fallback", .category = "built-in"
    };
    if (const SculpturePresetAsset* preset = presetLibrary_.at(index)) {
        return *preset;
    }
    return fallback;
}

const SculpturePresetAsset& SculptureApplication::selectedPreset() const noexcept {
    return presetAt(control_.presetIndex());
}

bool SculptureApplication::selectPreset(
    std::size_t index, float durationSeconds, SculpturePresetTakeover takeover) {
    const auto transition = control_.selectPreset(
        presetLibrary_, index, durationSeconds, takeover, appState_.audioEnabled);
    if (!transition) {
        std::fprintf(stderr,
            "[SculptureApplication] Preset selection rejected (index %zu, duration %.3fs); "
            "semantic source retained.\n", index, durationSeconds);
        return false;
    }
    applyPresetSelection(*transition);
    return true;
}

bool SculptureApplication::reloadPresetLibrary() {
    SculpturePresetLibrary candidate;
    try {
        if (!candidate.reload(resourcePaths_.presetDirectory)) {
            for (const auto& diagnostic : candidate.diagnostics()) {
                std::fprintf(stderr, "[SculptureApplication] Preset reload: %s\n",
                             diagnostic.c_str());
            }
            return false;
        }
    } catch (const std::exception& error) {
        std::fprintf(stderr,
            "[SculptureApplication] Preset reload preparation failed; previous catalog retained: %s\n",
            error.what());
        return false;
    } catch (...) {
        std::fprintf(stderr,
            "[SculptureApplication] Preset reload preparation failed; previous catalog retained.\n");
        return false;
    }

    const auto transition = control_.replacePresetLibrary(
        presetLibrary_, candidate, 0.5f, appState_.audioEnabled);
    if (!transition) {
        std::fprintf(stderr,
            "[SculptureApplication] Preset catalog replacement rejected; "
            "previous catalog and semantic selection retained.\n");
        for (const auto& diagnostic : candidate.diagnostics()) {
            std::fprintf(stderr, "[SculptureApplication] Preset reload: %s\n",
                         diagnostic.c_str());
        }
        return false;
    }
    // The transition borrows the newly published catalog. Native failures after
    // this point follow existing recovery rules, not semantic/catalog rollback.
    if (transition->playlistRetired) {
        appliedPlaylistTargetIndex_.reset();
        std::fprintf(stderr,
            "[SculptureApplication] Preset reload retired a stale or unversioned tour; "
            "select a tour again to build its cues from the new catalog.\n");
    }
    applyPresetSelection(*transition);
    return true;
}

void SculptureApplication::applyPresetSelection(const SculpturePresetTransition& transition) {
    if (appState_.stateJustLoaded) {
        // A successful selection/retarget or reload supersedes the older pending
        // load graph. Settle its cinematic exit without overwriting the new target.
        if (!control_.sources().cinematic()) {
            control_.sources().leaveCinematic(true);
        }
        appState_.requestTemporalReset = true;
        appState_.loadedModulationGraph.reset();
        appState_.stateJustLoaded = false;
    }
    applyPresetTransition(transition);
    if (control_.sources().deckActive()) {
        // In live performance mode, load selected preset into opposite deck
        // so the VJ can smoothly crossfade toward it
        const DeckId targetDeck = (control_.sources().deck().crossfadePosition() <= 0.5f) ? DeckId::DeckB : DeckId::DeckA;
        control_.sources().deck().loadDeck(targetDeck, transition.asset.params,
                                  transition.asset.id, transition.asset.title);
    }
}

void SculptureApplication::applyPresetIdentity(std::size_t index) {
    if (const auto transition = control_.bindPreset(presetLibrary_, index)) {
        applyPresetTransition(*transition);
    }
}

void SculptureApplication::applyPresetTransition(const SculpturePresetTransition& transition) {
    const SculpturePresetAsset& preset = transition.asset;
    capturedWorldRecipe_.reset();
    capturedCanonicalSource_.reset();
    beginCompactHostStagedTransition();
    deckLibraryStep_ = transition.index;
    presentationOwnership_ = preset.presentationOwnership;
    presentationBindings_->director().setRenderParadigm(
        presentationOwnership_ == SculpturePresentationOwnership::UnifiedCompositeField
            ? RenderParadigm::UnifiedComposite
            : (presentationOwnership_ == SculpturePresentationOwnership::ImplicitField
                ? RenderParadigm::RaymarchedImplicit
                : RenderParadigm::VolumetricInstancing));
    configureWorldCadence(preset.worldRecipe);
    // Selection/reload already committed matching routing before native effects.
    // Playlist identity binding retains its separate existing reconciliation.
    if (!transition.modulationAdmitted) applyModulationGraph(preset.routingMap);
    syncMorphSpaceToActiveCategory();
    // Discrete topology/output changes cannot be interpolated. Resetting at
    // their boundary keeps temporal feedback and delivered geometry coherent.
    appState_.requestTemporalReset = true;
    control_.setManual4D(std::nullopt);
}

void SculptureApplication::appendLibraryPlaylistCapture(SculptureWorldMode& mode) const {
    mode.playlist = {};
    const auto& targets = control_.sources().presetMorph().playlist();
    if (targets.empty() || targets.size() > SculpturePlaylistCaptureState::kMaxTargets) return;

    // Playlist identities can be restored only when every target has an exact,
    // unique library ID. A nonempty title/alias or a duplicated ID is insufficient.
    // Ad-hoc targets are deliberately omitted rather than serializing duplicate
    // parameter blobs beside the captured moment's canonical parameters.
    for (const MorphTarget& target : targets) {
        if (!presetLibrary_.resolveExactId(target.presetId).asset) return;
    }

    mode.playlist.hasLibraryTargets = true;
    mode.playlist.active = control_.sources().presetMorph().isPlaylistActive();
    mode.playlist.paused = control_.sources().presetMorph().isPlaylistPaused();
    mode.playlist.loop = control_.sources().presetMorph().playlistLoops();
    mode.playlist.playbackMode = control_.sources().presetMorph().tourPlaybackMode();
    mode.playlist.targetIndex = static_cast<std::uint32_t>(control_.sources().presetMorph().playlistIndex());
    mode.playlist.dwellSeconds = control_.sources().presetMorph().playlistDwellTimer();
    mode.playlist.targets.reserve(targets.size());
    for (const MorphTarget& target : targets) {
        mode.playlist.targets.push_back({
            .presetId = target.presetId,
            .durationSeconds = target.durationSeconds,
            .dwellSeconds = target.dwellSeconds,
            .easing = target.easing,
            .presetRevision = target.presetRevision,
        });
    }
}

bool SculptureApplication::prepareLibraryPlaylistCapture(
    const SculpturePlaylistCaptureState& state, SculptureMorphChoreographer& restored) const {
    return tryRestoreLibraryPlaylist(presetLibrary_.entries(), state, restored);
}

void SculptureApplication::populateMorphSpaceFromPresetLibrary() noexcept {
    // 1. Synthesize dedicated quadrant banks for all categories discovered in preset library
    synthesizeCategoryMorphBanks();

    // 2. Corner references are stable IDs, not interactive titles or aliases.
    control_.sources().morphSpace().refreshBankCornerParams([this](std::string_view id) -> const SculptureProgramParams* {
        const auto* asset = presetLibrary_.resolveExactId(id).asset;
        return asset ? &asset->params : nullptr;
    });

    // 3. Automatically synchronize morph space to active category
    syncMorphSpaceToActiveCategory();
}

bool SculptureApplication::bindMorphSpaceToCategory(std::string_view category, bool autoLoad) noexcept {
    auto assets = presetLibrary_.findByCategory(category);
    std::string matchedCategory(category);
    if (assets.empty()) {
        for (const auto& cat : presetLibrary_.categories()) {
            if (cat.size() == category.size() &&
                std::equal(cat.begin(), cat.end(), category.begin(), category.end(),
                           [](char a, char b) { return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b)); })) {
                assets = presetLibrary_.findByCategory(cat);
                matchedCategory = cat;
                break;
            }
        }
    }
    if (assets.empty()) return false;

    // Select 4 contrasting presets for SW, SE, NW, NE corners
    std::array<std::string, 4> cornerPresetNames;
    std::array<SculptureProgramParams, 4> cornerParams;
    const std::size_t n = assets.size();

    std::size_t idxSW = 0;
    std::size_t idxSE = (n > 1) ? std::min(n - 1, std::max<std::size_t>(1, n / 3)) : 0;
    std::size_t idxNW = (n > 2) ? std::min(n - 1, std::max<std::size_t>(2, (2 * n) / 3)) : (n > 1 ? 1 : 0);
    std::size_t idxNE = (n > 3) ? (n - 1) : (n > 2 ? 2 : (n > 1 ? 1 : 0));

    const std::array<std::size_t, 4> chosenIndices = {idxSW, idxSE, idxNW, idxNE};
    for (std::size_t c = 0; c < 4; ++c) {
        const auto* asset = assets[chosenIndices[c]];
        cornerPresetNames[c] = asset->id;
        cornerParams[c] = asset->params;
    }

    const std::string bankId = "category_" + matchedCategory;
    std::string bankTitle = matchedCategory;
    if (!bankTitle.empty()) {
        bankTitle[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(bankTitle[0])));
    }
    bankTitle += " Quad";

    return control_.sources().morphSpace().tryUpsertCategoryBank(
        bankId, bankTitle, cornerPresetNames, cornerParams, autoLoad).has_value();
}

bool SculptureApplication::syncMorphSpaceToActiveCategory() noexcept {
    std::string category = selectedPreset().category;
    if (category.empty()) {
        category = "dimensional";
    }
    return bindMorphSpaceToCategory(category, /*autoLoad=*/true);
}

std::size_t SculptureApplication::synthesizeCategoryMorphBanks() noexcept {
    const auto cats = presetLibrary_.categories();
    std::size_t count = 0U;
    for (const auto& cat : cats) {
        if (bindMorphSpaceToCategory(cat, /*autoLoad=*/false)) {
            ++count;
        }
    }
    return count;
}

ReliefManifoldProjection SculptureApplication::resolveActiveReliefProjection() const noexcept {
    if (activeReliefProjectionOverride_.has_value()) {
        return *activeReliefProjectionOverride_;
    }
    return EpimorphicReliefGenerator::resolveReliefProjection(activeParams_);
}

void SculptureApplication::cycleReliefProjection(int direction) noexcept {
    constexpr int kTotalStates = 1 + static_cast<int>(kReliefManifoldProjectionCount);
    int currentIdx = 0;
    if (activeReliefProjectionOverride_.has_value()) {
        currentIdx = 1 + static_cast<int>(*activeReliefProjectionOverride_);
    }
    int nextIdx = (currentIdx + direction) % kTotalStates;
    if (nextIdx < 0) nextIdx += kTotalStates;

    if (nextIdx == 0) {
        activeReliefProjectionOverride_ = std::nullopt;
        std::fprintf(stderr,
            "[SculptureApplication] 2.5D Epimorphic Relief Projection: AUTO-RESOLVED (%s)\n",
            name(resolveActiveReliefProjection()));
    } else {
        const auto proj = static_cast<ReliefManifoldProjection>(nextIdx - 1);
        activeReliefProjectionOverride_ = proj;
        std::fprintf(stderr,
            "[SculptureApplication] 2.5D Epimorphic Relief Projection: MANUAL OVERRIDE -> %s (metric: %s)\n",
            name(proj),
            metricDescription(proj));
    }
}

} // namespace epi_sculpt
