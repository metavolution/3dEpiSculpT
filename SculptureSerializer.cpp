#include "SculptureSerializer.h"
#include "SculptureJsonView.h"
#include "SculptureFilePublication.h"
#include "SculptureParameterRegistry.h"
#include <epi/io/bounded_text.hpp>
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <map>
#include <filesystem>
#include <cctype>
#include <charconv>
#include <cmath>
#include <limits>
#include <type_traits>
#include <utility>

namespace epi_sculpt {

namespace {

// Educational boundary: select source fields once, then interpret their math.
// This private index borrows immutable input bytes; it owns no application state.
// Named legacy envelopes are explicit projections, not recursive JSON lookup.
struct PresetJsonFields {
    std::map<std::string_view, std::string_view> values;
    std::vector<std::pair<std::string_view, std::string_view>> routes;
    std::optional<std::string_view> macro;
    bool syntaxValid = false;
    bool admitted = false;
    SculpturePresetDecodeError error = SculpturePresetDecodeError::InvalidJson;
    std::string_view errorField;
};

PresetJsonFields selectPresetJsonFields(std::string_view content) {
    PresetJsonFields result;
    std::vector<std::pair<std::string_view, std::string_view>> envelopes;
    bool macroValid = true;
    result.syntaxValid = json_view::object(content, [&](std::string_view key, std::string_view value) {
        // First occurrence wins, matching existing scalar lookup. Route order
        // is recorded separately so indexing cannot reorder authored execution.
        result.values.try_emplace(key, value);
        if (key.starts_with("route_")) result.routes.emplace_back(key, value);
        if (key == "params" || key == "sculptureParams" || key == "meta")
            envelopes.emplace_back(key, value);
        if (key == "macro_automation") {
            if (result.macro || !value.starts_with('{')) macroValid = false;
            else result.macro = value;
        }
        return true; // Finish syntax assessment independently of field admission.
    });
    if (!result.syntaxValid) return result;
    if (!macroValid) {
        result.error = SculpturePresetDecodeError::InvalidMacroAutomation;
        result.errorField = "macro_automation";
        return result;
    }

    const auto identityField = [](std::string_view key) {
        return key == "preset" || key == "presetId" || key == "title" || key == "id";
    };
    const bool rootIdentity = result.values.contains("preset") || result.values.contains("presetId")
        || result.values.contains("title") || result.values.contains("id");
    const bool rootRouting = !result.routes.empty();
    for (const auto& [key, value] : envelopes) {
        result.error = SculpturePresetDecodeError::InvalidProjection;
        result.errorField = key;
        if (key == "meta") {
            if (!value.starts_with('{') || rootIdentity) continue;
            if (!json_view::object(value, [&](std::string_view name, std::string_view field) {
                    if (identityField(name)) result.values.try_emplace(name, field);
                    return true;
                })) return result;
        } else if (!json_view::object(value, [&](std::string_view name, std::string_view field) {
                if (!identityField(name)) result.values.try_emplace(name, field);
                if (!rootRouting && name.starts_with("route_")) result.routes.emplace_back(name, field);
                return true;
            })) return result;
    }
    result.admitted = true;
    result.error = SculpturePresetDecodeError::None;
    result.errorField = {};
    return result;
}

// Revisions are opaque provenance strings. Hex keeps arbitrary bytes out of
// WorldMode's line/key and playlist delimiter syntax without restricting a
// future fingerprint algorithm. Empty means explicitly unversioned.
std::string encodePlaylistRevision(std::string_view revision) {
    constexpr char digits[] = "0123456789abcdef";
    std::string encoded;
    for (const unsigned char byte : revision) {
        encoded.push_back(digits[byte >> 4U]);
        encoded.push_back(digits[byte & 15U]);
    }
    return encoded;
}

std::optional<std::string> decodePlaylistRevision(std::string_view encoded) {
    if (encoded.size() % 2U != 0U) return std::nullopt;
    const auto digit = [](char byte) -> int {
        if (byte >= '0' && byte <= '9') return byte - '0';
        if (byte >= 'a' && byte <= 'f') return byte - 'a' + 10;
        return -1;
    };
    std::string revision;
    revision.reserve(encoded.size() / 2U);
    for (std::size_t index = 0; index < encoded.size(); index += 2U) {
        const int high = digit(encoded[index]);
        const int low = digit(encoded[index + 1U]);
        if (high < 0 || low < 0) return std::nullopt;
        revision.push_back(static_cast<char>((high << 4U) | low));
    }
    return revision;
}

bool parseFloat(const std::string& text, float& value) {
    try {
        std::size_t consumed = 0U;
        const float parsed = std::stof(text, &consumed);
        if (consumed != text.size() || !std::isfinite(parsed)) return false;
        value = parsed;
        return true;
    } catch (...) {
        return false;
    }
}

bool parseInt(const std::string& text, int& value) {
    try {
        std::size_t consumed = 0U;
        const long long parsed = std::stoll(text, &consumed);
        if (consumed != text.size()
            || parsed < std::numeric_limits<int>::min()
            || parsed > std::numeric_limits<int>::max()) {
            return false;
        }
        value = static_cast<int>(parsed);
        return true;
    } catch (...) {
        return false;
    }
}

bool parseUint32(const std::string& text, std::uint32_t& value) {
    // stoull accepts a minus sign and applies unsigned negation. For example,
    // "-0" becomes zero and a large negative magnitude can wrap into range.
    // Keep existing positive/whitespace compatibility but reject negative input.
    std::size_t first = 0;
    while (first < text.size() && std::isspace(static_cast<unsigned char>(text[first]))) ++first;
    if (first < text.size() && text[first] == '-') return false;
    try {
        std::size_t consumed = 0U;
        const unsigned long long parsed = std::stoull(text, &consumed);
        if (consumed != text.size()
            || parsed > std::numeric_limits<std::uint32_t>::max()) {
            return false;
        }
        value = static_cast<std::uint32_t>(parsed);
        return true;
    } catch (...) {
        return false;
    }
}

// Recorder-owned identity counters use unsigned decimal text. from_chars
// rejects signs, whitespace, overflow and trailing data without throwing.
bool parseRecordedUint64(const std::string& text, std::uint64_t& value) noexcept {
    if (text.empty()) return false;
    std::uint64_t parsed = 0;
    const auto result = std::from_chars(text.data(), text.data() + text.size(), parsed);
    if (result.ec != std::errc{} || result.ptr != text.data() + text.size()) return false;
    value = parsed;
    return true;
}

template <typename Enum>
bool parseContiguousEnum(const std::string& text, Enum last, Enum& value) {
    int parsed = 0;
    if (!parseInt(text, parsed) || parsed < 0
        || parsed > static_cast<int>(last)) {
        return false;
    }
    value = static_cast<Enum>(parsed);
    return true;
}

bool parseColorEffect(const std::string& text, ColorEffectType& value) {
    int parsed = 0;
    if (!parseInt(text, parsed) || parsed < 0) {
        return false;
    }
    if (parsed <= static_cast<int>(ColorEffectType::ChladniAcoustic) ||
        parsed == static_cast<int>(ColorEffectType::LivingPaletteAgent)) {
        value = static_cast<ColorEffectType>(parsed);
        return true;
    }
    return false;
}

bool validResolutionScale(StageResolutionScale scale) noexcept {
    switch (scale) {
    case StageResolutionScale::Full:
    case StageResolutionScale::Half:
    case StageResolutionScale::Quarter:
    case StageResolutionScale::Eighth:
        return true;
    }
    return false;
}

bool validProgramParams(const SculptureProgramParams& params) noexcept {
    const auto finiteAffine = [](const ComplexAffine& affine) {
        return std::isfinite(affine.scaleReal)
            && std::isfinite(affine.scaleImag)
            && std::isfinite(affine.offsetReal)
            && std::isfinite(affine.offsetImag);
    };
    if (!finiteAffine(params.pre) || !finiteAffine(params.post)) return false;
    for (float band : params.audioSpectrum) {
        if (!std::isfinite(band)) return false;
    }
    for (const auto& meta : kFloatParams) {
        if (!std::isfinite(params.*meta.member)) return false;
    }

    // Underlying types for InnerMap, ReductionMode, SeedType, and ColorEffectType
    // are unsigned (uint8_t); enforce upper-bound domain contracts without
    // triggering tautological < 0 warnings under -Wtype-limits.
    static_assert(std::is_unsigned_v<std::underlying_type_t<InnerMap>>);
    static_assert(std::is_unsigned_v<std::underlying_type_t<ReductionMode>>);
    static_assert(std::is_unsigned_v<std::underlying_type_t<SeedType>>);
    static_assert(std::is_unsigned_v<std::underlying_type_t<ColorEffectType>>);
    if (params.inner > InnerMap::KaliSetOrbit
        || params.innerB > InnerMap::KaliSetOrbit
        || params.reduction > ReductionMode::CliffordFoliation
        || params.reductionB > ReductionMode::CliffordFoliation
        || params.seedType > SeedType::QuantumProbabilityCloud
        || params.seedTypeB > SeedType::QuantumProbabilityCloud
        || (params.colorEffect != ColorEffectType::LivingPaletteAgent
            && params.colorEffect > ColorEffectType::ChladniAcoustic)
        || !validResolutionScale(params.auxiliaryResolutionScale)) {
        return false;
    }

    // Enforce only bounds declared by the renderer-neutral program contract.
    // Supports all 16 canonical output manifold modes (0 through 15) and extended 4D foliation
    return params.enable4D <= 24U
        && params.enable4DB <= 24U
        && params.outputMode <= 15U
        && params.outputModeB <= 15U
        && params.temporalFeedbackBlend >= 0.0f
        && params.temporalFeedbackBlend <= 1.0f
        && params.temporalFade > 0.0f
        && params.temporalFade <= 1.0f
        && params.fogAnisotropy > -1.0f
        && params.fogAnisotropy < 1.0f
        && params.filmThickness >= 100.0f
        && params.filmThickness <= 1200.0f;
}

void writeVec4(std::ostringstream& out, const std::string& key, const ComplexAffine& v) {
    out << key << "=" << v.scaleReal << "," << v.scaleImag << "," << v.offsetReal << "," << v.offsetImag << "\n";
}

void writeSpectrum(std::ostringstream& out, const std::string& key, const std::array<float, 4>& s) {
    out << key << "=" << s[0] << "," << s[1] << "," << s[2] << "," << s[3] << "\n";
}

bool parseVec4(const std::string& val, ComplexAffine& v) {
    std::stringstream ss(val);
    std::string token;
    float arr[4] = {0};
    int i = 0;
    while (std::getline(ss, token, ',')) {
        if (i >= 4 || !parseFloat(token, arr[i])) return false;
        ++i;
    }
    if (i == 4) {
        v.scaleReal = arr[0]; v.scaleImag = arr[1]; v.offsetReal = arr[2]; v.offsetImag = arr[3];
        return true;
    }
    return false;
}

bool parseSpectrum(const std::string& val, std::array<float, 4>& s) {
    ComplexAffine dummy;
    if (parseVec4(val, dummy)) {
        s = {dummy.scaleReal, dummy.scaleImag, dummy.offsetReal, dummy.offsetImag};
        return true;
    }
    return false;
}

} // namespace

std::string SculptureSerializer::serialize(const SculptureProgramParams& params) {
    std::ostringstream out;
    out << std::setprecision(std::numeric_limits<float>::max_digits10);
    out << "# EpiC 3D Sculpture State v1\n";
    writeVec4(out, "pre", params.pre);
    writeVec4(out, "post", params.post);
    writeSpectrum(out, "audioSpectrum", params.audioSpectrum);
    out << "inner=" << static_cast<int>(params.inner) << "\n";
    out << "innerB=" << static_cast<int>(params.innerB) << "\n";
    out << "reduction=" << static_cast<int>(params.reduction) << "\n";
    out << "reductionB=" << static_cast<int>(params.reductionB) << "\n";
    out << "seedType=" << static_cast<int>(params.seedType) << "\n";
    out << "seedTypeB=" << static_cast<int>(params.seedTypeB) << "\n";
    out << "colorEffect=" << static_cast<int>(params.colorEffect) << "\n";
    out << "auxiliaryResolutionScale=" << static_cast<int>(params.auxiliaryResolutionScale) << "\n";
    out << "temporalReset=" << (params.temporalReset ? 1 : 0) << "\n";

    for (const auto& meta : kUintParams) {
        out << meta.name << "=" << params.*meta.member << "\n";
    }
    for (const auto& meta : kFloatParams) {
        out << meta.name << "=" << params.*meta.member << "\n";
    }
    
    return out.str();
}

std::optional<SculptureProgramParams> SculptureSerializer::deserialize(const std::string& payload) {
    SculptureProgramParams params{};
    std::istringstream stream(payload);
    std::string line;
    
    std::map<std::string, std::string> kv;
    while (std::getline(stream, line)) {
        if (line.empty() || line[0] == '#') continue;
        auto delim = line.find('=');
        if (delim != std::string::npos) {
            kv[line.substr(0, delim)] = line.substr(delim + 1);
        }
    }

    if ((kv.count("pre") && !parseVec4(kv["pre"], params.pre))
        || (kv.count("post") && !parseVec4(kv["post"], params.post))
        || (kv.count("audioSpectrum")
            && !parseSpectrum(kv["audioSpectrum"], params.audioSpectrum))
        || (kv.count("inner")
            && !parseContiguousEnum(kv["inner"], InnerMap::KaliSetOrbit, params.inner))
        || (kv.count("innerB")
            && !parseContiguousEnum(kv["innerB"], InnerMap::KaliSetOrbit, params.innerB))
        || (kv.count("reduction")
            && !parseContiguousEnum(kv["reduction"], ReductionMode::CliffordFoliation, params.reduction))
        || (kv.count("reductionB")
            && !parseContiguousEnum(kv["reductionB"], ReductionMode::CliffordFoliation, params.reductionB))
        || (kv.count("seedType")
            && !parseContiguousEnum(kv["seedType"], SeedType::QuantumProbabilityCloud, params.seedType))
        || (kv.count("seedTypeB")
            && !parseContiguousEnum(kv["seedTypeB"], SeedType::QuantumProbabilityCloud, params.seedTypeB))
        || (kv.count("colorEffect")
            && !parseColorEffect(kv["colorEffect"], params.colorEffect))) {
        return std::nullopt;
    }

    if (kv.count("auxiliaryResolutionScale")) {
        int rawScale = 0;
        if (!parseInt(kv["auxiliaryResolutionScale"], rawScale)) return std::nullopt;
        params.auxiliaryResolutionScale = static_cast<StageResolutionScale>(rawScale);
    }
    if (kv.count("temporalReset")) {
        int reset = 0;
        if (!parseInt(kv["temporalReset"], reset) || (reset != 0 && reset != 1)) {
            return std::nullopt;
        }
        params.temporalReset = reset != 0;
    }

    for (const auto& meta : kUintParams) {
        const auto key = std::string(meta.name);
        if (kv.count(key) && !parseUint32(kv[key], params.*meta.member)) {
            return std::nullopt;
        }
    }
    for (const auto& meta : kFloatParams) {
        const auto key = std::string(meta.name);
        if (kv.count(key) && !parseFloat(kv[key], params.*meta.member)) {
            return std::nullopt;
        }
    }

    return validProgramParams(params)
        ? std::optional<SculptureProgramParams>{params}
        : std::nullopt;
}

bool SculptureSerializer::saveToFile(const std::string& filepath, const SculptureProgramParams& params) {
    return file_publication_detail::publishTextFile(filepath, [&] { return serialize(params); });
}

std::optional<std::string> SculptureSerializer::readStateFile(
    const std::string& filepath, const StateLoadLimits& limits) {
    if (limits.maxBytes == 0U || filepath.empty()
        || filepath.find('\0') != std::string::npos) return std::nullopt;
    try {
        return epi::io::readBoundedText(filepath, limits.maxBytes, "state file");
    } catch (const std::runtime_error&) {
        return std::nullopt;
    }
}

std::optional<SculptureProgramParams> SculptureSerializer::deserializeParameterState(
    const std::string& content) {
    // An explicitly versioned capture must satisfy its complete contract, even
    // when a caller asks only for parameters or uses a nonstandard extension.
    if (content.find("worldmode_version=") != std::string::npos) {
        const auto world = deserializeWorldMode(content);
        if (!world) return std::nullopt;
        return world->params;
    }
    return deserialize(content);
}

std::optional<SculptureProgramParams> SculptureSerializer::loadFromFile(const std::string& filepath) {
    return loadFromFile(filepath, StateLoadLimits{});
}

std::optional<SculptureProgramParams> SculptureSerializer::loadFromFile(
    const std::string& filepath, const StateLoadLimits& limits) {
    const auto content = readStateFile(filepath, limits);
    return content ? deserializeParameterState(*content) : std::nullopt;
}

SculptureStateDiff SculptureSerializer::diff(
    const SculptureProgramParams& base,
    const SculptureProgramParams& target,
    float epsilon) {
    SculptureStateDiff result;

    // 1. ComplexAffine pre and post
    const auto diffAffine = [&](const char* prefix, const ComplexAffine& b, const ComplexAffine& t) {
        if (std::abs(b.scaleReal - t.scaleReal) > epsilon) {
            result.deltas.push_back({std::string(prefix) + ".scaleReal", std::to_string(b.scaleReal), std::to_string(t.scaleReal), t.scaleReal - b.scaleReal, true});
        }
        if (std::abs(b.scaleImag - t.scaleImag) > epsilon) {
            result.deltas.push_back({std::string(prefix) + ".scaleImag", std::to_string(b.scaleImag), std::to_string(t.scaleImag), t.scaleImag - b.scaleImag, true});
        }
        if (std::abs(b.offsetReal - t.offsetReal) > epsilon) {
            result.deltas.push_back({std::string(prefix) + ".offsetReal", std::to_string(b.offsetReal), std::to_string(t.offsetReal), t.offsetReal - b.offsetReal, true});
        }
        if (std::abs(b.offsetImag - t.offsetImag) > epsilon) {
            result.deltas.push_back({std::string(prefix) + ".offsetImag", std::to_string(b.offsetImag), std::to_string(t.offsetImag), t.offsetImag - b.offsetImag, true});
        }
    };
    diffAffine("pre", base.pre, target.pre);
    diffAffine("post", base.post, target.post);

    // 2. Audio spectrum
    for (std::size_t i = 0; i < 4; ++i) {
        if (std::abs(base.audioSpectrum[i] - target.audioSpectrum[i]) > epsilon) {
            result.deltas.push_back({
                "audioSpectrum[" + std::to_string(i) + "]",
                std::to_string(base.audioSpectrum[i]),
                std::to_string(target.audioSpectrum[i]),
                target.audioSpectrum[i] - base.audioSpectrum[i],
                true
            });
        }
    }

    // 3. Reflective uint params
    for (const auto& meta : kUintParams) {
        const std::uint32_t bVal = base.*meta.member;
        const std::uint32_t tVal = target.*meta.member;
        if (bVal != tVal) {
            result.deltas.push_back({
                std::string(meta.name),
                std::to_string(bVal),
                std::to_string(tVal),
                static_cast<float>(tVal) - static_cast<float>(bVal),
                true
            });
        }
    }

    // 4. Reflective float params
    for (const auto& meta : kFloatParams) {
        const float bVal = base.*meta.member;
        const float tVal = target.*meta.member;
        if (std::abs(bVal - tVal) > epsilon) {
            result.deltas.push_back({
                std::string(meta.name),
                std::to_string(bVal),
                std::to_string(tVal),
                tVal - bVal,
                true
            });
        }
    }

    // 5. Enums
    const auto diffEnum = [&](const char* name, int bVal, int tVal) {
        if (bVal != tVal) {
            result.deltas.push_back({
                std::string(name),
                std::to_string(bVal),
                std::to_string(tVal),
                static_cast<float>(tVal - bVal),
                true
            });
        }
    };
    diffEnum("inner", static_cast<int>(base.inner), static_cast<int>(target.inner));
    diffEnum("innerB", static_cast<int>(base.innerB), static_cast<int>(target.innerB));
    diffEnum("reduction", static_cast<int>(base.reduction), static_cast<int>(target.reduction));
    diffEnum("reductionB", static_cast<int>(base.reductionB), static_cast<int>(target.reductionB));
    diffEnum("seedType", static_cast<int>(base.seedType), static_cast<int>(target.seedType));
    diffEnum("seedTypeB", static_cast<int>(base.seedTypeB), static_cast<int>(target.seedTypeB));
    diffEnum("colorEffect", static_cast<int>(base.colorEffect), static_cast<int>(target.colorEffect));
    diffEnum("auxiliaryResolutionScale", static_cast<int>(base.auxiliaryResolutionScale), static_cast<int>(target.auxiliaryResolutionScale));

    // 6. Bool
    if (base.temporalReset != target.temporalReset) {
        result.deltas.push_back({
            "temporalReset",
            base.temporalReset ? "1" : "0",
            target.temporalReset ? "1" : "0",
            target.temporalReset ? 1.0f : -1.0f,
            true
        });
    }

    return result;
}

bool SculptureSerializer::applyDiff(
    SculptureProgramParams& target,
    const SculptureStateDiff& diff) {
    bool allApplied = true;

    for (const auto& delta : diff.deltas) {
        bool handled = false;

        // 1. ComplexAffine subfields or whole keys
        if (delta.key == "pre.scaleReal") {
            float v = 0.0f;
            if (parseFloat(delta.newValue, v)) { target.pre.scaleReal = v; handled = true; }
        } else if (delta.key == "pre.scaleImag") {
            float v = 0.0f;
            if (parseFloat(delta.newValue, v)) { target.pre.scaleImag = v; handled = true; }
        } else if (delta.key == "pre.offsetReal") {
            float v = 0.0f;
            if (parseFloat(delta.newValue, v)) { target.pre.offsetReal = v; handled = true; }
        } else if (delta.key == "pre.offsetImag") {
            float v = 0.0f;
            if (parseFloat(delta.newValue, v)) { target.pre.offsetImag = v; handled = true; }
        } else if (delta.key == "pre") {
            ComplexAffine aff;
            if (parseVec4(delta.newValue, aff)) { target.pre = aff; handled = true; }
        } else if (delta.key == "post.scaleReal") {
            float v = 0.0f;
            if (parseFloat(delta.newValue, v)) { target.post.scaleReal = v; handled = true; }
        } else if (delta.key == "post.scaleImag") {
            float v = 0.0f;
            if (parseFloat(delta.newValue, v)) { target.post.scaleImag = v; handled = true; }
        } else if (delta.key == "post.offsetReal") {
            float v = 0.0f;
            if (parseFloat(delta.newValue, v)) { target.post.offsetReal = v; handled = true; }
        } else if (delta.key == "post.offsetImag") {
            float v = 0.0f;
            if (parseFloat(delta.newValue, v)) { target.post.offsetImag = v; handled = true; }
        } else if (delta.key == "post") {
            ComplexAffine aff;
            if (parseVec4(delta.newValue, aff)) { target.post = aff; handled = true; }
        } else if (delta.key == "audioSpectrum[0]" || delta.key == "audioSpectrum.0") {
            float v = 0.0f;
            if (parseFloat(delta.newValue, v)) { target.audioSpectrum[0] = v; handled = true; }
        } else if (delta.key == "audioSpectrum[1]" || delta.key == "audioSpectrum.1") {
            float v = 0.0f;
            if (parseFloat(delta.newValue, v)) { target.audioSpectrum[1] = v; handled = true; }
        } else if (delta.key == "audioSpectrum[2]" || delta.key == "audioSpectrum.2") {
            float v = 0.0f;
            if (parseFloat(delta.newValue, v)) { target.audioSpectrum[2] = v; handled = true; }
        } else if (delta.key == "audioSpectrum[3]" || delta.key == "audioSpectrum.3") {
            float v = 0.0f;
            if (parseFloat(delta.newValue, v)) { target.audioSpectrum[3] = v; handled = true; }
        } else if (delta.key == "audioSpectrum") {
            std::array<float, 4> spec{};
            if (parseSpectrum(delta.newValue, spec)) { target.audioSpectrum = spec; handled = true; }
        } else if (delta.key == "inner") {
            InnerMap m{};
            if (parseContiguousEnum(delta.newValue, InnerMap::KaliSetOrbit, m)) { target.inner = m; handled = true; }
        } else if (delta.key == "innerB") {
            InnerMap m{};
            if (parseContiguousEnum(delta.newValue, InnerMap::KaliSetOrbit, m)) { target.innerB = m; handled = true; }
        } else if (delta.key == "reduction") {
            ReductionMode r{};
            if (parseContiguousEnum(delta.newValue, ReductionMode::CliffordFoliation, r)) { target.reduction = r; handled = true; }
        } else if (delta.key == "reductionB") {
            ReductionMode r{};
            if (parseContiguousEnum(delta.newValue, ReductionMode::CliffordFoliation, r)) { target.reductionB = r; handled = true; }
        } else if (delta.key == "seedType") {
            SeedType s{};
            if (parseContiguousEnum(delta.newValue, SeedType::QuantumProbabilityCloud, s)) { target.seedType = s; handled = true; }
        } else if (delta.key == "seedTypeB") {
            SeedType s{};
            if (parseContiguousEnum(delta.newValue, SeedType::QuantumProbabilityCloud, s)) { target.seedTypeB = s; handled = true; }
        } else if (delta.key == "colorEffect") {
            ColorEffectType c{};
            if (parseColorEffect(delta.newValue, c)) { target.colorEffect = c; handled = true; }
        } else if (delta.key == "auxiliaryResolutionScale") {
            int rawScale = 0;
            if (parseInt(delta.newValue, rawScale) && validResolutionScale(static_cast<StageResolutionScale>(rawScale))) {
                target.auxiliaryResolutionScale = static_cast<StageResolutionScale>(rawScale);
                handled = true;
            }
        } else if (delta.key == "temporalReset") {
            int reset = 0;
            if (parseInt(delta.newValue, reset)) {
                target.temporalReset = (reset != 0);
                handled = true;
            }
        }

        if (!handled) {
            // Check kUintParams
            for (const auto& meta : kUintParams) {
                if (meta.name == delta.key) {
                    std::uint32_t uVal = 0;
                    if (parseUint32(delta.newValue, uVal)) {
                        target.*meta.member = uVal;
                        handled = true;
                    }
                    break;
                }
            }
        }

        if (!handled) {
            // Check kFloatParams
            for (const auto& meta : kFloatParams) {
                if (meta.name == delta.key) {
                    float fVal = 0.0f;
                    if (parseFloat(delta.newValue, fVal)) {
                        target.*meta.member = fVal;
                        handled = true;
                    }
                    break;
                }
            }
        }

        if (!handled) {
            allApplied = false;
        }
    }

    return allApplied;
}

std::string SculptureSerializer::formatDiff(const SculptureStateDiff& diff) {
    if (diff.empty()) {
        return "[DIFF] No parameters modified (identical to baseline).\n";
    }
    std::ostringstream oss;
    oss << "[DIFF] " << diff.size() << " parameter(s) modified:\n";
    for (const auto& d : diff.deltas) {
        oss << "  - " << d.key << ": " << d.oldValue << " -> " << d.newValue;
        if (d.isNumeric) {
            char sign = (d.numericDelta >= 0.0f) ? '+' : '-';
            oss << " (" << sign << std::abs(d.numericDelta) << ")";
        }
        oss << "\n";
    }
    return oss.str();
}

std::string SculptureSerializer::serializeDiff(const SculptureStateDiff& diff) {
    std::ostringstream out;
    out << "# EpiC 3D Parameter Patch v1\n";
    out << "delta_count=" << diff.size() << "\n";
    for (const auto& d : diff.deltas) {
        out << d.key << "=" << d.newValue << "\n";
    }
    return out.str();
}

std::optional<SculptureStateDiff> SculptureSerializer::deserializeDiff(const std::string& payload) {
    SculptureStateDiff diff{};
    std::istringstream stream(payload);
    std::string line;
    while (std::getline(stream, line)) {
        if (line.empty() || line[0] == '#') continue;
        const auto delim = line.find('=');
        if (delim == std::string::npos) continue;
        std::string key = line.substr(0, delim);
        std::string val = line.substr(delim + 1);
        if (key == "delta_count") continue;
        ParameterDelta delta;
        delta.key = std::move(key);
        delta.newValue = std::move(val);
        float num = 0.0f;
        if (parseFloat(delta.newValue, num)) {
            delta.numericDelta = num;
            delta.isNumeric = true;
        }
        diff.deltas.push_back(std::move(delta));
    }
    return diff;
}

bool SculptureSerializer::saveDiffToFile(const std::string& filepath, const SculptureStateDiff& diff) {
    return file_publication_detail::publishTextFile(filepath, [&] { return serializeDiff(diff); });
}

std::optional<SculptureStateDiff> SculptureSerializer::loadDiffFromFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file) return std::nullopt;
    std::ostringstream ss;
    ss << file.rdbuf();
    return deserializeDiff(ss.str());
}

