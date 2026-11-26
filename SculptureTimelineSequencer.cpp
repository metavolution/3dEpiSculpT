#include "SculptureTimelineSequencer.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cctype>

namespace epi_sculpt {

namespace {

[[nodiscard]] std::string trim(std::string_view s) {
    std::size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) {
        ++start;
    }
    std::size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) {
        --end;
    }
    return std::string(s.substr(start, end - start));
}

[[nodiscard]] CrossfadeCurve parseCurve(std::string_view str) {
    if (str == "Linear" || str == "linear") return CrossfadeCurve::Linear;
    if (str == "EqualPower" || str == "equal_power" || str == "equalpower") return CrossfadeCurve::EqualPower;
    if (str == "Smoothstep" || str == "smoothstep" || str == "smooth_step") return CrossfadeCurve::Smoothstep;
    if (str == "SharpCut" || str == "sharp_cut" || str == "cut") return CrossfadeCurve::SharpCut;
    return CrossfadeCurve::EqualPower;
}

[[nodiscard]] std::string_view curveName(CrossfadeCurve curve) noexcept {
    switch (curve) {
    case CrossfadeCurve::Linear: return "Linear";
    case CrossfadeCurve::EqualPower: return "EqualPower";
    case CrossfadeCurve::Smoothstep: return "Smoothstep";
    case CrossfadeCurve::SharpCut: return "SharpCut";
    }
    return "EqualPower";
}

} // namespace

SculptureTimelineSequencer::SculptureTimelineSequencer(TimelineSequence sequence) {
    loadTimeline(std::move(sequence));
}

void SculptureTimelineSequencer::loadTimeline(TimelineSequence sequence) {
    sequence_ = std::move(sequence);
    stop();
}

