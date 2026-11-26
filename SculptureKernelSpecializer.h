/**
 * ============================================================================
 * EPI2 / 3dEpiSculpTMorph - MODULAR OPENCL KERNEL SPECIALIZER
 * ============================================================================
 *
 * Educational Annotation: BRANCHLESS GPU KERNEL SPECIALIZATION
 * ----------------------------------------------------------------------------
 * Governed by docu/project-vision.txt lines 34-36 & 165-202:
 *  "OpenCL modular kernel system with fast recompilation and runtime parameter
 *   specialization... the primary target implementation will be: OpenCL...
 *   main focus backend is openCL !!"
 *
 * Problem on Modern GPU Architectures:
 *  Modern GPUs group execution threads into 32-thread warps (NVIDIA) or 64-thread
 *  wavefronts (AMD). In a raymarching loop evaluating 38+ InnerMaps or reductions
 *  via dynamic switch() statements, adjacent rays taking different execution paths
 *  cause warp divergence, forcing sequential serialization of divergent branches.
 *
 * Architectural Solution:
 *  SculptureKernelSpecializer generates specialized compile-time OpenCL preprocessor
 *  directives, inlined mathematical functions, and compiler build options tailored
 *  to the active preset parameters. Divergent runtime branch evaluation is completely
 *  eliminated from the GPU inner loop, yielding maximum SIMD execution efficiency.
 * ============================================================================
 */

#pragma once

#include "SculptureProgramSchema.h"
#include <string>
#include <string_view>
#include <vector>
#include <cstdint>

namespace epi_sculpt {

struct KernelSpecializationConfig {
    bool inlineInnerMap = true;
    bool inlineReduction = true;
    bool inlineColorEffect = true;
    bool unrollFeedbackLoop = false;
    bool fastRelaxedMath = true;
    bool enableWavefrontOptimizations = true;
    std::uint32_t targetOpenClVersion = 300U; // OpenCL 3.0
};

struct SpecializedKernelSource {
    std::uint64_t signatureHash = 0ULL;
    std::string buildOptions;
    std::string specializedPreamble;
    std::string inlinedInnerSnippet;
    std::string inlinedReductionSnippet;
    std::string inlinedColorSnippet;
    std::string specializedKernelSource;
};

class SculptureKernelSpecializer {
public:
    // Computes deterministic 64-bit specialization signature hash
    [[nodiscard]] static std::uint64_t computeSignatureHash(
        const SculptureProgramParams& params,
        const KernelSpecializationConfig& config = {}
    ) noexcept;

    // Generates OpenCL build options string (-D flags, -cl-fast-relaxed-math, etc.)
    [[nodiscard]] static std::string generateBuildOptions(
        const SculptureProgramParams& params,
        const KernelSpecializationConfig& config = {}
    );

    // Generates complete inlined OpenCL C mathematical preamble
    [[nodiscard]] static std::string generateSpecializedPreamble(
        const SculptureProgramParams& params,
        const KernelSpecializationConfig& config = {}
    );

    // Generates specialized raymarching / transformation kernel source
    [[nodiscard]] static SpecializedKernelSource specializeKernel(
        const SculptureProgramParams& params,
        std::string_view baseKernelSource,
        const KernelSpecializationConfig& config = {}
    );

    // Human-readable diagnostic description of specialized stages
    [[nodiscard]] static std::string formatSpecializationSummary(
        const SculptureProgramParams& params
    );
};

} // namespace epi_sculpt
