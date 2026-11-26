#pragma once

#include "epi/io/bounded_text.hpp"
#include "SculptureCaptureStereo.h"

#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace epi_sculpt {

// Cold host-document budget shared by capture publication and reference loading.
// This is not a field/checkpoint budget or evidence of native presentation.
inline constexpr std::size_t kMaxCaptureManifestBytes = 64 * 1024;

// Version-1 canonical capture field extent is bounded independently of the
// presentation image extent. The recorder and read-only inspector share it.
inline constexpr std::uint32_t kMaxCanonicalCaptureFieldExtent = 16384U;

// Serialized producer input, distinct from the field/checkpoint byte budget.
inline constexpr std::size_t kMaxCanonicalCaptureModeBytes = 4U * 1024U * 1024U;

// Encode optional recipe metadata as bytes so embedded newlines, NULs and '='
// cannot introduce manifest fields. Oversized or empty values are omitted;
// optional inspection metadata must not make an otherwise valid capture fail.
inline constexpr std::size_t kMaxCaptureRecipeMetadataBytes = 1024;

[[nodiscard]] inline std::optional<std::string> encodeCaptureRecipeMetadata(std::string_view value) {
    if (value.empty() || value.size() > kMaxCaptureRecipeMetadataBytes) return std::nullopt;
    constexpr char digits[] = "0123456789abcdef";
    std::string encoded;
    encoded.reserve(value.size() * 2);
    for (const unsigned char byte : value) {
        encoded.push_back(digits[byte >> 4]);
        encoded.push_back(digits[byte & 15]);
    }
    return encoded;
}

[[nodiscard]] inline std::optional<std::string> decodeCaptureRecipeMetadata(std::string_view encoded) {
    if (encoded.empty() || encoded.size() % 2 != 0
        || encoded.size() > kMaxCaptureRecipeMetadataBytes * 2) return std::nullopt;
    const auto nibble = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    };
    std::string decoded;
    decoded.reserve(encoded.size() / 2);
    for (std::size_t i = 0; i < encoded.size(); i += 2) {
        const int high = nibble(encoded[i]);
        const int low = nibble(encoded[i + 1]);
        if (high < 0 || low < 0) return std::nullopt;
        decoded.push_back(static_cast<char>((high << 4) | low));
    }
    return decoded;
}

[[nodiscard]] inline bool captureManifestTextAdmitted(std::string_view text) noexcept {
    return text.size() <= kMaxCaptureManifestBytes && text.find('\0') == std::string_view::npos;
}

struct CaptureManifestReferences {
    std::string world;
    std::string epimode;
    std::string volmem;
    std::string format;
    std::size_t formatCount = 0;
    std::size_t worldCount = 0;
    std::size_t epimodeCount = 0;
    std::size_t volmemCount = 0;
    std::string canonicalCaptureVersion;
    std::string canonicalMode;
    std::size_t canonicalCaptureVersionCount = 0;
    std::size_t canonicalModeCount = 0;
    std::string canonicalSeedImage;
    std::size_t canonicalSeedImageCount = 0;
    // Optional recorded provenance, not restore authority or authentication.
    std::optional<std::string> presetIdMetadata;
    std::optional<std::string> presetRevisionMetadata;
    std::size_t presetIdMetadataCount = 0;
    std::size_t presetRevisionMetadataCount = 0;
    CaptureStereoMetadata stereo;
};

// The v1 writer records Linux sibling filenames, not relative paths. Empty and
// "none" are handled separately as optional reference sentinels.
[[nodiscard]] inline bool captureManifestSiblingName(std::string_view name) noexcept {
    return !name.empty() && name != "." && name != ".." && name != "none"
        && name.find_first_of("/\r\n") == std::string_view::npos
        && name.find('\0') == std::string_view::npos;
}

// Image loaders normalize backslashes as separators, even on Linux. A declared
// image sibling must therefore reject both separator spellings before pinning.
[[nodiscard]] inline bool captureSeedImageSiblingName(std::string_view name) noexcept {
    return captureManifestSiblingName(name) && name.find('\\') == std::string_view::npos;
}

