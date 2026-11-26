#pragma once

#include "SculptureCausalExecutionGraph.h"

#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

namespace epi_sculpt::inspection {

// Shared inspection facts, independent of HUD text and JSON formatting.
// All returned pointers borrow the immutable graph; do not outlive it.
// A view over one immutable finalized graph. Edges distinguish work scheduled
// by this frame from an older accepted publication selected by this frame.
// This does not request work, advance clocks, or infer stage execution.
struct ReceiptTrace {
    const SculptureCausalFrameIntent* intent = nullptr;
    const SculptureProductionReceipt* scheduled = nullptr;
    const SculptureProductionReceipt* selectedReceipt = nullptr;
    const SculpturePresentationReceipt* presentation = nullptr;
    SculpturePresentationContribution selected{};
};

// One short-lived read view per inspection pass. Resolve the common frame nodes
// and selected family once, then query independent route edges against that same
// immutable snapshot. This is not a retained cache or another runtime authority.
// The graph must stay alive and unchanged; do not carry the view across frames.
class SelectionContextView final {
public:
    explicit SelectionContextView(const SculptureCausalExecutionSnapshot& graph) noexcept
        : graph_(graph) {
        for (const auto& node : graph_.nodes()) {
            if (node.key.kind == SculptureCausalNodeKind::FrameIntent &&
                node.key.sequence == graph_.frameSequence()) intent_ = &node.frameIntent;
            if (node.key.kind == SculptureCausalNodeKind::Presentation &&
                node.key.sequence == graph_.frameSequence()) presentation_ = &node.presentation;
        }
        if (presentation_) selected_ = presentation_->selectedProductions();
    }
    SelectionContextView(SculptureCausalExecutionSnapshot&&) = delete;
    SelectionContextView(const SculptureCausalExecutionSnapshot&&) = delete;

    // The span borrows this named view; never expose its array from a temporary.
    // ReceiptTrace values below borrow graph_ instead and remain safe to return.
    [[nodiscard]] std::span<const SculpturePresentationContribution> selectedProductions() const & noexcept {
        return selected_;
    }
    [[nodiscard]] std::span<const SculpturePresentationContribution> selectedProductions() const && noexcept = delete;

    [[nodiscard]] ReceiptTrace trace() const noexcept { return trace(graph_.route()); }

