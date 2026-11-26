#pragma once

#include "SculptureDiagnosticComparison.h"
#include "SculptureRaymarcher.h"

namespace epi_sculpt {

// Explicit candidate observation against a completed reference. Freeze settings
// and admit layout/reference channels before any rendering or filesystem work.
// Render once at the reference extent, settle workers, then publish both sources.
// Caller keeps the reference and borrowed render inputs stable until return and
// chooses a compatible camera; no alignment, checkpoint or native receipt is inferred.
// Render exceptions propagate without opening output. Later IO refusal returns
// false. Empty reference/path refusal returns false; invalid admission throws.
[[nodiscard]] inline bool renderSculptureDiagnosticComparisonPpm(
    const std::string& filepath, const SculptureGBuffer& reference,
    const SculptureRaymarcher& marcher,
    const SculptureVec3& cameraPos, const SculptureVec3& cameraTarget,
    const SculptureVec3& cameraUp, float fovRadians,
    const SculptureProgram& program, float time, float morph,
    const SculptureDiagnosticComparisonSettings& settings = {},
    const std::vector<float>& spatialBuffer = {}, std::uint32_t threadCount=4,
    const SculptureVolumetricMemory* volumetricMemory=nullptr, std::uint32_t gutter=2) {
    if (!file_publication_detail::admittedFilePath(filepath)) return false;
    const auto frozen=settings;
    const auto layout=detail::prepareDiagnosticComparisonLayout(reference.width,reference.height,frozen,gutter);
    if (layout.empty()) return false;
    const detail::CheckedDiagnosticObservation admittedReference(reference,frozen.diagnostic);
    SculptureGBuffer candidate;
    marcher.renderGBuffer(admittedReference.width(),admittedReference.height(),
        cameraPos,cameraTarget,cameraUp,fovRadians,program,time,morph,candidate,
        spatialBuffer,threadCount,volumetricMemory);
    return detail::publishDiagnosticComparison(filepath,reference,candidate,frozen,layout);
}

} // namespace epi_sculpt