enum class CaptureManifestFormatAdmission { Legacy, V1, Duplicate, Unsupported };

struct CaptureManifestRestoreAdmission {
    CaptureManifestFormatAdmission format;
    bool worldValid;
    bool epimodeValid;
    bool volmemValid;
    bool worldDuplicate = false;
    bool epimodeDuplicate = false;
    bool volmemDuplicate = false;

    [[nodiscard]] bool admitted() const noexcept {
        return format == CaptureManifestFormatAdmission::Legacy ||
            (format == CaptureManifestFormatAdmission::V1 && worldValid && epimodeValid && volmemValid
             && !worldDuplicate && !epimodeDuplicate && !volmemDuplicate);
    }
};

// Shared policy for restore and its read-only diagnosis. Legacy paths remain
// unrestricted; only v1 has the sibling-name contract. A duplicate declaration
// cannot downgrade a v1 document to legacy handling.
[[nodiscard]] inline CaptureManifestRestoreAdmission captureManifestRestoreAdmission(
    const CaptureManifestReferences& refs) noexcept {
    if (refs.formatCount == 0) return {CaptureManifestFormatAdmission::Legacy, true, true, true};
    if (refs.formatCount != 1) return {CaptureManifestFormatAdmission::Duplicate, false, false, false};
    if (refs.format != "epic-capture-bundle-v1")
        return {CaptureManifestFormatAdmission::Unsupported, false, false, false};
    const auto valid = [](std::string_view value) {
        return value.empty() || value == "none" || captureManifestSiblingName(value);
    };
    return {CaptureManifestFormatAdmission::V1, valid(refs.world), valid(refs.epimode), valid(refs.volmem),
            refs.worldCount > 1, refs.epimodeCount > 1, refs.volmemCount > 1};
}

[[nodiscard]] inline bool captureManifestRestoreReferencesAdmitted(
    const CaptureManifestReferences& refs) noexcept {
    return captureManifestRestoreAdmission(refs).admitted();
}

// Canonical input is optional, but advertising it activates the v1 sibling
// contract. Shared lexical policy only: no path is opened and no mode decoded.
enum class CanonicalCaptureDeclarationAssessment {
    NotRecorded, DeclaredV1, RejectedManifest, DuplicateVersion, DuplicateMode,
    MissingVersion, MissingMode, UnsupportedVersion, InvalidModeReference,
    DuplicateSeedImage, InvalidSeedImageReference,
};

[[nodiscard]] inline CanonicalCaptureDeclarationAssessment assessCanonicalCaptureDeclaration(
    const CaptureManifestReferences& refs) noexcept {
    using Assessment = CanonicalCaptureDeclarationAssessment;
    if (refs.canonicalCaptureVersionCount == 0 && refs.canonicalModeCount == 0
        && refs.canonicalSeedImageCount == 0)
        return Assessment::NotRecorded;
    const auto admission = captureManifestRestoreAdmission(refs);
    if (admission.format != CaptureManifestFormatAdmission::V1 || !admission.admitted())
        return Assessment::RejectedManifest;
    // Deterministic precedence: manifest gate, ambiguity, completeness, version,
    // sibling name. A later valid value cannot hide an ambiguous declaration.
    if (refs.canonicalCaptureVersionCount > 1) return Assessment::DuplicateVersion;
    if (refs.canonicalModeCount > 1) return Assessment::DuplicateMode;
    if (refs.canonicalSeedImageCount > 1) return Assessment::DuplicateSeedImage;
    if (refs.canonicalCaptureVersionCount == 0) return Assessment::MissingVersion;
    if (refs.canonicalModeCount == 0) return Assessment::MissingMode;
    if (refs.canonicalCaptureVersion != "1") return Assessment::UnsupportedVersion;
    if (!captureManifestSiblingName(refs.canonicalMode)) return Assessment::InvalidModeReference;
    if (refs.canonicalSeedImageCount != 0 && !captureSeedImageSiblingName(refs.canonicalSeedImage))
        return Assessment::InvalidSeedImageReference;
    return Assessment::DeclaredV1;
}

