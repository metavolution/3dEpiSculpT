#pragma once

#include "SculptureCausalExecutionHistory.h"
#include "SculptureFilePublication.h"
#include "SculptureInspectionJson.h"
#include "SculptureSelectionContext.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <locale>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>

namespace epi_sculpt {

enum class SculptureCausalJsonVersion { V1, V2 };

// Protocol-neutral JSON projection for capture/debug/dashboard adapters. This
// serializer consumes only copied causal read-model values and has no route back
// into producers, scheduling, or native GPU ownership.
class SculptureCausalExecutionJson final {
public:
    [[nodiscard]] static std::string serialize(
        const SculptureCausalExecutionSnapshot& snapshot,
        SculptureCausalJsonVersion version = SculptureCausalJsonVersion::V2) {
        std::ostringstream out;
        prepare(out);
        out << "{\n"
            << "  \"schema\": \"" << schema(version) << "\",\n"
            << "  \"kind\": \"snapshot\",\n"
            << "  \"revision\": " << snapshot.revision() << ",\n"
            << "  \"frame_sequence\": " << snapshot.frameSequence() << ",\n"
            << "  \"route\": \"" << name(snapshot.route()) << "\",\n"
            << "  \"nodes\": [\n";

        bool first = true;
        for (const SculptureCausalNode& node : snapshot.nodes()) {
            if (!first) out << ",\n";
            first = false;
            writeNode(out, node, "    ", version);
        }
        out << "\n  ],\n  \"edges\": [\n";
        first = true;
        for (const SculptureCausalEdge& edge : snapshot.edges()) {
            if (!first) out << ",\n";
            first = false;
            writeEdge(out, edge, "    ");
        }
        out << "\n  ]";
        if (version == SculptureCausalJsonVersion::V2) {
            const inspection::SelectionContextView selectionView(snapshot);
            const auto active = selectionView.trace();
            const auto activeExplanation = inspection::explainSelection(active);
            out << ",\n  \"active_route_selection\":";
            writeSelectionExplanation(out, activeExplanation);
            out << ",\n  \"selected_contribution_contexts\":[";
            bool firstContribution = true;
            if (active.presentation) {
                for (const auto& contribution : selectionView.selectedProductions()) {
                    if (!contribution.selected()) continue;
                    if (!firstContribution) out << ',';
                    firstContribution = false;
                    out << "{\"route\":\"" << name(contribution.route)
                        << "\",\"generation\":" << contribution.generation
                        << ",\"reset_epoch\":" << contribution.resetEpoch
                        << ",\"production_receipt_sequence\":" << contribution.productionReceiptSequence
                        << ",\"context\":";
                    const auto explanation = contribution.route == snapshot.route()
                        ? activeExplanation
                        : inspection::explainSelection(selectionView.trace(contribution.route));
                    writeSelectionExplanation(out, explanation);
                    out << '}';
                }
            }
            out << ']';
        }
        out << "\n}\n";
        return out.str();
    }

    [[nodiscard]] static std::string serialize(
        const SculptureCausalExecutionDelta& delta,
        SculptureCausalJsonVersion version = SculptureCausalJsonVersion::V2) {
        std::ostringstream out;
        prepare(out);
        out << "{\n"
            << "  \"schema\": \"" << schema(version) << "\",\n"
            << "  \"kind\": \"delta\",\n"
            << "  \"from_revision\": " << delta.fromRevision << ",\n"
            << "  \"to_revision\": " << delta.toRevision << ",\n"
            << "  \"frame_sequence\": " << delta.frameSequence << ",\n"
            << "  \"upsert_nodes\": [\n";
        for (std::size_t i = 0U; i < delta.upsertedNodeCount; ++i) {
            if (i != 0U) out << ",\n";
            writeNode(out, delta.upsertedNodes[i], "    ", version);
        }
        out << "\n  ],\n  \"remove_nodes\": [";
        for (std::size_t i = 0U; i < delta.removedNodeCount; ++i) {
            if (i != 0U) out << ", ";
            writeKey(out, delta.removedNodes[i]);
        }
        out << "],\n  \"add_edges\": [\n";
        for (std::size_t i = 0U; i < delta.addedEdgeCount; ++i) {
            if (i != 0U) out << ",\n";
            writeEdge(out, delta.addedEdges[i], "    ");
        }
        out << "\n  ],\n  \"remove_edges\": [\n";
        for (std::size_t i = 0U; i < delta.removedEdgeCount; ++i) {
            if (i != 0U) out << ",\n";
            writeEdge(out, delta.removedEdges[i], "    ");
        }
        out << "\n  ]\n}\n";
        return out.str();
    }