std::string SculptureSerializer::serializeWorldMode(const SculptureWorldMode& mode) {
    if (!mode.volumePresentation.valid())
        throw std::invalid_argument("invalid volume presentation state");
    std::ostringstream out;
    out << std::setprecision(std::numeric_limits<float>::max_digits10);
    out << "# EpiC 3D WorldMode\n";
    out << "worldmode_version=" << SculptureWorldMode::kCurrentVersion << "\n";
    const auto& volume = mode.volumePresentation;
    out << "volume_geometry_recipe=" << volume.recipe << "\n";
    out << "field_color_channel=" << volume.colorChannel << "\n";
    const auto writeVolumeVector = [&out](const char* key, const std::array<float, 4>& values) {
        out << key << '=' << values[0] << ',' << values[1] << ',' << values[2] << ',' << values[3] << '\n';
    };
    writeVolumeVector("field_low_color_and_minimum", volume.lowColorAndMinimum);
    writeVolumeVector("field_high_color_and_maximum", volume.highColorAndMaximum);
    writeVolumeVector("field_optics", volume.optics);
    out << "preset=" << mode.presetName << "\n";
    out << "preset_revision=" << mode.presetRevision << "\n";
    const char* presentation = mode.presentationOwnership == SculpturePresentationOwnership::UnifiedCompositeField ? "composite" : mode.presentationOwnership == SculpturePresentationOwnership::ImplicitField
        ? "implicit"
        : mode.presentationOwnership == SculpturePresentationOwnership::DedicatedWorld
            ? "world"
            : mode.presentationOwnership == SculpturePresentationOwnership::CanonicalFeedback
                ? "canonical"
            : "compact";
    out << "presentation=" << presentation << "\n";
    out << "world_recipe_present=" << (mode.hasResolvedWorldRecipe ? 1U : 0U) << "\n";
    if (mode.hasResolvedWorldRecipe) {
        const WorldRecipe& recipe = mode.resolvedWorldRecipe;
        out << "world_recipe_generator=" << static_cast<std::uint32_t>(recipe.generator) << "\n";
        out << "world_recipe_primitive=" << static_cast<std::uint32_t>(recipe.primitive) << "\n";
        out << "world_recipe_delivery=" << static_cast<std::uint32_t>(recipe.delivery) << "\n";
        out << "world_recipe_presentation=" << static_cast<std::uint32_t>(recipe.presentation) << "\n";
        out << "world_recipe_instance_budget=" << recipe.instanceBudget << "\n";
        out << "world_recipe_generator_hz=" << recipe.generatorHz << "\n";
        out << "world_recipe_material_bands=" << static_cast<std::uint32_t>(recipe.materialBandCount) << "\n";
        out << "world_recipe_uses_4d=" << (recipe.uses4DProjection ? 1U : 0U) << "\n";
        out << "world_recipe_mirror_topology=" << static_cast<std::uint32_t>(recipe.mirrorTopology) << "\n";
    }
    out << "playlist_library_present=" << (mode.playlist.hasLibraryTargets ? 1U : 0U) << "\n";
    if (mode.playlist.hasLibraryTargets) {
        out << "playlist_active=" << (mode.playlist.active ? 1U : 0U) << "\n";
        out << "playlist_paused=" << (mode.playlist.paused ? 1U : 0U) << "\n";
        out << "playlist_loop=" << (mode.playlist.loop ? 1U : 0U) << "\n";
        out << "playlist_playback_mode="
            << static_cast<std::uint32_t>(mode.playlist.playbackMode) << "\n";
        out << "playlist_target_index=" << mode.playlist.targetIndex << "\n";
        out << "playlist_dwell_seconds=" << mode.playlist.dwellSeconds << "\n";
        out << "playlist_target_count=" << mode.playlist.targets.size() << "\n";
        for (std::size_t index = 0U; index < mode.playlist.targets.size(); ++index) {
            const auto& target = mode.playlist.targets[index];
            out << "playlist_target_" << index << "=" << target.presetId << "|"
                << target.durationSeconds << "|" << target.dwellSeconds << "|"
                << static_cast<std::uint32_t>(target.easing) << "\n";
            out << "playlist_target_revision_hex_" << index << "="
                << encodePlaylistRevision(target.presetRevision) << "\n";
        }
    }
    out << "tuning_present=" << (mode.presentationTuning.hasTuning ? 1U : 0U) << "\n";
    if (mode.presentationTuning.hasTuning) {
        out << "tuning_fog_density=" << mode.presentationTuning.fogDensity << "\n";
        out << "tuning_emission_threshold=" << mode.presentationTuning.emissionThreshold << "\n";
        out << "tuning_emission_multiplier=" << mode.presentationTuning.emissionMultiplier << "\n";
    }
    out << "world_quality_requested_budget=" << mode.requestedWorldBudget << "\n";
    out << "world_quality_effective_budget=" << mode.effectiveWorldBudget << "\n";
    out << "world_quality_effective_hz=" << mode.effectiveWorldUpdateHz << "\n";
    out << "canonical_feedback_generation=" << mode.canonicalFeedbackGeneration << "\n";
    out << "canonical_feedback_reset_epoch=" << mode.canonicalFeedbackResetEpoch << "\n";
    out << "compact_generation=" << mode.compactGeneration << "\n";
    out << "world_generation=" << mode.worldGeneration << "\n";
    out << "implicit_generation=" << mode.implicitGeneration << "\n";
    out << "presentation_frame_sequence=" << mode.presentationFrameSequence << "\n";
    out << "presentation_device_generation=" << mode.presentationDeviceGeneration << "\n";
    out << "presentation_surface_generation=" << mode.presentationSurfaceGeneration << "\n";
    out << "capture_submission_serial=" << mode.captureSubmissionSerial << "\n";
    out << "selected_compact_generation=" << mode.selectedCompactGeneration << "\n";
    out << "selected_compact_production_receipt_sequence="
        << mode.selectedCompactProductionReceiptSequence << "\n";
    out << "selected_world_generation=" << mode.selectedWorldGeneration << "\n";
    out << "selected_world_production_receipt_sequence="
        << mode.selectedWorldProductionReceiptSequence << "\n";
    out << "selected_canonical_feedback_generation=" << mode.selectedCanonicalFeedbackGeneration << "\n";
    out << "selected_canonical_feedback_reset_epoch=" << mode.selectedCanonicalFeedbackResetEpoch << "\n";
    out << "selected_canonical_feedback_production_receipt_sequence="
        << mode.selectedCanonicalFeedbackProductionReceiptSequence << "\n";
    out << "selected_implicit_generation=" << mode.selectedImplicitGeneration << "\n";
    out << "selected_implicit_production_receipt_sequence="
        << mode.selectedImplicitProductionReceiptSequence << "\n";
    out << "presentation_outcome=" << mode.presentationOutcome << "\n";
    out << "presentation_outcome_known=" << (mode.presentationOutcomeKnown ? 1U : 0U) << "\n";
    out << "presentation_confirmed=" << (mode.presentationConfirmed ? 1U : 0U) << "\n";
    out << "implicit_external_active=" << (mode.implicitExternalActive ? "true" : "false") << "\n";
    out << "implicit_diagnostic=" << mode.implicitDiagnostic << "\n";
    out << "implicit_quarantine_count=" << mode.implicitQuarantineCount << "\n";
    out << "\n";
    out << "world_version=" << mode.state.version << "\n";
    out << "world_time=" << mode.state.time << "\n";
    out << "world_morph=" << mode.state.morph << "\n";
    out << "world_cameraAngle=" << mode.state.cameraAngle << "\n";
    out << "world_cameraEye=" << mode.state.cameraEye[0] << "," << mode.state.cameraEye[1] << "," << mode.state.cameraEye[2] << "\n";
    out << "world_cameraTarget=" << mode.state.cameraTarget[0] << "," << mode.state.cameraTarget[1] << "," << mode.state.cameraTarget[2] << "\n";
    out << "world_cameraUp=" << mode.state.cameraUp[0] << "," << mode.state.cameraUp[1] << "," << mode.state.cameraUp[2] << "\n";
    out << "world_camera_override=" << (mode.state.hasCameraOverride ? 1U : 0U) << "\n";
    out << "routing_count=" << mode.routingMap.size() << "\n";
    for (std::size_t index = 0; index < mode.routingMap.size(); ++index) {
        out << "routing_" << index << "=" << mode.routingMap[index] << "\n";
    }
    out << "morphspace_active=" << (mode.morphSpace.active ? "true" : "false") << "\n";
    out << "morphspace_cursor=" << mode.morphSpace.cursorX << "," << mode.morphSpace.cursorY << "\n";
    out << "morphspace_motion=" << static_cast<int>(mode.morphSpace.motionMode) << "\n";
    out << "morphspace_speed=" << mode.morphSpace.motionSpeed << "\n";
    out << "morphspace_power=" << mode.morphSpace.shepardPower << "\n";
    out << "morphspace_bank=" << mode.morphSpace.activeBankName << "\n";
    out << "morphspace_path_looping=" << (mode.morphSpace.pathLooping ? "true" : "false") << "\n";
    out << "morphspace_waypoint_count=" << mode.morphSpace.waypoints.size() << "\n";
    for (std::size_t i = 0; i < mode.morphSpace.waypoints.size(); ++i) {
        const auto& wp = mode.morphSpace.waypoints[i];
        out << "morphspace_wp_" << i << "=" << wp.x << "," << wp.y << "," << wp.dwellSeconds << "," << wp.travelSeconds << "," << wp.tag << "\n";
    }
    out << serialize(mode.params);
    return out.str();
}

