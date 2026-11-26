#include "SculptureTimelineDirector.h"
#include "SculptureAudioFeedDirector.h"

#include <algorithm>
#include <cstdio>
#include <cmath>
#include <fstream>
#include <sstream>
#include <iomanip>

namespace epi_sculpt {

namespace {

[[nodiscard]] NavigatorAction parseActionName(std::string_view name) noexcept {
    if (name == "NextWorld") return NavigatorAction::NextWorld;
    if (name == "PrevWorld") return NavigatorAction::PrevWorld;
    if (name == "ToggleTour") return NavigatorAction::ToggleTour;
    if (name == "CycleTour") return NavigatorAction::CycleTour;
    if (name == "StepTourNext") return NavigatorAction::StepTourNext;
    if (name == "StepTourPrev") return NavigatorAction::StepTourPrev;
    if (name == "Mutate") return NavigatorAction::Mutate;
    if (name == "RandomizePalette") return NavigatorAction::RandomizePalette;
    if (name == "ResetParameters") return NavigatorAction::ResetParameters;
    if (name == "CycleInnerMap") return NavigatorAction::CycleInnerMap;
    if (name == "CycleReduction") return NavigatorAction::CycleReduction;
    if (name == "CycleMirrorTopology") return NavigatorAction::CycleMirrorTopology;
    if (name == "SetMirrorTopology") return NavigatorAction::SetMirrorTopology;
    if (name == "ResetCamera") return NavigatorAction::ResetCamera;
    if (name == "CyclePresentationMode") return NavigatorAction::CyclePresentationMode;
    if (name == "CycleReliefProjection") return NavigatorAction::CycleReliefProjection;
    if (name == "CycleResolutionScale") return NavigatorAction::CycleResolutionScale;
    if (name == "ToggleStagedGovernor") return NavigatorAction::ToggleStagedGovernor;
    if (name == "Toggle4DFoliationWave") return NavigatorAction::Toggle4DFoliationWave;
    if (name == "Cycle4DManifold") return NavigatorAction::Cycle4DManifold;
    if (name == "CyclePolychoronSource") return NavigatorAction::CyclePolychoronSource;
    if (name == "CyclePolychoronTarget") return NavigatorAction::CyclePolychoronTarget;
    if (name == "StepPolychoronMorph") return NavigatorAction::StepPolychoronMorph;
    if (name == "CycleRotorSpeed") return NavigatorAction::CycleRotorSpeed;
    if (name == "TogglePaletteAgent") return NavigatorAction::TogglePaletteAgent;
    if (name == "CyclePaletteAgentMode") return NavigatorAction::CyclePaletteAgentMode;
    if (name == "CycleLearnStage") return NavigatorAction::CycleLearnStage;
    if (name == "ResetLearnMode") return NavigatorAction::ResetLearnMode;
    if (name == "CycleDiagnostic") return NavigatorAction::CycleDiagnostic;
    if (name == "IncreaseSpeed") return NavigatorAction::IncreaseSpeed;
    if (name == "DecreaseSpeed") return NavigatorAction::DecreaseSpeed;
    if (name == "ResetSpeed") return NavigatorAction::ResetSpeed;
    if (name == "TogglePause") return NavigatorAction::TogglePause;
    if (name == "AdjustLatentW") return NavigatorAction::AdjustLatentW;
    if (name == "AdjustRotorAlpha") return NavigatorAction::AdjustRotorAlpha;
    if (name == "AdjustRotorBeta") return NavigatorAction::AdjustRotorBeta;
    if (name == "Reset4DRotors") return NavigatorAction::Reset4DRotors;
    if (name == "ToggleSequenceRecording") return NavigatorAction::ToggleSequenceRecording;
    if (name == "RequestSnapshot") return NavigatorAction::RequestSnapshot;
    if (name == "ReloadLatestSnapshot") return NavigatorAction::ReloadLatestSnapshot;
    if (name == "ToggleTimeline") return NavigatorAction::ToggleTimeline;
    if (name == "TogglePerfMonitor") return NavigatorAction::TogglePerfMonitor;
    if (name == "CycleTargetFps") return NavigatorAction::CycleTargetFps;
    if (name == "ResetPerfStats") return NavigatorAction::ResetPerfStats;
    if (name == "RevertToBaseline") return NavigatorAction::RevertToBaseline;
    if (name == "ExportDiffPatch") return NavigatorAction::ExportDiffPatch;
    if (name == "ToggleAtmosphere") return NavigatorAction::ToggleAtmosphere;
    if (name == "CycleAudioFeedMode") return NavigatorAction::CycleAudioFeedMode;
    if (name == "AdjustAudioBpm") return NavigatorAction::AdjustAudioBpm;
    if (name == "ToggleAudioFeed") return NavigatorAction::ToggleAudioFeed;
    if (name == "BuildBeatSyncedTimeline") return NavigatorAction::BuildBeatSyncedTimeline;
    if (name == "ExportTimelineScript") return NavigatorAction::ExportTimelineScript;
    if (name == "TogglePerformanceDeck") return NavigatorAction::TogglePerformanceDeck;
    if (name == "CrossfadeDeck") return NavigatorAction::CrossfadeDeck;
    if (name == "TriggerAutoCrossfade") return NavigatorAction::TriggerAutoCrossfade;
    if (name == "SwapPerformanceDecks") return NavigatorAction::SwapPerformanceDecks;
    if (name == "LoadDeckA") return NavigatorAction::LoadDeckA;
    if (name == "LoadDeckB") return NavigatorAction::LoadDeckB;
    if (name == "SetVariableResolutionScale") return NavigatorAction::SetVariableResolutionScale;
    if (name == "ToggleAutoResolutionScaling") return NavigatorAction::ToggleAutoResolutionScaling;
    if (name == "CycleVolumetricResolution") return NavigatorAction::CycleVolumetricResolution;
    if (name == "ToggleVolumetricFeedback") return NavigatorAction::ToggleVolumetricFeedback;
    if (name == "ResetVolumetricMemory") return NavigatorAction::ResetVolumetricMemory;
    if (name == "ToggleVolumetricWrap") return NavigatorAction::ToggleVolumetricWrap;
    if (name == "ToggleVolumetricBackend") return NavigatorAction::ToggleVolumetricBackend;
    if (name == "ToggleVolumetricCoupling") return NavigatorAction::ToggleVolumetricCoupling;
    if (name == "ToggleVolumetricAdvection") return NavigatorAction::ToggleVolumetricAdvection;
    if (name == "ToggleAcousticSynesthesia") return NavigatorAction::ToggleAcousticSynesthesia;
    if (name == "PlayCategoryTour") return NavigatorAction::PlayCategoryTour;
    if (name == "CycleCategoryTour") return NavigatorAction::CycleCategoryTour;
    if (name == "Undo") return NavigatorAction::Undo;
    if (name == "Redo") return NavigatorAction::Redo;
    if (name == "ToggleMorphSpace") return NavigatorAction::ToggleMorphSpace;
    if (name == "CycleMorphSpaceMotion") return NavigatorAction::CycleMorphSpaceMotion;
    if (name == "SetMorphSpaceX") return NavigatorAction::SetMorphSpaceX;
    if (name == "SetMorphSpaceY") return NavigatorAction::SetMorphSpaceY;
    if (name == "NudgeMorphSpaceX") return NavigatorAction::NudgeMorphSpaceX;
    if (name == "NudgeMorphSpaceY") return NavigatorAction::NudgeMorphSpaceY;
    if (name == "CycleMorphSpaceBank") return NavigatorAction::CycleMorphSpaceBank;
    if (name == "ToggleMorphSpacePath") return NavigatorAction::ToggleMorphSpacePath;
    if (name == "SoloStage1") return NavigatorAction::SoloStage1;
    if (name == "SoloStage2") return NavigatorAction::SoloStage2;
    if (name == "SoloStage3") return NavigatorAction::SoloStage3;
    if (name == "SoloStage4") return NavigatorAction::SoloStage4;
    if (name == "SoloStage5") return NavigatorAction::SoloStage5;
    if (name == "SoloStage6") return NavigatorAction::SoloStage6;
    if (name == "ResetLearnStages") return NavigatorAction::ResetLearnStages;
    if (name == "SoloStage") return NavigatorAction::SoloStage;
    if (name == "SetLearnAccumulative") return NavigatorAction::SetLearnAccumulative;
    if (name == "SyncMorphSpaceCategory" || name == "SyncMorphCategory") return NavigatorAction::SyncMorphSpaceCategory;
    if (name == "BindMorphSpaceCategory" || name == "BindMorphCategory") return NavigatorAction::BindMorphSpaceCategory;
    if (name == "ReloadPresetsAndKernel") return NavigatorAction::ReloadPresetsAndKernel;
    return NavigatorAction::NextWorld;
}

[[nodiscard]] EasingType parseEasingName(std::string_view name) noexcept {
    if (name == "Linear") return EasingType::Linear;
    if (name == "Smoothstep" || name == "EqualPower") return EasingType::Smoothstep;
    if (name == "CubicInOut") return EasingType::CubicInOut;
    if (name == "ElasticOut") return EasingType::ElasticOut;
    if (name == "BounceOut") return EasingType::BounceOut;
    return EasingType::Smoothstep;
}

[[nodiscard]] constexpr std::string_view easingName(EasingType easing) noexcept {
    switch (easing) {
    case EasingType::Linear:     return "Linear";
    case EasingType::Smoothstep: return "Smoothstep";
    case EasingType::CubicInOut: return "CubicInOut";
    case EasingType::ElasticOut: return "ElasticOut";
    case EasingType::BounceOut:  return "BounceOut";
    default:                     return "Smoothstep";
    }
}

} // namespace

float TimelineChannelTrack::evaluate(float t) const noexcept {
    if (keyframes.empty()) return 0.0f;
    if (keyframes.size() == 1U || t <= keyframes.front().timeSeconds) {
        return keyframes.front().value;
    }
    if (t >= keyframes.back().timeSeconds) {
        return keyframes.back().value;
    }

    for (std::size_t i = 0; i + 1 < keyframes.size(); ++i) {
        const auto& k0 = keyframes[i];
        const auto& k1 = keyframes[i + 1];
        if (t >= k0.timeSeconds && t <= k1.timeSeconds) {
            const float dt = k1.timeSeconds - k0.timeSeconds;
            const float u = (dt > 1e-5f) ? (t - k0.timeSeconds) / dt : 0.0f;
            const float eased_u = Easing::evaluate(k0.easing, u);
            return k0.value + (k1.value - k0.value) * eased_u;
        }
    }
    return keyframes.back().value;
}

void WorldTimelineScript::sortCues() noexcept {
    std::stable_sort(actionCues.begin(), actionCues.end(), [](const TimelineActionCue& a, const TimelineActionCue& b) {
        return a.timestampSeconds < b.timestampSeconds;
    });
}

SculptureTimelineDirector::SculptureTimelineDirector(WorldTimelineScript script) {
    loadScript(std::move(script));
}

void SculptureTimelineDirector::loadScript(WorldTimelineScript script) {
    script_ = std::move(script);
    script_.sortCues();
    stop();
}

bool SculptureTimelineDirector::loadScriptFromFile(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::fprintf(stderr, "[SculptureTimelineDirector] Error: Failed to open timeline file '%s'\n", path.string().c_str());
        return false;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return loadScriptFromJson(buffer.str());
}

bool SculptureTimelineDirector::loadScriptFromJson(std::string_view jsonStr) {
    WorldTimelineScript script;

    const auto extractStringField = [&](std::string_view key) -> std::optional<std::string> {
        const std::string needle = "\"" + std::string(key) + "\"";
        const std::size_t pos = jsonStr.find(needle);
        if (pos == std::string_view::npos) return std::nullopt;
        const std::size_t colon = jsonStr.find(':', pos + needle.size());
        if (colon == std::string_view::npos) return std::nullopt;
        const std::size_t openQuote = jsonStr.find('"', colon + 1);
        if (openQuote == std::string_view::npos) return std::nullopt;
        const std::size_t closeQuote = jsonStr.find('"', openQuote + 1);
        if (closeQuote == std::string_view::npos) return std::nullopt;
        return std::string(jsonStr.substr(openQuote + 1, closeQuote - openQuote - 1));
    };

    const auto extractFloatField = [&](std::string_view key) -> std::optional<float> {
        const std::string needle = "\"" + std::string(key) + "\"";
        const std::size_t pos = jsonStr.find(needle);
        if (pos == std::string_view::npos) return std::nullopt;
        const std::size_t colon = jsonStr.find(':', pos + needle.size());
        if (colon == std::string_view::npos) return std::nullopt;
        std::size_t start = colon + 1;
        while (start < jsonStr.size() && std::isspace(static_cast<unsigned char>(jsonStr[start]))) ++start;
        if (start >= jsonStr.size()) return std::nullopt;
        std::size_t end = start;
        while (end < jsonStr.size() && jsonStr[end] != ',' && jsonStr[end] != '}' && !std::isspace(static_cast<unsigned char>(jsonStr[end]))) ++end;
        const std::string token(jsonStr.substr(start, end - start));
        char* err = nullptr;
        float val = std::strtof(token.c_str(), &err);
        if (err == token.c_str()) return std::nullopt;
        return val;
    };

    const auto extractBoolField = [&](std::string_view key) -> std::optional<bool> {
        const std::string needle = "\"" + std::string(key) + "\"";
        const std::size_t pos = jsonStr.find(needle);
        if (pos == std::string_view::npos) return std::nullopt;
        const std::size_t colon = jsonStr.find(':', pos + needle.size());
        if (colon == std::string_view::npos) return std::nullopt;
        const std::size_t tPos = jsonStr.find("true", colon);
        const std::size_t fPos = jsonStr.find("false", colon);
        const std::size_t comma = jsonStr.find(',', colon);
        const std::size_t brace = jsonStr.find('}', colon);
        const std::size_t limit = std::min(comma, brace);
        if (tPos != std::string_view::npos && (limit == std::string_view::npos || tPos < limit)) return true;
        if (fPos != std::string_view::npos && (limit == std::string_view::npos || fPos < limit)) return false;
        return std::nullopt;
    };

    if (auto id = extractStringField("id")) script.id = *id;
    if (auto title = extractStringField("title")) script.title = *title;
    if (auto desc = extractStringField("description")) script.description = *desc;
    bool explicitDuration = false;
    if (auto dur = extractFloatField("duration_seconds")) { script.durationSeconds = *dur; explicitDuration = true; }
    else if (auto dur2 = extractFloatField("duration")) { script.durationSeconds = *dur2; explicitDuration = true; }
    else if (auto dur3 = extractFloatField("duration_sec")) { script.durationSeconds = *dur3; explicitDuration = true; }
    if (auto loop = extractBoolField("loop")) script.loop = *loop;

    // 1. Parse discrete action cues array
    std::size_t cuesPos = jsonStr.find("\"cues\"");
    if (cuesPos == std::string_view::npos) cuesPos = jsonStr.find("\"action_cues\"");
    if (cuesPos == std::string_view::npos) cuesPos = jsonStr.find("\"actions\"");
    if (cuesPos == std::string_view::npos) cuesPos = jsonStr.find("\"sequence\"");

    if (cuesPos != std::string_view::npos) {
        const std::size_t openBracket = jsonStr.find('[', cuesPos);
        if (openBracket != std::string_view::npos) {
            std::size_t cur = openBracket + 1;
            float accumulatedTime = 0.0f;
            while (cur < jsonStr.size()) {
                const std::size_t objStart = jsonStr.find('{', cur);
                if (objStart == std::string_view::npos) break;
                const std::size_t objEnd = jsonStr.find('}', objStart);
                if (objEnd == std::string_view::npos) break;

                std::string_view cueJson = jsonStr.substr(objStart, objEnd - objStart + 1);
                TimelineActionCue cue;

                const auto getField = [&](std::string_view key) -> std::optional<std::string> {
                    const std::string needle = "\"" + std::string(key) + "\"";
                    const std::size_t f = cueJson.find(needle);
                    if (f == std::string_view::npos) return std::nullopt;
                    const std::size_t col = cueJson.find(':', f + needle.size());
                    if (col == std::string_view::npos) return std::nullopt;
                    std::size_t s = col + 1;
                    while (s < cueJson.size() && std::isspace(static_cast<unsigned char>(cueJson[s]))) ++s;
                    if (s >= cueJson.size()) return std::nullopt;
                    if (cueJson[s] == '"') {
                        const std::size_t e = cueJson.find('"', s + 1);
                        if (e == std::string_view::npos) return std::nullopt;
                        return std::string(cueJson.substr(s + 1, e - s - 1));
                    }
                    std::size_t e = s;
                    while (e < cueJson.size() && cueJson[e] != ',' && cueJson[e] != '}' && !std::isspace(static_cast<unsigned char>(cueJson[e]))) ++e;
                    return std::string(cueJson.substr(s, e - s));
                };

                if (const auto t = getField("time_sec")) cue.timestampSeconds = std::strtof(t->c_str(), nullptr);
                else if (const auto t2 = getField("time")) cue.timestampSeconds = std::strtof(t2->c_str(), nullptr);
                else if (const auto t3 = getField("timestamp_sec")) cue.timestampSeconds = std::strtof(t3->c_str(), nullptr);
                else if (const auto t4 = getField("timestampSec")) cue.timestampSeconds = std::strtof(t4->c_str(), nullptr);
                else if (const auto t5 = getField("timestamp")) cue.timestampSeconds = std::strtof(t5->c_str(), nullptr);
                else {
                    cue.timestampSeconds = accumulatedTime;
                }

                if (const auto a = getField("action")) cue.action = parseActionName(*a);
                else if (getField("preset").has_value()) {
                    cue.action = NavigatorAction::NextWorld;
                }

                if (const auto v = getField("value")) cue.value = std::strtof(v->c_str(), nullptr);
                else if (const auto v2 = getField("param")) cue.value = std::strtof(v2->c_str(), nullptr);

                if (const auto d = getField("description")) cue.description = *d;
                else if (const auto t = getField("title")) cue.description = *t;

                float dwell = 4.0f;
                float trans = 2.0f;
                if (const auto d = getField("dwell_sec")) dwell = std::strtof(d->c_str(), nullptr);
                if (const auto tr = getField("transition_sec")) trans = std::strtof(tr->c_str(), nullptr);
                accumulatedTime += (dwell + trans);

                script.actionCues.push_back(cue);
                cur = objEnd + 1;
                const std::size_t nextClose = jsonStr.find(']', cur);
                const std::size_t nextObj = jsonStr.find('{', cur);
                if (nextClose != std::string_view::npos && (nextObj == std::string_view::npos || nextClose < nextObj)) {
                    break;
                }
            }
            if (!explicitDuration) {
                if (script.durationSeconds <= 0.0f || script.durationSeconds < accumulatedTime) {
                    script.durationSeconds = accumulatedTime;
                }
            }
        }
    }

    // 2. Parse continuous tracks array
    const std::size_t tracksPos = jsonStr.find("\"tracks\"");
    if (tracksPos != std::string_view::npos) {
        const std::size_t openBracket = jsonStr.find('[', tracksPos);
        if (openBracket != std::string_view::npos) {
            std::size_t cur = openBracket + 1;
            while (cur < jsonStr.size()) {
                const std::size_t objStart = jsonStr.find('{', cur);
                if (objStart == std::string_view::npos) break;

                int depth = 0;
                std::size_t objEnd = objStart;
                for (; objEnd < jsonStr.size(); ++objEnd) {
                    if (jsonStr[objEnd] == '{') ++depth;
                    else if (jsonStr[objEnd] == '}') {
                        --depth;
                        if (depth == 0) break;
                    }
                }
                if (objEnd >= jsonStr.size()) break;

                std::string_view trackJson = jsonStr.substr(objStart, objEnd - objStart + 1);
                TimelineChannelTrack track;

                const auto getField = [&](std::string_view key) -> std::optional<std::string> {
                    const std::string needle = "\"" + std::string(key) + "\"";
                    const std::size_t f = trackJson.find(needle);
                    if (f == std::string_view::npos) return std::nullopt;
                    const std::size_t col = trackJson.find(':', f + needle.size());
                    if (col == std::string_view::npos) return std::nullopt;
                    std::size_t s = col + 1;
                    while (s < trackJson.size() && std::isspace(static_cast<unsigned char>(trackJson[s]))) ++s;
                    if (s >= trackJson.size()) return std::nullopt;
                    if (trackJson[s] == '"') {
                        const std::size_t e = trackJson.find('"', s + 1);
                        if (e == std::string_view::npos) return std::nullopt;
                        return std::string(trackJson.substr(s + 1, e - s - 1));
                    }
                    std::size_t e = s;
                    while (e < trackJson.size() && trackJson[e] != ',' && trackJson[e] != '}' && !std::isspace(static_cast<unsigned char>(trackJson[e]))) ++e;
                    return std::string(trackJson.substr(s, e - s));
                };

                if (const auto ch = getField("channel")) track.channelName = *ch;
                else if (const auto ch2 = getField("channelName")) track.channelName = *ch2;
                else if (const auto ch3 = getField("name")) track.channelName = *ch3;

                const std::size_t kfPos = trackJson.find("\"keyframes\"");
                if (kfPos != std::string_view::npos) {
                    const std::size_t kfOpen = trackJson.find('[', kfPos);
                    if (kfOpen != std::string_view::npos) {
                        std::size_t kfCur = kfOpen + 1;
                        while (kfCur < trackJson.size()) {
                            const std::size_t kfStart = trackJson.find('{', kfCur);
                            if (kfStart == std::string_view::npos) break;

                            int kfDepth = 0;
                            std::size_t kfEnd = kfStart;
                            for (; kfEnd < trackJson.size(); ++kfEnd) {
                                if (trackJson[kfEnd] == '{') ++kfDepth;
                                else if (trackJson[kfEnd] == '}') {
                                    --kfDepth;
                                    if (kfDepth == 0) break;
                                }
                            }
                            if (kfEnd >= trackJson.size()) break;

                            std::string_view kfJson = trackJson.substr(kfStart, kfEnd - kfStart + 1);
                            TimelineFloatKeyframe kf;

                            const auto getKfField = [&](std::string_view key) -> std::optional<std::string> {
                                const std::string needle = "\"" + std::string(key) + "\"";
                                const std::size_t f = kfJson.find(needle);
                                if (f == std::string_view::npos) return std::nullopt;
                                const std::size_t col = kfJson.find(':', f + needle.size());
                                if (col == std::string_view::npos) return std::nullopt;
                                std::size_t s = col + 1;
                                while (s < kfJson.size() && std::isspace(static_cast<unsigned char>(kfJson[s]))) ++s;
                                if (s >= kfJson.size()) return std::nullopt;
                                if (kfJson[s] == '"') {
                                    const std::size_t e = kfJson.find('"', s + 1);
                                    if (e == std::string_view::npos) return std::nullopt;
                                    return std::string(kfJson.substr(s + 1, e - s - 1));
                                }
                                std::size_t e = s;
                                while (e < kfJson.size() && kfJson[e] != ',' && kfJson[e] != '}' && !std::isspace(static_cast<unsigned char>(kfJson[e]))) ++e;
                                return std::string(kfJson.substr(s, e - s));
                            };

                            if (const auto t = getKfField("time_sec")) kf.timeSeconds = std::strtof(t->c_str(), nullptr);
                            else if (const auto t2 = getKfField("time")) kf.timeSeconds = std::strtof(t2->c_str(), nullptr);
                            if (const auto v = getKfField("value")) kf.value = std::strtof(v->c_str(), nullptr);
                            if (const auto e = getKfField("easing")) kf.easing = parseEasingName(*e);

                            track.keyframes.push_back(kf);
                            kfCur = kfEnd + 1;
                            const std::size_t closeKf = trackJson.find(']', kfCur);
                            const std::size_t nextKf = trackJson.find('{', kfCur);
                            if (closeKf != std::string_view::npos && (nextKf == std::string_view::npos || closeKf < nextKf)) {
                                break;
                            }
                        }
                    }
                }

                if (!track.channelName.empty() && !track.keyframes.empty()) {
                    script.tracks.push_back(std::move(track));
                }

                cur = objEnd + 1;
                const std::size_t nextClose = jsonStr.find(']', cur);
                const std::size_t nextObj = jsonStr.find('{', cur);
                if (nextClose != std::string_view::npos && (nextObj == std::string_view::npos || nextClose < nextObj)) {
                    break;
                }
            }
        }
    }

    loadScript(std::move(script));
    return true;
}

std::string SculptureTimelineDirector::serializeToJson() const {
    return serializeScriptToJson(script_);
}

std::string SculptureTimelineDirector::serializeScriptToJson(const WorldTimelineScript& script) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(3);
    ss << "{\n";
    ss << "  \"id\": \"" << script.id << "\",\n";
    ss << "  \"title\": \"" << script.title << "\",\n";
    ss << "  \"description\": \"" << script.description << "\",\n";
    ss << "  \"duration_seconds\": " << script.durationSeconds << ",\n";
    ss << "  \"loop\": " << (script.loop ? "true" : "false") << ",\n";

