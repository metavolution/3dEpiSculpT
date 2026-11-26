#include "RecursiveHopfTorusRenderBridge.h"
#include "RecursiveHopfDeviceInstanceStream.h"
#include "RecursiveHopfTorusOpenCl.h"

#include "mgfx/Engine.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <numbers>
#include <stdexcept>
#include <utility>

namespace epi_sculpt::recursive_hopf {

namespace {

constexpr double kSqrtTwo = std::numbers::sqrt2_v<double>;
constexpr double kTau = 2.0 * std::numbers::pi_v<double>;
constexpr double kLod0ScaleThreshold = 0.06;

mgfx::math::vec3 lerpVec3(
    const mgfx::math::vec3& a,
    const mgfx::math::vec3& b,
    float t) noexcept {
    const float u = std::clamp(t, 0.0f, 1.0f);
    return a * (1.0f - u) + b * u;
}

// Build a smooth closed tubular mesh around a 3D curve evaluated in math Z-up
// and converted to mGFX Y-up coordinates using a Wang double-reflection
// Rotation-Minimizing Frame (RMF) with exact periodic holonomy closure.
mgfx::vk::Mesh buildWovenFiberPrototypeMesh(
    mgfx::vk::Context& ctx,
    const Grammar& rootGrammar,
    Chirality family,
    std::uint32_t tubularSegments,
    std::uint32_t radialSegments) {
    using mgfx::math::vec2;
    using mgfx::math::vec3;
    using mgfx::math::vec4;
    using mgfx::vk::Vertex;

    const std::uint32_t cols = tubularSegments + 1U;
    const std::uint32_t rows = radialSegments + 1U;
    const float tubeRadius = static_cast<float>(rootGrammar.recipe().fiberRadius);

    std::vector<vec3> centers(cols);
    std::vector<vec3> tangents(cols);
    std::vector<vec3> normals(cols);

    vec3 surfNormal0(0.0f, 1.0f, 0.0f);
    for (std::uint32_t j = 0U; j < cols; ++j) {
        const double uCoord = static_cast<double>(j % tubularSegments) / static_cast<double>(tubularSegments);
        const double t = kTau * uCoord;
        const double eps = 1.0e-3;
        const Point p0 = rootGrammar.wovenCurvePoint(family, 0U, t);
        const Point pPrev = rootGrammar.wovenCurvePoint(family, 0U, t - eps);
        const Point pNext = rootGrammar.wovenCurvePoint(family, 0U, t + eps);

        const Point dp{
            (pNext[0] - pPrev[0]) / (2.0 * eps),
            (pNext[1] - pPrev[1]) / (2.0 * eps),
            (pNext[2] - pPrev[2]) / (2.0 * eps),
        };
        centers[j] = RecursiveHopfTorusRenderBridge::mathToEnginePoint(p0);
        tangents[j] = glm::normalize(RecursiveHopfTorusRenderBridge::mathToEnginePoint(dp));

        if (j == 0U) {
            const double phase0 = (family == Chirality::Plus)
                ? rootGrammar.phases().plus
                : rootGrammar.phases().minus;
            const double uTorus = ((family == Chirality::Plus) ? t : -t) + phase0;
            const Basis surfFrame = torusFrame(uTorus, t);
            surfNormal0 = glm::normalize(RecursiveHopfTorusRenderBridge::mathToEnginePoint(surfFrame.z));
        }
    }

    // Orthogonalize initial normal against T_0 and propagate via double reflection.
    vec3 n0 = surfNormal0 - tangents[0] * glm::dot(surfNormal0, tangents[0]);
    const float n0Len = glm::length(n0);
    n0 = (n0Len > 1.0e-5f) ? (n0 / n0Len) : vec3(0.0f, 1.0f, 0.0f);

    std::vector<vec3> rmfNormals(cols);
    rmfNormals[0] = n0;
    for (std::uint32_t j = 0U; j < tubularSegments; ++j) {
        const vec3 v1 = centers[j + 1U] - centers[j];
        const float c1 = glm::dot(v1, v1);
        if (c1 < 1.0e-12f) {
            rmfNormals[j + 1U] = rmfNormals[j];
            continue;
        }
        const vec3 rL = rmfNormals[j] - (2.0f / c1) * glm::dot(v1, rmfNormals[j]) * v1;
        const vec3 tL = tangents[j] - (2.0f / c1) * glm::dot(v1, tangents[j]) * v1;
        const vec3 v2 = tangents[j + 1U] - tL;
        const float c2 = glm::dot(v2, v2);
        vec3 nNext = (c2 > 1.0e-12f)
            ? (rL - (2.0f / c2) * glm::dot(v2, rL) * v2)
            : rL;
        nNext -= tangents[j + 1U] * glm::dot(nNext, tangents[j + 1U]);
        const float len = glm::length(nNext);
        rmfNormals[j + 1U] = (len > 1.0e-5f) ? (nNext / len) : rmfNormals[j];
    }

    // Distribute residual periodic holonomy twist uniformly along the closed loop.
    const vec3 nEnd = rmfNormals[tubularSegments];
    const vec3 b0 = glm::normalize(glm::cross(tangents[0], n0));
    const float holonomy = std::atan2(glm::dot(nEnd, b0), glm::dot(nEnd, n0));

    for (std::uint32_t j = 0U; j < cols; ++j) {
        if (j == tubularSegments) {
            normals[j] = n0;
            continue;
        }
        const float frac = static_cast<float>(j) / static_cast<float>(tubularSegments);
        const float angle = -holonomy * frac;
        const vec3 T = tangents[j];
        const vec3 N = rmfNormals[j];
        const vec3 B = glm::normalize(glm::cross(T, N));
        normals[j] = glm::normalize(N * std::cos(angle) + B * std::sin(angle));
    }

    std::vector<Vertex> vertices;
    vertices.reserve(rows * cols);
    const vec4 white(1.0f);

    for (std::uint32_t i = 0U; i < rows; ++i) {
        const float vCoord = static_cast<float>(i) / static_cast<float>(radialSegments);
        const float phi = static_cast<float>(kTau) * vCoord;
        const float cph = std::cos(phi);
        const float sph = std::sin(phi);

        for (std::uint32_t j = 0U; j < cols; ++j) {
            const float uCoord = static_cast<float>(j) / static_cast<float>(tubularSegments);
            const vec3 T = tangents[j];
            const vec3 N = normals[j];
            const vec3 B = glm::normalize(glm::cross(T, N));

            const vec3 radialDir = glm::normalize(N * cph + B * sph);
            const vec3 pos = centers[j] + radialDir * tubeRadius;
            vertices.push_back(Vertex{
                .position = pos,
                .normal = radialDir,
                .uv = vec2(uCoord, vCoord),
                .color = white,
                .tangent = vec4(T, 1.0f),
            });
        }
    }

    std::vector<std::uint32_t> indices;
    indices.reserve(radialSegments * tubularSegments * 6U);
    for (std::uint32_t i = 0U; i < radialSegments; ++i) {
        for (std::uint32_t j = 0U; j < tubularSegments; ++j) {
            const std::uint32_t a = i * cols + j;
            const std::uint32_t b = a + 1U;
            const std::uint32_t c = a + cols + 1U;
            const std::uint32_t d = a + cols;
            indices.push_back(a);
            indices.push_back(b);
            indices.push_back(c);
            indices.push_back(a);
            indices.push_back(c);
            indices.push_back(d);
        }
    }

    return mgfx::vk::Mesh(ctx, vertices, indices);
}

// Build the exact stereographic Clifford torus P(u,v) = (cos u, sin u, cos v)/(sqrt(2) - sin v)
// as a renderable mGFX mesh in engine Y-up coordinates.
mgfx::vk::Mesh buildCliffordTorusSubstrateMesh(
    mgfx::vk::Context& ctx,
    std::uint32_t uSegments,
    std::uint32_t vSegments) {
    using mgfx::math::vec2;
    using mgfx::math::vec3;
    using mgfx::math::vec4;
    using mgfx::vk::Vertex;

    const std::uint32_t cols = uSegments + 1U;
    const std::uint32_t rows = vSegments + 1U;
    std::vector<Vertex> vertices;
    vertices.reserve(rows * cols);
    const vec4 white(1.0f);

    for (std::uint32_t i = 0U; i < rows; ++i) {
        const double vCoord = static_cast<double>(i % vSegments) / static_cast<double>(vSegments);
        const double v = kTau * vCoord;
        for (std::uint32_t j = 0U; j < cols; ++j) {
            const double uCoord = static_cast<double>(j % uSegments) / static_cast<double>(uSegments);
            const double u = kTau * uCoord;
            const Point p = torusPoint(u, v);
            const Basis f = torusFrame(u, v);
            const vec3 pos = RecursiveHopfTorusRenderBridge::mathToEnginePoint(p);
            const vec3 nrm = glm::normalize(RecursiveHopfTorusRenderBridge::mathToEnginePoint(f.z));
            const vec3 tanU = glm::normalize(RecursiveHopfTorusRenderBridge::mathToEnginePoint(f.x));
            vertices.push_back(Vertex{
                .position = pos,
                .normal = nrm,
                .uv = vec2(static_cast<float>(uCoord), static_cast<float>(vCoord)),
                .color = white,
                .tangent = vec4(tanU, 1.0f),
            });
        }
    }

    std::vector<std::uint32_t> indices;
    indices.reserve(uSegments * vSegments * 6U);
    for (std::uint32_t i = 0U; i < vSegments; ++i) {
        for (std::uint32_t j = 0U; j < uSegments; ++j) {
            const std::uint32_t a = i * cols + j;
            const std::uint32_t b = a + 1U;
            const std::uint32_t c = a + cols + 1U;
            const std::uint32_t d = a + cols;
            indices.push_back(a);
            indices.push_back(b);
            indices.push_back(c);
            indices.push_back(a);
            indices.push_back(c);
            indices.push_back(d);
        }
    }
    return mgfx::vk::Mesh(ctx, vertices, indices);
}

[[nodiscard]] bool exactEyeEqual(const ObservationEye& a, const ObservationEye& b) noexcept {
    return a.position == b.position && a.forward == b.forward
        && a.cosHalfFov == b.cosHalfFov
        && a.focalLengthPixels == b.focalLengthPixels;
}

[[nodiscard]] bool exactBudgetEqual(const TraversalBudget& a, const TraversalBudget& b) noexcept {
    if (a.maxDepth != b.maxDepth || a.maxNodes != b.maxNodes
        || a.maxFibers != b.maxFibers || a.minWorldScale != b.minWorldScale
        || a.includeCrossings != b.includeCrossings
        || a.includeInterstitials != b.includeInterstitials
        || a.enableViewCulling != b.enableViewCulling) {
        return false;
    }
    if (!a.enableViewCulling) return true;
    if (a.eyeCount != b.eyeCount
        || a.minProjectedRadiusPixels != b.minProjectedRadiusPixels) {
        return false;
    }
    const std::uint32_t count = std::min<std::uint32_t>(a.eyeCount, 2U);
    for (std::uint32_t i = 0U; i < count; ++i) {
        if (!exactEyeEqual(a.eyes[i], b.eyes[i])) return false;
    }
    return true;
}

[[nodiscard]] bool exactPaletteEqual(
    const RecursiveHopfPalette& a,
    const RecursiveHopfPalette& b) noexcept {
    return a.plusPrimary == b.plusPrimary && a.plusDeep == b.plusDeep
        && a.minusPrimary == b.minusPrimary && a.minusDeep == b.minusDeep
        && a.crossingColor == b.crossingColor
        && a.interstitialColor == b.interstitialColor
        && a.metallic == b.metallic && a.roughness == b.roughness
        && a.baseEmission == b.baseEmission
        && a.depthEmissionBoost == b.depthEmissionBoost;
}

void populateRenderBatchesInto(
    const Grammar& grammar,
    const TraversalResult& traversal,
    const RecursiveHopfRenderOptions& options,
    RecursiveHopfRenderBatches& batches) {
    batches.plusFibersLod0.clear();
    batches.plusFibersLod1.clear();
    batches.minusFibersLod0.clear();
    batches.minusFibersLod1.clear();
    batches.crossingMarkers.clear();
    batches.interstitialMarkers.clear();

    batches.deepestObservedDepth = traversal.deepestObservedDepth;
    batches.visitedNodes = traversal.visitedNodes;
    batches.emittedFibers = traversal.emittedFibers;
    batches.frustumCulledNodes = traversal.frustumCulledNodes;
    batches.lodCulledNodes = traversal.lodCulledNodes;
    batches.budgetOmittedNodes = traversal.budgetOmittedNodes;
    batches.truncatedByBudget = traversal.truncatedByBudget;
    batches.wovenMode = grammar.recipe().weaveAmplitude > 0.0;

    const std::uint32_t m = std::max(1U, grammar.recipe().fibersPerFamily);
    const float maxDepthFloat = static_cast<float>(std::max(1U, traversal.effectiveMaxDepth));

    batches.plusFibersLod0.reserve(traversal.fibers.size() / 2U);
    batches.minusFibersLod0.reserve(traversal.fibers.size() / 2U);

    for (const auto& fiber : traversal.fibers) {
        const float depthFrac = static_cast<float>(fiber.depth) / maxDepthFloat;
        const float fiberHarmonic = 0.88f + 0.12f * std::cos(
            static_cast<float>(kTau * fiber.fiber) / static_cast<float>(m));

        const mgfx::math::vec3 baseRgb = (fiber.family == Chirality::Plus)
            ? lerpVec3(options.palette.plusPrimary, options.palette.plusDeep, depthFrac)
            : lerpVec3(options.palette.minusPrimary, options.palette.minusDeep, depthFrac);

        const float emission = options.palette.baseEmission
            + depthFrac * options.palette.depthEmissionBoost;

        mgfx::math::mat4 model{};
        if (batches.wovenMode) {
            // In woven mode, fiber 0 is the prototype and fiber k is rotated by
            // 2*pi*k/m around the local torus Z axis (which is +Y in engine space).
            const float angleRad = static_cast<float>(kTau * fiber.fiber) / static_cast<float>(m);
            const mgfx::math::mat4 rotY = glm::rotate(
                mgfx::math::mat4(1.0f), angleRad, mgfx::math::vec3(0.0f, 1.0f, 0.0f));
            model = options.rootTransform * RecursiveHopfTorusRenderBridge::nodeModelMatrix(fiber.nodeTransform) * rotY;
        } else {
            model = options.rootTransform * RecursiveHopfTorusRenderBridge::circleModelMatrix(fiber.worldCircle, fiber.nodeTransform.scale);
        }

        const mgfx::vk::InstanceData inst{
            .model = model,
            .baseColorTint = mgfx::math::vec4(baseRgb * fiberHarmonic, 1.0f),
            .pbrOverride = mgfx::math::vec4(
                options.palette.metallic,
                std::clamp(options.palette.roughness + 0.04f * depthFrac, 0.05f, 0.95f),
                0.0f,
                emission),
            .velocity = mgfx::math::vec4(0.0f),
        };

        const bool useLod0 = (fiber.nodeTransform.scale >= kLod0ScaleThreshold);
        if (fiber.family == Chirality::Plus) {
            if (useLod0) batches.plusFibersLod0.push_back(inst);
            else batches.plusFibersLod1.push_back(inst);
        } else {
            if (useLod0) batches.minusFibersLod0.push_back(inst);
            else batches.minusFibersLod1.push_back(inst);
        }
    }

    for (const auto& cross : traversal.crossings) {
        const float r = static_cast<float>(cross.worldScale) * options.markerScale;
        if (r < 1.0e-4f) continue;
        mgfx::math::mat4 mMat = glm::translate(
            mgfx::math::mat4(1.0f), RecursiveHopfTorusRenderBridge::mathToEnginePoint(cross.worldPosition));
        mMat = glm::scale(mMat, mgfx::math::vec3(r));
        batches.crossingMarkers.push_back(mgfx::vk::InstanceData{
            .model = options.rootTransform * mMat,
            .baseColorTint = mgfx::math::vec4(options.palette.crossingColor, 1.0f),
            .pbrOverride = mgfx::math::vec4(0.35f, 0.10f, 0.0f, 1.35f),
            .velocity = mgfx::math::vec4(0.0f),
        });
    }

    for (const auto& inter : traversal.interstitials) {
        const float r = static_cast<float>(inter.worldScale) * options.markerScale * 1.35f;
        if (r < 1.0e-4f) continue;
        mgfx::math::mat4 mMat = glm::translate(
            mgfx::math::mat4(1.0f), RecursiveHopfTorusRenderBridge::mathToEnginePoint(inter.worldPosition));
        mMat = glm::scale(mMat, mgfx::math::vec3(r));
        batches.interstitialMarkers.push_back(mgfx::vk::InstanceData{
            .model = options.rootTransform * mMat,
            .baseColorTint = mgfx::math::vec4(options.palette.interstitialColor, 1.0f),
            .pbrOverride = mgfx::math::vec4(0.55f, 0.12f, 0.0f, 1.15f),
            .velocity = mgfx::math::vec4(0.0f),
        });
    }
}

void populateRenderBatchesFromGpuInstancesInto(
    const Grammar& grammar,
    std::span<const gpu_abi::RecursiveHopfInstanceGpu> gpuInstances,
    const RecursiveHopfRenderOptions& options,
    const TraversalResult* optionalMarkers,
    RecursiveHopfRenderBatches& batches) {
    batches.plusFibersLod0.clear();
    batches.plusFibersLod1.clear();
    batches.minusFibersLod0.clear();
    batches.minusFibersLod1.clear();
    batches.crossingMarkers.clear();
    batches.interstitialMarkers.clear();

    const std::uint32_t m = std::max(1U, grammar.recipe().fibersPerFamily);
    const std::uint32_t effDepth = std::min(grammar.recipe().logicalDepth, options.budget.maxDepth);
    const float maxDepthFloat = static_cast<float>(std::max(1U, effDepth));

    batches.deepestObservedDepth = 0U;
    batches.emittedFibers = static_cast<std::uint32_t>(gpuInstances.size());
    batches.visitedNodes = static_cast<std::uint32_t>(gpuInstances.size() / (2U * m));
    batches.frustumCulledNodes = optionalMarkers ? optionalMarkers->frustumCulledNodes : 0U;
    batches.lodCulledNodes = optionalMarkers ? optionalMarkers->lodCulledNodes : 0U;
    batches.budgetOmittedNodes = optionalMarkers ? optionalMarkers->budgetOmittedNodes : 0U;
    batches.truncatedByBudget = optionalMarkers
        ? optionalMarkers->truncatedByBudget
        : (gpuInstances.size() >= options.budget.maxFibers);
    batches.wovenMode = false;

    batches.plusFibersLod0.reserve(gpuInstances.size() / 2U);
    batches.minusFibersLod0.reserve(gpuInstances.size() / 2U);

    for (const auto& instGpu : gpuInstances) {
        batches.deepestObservedDepth = std::max(batches.deepestObservedDepth, instGpu.depth);
        const float depthFrac = static_cast<float>(instGpu.depth) / maxDepthFloat;
        const float fiberHarmonic = 0.88f + 0.12f * std::cos(
            static_cast<float>(kTau * instGpu.fiberIndex) / static_cast<float>(m));

        const bool isPlus = (instGpu.chirality == 0U);
        const mgfx::math::vec3 baseRgb = isPlus
            ? lerpVec3(options.palette.plusPrimary, options.palette.plusDeep, depthFrac)
            : lerpVec3(options.palette.minusPrimary, options.palette.minusDeep, depthFrac);

        const float emission = options.palette.baseEmission
            + depthFrac * options.palette.depthEmissionBoost;

        const Circle worldCircle{
            .center = {
                static_cast<double>(instGpu.centerRadius[0]),
                static_cast<double>(instGpu.centerRadius[1]),
                static_cast<double>(instGpu.centerRadius[2]),
            },
            .axes = {
                .x = {
                    static_cast<double>(instGpu.axisX[0]),
                    static_cast<double>(instGpu.axisX[1]),
                    static_cast<double>(instGpu.axisX[2]),
                },
                .y = {
                    static_cast<double>(instGpu.axisY[0]),
                    static_cast<double>(instGpu.axisY[1]),
                    static_cast<double>(instGpu.axisY[2]),
                },
                .z = {
                    static_cast<double>(instGpu.axisZ[0]),
                    static_cast<double>(instGpu.axisZ[1]),
                    static_cast<double>(instGpu.axisZ[2]),
                },
            },
            .radius = static_cast<double>(instGpu.centerRadius[3]),
        };
        const double nodeScale = static_cast<double>(instGpu.axisX[3]);
        const mgfx::math::mat4 model =
            options.rootTransform * RecursiveHopfTorusRenderBridge::circleModelMatrix(worldCircle, nodeScale);

        const mgfx::vk::InstanceData inst{
            .model = model,
            .baseColorTint = mgfx::math::vec4(baseRgb * fiberHarmonic, 1.0f),
            .pbrOverride = mgfx::math::vec4(
                options.palette.metallic,
                std::clamp(options.palette.roughness + 0.04f * depthFrac, 0.05f, 0.95f),
                0.0f,
                emission),
            .velocity = mgfx::math::vec4(0.0f),
        };

        const bool useLod0 = (nodeScale >= kLod0ScaleThreshold);
        if (isPlus) {
            if (useLod0) batches.plusFibersLod0.push_back(inst);
            else batches.plusFibersLod1.push_back(inst);
        } else {
            if (useLod0) batches.minusFibersLod0.push_back(inst);
            else batches.minusFibersLod1.push_back(inst);
        }
    }

    if (optionalMarkers != nullptr) {
        for (const auto& cross : optionalMarkers->crossings) {
            const float r = static_cast<float>(cross.worldScale) * options.markerScale;
            if (r < 1.0e-4f) continue;
            mgfx::math::mat4 mMat = glm::translate(
                mgfx::math::mat4(1.0f), RecursiveHopfTorusRenderBridge::mathToEnginePoint(cross.worldPosition));
            mMat = glm::scale(mMat, mgfx::math::vec3(r));
            batches.crossingMarkers.push_back(mgfx::vk::InstanceData{
                .model = options.rootTransform * mMat,
                .baseColorTint = mgfx::math::vec4(options.palette.crossingColor, 1.0f),
                .pbrOverride = mgfx::math::vec4(0.35f, 0.10f, 0.0f, 1.35f),
                .velocity = mgfx::math::vec4(0.0f),
            });
        }

        for (const auto& inter : optionalMarkers->interstitials) {
            const float r = static_cast<float>(inter.worldScale) * options.markerScale * 1.35f;
            if (r < 1.0e-4f) continue;
            mgfx::math::mat4 mMat = glm::translate(
                mgfx::math::mat4(1.0f), RecursiveHopfTorusRenderBridge::mathToEnginePoint(inter.worldPosition));
            mMat = glm::scale(mMat, mgfx::math::vec3(r));
            batches.interstitialMarkers.push_back(mgfx::vk::InstanceData{
                .model = options.rootTransform * mMat,
                .baseColorTint = mgfx::math::vec4(options.palette.interstitialColor, 1.0f),
                .pbrOverride = mgfx::math::vec4(0.55f, 0.12f, 0.0f, 1.15f),
                .velocity = mgfx::math::vec4(0.0f),
            });
        }
    }
}

} // namespace

// -----------------------------------------------------------------------------
// [EDUCATIONAL NOTE: Generation-Keyed Presentation Cache & Prototype Instancing]
// Although the recursive counter-winding Hopf hierarchy has B = 2*m^2 child
// similarity maps per node, every Villarceau circle in R^3 is an exact affine
// image of a single unit torus prototype (or 2 woven prototypes when
// weaveAmplitude > 0). Furthermore, when the grammar state, phase pair, and
// culling budget are unchanged across frames (e.g. while paused or inspecting
// with view culling disabled), GpuCache::cachedBatches skips CPU tree
// traversal completely, and retains vector capacities across animated frames
// so unchanged selections reuse host batch storage. Native prototype
// generations are separately retained through observed graphics completion.
// -----------------------------------------------------------------------------
struct RecursiveHopfTorusRenderBridge::GpuCache {
    RecursiveHopfDeviceInstanceStream deviceStream;
    bool initialized = false;
    mgfx::Engine* engineOwner = nullptr;
    VkDevice deviceOwner = VK_NULL_HANDLE;