static bool parseVec3Array(const std::string& val, float arr[3]) {
    float temp[3];
    std::size_t start = 0;
    for (std::size_t i = 0; i < 3; ++i) {
        const auto comma = val.find(',', start);
        if ((i < 2 && comma == std::string::npos)
            || (i == 2 && comma != std::string::npos)) return false;
        const auto end = comma == std::string::npos ? val.size() : comma;
        if (!parseFloat(val.substr(start, end - start), temp[i])) return false;
        start = end + 1;
    }
    arr[0] = temp[0]; arr[1] = temp[1]; arr[2] = temp[2];
    return true;
}

static bool parseVolumeVector(std::string_view text, std::array<float, 4>& result) noexcept {
    std::array<float, 4> candidate{};
    for (std::size_t i = 0; i < candidate.size(); ++i) {
        const auto comma = text.find(',');
        if ((i < 3 && comma == std::string_view::npos) || (i == 3 && comma != std::string_view::npos)) return false;
        const auto token = i < 3 ? text.substr(0, comma) : text;
        if (token.empty()) return false;
        const auto parsed = std::from_chars(token.data(), token.data() + token.size(), candidate[i]);
        if (parsed.ec != std::errc{} || parsed.ptr != token.data() + token.size() ||
            !std::isfinite(candidate[i])) return false;
        if (i < 3) text.remove_prefix(comma + 1);
    }
    result = candidate;
    return true;
}