    // 1. Action Cues
    ss << "  \"cues\": [\n";
    for (std::size_t i = 0; i < script.actionCues.size(); ++i) {
        const auto& c = script.actionCues[i];
        ss << "    {\n";
        ss << "      \"time_sec\": " << c.timestampSeconds << ",\n";
        ss << "      \"action\": \"" << actionName(c.action) << "\",\n";
        ss << "      \"value\": " << c.value << ",\n";
        ss << "      \"description\": \"" << c.description << "\"\n";
        ss << "    }" << (i + 1 < script.actionCues.size() ? "," : "") << "\n";
    }
    ss << "  ],\n";

    // 2. Continuous Parameter Tracks
    ss << "  \"tracks\": [\n";
    for (std::size_t i = 0; i < script.tracks.size(); ++i) {
        const auto& t = script.tracks[i];
        ss << "    {\n";
        ss << "      \"channel\": \"" << t.channelName << "\",\n";
        ss << "      \"keyframes\": [\n";
        for (std::size_t k = 0; k < t.keyframes.size(); ++k) {
            const auto& kf = t.keyframes[k];
            ss << "        {\n";
            ss << "          \"time_sec\": " << kf.timeSeconds << ",\n";
            ss << "          \"value\": " << kf.value << ",\n";
            ss << "          \"easing\": \"" << easingName(kf.easing) << "\"\n";
            ss << "        }" << (k + 1 < t.keyframes.size() ? "," : "") << "\n";
        }
        ss << "      ]\n";
        ss << "    }" << (i + 1 < script.tracks.size() ? "," : "") << "\n";
    }
    ss << "  ]\n";
    ss << "}\n";
    return ss.str();
}