bool SculptureTimelineSequencer::loadTimelineFromJson(std::string_view jsonStr) {
    // Lightweight JSON parser designed for TimelineSequence schemas without
    // pulling in heavy third-party parsing dependencies.
    TimelineSequence seq;
    seq.cues.clear();

    std::size_t pos = 0;
    const auto findNext = [&](std::string_view key) -> std::optional<std::string> {
        const std::string needle = "\"" + std::string(key) + "\"";
        const std::size_t found = jsonStr.find(needle, pos);
        if (found == std::string_view::npos) return std::nullopt;
        const std::size_t colon = jsonStr.find(':', found + needle.size());
        if (colon == std::string_view::npos) return std::nullopt;
        std::size_t start = colon + 1;
        while (start < jsonStr.size() && std::isspace(static_cast<unsigned char>(jsonStr[start]))) ++start;
        if (start >= jsonStr.size()) return std::nullopt;

        if (jsonStr[start] == '"') {
            const std::size_t end = jsonStr.find('"', start + 1);
            if (end == std::string_view::npos) return std::nullopt;
            return std::string(jsonStr.substr(start + 1, end - start - 1));
        }

        std::size_t end = start;
        while (end < jsonStr.size() && jsonStr[end] != ',' && jsonStr[end] != '}' && jsonStr[end] != ']'
               && !std::isspace(static_cast<unsigned char>(jsonStr[end]))) {
            ++end;
        }
        return std::string(jsonStr.substr(start, end - start));
    };

    // Parse top-level fields
    if (const auto nameVal = findNext("name")) {
        seq.name = *nameVal;
    }
    if (const auto loopVal = findNext("loop")) {
        seq.loop = (*loopVal == "true" || *loopVal == "1");
    }

    // Parse cues array
    const std::size_t cuesPos = jsonStr.find("\"cues\"");
    if (cuesPos != std::string_view::npos) {
        std::size_t openBracket = jsonStr.find('[', cuesPos);
        if (openBracket != std::string_view::npos) {
            std::size_t cur = openBracket + 1;
            while (cur < jsonStr.size()) {
                const std::size_t objStart = jsonStr.find('{', cur);
                if (objStart == std::string_view::npos) break;
                const std::size_t objEnd = jsonStr.find('}', objStart);
                if (objEnd == std::string_view::npos) break;

                std::string_view cueJson = jsonStr.substr(objStart, objEnd - objStart + 1);
                TimelineCue cue;

                const auto getField = [&](std::string_view key) -> std::optional<std::string> {
                    const std::string needle = "\"" + std::string(key) + "\"";
                    const std::size_t f = cueJson.find(needle);
                    if (f == std::string_view::npos) return std::nullopt;
                    const std::size_t c = cueJson.find(':', f + needle.size());
                    if (c == std::string_view::npos) return std::nullopt;
                    std::size_t s = c + 1;
                    while (s < cueJson.size() && std::isspace(static_cast<unsigned char>(cueJson[s]))) ++s;
                    if (s >= cueJson.size()) return std::nullopt;
                    if (cueJson[s] == '"') {
                        const std::size_t e = cueJson.find('"', s + 1);
                        if (e == std::string_view::npos) return std::nullopt;
                        return std::string(cueJson.substr(s + 1, e - s - 1));
                    }
                    std::size_t e = s;
                    while (e < cueJson.size() && cueJson[e] != ',' && cueJson[e] != '}'
                           && !std::isspace(static_cast<unsigned char>(cueJson[e]))) ++e;
                    return std::string(cueJson.substr(s, e - s));
                };

                if (auto val = getField("preset")) cue.presetIdOrName = *val;
                else if (auto val2 = getField("preset_id")) cue.presetIdOrName = *val2;
                else if (auto val3 = getField("id")) cue.presetIdOrName = *val3;

                if (auto val = getField("title")) cue.title = *val;

                if (auto val = getField("dwell_sec")) {
                    try { cue.dwellSeconds = std::stof(*val); } catch (...) {}
                } else if (auto val2 = getField("dwellSeconds")) {
                    try { cue.dwellSeconds = std::stof(*val2); } catch (...) {}
                }

                if (auto val = getField("transition_sec")) {
                    try { cue.transitionSeconds = std::stof(*val); } catch (...) {}
                } else if (auto val2 = getField("transitionSeconds")) {
                    try { cue.transitionSeconds = std::stof(*val2); } catch (...) {}
                }

                if (auto val = getField("curve")) {
                    cue.curve = parseCurve(*val);
                }

                if (!cue.presetIdOrName.empty()) {
                    if (cue.title.empty()) {
                        cue.title = cue.presetIdOrName;
                    }
                    seq.cues.push_back(std::move(cue));
                }

                cur = objEnd + 1;
                const std::size_t closeBracket = jsonStr.find(']', cur);
                const std::size_t nextObj = jsonStr.find('{', cur);
                if (closeBracket != std::string_view::npos && (nextObj == std::string_view::npos || closeBracket < nextObj)) {
                    break;
                }
            }
        }
    }

    if (seq.cues.empty()) {
        std::fprintf(stderr, "[SculptureTimelineSequencer] Warning: Parsed timeline JSON yielded 0 cues.\n");
        return false;
    }

    loadTimeline(std::move(seq));
    std::fprintf(stderr, "[SculptureTimelineSequencer] Loaded timeline \"%s\" with %zu cues (total duration: %.1fs).\n",
        sequence_.name.c_str(), sequence_.cues.size(), sequence_.totalDuration());
    return true;
}

bool SculptureTimelineSequencer::loadTimelineFromFile(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::fprintf(stderr, "[SculptureTimelineSequencer] Failed opening timeline file: %s\n", path.string().c_str());
        return false;
    }
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return loadTimelineFromJson(content);
}