    // Immutable publication: draw items borrow Mesh objects, so retaining only
    // their underlying buffers is insufficient. Keep the objects and their
    // buffers together until every referencing frame has completed.
    struct PrototypeSet {
        std::shared_ptr<const mgfx::vk::Mesh> circleLod0;
        std::shared_ptr<const mgfx::vk::Mesh> circleLod1;
        std::shared_ptr<const mgfx::vk::Mesh> wovenPlusLod0;
        std::shared_ptr<const mgfx::vk::Mesh> wovenMinusLod0;
        std::shared_ptr<const mgfx::vk::Mesh> wovenPlusLod1;
        std::shared_ptr<const mgfx::vk::Mesh> wovenMinusLod1;
        std::shared_ptr<const mgfx::vk::Mesh> markerSphere;
        std::shared_ptr<const mgfx::vk::Mesh> cliffordSubstrate;
        float circleFiberRadius = -1.0f;
        float wovenFiberRadius = -1.0f;
        double weaveAmplitude = -1.0;
        std::uint32_t fibersPerFamily = 0U;
        Phases phases{};
        std::uint64_t retainedBytes = 0U;

        void countBytes() {
            retainedBytes = 0U;
            for (const auto* mesh : {circleLod0.get(), circleLod1.get(),
                    wovenPlusLod0.get(), wovenMinusLod0.get(), wovenPlusLod1.get(),
                    wovenMinusLod1.get(), markerSphere.get(), cliffordSubstrate.get()}) {
                if (!mesh) continue;
                if (!mesh->ownsBuffers())
                    throw std::logic_error("Hopf prototypes must own their native buffers");
                retainedBytes += mesh->vertexBufferOwner()->size() + mesh->indexBufferOwner()->size();
            }
        }
    };
    std::shared_ptr<const PrototypeSet> prototypes;