bool SculptureTimelineDirector::saveScriptToFile(const std::filesystem::path& path) const {
    return saveScriptToFile(path, script_);
}

bool SculptureTimelineDirector::saveScriptToFile(const std::filesystem::path& path, const WorldTimelineScript& script) {
    if (path.has_parent_path()) {
        std::error_code ec;
        std::filesystem::create_directories(path.parent_path(), ec);
    }
    std::ofstream file(path);
    if (!file.is_open()) {
        std::fprintf(stderr, "[SculptureTimelineDirector] Error: Failed to write timeline to '%s'\n", path.string().c_str());
        return false;
    }
    file << serializeScriptToJson(script);
    return true;
}

WorldTimelineScript SculptureTimelineDirector::generateBeatSynchronizedScript(
    const std::vector<std::string>& presetNames,
    float bpm,
    std::uint32_t barsPerWorld,
    bool loop
) {
    WorldTimelineScript s{};
    s.id = "procedural_beat_synced";
    s.title = "Procedural Beat-Synchronized Odyssey";
    s.description = "Auto-generated mathematical foliation timeline synchronized to musical bar clock";
    s.loop = loop;

    const float effectiveBpm = std::clamp(bpm, 40.0f, 240.0f);
    const float beatSec = 60.0f / effectiveBpm;
    const float barSec = beatSec * 4.0f;
    const float worldDuration = barSec * std::max(barsPerWorld, 2U);

    const std::size_t worldCount = std::max<std::size_t>(presetNames.size(), 4U);
    s.durationSeconds = static_cast<float>(worldCount) * worldDuration;

    // 1. Action cues: world switches, mutations, and palette pulses on musical downbeats
    float t = 0.0f;
    for (std::size_t i = 0; i < worldCount; ++i) {
        std::string nameDesc = (i < presetNames.size()) ? presetNames[i] : ("World #" + std::to_string(i + 1));

        // Downbeat transition to world
        s.actionCues.push_back({
            .timestampSeconds = t,
            .action = NavigatorAction::NextWorld,
            .value = 0.0f,
            .description = "Phrase Downbeat: " + nameDesc
        });

        // Mid-phrase harmonic mutation at midpoint bar
        const float midBar = t + (worldDuration * 0.5f);
        s.actionCues.push_back({
            .timestampSeconds = midBar,
            .action = NavigatorAction::Mutate,
            .value = 0.12f,
            .description = "Mid-Phrase Geometric Mutation"
        });

        // 1 bar before drop: dynamic palette burst
        const float preDrop = t + worldDuration - barSec;
        if (preDrop > t) {
            s.actionCues.push_back({
                .timestampSeconds = preDrop,
                .action = NavigatorAction::RandomizePalette,
                .value = 0.0f,
                .description = "Pre-Transition Palette Excitation"
            });
        }

        t += worldDuration;
    }

    // 2. Continuous Tracks: 4D Spacetime & SO(4) Rotors
    TimelineChannelTrack trackW;
    trackW.channelName = "latentW";

    TimelineChannelTrack trackAlpha;
    trackAlpha.channelName = "rotorAlpha";

    TimelineChannelTrack trackBeta;
    trackBeta.channelName = "rotorBeta";

    // 3. Continuous Tracks: Spherical Cinematic Camera
    TimelineChannelTrack trackCamDist;
    trackCamDist.channelName = "cameraDistance";

    TimelineChannelTrack trackCamAz;
    trackCamAz.channelName = "cameraAzimuth";

    TimelineChannelTrack trackCamEl;
    trackCamEl.channelName = "cameraElevation";

    const std::size_t totalKeyframes = worldCount * 4U + 1U;
    for (std::size_t k = 0; k < totalKeyframes; ++k) {
        const float frac = static_cast<float>(k) / static_cast<float>(totalKeyframes - 1U);
        const float time = frac * s.durationSeconds;

        // 4D Latent W: Smooth sinusoid undulating through hyperspace
        const float wVal = 0.75f * std::sin(frac * 6.2831853f * static_cast<float>(worldCount * 0.5f));
        trackW.keyframes.push_back({time, wVal, EasingType::Smoothstep});

        // SO(4) Rotor Alpha: Continuous progressive rotation
        const float alphaVal = frac * 6.2831853f * 2.0f;
        trackAlpha.keyframes.push_back({time, alphaVal, EasingType::Linear});

        // SO(4) Rotor Beta: Harmonic breathing
        const float betaVal = 0.45f * std::cos(frac * 6.2831853f * 3.0f);
        trackBeta.keyframes.push_back({time, betaVal, EasingType::Smoothstep});

        // Camera distance: Dynamic zoom breathing between 3.8 and 5.6
        const float camDist = 4.2f + 1.2f * std::sin(frac * 6.2831853f * static_cast<float>(worldCount));
        trackCamDist.keyframes.push_back({time, camDist, EasingType::Smoothstep});

        // Camera Azimuth: Continuous smooth 360-degree orbital sweep
        const float camAz = frac * 6.2831853f;
        trackCamAz.keyframes.push_back({time, camAz, EasingType::Linear});

        // Camera Elevation: Gentle harmonic nodding
        const float camEl = 0.15f + 0.25f * std::sin(frac * 6.2831853f * 2.0f);
        trackCamEl.keyframes.push_back({time, camEl, EasingType::CubicInOut});
    }

    s.tracks.push_back(std::move(trackW));
    s.tracks.push_back(std::move(trackAlpha));
    s.tracks.push_back(std::move(trackBeta));
    s.tracks.push_back(std::move(trackCamDist));
    s.tracks.push_back(std::move(trackCamAz));
    s.tracks.push_back(std::move(trackCamEl));

    s.sortCues();
    return s;
}