bool SculptureTimelineSequencer::buildFromTour(
    const SculpturePresetLibrary& library,
    std::string_view tourName,
    float dwellSeconds,
    float transitionSeconds,
    CrossfadeCurve curve,
    bool loop
) {
    std::vector<std::string_view> idList;
    if (tourName == "higher_dimensional_soliton_odyssey") {
        idList = {
            "authored/dimensional/140_so4_clifford_hopf_fibration_4d",
            "authored/dimensional/141_poincare_dodecahedral_homotopy_4d",
            "authored/dimensional/142_calabi_yau_threefold_quintic_35d",
            "authored/dimensional/143_e8_coxeter_plane_projection_4d",
            "authored/dimensional/144_belousov_scroll_wave_filament_3d",
            "authored/dimensional/145_seifert_hyperbolic_knot_soliton_35d",
        };
    } else if (tourName == "non_euclidean_poincare_clifford_odyssey") {
        idList = {
            "authored/dimensional/108_poincare_hyperbolic_disk_reduction_4d",
            "authored/dimensional/109_clifford_foliation_hyper_torus_35d",
            "authored/dimensional/110_hyperbolic_to_clifford_homotopy_4d",
            "authored/dimensional/71_hyperbolic_poincare_geodesic_honeycomb",
            "authored/dimensional/80_clifford_so4_isoclinic_fibration",
        };
    } else if (tourName == "hyperdimensional_4d_projections_odyssey") {
        idList = {
            "authored/dimensional/100_clifford_torus_stereographic_projection",
            "authored/dimensional/101_calabi_yau_cross_cap_foliation",
            "authored/dimensional/102_hopf_fibration_quantum_vortex",
            "authored/dimensional/103_hyperbolic_icosahedral_honeycomb_4d",
            "authored/dimensional/104_epimorphic_complex_plane_relief_25d",
            "authored/dimensional/80_clifford_so4_isoclinic_fibration",
        };
    } else if (tourName == "vision_dimensional_frontiers") {
        idList = {
            "authored/dimensional/121_apollonian_gasket_spherepack_3d",
            "authored/dimensional/122_conformal_wing_aerofoil_relief_25d",
            "authored/dimensional/123_quartic_spiral_pinwheel_foliation_4d",
            "authored/dimensional/124_hofstadter_butterfly_quantum_lattice_35d",
        };
    } else {
        // Fallback: pick up to 6 presets from the catalog
        const auto& entries = library.entries();
        if (entries.empty()) return false;
        const std::size_t count = std::min<std::size_t>(6U, entries.size());
        for (std::size_t i = 0; i < count; ++i) {
            TimelineCue cue;
            cue.presetIdOrName = entries[i].id;
            cue.title = entries[i].title;
            cue.dwellSeconds = dwellSeconds;
            cue.transitionSeconds = transitionSeconds;
            cue.curve = curve;
            sequence_.cues.push_back(std::move(cue));
        }
        sequence_.name = std::string(tourName);
        sequence_.loop = loop;
        stop();
        return true;
    }

    TimelineSequence seq;
    seq.name = std::string(tourName);
    seq.loop = loop;
    for (const auto id : idList) {
        const SculpturePresetAsset* asset = library.find(id);
        TimelineCue cue;
        cue.presetIdOrName = std::string(id);
        cue.title = asset ? asset->title : std::string(id);
        cue.dwellSeconds = dwellSeconds;
        cue.transitionSeconds = transitionSeconds;
        cue.curve = curve;
        seq.cues.push_back(std::move(cue));
    }

    loadTimeline(std::move(seq));
    return !sequence_.cues.empty();
}

void SculptureTimelineSequencer::play() {
    if (sequence_.cues.empty()) return;
    isPlaying_ = true;
    isPaused_ = false;
}

void SculptureTimelineSequencer::pause() noexcept {
    isPaused_ = true;
}

void SculptureTimelineSequencer::resume() noexcept {
    if (isPlaying_) {
        isPaused_ = false;
    } else {
        play();
    }
}

void SculptureTimelineSequencer::stop() noexcept {
    isPlaying_ = false;
    isPaused_ = false;
    stage_ = TimelineStage::Idle;
    activeCueIndex_ = 0U;
    nextCueIndex_ = sequence_.cues.size() > 1U ? 1U : 0U;
    cueCountdown_ = 0.0f;
    totalElapsedTime_ = 0.0f;
    cuesStaged_ = false;
    liveDeck_ = DeckId::DeckA;
}

void SculptureTimelineSequencer::togglePlayPause() {
    if (!isPlaying_) {
        play();
    } else {
        isPaused_ = !isPaused_;
    }
}

void SculptureTimelineSequencer::stageCueToDeck(
    DeckId targetDeck,
    const TimelineCue& cue,
    SculpturePerformanceDeck& deck,
    const SculpturePresetLibrary* library
) {
    if (library != nullptr) {
        const SculpturePresetAsset* asset = library->find(cue.presetIdOrName);
        if (asset != nullptr) {
            deck.loadDeck(targetDeck, *asset);
            return;
        }
    }
    // Fallback if library not available or asset not found: retain current parameters and set title
    SculptureProgramParams params = deck.deck(targetDeck).params;
    deck.loadDeck(targetDeck, params, cue.presetIdOrName, cue.title);
}

