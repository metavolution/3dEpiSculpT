#pragma once

#include "SculpturePresetAsset.h"
#include "SculpturePresetConfiguration.h"
#include "SculptureFilePublication.h"

#include <filesystem>
#include <optional>
#include <span>
#include <string_view>

namespace epi_sculpt {

enum class PresetExportError {
    None, InvalidAnnotation, UnrepresentableConfiguration, OutputUnavailable,
    CatalogSourceUnavailable, CatalogOverwrite, PublicationFailed, InvalidModulationRoute,
    AuthoredAutomation
};

[[nodiscard]] constexpr std::string_view presetExportErrorMessage(PresetExportError error) noexcept {
    switch (error) {
    case PresetExportError::None: return "none";
    case PresetExportError::InvalidAnnotation: return "annotation must contain comment lines only";
    case PresetExportError::UnrepresentableConfiguration:
        return "cannot preserve this configuration through the current codec";
    case PresetExportError::OutputUnavailable: return "output cannot be resolved";
    case PresetExportError::CatalogSourceUnavailable: return "catalog source cannot be resolved";
    case PresetExportError::CatalogOverwrite: return "cannot overwrite a catalog source";
    case PresetExportError::PublicationFailed: return "publication failed";
    case PresetExportError::InvalidModulationRoute: return "invalid-modulation-route";
    case PresetExportError::AuthoredAutomation:
        return "cannot preserve authored automation in the current WorldMode format";
    }
    return "unknown export failure";
}

// Owned publication facts. Route diagnostics occur only on route admission
// refusal and carry the original parser cause, not a second parse of the input.
// A default result is unavailable, never evidence that a file was published.
struct PresetExportResult {
    PresetExportError error = PresetExportError::PublicationFailed;
    std::optional<ModulationRouteDiagnostic> routeDiagnostic = std::nullopt;

    [[nodiscard]] bool published() const noexcept { return error == PresetExportError::None; }
};

// One cold authoring transaction, shared by actual mutation/morph/tour authors.
// Callers admit domain compatibility and supply a complete immutable value plus
// the catalog whose files must be protected. No catalog reload, UI or live state.
// A discovery owner also supplies its complete candidate path inventory, since
// rejected files have no asset entry. Value-only callers retain asset protection.
// File publication keeps the existing POSIX single-writer policy and exceptions;
// this does not provide bundle atomicity or power-loss directory durability.
[[nodiscard]] inline PresetExportResult publishPresetConfigurationDetailed(
    std::span<const SculpturePresetAsset> catalog, const SculptureWorldMode& world,
    const std::filesystem::path& output, std::string_view annotation = {},
    std::span<const std::filesystem::path> discoveredSources = {}) {
    // An annotation must never override encoded state or swallow its first line.
    if (annotation.find('\0') != std::string_view::npos)
        return {PresetExportError::InvalidAnnotation};
    for (std::size_t begin = 0; begin < annotation.size();) {
        const auto end = annotation.find('\n', begin);
        const auto line = annotation.substr(begin, end == std::string_view::npos
            ? annotation.size() - begin : end - begin);
        if (!line.empty() && line.front() != '#') return {PresetExportError::InvalidAnnotation};
        if (end == std::string_view::npos) break;
        begin = end + 1;
    }

    // Shared route/codec admission precedes any filesystem effects. Consume the
    // complete WorldMode; adapters must not reconstruct or separately admit it.
    const auto prepared = preparePresetConfiguration(world);
    if (prepared.error == PresetConfigurationError::AuthoredAutomation)
        return {PresetExportError::AuthoredAutomation};
    if (prepared.error == PresetConfigurationError::InvalidModulationRoute)
        return {PresetExportError::InvalidModulationRoute, prepared.routeDiagnostic};
    if (!prepared.ready())
        return {PresetExportError::UnrepresentableConfiguration};
    const auto& encoded = *prepared.encoded;

    const auto filepath = output.string();
    if (!file_publication_detail::admittedFilePath(filepath)) return {PresetExportError::OutputUnavailable};
    std::error_code error;
    const auto destination = std::filesystem::weakly_canonical(output, error);
    if (error) return {PresetExportError::OutputUnavailable};
    const auto admitSource = [&](const std::filesystem::path& source) {
        if (source.empty()) return PresetExportError::None;
        const auto origin = std::filesystem::weakly_canonical(source, error);
        if (error) return PresetExportError::CatalogSourceUnavailable;
        const bool sameFile = std::filesystem::equivalent(output, source, error);
        error.clear(); // New destinations and absent source files are permitted.
        return destination == origin || sameFile ? PresetExportError::CatalogOverwrite : PresetExportError::None;
    };
    for (const auto& source : discoveredSources) {
        if (const auto refusal = admitSource(source); refusal != PresetExportError::None)
            return {refusal};
    }
    for (const auto& source : catalog) {
        if (const auto refusal = admitSource(source.sourcePath); refusal != PresetExportError::None)
            return {refusal};
    }
    // The codec has already produced an owned buffer. Write it and the borrowed
    // comment prefix directly into private staging: no second full payload concat.
    const bool published = file_publication_detail::publishFile(filepath, [&](int fd) {
        return file_publication_detail::writeAll(fd, annotation.data(), annotation.size())
            && (annotation.empty() || annotation.back() == '\n'
                || file_publication_detail::writeAll(fd, "\n", 1))
            && file_publication_detail::writeAll(fd, encoded.data(), encoded.size());
    });
    return {published ? PresetExportError::None : PresetExportError::PublicationFailed};
}

// Compatibility projection: preserve the original signature and error values
// while delegating to exactly one preparation/publication transaction.
[[nodiscard]] inline PresetExportError publishPresetConfiguration(
    std::span<const SculpturePresetAsset> catalog, const SculptureWorldMode& world,
    const std::filesystem::path& output, std::string_view annotation = {}) {
    return publishPresetConfigurationDetailed(catalog, world, output, annotation).error;
}

} // namespace epi_sculpt
