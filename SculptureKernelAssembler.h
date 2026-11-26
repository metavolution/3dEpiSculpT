#pragma once

#include "SculptureProgramSchema.h"
#include <string>
#include <string_view>
#include <filesystem>

namespace epi_sculpt {

class SculptureKernelAssembler {
public:
    // An authored color can return NaN/Inf; its channel recovery contract
    // requires compilation without finite-only assumptions.
    static bool requiresStrictColorMath(std::string_view prefix) noexcept {
        return prefix.find("#define EPI_COLOR_EFFECT_OVERRIDE_ID ") != std::string_view::npos;
    }
    // Dynamically resolves #define macro strings for all swappable mathematical components.
    // Falls back to SculptureFractalCatalog when a .cl file is absent, except
    // reduction/color: absent overrides retain authoritative kernel defaults.
    // reduction/<catalog name>.cl is a float3 body with sanitized float3 p;
    // it must return float3. Applies only to the selected reduction ID in 3D
    // (including an equal B ID); distinct B IDs and 4D retain built-in math.
    // color/<catalog name>.cl is a float3 post-effect body with float3 rgb
    // and float materialCoord, orbit_energy, magnitude, time. It returns RGB;
    // finite channels clamp to [0,1], nonfinite channels retain the base palette.
    // Applies only to the matching compact palette ID. ID 0 is rejected because
    // that route packs material metadata, not RGB. Learn diagnostics and the
    // nonzero-flow KineticVelocity path take precedence; other render routes
    // retain their own color contracts. Historical HSV catalog bodies are not
    // injected as defaults into this different, explicit RGB contract.
    // Each category/name is resolved once per call; A/B aliases share a body.
    // Disk bodies may span lines and contain comments; directives are rejected.
    // Malformed/unreadable overrides throw with their path instead of silently
    // selecting a different mathematical component. No GPU work occurs here.
    static std::string assemblePrefix(
        const SculptureProgramParams& params,
        const std::filesystem::path& componentsDir
    );

private:
    static std::string resolveSnippet(
        std::string_view componentName,
        std::string_view fallbackCode,
        const std::filesystem::path& componentsDir,
        const std::string& category
    );
};

} // namespace epi_sculpt
