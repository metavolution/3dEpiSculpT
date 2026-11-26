#include <array>
#include "SculptureProgram.h"
#include "SculptureExecutionPolicy.h"
#include "SculptureTemporalHistory.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <utility>
#include <thread>
#include <vector>
#include <random>
#include <numeric>
#include <stdexcept>
#include "SculptureGpuAbi.h"
#include "QuaternionAlgebra.h"
#include "OctonionAlgebra.h"
#include "SculptureThreadPool.h"
#include "SculptureFutureDrain.h"
#include "SculptureVoxelSampling.h"

#include "epi/core/complex.hpp"
#include "epi/seed/sdf.hpp"

#include "SculptureMathOracle.h"
namespace epi_sculpt {
using namespace epi_sculpt::oracle;

// -----------------------------------------------------------------------------
// Educational Annotation: CPU Oracle (Deterministic Mathematics)
// -----------------------------------------------------------------------------
// Bounded CPU reference and explicit fallback evaluation. Canonical production
// mathematics lives in OpenCL; complete CPU feature parity is not required.
// Consumers select recovery policy and must treat a failed evaluation as a
// rejected observation rather than publishing its partially written output.
// -----------------------------------------------------------------------------



SculptureProgram::SculptureProgram(SculptureProgramParams params)
    : params_(std::move(params)) {
    params_.stageMask = learn_mode::normalize(params_.stageMask);
}

const SculptureProgramParams& SculptureProgram::params() const noexcept {
    return params_;
}

void SculptureProgram::resetTemporalHistory() noexcept {
    temporalHistoryBuffer_.reset();
    temporalScratchBuffer_.reset();
    cadenceAccumulator_ = 0.0f;
    simulatedTime_ = 0.0f;
}

bool SculptureProgram::hasTemporalHistory() const noexcept {
    return temporalHistoryBuffer_ != nullptr;
}

const SculptureSnapshot* SculptureProgram::currentTemporalHistory() const noexcept {
    return temporalHistoryBuffer_.get();
}

void SculptureProgram::evaluateWithCadence(float wallTime, float morph, float deltaSeconds, SculptureSnapshot& outSnapshot) {
    // Refuse invalid execution offers before even applying a pending reset.
    // std::clamp/max do not sanitize NaN; retaining one would poison later time.
    if (!std::isfinite(wallTime) || !std::isfinite(morph) || !std::isfinite(deltaSeconds)
        || !std::isfinite(params_.temporalFeedbackBlend)
        || (params_.temporalFeedbackBlend > 0.0f && !std::isfinite(params_.temporalFixedDelta))) {
        throw std::invalid_argument("CPU cadence requires finite execution inputs");
    }
    if (params_.temporalReset) {
        resetTemporalHistory();
    }

    if (params_.temporalFeedbackBlend <= 0.0f) {
        evaluate(wallTime, morph, outSnapshot, nullptr);
        if (!temporalScratchBuffer_) {
            temporalScratchBuffer_ = std::make_unique<SculptureSnapshot>();
        }
        // Copying the variable-size field may throw after scalar/sample writes.
        // Only a complete candidate may replace the last accepted history.
        *temporalScratchBuffer_ = outSnapshot;
        temporalHistoryBuffer_.swap(temporalScratchBuffer_);
        return;
    }

    cadenceAccumulator_ += std::max(deltaSeconds, 0.0f);
    const float dt = std::clamp(params_.temporalFixedDelta, 0.001f, 0.1f);
    cadenceAccumulator_ = std::min(cadenceAccumulator_, 0.2f); // Cap to prevent runaway latency death

    if (!temporalScratchBuffer_) {
        temporalScratchBuffer_ = std::make_unique<SculptureSnapshot>();
    }
    if (!temporalHistoryBuffer_) {
        evaluate(wallTime, morph, *temporalScratchBuffer_, nullptr);
        temporalHistoryBuffer_.swap(temporalScratchBuffer_);
    }

    while (cadenceAccumulator_ >= dt) {
        if (!temporalScratchBuffer_) {
            temporalScratchBuffer_ = std::make_unique<SculptureSnapshot>();
        }
        const float nextTime = simulatedTime_ + dt;
        evaluate(nextTime, morph, *temporalScratchBuffer_, temporalHistoryBuffer_.get());

        // The swap is non-throwing: history and its clock/debit commit together.
        // Earlier accepted catch-up steps survive any later failed attempt.
        temporalHistoryBuffer_.swap(temporalScratchBuffer_);
        simulatedTime_ = nextTime;
        cadenceAccumulator_ -= dt;
    }

    // Observation failure cannot undo accepted simulation or make it run twice.
    outSnapshot = *temporalHistoryBuffer_;
}

