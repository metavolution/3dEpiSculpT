#include "SculptureWorldBenchmarkProfile.h"
#include "SculptureWorldNavigator.h"
#include "SculpturePipelineCoordinator.h"
#include "SculptureVolumetricMemory.h"
#include "SculptureRaymarcher.h"
#include "SculptureWorldRecipe.h"

#include <array>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace epi_sculpt {

WorldProfileResult SculptureWorldBenchmarkProfile::profileWorld(
    SculptureWorldNavigator& navigator,
    std::string_view worldIdOrAlias,
    std::size_t stepCount
) {
    WorldProfileResult result{};
    if (!navigator.selectWorld(worldIdOrAlias, 0.0f)) {
        throw std::invalid_argument("Cannot profile unknown or unsupported world: "
            + std::string(worldIdOrAlias));
    }
    navigator.coordinator().snapToTarget();

    const auto& entry = navigator.activeEntry();
    const auto& recipe = navigator.activeRecipe();

    result.catalogIndex = navigator.activeIndex();
    result.worldId = navigator.activePresetName();
    if (!worldIdOrAlias.empty() && !std::isdigit(static_cast<unsigned char>(worldIdOrAlias.front()))) {
        result.alias = std::string(worldIdOrAlias);
    } else if (!entry.aliases[0].empty()) {
        result.alias = std::string(entry.aliases[0]);
    } else {
        result.alias = std::string(entry.canonicalPresetId);
    }
    result.worldTitle = std::string(entry.title);
    result.primitiveName = std::string(worldPrimitiveName(recipe.primitive));
    result.instanceBudget = recipe.instanceBudget;
    result.uses4D = recipe.uses4DProjection;

    auto* volMem = navigator.volumetricMemory();
    if (volMem != nullptr) {
        volMem->reset();

        const auto params = navigator.activeParams();
        const float dt = 0.016f;

        auto t0 = std::chrono::high_resolution_clock::now();
        for (std::size_t s = 0; s < stepCount; ++s) {
            volMem->stepCpuOracle(params, static_cast<float>(s) * dt, dt);
        }
        auto t1 = std::chrono::high_resolution_clock::now();

        const double totalUs = std::chrono::duration<double, std::micro>(t1 - t0).count();
        result.averageStepTimeUs = stepCount > 0 ? (totalUs / static_cast<double>(stepCount)) : 0.0;

        const auto& telem = volMem->telemetry();
        result.macroOccupancyRatio = volMem->macroOccupancyRatio();
        result.meanDensity = telem.meanDensity;
        result.peakEnergy = telem.peakEnergy;
    }

    SculptureRay testRay{ SculptureVec3{0.5f, 0.5f, -2.5f}, SculptureVec3{0.0f, 0.0f, 1.0f} };
    const SculptureRayHit hit = navigator.marchVolumetricRay(testRay);

    result.transmittance = hit.transmittance;
    result.inScatteringRadiance = (hit.volumetricR + hit.volumetricG + hit.volumetricB) / 3.0f;

    result.healthy = std::isfinite(result.meanDensity)
                  && std::isfinite(result.peakEnergy)
                  && std::isfinite(result.transmittance)
                  && std::isfinite(result.inScatteringRadiance)
                  && result.transmittance >= 0.0f && result.transmittance <= 1.0f;

    return result;
}

std::vector<WorldProfileResult> SculptureWorldBenchmarkProfile::profileCanonicalBenchmarks(
    SculptureWorldNavigator& navigator,
    std::size_t stepCount
) {
    static constexpr std::array<std::string_view, 11> kCanonicalBenchmarkAliases = {
        "neon_bubblesphere",
        "kuramoto_synchronization_manifold",
        "hopf_fibration_drift",
        "exp_golden_phyllotaxis_cathedral",
        "sinh_rossler_spiral_ribbon",
        "120_cell_quantum_hologram",
        "ammann_beenker_quasicrystal",
        "faraday_chladni_resonance",
        "lichtenberg_forest",
        "gyroid_maze",
        "hypercube_tesseract"
    };

    std::vector<WorldProfileResult> results;
    results.reserve(kCanonicalBenchmarkAliases.size());

    for (const auto alias : kCanonicalBenchmarkAliases) {
        results.push_back(profileWorld(navigator, alias, stepCount));
    }
    return results;
}

std::vector<WorldProfileResult> SculptureWorldBenchmarkProfile::profileAllCatalogWorlds(
    SculptureWorldNavigator& navigator,
    std::size_t stepCount
) {
    std::vector<WorldProfileResult> results;
    const std::size_t count = navigator.count();
    results.reserve(count);

    for (std::size_t i = 0; i < count; ++i) {
        results.push_back(profileWorld(navigator, std::to_string(i), stepCount));
    }
    return results;
}

void SculptureWorldBenchmarkProfile::printProfileTable(
    const std::vector<WorldProfileResult>& results,
    std::ostream& out
) {
    out << "========================================================================================================================\n";
    out << "  epiC World Catalog CPU-Oracle Profile (" << results.size() << " Worlds; not GPU timing)\n";
    out << "========================================================================================================================\n";
    out << std::left
        << std::setw(6)  << "Idx"
        << std::setw(28) << "World / Alias"
        << std::setw(12) << "Primitive"
        << std::setw(8)  << "Budget"
        << std::setw(6)  << "4D"
        << std::setw(11) << "Occupancy"
        << std::setw(12) << "Density"
        << std::setw(12) << "Energy"
        << std::setw(10) << "Transmit"
        << std::setw(13) << "Step Time"
        << std::setw(8)  << "Status"
        << "\n";
    out << "------------------------------------------------------------------------------------------------------------------------\n";

    for (const auto& r : results) {
        const std::string displayName = !r.alias.empty() ? r.alias : r.worldId;
        out << std::left
            << std::setw(6)  << r.catalogIndex
            << std::setw(28) << (displayName.size() > 27 ? displayName.substr(0, 24) + "..." : displayName)
            << std::setw(12) << r.primitiveName
            << std::setw(8)  << r.instanceBudget
            << std::setw(6)  << (r.uses4D ? "YES" : "NO")
            << std::setw(10) << (std::to_string(static_cast<int>(r.macroOccupancyRatio * 100.0f)) + "% ")
            << std::setw(12) << std::fixed << std::setprecision(4) << r.meanDensity
            << std::setw(12) << std::fixed << std::setprecision(4) << r.peakEnergy
            << std::setw(10) << std::fixed << std::setprecision(3) << r.transmittance
            << std::setw(11) << (std::to_string(static_cast<int>(r.averageStepTimeUs)) + " us ")
            << std::setw(8)  << (r.healthy ? "NOMINAL" : "WARN")
            << "\n";
    }
    out << "========================================================================================================================\n";
}

} // namespace epi_sculpt
