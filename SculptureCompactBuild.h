#pragma once
#include "SculptureKernelAssembler.h"
#include "SculptureKernelSpecializer.h"

#include <utility>

namespace epi_sculpt {
// Common compact build identity for runtime initialization, reload and warmup.
// Owns text only; source loading, cache lookup and OpenCL lifetime stay with callers.
struct SculptureCompactBuild {
    static constexpr const char* kernelTag = "sculpture_generate";
    std::string prefix;
    std::string options;

    static SculptureCompactBuild describe(const SculptureProgramParams& selected,
                                           const std::filesystem::path& directory,
                                           bool allowFilesystemIncludes = true) {
        // The live compiler request currently carries six component IDs. Keep
        // warmup on that exact contract; do not bake unrelated preset values
        // into a binary the runtime would never request.
        SculptureProgramParams params{};
        params.inner = selected.inner; params.innerB = selected.innerB;
        params.seedType = selected.seedType; params.seedTypeB = selected.seedTypeB;
        params.reduction = selected.reduction; params.colorEffect = selected.colorEffect;
        const auto components = SculptureKernelAssembler::assemblePrefix(params, directory / "components");
        KernelSpecializationConfig config{};
        config.targetOpenClVersion = 120U;
        config.fastRelaxedMath = !SculptureKernelAssembler::requiresStrictColorMath(components);
        auto options = SculptureKernelSpecializer::generateBuildOptions(params, config)
            + "-cl-denorms-are-zero";
        if (allowFilesystemIncludes) options += " -I \"" + directory.string() + "\"";
        return {SculptureKernelSpecializer::generateSpecializedPreamble(params, config)
                    + "\n" + components, std::move(options)};
    }
};
} // namespace epi_sculpt