std::optional<SculptureWorldMode> SculptureSerializer::deserializeWorldMode(const std::string& payload) {
    std::istringstream stream(payload);
    std::map<std::string, std::string> kv;
    bool duplicatePlaylistRevision = false;
    bool duplicateVolumePresentation = false;
    bool duplicateWorldVersion = false;
    std::string line;
    while (std::getline(stream, line)) {
        if (line.empty() || line[0] == '#') continue;
        const auto delimiter = line.find('=');
        if (delimiter != std::string::npos) {
            const auto [entry, inserted] = kv.insert_or_assign(
                line.substr(0, delimiter), line.substr(delimiter + 1));
            if (!inserted && entry->first.starts_with("playlist_target_revision_hex_"))
                duplicatePlaylistRevision = true;
            if (!inserted && entry->first == "worldmode_version") duplicateWorldVersion = true;
            if (!inserted && (entry->first == "volume_geometry_recipe" || entry->first == "field_color_channel" ||
                entry->first == "field_low_color_and_minimum" || entry->first == "field_high_color_and_maximum" ||
                entry->first == "field_optics")) duplicateVolumePresentation = true;
        }
    }

    if (!kv.count("worldmode_version") || duplicateWorldVersion) return std::nullopt;

    int version = 0;
    if (!parseInt(kv["worldmode_version"], version)
        || version < 2 || version > SculptureWorldMode::kCurrentVersion) {
        return std::nullopt;
    }

    SculptureWorldMode mode{};
    if (version >= SculptureWorldMode::kPlaylistRevisionVersion && duplicatePlaylistRevision)
        return std::nullopt;
    mode.version = version;
    if (version >= SculptureWorldMode::kVolumePresentationVersion) {
        constexpr std::array<std::string_view, 5> keys{"volume_geometry_recipe", "field_color_channel",
            "field_low_color_and_minimum", "field_high_color_and_maximum", "field_optics"};
        if (duplicateVolumePresentation) return std::nullopt;
        for (const auto key : keys) if (!kv.contains(std::string(key))) return std::nullopt;
        auto& volume = mode.volumePresentation;
        const auto strictUint = [](const std::string& text, std::uint32_t& out) {
            if (text.empty()) return false;
            const auto parsed = std::from_chars(text.data(), text.data() + text.size(), out);
            return parsed.ec == std::errc{} && parsed.ptr == text.data() + text.size();
        };
        if (!strictUint(kv["volume_geometry_recipe"], volume.recipe) ||
            !strictUint(kv["field_color_channel"], volume.colorChannel) ||
            !parseVolumeVector(kv["field_low_color_and_minimum"], volume.lowColorAndMinimum) ||
            !parseVolumeVector(kv["field_high_color_and_maximum"], volume.highColorAndMaximum) ||
            !parseVolumeVector(kv["field_optics"], volume.optics) || !volume.valid()) return std::nullopt;
    }
    mode.presetName = kv.count("preset") ? kv["preset"] : "";
    if (version >= SculptureWorldMode::kPresetProvenanceVersion) {
        // A v4 capture must carry provenance. Older captures remain loadable,
        // but honestly report that their original recipe revision is unknown.
        if (!kv.count("preset_revision")) return std::nullopt;
        mode.presetRevision = kv["preset_revision"];
    }
    if (version >= 5) {
        if (!kv.count("presentation")) return std::nullopt;
        if (kv["presentation"] == "composite") mode.presentationOwnership = SculpturePresentationOwnership::UnifiedCompositeField;
        else if (kv["presentation"] == "implicit") mode.presentationOwnership = SculpturePresentationOwnership::ImplicitField;
        else if (kv["presentation"] == "world") mode.presentationOwnership = SculpturePresentationOwnership::DedicatedWorld;
        else if (kv["presentation"] == "canonical" || kv["presentation"] == "relief") {
            mode.presentationOwnership = SculpturePresentationOwnership::CanonicalFeedback;
        } else if (kv["presentation"] != "compact") return std::nullopt;
    }
    if (version >= 11) {
        std::uint32_t present = 0U;
        if (!kv.count("world_recipe_present")
            || !parseUint32(kv["world_recipe_present"], present) || present > 1U) {
            return std::nullopt;
        }
        mode.hasResolvedWorldRecipe = present != 0U;
        if (mode.hasResolvedWorldRecipe) {
            constexpr std::array<std::string_view, 9> kRecipeKeys{
                "world_recipe_generator", "world_recipe_primitive",
                "world_recipe_delivery", "world_recipe_presentation",
                "world_recipe_instance_budget", "world_recipe_generator_hz",
                "world_recipe_material_bands", "world_recipe_uses_4d",
                "world_recipe_mirror_topology",
            };
            for (const std::string_view key : kRecipeKeys) {
                if (!kv.count(std::string(key))) return std::nullopt;
            }
            std::uint32_t generator = 0U;
            std::uint32_t primitive = 0U;
            std::uint32_t delivery = 0U;
            std::uint32_t presentationKind = 0U;
            std::uint32_t materialBands = 0U;
            std::uint32_t uses4D = 0U;
            std::uint32_t topology = 0U;
            WorldRecipe& recipe = mode.resolvedWorldRecipe;
            if (!parseUint32(kv["world_recipe_generator"], generator)
                || !parseUint32(kv["world_recipe_primitive"], primitive)
                || !parseUint32(kv["world_recipe_delivery"], delivery)
                || !parseUint32(kv["world_recipe_presentation"], presentationKind)
                || !parseUint32(kv["world_recipe_instance_budget"], recipe.instanceBudget)
                || !parseUint32(kv["world_recipe_generator_hz"], recipe.generatorHz)
                || !parseUint32(kv["world_recipe_material_bands"], materialBands)
                || !parseUint32(kv["world_recipe_uses_4d"], uses4D)
                || !parseUint32(kv["world_recipe_mirror_topology"], topology)
                || generator >= kWorldGeneratorCount
                || primitive > static_cast<std::uint32_t>(WorldPrimitiveKind::TrussStrut)
                || delivery > static_cast<std::uint32_t>(WorldRecipeDelivery::DedicatedGeneratorRequired)
                || presentationKind > static_cast<std::uint32_t>(WorldPresentationKind::ExperimentResources)
                || materialBands == 0U || materialBands > 16U || uses4D > 1U
                || topology >= kMirrorCellTopologyCount) {
                return std::nullopt;
            }
            recipe.generator = static_cast<WorldGeneratorKind>(generator);
            recipe.primitive = static_cast<WorldPrimitiveKind>(primitive);
            recipe.delivery = static_cast<WorldRecipeDelivery>(delivery);
            recipe.presentation = static_cast<WorldPresentationKind>(presentationKind);
            recipe.materialBandCount = static_cast<std::uint8_t>(materialBands);
            recipe.uses4DProjection = uses4D != 0U;
            recipe.mirrorTopology = static_cast<MirrorCellTopology>(topology);
        }
    }
    if (version >= 12) {
        std::uint32_t present = 0U;
        if (!kv.count("playlist_library_present")
            || !parseUint32(kv["playlist_library_present"], present) || present > 1U) {
            return std::nullopt;
        }
        mode.playlist.hasLibraryTargets = present != 0U;
        if (mode.playlist.hasLibraryTargets) {
            std::uint32_t active = 0U, paused = 0U, loop = 0U, count = 0U;
            if (!kv.count("playlist_active") || !kv.count("playlist_paused")
                || !kv.count("playlist_loop") || !kv.count("playlist_target_index")
                || !kv.count("playlist_dwell_seconds") || !kv.count("playlist_target_count")
                || !parseUint32(kv["playlist_active"], active) || active > 1U
                || !parseUint32(kv["playlist_paused"], paused) || paused > 1U
                || !parseUint32(kv["playlist_loop"], loop) || loop > 1U
                || !parseUint32(kv["playlist_target_index"], mode.playlist.targetIndex)
                || !parseFloat(kv["playlist_dwell_seconds"], mode.playlist.dwellSeconds)
                || !parseUint32(kv["playlist_target_count"], count)
                // Bound work before reserve; shared value admission follows decoding.
                || count == 0U || count > SculpturePlaylistCaptureState::kMaxTargets) {
                return std::nullopt;
            }
            if (version >= 20
                && (!kv.count("playlist_playback_mode")
                    || !parseContiguousEnum(kv["playlist_playback_mode"],
                        TourPlaybackMode::ContinuousSpline, mode.playlist.playbackMode))) {
                return std::nullopt;
            }
            mode.playlist.active = active != 0U;
            mode.playlist.paused = paused != 0U;
            mode.playlist.loop = loop != 0U;
            mode.playlist.targets.reserve(count);
            for (std::uint32_t index = 0U; index < count; ++index) {
                const auto key = "playlist_target_" + std::to_string(index);
                if (!kv.count(key)) return std::nullopt;
                std::stringstream fields(kv[key]);
                SculpturePlaylistCaptureTarget target{};
                std::string duration, dwell, easing;
                std::uint32_t easingValue = 0U;
                if (!std::getline(fields, target.presetId, '|')
                    || !std::getline(fields, duration, '|')
                    || !std::getline(fields, dwell, '|')
                    || !std::getline(fields, easing, '|')
                    || fields.rdbuf()->in_avail() != 0
                    || !parseFloat(duration, target.durationSeconds)
                    || !parseFloat(dwell, target.dwellSeconds)
                    || !parseUint32(easing, easingValue)
                    // Reject invalid raw values before narrowing to the enum's storage.
                    || easingValue >= kMorphEasingCount) {
                    return std::nullopt;
                }
                target.easing = static_cast<MorphEasing>(easingValue);
                if (version >= SculptureWorldMode::kPlaylistRevisionVersion) {
                    const auto revision = kv.find("playlist_target_revision_hex_" + std::to_string(index));
                    if (revision == kv.end()) return std::nullopt;
                    auto decodedRevision = decodePlaylistRevision(revision->second);
                    if (!decodedRevision) return std::nullopt;
                    target.presetRevision = std::move(*decodedRevision);
                }
                mode.playlist.targets.push_back(std::move(target));
            }
            if (!isPlaylistCaptureValid(mode.playlist)) return std::nullopt;
        }
    }
    if (version >= 6) {
        if (!kv.count("world_quality_requested_budget")
            || !kv.count("world_quality_effective_budget")
            || !kv.count("world_quality_effective_hz")
            || !parseUint32(kv["world_quality_requested_budget"], mode.requestedWorldBudget)
            || !parseUint32(kv["world_quality_effective_budget"], mode.effectiveWorldBudget)
            || !parseFloat(kv["world_quality_effective_hz"], mode.effectiveWorldUpdateHz)) {
            return std::nullopt;
        }
    }
    if (version >= 7) {
        std::uint32_t cameraOverride = 0U;
        if (!kv.count("world_camera_override")
            || !parseUint32(kv["world_camera_override"], cameraOverride)
            || cameraOverride > 1U) {
            return std::nullopt;
        }
        mode.state.hasCameraOverride = cameraOverride != 0U;
    }
    if (version >= 8) {
        if (!kv.count("canonical_feedback_generation")
            || !kv.count("canonical_feedback_reset_epoch")
            || !parseRecordedUint64(kv["canonical_feedback_reset_epoch"], mode.canonicalFeedbackResetEpoch)) {
            return std::nullopt;
        }
        if (version >= SculptureWorldMode::kWideCanonicalGenerationVersion) {
            if (!parseRecordedUint64(kv["canonical_feedback_generation"], mode.canonicalFeedbackGeneration))
                return std::nullopt;
        } else {
            // Versions 8..20 keep their historical width and positive-number
            // import grammar. Widen the admitted value, never truncate a new ID.
            std::uint32_t legacyGeneration = 0U;
            if (!parseUint32(kv["canonical_feedback_generation"], legacyGeneration))
                return std::nullopt;
            mode.canonicalFeedbackGeneration = legacyGeneration;
        }
        if (kv.count("implicit_external_active")) {
            mode.implicitExternalActive = (kv["implicit_external_active"] == "true");
        }
        if (kv.count("implicit_diagnostic")) {
            mode.implicitDiagnostic = kv["implicit_diagnostic"];
        }
        if (kv.count("implicit_quarantine_count")
            && !parseRecordedUint64(kv["implicit_quarantine_count"], mode.implicitQuarantineCount))
            return std::nullopt;
    }
    if (version >= 13) {
        std::uint32_t tuningPresent = 0U;
        if (!kv.count("tuning_present") || !parseUint32(kv["tuning_present"], tuningPresent) || tuningPresent > 1U) {
            return std::nullopt;
        }
        mode.presentationTuning.hasTuning = tuningPresent != 0U;
        if (mode.presentationTuning.hasTuning) {
            if (!kv.count("tuning_fog_density") || !parseFloat(kv["tuning_fog_density"], mode.presentationTuning.fogDensity) ||
                !kv.count("tuning_emission_threshold") || !parseFloat(kv["tuning_emission_threshold"], mode.presentationTuning.emissionThreshold) ||
                !kv.count("tuning_emission_multiplier") || !parseFloat(kv["tuning_emission_multiplier"], mode.presentationTuning.emissionMultiplier)) {
                return std::nullopt;
            }
        }
    }
    if (version >= 14) {
        if (!kv.count("compact_generation")
            || !parseUint32(kv["compact_generation"], mode.compactGeneration)
            || !kv.count("world_generation")
            || !parseUint32(kv["world_generation"], mode.worldGeneration)) {
            return std::nullopt;
        }
    } else {
        // Older versions may omit these observations, but a recorded value
        // must not silently turn into default provenance when malformed.
        if ((kv.count("compact_generation")
             && !parseUint32(kv["compact_generation"], mode.compactGeneration))
            || (kv.count("world_generation")
                && !parseUint32(kv["world_generation"], mode.worldGeneration))) {
            return std::nullopt;
        }
    }
    if (version >= 15) {
        std::uint32_t outcomeKnown = 0U;
        std::uint32_t confirmed = 0U;
        if (!kv.count("presentation_frame_sequence")
            || !kv.count("selected_compact_generation")
            || !kv.count("selected_world_generation")
            || !kv.count("selected_canonical_feedback_generation")
            || !kv.count("selected_canonical_feedback_reset_epoch")
            || !kv.count("presentation_outcome")
            || !kv.count("presentation_outcome_known")
            || !kv.count("presentation_confirmed")
            || !parseUint32(kv["selected_compact_generation"], mode.selectedCompactGeneration)
            || !parseUint32(kv["selected_world_generation"], mode.selectedWorldGeneration)
            || !parseUint32(kv["presentation_outcome"], mode.presentationOutcome)
            || !parseRecordedUint64(kv["presentation_frame_sequence"], mode.presentationFrameSequence)
            || !parseRecordedUint64(kv["selected_canonical_feedback_generation"], mode.selectedCanonicalFeedbackGeneration)
            || !parseRecordedUint64(kv["selected_canonical_feedback_reset_epoch"], mode.selectedCanonicalFeedbackResetEpoch)
            // Persisted ordinal mirrors the value-only FrameOutcome contract,
            // which now includes three submitted outcomes (4..6) and recovery.
            || mode.presentationOutcome > 8U
            || !parseUint32(kv["presentation_outcome_known"], outcomeKnown)
            || !parseUint32(kv["presentation_confirmed"], confirmed)
            || outcomeKnown > 1U || confirmed > 1U) {
            return std::nullopt;
        }
        mode.presentationOutcomeKnown = outcomeKnown != 0U;
        mode.presentationConfirmed = confirmed != 0U;
        if (mode.presentationConfirmed && !mode.presentationOutcomeKnown) {
            return std::nullopt;
        }
    }
    if (version >= 16) {
        if (!kv.count("selected_compact_production_receipt_sequence")
            || !kv.count("selected_world_production_receipt_sequence")
            || !kv.count("selected_canonical_feedback_production_receipt_sequence")
            || !parseRecordedUint64(
                kv["selected_compact_production_receipt_sequence"],
                mode.selectedCompactProductionReceiptSequence)
            || !parseRecordedUint64(
                kv["selected_world_production_receipt_sequence"],
                mode.selectedWorldProductionReceiptSequence)
            || !parseRecordedUint64(
                kv["selected_canonical_feedback_production_receipt_sequence"],
                mode.selectedCanonicalFeedbackProductionReceiptSequence)) {
            return std::nullopt;
        }
    }
    if (version >= 17) {
        if (!kv.count("implicit_generation")
            || !kv.count("selected_implicit_generation")
            || !kv.count("selected_implicit_production_receipt_sequence")
            || !parseUint32(kv["implicit_generation"], mode.implicitGeneration)
            || !parseUint32(
                kv["selected_implicit_generation"], mode.selectedImplicitGeneration)
            || !parseRecordedUint64(
                kv["selected_implicit_production_receipt_sequence"],
                mode.selectedImplicitProductionReceiptSequence)) {
            return std::nullopt;
        }
    }
    if (version >= 18) {
        if (!kv.count("presentation_device_generation")
            || !kv.count("presentation_surface_generation")
            || !parseRecordedUint64(
                kv["presentation_device_generation"],
                mode.presentationDeviceGeneration)
            || !parseRecordedUint64(
                kv["presentation_surface_generation"],
                mode.presentationSurfaceGeneration)) {
            return std::nullopt;
        }
    }
    if (version >= 19) {
        const auto it = kv.find("capture_submission_serial");
        if (it == kv.end() || !parseRecordedUint64(it->second, mode.captureSubmissionSerial)) {
            return std::nullopt;
        }
        if (mode.captureSubmissionSerial != 0U
            && (!mode.presentationOutcomeKnown || mode.presentationFrameSequence == 0U
                || mode.presentationDeviceGeneration == 0U || mode.presentationSurfaceGeneration == 0U
                || mode.presentationOutcome < 4U || mode.presentationOutcome > 6U
                || (mode.presentationConfirmed && mode.presentationOutcome != 4U))) {
            return std::nullopt;
        }
    }
    if ((kv.count("world_version")
         && !parseInt(kv["world_version"], mode.state.version))
        || (kv.count("world_time")
            && !parseFloat(kv["world_time"], mode.state.time))
        || (kv.count("world_morph")
            && !parseFloat(kv["world_morph"], mode.state.morph))
        || (kv.count("world_cameraAngle")
            && !parseFloat(kv["world_cameraAngle"], mode.state.cameraAngle))) {
        return std::nullopt;
    }

    if ((kv.count("world_cameraEye") && !parseVec3Array(kv["world_cameraEye"], mode.state.cameraEye))
        || (kv.count("world_cameraTarget") && !parseVec3Array(kv["world_cameraTarget"], mode.state.cameraTarget))
        || (kv.count("world_cameraUp") && !parseVec3Array(kv["world_cameraUp"], mode.state.cameraUp))) {
        return std::nullopt;
    }
    if (version < 7) {
        // Legacy WorldMode used nonzero coordinates as its camera-override
        // signal. Preserve that historical behavior when importing it.
        mode.state.hasCameraOverride = mode.state.cameraEye[0] != 0.0f
            || mode.state.cameraEye[1] != 0.0f || mode.state.cameraEye[2] != 0.0f
            || mode.state.cameraTarget[0] != 0.0f || mode.state.cameraTarget[1] != 0.0f
            || mode.state.cameraTarget[2] != 0.0f;
    }

    if (version >= 3) {
        // v3 declares the routing map explicitly so a truncated capture
        // cannot silently load with different live modulation behavior.
        if (!kv.count("routing_count")) return std::nullopt;
        std::uint32_t routeCount = 0U;
        if (!parseUint32(kv["routing_count"], routeCount)
            || routeCount > 256U) {
            return std::nullopt;
        }
        mode.routingMap.reserve(routeCount);
        for (std::size_t index = 0; index < routeCount; ++index) {
            const auto key = "routing_" + std::to_string(index);
            if (!kv.count(key)) return std::nullopt;
            mode.routingMap.push_back(kv[key]);
        }
    }

    // Missing legacy fields retain defaults. A declared controller must not
    // silently become a different program when one of its fields is malformed.
    const auto readMorphBool = [&kv](const char* key, bool& value) {
        const auto field = kv.find(key);
        if (field == kv.end()) return true;
        if (field->second != "true" && field->second != "false") return false;
        value = field->second == "true";
        return true;
    };
    if (!readMorphBool("morphspace_active", mode.morphSpace.active)
        || !readMorphBool("morphspace_path_looping", mode.morphSpace.pathLooping)
        || (kv.count("morphspace_motion")
            && !parseContiguousEnum(kv["morphspace_motion"], MorphSpaceMotion::Path,
                                    mode.morphSpace.motionMode))
        || (kv.count("morphspace_speed")
            && !parseFloat(kv["morphspace_speed"], mode.morphSpace.motionSpeed))
        || (kv.count("morphspace_power")
            && !parseFloat(kv["morphspace_power"], mode.morphSpace.shepardPower))) {
        return std::nullopt;
    }
    if (kv.count("morphspace_cursor")) {
        const auto& cursor = kv["morphspace_cursor"];
        const auto comma = cursor.find(',');
        float cur[2]{};
        if (comma == std::string::npos || cursor.find(',', comma + 1) != std::string::npos
            || !parseFloat(cursor.substr(0, comma), cur[0])
            || !parseFloat(cursor.substr(comma + 1), cur[1])) {
            return std::nullopt;
        }
        mode.morphSpace.cursorX = std::clamp(cur[0], 0.0f, 1.0f);
        mode.morphSpace.cursorY = std::clamp(cur[1], 0.0f, 1.0f);
    }
    if (kv.count("morphspace_bank")) {
        mode.morphSpace.activeBankName = kv["morphspace_bank"];
    }
    if (kv.count("morphspace_waypoint_count")) {
        int wpCount = 0;
        if (!parseInt(kv["morphspace_waypoint_count"], wpCount) || wpCount < 0 || wpCount > 128) {
            return std::nullopt;
        }
        mode.morphSpace.waypoints.reserve(wpCount);
        for (int i = 0; i < wpCount; ++i) {
            const auto row = kv.find("morphspace_wp_" + std::to_string(i));
            if (row == kv.end()) return std::nullopt;
            std::stringstream ss(row->second);
            std::string token;
            SculptureMorphSpaceState::WaypointState wp{};
            for (float* component : {&wp.x, &wp.y, &wp.dwellSeconds, &wp.travelSeconds}) {
                if (!std::getline(ss, token, ',') || !parseFloat(token, *component)) {
                    return std::nullopt;
                }
            }
            // The tag remains optional, with the existing comma-delimited policy.
            std::getline(ss, wp.tag, ',');
            mode.morphSpace.waypoints.push_back(std::move(wp));
        }
    }

    const auto params = deserialize(payload);
    if (!params) return std::nullopt;
    mode.params = *params;
    return mode;
}

