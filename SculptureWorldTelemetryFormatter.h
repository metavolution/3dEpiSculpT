#pragma once

#include <cstddef>
#include <cstdint>

namespace epi_sculpt {

class SculptureWorldNavigator;

// -----------------------------------------------------------------------------
// [EDUCATIONAL NOTE] Pure HUD, Learn Mode & Telemetry Formatter
// -----------------------------------------------------------------------------
// Decouples string assembly and HUD status formatting from SculptureWorldNavigator's
// simulation/rendering orchestration. Operates as a pure, non-mutating observer
// over the navigator's active world catalog entry, 6-stage pipeline configuration,
// 4D SO(4) foliation/polychoron telemetry, volumetric memory metrics, and
// performance budget health.
// -----------------------------------------------------------------------------
class SculptureWorldTelemetryFormatter {
public:
    static void formatHud(
        const SculptureWorldNavigator& nav,
        char* buffer,
        std::size_t size,
        std::uint32_t generation,
        std::size_t instanceCount) noexcept;
};

} // namespace epi_sculpt
