// Native mGFX/Vulkan presentation adapter for the Recursive Counter-Winding Hopf Torus.
//
// Consumes the immutable Grammar / TraversalResult contract and emits instanced
// draw calls using shared circular tube prototypes (or corrugated woven prototypes
// when weaveAmplitude > 0), optional crossing/interstitial site lattice markers,
// and an optional translucent stereographic Clifford torus substrate.

#pragma once

#include "RecursiveHopfTorusGpuAbi.h"

#include "mgfx/Drawable.h"
#include "mgfx/math/Types.h"
#include "mgfx/pbr/Material.h"
#include "mgfx/vk/Instance.h"
#include "mgfx/vk/Mesh.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace mgfx {
class Engine;
}

namespace epi_sculpt::recursive_hopf {

struct RecursiveHopfDelivery;
class RecursiveHopfExternalGrammarView;

enum class SiteMarkerMode : std::uint8_t {
    Off = 0,
    CrossingsOnly = 1,
    InterstitialsOnly = 2,
    CrossingsAndInterstitials = 3,
};

struct RecursiveHopfPalette {
    mgfx::math::vec3 plusPrimary{1.00f, 0.78f, 0.22f};   // Solar gold for (1,1)
    mgfx::math::vec3 plusDeep{1.00f, 0.36f, 0.12f};      // Warm amber at deep levels
    mgfx::math::vec3 minusPrimary{0.14f, 0.82f, 1.00f};  // Glacial cyan for (-1,1)
    mgfx::math::vec3 minusDeep{0.58f, 0.32f, 1.00f};     // Violet sapphire at deep levels
    mgfx::math::vec3 crossingColor{1.00f, 0.96f, 0.88f}; // Incandescent white-gold
    mgfx::math::vec3 interstitialColor{0.18f, 1.00f, 0.68f}; // Emerald rosette anchor
    float metallic = 0.86f;
    float roughness = 0.18f;
    float baseEmission = 0.35f;
    float depthEmissionBoost = 0.55f;
};

struct RecursiveHopfRenderOptions {
    TraversalBudget budget{};
    RecursiveHopfPalette palette{};
    SiteMarkerMode siteMarkerMode = SiteMarkerMode::Off;
    bool showSubstrateTorus = false;
    float markerScale = 0.045f;
    mgfx::math::mat4 rootTransform = mgfx::math::mat4(1.0f);
};

struct RecursiveHopfRenderBatches {
    std::vector<mgfx::vk::InstanceData> plusFibersLod0{};
    std::vector<mgfx::vk::InstanceData> plusFibersLod1{};
    std::vector<mgfx::vk::InstanceData> minusFibersLod0{};
    std::vector<mgfx::vk::InstanceData> minusFibersLod1{};
    std::vector<mgfx::vk::InstanceData> crossingMarkers{};
    std::vector<mgfx::vk::InstanceData> interstitialMarkers{};
    std::uint32_t deepestObservedDepth = 0U;
    std::uint32_t visitedNodes = 0U;
    std::uint32_t emittedFibers = 0U;
    std::uint32_t frustumCulledNodes = 0U;
    std::uint32_t lodCulledNodes = 0U;
    std::uint32_t budgetOmittedNodes = 0U;
    bool truncatedByBudget = false;
    bool wovenMode = false;
    // GPU route returns statistics without materializing the instance vectors.
    bool deviceLocalInstanceStream = false;
    // Native lease pressure may defer all geometry while the host still
    // submits HUD/clear work. Reference selection counts are not draw evidence.
    bool submissionDeferred = false;
    bool externalGrammar = false;
    std::uint64_t externalGrammarAllocation = 0U;
    std::uint64_t externalGrammarHandoff = 0U;
};

class RecursiveHopfTorusRenderBridge final {
public:
    RecursiveHopfTorusRenderBridge();
    ~RecursiveHopfTorusRenderBridge();

    RecursiveHopfTorusRenderBridge(const RecursiveHopfTorusRenderBridge&) = delete;
    RecursiveHopfTorusRenderBridge& operator=(const RecursiveHopfTorusRenderBridge&) = delete;

