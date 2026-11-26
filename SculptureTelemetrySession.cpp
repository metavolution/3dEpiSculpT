#include "SculptureTelemetrySession.h"
#include <chrono>

namespace epi_sculpt {

SculptureTelemetrySession::~SculptureTelemetrySession() {
    endSession();
}

bool SculptureTelemetrySession::beginSession(const std::string& outputPath, const std::string& profileName) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (active_) return false;
    
    file_.open(outputPath, std::ios::out | std::ios::trunc);
    if (!file_.is_open()) return false;
    
    outputPath_ = outputPath;
    profileName_ = profileName;
    active_ = true;
    
    // Write CSV header
    file_ << "ProfileName,TimestampMs,Generation,ActiveInstances,"
             "OpenCL_TotalMs,OpenCL_KernelMs,OpenCL_ReadbackMs,ReadbackBytes,"
             "CPU_RebuildMs,CPU_AcceptedInstances,CPU_DroppedInstances,CPU_ActiveBatches,"
             "Vulkan_SubmitMs\n";
    return true;
}

void SculptureTelemetrySession::endSession() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!active_) return;
    flush();
    file_.close();
    active_ = false;
}

void SculptureTelemetrySession::recordWorldGeneration(
    const WorldGenerationMetrics& genMetrics,
    const WorldBridgeMetrics& bridgeMetrics,
    const WorldPresentationMetrics& presMetrics) {
    
    std::lock_guard<std::mutex> lock(mutex_);
    if (!active_) return;
    
    buffer_.push_back({genMetrics, bridgeMetrics, presMetrics});
    
    if (buffer_.size() >= 60) {
        flush();
    }
}

void SculptureTelemetrySession::flush() {
    if (!file_.is_open() || buffer_.empty()) return;
    
    auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    for (const auto& rec : buffer_) {
        file_ << profileName_ << "," 
              << nowMs << ","
              << rec.gen.generation << ","
              << rec.gen.activeInstanceCount << ","
              << rec.gen.queueToDeliveryMilliseconds << ","
              << rec.gen.kernelMilliseconds << ","
              << rec.gen.readbackMilliseconds << ","
              << rec.gen.readbackBytes << ","
              << rec.bridge.rebuildMilliseconds << ","
              << rec.bridge.acceptedInstanceCount << ","
              << rec.bridge.droppedInstanceCount << ","
              << rec.bridge.activeBatchCount << ","
              << rec.pres.submitCpuMilliseconds << "\n";
    }
    buffer_.clear();
    file_.flush();
}

} // namespace epi_sculpt