    [[nodiscard]] static bool writeSnapshotFile(
        const std::string& path,
        const SculptureCausalExecutionSnapshot& snapshot) {
        if (path.empty() || snapshot.revision() == 0U) return false;
        // Prepare complete JSON before IO; exclusive sibling staging avoids
        // touching a pre-existing .tmp file or following its symlink.
        return file_publication_detail::publishTextFile(
            path, [&] { return serialize(snapshot); });
    }

private:
    static void writeSelectionExplanation(std::ostream& out,
                                         const inspection::SelectionExplanation& explanation) {
        out << "{\"explanation\":\"" << inspection::selectionExplanationName(explanation.kind)
            << "\",\"age_unit\":\"application-frames\",\"source_frame_sequence\":";
        if (explanation.sourceFrameSequence) out << *explanation.sourceFrameSequence;
        else out << "null";
        out << ",\"source_frame_gap\":";
        if (explanation.sourceFrameGap) out << *explanation.sourceFrameGap;
        else out << "null";
        out << ",\"deferral_reason\":\"" << name(explanation.deferralReason) << "\"}";
    }

    static constexpr const char* schema(SculptureCausalJsonVersion version) noexcept {
        return version == SculptureCausalJsonVersion::V1 ? "epic-causal-execution-v1" : "epic-causal-execution-v2";
    }
    static void prepare(std::ostream& out) {
        out.imbue(std::locale::classic());
        out << std::setprecision(std::numeric_limits<float>::max_digits10);
    }
    static void scalar(std::ostream& out, float value) {
        if (!std::isfinite(value)) out << "null";
        else if (value == 0.0f && std::signbit(value)) out << "-0.0";
        else out << value;
    }
    static void writeKey(std::ostream& out, const SculptureCausalNodeKey& key) {
        out << "{\"kind\":\"" << name(key.kind)
            << "\",\"route\":\"" << name(key.route)
            << "\",\"sequence\":" << key.sequence << "}";
    }