WorldTimelineScript SculptureTimelineDirector::generateOrbitalCinematicScript(
    std::string_view presetName,
    float durationSeconds,
    float baseRadius,
    bool loop
) {
    WorldTimelineScript s{};
    s.id = "cinematic_orbit_" + std::string(presetName);
    s.title = "Cinematic Flight: " + std::string(presetName);
    s.description = "4-phase multi-perspective architectural showcase flight";
    s.durationSeconds = std::max(durationSeconds, 10.0f);
    s.loop = loop;

    const float d = s.durationSeconds;

    // Cues
    s.actionCues.push_back({0.0f, NavigatorAction::ResetCamera, 0.0f, "Align baseline framing"});
    s.actionCues.push_back({d * 0.25f, NavigatorAction::Mutate, 0.08f, "Subtle structural shift"});
    s.actionCues.push_back({d * 0.50f, NavigatorAction::CycleInnerMap, 0.0f, "Inner topological transformation"});
    s.actionCues.push_back({d * 0.75f, NavigatorAction::RandomizePalette, 0.0f, "Atmospheric color drift"});

    // Tracks: Camera Distance, Azimuth, Elevation
    TimelineChannelTrack distTrack; distTrack.channelName = "cameraDistance";
    distTrack.keyframes.push_back({0.0f, baseRadius * 1.35f, EasingType::Smoothstep});
    distTrack.keyframes.push_back({d * 0.25f, baseRadius * 0.85f, EasingType::CubicInOut});
    distTrack.keyframes.push_back({d * 0.50f, baseRadius * 1.20f, EasingType::Smoothstep});
    distTrack.keyframes.push_back({d * 0.75f, baseRadius * 0.90f, EasingType::CubicInOut});
    distTrack.keyframes.push_back({d, baseRadius * 1.35f, EasingType::Smoothstep});

    TimelineChannelTrack azTrack; azTrack.channelName = "cameraAzimuth";
    azTrack.keyframes.push_back({0.0f, 0.0f, EasingType::Linear});
    azTrack.keyframes.push_back({d, 6.2831853f, EasingType::Linear});

    TimelineChannelTrack elTrack; elTrack.channelName = "cameraElevation";
    elTrack.keyframes.push_back({0.0f, 0.35f, EasingType::Smoothstep});
    elTrack.keyframes.push_back({d * 0.25f, -0.20f, EasingType::CubicInOut});
    elTrack.keyframes.push_back({d * 0.50f, 0.75f, EasingType::Smoothstep});
    elTrack.keyframes.push_back({d * 0.75f, 0.10f, EasingType::CubicInOut});
    elTrack.keyframes.push_back({d, 0.35f, EasingType::Smoothstep});

    TimelineChannelTrack wTrack; wTrack.channelName = "latentW";
    wTrack.keyframes.push_back({0.0f, -0.6f, EasingType::Smoothstep});
    wTrack.keyframes.push_back({d * 0.5f, 0.6f, EasingType::Smoothstep});
    wTrack.keyframes.push_back({d, -0.6f, EasingType::Smoothstep});

    s.tracks.push_back(std::move(distTrack));
    s.tracks.push_back(std::move(azTrack));
    s.tracks.push_back(std::move(elTrack));
    s.tracks.push_back(std::move(wTrack));

    s.sortCues();
    return s;
}

