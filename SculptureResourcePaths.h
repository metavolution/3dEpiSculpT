#pragma once

#include <filesystem>
#include <optional>
#include <span>
#include <string_view>

namespace epi_sculpt {

// Immutable resource recipe for epiC's native 3D/application layer.
//
// Runtime code receives resolved paths as values. It never changes process cwd
// and never rediscovers repository structure from arbitrary relative paths.
// Explicit --resource-root / EPIC_RESOURCE_ROOT wins; executable-relative and
// compile-time developer fallbacks exist only to bootstrap ordinary builds.
struct SculptureResourcePaths final {
    // Explicit standalone world recipe; no parent-engine/cwd/ancestry discovery.
    [[nodiscard]] static SculptureResourcePaths nativeWorld(
        const std::filesystem::path& root,
        const std::filesystem::path& shaderBinaries,
        const std::filesystem::path& font,
        std::span<const std::string_view> requiredKernels = {});
    std::filesystem::path root;
    std::filesystem::path shaderBinaryDirectory;
    std::filesystem::path shaderSourceDirectory;
    std::filesystem::path fontPath;
    std::filesystem::path sculptureKernelDirectory;
    std::filesystem::path canonicalKernelDirectory;
    std::filesystem::path generatedKernelDirectory;
    std::filesystem::path presetDirectory;

    // Frequently used named resources are materialized once at composition
    // time. More specialized kernels are resolved through the directory fields.
    std::filesystem::path sculptureKernel;
    std::filesystem::path worldKernel;
    std::filesystem::path canonicalFeedbackKernel;
    std::filesystem::path implicitKernel;

    [[nodiscard]] bool hasResourceRoot() const noexcept { return !root.empty(); }
    [[nodiscard]] std::filesystem::path sculptureKernelSource(
        std::string_view filename) const;
    [[nodiscard]] std::filesystem::path canonicalKernelSource(
        std::string_view filename) const;
};

class SculptureResourceLocator final {
public:
    [[nodiscard]] static SculptureResourcePaths resolve(
        const char* executablePath,
        const std::optional<std::filesystem::path>& explicitRoot = std::nullopt);
};

} // namespace epi_sculpt
