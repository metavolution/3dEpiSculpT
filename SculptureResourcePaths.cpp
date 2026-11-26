#include "SculptureResourcePaths.h"

#include <cstdlib>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

namespace epi_sculpt {
namespace {

[[nodiscard]] std::filesystem::path canonicalIfPossible(
    const std::filesystem::path& path) {
    std::error_code ec;
    auto canonical = std::filesystem::weakly_canonical(path, ec);
    return ec ? path.lexically_normal() : canonical;
}

[[nodiscard]] bool isFile(const std::filesystem::path& path) noexcept {
    std::error_code ec;
    return std::filesystem::is_regular_file(path, ec) && !ec;
}

[[nodiscard]] bool isDirectory(const std::filesystem::path& path) noexcept {
    std::error_code ec;
    return std::filesystem::is_directory(path, ec) && !ec;
}

void appendUnique(std::vector<std::filesystem::path>& out,
                  std::filesystem::path path) {
    if (path.empty()) return;
    path = path.lexically_normal();
    for (const auto& existing : out) {
        if (existing == path) return;
    }
    out.push_back(std::move(path));
}

[[nodiscard]] bool looksLikeResourceRoot(
    const std::filesystem::path& root) noexcept {
    const bool hasShaders = isDirectory(root / "shaders")
        || isDirectory(root / "mGFX/shaders");
    const bool hasSculpt = (isDirectory(root / "MODULES/3dEpiSculpT/presets")
            && isDirectory(root / "MODULES/3dEpiSculpT/kernels"))
        || (isDirectory(root / "3dEpiSculpTMorph/presets")
            && isDirectory(root / "3dEpiSculpTMorph/kernels"));
    const bool hasCanonical = isFile(root / "MODULES/epiC/kernels/main.cl")
        || isFile(root / "epiC/kernels/main.cl");
    return hasShaders && hasSculpt && hasCanonical;
}

[[nodiscard]] std::filesystem::path resolveShaderSourceDir(
    const std::filesystem::path& root) {
    if (isDirectory(root / "shaders")) return root / "shaders";
    return root / "mGFX/shaders";
}

[[nodiscard]] std::filesystem::path resolveSculptureKernelDir(
    const std::filesystem::path& root) {
    if (isDirectory(root / "MODULES/3dEpiSculpT/kernels"))
        return root / "MODULES/3dEpiSculpT/kernels";
    return root / "3dEpiSculpTMorph/kernels";
}

[[nodiscard]] std::filesystem::path resolveSculpturePresetDir(
    const std::filesystem::path& root) {
    if (isDirectory(root / "MODULES/3dEpiSculpT/presets"))
        return root / "MODULES/3dEpiSculpT/presets";
    return root / "3dEpiSculpTMorph/presets";
}

[[nodiscard]] std::filesystem::path resolveCanonicalKernelDir(
    const std::filesystem::path& root) {
    if (isDirectory(root / "MODULES/epiC/kernels"))
        return root / "MODULES/epiC/kernels";
    return root / "epiC/kernels";
}

[[nodiscard]] std::filesystem::path searchModularKernel(
    const std::filesystem::path& root,
    const std::filesystem::path& sculptureKernelDirectory,
    const std::filesystem::path& filename) {
    if (!sculptureKernelDirectory.empty()) {
        const auto direct = sculptureKernelDirectory / filename;
        if (isFile(direct)) return canonicalIfPossible(direct);
    }
    if (!root.empty()) {
        const std::filesystem::path extraDirs[] = {
            root / "PROCEDURAL/water/kernels",
            root / "MODULES/external_compute/kernels",
            root / "WORLDS/island01/kernels",
            root / "WORLDS/forest_opus/kernels",
        };
        for (const auto& dir : extraDirs) {
            const auto candidate = dir / filename;
            if (isFile(candidate)) return canonicalIfPossible(candidate);
        }
    }
    return sculptureKernelDirectory.empty()
        ? std::filesystem::path{} : (sculptureKernelDirectory / filename);
}

[[nodiscard]] std::vector<std::filesystem::path> candidateRoots(
    const char* executablePath) {
    std::vector<std::filesystem::path> roots;
    roots.reserve(24U);

    auto addRootForms = [&](const std::filesystem::path& root) {
        appendUnique(roots, root);
        appendUnique(roots, root / "mGFX.production");
        appendUnique(roots, root / "share/epiC");
    };

    // Executable ancestry supports build-tree and installed layouts without
    // granting arbitrary process cwd the authority to redefine assets.
    if (executablePath && *executablePath != '\0') {
        std::error_code ec;
        auto executable = std::filesystem::absolute(executablePath, ec);
        if (ec) executable = std::filesystem::path(executablePath);
        auto dir = executable.parent_path();
        for (int depth = 0; depth < 6 && !dir.empty(); ++depth) {
            addRootForms(dir);
            const auto parent = dir.parent_path();
            if (parent == dir) break;
            dir = parent;
        }
    }

#ifdef MGFX_EPI_RESOURCE_ROOT
    addRootForms(std::filesystem::path(MGFX_EPI_RESOURCE_ROOT));
#endif
    return roots;
}

[[nodiscard]] std::filesystem::path resolveConfiguredRoot(
    const std::filesystem::path& configured) {
    if (configured.empty()) return {};
    if (looksLikeResourceRoot(configured)) return canonicalIfPossible(configured);
    const auto production = configured / "mGFX.production";
    if (looksLikeResourceRoot(production)) return canonicalIfPossible(production);
    const auto packaged = configured / "share/epiC";
    if (looksLikeResourceRoot(packaged)) return canonicalIfPossible(packaged);
    return {};
}

[[nodiscard]] std::filesystem::path discoverShaderBinaryDirectory(
    const char* executablePath,
    const std::filesystem::path& root) {
    std::vector<std::filesystem::path> candidates;
    candidates.reserve(8U);

    if (const char* env = std::getenv("EPIC_SHADER_BINARY_DIR"); env && *env != '\0') {
        appendUnique(candidates, env);
    }
    if (executablePath && *executablePath != '\0') {
        std::error_code ec;
        auto executable = std::filesystem::absolute(executablePath, ec);
        if (ec) executable = std::filesystem::path(executablePath);
        appendUnique(candidates, executable.parent_path() / "shaders");
    }
#ifdef MGFX_EPI_SHADER_BINARY_DIR
    appendUnique(candidates, MGFX_EPI_SHADER_BINARY_DIR);
#endif
    appendUnique(candidates, root / "share/epiC/shaders");
    appendUnique(candidates, root / "shaders");

    for (const auto& candidate : candidates) {
        if (isFile(candidate / "mesh.vert.spv")
            || isFile(candidate / "post.vert.spv")) {
            return canonicalIfPossible(candidate);
        }
    }
    return {};
}

[[nodiscard]] std::filesystem::path discoverGeneratedKernelDirectory(
    const std::filesystem::path& root) {
    std::vector<std::filesystem::path> candidates;
    candidates.reserve(6U);
    if (const char* env = std::getenv("EPI2_GENERATED_KERNEL_DIR"); env && *env != '\0') {
        appendUnique(candidates, env);
    }
#ifdef EPI2_GENERATED_KERNEL_DIR
    appendUnique(candidates, EPI2_GENERATED_KERNEL_DIR);
#endif
    appendUnique(candidates, root / "share/epiC/generated/kernels");
    appendUnique(candidates, root / "generated/kernels");
    appendUnique(candidates, root / "generated/epi2/kernels");

    for (const auto& candidate : candidates) {
        if (isFile(candidate / "generated_inner_math.cl")
            && isFile(candidate / "generated_color_math.cl")
            && isFile(candidate / "generated_seed_math.cl")) {
            return canonicalIfPossible(candidate);
        }
    }
    return {};
}

} // namespace

SculptureResourcePaths SculptureResourcePaths::nativeWorld(
    const std::filesystem::path& root,
    const std::filesystem::path& shaderBinaries,
    const std::filesystem::path& font,
    std::span<const std::string_view> requiredKernels) {
    if (root.empty() || shaderBinaries.empty() || font.empty())
        throw std::invalid_argument("Standalone world requires explicit root, shader and font paths");
    SculptureResourcePaths result{};
    result.root = canonicalIfPossible(root);
    result.shaderSourceDirectory = canonicalIfPossible(resolveShaderSourceDir(result.root));
    result.shaderBinaryDirectory = canonicalIfPossible(shaderBinaries);
    result.fontPath = canonicalIfPossible(font);
    result.sculptureKernelDirectory = canonicalIfPossible(resolveSculptureKernelDir(result.root));
    result.presetDirectory = canonicalIfPossible(resolveSculpturePresetDir(result.root));
    const auto require = [](bool present, const std::filesystem::path& path) {
        if (!present)
            throw std::runtime_error("Standalone world resource is missing: " + path.string());
    };
    require(isDirectory(result.shaderSourceDirectory), result.shaderSourceDirectory);
    require(isDirectory(result.presetDirectory), result.presetDirectory);
    require(isDirectory(result.sculptureKernelDirectory), result.sculptureKernelDirectory);
    require(isFile(result.shaderBinaryDirectory / "mesh.vert.spv"),
        result.shaderBinaryDirectory / "mesh.vert.spv");
    require(isFile(result.fontPath), result.fontPath);
    // Each composition root declares its own native producers. A shared recipe
    // must not make every standalone world depend on Island's terrain kernel.
    for (const auto kernel : requiredKernels) {
        const auto path = searchModularKernel(
            result.root, result.sculptureKernelDirectory, std::filesystem::path(kernel));
        require(!kernel.empty() && isFile(path), path);
    }
    return result;
}

std::filesystem::path SculptureResourcePaths::sculptureKernelSource(
    std::string_view filename) const {
    if (filename.empty()) return {};
    const std::filesystem::path input(filename);
    if (input.is_absolute()) return input;
    return searchModularKernel(root, sculptureKernelDirectory, input.filename());
}

std::filesystem::path SculptureResourcePaths::canonicalKernelSource(
    std::string_view filename) const {
    if (filename.empty()) return {};
    const std::filesystem::path input(filename);
    if (input.is_absolute()) return input;
    return canonicalKernelDirectory.empty()
        ? std::filesystem::path{} : canonicalKernelDirectory / input.filename();
}

SculptureResourcePaths SculptureResourceLocator::resolve(
    const char* executablePath,
    const std::optional<std::filesystem::path>& explicitRoot) {
    SculptureResourcePaths result{};
    if (explicitRoot && !explicitRoot->empty()) {
        result.root = resolveConfiguredRoot(*explicitRoot);
        if (result.root.empty()) {
            throw std::runtime_error(
                "Configured epiC resource root is invalid: " + explicitRoot->string());
        }
    } else if (const char* env = std::getenv("EPIC_RESOURCE_ROOT");
               env && *env != '\0') {
        result.root = resolveConfiguredRoot(std::filesystem::path(env));
        if (result.root.empty()) {
            throw std::runtime_error(
                "EPIC_RESOURCE_ROOT does not contain an epiC resource tree: "
                + std::string(env));
        }
    } else {
        for (const auto& candidate : candidateRoots(executablePath)) {
            if (looksLikeResourceRoot(candidate)) {
                result.root = canonicalIfPossible(candidate);
                break;
            }
        }
    }

    if (!result.root.empty()) {
        result.shaderSourceDirectory = canonicalIfPossible(resolveShaderSourceDir(result.root));
        result.shaderBinaryDirectory = discoverShaderBinaryDirectory(
            executablePath, result.root);
        result.sculptureKernelDirectory = canonicalIfPossible(resolveSculptureKernelDir(result.root));
        result.canonicalKernelDirectory = canonicalIfPossible(resolveCanonicalKernelDir(result.root));
        result.generatedKernelDirectory = discoverGeneratedKernelDirectory(result.root);
        result.presetDirectory = canonicalIfPossible(resolveSculpturePresetDir(result.root));
        result.sculptureKernel = result.sculptureKernelDirectory / "SculptureGenerate.cl";
        result.worldKernel = result.sculptureKernelDirectory / "SculptureWorldGenerate.cl";
        result.canonicalFeedbackKernel = result.canonicalKernelDirectory / "main.cl";
        result.implicitKernel = result.sculptureKernelDirectory / "SculptureRaymarch.cl";
    } else {
        result.generatedKernelDirectory = discoverGeneratedKernelDirectory({});
    }

    if (const char* env = std::getenv("EPIC_FONT_PATH"); env && *env != '\0'
        && isFile(env)) {
        result.fontPath = env;
    }
#ifdef MGFX_DEMO_FONT_PATH
    if (result.fontPath.empty() && isFile(MGFX_DEMO_FONT_PATH)) {
        result.fontPath = MGFX_DEMO_FONT_PATH;
    }
#endif
    if (result.fontPath.empty() && isFile("/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf")) {
        result.fontPath = "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf";
    }
    return result;
}

} // namespace epi_sculpt