void SculptureTimelineSequencer::stageCurrentAndNextCues(
    SculpturePerformanceDeck& deck,
    const SculpturePresetLibrary* library
) {
    if (sequence_.cues.empty()) return;

    // Active cue to live deck (Deck A by default)
    stageCueToDeck(liveDeck_, sequence_.cues[activeCueIndex_], deck, library);
    deck.setCrossfadePosition(liveDeck_ == DeckId::DeckA ? 0.0f : 1.0f);

    // Stage next cue onto inactive deck
    if (sequence_.cues.size() > 1U) {
        const DeckId inactiveDeck = liveDeck_ == DeckId::DeckA ? DeckId::DeckB : DeckId::DeckA;
        stageCueToDeck(inactiveDeck, sequence_.cues[nextCueIndex_], deck, library);
    }
    cuesStaged_ = true;
}

void SculptureTimelineSequencer::skipToCue(
    std::size_t cueIndex,
    SculpturePerformanceDeck& deck,
    const SculpturePresetLibrary* library
) {
    if (sequence_.cues.empty()) return;
    activeCueIndex_ = cueIndex % sequence_.cues.size();
    nextCueIndex_ = (activeCueIndex_ + 1U) % sequence_.cues.size();
    cuesStaged_ = false;
    stageCurrentAndNextCues(deck, library);
    stage_ = TimelineStage::Dwelling;
    cueCountdown_ = sequence_.cues[activeCueIndex_].dwellSeconds;
}

void SculptureTimelineSequencer::nextCue(
    SculpturePerformanceDeck& deck,
    const SculpturePresetLibrary* library
) {
    if (sequence_.cues.empty()) return;
    skipToCue(activeCueIndex_ + 1U, deck, library);
}

void SculptureTimelineSequencer::previousCue(
    SculpturePerformanceDeck& deck,
    const SculpturePresetLibrary* library
) {
    if (sequence_.cues.empty()) return;
    const std::size_t prev = activeCueIndex_ > 0 ? activeCueIndex_ - 1U : sequence_.cues.size() - 1U;
    skipToCue(prev, deck, library);
}

void SculptureTimelineSequencer::update(
    float deltaSeconds,
    SculpturePerformanceDeck& deck,
    const SculpturePresetLibrary* library
) {
    if (!isPlaying_ || isPaused_ || sequence_.cues.empty()) {
        return;
    }

    // Initial staging on first playback tick
    if (!cuesStaged_) {
        stageCurrentAndNextCues(deck, library);
        stage_ = TimelineStage::Dwelling;
        cueCountdown_ = sequence_.cues[activeCueIndex_].dwellSeconds;
    }

    totalElapsedTime_ += deltaSeconds;

    if (stage_ == TimelineStage::Dwelling) {
        cueCountdown_ -= deltaSeconds;
        if (cueCountdown_ <= 0.0f) {
            // Check if there is a next cue to transition to
            if (activeCueIndex_ + 1U >= sequence_.cues.size() && !sequence_.loop) {
                stage_ = TimelineStage::Finished;
                isPlaying_ = false;
                return;
            }

            // Dwell expired; begin crossfading to opposite deck
            if (sequence_.cues.size() == 1U) {
                if (sequence_.loop) {
                    cueCountdown_ = sequence_.cues[activeCueIndex_].dwellSeconds;
                } else {
                    stage_ = TimelineStage::Finished;
                    isPlaying_ = false;
                }
                return;
            }

            const DeckId targetDeck = (liveDeck_ == DeckId::DeckA) ? DeckId::DeckB : DeckId::DeckA;
            const auto& activeCue = sequence_.cues[activeCueIndex_];
            deck.setCrossfadeCurve(activeCue.curve);
            deck.triggerAutoCrossfade(activeCue.transitionSeconds, targetDeck);
            stage_ = TimelineStage::Transitioning;
        }
    } else if (stage_ == TimelineStage::Transitioning) {
        if (!deck.isAutoCrossfading()) {
            // Crossfade completed! The formerly inactive deck is now live
            liveDeck_ = (liveDeck_ == DeckId::DeckA) ? DeckId::DeckB : DeckId::DeckA;
            activeCueIndex_ = nextCueIndex_;

            // Determine subsequent cue index
            std::size_t upcoming = activeCueIndex_ + 1U;
            if (upcoming >= sequence_.cues.size()) {
                if (sequence_.loop) {
                    upcoming = 0U;
                } else {
                    upcoming = activeCueIndex_; // No more upcoming cues
                }
            }
            nextCueIndex_ = upcoming;

            // Stage upcoming cue onto the new inactive deck to trigger pre-warming
            if (upcoming != activeCueIndex_) {
                const DeckId newInactiveDeck = (liveDeck_ == DeckId::DeckA) ? DeckId::DeckB : DeckId::DeckA;
                stageCueToDeck(newInactiveDeck, sequence_.cues[nextCueIndex_], deck, library);
            }

            stage_ = TimelineStage::Dwelling;
            cueCountdown_ = sequence_.cues[activeCueIndex_].dwellSeconds;
        }
    }
}