    mgfx::MaterialHandle fiberMaterial{};
    mgfx::MaterialHandle markerMaterial{};
    mgfx::MaterialHandle substrateMaterial{};

    struct BatchCacheKey {
        bool valid = false;
        Recipe recipe{};
        bool expanded = false;
        Phases phases{};
        TraversalBudget budget{};
        RecursiveHopfPalette palette{};
        SiteMarkerMode siteMarkerMode = SiteMarkerMode::Off;
        float markerScale = 0.0f;
        mgfx::math::mat4 rootTransform{1.0f};
    };
    BatchCacheKey batchKey{};
    RecursiveHopfRenderBatches cachedBatches{};
    std::vector<gpu_abi::RecursiveHopfInstanceGpu> expandedSource;

    [[nodiscard]] bool matchesBatchKey(
        const Recipe& recipe,
        bool expanded,
        const Phases& phases,
        const TraversalBudget& budget,
        const RecursiveHopfRenderOptions& options) const noexcept {
        return batchKey.valid
            && batchKey.recipe == recipe && batchKey.expanded == expanded
            && batchKey.phases.plus == phases.plus
            && batchKey.phases.minus == phases.minus
            && exactBudgetEqual(batchKey.budget, budget)
            && exactPaletteEqual(batchKey.palette, options.palette)
            && batchKey.siteMarkerMode == options.siteMarkerMode
            && batchKey.markerScale == options.markerScale
            && batchKey.rootTransform == options.rootTransform;
    }