    // -------------------------------------------------------------------------
    // Educational Annotation: DETERMINISTIC CPU ORACLE & FALLBACK
    // -------------------------------------------------------------------------
    // OpenCL owns canonical production. This deterministic CPU evaluator is a
    // reference and explicit fallback, without exhaustive CPU/OpenCL parity as
    // a project gate. Its caller owns fallback selection, retry and publication;
    // this function reports failure after settling every submitted task.
    // -------------------------------------------------------------------------
void SculptureProgram::evaluate(float time, float morph, SculptureSnapshot& outSnapshot, const SculptureSnapshot* previousFrame) const {
    if (previousFrame == &outSnapshot)
        throw std::invalid_argument("CPU sculpture output cannot alias its previous frame");
    outSnapshot.sourceHistoryValid = false;
    // Surface programs should remain a lightweight CPU fallback. The spatial
    // field is an explicit voxel-family resource, not hidden per-frame work
    // performed by every output mode.
    const bool usesVoxelField =
        temporalFamily(params_) == SculptureTemporalFamily::Voxel;
    const std::size_t sourceAxis = getAuxiliaryVoxelGridDimension(params_.auxiliaryResolutionScale);
    const std::size_t sourceCount = usesVoxelField
        ? sourceAxis * sourceAxis * sourceAxis : outSnapshot.samples.size();
    if (sourceCount > kSculptureSourceHistoryCapacity)
        throw std::length_error("CPU sculpture source history exceeds its bounded domain");
    const bool priorHistoryAdmitted = previousFrame && previousFrame->sourceHistoryValid &&
        canUseSourceSampleHistory(params_, previousFrame->sourceHistoryFamily,
                                  previousFrame->sourceHistoryScale) &&
        previousFrame->sourceHistory.size() == sourceCount;
    const auto priorSource = [&](std::size_t index) -> const SculptureSourceSample* {
        if (!priorHistoryAdmitted || index >= sourceCount) return nullptr;
        const auto& candidate = previousFrame->sourceHistory[index];
        return candidate.valid() ? &candidate : nullptr;
    };
    // A failed task never exposes a complete history; each thread owns its
    // distinct source index and writes before display compaction.
    outSnapshot.sourceHistory.assign(sourceCount, SculptureSourceSample{});

    std::array<bool, gpu_abi::kVolumeMacroBlockCount> activeBlocks{};
    if (usesVoxelField) {
        for (std::size_t b = 0; b < gpu_abi::kVolumeMacroBlockCount; ++b) {
            const std::size_t bx = b % 5;
            const std::size_t by = (b / 5) % 5;
            const std::size_t bz = b / 25;
            const std::size_t centerIndex = (bx * 10 + 5) + (by * 10 + 5) * gpu_abi::kVolumeGridSize + (bz * 10 + 5) * (gpu_abi::kVolumeGridSize * gpu_abi::kVolumeGridSize);
            
            const float u = static_cast<float>(centerIndex) / static_cast<float>(kSampleCount);
            const Vec3f seed = seedPointShape(centerIndex, time, 1, params_.auxiliaryResolutionScale);
            Vec3f prevFeedback = seed;
            if (const auto* source = priorSource(centerIndex);
                source && params_.temporalFeedbackBlend > 0.0f) {
                prevFeedback = {source->positionAndValidity[0],
                                source->positionAndValidity[1],
                                source->positionAndValidity[2]};
            }
            OrbitResult orbit = evaluateOrbit(seed, u, time, params_, prevFeedback);
            Vec3f feedback = orbit.feedback;
            const float shapedMorph = smoothstep(0.0f, 1.0f, std::clamp(morph, 0.0f, 1.0f));
            const Vec3f domain = epi::lerp(seed, feedback, shapedMorph);
            const float magnitude = domain.length();
            activeBlocks[b] = std::abs(magnitude - 1.0f) < 0.9f;
        }
    } else {
        activeBlocks.fill(true);
    }

    outSnapshot.spatialBuffer.clear();
    outSnapshot.time = time;
    outSnapshot.morph = std::clamp(morph, 0.0f, 1.0f);
    if (usesVoxelField) {
        const std::uint32_t gridN = getAuxiliaryVoxelGridDimension(params_.auxiliaryResolutionScale);
        const std::size_t totalVoxels = static_cast<std::size_t>(gridN * gridN * gridN);
        outSnapshot.spatialBuffer.resize(totalVoxels, 0.0f);
        const float maxCoord = static_cast<float>(gridN - 1U);
        const float coordScale = 49.0f / std::max(maxCoord, 1.0f);

        // Fluid Diffusion via Trilinear Advection. Decoupled to variable grid scales.
        auto sampleFluid = [&](float x, float y, float z) -> float {
            if (!previousFrame || previousFrame->spatialBuffer.empty()) return 0.0f;
            const SculptureVec3 normLookup{
                std::clamp(x / std::max(maxCoord, 1.0f), 0.0f, 1.0f),
                std::clamp(y / std::max(maxCoord, 1.0f), 0.0f, 1.0f),
                std::clamp(z / std::max(maxCoord, 1.0f), 0.0f, 1.0f)
            };
            return trilinearSampleVoxelBuffer(previousFrame->spatialBuffer, normLookup);
        };

        for (std::size_t index = 0; index < totalVoxels; ++index) {
            std::size_t vx = index % gridN;
            std::size_t vy = (index / gridN) % gridN;
            std::size_t vz = index / (gridN * gridN);
            
            float currentFluid = 0.0f;
            if (shouldUseTemporalHistory(params_)) {
                // Convection velocity: upwards with slight swirl based on time and height
                float vyNorm = static_cast<float>(vy) * coordScale;
                float vel_x = std::sin(vyNorm * 0.15f + time * 0.5f) * (1.2f / coordScale);
                float vel_y = 1.8f / coordScale; // Strong upward thermal convection
                float vel_z = std::cos(vyNorm * 0.15f + time * 0.5f) * (1.2f / coordScale);
                
                float src_x = static_cast<float>(vx) - vel_x;
                float src_y = static_cast<float>(vy) - vel_y;
                float src_z = static_cast<float>(vz) - vel_z;
                
                currentFluid = sampleFluid(src_x, src_y, src_z);
                
                // Add slight 6-way diffusion for stability
                if (previousFrame && previousFrame->spatialBuffer.size() == totalVoxels && vx > 0 && vx < (gridN - 1) && vy > 0 && vy < (gridN - 1) && vz > 0 && vz < (gridN - 1)) {
                    float neighbors = previousFrame->spatialBuffer[index - 1] + previousFrame->spatialBuffer[index + 1] +
                                      previousFrame->spatialBuffer[index - gridN] + previousFrame->spatialBuffer[index + gridN] +
                                      previousFrame->spatialBuffer[index - (gridN * gridN)] + previousFrame->spatialBuffer[index + (gridN * gridN)];
                    currentFluid = std::lerp(currentFluid, neighbors / 6.0f, 0.15f);
                }
            }
            
            currentFluid *= 0.96f; // Reduced decay for longer fluid trails
            outSnapshot.spatialBuffer[index] = currentFluid;
            
            if (currentFluid > 0.05f) {
                std::size_t bx = (vx * 5) / gridN;
                std::size_t by = (vy * 5) / gridN;
                std::size_t bz = (vz * 5) / gridN;
                std::size_t b = bx + by * 5 + bz * 25;
                if (b < activeBlocks.size()) {
                    activeBlocks[b] = true;
                }
            }
        }
    }

    std::size_t writeIndex = 0;
    const std::size_t evalCount = sourceCount;
    
    std::vector<SculptureSample> tempSamples(evalCount);
    const std::size_t numThreads = sculptureWorkerCount(
        temporalFamily(params_), evalCount, std::thread::hardware_concurrency());
    const float deltaSeconds = previousFrame ? std::max(0.0f, time - previousFrame->time) : 0.0166667f;
    const auto evaluateRange = [&](std::size_t start, std::size_t end) {
        for (std::size_t index = start; index < end; ++index) {
            SculptureSample previous{};
            const SculptureSourceSample* source = priorSource(index);
            const SculptureSample* prevSample = nullptr;
            if (source) {
                previous.x = source->positionAndValidity[0];
                previous.y = source->positionAndValidity[1];
                previous.z = source->positionAndValidity[2];
                previous.vx = source->velocity[0];
                previous.vy = source->velocity[1];
                previous.vz = source->velocity[2];
                prevSample = &previous;
            }
            tempSamples[index] = evaluateSample(
                index, prevSample, time, outSnapshot.morph, params_, activeBlocks,
                outSnapshot.spatialBuffer, deltaSeconds);
            const auto& sample = tempSamples[index];
            if (sample.radius > 0.0f && std::isfinite(sample.radius) && std::isfinite(sample.x) && std::isfinite(sample.y) &&
                std::isfinite(sample.z) && std::isfinite(sample.vx) && std::isfinite(sample.vy) &&
                std::isfinite(sample.vz)) {
                outSnapshot.sourceHistory[index] = {
                    {sample.x, sample.y, sample.z, 1.0f}, {sample.vx, sample.vy, sample.vz, 0.0f}};
            }
        }
    };

    if (numThreads == 1U) {
        evaluateRange(0U, evalCount);
    } else {
        static SculptureThreadPool sharedWorkerPool(std::thread::hardware_concurrency());
        
        std::vector<std::future<void>> futures;
        futures.reserve(numThreads);
        detail::SculptureFutureDrain<void> settle(futures);
        
        for (std::size_t t = 0; t < numThreads; ++t) {
            const std::size_t start = (evalCount * t) / numThreads;
            const std::size_t end = (evalCount * (t + 1)) / numThreads;
            futures.push_back(sharedWorkerPool.enqueue([&evaluateRange, start, end]() {
                evaluateRange(start, end);
            }));
        }
        
        for (auto& f : futures) {
            f.get();
        }
    }
    
    for (std::size_t index = 0; index < evalCount && writeIndex < outSnapshot.samples.size(); ++index) {
        if (tempSamples[index].radius > 0.0f || evalCount == outSnapshot.samples.size()) {
            outSnapshot.samples[writeIndex++] = tempSamples[index];
        }
    }
    for (; writeIndex < outSnapshot.samples.size(); ++writeIndex) {
        outSnapshot.samples[writeIndex] = SculptureSample{};
    }
    outSnapshot.sourceHistoryFamily = temporalFamily(params_);
    outSnapshot.sourceHistoryScale = params_.auxiliaryResolutionScale;
    outSnapshot.sourceHistoryValid = true;
}

std::uint32_t SculptureProgram::getAuxiliaryVoxelGridDimension(StageResolutionScale scale) noexcept {
    switch (scale) {
    case StageResolutionScale::Full: return 50U;
    case StageResolutionScale::Half: return 25U;
    case StageResolutionScale::Quarter: return 12U;
    case StageResolutionScale::Eighth: return 6U;
    }
    return 50U;
}

float SculptureProgram::trilinearSampleVoxelBuffer(const std::vector<float>& spatialBuffer, const SculptureVec3& normPos) {
    if (!std::isfinite(normPos.x) || !std::isfinite(normPos.y) || !std::isfinite(normPos.z)) return 0.0f;
    const std::size_t N = detail::voxelSamplingDimension(spatialBuffer.size());
    if (N == 0U) return 0.0f;

    const float maxCoord = static_cast<float>(N - 1U);
    // Clamp finite normalized coordinates first, so even FLT_MAX cannot overflow.
    const float gx = std::clamp(normPos.x, 0.0f, 1.0f) * maxCoord;
    const float gy = std::clamp(normPos.y, 0.0f, 1.0f) * maxCoord;
    const float gz = std::clamp(normPos.z, 0.0f, 1.0f) * maxCoord;

    const std::size_t x0 = static_cast<std::size_t>(gx);
    const std::size_t y0 = static_cast<std::size_t>(gy);
    const std::size_t z0 = static_cast<std::size_t>(gz);

    const std::size_t x1 = std::min(x0 + 1U, N - 1U);
    const std::size_t y1 = std::min(y0 + 1U, N - 1U);
    const std::size_t z1 = std::min(z0 + 1U, N - 1U);

    const float fx = gx - static_cast<float>(x0);
    const float fy = gy - static_cast<float>(y0);
    const float fz = gz - static_cast<float>(z0);

    // Admission proved N^3 <= size(): strides and every flat index now fit size_t.
    const std::size_t sliceStride = N * N;
    auto idx = [N, sliceStride](std::size_t x, std::size_t y, std::size_t z) -> std::size_t {
        return x + y * N + z * sliceStride;
    };

    const float c000 = spatialBuffer[idx(x0, y0, z0)];
    const float c100 = spatialBuffer[idx(x1, y0, z0)];
    const float c010 = spatialBuffer[idx(x0, y1, z0)];
    const float c110 = spatialBuffer[idx(x1, y1, z0)];
    const float c001 = spatialBuffer[idx(x0, y0, z1)];
    const float c101 = spatialBuffer[idx(x1, y0, z1)];
    const float c011 = spatialBuffer[idx(x0, y1, z1)];
    const float c111 = spatialBuffer[idx(x1, y1, z1)];

    const float c00 = std::lerp(c000, c100, fx);
    const float c10 = std::lerp(c010, c110, fx);
    const float c01 = std::lerp(c001, c101, fx);
    const float c11 = std::lerp(c011, c111, fx);

    const float c0 = std::lerp(c00, c10, fy);
    const float c1 = std::lerp(c01, c11, fy);

    return std::lerp(c0, c1, fz);
}

float SculptureProgram::sampleDensityField(const SculptureVec3& pos, float time, float morph, const std::vector<float>& spatialBuffer) const {
    const Vec3f p{pos.x, pos.y, pos.z};
    const OrbitResult orbit = evaluateOrbit(p, 0.5f, time, params_, p);
    const float shapedMorph = smoothstep(0.0f, 1.0f, std::clamp(morph, 0.0f, 1.0f));
    const Vec3f domain = epi::lerp(p, orbit.feedback, shapedMorph);
    const float magnitude = domain.length();
    
    const float orbitShell = std::clamp(1.0f - std::abs(magnitude - 1.0f) * 1.5f, 0.0f, 1.0f);
    const float trapDensity = std::clamp(1.0f - orbit.orbitTrap * 1.5f, 0.0f, 1.0f);
    const float seedWeight = orbit.finalSeedWeight;
    
    float fluidDensity = 0.0f;
    if (!spatialBuffer.empty()) {
        const SculptureVec3 normPos{
            std::clamp((pos.x + 1.0f) * 0.5f, 0.0f, 1.0f),
            std::clamp((pos.y + 1.0f) * 0.5f, 0.0f, 1.0f),
            std::clamp((pos.z + 1.0f) * 0.5f, 0.0f, 1.0f)
        };
        fluidDensity = std::clamp(trilinearSampleVoxelBuffer(spatialBuffer, normPos), 0.0f, 1.0f);
    }
    
    float density = 0.50f * orbitShell + 0.25f * trapDensity + 0.25f * seedWeight + 0.35f * fluidDensity;
    
    if (!learn_mode::enabled(params_.stageMask, learn_mode::Stage::Transform)) density *= 0.5f;
    if (!learn_mode::enabled(params_.stageMask, learn_mode::Stage::Seed)) density *= 0.7f;
    
    return std::clamp(density, 0.0f, 1.0f);
}

// -----------------------------------------------------------------------------
// Educational Annotation: Tetrahedral Gradient Estimation
// -----------------------------------------------------------------------------
// Evaluating the analytic gradient of the density field is an expensive 
// operation. Instead of the standard 6-tap central difference (which evaluates 
// the SDF 6 times), this uses a 4-tap tetrahedral simplex arrangement.
// This reduces compute load by 33% and improves performance for heavy volumetrics.
// -----------------------------------------------------------------------------
SculptureVec3 SculptureProgram::evaluateFieldGradient(const SculptureVec3& pos, float time, float morph, float eps, const std::vector<float>& spatialBuffer) const {
    const SculptureVec3 p1{pos.x + eps, pos.y - eps, pos.z - eps};
    const SculptureVec3 p2{pos.x - eps, pos.y - eps, pos.z + eps};
    const SculptureVec3 p3{pos.x - eps, pos.y + eps, pos.z - eps};
    const SculptureVec3 p4{pos.x + eps, pos.y + eps, pos.z + eps};

    const float d1 = sampleDensityField(p1, time, morph, spatialBuffer);
    const float d2 = sampleDensityField(p2, time, morph, spatialBuffer);
    const float d3 = sampleDensityField(p3, time, morph, spatialBuffer);
    const float d4 = sampleDensityField(p4, time, morph, spatialBuffer);

    SculptureVec3 grad{
        d1 - d2 - d3 + d4,
        -d1 - d2 + d3 + d4,
        -d1 + d2 - d3 + d4
    };

    const float len = std::sqrt(grad.x * grad.x + grad.y * grad.y + grad.z * grad.z);
    if (len > 1e-6f) {
        grad.x /= len;
        grad.y /= len;
        grad.z /= len;
    } else {
        grad = SculptureVec3{0.0f, 1.0f, 0.0f};
    }
    return grad;
}

} // namespace epi_sculpt