TimelinePlayheadState SculptureTimelineSequencer::playheadState() const noexcept {
    TimelinePlayheadState state;
    state.isPlaying = isPlaying_;
    state.isPaused = isPaused_;
    state.activeCueIndex = activeCueIndex_;
    state.nextCueIndex = nextCueIndex_;
    state.stage = stage_;
    state.cueTimeRemaining = std::max(cueCountdown_, 0.0f);
    state.totalElapsedTime = totalElapsedTime_;
    state.totalDuration = sequence_.totalDuration();
    state.liveDeck = liveDeck_;
    if (activeCueIndex_ < sequence_.cues.size()) {
        state.activePresetTitle = sequence_.cues[activeCueIndex_].title;
    }
    if (nextCueIndex_ < sequence_.cues.size()) {
        state.nextPresetTitle = sequence_.cues[nextCueIndex_].title;
    }
    return state;
}

std::string SculptureTimelineSequencer::formatHudString() const {
    if (!isPlaying_) {
        return "[TIMELINE ■ Stopped]";
    }
    if (isPaused_) {
        return "[TIMELINE ❚❚ Paused]";
    }
    if (stage_ == TimelineStage::Finished) {
        return "[TIMELINE ✔ Complete]";
    }

    char buf[256];
    const std::size_t totalCues = sequence_.cues.size();
    const std::string_view liveDeckStr = (liveDeck_ == DeckId::DeckA) ? "Deck A" : "Deck B";
    const std::string_view targetDeckStr = (liveDeck_ == DeckId::DeckA) ? "Deck B" : "Deck A";

    if (stage_ == TimelineStage::Dwelling) {
        std::snprintf(buf, sizeof(buf),
            "[TIMELINE ▶ Cue %zu/%zu \"%s\" | Dwell: %.1fs | %s LIVE]",
            activeCueIndex_ + 1U, totalCues,
            activeCueIndex_ < totalCues ? sequence_.cues[activeCueIndex_].title.c_str() : "",
            cueCountdown_, liveDeckStr.data());
    } else {
        const auto& cue = sequence_.cues[activeCueIndex_];
        std::snprintf(buf, sizeof(buf),
            "[TIMELINE ⧗ Crossfading -> %s (%s, %.1fs)]",
            targetDeckStr.data(), curveName(cue.curve).data(), cue.transitionSeconds);
    }
    return std::string(buf);
}

std::string SculptureTimelineSequencer::formatSummary() const {
    std::ostringstream ss;
    ss << "Timeline: " << sequence_.name << " (" << sequence_.cues.size() << " cues, "
       << std::fixed << std::setprecision(1) << sequence_.totalDuration() << "s total, loop: "
       << (sequence_.loop ? "YES" : "NO") << ")\n";
    for (std::size_t i = 0; i < sequence_.cues.size(); ++i) {
        const auto& c = sequence_.cues[i];
        ss << "  [" << (i + 1) << "] " << c.title << " (" << c.presetIdOrName << ") "
           << "dwell=" << c.dwellSeconds << "s, fade=" << c.transitionSeconds << "s ("
           << curveName(c.curve) << ")\n";
    }
    return ss.str();
}

} // namespace epi_sculpt