[[nodiscard]] inline const char* canonicalCaptureDeclarationCode(
    CanonicalCaptureDeclarationAssessment assessment) noexcept {
    using Assessment = CanonicalCaptureDeclarationAssessment;
    switch (assessment) {
    case Assessment::NotRecorded: return "not_recorded";
    case Assessment::DeclaredV1: return "declared_v1";
    case Assessment::RejectedManifest: return "rejected_manifest";
    case Assessment::DuplicateVersion: return "duplicate_version";
    case Assessment::DuplicateMode: return "duplicate_mode";
    case Assessment::MissingVersion: return "missing_version";
    case Assessment::MissingMode: return "missing_mode";
    case Assessment::UnsupportedVersion: return "unsupported_version";
    case Assessment::InvalidModeReference: return "invalid_mode_reference";
    case Assessment::DuplicateSeedImage: return "duplicate_seed_image";
    case Assessment::InvalidSeedImageReference: return "invalid_seed_image_reference";
    }
    return "rejected_manifest";
}

// Retain last values for legacy compatibility and read-only inspection; restore
// owners use counts to reject ambiguous declarations. Canonical input admission
// belongs to the flagship helper, not the navigator's optional-volume policy.
[[nodiscard]] inline std::optional<CaptureManifestReferences> parseCaptureManifestReferences(
    std::string_view text) {
    if (!captureManifestTextAdmitted(text)) return std::nullopt;
    CaptureManifestReferences result;
    result.stereo = parseCaptureStereoMetadata(text);
    while (!text.empty()) {
        const auto newline = text.find('\n');
        const auto line = text.substr(0, newline);
        if (line.starts_with("format=")) {
            result.format = line.substr(7);
            ++result.formatCount;
        } else if (line.starts_with("world=")) { result.world = line.substr(6); ++result.worldCount; }
        else if (line.starts_with("epimode=")) { result.epimode = line.substr(8); ++result.epimodeCount; }
        else if (line.starts_with("volmem=")) { result.volmem = line.substr(7); ++result.volmemCount; }
        else if (line.starts_with("canonical_feedback_capture_version=")) {
            result.canonicalCaptureVersion = line.substr(35);
            ++result.canonicalCaptureVersionCount;
        } else if (line.starts_with("canonical_feedback_mode=")) {
            result.canonicalMode = line.substr(24);
            ++result.canonicalModeCount;
        } else if (line.starts_with("canonical_feedback_seed_image=")) {
            result.canonicalSeedImage = line.substr(std::string_view("canonical_feedback_seed_image=").size());
            ++result.canonicalSeedImageCount;
        } else if (line.starts_with("preset_id_hex=")) {
            result.presetIdMetadata = decodeCaptureRecipeMetadata(line.substr(14));
            ++result.presetIdMetadataCount;
        } else if (line.starts_with("preset_revision_hex=")) {
            result.presetRevisionMetadata = decodeCaptureRecipeMetadata(line.substr(20));
            ++result.presetRevisionMetadataCount;
        }
        if (newline == std::string_view::npos) break;
        text.remove_prefix(newline + 1);
    }
    return result;
}

[[nodiscard]] inline std::optional<CaptureManifestReferences> readCaptureManifestReferences(
    const std::filesystem::path& path) {
    if (path.empty() || path.native().find(std::filesystem::path::value_type{})
            != std::filesystem::path::string_type::npos) return std::nullopt;
    try {
        // Complete bounded acquisition precedes parsing/publication, so an IO
        // error or over-budget tail cannot expose earlier valid reference lines.
        const auto text = epi::io::readBoundedText(path, kMaxCaptureManifestBytes, "capture manifest");
        return parseCaptureManifestReferences(text);
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

} // namespace epi_sculpt