    void ensureResources(mgfx::Engine& engine, const Grammar& grammar) {
        if (engineOwner && (engineOwner != &engine
                || deviceOwner != engine.context().device().logical()))
            throw std::logic_error("Recreate the Hopf bridge for a replacement Engine/device");
        const auto& recipe = grammar.recipe();
        const auto phases = grammar.phases();
        const float radius = static_cast<float>(recipe.fiberRadius);
        const bool needsCircles = !prototypes || radius != prototypes->circleFiberRadius;
        const bool needsWoven = recipe.weaveAmplitude > 0.0 && (!prototypes
            || !prototypes->wovenPlusLod0 || !prototypes->wovenMinusLod0
            || !prototypes->wovenPlusLod1 || !prototypes->wovenMinusLod1
            || radius != prototypes->wovenFiberRadius
            || recipe.weaveAmplitude != prototypes->weaveAmplitude
            || recipe.fibersPerFamily != prototypes->fibersPerFamily
            || phases.plus != prototypes->phases.plus || phases.minus != prototypes->phases.minus);
        if (initialized && !needsCircles && !needsWoven) return;

        // Construct privately, publish last. An allocation/mesh failure leaves
        // the previous complete selection untouched; it is never rendered under
        // the new recipe as though reconstruction had succeeded.
        auto candidate = prototypes ? std::make_shared<PrototypeSet>(*prototypes)
                                    : std::make_shared<PrototypeSet>();
        if (!candidate->markerSphere)
            candidate->markerSphere = std::make_shared<mgfx::vk::Mesh>(
                mgfx::vk::Mesh::makeSphere(engine.context(), 1.0f, 20U, 12U));
        if (!candidate->cliffordSubstrate)
            candidate->cliffordSubstrate = std::make_shared<mgfx::vk::Mesh>(
                buildCliffordTorusSubstrateMesh(engine.context(), 72U, 48U));
        if (needsCircles) {
            const float mainR = static_cast<float>(kSqrtTwo);
            candidate->circleLod0 = std::make_shared<mgfx::vk::Mesh>(
                mgfx::vk::Mesh::makeTorus(engine.context(), mainR, radius, 96U, 28U));
            candidate->circleLod1 = std::make_shared<mgfx::vk::Mesh>(
                mgfx::vk::Mesh::makeTorus(engine.context(), mainR, radius, 48U, 14U));
            candidate->circleFiberRadius = radius;
        }
        if (needsWoven) {
            candidate->wovenPlusLod0 = std::make_shared<mgfx::vk::Mesh>(
                buildWovenFiberPrototypeMesh(engine.context(), grammar, Chirality::Plus, 128U, 20U));
            candidate->wovenMinusLod0 = std::make_shared<mgfx::vk::Mesh>(
                buildWovenFiberPrototypeMesh(engine.context(), grammar, Chirality::Minus, 128U, 20U));
            candidate->wovenPlusLod1 = std::make_shared<mgfx::vk::Mesh>(
                buildWovenFiberPrototypeMesh(engine.context(), grammar, Chirality::Plus, 56U, 12U));
            candidate->wovenMinusLod1 = std::make_shared<mgfx::vk::Mesh>(
                buildWovenFiberPrototypeMesh(engine.context(), grammar, Chirality::Minus, 56U, 12U));
            candidate->wovenFiberRadius = radius;
            candidate->weaveAmplitude = recipe.weaveAmplitude;
            candidate->fibersPerFamily = recipe.fibersPerFamily;
            candidate->phases = phases;
        }
        candidate->countBytes();
        if (!initialized) {
            std::array<mgfx::MaterialHandle, 3> handles{};
            std::size_t created = 0U;
            try {
                mgfx::pbr::Material fiberMat{};
                fiberMat.baseColorFactor = mgfx::math::vec4(1.0f);
                fiberMat.metallic = 0.85f;
                fiberMat.roughness = 0.18f;
                fiberMat.clearcoatFactor = 0.82f;
                fiberMat.clearcoatRoughnessFactor = 0.06f;
                handles[created] = engine.materials()->createMaterial(fiberMat);
                ++created;

                mgfx::pbr::Material markerMat{};
                markerMat.baseColorFactor = mgfx::math::vec4(1.0f);
                markerMat.metallic = 0.45f;
                markerMat.roughness = 0.12f;
                markerMat.clearcoatFactor = 0.90f;
                markerMat.clearcoatRoughnessFactor = 0.04f;
                handles[created] = engine.materials()->createMaterial(markerMat);
                ++created;

                mgfx::pbr::Material subMat{};
                subMat.baseColorFactor = mgfx::math::vec4(0.35f, 0.58f, 0.95f, 0.16f);
                subMat.metallic = 0.15f;
                subMat.roughness = 0.25f;
                subMat.transmissionFactor = 0.78f;
                subMat.ior = 1.45f;
                handles[created] = engine.materials()->createMaterial(subMat);
                ++created;
            } catch (...) {
                for (std::size_t i = 0U; i < created; ++i)
                    engine.materials()->releaseMaterial(handles[i]);
                throw;
            }
            fiberMaterial = handles[0];
            markerMaterial = handles[1];
            substrateMaterial = handles[2];
            initialized = true;
        }
        prototypes = std::move(candidate);
        engineOwner = &engine;
        deviceOwner = engine.context().device().logical();
    }