bool SculptureSerializer::saveWorldModeToFile(const std::string& filepath, const SculptureWorldMode& mode) {
    return file_publication_detail::publishTextFile(filepath, [&] { return serializeWorldMode(mode); });
}

std::optional<SculptureWorldMode> SculptureSerializer::loadWorldModeFromFile(const std::string& filepath) {
    return loadWorldModeFromFile(filepath, StateLoadLimits{});
}

std::optional<SculptureWorldMode> SculptureSerializer::loadWorldModeFromFile(
    const std::string& filepath, const StateLoadLimits& limits) {
    const auto content = readStateFile(filepath, limits);
    return content ? deserializeWorldMode(*content) : std::nullopt;
}

std::string SculptureSerializer::serializeEpimode(const SculptureWorldMode& mode) {
    std::ostringstream out;
    out << "{\n";
    out << "  \"version\": 1,\n";
    out << "  \"preset\": \"" << (mode.presetName.empty() ? "captured_moment" : mode.presetName) << "\",\n";
    if (!mode.presetRevision.empty()) {
        out << "  \"preset_revision\": \"" << mode.presetRevision << "\",\n";
    }
    if (mode.presentationOwnership != SculpturePresentationOwnership::CompactInstances) {
        out << "  \"presentation\": \""
            << (mode.presentationOwnership == SculpturePresentationOwnership::UnifiedCompositeField ? "composite" : mode.presentationOwnership == SculpturePresentationOwnership::ImplicitField
                    ? "implicit"
                    : mode.presentationOwnership == SculpturePresentationOwnership::DedicatedWorld
                        ? "world" : "canonical")
            << "\",\n";
    }
    out << "  \"time\": " << mode.state.time << ",\n";
    out << "  \"morph\": " << mode.state.morph << ",\n";
    out << "  \"cameraAngle\": " << mode.state.cameraAngle << ",\n";
    out << "  \"camera_override\": " << (mode.state.hasCameraOverride ? 1 : 0) << ",\n";
    out << "  \"cameraEye\": [" << mode.state.cameraEye[0] << ", " << mode.state.cameraEye[1] << ", " << mode.state.cameraEye[2] << "],\n";
    out << "  \"cameraTarget\": [" << mode.state.cameraTarget[0] << ", " << mode.state.cameraTarget[1] << ", " << mode.state.cameraTarget[2] << "],\n";
    out << "  \"cameraUp\": [" << mode.state.cameraUp[0] << ", " << mode.state.cameraUp[1] << ", " << mode.state.cameraUp[2] << "],\n";
    out << "  \"inner\": \"" << name(mode.params.inner) << "\",\n";
    out << "  \"innerB\": \"" << name(mode.params.innerB) << "\",\n";
    out << "  \"reduction\": \"" << name(mode.params.reduction) << "\",\n";
    out << "  \"reductionB\": \"" << name(mode.params.reductionB) << "\",\n";
    out << "  \"seedType\": \"" << name(mode.params.seedType) << "\",\n";
    out << "  \"seedTypeB\": \"" << name(mode.params.seedTypeB) << "\",\n";
    out << "  \"colorEffect\": \"" << name(mode.params.colorEffect) << "\",\n";
    out << "  \"pre_scaleReal\": " << mode.params.pre.scaleReal << ",\n";
    out << "  \"pre_scaleImag\": " << mode.params.pre.scaleImag << ",\n";
    out << "  \"pre_offsetReal\": " << mode.params.pre.offsetReal << ",\n";
    out << "  \"pre_offsetImag\": " << mode.params.pre.offsetImag << ",\n";
    out << "  \"post_scaleReal\": " << mode.params.post.scaleReal << ",\n";
    out << "  \"post_scaleImag\": " << mode.params.post.scaleImag << ",\n";
    out << "  \"post_offsetReal\": " << mode.params.post.offsetReal << ",\n";
    out << "  \"post_offsetImag\": " << mode.params.post.offsetImag << ",\n";
    
    for (const auto& meta : kUintParams) {
        out << "  \"" << meta.name << "\": " << mode.params.*meta.member << ",\n";
    }
    for (std::size_t i = 0; i < kFloatParams.size(); ++i) {
        const auto& meta = kFloatParams[i];
        out << "  \"" << meta.name << "\": " << mode.params.*meta.member;
        if (i + 1 < kFloatParams.size() || !mode.routingMap.empty()) {
            out << ",\n";
        } else {
            out << "\n";
        }
    }
    for (std::size_t i = 0; i < mode.routingMap.size(); ++i) {
        out << "  \"route_" << i << "\": \"" << mode.routingMap[i] << "\"";
        if (i + 1 < mode.routingMap.size() || !mode.macroAutomationJson.empty()) {
            out << ",\n";
        } else {
            out << "\n";
        }
    }
    if (!mode.macroAutomationJson.empty()) {
        if (mode.routingMap.empty() && kFloatParams.size() > 0) {
            out << ",\n";
        }
        out << "  \"macro_automation\": " << mode.macroAutomationJson << "\n";
    }
    out << "}\n";
    return out.str();
}