void SculptureTimelineDirector::play() noexcept {
    isPlaying_ = true;
    isPaused_ = false;
}

void SculptureTimelineDirector::pause() noexcept {
    isPaused_ = true;
}

void SculptureTimelineDirector::resume() noexcept {
    if (isPlaying_) {
        isPaused_ = false;
    } else {
        play();
    }
}

void SculptureTimelineDirector::stop() noexcept {
    isPlaying_ = false;
    isPaused_ = false;
    currentTime_ = 0.0f;
    nextCueIndex_ = 0U;
    cuesExecutedCount_ = 0U;
    cameraState_ = TimelineCameraState{};
}

void SculptureTimelineDirector::togglePlayPause() noexcept {
    if (!isPlaying_) {
        play();
    } else if (isPaused_) {
        resume();
    } else {
        pause();
    }
}

void SculptureTimelineDirector::seek(float timeSeconds) noexcept {
    currentTime_ = std::clamp(timeSeconds, 0.0f, script_.durationSeconds);
    nextCueIndex_ = 0U;
    cuesExecutedCount_ = 0U;
    while (nextCueIndex_ < script_.actionCues.size() && script_.actionCues[nextCueIndex_].timestampSeconds <= currentTime_) {
        nextCueIndex_++;
        cuesExecutedCount_++;
    }
}

