// Versioned host/GPU transfer contract for the Recursive Counter-Winding Hopf Torus.
//
// Keeps the finite grammar representation (2m root circle descriptors + B=2m^2
// interstitial child similarity transforms + crossing anchors) and bounded
// expanded fiber instance streams in a fixed-width, 16-byte aligned ABI shared
// by CPU, OpenCL, and Vulkan presentation layers.

#pragma once

#include "RecursiveHopfTorusMath.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>

namespace epi_sculpt::recursive_hopf::gpu_abi {

inline constexpr std::uint32_t kVersion = 1U;
inline constexpr std::uint32_t kRecipeBytes = 64U;
inline constexpr std::uint32_t kCircleBytes = 64U;
inline constexpr std::uint32_t kChildMapBytes = 96U;
inline constexpr std::uint32_t kGrammarHeaderBytes = 64U;
inline constexpr std::uint32_t kInstanceBytes = 80U;

inline constexpr std::uint32_t kMaxRootCircles = 2U * kMaxFibersPerFamily; // 16
inline constexpr std::uint32_t kMaxChildrenPerNode =
    2U * kMaxFibersPerFamily * kMaxFibersPerFamily; // 128
inline constexpr std::uint32_t kMaxExpandedInstances = 16384U;

inline constexpr std::uint32_t kFlagWovenMode = 0x00000001U;
inline constexpr std::uint32_t kFlagConformalScale = 0x00000002U;
inline constexpr std::uint32_t kFlagStrictClearance = 0x00000004U;

struct alignas(16) RecursiveHopfRecipeGpu {
    std::uint32_t abiVersion = kVersion;
    std::uint32_t byteSize = kRecipeBytes;
    std::uint32_t fibersPerFamily = 3U;
    std::uint32_t logicalDepth = 2U;

    float plusInitialRadians = 0.0f;
    float plusRadiansPerSecond = 0.1f;
    float minusInitialRadians = 0.0f;
    float minusRadiansPerSecond = -0.1f;

    float childScale = 0.1f;
    float childTwistRadians = 0.0f;
    float childNormalOffset = 0.0f;
    float fiberRadius = 0.02f;

    float conformalScaleExponent = 0.0f;
    float weaveAmplitude = 0.0f;
    std::uint32_t flags = 0U;
    std::uint32_t placementMode = 0U;
};

struct alignas(16) RecursiveHopfCircleGpu {
    std::array<float, 4> centerRadius{}; // xyz = center in math Z-up, w = radius (sqrt(2) at root)
    std::array<float, 4> axisX{};        // xyz = plane basis e1, w = chirality (+1 plus, -1 minus)
    std::array<float, 4> axisY{};        // xyz = plane basis e2, w = fiber index (0..m-1)
    std::array<float, 4> axisZ{};        // xyz = circle normal n = e1 x e2, w = fiber phase (rad)
};

struct alignas(16) RecursiveHopfChildMapGpu {
    std::array<float, 4> translationScale{}; // xyz = interstitial child translation, w = child uniform scale
    std::array<float, 4> axisX{};            // xyz = child frame column x, w = interstitial u*
    std::array<float, 4> axisY{};            // xyz = child frame column y, w = interstitial v*
    std::array<float, 4> axisZ{};            // xyz = child frame column z, w = crossing u
    std::array<float, 4> crossingPosition{}; // xyz = crossing position on T^2, w = crossing v
    std::uint32_t plusFiber = 0U;
    std::uint32_t minusFiber = 0U;
    std::uint32_t branch = 0U;
    std::uint32_t childIndex = 0U;
};

struct alignas(16) RecursiveHopfGrammarHeaderGpu {
    std::uint32_t abiVersion = kVersion;
    std::uint32_t byteSize = 0U;
    std::uint32_t generation = 0U;
    std::uint32_t rootCircleCount = 0U;

    std::uint32_t childrenPerNode = 0U;
    std::uint32_t logicalDepth = 0U;
    std::uint32_t flags = 0U;
    std::uint32_t reserved0 = 0U;

    float domainTime = 0.0f;
    float plusPhase = 0.0f;
    float minusPhase = 0.0f;
    float subtreeRadius = 0.0f;

