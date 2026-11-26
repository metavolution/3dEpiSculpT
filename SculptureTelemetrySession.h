#pragma once

#include "SculptureWorldPerformance.h"

#include <fstream>
#include <string>
#include <mutex>
#include <vector>

namespace epi_sculpt {

// -----------------------------------------------------------------------------
// Educational Annotation: PHASE 10C TELEMETRY SESSIONS
// -----------------------------------------------------------------------------
// Records generation-keyed end-to-end performance provenance for dedicated worlds.
// This telemetry provides the evidence needed to gate Phase 10D (Zero-Copy GPU
// Delivery). It logs the CPU readback and batching costs to prove if partitioning
// is actually the dominant bottleneck for dense procedurals.
class SculptureTelemetrySession {
public:
    SculptureTelemetrySession() = default;
    ~SculptureTelemetrySession();

    // Starts a durable CSV profile session
    [[nodiscard]] bool beginSession(const std::string& outputPath, const std::string& profileName);
    
    // Flushes pending records and closes the session
    void endSession();
    
    [[nodiscard]] bool isActive() const noexcept { return active_; }

    // Correlates and records a complete generation loop
    void recordWorldGeneration(const WorldGenerationMetrics& genMetrics, 
                               const WorldBridgeMetrics& bridgeMetrics, 
                               const WorldPresentationMetrics& presMetrics);

private:
    struct Record {
        WorldGenerationMetrics gen;
        WorldBridgeMetrics bridge;
        WorldPresentationMetrics pres;
    };
    
    void flush();

    std::string outputPath_;
    std::string profileName_;
    std::ofstream file_;
    bool active_ = false;
    std::mutex mutex_;
    std::vector<Record> buffer_;
};

} // namespace epi_sculpt