void SculptureTimelineDirector::tick(SculptureWorldNavigator& navigator, float deltaSeconds) {
    if (!isPlaying_ || isPaused_) return;

    const float prevTime = currentTime_;
    currentTime_ += deltaSeconds;

    if (currentTime_ >= script_.durationSeconds) {
        dispatchPendingCues(navigator, prevTime, script_.durationSeconds);

        if (script_.loop) {
            currentTime_ = std::fmod(currentTime_, script_.durationSeconds);
            nextCueIndex_ = 0U;
            cuesExecutedCount_ = 0U;
            dispatchPendingCues(navigator, -0.0001f, currentTime_);
        } else {
            currentTime_ = script_.durationSeconds;
            isPlaying_ = false;
        }
    } else {
        dispatchPendingCues(navigator, prevTime, currentTime_);
    }

    applyContinuousTracks(navigator, currentTime_);
}

void SculptureTimelineDirector::dispatchPendingCues(SculptureWorldNavigator& navigator, float /*prevTime*/, float currTime) {
    while (nextCueIndex_ < script_.actionCues.size() && script_.actionCues[nextCueIndex_].timestampSeconds <= currTime) {
        const auto& cue = script_.actionCues[nextCueIndex_++];
        cuesExecutedCount_++;
        navigator.dispatchAction(cue.action, cue.value);
        navigator.notifyTimelineCueFired(cue);
        std::printf("[SculptureTimelineDirector] Fired cue [%zu/%zu] at %.2fs: %s (param: %.2f) - %s\n",
                    cuesExecutedCount_, script_.actionCues.size(),
                    cue.timestampSeconds, actionName(cue.action).data(),
                    cue.value, cue.description.c_str());
    }
}