    std::uint64_t logicalNodes = 0U;
    std::uint64_t logicalFibers = 0U;
};

struct alignas(16) RecursiveHopfGrammarGpu {
    RecursiveHopfGrammarHeaderGpu header{};
    RecursiveHopfRecipeGpu recipe{};
    std::array<RecursiveHopfCircleGpu, kMaxRootCircles> rootCircles{};
    std::array<RecursiveHopfChildMapGpu, kMaxChildrenPerNode> children{};
};

// Bounded world-space fiber instance record emitted by CPU or OpenCL hierarchy expansion.
struct alignas(16) RecursiveHopfInstanceGpu {
    std::array<float, 4> centerRadius{}; // xyz = world circle center (math Z-up), w = world circle radius
    std::array<float, 4> axisX{};        // xyz = world circle basis e1, w = node uniform scale
    std::array<float, 4> axisY{};        // xyz = world circle basis e2, w = chirality (+1 plus, -1 minus)
    std::array<float, 4> axisZ{};        // xyz = world circle normal, w = fiberRadius * nodeScale
    std::uint32_t nodeIdLow = 0U;
    std::uint32_t depth = 0U;
    std::uint32_t chirality = 0U; // 0 = Plus, 1 = Minus
    std::uint32_t fiberIndex = 0U;
};

[[nodiscard]] RecursiveHopfRecipeGpu packRecipe(const Recipe& recipe) noexcept;
[[nodiscard]] bool isValid(const RecursiveHopfRecipeGpu& recipe) noexcept;

[[nodiscard]] RecursiveHopfGrammarGpu packGrammar(
    const Grammar& grammar,
    std::uint32_t generation = 1U) noexcept;
[[nodiscard]] bool isValid(const RecursiveHopfGrammarGpu& grammar) noexcept;

[[nodiscard]] std::size_t packExpandedInstances(
    const Grammar& grammar,
    const TraversalResult& traversal,
    std::span<RecursiveHopfInstanceGpu> outInstances) noexcept;

static_assert(std::is_standard_layout_v<RecursiveHopfRecipeGpu>);
static_assert(std::is_trivially_copyable_v<RecursiveHopfRecipeGpu>);
static_assert(sizeof(RecursiveHopfRecipeGpu) == kRecipeBytes);
static_assert(alignof(RecursiveHopfRecipeGpu) == 16U);

static_assert(std::is_standard_layout_v<RecursiveHopfCircleGpu>);
static_assert(std::is_trivially_copyable_v<RecursiveHopfCircleGpu>);
static_assert(sizeof(RecursiveHopfCircleGpu) == kCircleBytes);
static_assert(alignof(RecursiveHopfCircleGpu) == 16U);

static_assert(std::is_standard_layout_v<RecursiveHopfChildMapGpu>);
static_assert(std::is_trivially_copyable_v<RecursiveHopfChildMapGpu>);
static_assert(sizeof(RecursiveHopfChildMapGpu) == kChildMapBytes);
static_assert(alignof(RecursiveHopfChildMapGpu) == 16U);

static_assert(std::is_standard_layout_v<RecursiveHopfGrammarHeaderGpu>);
static_assert(std::is_trivially_copyable_v<RecursiveHopfGrammarHeaderGpu>);
static_assert(sizeof(RecursiveHopfGrammarHeaderGpu) == kGrammarHeaderBytes);
static_assert(alignof(RecursiveHopfGrammarHeaderGpu) == 16U);

static_assert(std::is_standard_layout_v<RecursiveHopfGrammarGpu>);
static_assert(std::is_trivially_copyable_v<RecursiveHopfGrammarGpu>);
static_assert(offsetof(RecursiveHopfGrammarGpu, header) == 0U);
static_assert(offsetof(RecursiveHopfGrammarGpu, recipe) == 64U);
static_assert(offsetof(RecursiveHopfGrammarGpu, rootCircles) == 128U);
static_assert(offsetof(RecursiveHopfGrammarGpu, children) == 128U + kMaxRootCircles * kCircleBytes);

static_assert(std::is_standard_layout_v<RecursiveHopfInstanceGpu>);
static_assert(std::is_trivially_copyable_v<RecursiveHopfInstanceGpu>);
static_assert(sizeof(RecursiveHopfInstanceGpu) == kInstanceBytes);
static_assert(alignof(RecursiveHopfInstanceGpu) == 16U);

} // namespace epi_sculpt::recursive_hopf::gpu_abi
