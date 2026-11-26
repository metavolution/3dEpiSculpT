#pragma once

#include "SculptureFramePlan.h"
#include "SculpturePresentationReceipt.h"
#include "SculptureProductionReceipt.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace epi_sculpt {

// Derived, immutable causal read model for one completed frame evaluation.
// A committed plan can be observed even when rendering aborts; the presentation
// receipt's settled flag remains false when no native outcome was returned.
//
// The graph intentionally contains only copied scalar/value facts. It cannot
// schedule work, retry a producer, mutate semantic state, or grant access to a
// GPU/native resource. Runtime contracts remain authoritative; this projection
// exists so inspection/capture/dashboard code never has to reconstruct causality
// by reading mutable application internals.
enum class SculptureCausalNodeKind : std::uint8_t {
    FrameIntent,
    ProductionExecution,
    Presentation,
};

enum class SculptureCausalEdgeKind : std::uint8_t {
    SchedulesProduction,
    EvaluatesPresentation,
    SelectedForPresentation,
};

[[nodiscard]] constexpr std::string_view name(
    SculptureCausalNodeKind kind) noexcept {
    switch (kind) {
    case SculptureCausalNodeKind::FrameIntent: return "frame-intent";
    case SculptureCausalNodeKind::ProductionExecution: return "production-execution";
    case SculptureCausalNodeKind::Presentation: return "presentation";
    }
    return "unknown";
}

[[nodiscard]] constexpr std::string_view name(
    SculptureCausalEdgeKind kind) noexcept {
    switch (kind) {
    case SculptureCausalEdgeKind::SchedulesProduction: return "schedules-production";
    case SculptureCausalEdgeKind::EvaluatesPresentation: return "evaluates-presentation";
    case SculptureCausalEdgeKind::SelectedForPresentation: return "selected-for-presentation";
    }
    return "unknown";
}

struct SculptureCausalNodeKey final {
    SculptureCausalNodeKind kind = SculptureCausalNodeKind::FrameIntent;
    SculptureProductionRoute route = SculptureProductionRoute::CompactInstances;
    std::uint64_t sequence = 0U;

    [[nodiscard]] constexpr bool operator==(const SculptureCausalNodeKey&) const noexcept = default;
};

struct SculptureCausalFrameIntent final {
    std::uint64_t frameSequence = 0U;
    SculptureProductionRoute route = SculptureProductionRoute::CompactInstances;
    std::size_t presetIndex = 0U;
    SculpturePresentationOwnership presentationOwnership =
        SculpturePresentationOwnership::CompactInstances;
    std::uint64_t compactPresentationLeaseEpoch = 0U;
    float simulationTime = 0.0f;
    float effectiveWorldUpdateHz = 0.0f;
    bool dedicatedWorldFallback = false;
    bool vrEnabled = false;
    bool temporalActive = false;
    // Bounded copied authoring identity and control projection, recorded with
    // this frame. Neither attests which mathematical stages actually ran.
    std::array<char, 96> presetId{};
    bool presetIdTruncated = false;
    bool presetIdSanitized = false;
    std::uint32_t requestedStageMask = 0U;
    ReductionMode reduction = ReductionMode::Torus;
    SeedType seed = SeedType::Border;
    ColorEffectType color = ColorEffectType::StandardHSV;
    std::uint32_t feedbackIterations = 0U;
    float latentW = 0.0f;

    [[nodiscard]] constexpr bool operator==(
        const SculptureCausalFrameIntent&) const noexcept = default;
};

struct SculptureCausalNode final {
    SculptureCausalNodeKey key{};
    SculptureCausalFrameIntent frameIntent{};
    SculptureProductionReceipt production{};
    SculpturePresentationReceipt presentation{};

    [[nodiscard]] constexpr bool operator==(
        const SculptureCausalNode&) const noexcept = default;
};

struct SculptureCausalEdge final {
    SculptureCausalNodeKey from{};
    SculptureCausalNodeKey to{};
    SculptureCausalEdgeKind kind = SculptureCausalEdgeKind::SchedulesProduction;

    [[nodiscard]] constexpr bool operator==(
        const SculptureCausalEdge&) const noexcept = default;
};

class SculptureCausalExecutionSnapshot final {
public:
    static constexpr std::size_t kMaxNodes = 16U;
    static constexpr std::size_t kMaxEdges = 16U;

    [[nodiscard]] std::uint64_t revision() const noexcept { return revision_; }
    [[nodiscard]] std::uint64_t frameSequence() const noexcept { return frameSequence_; }
    [[nodiscard]] SculptureProductionRoute route() const noexcept { return route_; }

    [[nodiscard]] std::span<const SculptureCausalNode> nodes() const noexcept {
        return {nodes_.data(), nodeCount_};
    }

    [[nodiscard]] std::span<const SculptureCausalEdge> edges() const noexcept {
        return {edges_.data(), edgeCount_};
    }

    [[nodiscard]] const SculptureCausalNode* find(
        const SculptureCausalNodeKey& key) const noexcept {
        for (std::size_t i = 0U; i < nodeCount_; ++i) {
            if (nodes_[i].key == key) return &nodes_[i];
        }
        return nullptr;
    }

private:
    friend class SculptureCausalExecutionGraphProjector;

    SculptureCausalNode* addNode(const SculptureCausalNode& node) noexcept {
        if (node.key.sequence == 0U) return nullptr;
        if (SculptureCausalNode* existing = findMutable(node.key)) return existing;
        if (nodeCount_ >= nodes_.size()) return nullptr;
        nodes_[nodeCount_] = node;
        return &nodes_[nodeCount_++];
    }

