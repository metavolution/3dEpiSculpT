/**
 * ============================================================================
 * EPI2 / 3dEpiSculpTMorph - STANDALONE SHADER & KERNEL SYNTHESIZER
 * ============================================================================
 *
 * Educational Annotation: STANDALONE MATHEMATICAL KERNEL SYNTHESIS
 * -------------------------------------------------------------------------
 * This module synthesizes self-contained, single-file GLSL fragment shaders
 * (compatible with Shadertoy, WebGL, and Vulkan) and pure OpenCL kernels
 * from active SculptureProgramParams and 4D foliation presets.
 *
 * Features:
 *  1. Analytical Signed Distance Function (SDF) synthesis from 6-stage orbits.
 *  2. Full OKLCH, Golden-Ratio, and Bioluminescence color model codegen.
 *  3. Interactive mouse/camera raymarching loop with Blinn-Phong & ambient occlusion.
 *  4. Single-file standalone portability for WebGL, mobile, and offline rendering.
 * ============================================================================
 */

#pragma once

#include "SculptureProgramSchema.h"
#include "SculptureSerializer.h"

#include <string>
#include <string_view>

namespace epi_sculpt {

enum class ShaderTargetLanguage : std::uint8_t {
    ShadertoyGLSL,
    VulkanComputeGLSL,
    StandaloneOpenCL,
    StandaloneHTML5,
};

enum class SpatialCurvature : std::uint8_t {
    Euclidean = 0,             // Flat Euclidean R^3 space (kappa = 0)
    HyperbolicPoincare = 1,     // Negative curvature H^3 conformal ball (kappa = -1)
    HyperbolicBeltrami = 2,     // Negative curvature projective disc (kappa = -1)
    SphericalElliptic = 3       // Positive curvature S^3 sphere (kappa = +1)
};

struct ShaderSynthesisConfig {
    ShaderTargetLanguage target = ShaderTargetLanguage::ShadertoyGLSL;
    SpatialCurvature curvature = SpatialCurvature::Euclidean;
    float curvatureConstant = -1.0f; // kappa for non-Euclidean geodesic integration
    bool enableDualTopologyHomotopy = true; // Blends inner and innerB via topologyBlend
    std::string title = "Epimorphic 3D Sculpture";
    std::string author = "EPI2 Mathematical Synthesizer";
    bool unrollIterations = false;
    bool bakeFixedParameters = false;
    bool enableShadows = true;
    bool enableAmbientOcclusion = true;
    bool enableChromaticDispersion = true;
    float dispersionStrength = 0.015f;
    std::uint32_t raymarchMaxSteps = 96U;
    float raymarchHitThreshold = 0.0015f;
    float raymarchMaxDistance = 12.0f;
    bool enableWebAudioSonification = true;
    bool enableInteractivePresetMenu = true;
    bool enableTourAutoPlay = false;
    float tourDwellSeconds = 4.0f;
    bool enableLearnModeDeconstruction = true;
};

class SculptureShaderSynthesizer {
public:
    // Generates standalone GLSL fragment shader formatted for Shadertoy / WebGL
    [[nodiscard]] static std::string synthesizeShadertoyGLSL(
        const SculptureProgramParams& params,
        const ShaderSynthesisConfig& config = {}
    );

    // Generates standalone Vulkan GLSL compute shader (GL_ARB_compute_shader)
    [[nodiscard]] static std::string synthesizeVulkanComputeGLSL(
        const SculptureProgramParams& params,
        const ShaderSynthesisConfig& config = {}
    );

    // Generates standalone OpenCL 3.0 compute kernel (.cl)
    [[nodiscard]] static std::string synthesizeStandaloneOpenCL(
        const SculptureProgramParams& params,
        const ShaderSynthesisConfig& config = {}
    );

    // Generates complete single-file interactive WebGL2 / HTML5 app
    [[nodiscard]] static std::string synthesizeStandaloneHTML5(
        const SculptureProgramParams& params,
        const ShaderSynthesisConfig& config = {}
    );

    // Computes a deterministic 64-bit FNV-1a hash key for OpenCL/GLSL program caching
    [[nodiscard]] static std::uint64_t computeKernelCacheKey(
        const SculptureProgramParams& params
    ) noexcept {
        std::uint64_t h = 0xcbf29ce484222325ULL;
        auto mix = [&h](std::uint32_t val) {
            h ^= static_cast<std::uint64_t>(val);
            h *= 0x100000001b3ULL;
        };
        mix(static_cast<std::uint32_t>(params.inner));
        mix(static_cast<std::uint32_t>(params.innerB));
        mix(static_cast<std::uint32_t>(params.seedType));
        mix(static_cast<std::uint32_t>(params.seedTypeB));
        mix(static_cast<std::uint32_t>(params.reduction));
        mix(static_cast<std::uint32_t>(params.reductionB));
        mix(static_cast<std::uint32_t>(params.colorEffect));
        return h;
    }

    // High-Level File Export Helpers
    static bool exportToFile(
        const std::string& filepath,
        const SculptureProgramParams& params,
        const ShaderSynthesisConfig& config = {}
    );

    static bool exportHtmlToFile(
        const std::string& filepath,
        const SculptureProgramParams& params,
        const ShaderSynthesisConfig& config = {}
    );
};

} // namespace epi_sculpt