bool SculptureSerializer::saveEpimodeToFile(const std::string& filepath, const SculptureWorldMode& mode) {
    return file_publication_detail::publishTextFile(filepath, [&] { return serializeEpimode(mode); });
}

bool SculptureSerializer::deserializeSequence(const std::string& content, SculptureSequencer& sequencer) {
    return deserializeSequence(content, sequencer, {});
}

bool SculptureSerializer::deserializeSequence(
    const std::string& content,
    SculptureSequencer& sequencer,
    const SequencePresetResolver& resolver) {
    return deserializeSequence(content, sequencer, resolver, SequenceLoadLimits{});
}

bool SculptureSerializer::deserializeSequence(
    const std::string& content,
    SculptureSequencer& sequencer,
    const SequencePresetResolver& resolver,
    const SequenceLoadLimits& limits) {
    // Reject before copying text into the parser or building replacement values.
    if (limits.maxBytes == 0U || limits.maxKeyframes == 0U
        || content.size() > limits.maxBytes) return false;
    // The sequencer owns inherited playback policy; the codec owns admission.
    // Prepare without cloning the old timeline or its cached samples.
    auto prepared = sequencer.makeReplacementCandidate();
    float acceptedDuration = 0.0f;
    const auto admitDuration = [&](float duration) {
        if (prepared.keyframeCount() >= limits.maxKeyframes) return false;
        const float nextDuration = acceptedDuration + duration;
        // Playback/seek use float timeline time. Every accepted keyframe must
        // advance that representable domain rather than overflow or disappear.
        if (!std::isfinite(nextDuration) || nextDuration <= acceptedDuration) return false;
        acceptedDuration = nextDuration;
        return true;
    };
    std::istringstream stream(content);
    std::string line;

    auto parseEasing = [](const std::string& s) -> KeyframeEasing {
        if (s == "Linear" || s == "linear") return KeyframeEasing::Linear;
        if (s == "Smoothstep" || s == "smoothstep") return KeyframeEasing::Smoothstep;
        if (s == "Exponential" || s == "exponential") return KeyframeEasing::Exponential;
        if (s == "CubicHermite" || s == "hermite") return KeyframeEasing::CubicHermite;
        return KeyframeEasing::CatmullRom;
    };

    while (std::getline(stream, line)) {
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        if (line.empty() || line[0] == '#') continue;

        // Declarations configure the candidate even before its first keyframe;
        // decoding must not activate playback. Later declarations take precedence.
        if (line.rfind("LOOP", 0) == 0 || line.rfind("loop", 0) == 0) {
            std::istringstream iss(line);
            std::string cmd, val;
            if (iss >> cmd >> val) {
                bool shouldLoop = (val == "true" || val == "1" || val == "yes" || val == "on");
                prepared.setLooping(shouldLoop);
            }
            continue;
        }

        std::string workingLine = line;
        if (workingLine.rfind("PRESET ", 0) == 0 || workingLine.rfind("preset ", 0) == 0) {
            workingLine = workingLine.substr(7);
        } else if (workingLine.rfind("EPIMODE ", 0) == 0 || workingLine.rfind("epimode ", 0) == 0) {
            workingLine = workingLine.substr(8);
        }
        workingLine.erase(0, workingLine.find_first_not_of(" \t"));

        std::string targetToken;
        std::string remainder;

        if (!workingLine.empty() && workingLine[0] == '"') {
            auto endQuote = workingLine.find('"', 1);
            if (endQuote != std::string::npos) {
                targetToken = workingLine.substr(1, endQuote - 1);
                remainder = workingLine.substr(endQuote + 1);
            } else {
                targetToken = workingLine.substr(1);
            }
        } else {
            std::istringstream iss(workingLine);
            iss >> targetToken;
            std::getline(iss, remainder);
        }

        float duration = 3.0f;
        std::string easingStr;
        if (!remainder.empty()) {
            std::istringstream remIss(remainder);
            std::string durationToken;
            if (remIss >> durationToken) {
                if (!parseFloat(durationToken, duration)
                    || duration < SculptureSequencer::kMinimumKeyframeDuration) return false;
                remIss >> easingStr;
            }
        }

        KeyframeEasing easing = parseEasing(easingStr);

        const bool isDigitOnly = !targetToken.empty()
            && std::all_of(targetToken.begin(), targetToken.end(),
                [](unsigned char ch) { return ch >= '0' && ch <= '9'; });
        if (isDigitOnly) {
            std::size_t idx = 0;
            const auto end = targetToken.data() + targetToken.size();
            const auto parsed = std::from_chars(targetToken.data(), end, idx);
            if (parsed.ec != std::errc{} || parsed.ptr != end) return false;
            if (idx < sculpturePresets().size()) {
                if (!admitDuration(duration)) return false;
                prepared.addKeyframe(idx, duration, easing);
                continue;
            }
        }

        if (resolver) {
            SculptureProgramParams params{};
            SculptureWorldState state{};
            std::vector<std::string> routingMap;
            if (resolver(targetToken, params, state, routingMap)) {
                if (!admitDuration(duration)) return false;
                prepared.addKeyframe(params, state, duration, easing, routingMap);
                continue;
            }
        }

        bool foundPreset = false;
        for (std::size_t i = 0; i < sculpturePresets().size(); ++i) {
            if (sculpturePresets()[i].name == targetToken) {
                if (!admitDuration(duration)) return false;
                prepared.addKeyframe(i, duration, easing);
                foundPreset = true;
                break;
            }
        }
        if (foundPreset) continue;

        if (targetToken.find(".epimode") != std::string::npos) {
            SculptureWorldState mode;
            std::string presetName;
            SculptureProgramParams params;
            std::vector<std::string> routingMap;
            if (SculptureSerializer::loadEpimodeFromFile(targetToken, mode, presetName, params, routingMap)) {
                if (!admitDuration(duration)) return false;
                prepared.addKeyframe(params, mode, duration, easing, routingMap);
            }
        }
    }
    if (prepared.keyframeCount() == 0) return false;
    static_assert(std::is_nothrow_move_assignable_v<SculptureSequencer>);
    sequencer = std::move(prepared);
    return true;
}

