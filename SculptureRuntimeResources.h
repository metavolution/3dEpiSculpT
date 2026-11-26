#pragma once

#include "ExperimentCatalog.h"
#include "SculptureResourcePaths.h"

#include <filesystem>
#include <stdexcept>
#include <string>
#include <string_view>

namespace epi_sculpt {

// Runtime resource resolution is a pure lookup over a composition-root recipe.
// It never changes cwd and never searches developer-machine absolute paths.
[[nodiscard]] inline std::filesystem::path resolveKernelSourcePath(
    const SculptureResourcePaths& resources,
    std::string_view filename) {
    const std::filesystem::path requested(filename);
    if (requested.is_absolute()) return requested;

    const auto sculpture = resources.sculptureKernelSource(filename);
    std::error_code ec;
    if (!sculpture.empty() && std::filesystem::is_regular_file(sculpture, ec) && !ec)
        return sculpture;
    ec.clear();

    const auto canonical = resources.canonicalKernelSource(filename);
    if (!canonical.empty() && std::filesystem::is_regular_file(canonical, ec) && !ec)
        return canonical;

    throw std::runtime_error(
        "Resource recipe cannot resolve OpenCL kernel '" + std::string(filename) + "'");
}

[[nodiscard]] inline std::filesystem::path resolveExperimentKernelPath(
    const SculptureResourcePaths& resources,
    std::string_view catalogId) {
    if (const ExperimentCatalogEntry* entry = findExperimentCatalogEntry(catalogId);
        entry != nullptr && !entry->kernelSource.empty()) {
        return resolveKernelSourcePath(resources, entry->kernelSource);
    }
    return resolveKernelSourcePath(resources, catalogId);
}

} // namespace epi_sculpt
