#pragma once

#include "SculptureProgramSchema.h"
#include <string>
#include <string_view>
#include <vector>
#include <cstdint>
#include <cstddef>
#include <iostream>

namespace epi_sculpt {

class SculptureWorldNavigator;

// -----------------------------------------------------------------------------
// Educational Annotation: TOPOLOGICAL COMPLEXITY & WORLD BENCHMARK PROFILE
// -----------------------------------------------------------------------------
// Each world in the epimorphic catalog constitutes a distinct dynamical system
// characterized by specific orbit geometries, macro-occupancy bounds, and
// raymarching optical density profiles.
//
// The Benchmark Profile measures:
//  1. Macro-Occupancy Ratio: Density of non-empty macro-blocks (vacancy leaping efficiency)
//  2. Mean Field Density & Peak Energy: Dynamic range and numerical stability
//  3. Volumetric Transmittance & In-Scattering: Optical thickness and depth decay
//  4. CPU Oracle Step Latency: Per-step execution microseconds
// -----------------------------------------------------------------------------

struct WorldProfileResult {
    std::size_t catalogIndex = 0U;
    std::string worldId;
    std::string alias;
    std::string worldTitle;
    std::string primitiveName;
    std::uint32_t instanceBudget = 0U;
    bool uses4D = false;

    float macroOccupancyRatio = 0.0f;
    float meanDensity = 0.0f;
    float peakEnergy = 0.0f;
    float transmittance = 1.0f;
    float inScatteringRadiance = 0.0f;
    double averageStepTimeUs = 0.0;
    bool healthy = true;
};

class SculptureWorldBenchmarkProfile {
public:
    // Profiles a single world by ID or alias over a specified number of warmup steps
    [[nodiscard]] static WorldProfileResult profileWorld(
        SculptureWorldNavigator& navigator,
        std::string_view worldIdOrAlias,
        std::size_t stepCount = 8
    );

    // Profiles all 11 canonical priority benchmark worlds
    [[nodiscard]] static std::vector<WorldProfileResult> profileCanonicalBenchmarks(
        SculptureWorldNavigator& navigator,
        std::size_t stepCount = 8
    );

    // Profiles all indexed worlds currently in the navigator's catalog
    [[nodiscard]] static std::vector<WorldProfileResult> profileAllCatalogWorlds(
        SculptureWorldNavigator& navigator,
        std::size_t stepCount = 4
    );

    // Formats and prints a benchmark profile table
    static void printProfileTable(
        const std::vector<WorldProfileResult>& results,
        std::ostream& out = std::cout
    );
};

} // namespace epi_sculpt