bool SculptureSerializer::loadSequenceFromFile(const std::string& filepath, SculptureSequencer& sequencer) {
    return loadSequenceFromFile(filepath, sequencer, {});
}

bool SculptureSerializer::loadSequenceFromFile(
    const std::string& filepath,
    SculptureSequencer& sequencer,
    const SequencePresetResolver& resolver) {
    return loadSequenceFromFile(filepath, sequencer, resolver, SequenceLoadLimits{});
}

bool SculptureSerializer::loadSequenceFromFile(
    const std::string& filepath,
    SculptureSequencer& sequencer,
    const SequencePresetResolver& resolver,
    const SequenceLoadLimits& limits) {
    if (limits.maxBytes == 0U || limits.maxKeyframes == 0U) return false;
    std::string content;
    try {
        content = epi::io::readBoundedText(filepath, limits.maxBytes, "sequence");
    } catch (const std::runtime_error&) {
        return false;
    }
    return deserializeSequence(content, sequencer, resolver, limits);
}

std::optional<SculptureDecodedPreset> SculptureSerializer::decodePresetContent(
    const std::string& content, std::string_view fallbackPresetName) {
    return decodePresetContentDetailed(content, fallbackPresetName).preset;
}

SculpturePresetDecodeResult SculptureSerializer::decodePresetContentDetailed(
    const std::string& content, std::string_view fallbackPresetName) {
    SculpturePresetDecodeResult result;
    SculptureDecodedPreset candidate;
    if (!decodeEpimodeInto(content, fallbackPresetName, candidate.state, candidate.presetName,
            candidate.params, candidate.routingMap, &candidate.presentationOwnership,
            &candidate.macroAutomationJson, &candidate.declaredWorldRecipe,
            &result.diagnostic)) return result;
    result.preset = std::move(candidate);
    return result;
}

bool SculptureSerializer::deserializeEpimode(
    const std::string& content, std::string_view fallbackPresetName,
    SculptureWorldState& outMode, std::string& outPresetName,
    SculptureProgramParams& outParams, std::vector<std::string>& outRoutingMap,
    SculpturePresentationOwnership* outOwnership, std::string* outMacroJson) {
    return decodeEpimodeInto(content, fallbackPresetName, outMode, outPresetName,
        outParams, outRoutingMap, outOwnership, outMacroJson, nullptr, nullptr);
}

