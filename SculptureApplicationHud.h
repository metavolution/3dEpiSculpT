#pragma once

#include <cstddef>

namespace epi_sculpt {
class SculptureApplication;
class SculptureFramePlan;

// Educational Annotation: MODULAR TELEMETRY FORMATTER
// Extracted to keep the main application class clean and focused on orchestration.
void formatSculptureApplicationHud(char* buffer, std::size_t size,
                                  const SculptureFramePlan& framePlan,
                                  const SculptureApplication& app);

}