void SculptureTimelineDirector::applyContinuousTracks(SculptureWorldNavigator& navigator, float time) {
    bool hasCameraTracks = false;

    for (const auto& track : script_.tracks) {
        const float val = track.evaluate(time);

        if (track.channelName == "latentW") {
            navigator.setLatentW(val);
        } else if (track.channelName == "rotorAlpha") {
            navigator.setRotorAlpha(val);
        } else if (track.channelName == "rotorBeta") {
            navigator.setRotorBeta(val);
        } else if (track.channelName == "speed" || track.channelName == "speedMultiplier") {
            // Adjust baseline speed directly
            navigator.resetSpeed();
            navigator.increaseSpeed(val);
        } else if (track.channelName == "audioBpm") {
            if (navigator.audioDirector() != nullptr) {
                navigator.audioDirector()->setBpm(val);
            }
        } else if (track.channelName == "cameraDistance" || track.channelName == "cameraRadius") {
            cameraState_.distance = val;
            hasCameraTracks = true;
        } else if (track.channelName == "cameraAzimuth") {
            cameraState_.azimuth = val;
            hasCameraTracks = true;
        } else if (track.channelName == "cameraElevation") {
            cameraState_.elevation = val;
            hasCameraTracks = true;
        } else if (track.channelName == "cameraTargetX") {
            cameraState_.targetX = val;
            hasCameraTracks = true;
        } else if (track.channelName == "cameraTargetY") {
            cameraState_.targetY = val;
            hasCameraTracks = true;
        } else if (track.channelName == "cameraTargetZ") {
            cameraState_.targetZ = val;
            hasCameraTracks = true;
        } else if (track.channelName == "rotorTheta" || track.channelName == "rotorTheta4D") {
            navigator.setRotorTheta4D(val);
        } else if (track.channelName == "morphSpaceX") {
            const float curY = (navigator.morphSpace() != nullptr) ? navigator.morphSpace()->cursorY() : 0.0f;
            navigator.setMorphSpaceCursor(val, curY);
        } else if (track.channelName == "morphSpaceY") {
            const float curX = (navigator.morphSpace() != nullptr) ? navigator.morphSpace()->cursorX() : 0.0f;
            navigator.setMorphSpaceCursor(curX, val);
        } else if (track.channelName == "crossfade" || track.channelName == "deckCrossfade") {
            navigator.setCrossfadePosition(val);
        }
    }

    cameraState_.active = hasCameraTracks;
}

