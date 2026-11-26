#pragma once

#include "SculptureDiagnosticAtlas.h"
#include "SculptureRaymarcher.h"

namespace epi_sculpt {

// Explicit synchronous observation/export composition. Freeze and admit all tile
// settings/extents before work, render exactly one G-buffer, then publish only
// after renderGBuffer has settled its workers. No cadence or simulation mutation.
// Caller keeps program, marcher and borrowed fields stable until return.
// Invalid settings/layout throw before rendering/IO; lexical path refusal and
// empty extent return false. Render exceptions propagate without opening output;
// later IO refusal returns false. Native presentation remains unconfirmed.
[[nodiscard]] inline bool renderSculptureDiagnosticAtlasPpm(
    const std::string& filepath,
    const SculptureRaymarcher& marcher,
    std::uint32_t width, std::uint32_t height,
    const SculptureVec3& cameraPos,
    const SculptureVec3& cameraTarget,
    const SculptureVec3& cameraUp,
    float fovRadians,
    const SculptureProgram& program,
    float time, float morph,
    std::span<const SculptureDiagnosticSettings> views = kSculptureDiagnosticAtlasViews,
    const std::vector<float>& spatialBuffer = {},
    std::uint32_t threadCount = 4,
    const SculptureVolumetricMemory* volumetricMemory = nullptr,
    std::uint32_t columns = 3, std::uint32_t gutter = 2) {
    if (!file_publication_detail::admittedFilePath(filepath)) return false;
    const auto layout=detail::prepareDiagnosticAtlasLayout(width,height,views,columns,gutter);
    if (layout.empty()) return false;
    SculptureGBuffer observation;
    marcher.renderGBuffer(width,height,cameraPos,cameraTarget,cameraUp,fovRadians,
        program,time,morph,observation,spatialBuffer,threadCount,volumetricMemory);
    return detail::publishDiagnosticAtlas(filepath,observation,layout);
}

} // namespace epi_sculpt