    // The common intent supplies chronology, not auxiliary mathematical controls.
    [[nodiscard]] ReceiptTrace trace(SculptureProductionRoute route) const noexcept {
        ReceiptTrace result{};
        result.intent = intent_;
        result.presentation = presentation_;
        if (!result.intent || !result.presentation) return result;
        for (const auto& contribution : selected_) {
            if (contribution.selected() && contribution.route == route) {
                result.selected = contribution;
                break;
            }
        }
        for (const auto& edge : graph_.edges()) {
            if (edge.kind == SculptureCausalEdgeKind::SchedulesProduction &&
                edge.from.kind == SculptureCausalNodeKind::FrameIntent &&
                edge.from.sequence == graph_.frameSequence() && edge.to.route == route) {
                const auto* node = graph_.find(edge.to);
                if (node && node->key.kind == SculptureCausalNodeKind::ProductionExecution &&
                    (!result.scheduled || node->production.scheduleSequence > result.scheduled->scheduleSequence))
                    result.scheduled = &node->production;
            }
            if (result.selected.productionReceiptSequence != 0 &&
                edge.kind == SculptureCausalEdgeKind::SelectedForPresentation &&
                edge.to.kind == SculptureCausalNodeKind::Presentation &&
                edge.to.sequence == graph_.frameSequence() && edge.from.route == result.selected.route &&
                edge.from.sequence == result.selected.productionReceiptSequence) {
                const auto* node = graph_.find(edge.from);
                if (node && node->production.published() &&
                    node->production.publication.generation == result.selected.generation &&
                    (result.selected.route != SculptureProductionRoute::CanonicalFeedback ||
                     node->production.publication.resetEpoch == result.selected.resetEpoch))
                    result.selectedReceipt = &node->production;
            }
        }
        return result;
    }

private:
    const SculptureCausalExecutionSnapshot& graph_;
    const SculptureCausalFrameIntent* intent_ = nullptr;
    const SculpturePresentationReceipt* presentation_ = nullptr;
    std::array<SculpturePresentationContribution, SculpturePresentationReceipt::kContributionCapacity> selected_{};
};

// Convenient single-route queries use the same join implementation. Returned
// traces do not borrow the temporary view, only the caller's immutable graph.
[[nodiscard]] inline ReceiptTrace receiptTrace(const SculptureCausalExecutionSnapshot& graph,
                                              SculptureProductionRoute route) noexcept {
    return SelectionContextView(graph).trace(route);
}

[[nodiscard]] inline ReceiptTrace receiptTrace(const SculptureCausalExecutionSnapshot& graph) noexcept {
    return SelectionContextView(graph).trace();
}

enum class SelectionExplanationKind : std::uint8_t {
    Incomplete,
    NoSelection,
    Unjoined,
    UnverifiedChronology,
    CurrentFrame,
    EarlierSource,
    EarlierSourceWithoutProduction,
    EarlierSourceWithDeferral,
    EarlierSourceWithUnpublishedWork,
    EarlierSourceWithOtherPublication,
};

[[nodiscard]] constexpr std::string_view selectionExplanationName(SelectionExplanationKind kind) noexcept {
    switch (kind) {
    case SelectionExplanationKind::Incomplete: return "incomplete";
    case SelectionExplanationKind::NoSelection: return "no-selection";
    case SelectionExplanationKind::Unjoined: return "unjoined-selection";
    case SelectionExplanationKind::UnverifiedChronology: return "unverified-chronology";
    case SelectionExplanationKind::CurrentFrame: return "current-frame-source";
    case SelectionExplanationKind::EarlierSource: return "earlier-frame-source";
    case SelectionExplanationKind::EarlierSourceWithoutProduction: return "earlier-source-without-production";
    case SelectionExplanationKind::EarlierSourceWithDeferral: return "earlier-source-with-deferral";
    case SelectionExplanationKind::EarlierSourceWithUnpublishedWork: return "earlier-source-with-unpublished-work";
    case SelectionExplanationKind::EarlierSourceWithOtherPublication: return "earlier-source-with-other-publication";
    }
    return "incomplete";
}

struct SelectionExplanation {
    SelectionExplanationKind kind = SelectionExplanationKind::Incomplete;
    std::optional<std::uint64_t> sourceFrameSequence;
    std::optional<std::uint64_t> sourceFrameGap;
    SculptureProductionDeferralReason deferralReason = SculptureProductionDeferralReason::None;
};

// Explains selected provenance and concurrent production facts, not a scheduler
// decision or proof of physical display. This is the production receipt's frame,
// not its first publication/selection frame: async delivery may arrive later.
// Frame distance is never simulation lag or a reason to advance a paused field.
[[nodiscard]] inline SelectionExplanation explainSelection(const ReceiptTrace& trace) noexcept {
    SelectionExplanation result{};
    if (!trace.intent || !trace.presentation) return result;
    if (!trace.selected.selected()) {
        result.kind = SelectionExplanationKind::NoSelection;
        return result;
    }
    if (!trace.selectedReceipt) {
        result.kind = SelectionExplanationKind::Unjoined;
        return result;
    }
    const auto source = trace.selectedReceipt->frameSequence;
    if (source != 0U) result.sourceFrameSequence = source;
    if (source == 0U || source > trace.intent->frameSequence) {
        result.kind = SelectionExplanationKind::UnverifiedChronology;
        return result;
    }
    result.sourceFrameGap = trace.intent->frameSequence - source;
    if (*result.sourceFrameGap == 0U) {
        result.kind = SelectionExplanationKind::CurrentFrame;
    } else if (!trace.scheduled) {
        result.kind = SelectionExplanationKind::EarlierSourceWithoutProduction;
    } else if (trace.scheduled->frameSequence != trace.intent->frameSequence) {
        result.kind = SelectionExplanationKind::EarlierSource;
    } else if (trace.scheduled->deferred()) {
        result.kind = SelectionExplanationKind::EarlierSourceWithDeferral;
        result.deferralReason = trace.scheduled->deferralReason;
    } else if (trace.scheduled->accepted() && !trace.scheduled->published()) {
        result.kind = SelectionExplanationKind::EarlierSourceWithUnpublishedWork;
    } else if (trace.scheduled->published() &&
               trace.scheduled->scheduleSequence != trace.selectedReceipt->scheduleSequence) {
        result.kind = SelectionExplanationKind::EarlierSourceWithOtherPublication;
    } else {
        result.kind = SelectionExplanationKind::EarlierSource;
    }
    return result;
}

} // namespace epi_sculpt::inspection
