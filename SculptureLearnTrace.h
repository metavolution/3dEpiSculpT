#pragma once

#include "SculptureSelectionContext.h"
#include "SculptureLearnMode.h"

#include <cstdio>
#include <cstddef>
#include <string_view>

namespace epi_sculpt::learn_mode {

[[nodiscard]] constexpr std::string_view selectionExplanationText(inspection::SelectionExplanationKind kind) noexcept {
    switch (kind) {
    case inspection::SelectionExplanationKind::Incomplete: return "receipt incomplete";
    case inspection::SelectionExplanationKind::NoSelection: return "no active-route publication selected";
    case inspection::SelectionExplanationKind::Unjoined: return "selected source receipt unavailable";
    case inspection::SelectionExplanationKind::UnverifiedChronology: return "source frame chronology unverified";
    case inspection::SelectionExplanationKind::CurrentFrame: return "source produced in this frame";
    case inspection::SelectionExplanationKind::EarlierSource: return "earlier-frame source selected";
    case inspection::SelectionExplanationKind::EarlierSourceWithoutProduction: return "earlier-frame source selected; no production scheduled";
    case inspection::SelectionExplanationKind::EarlierSourceWithDeferral: return "earlier-frame source selected; production deferred";
    case inspection::SelectionExplanationKind::EarlierSourceWithUnpublishedWork: return "earlier-frame source selected; accepted work not published";
    case inspection::SelectionExplanationKind::EarlierSourceWithOtherPublication: return "earlier-frame source selected; another publication exists";
    }
    return "receipt incomplete";
}

// Fits at the head of the bounded application HUD. The numerical controls and
// stage mask describe the frozen frame intent, not per-stage execution evidence.
inline void formatReceiptTrace(char* out, std::size_t size,
                               const SculptureCausalExecutionSnapshot* graph) noexcept {
    if (!out || size == 0) return;
    if (!graph) {
        std::snprintf(out, size, "LEARN last finalized frame: unknown (no receipt yet)\n");
        return;
    }
    const inspection::SelectionContextView selectionView(*graph);
    const auto trace = selectionView.trace();
    if (!trace.intent || !trace.presentation) {
        std::snprintf(out, size, "LEARN last finalized frame: incomplete receipt\n");
        return;
    }
    const auto& intent = *trace.intent;
    const auto& presentation = *trace.presentation;
    const auto route = name(intent.route);
    const auto reduction = name(intent.reduction);
    const auto seed = name(intent.seed);
    const auto color = name(intent.color);
    const auto* scheduled = trace.scheduled;
    const auto attempt = scheduled ? name(scheduled->attempt) : std::string_view{"not-scheduled"};
    const auto publication = scheduled ? name(scheduled->publicationOutcome) : std::string_view{"unknown"};
    const auto reason = scheduled && scheduled->deferred() ? name(scheduled->deferralReason) : std::string_view{"none"};
    const char* native = !presentation.settled ? "unknown" : presentation.presentationConfirmed() ? "presented" :
        presentation.submitted() ? "submitted-unconfirmed" : "not-submitted";
    const auto selectedStatus = !trace.selected.selected() ? "unknown" : trace.selectedReceipt ? "joined" : "unjoined";
    const auto explanation = inspection::explainSelection(trace);
    const auto explanationText = selectionExplanationText(explanation.kind);
    char frameGap[32] = "unknown";
    if (explanation.sourceFrameGap)
        std::snprintf(frameGap, sizeof(frameGap), "%llu",
            static_cast<unsigned long long>(*explanation.sourceFrameGap));
    const int written = std::snprintf(out, size,
        "LEARN last finalized frame %llu | preset %s%s | route %.*s\n"
        "  requested math transform:%c reduce:%c sample:%c color:%c seed:%c blend:%c (execution unverified)\n"
        "  control projection reduce:%.*s seed:%.*s color:%.*s iterations:%u W:%.2f\n"
        "  scheduled:%.*s reason:%.*s accepted-gen:%llu publication:%.*s\n"
        "  selected:%s gen:%llu receipt:%llu source-frame:%llu | native:%s device:%llu surface:%llu (0=unknown)\n"
        "  %.*s | source gap:%s app frames (not simulation lag)\n",
        static_cast<unsigned long long>(intent.frameSequence),
        intent.presetId[0] ? intent.presetId.data() : "unknown", intent.presetIdTruncated ? "..." : "",
        static_cast<int>(route.size()), route.data(),
        enabled(intent.requestedStageMask, Stage::Transform) ? 'Y' : '-',
        enabled(intent.requestedStageMask, Stage::Reduce) ? 'Y' : '-',
        enabled(intent.requestedStageMask, Stage::FeedbackSample) ? 'Y' : '-',
        enabled(intent.requestedStageMask, Stage::Color) ? 'Y' : '-',
        enabled(intent.requestedStageMask, Stage::Seed) ? 'Y' : '-',
        enabled(intent.requestedStageMask, Stage::Blend) ? 'Y' : '-',
        static_cast<int>(reduction.size()), reduction.data(), static_cast<int>(seed.size()), seed.data(),
        static_cast<int>(color.size()), color.data(), intent.feedbackIterations, static_cast<double>(intent.latentW),
        static_cast<int>(attempt.size()), attempt.data(), static_cast<int>(reason.size()), reason.data(),
        static_cast<unsigned long long>(scheduled && scheduled->accepted() ? scheduled->committedGeneration : 0U),
        static_cast<int>(publication.size()), publication.data(), selectedStatus,
        static_cast<unsigned long long>(trace.selected.generation),
        static_cast<unsigned long long>(trace.selected.productionReceiptSequence),
        static_cast<unsigned long long>(trace.selectedReceipt ? trace.selectedReceipt->frameSequence : 0U), native,
        static_cast<unsigned long long>(presentation.deviceGeneration),
        static_cast<unsigned long long>(presentation.surfaceGeneration),
        static_cast<int>(explanationText.size()), explanationText.data(), frameGap);
    if (written < 0 || static_cast<std::size_t>(written) >= size) return;
    std::size_t used = static_cast<std::size_t>(written);
    for (const auto& contribution : selectionView.selectedProductions()) {
        if (!contribution.selected() || contribution.route == intent.route) continue;
        const auto context = inspection::explainSelection(selectionView.trace(contribution.route));
        const auto routeName = name(contribution.route);
        const auto contextText = selectionExplanationText(context.kind);
        const auto deferral = name(context.deferralReason);
        char gap[32] = "unknown";
        if (context.sourceFrameGap)
            std::snprintf(gap, sizeof(gap), "%llu", static_cast<unsigned long long>(*context.sourceFrameGap));
        const int appended = std::snprintf(out + used, size - used,
            "  contribution %.*s gen:%llu receipt:%llu | %.*s | source gap:%s app frames | deferral:%.*s\n",
            static_cast<int>(routeName.size()), routeName.data(),
            static_cast<unsigned long long>(contribution.generation),
            static_cast<unsigned long long>(contribution.productionReceiptSequence),
            static_cast<int>(contextText.size()), contextText.data(), gap,
            static_cast<int>(deferral.size()), deferral.data());
        if (appended < 0 || static_cast<std::size_t>(appended) >= size - used) return;
        used += static_cast<std::size_t>(appended);
    }
}
} // namespace epi_sculpt::learn_mode