bool SculptureSerializer::decodeEpimodeInto(
    const std::string& content, std::string_view fallbackPresetName,
    SculptureWorldState& outMode, std::string& outPresetName,
    SculptureProgramParams& outParams, std::vector<std::string>& outRoutingMap,
    SculpturePresentationOwnership* outOwnership, std::string* outMacroJson,
    std::optional<WorldRecipe>* outRecipe,
    SculpturePresetDecodeDiagnostic* diagnostic) {
    if (diagnostic) *diagnostic = {};
    const auto reject = [&](SculpturePresetDecodeError error, std::string_view field = {}) {
        if (diagnostic) {
            diagnostic->error = error;
            diagnostic->field.assign(field);
        }
        return false;
    };
    if (outOwnership) *outOwnership = SculpturePresentationOwnership::CompactInstances;
    if (outMacroJson) outMacroJson->clear();
    if (outRecipe) outRecipe->reset();

    const auto fields = selectPresetJsonFields(content);
    // Syntax selects the format; field admission selects validity. Keep them
    // distinct so marker text inside a complete JSON object cannot choose the
    // WorldMode decoder, even when that object's authored fields are invalid.
    if (content.find("worldmode_version=") != std::string::npos && !fields.syntaxValid) {
        auto worldMode = deserializeWorldMode(content);
        if (!worldMode) return reject(SculpturePresetDecodeError::InvalidWorldMode);
        outMode = worldMode->state;
        outPresetName = std::move(worldMode->presetName);
        outParams = worldMode->params;
        outRoutingMap = std::move(worldMode->routingMap);
        if (outOwnership) *outOwnership = worldMode->presentationOwnership;
        if (outMacroJson) *outMacroJson = std::move(worldMode->macroAutomationJson);
        if (outRecipe && worldMode->hasResolvedWorldRecipe) *outRecipe = worldMode->resolvedWorldRecipe;
        return true;
    }
    
    if (!fields.admitted) return reject(fields.error, fields.errorField);
    const auto findValueStart = [&](std::string_view key) -> std::optional<std::size_t> {
        const auto found = fields.values.find(key);
        if (found == fields.values.end()) return std::nullopt;
        return static_cast<std::size_t>(found->second.data() - content.data());
    };

    const auto readStringAt = [&](std::size_t pos, std::optional<std::string>& value) {
        if (pos == content.size() || content[pos++] != '"') return false;
        std::string parsed;
        while (pos < content.size()) {
            const char ch = content[pos++];
            if (ch == '"') {
                while (pos < content.size()
                       && std::isspace(static_cast<unsigned char>(content[pos]))) ++pos;
                if (pos == content.size() || (content[pos] != ',' && content[pos] != '}')) return false;
                value = std::move(parsed);
                return true;
            }
            if (static_cast<unsigned char>(ch) < 0x20) return false;
            if (ch != '\\') {
                parsed += ch;
                continue;
            }
            if (pos == content.size()) return false;
            // Decode bounded JSON escapes; Unicode escapes need a separate codec
            // contract and are rejected rather than silently changing a selector.
            switch (content[pos++]) {
                case '"': parsed += '"'; break;
                case '\\': parsed += '\\'; break;
                case '/': parsed += '/'; break;
                case 'b': parsed += '\b'; break;
                case 'f': parsed += '\f'; break;
                case 'n': parsed += '\n'; break;
                case 'r': parsed += '\r'; break;
                case 't': parsed += '\t'; break;
                default: return false;
            }
        }
        return false;
    };
    const auto readString = [&](std::string_view key, std::optional<std::string>& value) {
        const auto start = findValueStart(key);
        if (start && !readStringAt(*start, value))
            return reject(SculpturePresetDecodeError::InvalidString, key);
        return true;
    };

    auto extractNumString = [&](std::string_view key) -> std::optional<std::string> {
        const auto pos = findValueStart(key);
        if (!pos) return std::nullopt;
        auto endPos = content.find_first_of(",}]", *pos);
        if (endPos == std::string::npos) endPos = content.size();
        while (endPos > *pos && std::isspace(static_cast<unsigned char>(content[endPos - 1]))) --endPos;
        // Keep exponents and trailing junk intact: checked conversion must
        // decide whether the whole value is valid, never accept its prefix.
        return content.substr(*pos, endPos - *pos);
    };

    const auto readFloat = [&](std::string_view key, float& value) {
        const auto text = extractNumString(key);
        if (text && !parseFloat(*text, value))
            return reject(SculpturePresetDecodeError::InvalidNumber, key);
        return true;
    };
    const auto readUint = [&](std::string_view key, std::uint32_t& value) {
        const auto text = extractNumString(key);
        if (text && (text->empty() || text->front() == '-' || !parseUint32(*text, value)))
            return reject(SculpturePresetDecodeError::InvalidNumber, key);
        return true;
    };

    // -----------------------------------------------------------------------------
    // Educational Annotation: Generic State Deserialization Engine
    // -----------------------------------------------------------------------------
    // Legacy .epimode files are parsed here using the centralized Reflection Registry
    // (kFloatParams / kUintParams) instead of brittle hardcoded JSON string maps.
    // Missing fields preserve defaults; malformed present fields reject the asset.
    // -----------------------------------------------------------------------------
    
    // Admit the complete set of scalar strings before applying their existing
    // precedence. Even a lower-priority present field must be well formed.
    std::optional<std::string> preset, presetId, title, id, presentation;
    std::optional<std::string> inner, innerB, reduction, reductionB, seedType, seedTypeB, colorEffect;
    if (!readString("preset", preset) || !readString("presetId", presetId)
        || !readString("title", title) || !readString("id", id)
        || !readString("presentation", presentation)
        || !readString("inner", inner) || !readString("innerB", innerB)
        || !readString("reduction", reduction) || !readString("reductionB", reductionB)
        || !readString("seedType", seedType) || !readString("seedTypeB", seedTypeB)
        || !readString("colorEffect", colorEffect)) return false;

    if (auto s = extractNumString("version"); s && !parseInt(*s, outMode.version))
        return reject(SculpturePresetDecodeError::InvalidNumber, "version");
    if (preset) outPresetName = *preset;
    else if (presetId) outPresetName = *presetId;
    else if (title) outPresetName = *title;
    else if (id) outPresetName = *id;
    else {
        outPresetName = std::string(fallbackPresetName);
    }
    if (const auto& s = presentation) {
        if (*s == "composite") { if (outOwnership) *outOwnership = SculpturePresentationOwnership::UnifiedCompositeField; }
        else if (*s == "implicit") { if (outOwnership) *outOwnership = SculpturePresentationOwnership::ImplicitField; }
        else if (*s == "world") { if (outOwnership) *outOwnership = SculpturePresentationOwnership::DedicatedWorld; }
        else if (*s == "canonical" || *s == "relief") {
            if (outOwnership) *outOwnership = SculpturePresentationOwnership::CanonicalFeedback;
        } else if (*s == "compact") {
            if (outOwnership) *outOwnership = SculpturePresentationOwnership::CompactInstances;
        } else {
            // Presentation chooses the OpenCL producer contract. Treating a
            // typo as compact would render plausible but mathematically wrong
            // output, which is much harder to diagnose than a rejected asset.
            return reject(SculpturePresetDecodeError::UnknownPresentation, "presentation");
        }
    }
    if (!readFloat("time", outMode.time) || !readFloat("morph", outMode.morph)
        || !readFloat("cameraAngle", outMode.cameraAngle)) return false;

    const auto readVec3 = [&](std::string_view key, float (&arr)[3],
                              bool* present = nullptr) -> bool {
        const auto start = findValueStart(key);
        if (present) *present = start.has_value();
        if (!start) return true; // Missing fields retain the caller's values.
        auto pos = *start;
        if (pos == content.size() || content[pos] != '[')
            return reject(SculpturePresetDecodeError::InvalidVector, key);
        ++pos;
        float parsed[3]{};
        for (std::size_t index = 0; index < 3; ++index) {
            const auto delimiter = content.find_first_of(",]", pos);
            if (delimiter == std::string::npos
                || content[delimiter] != (index < 2 ? ',' : ']'))
                return reject(SculpturePresetDecodeError::InvalidVector, key);
            auto tokenEnd = delimiter;
            while (tokenEnd > pos && std::isspace(static_cast<unsigned char>(content[tokenEnd - 1]))) --tokenEnd;
            if (!parseFloat(content.substr(pos, tokenEnd - pos), parsed[index]))
                return reject(SculpturePresetDecodeError::InvalidVector, key);
            pos = delimiter + 1;
        }
        while (pos < content.size() && std::isspace(static_cast<unsigned char>(content[pos]))) ++pos;
        if (pos < content.size() && content[pos] != ',' && content[pos] != '}')
            return reject(SculpturePresetDecodeError::InvalidVector, key);
        // A rejected component must never leave a partially restored camera vector.
        for (std::size_t index = 0; index < 3; ++index) arr[index] = parsed[index];
        return true;
    };

    if (auto s = extractNumString("camera_override")) {
        int overrideValue = 0;
        if (!parseInt(*s, overrideValue))
            return reject(SculpturePresetDecodeError::InvalidNumber, "camera_override");
        outMode.hasCameraOverride = overrideValue != 0;
    }
    bool cameraEyePresent = false;
    if (!readVec3("cameraEye", outMode.cameraEye, &cameraEyePresent)
        || !readVec3("cameraTarget", outMode.cameraTarget)
        || !readVec3("cameraUp", outMode.cameraUp)) return false;
    if (cameraEyePresent) {
        if (!outMode.hasCameraOverride) {
            outMode.hasCameraOverride = (outMode.cameraEye[0] != 0.0f || outMode.cameraEye[1] != 0.0f || outMode.cameraEye[2] != 0.0f);
        }
    }

    auto normalizeToken = [](std::string_view s) -> std::string {
        std::string res;
        res.reserve(s.size());
        for (char c : s) {
            if (c != '_' && c != ' ' && c != '-') {
                res.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
            }
        }
        return res;
    };

    auto parseInner = [&](const std::string& n) -> std::optional<InnerMap> {
        const std::string norm = normalizeToken(n);
        if (norm.find("jacobi") != std::string::npos) return InnerMap::JacobiEllipticSnCn;
        if (norm.find("turing") != std::string::npos) return InnerMap::ReactionDiffusionTuring;
        if (norm.find("homotopy") != std::string::npos) return InnerMap::HyperbolicExponentialHomotopy;
        if (norm.find("mandelbox") != std::string::npos) return InnerMap::HyperbolicMandelboxFold;
        if (norm.find("quatjulia") != std::string::npos) return InnerMap::QuaternionJulia;
        if (norm.find("quatmandel") != std::string::npos) return InnerMap::QuaternionMandelbrot;
        if (norm == "hopffibration" || norm == "hopf") return InnerMap::CliffordHopf;
        if (norm == "kleiniangroup") return InnerMap::KleinProjective;
        if (norm == "mandelbulb" || norm == "hypermandelbulb") return InnerMap::HyperbolicHoneycomb;
        for (int i = 0; i <= static_cast<int>(InnerMap::KaliSetOrbit); ++i) {
            const std::string cand = normalizeToken(name(static_cast<InnerMap>(i)));
            if (cand == norm) return static_cast<InnerMap>(i);
        }
        for (int i = 0; i <= static_cast<int>(InnerMap::KaliSetOrbit); ++i) {
            const std::string cand = normalizeToken(name(static_cast<InnerMap>(i)));
            if (norm.size() >= 4 && (norm.find(cand) != std::string::npos || cand.find(norm) != std::string::npos)) {
                return static_cast<InnerMap>(i);
            }
        }
        return std::nullopt;
    };
    auto parseReduction = [&](const std::string& n) -> std::optional<ReductionMode> {
        const std::string norm = normalizeToken(n);
        if (norm.find("ammann") != std::string::npos || norm.find("silverratio") != std::string::npos) return ReductionMode::AmmannBeenker;
        if (norm.find("poincare") != std::string::npos) return ReductionMode::PoincareHyperbolic;
        if (norm.find("clifford") != std::string::npos || norm.find("foliation") != std::string::npos) return ReductionMode::CliffordFoliation;
        if (norm.find("pareidolia") != std::string::npos || norm.find("cathedral") != std::string::npos) return ReductionMode::PareidoliaCathedral;
        if (norm.find("clamp") != std::string::npos) return ReductionMode::Clamp;
        if (norm.find("grid") != std::string::npos) return ReductionMode::Grid;
        if (norm.find("torus") != std::string::npos) return ReductionMode::Torus;
        for (int i = 0; i <= static_cast<int>(ReductionMode::CliffordFoliation); ++i) {
            const std::string cand = normalizeToken(name(static_cast<ReductionMode>(i)));
            if (cand == norm) return static_cast<ReductionMode>(i);
        }
        for (int i = 0; i <= static_cast<int>(ReductionMode::CliffordFoliation); ++i) {
            const std::string cand = normalizeToken(name(static_cast<ReductionMode>(i)));
            if (norm.size() >= 4 && (cand.find(norm) != std::string::npos || norm.find(cand) != std::string::npos)) return static_cast<ReductionMode>(i);
        }
        return std::nullopt;
    };
    auto parseSeed = [&](const std::string& n) -> std::optional<SeedType> {
        const std::string norm = normalizeToken(n);
        for (int i = 0; i <= static_cast<int>(SeedType::QuantumProbabilityCloud); ++i) {
            const std::string cand = normalizeToken(name(static_cast<SeedType>(i)));
            if (cand == norm) return static_cast<SeedType>(i);
        }
        for (int i = 0; i <= static_cast<int>(SeedType::QuantumProbabilityCloud); ++i) {
            const std::string cand = normalizeToken(name(static_cast<SeedType>(i)));
            if (norm.size() >= 4 && (norm.find(cand) != std::string::npos || cand.find(norm) != std::string::npos)) {
                return static_cast<SeedType>(i);
            }
        }
        return std::nullopt;
    };
    auto parseColor = [&](const std::string& n) -> std::optional<ColorEffectType> {
        const std::string norm = normalizeToken(n);
        if (norm == "quatcurvature") return ColorEffectType::QuaternionCurvature;
        if (norm == "neuralsynapticfiring") return ColorEffectType::Bioluminescence;
        if (norm == "phasecoherence") return ColorEffectType::KineticVelocity;
        if (norm.find("thinfilm") != std::string::npos) return ColorEffectType::ThinFilmIridescence;
        if (norm == "livingpaletteagent" || norm == "livingpalette") return ColorEffectType::LivingPaletteAgent;
        for (int i = 0; i <= static_cast<int>(ColorEffectType::ChladniAcoustic); ++i) {
            const std::string cand = normalizeToken(name(static_cast<ColorEffectType>(i)));
            if (cand == norm) return static_cast<ColorEffectType>(i);
        }
        for (int i = 0; i <= static_cast<int>(ColorEffectType::ChladniAcoustic); ++i) {
            const std::string cand = normalizeToken(name(static_cast<ColorEffectType>(i)));
            if (norm.size() >= 4 && (norm.find(cand) != std::string::npos || cand.find(norm) != std::string::npos)) {
                return static_cast<ColorEffectType>(i);
            }
        }
        return std::nullopt;
    };

    const auto readSelector = [&](std::string_view key, const std::optional<std::string>& text,
                                  auto& value, const auto& resolve) {
        if (!text) return true;
        const auto parsed = resolve(*text);
        // An explicit default algorithm is valid; failure to resolve authored
        // input must not silently substitute that default's mathematics.
        if (!parsed) return reject(SculpturePresetDecodeError::UnknownSelector, key);
        value = *parsed;
        return true;
    };
    if (!readSelector("inner", inner, outParams.inner, parseInner)
        || !readSelector("innerB", innerB, outParams.innerB, parseInner)
        || !readSelector("reduction", reduction, outParams.reduction, parseReduction)
        || !readSelector("reductionB", reductionB, outParams.reductionB, parseReduction)
        || !readSelector("seedType", seedType, outParams.seedType, parseSeed)
        || !readSelector("seedTypeB", seedTypeB, outParams.seedTypeB, parseSeed)
        || !readSelector("colorEffect", colorEffect, outParams.colorEffect, parseColor)) return false;
    if (!reductionB) outParams.reductionB = outParams.reduction;

    if (!readFloat("pre_scaleReal", outParams.pre.scaleReal)
        || !readFloat("pre_scaleImag", outParams.pre.scaleImag)
        || !readFloat("pre_offsetReal", outParams.pre.offsetReal)
        || !readFloat("pre_offsetImag", outParams.pre.offsetImag)
        || !readFloat("post_scaleReal", outParams.post.scaleReal)
        || !readFloat("post_scaleImag", outParams.post.scaleImag)
        || !readFloat("post_offsetReal", outParams.post.offsetReal)
        || !readFloat("post_offsetImag", outParams.post.offsetImag)) return false;

    // Universal Parameter Deserialization via Reflection Registry
    for (const auto& meta : kUintParams) {
        if (!readUint(meta.name, outParams.*meta.member)) return false;
    }
    
    for (const auto& meta : kFloatParams) {
        if (!readFloat(meta.name, outParams.*meta.member)) return false;
    }
    
    // Educational & Fault-Tolerant Normalization:
    // Some presets specify thin-film thickness in micrometers (e.g. 0.5 um = 500 nm).
    // The OpenCL optical interference kernel and program parameter validation contract
    // expect physical nanometers in [100.0, 1200.0]. Scale up gracefully.
    if (outParams.filmThickness > 0.0f && outParams.filmThickness < 10.0f) {
        outParams.filmThickness *= 1000.0f;
    }
    if (outParams.filmThickness < 100.0f) {
        outParams.filmThickness = 480.0f;
    } else if (outParams.filmThickness > 1200.0f) {
        outParams.filmThickness = 1200.0f;
    }
    
    // Selection already fixed source order and whole-map precedence. Decoding
    // retains the compatibility adapter's append-to-caller policy.
    for (const auto& [key, value] : fields.routes) {
        std::optional<std::string> route;
        if (!readStringAt(static_cast<std::size_t>(value.data() - content.data()), route))
            return reject(SculpturePresetDecodeError::InvalidRoute, key);
        outRoutingMap.push_back(std::move(*route));
    }

    if (outMacroJson && fields.macro) outMacroJson->assign(*fields.macro);
    return true;
}

bool SculptureSerializer::loadEpimodeFromFile(
    const std::string& path, SculptureWorldState& outMode, std::string& outPresetName,
    SculptureProgramParams& outParams, std::vector<std::string>& outRoutingMap,
    SculpturePresentationOwnership* outOwnership, std::string* outMacroJson) {
    return loadEpimodeFromFile(path, outMode, outPresetName, outParams, outRoutingMap,
                               outOwnership, outMacroJson, StateLoadLimits{});
}

bool SculptureSerializer::loadEpimodeFromFile(
    const std::string& path, SculptureWorldState& outMode, std::string& outPresetName,
    SculptureProgramParams& outParams, std::vector<std::string>& outRoutingMap,
    SculpturePresentationOwnership* outOwnership, std::string* outMacroJson,
    const StateLoadLimits& limits) {
    // Keep optional outputs' historical defaults even when acquisition fails.
    if (outOwnership) *outOwnership = SculpturePresentationOwnership::CompactInstances;
    if (outMacroJson) outMacroJson->clear();
    const auto content = readStateFile(path, limits);
    if (!content) return false;
    return deserializeEpimode(*content, std::filesystem::path(path).stem().string(),
                              outMode, outPresetName, outParams, outRoutingMap,
                              outOwnership, outMacroJson);
}

bool SculptureSerializer::loadEpimodeFromFile(const std::string& path, SculptureWorldState& outMode, std::string& outPresetName, SculptureProgramParams& outParams, std::vector<std::string>& outRoutingMap, SculpturePresentationOwnership* outOwnership) {
    return loadEpimodeFromFile(path, outMode, outPresetName, outParams, outRoutingMap, outOwnership, nullptr);
}


} // namespace epi_sculpt