    void addEdge(
        const SculptureCausalNodeKey& from,
        const SculptureCausalNodeKey& to,
        SculptureCausalEdgeKind kind) noexcept {
        if (from.sequence == 0U || to.sequence == 0U || edgeCount_ >= edges_.size()) return;
        for (std::size_t i = 0U; i < edgeCount_; ++i) {
            if (edges_[i].from == from && edges_[i].to == to && edges_[i].kind == kind) {
                return;
            }
        }
        edges_[edgeCount_++] = {.from = from, .to = to, .kind = kind};
    }

    [[nodiscard]] SculptureCausalNode* findMutable(
        const SculptureCausalNodeKey& key) noexcept {
        for (std::size_t i = 0U; i < nodeCount_; ++i) {
            if (nodes_[i].key == key) return &nodes_[i];
        }
        return nullptr;
    }

    std::uint64_t revision_ = 0U;
    std::uint64_t frameSequence_ = 0U;
    SculptureProductionRoute route_ = SculptureProductionRoute::CompactInstances;
    std::array<SculptureCausalNode, kMaxNodes> nodes_{};
    std::array<SculptureCausalEdge, kMaxEdges> edges_{};
    std::size_t nodeCount_ = 0U;
    std::size_t edgeCount_ = 0U;
};

class SculptureCausalExecutionGraphProjector final {
public:
    [[nodiscard]] static SculptureCausalExecutionSnapshot project(
        std::uint64_t revision,
        const SculptureFramePlan& framePlan,
        std::span<const SculptureProductionReceipt* const> currentProductions,
        const SculpturePresentationReceipt& presentation,
        std::span<const SculptureProductionReceipt* const> selectedProductions,
        std::string_view presetId = {}) noexcept {
        SculptureCausalExecutionSnapshot snapshot{};
        snapshot.revision_ = revision;
        snapshot.frameSequence_ = framePlan.frameSequence();
        snapshot.route_ = framePlan.production().route;

        // Copy before publishing the read model; catalog reload and captured
        // source replacement must not alter a previously settled frame.
        std::array<char, 96> copiedPreset{};
        const auto idLength = std::min(presetId.size(), copiedPreset.size() - 1U);
        bool idSanitized = false;
        for (std::size_t i = 0; i < idLength; ++i) {
            const auto c = static_cast<unsigned char>(presetId[i]);
            const bool printable = c >= 32U && c < 127U;
            copiedPreset[i] = printable ? static_cast<char>(c) : '?';
            idSanitized = idSanitized || !printable;
        }
        const auto& params = framePlan.params();
        const SculptureCausalNodeKey frameKey{
            .kind = SculptureCausalNodeKind::FrameIntent,
            .route = framePlan.production().route,
            .sequence = framePlan.frameSequence(),
        };
        snapshot.addNode({
            .key = frameKey,
            .frameIntent = {
                .frameSequence = framePlan.frameSequence(),
                .route = framePlan.production().route,
                .presetIndex = framePlan.presetIndex(),
                .presentationOwnership = framePlan.presentationOwnership(),
                .compactPresentationLeaseEpoch = framePlan.compactPresentationLeaseEpoch(),
                .simulationTime = framePlan.worldState().time,
                .effectiveWorldUpdateHz = framePlan.effectiveWorldUpdateHz(),
                .dedicatedWorldFallback = framePlan.production().dedicatedWorldFallback,
                .vrEnabled = framePlan.presentation().vrEnabled,
                .temporalActive = framePlan.presentation().temporalActive,
                .presetId = copiedPreset,
                .presetIdTruncated = presetId.size() > idLength,
                .presetIdSanitized = idSanitized,
                .requestedStageMask = params.stageMask,
                .reduction = params.reduction,
                .seed = params.seedType,
                .color = params.colorEffect,
                .feedbackIterations = params.feedbackIterations,
                .latentW = params.latentW,
            },
        });

        const SculptureCausalNodeKey presentationKey{
            .kind = SculptureCausalNodeKind::Presentation,
            .route = presentation.route,
            .sequence = presentation.frameSequence,
        };
        snapshot.addNode({
            .key = presentationKey,
            .presentation = presentation,
        });
        snapshot.addEdge(
            frameKey, presentationKey, SculptureCausalEdgeKind::EvaluatesPresentation);

        for (const SculptureProductionReceipt* currentProduction : currentProductions) {
            if (currentProduction == nullptr) continue;
            const SculptureCausalNodeKey productionKey = addProductionNode(
                snapshot, *currentProduction);
            snapshot.addEdge(
                frameKey, productionKey, SculptureCausalEdgeKind::SchedulesProduction);
        }

        for (const SculptureProductionReceipt* selectedProduction : selectedProductions) {
            linkSelectedProduction(snapshot, selectedProduction, presentationKey);
        }

        return snapshot;
    }

private:
    [[nodiscard]] static SculptureCausalNodeKey addProductionNode(
        SculptureCausalExecutionSnapshot& snapshot,
        const SculptureProductionReceipt& receipt) noexcept {
        const SculptureCausalNodeKey key{
            .kind = SculptureCausalNodeKind::ProductionExecution,
            .route = receipt.route,
            .sequence = receipt.scheduleSequence,
        };
        snapshot.addNode({
            .key = key,
            .production = receipt,
        });
        return key;
    }

    static void linkSelectedProduction(
        SculptureCausalExecutionSnapshot& snapshot,
        const SculptureProductionReceipt* receipt,
        const SculptureCausalNodeKey& presentationKey) noexcept {
        if (receipt == nullptr || receipt->scheduleSequence == 0U) return;
        const SculptureCausalNodeKey productionKey = addProductionNode(snapshot, *receipt);
        snapshot.addEdge(
            productionKey, presentationKey,
            SculptureCausalEdgeKind::SelectedForPresentation);
    }
};

} // namespace epi_sculpt
