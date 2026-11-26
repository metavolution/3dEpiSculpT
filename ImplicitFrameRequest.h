#pragma once

#include "SculptureGpuAbi.h"
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>

namespace epi_sculpt {

struct ImplicitCamera {
    std::array<float, 3> position{0.0f, 0.0f, 15.0f};
    std::array<float, 3> target{0.0f, 0.0f, 0.0f};
    std::array<float, 3> up{0.0f, 1.0f, 0.0f};
    float fovYRadians = 1.0f;
    float eyeSeparation = 0.0f;
};

inline constexpr float kImplicitMaximumDistance = 100.0f;

// Immutable semantic request presented to the OpenCL binding layer. Keeping
// packing here makes it possible to prove request correctness without a GPU.
struct ImplicitFrameRequest {
    gpu_abi::SculptureProgramParamsGpu params{};
    gpu_abi::SculptureFrameGpu frame{};
    std::array<float, 4> cameraPosition{};
    std::array<float, 4> cameraTarget{};
    std::array<float, 4> cameraUp{};
    float fovYRadians = 1.0f;
    float eyeSeparation = 0.0f;
    std::uint32_t width = 0U;
    std::uint32_t height = 0U;
    // CPU-side producer lifetime identity. It is intentionally not part of the
    // GPU ABI; cadence uses it to distinguish an equivalent camera request sent
    // to a newly rebuilt producer/resource set.
    std::uint64_t producerEpoch = 0U;
};

// Shared admission validation for both authored and already-packed requests.
[[nodiscard]] inline bool validateImplicitFrameRequest(
    const ImplicitFrameRequest& request, std::string& diagnostic) {
    const auto width = request.width, height = request.height;
    const auto time = request.frame.time, morph = request.frame.morph;
    const auto deltaSeconds = request.frame.deltaSeconds;
    const ImplicitCamera camera{
        {request.cameraPosition[0], request.cameraPosition[1], request.cameraPosition[2]},
        {request.cameraTarget[0], request.cameraTarget[1], request.cameraTarget[2]},
        {request.cameraUp[0], request.cameraUp[1], request.cameraUp[2]},
        request.fovYRadians, request.eyeSeparation};
    if (request.params.abiVersion != gpu_abi::kVersion
        || request.params.byteSize != gpu_abi::kProgramParamsBytes
        || request.frame.abiVersion != gpu_abi::kVersion
        || request.frame.byteSize != gpu_abi::kFrameBytes) {
        diagnostic = "implicit request GPU ABI mismatch";
        return false;
    }
    const auto finite3 = [](const std::array<float, 3>& value) {
        return std::isfinite(value[0]) && std::isfinite(value[1])
            && std::isfinite(value[2]);
    };
    if (width == 0U || height == 0U) {
        diagnostic = "implicit dimensions must be non-zero";
        return false;
    }
    const std::uint64_t pixels = static_cast<std::uint64_t>(width) * height;
    if (pixels > std::numeric_limits<std::size_t>::max() / (4U * sizeof(float))) {
        diagnostic = "implicit dimensions overflow host buffer size";
        return false;
    }
    if (!std::isfinite(time) || !std::isfinite(morph)
        || !std::isfinite(deltaSeconds) || deltaSeconds < 0.0f) {
        diagnostic = "implicit frame time, morph, and non-negative delta must be finite";
        return false;
    }
    if (!finite3(camera.position) || !finite3(camera.target) || !finite3(camera.up)) {
        diagnostic = "implicit camera values must be finite";
        return false;
    }
    if (!std::isfinite(camera.fovYRadians)
        || camera.fovYRadians <= 0.0f
        || camera.fovYRadians >= 3.14159265f) {
        diagnostic = "implicit vertical field of view must be finite and between 0 and pi";
        return false;
    }
    if (!std::isfinite(camera.eyeSeparation) || camera.eyeSeparation < 0.0f) {
        diagnostic = "implicit eye separation must be finite and non-negative";
        return false;
    }
    if (camera.eyeSeparation > 0.0f && (width < 2U || (width & 1U) != 0U)) {
        diagnostic = "stereo implicit rendering requires an even width >= 2";
        return false;
    }
    const std::array<float, 3> forward{
        camera.target[0] - camera.position[0],
        camera.target[1] - camera.position[1],
        camera.target[2] - camera.position[2]};
    const float forwardLength2 = forward[0] * forward[0]
        + forward[1] * forward[1] + forward[2] * forward[2];
    const float upLength2 = camera.up[0] * camera.up[0]
        + camera.up[1] * camera.up[1] + camera.up[2] * camera.up[2];
    const std::array<float, 3> cross{
        forward[1] * camera.up[2] - forward[2] * camera.up[1],
        forward[2] * camera.up[0] - forward[0] * camera.up[2],
        forward[0] * camera.up[1] - forward[1] * camera.up[0]};
    const float crossLength2 = cross[0] * cross[0]
        + cross[1] * cross[1] + cross[2] * cross[2];
    if (!std::isfinite(forwardLength2) || !std::isfinite(upLength2)
        || !std::isfinite(crossLength2) || forwardLength2 <= 1.0e-12f || upLength2 <= 1.0e-12f
        || crossLength2 <= 1.0e-12f) {
        diagnostic = "implicit camera requires distinct eye/target and a non-parallel up vector";
        return false;
    }

    const auto finite4 = [](const std::array<float, 4>& value) {
        return std::isfinite(value[0]) && std::isfinite(value[1])
            && std::isfinite(value[2]) && std::isfinite(value[3]);
    };
    if (!std::isfinite(request.params.latentW)
        || !std::isfinite(request.params.topologyBlend)
        || !std::isfinite(request.params.attractorStrength)
        || !finite4(request.params.preAffine)
        || !finite4(request.params.postAffine)
        || !finite4(request.params.dynamicsSeed)
        || !finite4(request.params.pulseMaterial)
        || !finite4(request.params.outputScale)
        || !finite4(request.params.kineticFlow)
        || !finite4(request.params.audioSpectrum)
        || !finite4(request.params.opticalVolumetric)
        || !finite4(request.params.temporalControl)
        || !std::isfinite(request.params.chromaBreath)
        || !std::isfinite(request.params.lumaDrift)
        || !std::isfinite(request.params.rotorAlpha)
        || !std::isfinite(request.params.rotorBeta)
        || !std::isfinite(request.params.rotorTheta4D)) {
        diagnostic = "implicit program parameters must pack to finite GPU values";
        return false;
    }

    diagnostic.clear();
    return true;
}

[[nodiscard]] inline bool makeImplicitFrameRequest(
    const SculptureProgramParams& params,
    float time,
    float morph,
    float deltaSeconds,
    std::uint32_t generation,
    std::uint32_t width,
    std::uint32_t height,
    const ImplicitCamera& camera,
    ImplicitFrameRequest& out,
    std::string& diagnostic) {
    // Reject invalid source timing before packFrame's normalizing operations.
    if (!std::isfinite(time) || !std::isfinite(morph)
        || !std::isfinite(deltaSeconds) || deltaSeconds < 0.0f) {
        diagnostic = "implicit frame time, morph, and non-negative delta must be finite";
        return false;
    }
    ImplicitFrameRequest candidate{};
    candidate.params = gpu_abi::packProgramParams(params);
    candidate.frame = gpu_abi::packFrame(
        time, morph, deltaSeconds, generation);
    candidate.cameraPosition = {
        camera.position[0], camera.position[1], camera.position[2], 0.0f};
    candidate.cameraTarget = {
        camera.target[0], camera.target[1], camera.target[2], 0.0f};
    candidate.cameraUp = {camera.up[0], camera.up[1], camera.up[2], 0.0f};
    candidate.fovYRadians = camera.fovYRadians;
    candidate.eyeSeparation = camera.eyeSeparation;
    candidate.width = width;
    candidate.height = height;

    if (!validateImplicitFrameRequest(candidate, diagnostic)) return false;
    out = candidate;
    return true;
}

} // namespace epi_sculpt