    static void writeNode(
        std::ostream& out,
        const SculptureCausalNode& node,
        std::string_view indent,
        SculptureCausalJsonVersion version) {
        out << indent << "{\"key\":";
        writeKey(out, node.key);
        switch (node.key.kind) {
        case SculptureCausalNodeKind::FrameIntent: {
            const auto& value = node.frameIntent;
            out << ",\"frame_intent\":{"
                << "\"frame_sequence\":" << value.frameSequence
                << ",\"route\":\"" << name(value.route) << "\""
                << ",\"preset_index\":" << value.presetIndex
                << ",\"presentation_ownership\":"
                << static_cast<unsigned>(value.presentationOwnership)
                << ",\"compact_presentation_lease_epoch\":"
                << value.compactPresentationLeaseEpoch
                << ",\"simulation_time\":";
            scalar(out, value.simulationTime);
            out << ",\"effective_world_update_hz\":";
            scalar(out, value.effectiveWorldUpdateHz);
            out << ",\"dedicated_world_fallback\":" << boolean(value.dedicatedWorldFallback)
                << ",\"vr_enabled\":" << boolean(value.vrEnabled)
                << ",\"temporal_active\":" << boolean(value.temporalActive);
            if (version == SculptureCausalJsonVersion::V2) {
                const auto end = std::find(value.presetId.begin(), value.presetId.end(), '\0');
                const auto length = static_cast<std::size_t>(end - value.presetId.begin());
                out << ",\"frozen_control_projection\":{\"preset_id_label\":";
                if (length == 0U) out << "null";
                else inspection_json_detail::label(out, std::string_view(value.presetId.data(), length));
                out << ",\"preset_id_truncated\":" << boolean(value.presetIdTruncated)
                    << ",\"preset_id_sanitized\":" << boolean(value.presetIdSanitized)
                    << ",\"requested_stage_mask\":" << value.requestedStageMask
                    << ",\"stage_execution\":\"unknown\""
                    << ",\"reduction_id\":" << static_cast<unsigned>(value.reduction)
                    << ",\"seed_id\":" << static_cast<unsigned>(value.seed)
                    << ",\"color_effect_id\":" << static_cast<unsigned>(value.color)
                    << ",\"feedback_iterations\":" << value.feedbackIterations
                    << ",\"latent_w\":";
                scalar(out, value.latentW);
                out << "}";
            }
            out << "}";
            break;
        }
        case SculptureCausalNodeKind::ProductionExecution: {
            const auto& value = node.production;
            out << ",\"production\":{"
                << "\"schedule_sequence\":" << value.scheduleSequence
                << ",\"frame_sequence\":" << value.frameSequence
                << ",\"route\":\"" << name(value.route) << "\""
                << ",\"phase\":" << static_cast<unsigned>(value.phase)
                << ",\"generation_intent\":" << value.generationIntent
                << ",\"reset_epoch\":" << value.resetEpoch
                << ",\"semantic_lease_epoch\":" << value.semanticLeaseEpoch
                << ",\"simulation_time\":";
            scalar(out, value.simulationTime);
            out << ",\"simulation_delta_seconds\":";
            scalar(out, value.simulationDeltaSeconds);
            out << ",\"cadence_due\":" << boolean(value.cadenceDue)
                << ",\"parameter_triggered\":" << boolean(value.parameterTriggered)
                << ",\"attempt\":\"" << name(value.attempt) << "\""
                << ",\"deferral_reason\":\"" << name(value.deferralReason) << "\""
                << ",\"committed_generation\":" << value.committedGeneration
                << ",\"committed_reset_epoch\":" << value.committedResetEpoch
                << ",\"committed_semantic_lease_epoch\":"
                << value.committedSemanticLeaseEpoch
                << ",\"publication_outcome\":\"" << name(value.publicationOutcome) << "\""
                << ",\"publication\":{"
                << "\"generation\":" << value.publication.generation
                << ",\"reset_epoch\":" << value.publication.resetEpoch
                << ",\"slot_index\":" << value.publication.slotIndex
                << ",\"transport_lease_epoch\":" << value.publication.transportLeaseEpoch
                << ",\"allocation_generation\":" << value.publication.allocationGeneration
                << ",\"ready_serial\":" << value.publication.readySerial
                << ",\"transport\":\"" << name(value.publication.transport) << "\"}}";
            break;
        }
        case SculptureCausalNodeKind::Presentation: {
            const auto& value = node.presentation;
            out << ",\"presentation\":{"
                << "\"route\":\"" << name(value.route) << "\""
                << ",\"frame_sequence\":" << value.frameSequence
                << ",\"device_generation\":" << value.deviceGeneration
                << ",\"surface_generation\":" << value.surfaceGeneration
                << ",\"compact_generation\":" << value.compactGeneration
                << ",\"compact_production_receipt_sequence\":"
                << value.compactProductionReceiptSequence
                << ",\"world_generation\":" << value.worldGeneration
                << ",\"world_production_receipt_sequence\":"
                << value.worldProductionReceiptSequence
                << ",\"canonical_feedback_generation\":"
                << value.canonicalFeedbackGeneration
                << ",\"canonical_feedback_reset_epoch\":"
                << value.canonicalFeedbackResetEpoch
                << ",\"canonical_feedback_production_receipt_sequence\":"
                << value.canonicalFeedbackProductionReceiptSequence
                << ",\"implicit_generation\":" << value.implicitGeneration
                << ",\"implicit_production_receipt_sequence\":"
                << value.implicitProductionReceiptSequence
                << ",\"frame_outcome\":" << static_cast<unsigned>(value.outcome)
                << ",\"settled\":" << boolean(value.settled)
                << ",\"submitted\":" << boolean(value.submitted())
                << ",\"presentation_confirmed\":" << boolean(value.presentationConfirmed())
                << "}";
            break;
        }
        }
        out << "}";
    }

    static void writeEdge(
        std::ostream& out,
        const SculptureCausalEdge& edge,
        std::string_view indent) {
        out << indent << "{\"from\":";
        writeKey(out, edge.from);
        out << ",\"to\":";
        writeKey(out, edge.to);
        out << ",\"kind\":\"" << name(edge.kind) << "\"}";
    }

    [[nodiscard]] static constexpr const char* boolean(bool value) noexcept {
        return value ? "true" : "false";
    }
};

} // namespace epi_sculpt