    // Pure coordinate & transform helpers (callable in device-free tests):
    // Maps mathematical Z-up (x, y, z) to mGFX Y-up (x, z, -y).
    [[nodiscard]] static mgfx::math::vec3 mathToEnginePoint(const Point& p) noexcept;
    // Maps mGFX Y-up (x, y, z) back to mathematical Z-up (x, -z, y).
    [[nodiscard]] static Point engineToMathPoint(const mgfx::math::vec3& p) noexcept;
    // Builds a mathematical Z-up ObservationEye from an engine Y-up camera configuration.
    [[nodiscard]] static ObservationEye makeObservationEye(
        const mgfx::math::vec3& engineEyePos,
        const mgfx::math::vec3& engineTargetPos,
        float fovYRadians,
        float aspectRatio,
        std::uint32_t viewportHeight) noexcept;
    // Live drawing uses the renderer-selected perspective and raster grid,
    // including off-center XR projections. Coordinates assume identity root.
    [[nodiscard]] static ObservationEye makeObservationEye(
        const mgfx::FrameView& view) noexcept;
    // Maps a local XZ-plane torus mesh of major radius sqrt(2) onto a world-space Circle.
    [[nodiscard]] static mgfx::math::mat4 circleModelMatrix(
        const Circle& worldCircle,
        double nodeScale) noexcept;
    // Maps a mesh authored in math Z-up (converted via mathToEnginePoint) through a node Similarity.
    [[nodiscard]] static mgfx::math::mat4 nodeModelMatrix(
        const Similarity& nodeTransform) noexcept;

    // Device-free batch assembly from a prepared Grammar.
    [[nodiscard]] static RecursiveHopfRenderBatches buildRenderBatches(
        const Grammar& grammar,
        const TraversalResult& traversal,
        const RecursiveHopfRenderOptions& options = {});

    // Device-free batch assembly directly from an OpenCL/GPU-expanded world-circle
    // instance stream (plus optional CPU-evaluated crossing/interstitial markers).
    [[nodiscard]] static RecursiveHopfRenderBatches buildRenderBatchesFromGpuInstances(
        const Grammar& grammar,
        std::span<const gpu_abi::RecursiveHopfInstanceGpu> gpuInstances,
        const RecursiveHopfRenderOptions& options = {},
        const TraversalResult* optionalMarkers = nullptr);

    // Live Vulkan/mGFX submission. Lazily creates and caches prototype meshes and materials.
    RecursiveHopfRenderBatches draw(
        mgfx::Drawable& drawable,
        const Grammar& grammar,
        const RecursiveHopfRenderOptions& options = {});

    // Live Vulkan/mGFX submission consuming an OpenCL-expanded world-circle
    // instance stream when non-empty (falling back to cached CPU traversal when
    // woven mode, view-cone culling, or site markers require full node frames).
    RecursiveHopfRenderBatches drawExpanded(
        mgfx::Drawable& drawable,
        const Grammar& grammar,
        std::span<const gpu_abi::RecursiveHopfInstanceGpu> gpuInstances,
        const RecursiveHopfRenderOptions& options = {});

    // Prefer renderer-local, device-local expansion of the published grammar
    // for exact uniform circles without CPU view culling/markers. Unsupported
    // geometry or bounded native pressure uses the existing CPU presentation.
    RecursiveHopfRenderBatches drawDelivery(
        mgfx::Drawable& drawable,
        const RecursiveHopfDelivery& delivery,
        const RecursiveHopfRenderOptions& options = {},
        std::shared_ptr<const RecursiveHopfExternalGrammarView> externalGrammar = {});

    [[nodiscard]] static bool acceptsDeviceLocalDelivery(
        const RecursiveHopfDelivery& delivery,
        const RecursiveHopfRenderOptions& options) noexcept;

private:
    struct GpuCache;
    std::unique_ptr<GpuCache> cache_;
};

} // namespace epi_sculpt::recursive_hopf