mgfx::Camera SculptureTimelineDirector::computeCamera(float fovYRad) const noexcept {
    const float r = std::max(cameraState_.distance, 0.1f);
    const float az = cameraState_.azimuth;
    const float el = std::clamp(cameraState_.elevation, -1.5f, 1.5f);

    const float cosEl = std::cos(el);
    const float sinEl = std::sin(el);
    const float cosAz = std::cos(az);
    const float sinAz = std::sin(az);

    const mgfx::math::vec3 target(cameraState_.targetX, cameraState_.targetY, cameraState_.targetZ);
    const mgfx::math::vec3 eye(
        target.x + r * cosEl * sinAz,
        target.y + r * sinEl,
        target.z + r * cosEl * cosAz
    );

    return mgfx::Camera{
        .eye = eye,
        .target = target,
        .up = mgfx::math::vec3(0.0f, 1.0f, 0.0f),
        .fovYRad = fovYRad
    };
}

TimelinePlayheadStatus SculptureTimelineDirector::status() const noexcept {
    TimelinePlayheadStatus s{};
    s.isPlaying = isPlaying_;
    s.isPaused = isPaused_;
    s.currentTime = currentTime_;
    s.totalDuration = script_.durationSeconds;
    s.progress = (script_.durationSeconds > 0.0f) ? std::clamp(currentTime_ / script_.durationSeconds, 0.0f, 1.0f) : 0.0f;
    s.cuesExecuted = cuesExecutedCount_;
    s.totalCues = script_.actionCues.size();
    s.activeScriptTitle = script_.title;
    s.hasCameraOverride = cameraState_.active;

    if (nextCueIndex_ < script_.actionCues.size()) {
        const auto& nextCue = script_.actionCues[nextCueIndex_];
        s.nextCueDescription = nextCue.description.empty() ? std::string(actionName(nextCue.action)) : nextCue.description;
        s.nextCueTimeRemaining = std::max(0.0f, nextCue.timestampSeconds - currentTime_);
    }

    return s;
}

std::string SculptureTimelineDirector::formatHud() const {
    if (!isPlaying_) return "";
    char buffer[256];
    const auto st = status();
    const char* const camTag = st.hasCameraOverride ? " [CAM-PATH]" : "";
    if (!st.nextCueDescription.empty()) {
        std::snprintf(buffer, sizeof(buffer), "TIMELINE: \"%s\" [%.1fs/%.1fs]%s (Next: %s in %.1fs)",
                      st.activeScriptTitle.c_str(), st.currentTime, st.totalDuration, camTag,
                      st.nextCueDescription.c_str(), st.nextCueTimeRemaining);
    } else {
        std::snprintf(buffer, sizeof(buffer), "TIMELINE: \"%s\" [%.1fs/%.1fs]%s (Complete)",
                      st.activeScriptTitle.c_str(), st.currentTime, st.totalDuration, camTag);
    }
    return std::string(buffer);
}

} // namespace epi_sculpt