    [[nodiscard]] bool retainPrototypes(mgfx::Engine& engine) const {
        return prototypes && engine.retainFramePayloadResources(prototypes, prototypes->retainedBytes);
    }

    bool submitCachedBatches(mgfx::Drawable& drawable, const RecursiveHopfRenderOptions& options) const {
        if (!retainPrototypes(drawable.engine())) return false;
        const auto selected = prototypes;
        const auto& circleLod0 = selected->circleLod0;
        const auto& circleLod1 = selected->circleLod1;
        const auto& wovenPlusLod0 = selected->wovenPlusLod0;
        const auto& wovenMinusLod0 = selected->wovenMinusLod0;
        const auto& wovenPlusLod1 = selected->wovenPlusLod1;
        const auto& wovenMinusLod1 = selected->wovenMinusLod1;
        const auto& markerSphere = selected->markerSphere;
        const auto& cliffordSubstrate = selected->cliffordSubstrate;
        const auto& batches = cachedBatches;

        if (batches.wovenMode && wovenPlusLod0 && wovenMinusLod0
            && wovenPlusLod1 && wovenMinusLod1) {
            if (!batches.plusFibersLod0.empty()) {
                drawable.drawInstanced(*wovenPlusLod0, fiberMaterial, batches.plusFibersLod0);
            }
            if (!batches.plusFibersLod1.empty()) {
                drawable.drawInstanced(*wovenPlusLod1, fiberMaterial, batches.plusFibersLod1);
            }
            if (!batches.minusFibersLod0.empty()) {
                drawable.drawInstanced(*wovenMinusLod0, fiberMaterial, batches.minusFibersLod0);
            }
            if (!batches.minusFibersLod1.empty()) {
                drawable.drawInstanced(*wovenMinusLod1, fiberMaterial, batches.minusFibersLod1);
            }
        } else if (circleLod0 && circleLod1) {
            if (!batches.plusFibersLod0.empty()) {
                drawable.drawInstanced(*circleLod0, fiberMaterial, batches.plusFibersLod0);
            }
            if (!batches.plusFibersLod1.empty()) {
                drawable.drawInstanced(*circleLod1, fiberMaterial, batches.plusFibersLod1);
            }
            if (!batches.minusFibersLod0.empty()) {
                drawable.drawInstanced(*circleLod0, fiberMaterial, batches.minusFibersLod0);
            }
            if (!batches.minusFibersLod1.empty()) {
                drawable.drawInstanced(*circleLod1, fiberMaterial, batches.minusFibersLod1);
            }
        }

        if (markerSphere) {
            if (!batches.crossingMarkers.empty()) {
                drawable.drawInstanced(*markerSphere, markerMaterial, batches.crossingMarkers);
            }
            if (!batches.interstitialMarkers.empty()) {
                drawable.drawInstanced(*markerSphere, markerMaterial, batches.interstitialMarkers);
            }
        }

        if (options.showSubstrateTorus && cliffordSubstrate) {
            drawable.drawTranslucent(
                *cliffordSubstrate,
                substrateMaterial,
                options.rootTransform,
                mgfx::math::vec4(0.35f, 0.62f, 1.0f, 0.16f));
        }
        return true;
    }
};

RecursiveHopfTorusRenderBridge::RecursiveHopfTorusRenderBridge()
    : cache_(std::make_unique<GpuCache>()) {}

RecursiveHopfTorusRenderBridge::~RecursiveHopfTorusRenderBridge() = default;

mgfx::math::vec3 RecursiveHopfTorusRenderBridge::mathToEnginePoint(const Point& p) noexcept {
    return mgfx::math::vec3(
        static_cast<float>(p[0]),
        static_cast<float>(p[2]),
        static_cast<float>(-p[1]));
}

Point RecursiveHopfTorusRenderBridge::engineToMathPoint(const mgfx::math::vec3& p) noexcept {
    return Point{
        static_cast<double>(p.x),
        static_cast<double>(-p.z),
        static_cast<double>(p.y)};
}

ObservationEye RecursiveHopfTorusRenderBridge::makeObservationEye(
    const mgfx::math::vec3& engineEyePos,
    const mgfx::math::vec3& engineTargetPos,
    float fovYRadians,
    float aspectRatio,
    std::uint32_t viewportHeight) noexcept {
    const mgfx::math::vec3 dirEngine = engineTargetPos - engineEyePos;
    const float dirLen = glm::length(dirEngine);
    const mgfx::math::vec3 unitEngine = (dirLen > 1.0e-5f)
        ? (dirEngine / dirLen)
        : mgfx::math::vec3(0.0f, 0.0f, -1.0f);

    const double halfFovY = 0.5 * std::clamp(static_cast<double>(fovYRadians), 0.1, 2.8);
    const double tanHalfY = std::tan(halfFovY);
    const double tanHalfX = tanHalfY * std::max(0.5, static_cast<double>(aspectRatio));
    // Diagonal half-angle of view frustum plus a 10% guard margin
    const double diagHalfAngle = std::min(
        std::numbers::pi_v<double> * 0.49,
        std::atan(std::hypot(tanHalfX, tanHalfY)) * 1.10);

    return ObservationEye{
        .position = engineToMathPoint(engineEyePos),
        .forward = engineToMathPoint(unitEngine),
        .cosHalfFov = std::cos(diagHalfAngle),
        .focalLengthPixels = (0.5 * static_cast<double>(std::max(1U, viewportHeight))) / std::max(1.0e-4, tanHalfY),
    };
}

ObservationEye RecursiveHopfTorusRenderBridge::makeObservationEye(
    const mgfx::FrameView& view) noexcept {
    const auto& P = view.unjitteredProjection;
    // For perspective clip.w=-z, a corner ray has x=(ndc.x+P[2][0])/P[0][0]
    // and y=(ndc.y+P[2][1])/P[1][1], with z=-1. The largest corner angle
    // encloses the full (possibly asymmetric) rectangular view. One raster pixel
    // plus the existing 10% angular guard avoids jitter-driven edge churn.
    const double width = static_cast<double>(view.sceneExtent.width);
    const double height = static_cast<double>(view.sceneExtent.height);
    const double tanHalfX = (1.0 + 2.0 / width + std::abs(static_cast<double>(P[2][0]))) /
                            std::abs(static_cast<double>(P[0][0]));
    const double tanHalfY = (1.0 + 2.0 / height + std::abs(static_cast<double>(P[2][1]))) /
                            std::abs(static_cast<double>(P[1][1]));
    const double coneHalfAngle = std::min(
        std::numbers::pi_v<double>, std::atan(std::hypot(tanHalfX, tanHalfY)) * 1.10);
    const mgfx::math::vec3 forward = glm::normalize(
        glm::transpose(mgfx::math::mat3(view.view)) * mgfx::math::vec3(0.0f, 0.0f, -1.0f));
    return ObservationEye{
        .position = engineToMathPoint(view.eye),
        .forward = engineToMathPoint(forward),
        .cosHalfFov = std::cos(coneHalfAngle),
        .focalLengthPixels = 0.5 * std::max(
            width * std::abs(static_cast<double>(P[0][0])),
            height * std::abs(static_cast<double>(P[1][1]))),
    };
}

mgfx::math::mat4 RecursiveHopfTorusRenderBridge::circleModelMatrix(
    const Circle& worldCircle,
    double nodeScale) noexcept {
    const float s = static_cast<float>(nodeScale);
    const mgfx::math::vec3 e1 = mathToEnginePoint(worldCircle.axes.x);
    const mgfx::math::vec3 e2 = mathToEnginePoint(worldCircle.axes.y);
    const mgfx::math::vec3 n = mathToEnginePoint(worldCircle.axes.z);
    const mgfx::math::vec3 c = mathToEnginePoint(worldCircle.center);

    // Mesh::makeTorus lies in local XZ plane with normal +Y.
    // Sending local (X, Y, Z) -> s * (e2, n, e1) has det = +s^3 > 0 because
    // e1 x e2 = n => e2 x n = e1.
    mgfx::math::mat4 model(1.0f);
    model[0] = mgfx::math::vec4(e2 * s, 0.0f);
    model[1] = mgfx::math::vec4(n * s, 0.0f);
    model[2] = mgfx::math::vec4(e1 * s, 0.0f);
    model[3] = mgfx::math::vec4(c, 1.0f);
    return model;
}

mgfx::math::mat4 RecursiveHopfTorusRenderBridge::nodeModelMatrix(
    const Similarity& nodeTransform) noexcept {
    const float s = static_cast<float>(nodeTransform.scale);
    const mgfx::math::vec3 ax = mathToEnginePoint(nodeTransform.axes.x);
    const mgfx::math::vec3 ay = mathToEnginePoint(nodeTransform.axes.y);
    const mgfx::math::vec3 az = mathToEnginePoint(nodeTransform.axes.z);
    const mgfx::math::vec3 tr = mathToEnginePoint(nodeTransform.translation);

    // Conjugate math-space basis [ax, ay, az] by mathToEngine (X_e=X_m, Y_e=Z_m, Z_e=-Y_m).
    mgfx::math::mat4 model(1.0f);
    model[0] = mgfx::math::vec4(ax * s, 0.0f);
    model[1] = mgfx::math::vec4(az * s, 0.0f);
    model[2] = mgfx::math::vec4(-ay * s, 0.0f);
    model[3] = mgfx::math::vec4(tr, 1.0f);
    return model;
}

RecursiveHopfRenderBatches RecursiveHopfTorusRenderBridge::buildRenderBatches(
    const Grammar& grammar,
    const TraversalResult& traversal,
    const RecursiveHopfRenderOptions& options) {
    RecursiveHopfRenderBatches batches{};
    populateRenderBatchesInto(grammar, traversal, options, batches);
    return batches;
}

RecursiveHopfRenderBatches RecursiveHopfTorusRenderBridge::buildRenderBatchesFromGpuInstances(
    const Grammar& grammar,
    std::span<const gpu_abi::RecursiveHopfInstanceGpu> gpuInstances,
    const RecursiveHopfRenderOptions& options,
    const TraversalResult* optionalMarkers) {
    if (grammar.recipe().weaveAmplitude > 0.0) {
        if (optionalMarkers != nullptr) {
            return buildRenderBatches(grammar, *optionalMarkers, options);
        }
        return buildRenderBatches(grammar, grammar.traverse(options.budget), options);
    }
    RecursiveHopfRenderBatches batches{};
    populateRenderBatchesFromGpuInstancesInto(
        grammar, gpuInstances, options, optionalMarkers, batches);
    return batches;
}

RecursiveHopfRenderBatches RecursiveHopfTorusRenderBridge::draw(
    mgfx::Drawable& drawable,
    const Grammar& grammar,
    const RecursiveHopfRenderOptions& options) {
    cache_->ensureResources(drawable.engine(), grammar);

    TraversalBudget budget = options.budget;
    budget.includeCrossings =
        (options.siteMarkerMode == SiteMarkerMode::CrossingsOnly
         || options.siteMarkerMode == SiteMarkerMode::CrossingsAndInterstitials);
    budget.includeInterstitials =
        (options.siteMarkerMode == SiteMarkerMode::InterstitialsOnly
         || options.siteMarkerMode == SiteMarkerMode::CrossingsAndInterstitials);

    const auto& recipe = grammar.recipe();
    const auto phases = grammar.phases();
    if (!cache_->matchesBatchKey(recipe, false, phases, budget, options)) {
        cache_->batchKey.valid = false;
        const TraversalResult traversal = grammar.traverse(budget);
        populateRenderBatchesInto(grammar, traversal, options, cache_->cachedBatches);
        cache_->batchKey = GpuCache::BatchCacheKey{
            .valid = true,
            .recipe = recipe,
            .phases = phases,
            .budget = budget,
            .palette = options.palette,
            .siteMarkerMode = options.siteMarkerMode,
            .markerScale = options.markerScale,
            .rootTransform = options.rootTransform,
        };
    }

    cache_->cachedBatches.submissionDeferred = !cache_->submitCachedBatches(drawable, options);
    return cache_->cachedBatches;
}

RecursiveHopfRenderBatches RecursiveHopfTorusRenderBridge::drawExpanded(
    mgfx::Drawable& drawable,
    const Grammar& grammar,
    std::span<const gpu_abi::RecursiveHopfInstanceGpu> gpuInstances,
    const RecursiveHopfRenderOptions& options) {
    if (gpuInstances.empty()
        || grammar.recipe().weaveAmplitude > 0.0
        || options.siteMarkerMode != SiteMarkerMode::Off
        || options.budget.enableViewCulling) {
        return draw(drawable, grammar, options);
    }

    cache_->ensureResources(drawable.engine(), grammar);

    const auto& recipe = grammar.recipe();
    const auto phases = grammar.phases();
    const bool sameExpandedSource = cache_->expandedSource.size() == gpuInstances.size()
        && std::memcmp(cache_->expandedSource.data(), gpuInstances.data(), gpuInstances.size_bytes()) == 0;
    if (!cache_->matchesBatchKey(recipe, true, phases, options.budget, options) || !sameExpandedSource) {
        cache_->batchKey.valid = false;
        cache_->expandedSource.assign(gpuInstances.begin(), gpuInstances.end());
        populateRenderBatchesFromGpuInstancesInto(
            grammar, gpuInstances, options, nullptr, cache_->cachedBatches);
        cache_->batchKey = GpuCache::BatchCacheKey{
            .valid = true,
            .recipe = recipe,
            .expanded = true,
            .phases = phases,
            .budget = options.budget,
            .palette = options.palette,
            .siteMarkerMode = options.siteMarkerMode,
            .markerScale = options.markerScale,
            .rootTransform = options.rootTransform,
        };
    }

    cache_->cachedBatches.submissionDeferred = !cache_->submitCachedBatches(drawable, options);
    return cache_->cachedBatches;
}

RecursiveHopfRenderBatches RecursiveHopfTorusRenderBridge::drawDelivery(
    mgfx::Drawable& drawable, const RecursiveHopfDelivery& delivery,
    const RecursiveHopfRenderOptions& options,
    std::shared_ptr<const RecursiveHopfExternalGrammarView> externalGrammar) {
    cache_->ensureResources(drawable.engine(), delivery.grammar);
    RecursiveHopfRenderBatches statistics{};
    if (!cache_->retainPrototypes(drawable.engine())) {
        statistics.submissionDeferred = true;
        return statistics;
    }
    const auto selected = cache_->prototypes;
    if (!cache_->deviceStream.tryDraw(drawable, delivery, options,
            selected->circleLod0, selected->circleLod1, cache_->fiberMaterial, statistics, std::move(externalGrammar)))
        return draw(drawable, delivery.grammar, options);
    if (options.showSubstrateTorus && selected->cliffordSubstrate)
        drawable.drawTranslucent(*selected->cliffordSubstrate, cache_->substrateMaterial,
            options.rootTransform, mgfx::math::vec4(0.35f, 0.62f, 1.0f, 0.16f));
    return statistics;
}

bool RecursiveHopfTorusRenderBridge::acceptsDeviceLocalDelivery(
    const RecursiveHopfDelivery& delivery, const RecursiveHopfRenderOptions& options) noexcept {
    return RecursiveHopfDeviceInstanceStream::accepts(delivery, options);
}

} // namespace epi_sculpt::recursive_hopf
