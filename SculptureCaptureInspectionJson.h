#pragma once

#include "SculptureCaptureInspection.h"
#include "SculptureCaptureBundleComparison.h"

#include <iomanip>
#include <limits>
#include <locale>
#include <ostream>
#include <sstream>

namespace epi_sculpt {
namespace capture_json_detail {

// Linux paths and recorded metadata are byte strings, not necessarily UTF-8.
// Hex is lossless even for NULs, invalid UTF-8 and terminal control characters.
inline void bytes(std::ostream& out, std::string_view value) {
    constexpr char hex[] = "0123456789abcdef";
    out << '"';
    for (const unsigned char byte : value) out << hex[byte >> 4] << hex[byte & 15];
    out << '"';
}

inline void bytes(std::ostream& out, const std::optional<std::string>& value) {
    if (value) bytes(out, std::string_view(*value));
    else out << "null";
}

// Decimal strings keep uint64 identity exact in JavaScript and other clients
// whose default JSON number type cannot represent values above 2^53.
inline void identity(std::ostream& out, const std::optional<std::uint64_t>& value) {
    if (value) out << '"' << *value << '"';
    else out << "null";
}

inline void boolean(std::ostream& out, const std::optional<bool>& value) {
    if (value) out << (*value ? "true" : "false");
    else out << "null";
}

inline void scalar(std::ostream& out, const std::optional<float>& value) {
    if (!value || !std::isfinite(*value)) out << "null";
    // Integer-looking -0 is commonly decoded as integer zero, losing its sign.
    // A fractional spelling preserves this recorded float without changing precision.
    else if (*value == 0.0f && std::signbit(*value)) out << "-0.0";
    else out << *value;
}

inline void stereoIntent(std::ostream& out, const CaptureStereoMetadata& metadata) {
    out << "{\"assessment\":\"" << captureStereoMetadataCode(metadata)
        << "\",\"scope\":\"frozen_frame_configuration\",\"enabled\":";
    const auto* stereo = metadata.value();
    boolean(out, stereo ? std::optional<bool>{stereo->enabled} : std::nullopt);
    const bool rigRecorded = stereo && stereo->enabled;
    out << ",\"requested_ipd_m\":";
    scalar(out, rigRecorded ? std::optional<float>{stereo->rig.interpupillaryDistance} : std::nullopt);
    out << ",\"requested_convergence_m\":";
    scalar(out, rigRecorded ? std::optional<float>{stereo->rig.focalConvergenceDistance} : std::nullopt);
    out << ",\"converged\":";
    boolean(out, rigRecorded ? std::optional<bool>{stereo->rig.toeInConvergence} : std::nullopt);
    out << ",\"diagnostic\":";
    if (const auto* code = captureStereoDiagnosticCode(metadata)) {
        out << "{\"code\":\"" << code << "\",\"field\":";
        if (const auto* field = captureStereoFieldName(metadata.problemField)) out << '"' << field << '"';
        else out << "null";
        out << '}';
    } else out << "null";
    out << '}';
}

inline const char* formatName(CaptureManifestFormatAdmission format) noexcept {
    switch (format) {
    case CaptureManifestFormatAdmission::Legacy: return "legacy";
    case CaptureManifestFormatAdmission::V1: return "v1";
    case CaptureManifestFormatAdmission::Duplicate: return "duplicate";
    case CaptureManifestFormatAdmission::Unsupported: return "unsupported";
    }
    return "unsupported";
}

inline void bundleV2(std::ostream& out, const CaptureInspection& report) {
    const auto* doc = report.bundleV2 ? &*report.bundleV2 : nullptr;
    out << "{\"assessment\":\"" << (doc ? "declared_v2" : "invalid_v2")
        << "\",\"scope\":\"declarations-only\",\"generation_hex\":";
    if (doc) bytes(out, std::string_view(doc->generation));
    else out << "null";
    out << ",\"width\":";
    identity(out, doc ? std::optional<std::uint64_t>{doc->width} : std::nullopt);
    out << ",\"height\":";
    identity(out, doc ? std::optional<std::uint64_t>{doc->height} : std::nullopt);
    out << ",\"attachments\":[";
    bool first = true;
    if (doc) for (std::size_t i = 0; i < doc->entries.size(); ++i) {
        const auto& entry = doc->entries[i];
        if (!entry.present) continue;
        if (!first) out << ',';
        first = false;
        out << "{\"role\":\"" << kCaptureV2RoleNames[i] << "\",\"name_hex\":";
        bytes(out, std::string_view(entry.name));
        out << ",\"declared_size\":\"" << entry.size
            << "\",\"declared_sha256\":\"" << captureV2Hex(entry.sha256) << "\"}";
    }
    out << "],\"artifact_files_probed\":false,\"hashes_verified\":false,\"receipt_join_verified\":false}";
}

inline void artifact(std::ostream& out, const std::filesystem::path& parent,
                     const CaptureInspection& report, CaptureInspectionArtifact kind) {
    const auto reference = captureInspectionReference(report, kind);
    out << "{\"recorded_hex\":";
    bytes(out, reference.name);
    out << ",\"status\":\""
        << captureInspectionPresenceCode(inspectCaptureReferencePresence(parent, reference)) << "\"}";
}

inline void contribution(std::ostream& out, const CaptureInspection& report,
                         std::optional<std::uint64_t> selected,
                         std::optional<std::uint64_t> displayed,
                         std::optional<std::uint64_t> receipt) {
    out << "{\"selected_generation\":";
    identity(out, selected);
    out << ",\"displayed_generation\":";
    identity(out, displayed);
    out << ",\"production_receipt_sequence\":";
    identity(out, receipt);
    out << ",\"assessment\":\""
        << captureContributionAssessmentCode(assessCaptureContribution(report, selected, displayed)) << "\"}";
}

} // namespace capture_json_detail

// A cold, derived read model. It grants no restore/production authority and
// cannot verify image contents, an accepted-input join or exact replay.
// Build the whole document before publishing to avoid partial JSON on failure.
[[nodiscard]] inline std::string captureInspectionJson(
    const CaptureInspection& report, const std::filesystem::path& manifest) {
    namespace json = capture_json_detail;
    using Artifact = CaptureInspectionArtifact;
    const auto& refs = report.references;
    const auto admission = captureManifestRestoreAdmission(refs);
    const auto parent = manifest.parent_path();
    std::ostringstream out;
    out.exceptions(std::ios::badbit | std::ios::failbit);
    out.imbue(std::locale::classic());
    out << std::setprecision(std::numeric_limits<float>::max_digits10);
    out << "{\"schema\":\"epic-capture-inspection-v1\","
           "\"byte_string_encoding\":\"hex\",\"unsigned_integer_encoding\":\"decimal-string\","
           "\"evidence\":\"recorded-values\",\"ambiguous_numeric_fields\":[";
    bool firstAmbiguousField = true;
    forEachAmbiguousCaptureField(report, [&](std::string_view key) {
        if (!firstAmbiguousField) out << ',';
        firstAmbiguousField = false;
        out << '"' << key << '"'; // Names come from the fixed inventory, not file bytes.
    });
    out << "],\"preset\":{\"id_hex\":";
    // Share the inspection model's unique-only provenance with text consumers.
    // Recorded origin is descriptive, never an authenticated identity.
    json::bytes(out, report.presetId);
    out << ",\"revision_hex\":";
    json::bytes(out, report.presetRevision);
    out << '}';
    if (report.bundleV2Declared) {
        out << ",\"bundle_v2\":";
        json::bundleV2(out, report);
    }
    out << ",\"restore_references\":{\"format\":\""
        << (report.bundleV2Declared && refs.formatCount == 1 ? "v2" : json::formatName(admission.format))
        << "\",\"lexically_admitted\":" << (admission.admitted() ? "true" : "false");
    if (report.bundleV2Declared)
        out << ",\"admission_scope\":\"application-v2-content-validation-not-performed\"";
    const auto restore = [&](const char* key, Artifact kind) {
        out << ",\"" << key << "\":";
        json::artifact(out, parent, report, kind);
    };
    restore("world", Artifact::World);
    restore("epimode", Artifact::Epimode);
    restore("volume", Artifact::Volume);
    out << "},\"artifacts\":{";
    const auto extra = [&](const char* key, Artifact kind, bool first = false) {
        if (!first) out << ',';
        out << '"' << key << "\":";
        json::artifact(out, parent, report, kind);
    };
    extra("image", Artifact::Image, true);
    extra("epi", Artifact::Epi);
    extra("profile", Artifact::Profile);
    extra("causal", Artifact::Causal);
    out << "},\"presentation\":{\"assessment\":\""
        << capturePresentationAssessmentCode(assessCapturePresentation(report)) << "\",\"outcome_known\":";
    json::boolean(out, report.outcomeKnown);
    out << ",\"confirmed\":";
    json::boolean(out, report.presentationConfirmed);
    out << ",\"outcome_code\":";
    json::identity(out, report.presentationOutcomeCode);
    out << ",\"frame_sequence\":";
    json::identity(out, report.frameSequence);
    out << ",\"device_generation\":";
    json::identity(out, report.deviceGeneration);
    out << ",\"surface_generation\":";
    json::identity(out, report.surfaceGeneration);
    out << ",\"capture_submission_serial\":";
    json::identity(out, report.captureSubmissionSerial);
    out << "},\"stereo_intent\":";
    json::stereoIntent(out, refs.stereo);
    out << ",\"contributions\":{";
    for (std::size_t i = 0; i < kCaptureContributionRoutes.size(); ++i) {
        const auto& route = kCaptureContributionRoutes[i];
        if (i != 0) out << ',';
        out << '"' << route.name << "\":";
        json::contribution(out, report, report.*(route.selected), report.*(route.displayed),
                           report.*(route.receipt));
    }
    out << "},\"sculpture_controls\":{\"inner_id\":";
    json::identity(out, report.innerId);
    out << ",\"reduction_id\":";
    json::identity(out, report.reductionId);
    out << ",\"color_effect_id\":";
    json::identity(out, report.colorEffectId);
    out << ",\"seed_id\":";
    json::identity(out, report.seedId);
    out << ",\"stage_mask\":";
    json::identity(out, report.sculptureStageMask);
    out << "},\"canonical\":{\"version_recorded\":"
        << (report.canonicalCaptureVersionRecorded ? "true" : "false") << ",\"version\":";
    json::identity(out, report.canonicalCaptureVersion);
    const auto declaration = assessInspectedCanonicalDeclaration(report);
    out << ",\"declaration_assessment\":\"" << canonicalCaptureDeclarationCode(declaration) << '"';
    out << ",\"input\":";
    json::artifact(out, parent, report, Artifact::CanonicalMode);
    out << ",\"seed_image\":";
    json::artifact(out, parent, report, Artifact::CanonicalSeedImage);
    out << ",\"field_width\":";
    json::identity(out, report.canonicalFieldWidth);
    out << ",\"field_height\":";
    json::identity(out, report.canonicalFieldHeight);
    out << ",\"field_generation\":";
    json::identity(out, report.canonicalFieldGeneration);
    out << ",\"field_reset_epoch\":";
    json::identity(out, report.canonicalFieldResetEpoch);
    out << ",\"selected_reset_epoch\":";
    json::identity(out, report.selectedCanonicalResetEpoch);
    out << ",\"submission_serial\":";
    json::identity(out, report.canonicalSubmissionSerial);
    out << ",\"readback_submission_serial\":";
    json::identity(out, report.canonicalReadbackSubmissionSerial);
    out << ",\"kernel_time\":";
    json::scalar(out, report.canonicalKernelTime);
    out << ",\"accepted_control_time\":";
    json::scalar(out, report.canonicalAcceptedControlTime);
    out << ",\"production_delta\":";
    json::scalar(out, report.canonicalProductionDelta);
    out << ",\"selection_and_producer_assessment\":\""
        << canonicalRecordedIdentityAssessmentCode(assessCanonicalSelectionAndProducer(report))
        << "\",\"submission_and_readback_assessment\":\""
        << canonicalRecordedIdentityAssessmentCode(assessCanonicalSubmissionAndReadback(report))
        << "\",\"capture_submission_join_assessment\":\""
        << canonicalRecordedIdentityAssessmentCode(assessCanonicalCaptureSubmissionJoin(report)) << '"';
    out << "},\"verification\":{\"artifact_contents_verified\":false,"
           "\"restoration_verified\":false,\"presentation_verified\":false,\"exact_replay_established\":false}}\n";
    return out.str();
}

// Requested configuration only; neither mode probes referenced artifact files.
[[nodiscard]] inline std::string captureStereoComparisonJson(
    const CaptureInspection& first, const CaptureInspection& second,
    const std::filesystem::path& firstManifest, const std::filesystem::path& secondManifest) {
    namespace json = capture_json_detail;
    const auto comparison = compareCaptureStereoIntent(first.references.stereo, second.references.stereo);
    std::ostringstream out;
    out.exceptions(std::ios::badbit | std::ios::failbit);
    out.imbue(std::locale::classic());
    out << std::setprecision(std::numeric_limits<float>::max_digits10)
        << "{\"schema\":\"epic-stereo-intent-comparison-v1\","
           "\"scope\":\"frozen_frame_configuration\",\"comparison\":\"numerical_float32_values\","
           "\"reference_encoding\":\"hex-bytes\",\"artifact_files_probed\":false,\"first\":{\"manifest_hex\":";
    json::bytes(out, std::string_view(firstManifest.string()));
    out << ",\"stereo_intent\":";
    json::stereoIntent(out, first.references.stereo);
    out << "},\"second\":{\"manifest_hex\":";
    json::bytes(out, std::string_view(secondManifest.string()));
    out << ",\"stereo_intent\":";
    json::stereoIntent(out, second.references.stereo);
    out << "},\"changes\":{\"configuration\":\"" << captureStereoChangeCode(comparison.configuration)
        << "\",\"enabled\":\"" << captureStereoChangeCode(comparison.enabled)
        << "\",\"converged\":\"" << captureStereoChangeCode(comparison.converged)
        << "\",\"requested_ipd_m\":\"" << captureStereoChangeCode(comparison.ipd)
        << "\",\"requested_convergence_m\":\"" << captureStereoChangeCode(comparison.convergenceDistance)
        << "\"},\"deltas_m\":{\"requested_ipd_m\":";
    const auto delta = [&](const std::optional<double>& value) {
        if (value) out << std::setprecision(std::numeric_limits<double>::max_digits10) << *value;
        else out << "null";
    };
    delta(comparison.ipdDeltaMeters);
    out << ",\"requested_convergence_m\":";
    delta(comparison.convergenceDeltaMeters);
    out << "},\"verification\":{\"artifact_contents_verified\":false,\"restoration_verified\":false,"
           "\"presentation_verified\":false,\"exact_replay_established\":false}}\n";
    return out.str();
}

// Machine-readable export of the pure dependency read model. Unlike a single
// capture's presence report, this never performs artifact filesystem queries.
[[nodiscard]] inline std::string captureCanonicalDependencyComparisonJson(
    const CaptureInspection& first, const CaptureInspection& second,
    const std::filesystem::path& firstManifest, const std::filesystem::path& secondManifest) {
    namespace json = capture_json_detail;
    using Artifact = CaptureInspectionArtifact;
    using Change = CaptureDependencyChange;
    const auto comparison = compareCanonicalCaptureDependencies(first, second);
    const auto code = [](Change change) {
        switch (change) {
        case Change::NotRecorded: return "not_recorded";
        case Change::Added: return "added";
        case Change::Removed: return "removed";
        case Change::SameReference: return "same_reference";
        case Change::ChangedReference: return "changed_reference";
        case Change::NotComparable: return "not_comparable";
        }
        return "not_comparable";
    };
    std::ostringstream out;
    out.exceptions(std::ios::badbit | std::ios::failbit);
    out.imbue(std::locale::classic());
    out << "{\"schema\":\"epic-canonical-dependency-comparison-v1\","
           "\"reference_encoding\":\"hex-bytes\",\"artifact_files_probed\":false,"
           "\"first\":{\"manifest_hex\":";
    json::bytes(out, std::string_view(firstManifest.string()));
    out << ",\"declaration_assessment\":\""
        << canonicalCaptureDeclarationCode(comparison.firstDeclaration)
        << "\"},\"second\":{\"manifest_hex\":";
    json::bytes(out, std::string_view(secondManifest.string()));
    out << ",\"declaration_assessment\":\""
        << canonicalCaptureDeclarationCode(comparison.secondDeclaration)
        << "\"},\"dependencies\":{";
    const auto dependency = [&](const char* key, Artifact kind, Change change) {
        const auto a = captureInspectionReference(first, kind);
        const auto b = captureInspectionReference(second, kind);
        out << '"' << key << "\":{\"change\":\"" << code(change) << "\",\"first_recorded_hex\":";
        json::bytes(out, a.name);
        out << ",\"second_recorded_hex\":";
        json::bytes(out, b.name);
        out << ",\"first_reference_admitted\":" << (a.mayProbe ? "true" : "false")
            << ",\"second_reference_admitted\":" << (b.mayProbe ? "true" : "false") << '}';
    };
    dependency("mode", Artifact::CanonicalMode, comparison.mode);
    out << ',';
    dependency("seed_image", Artifact::CanonicalSeedImage, comparison.seedImage);
    out << "},\"verification\":{\"artifact_contents_verified\":false,"
           "\"restoration_verified\":false,\"presentation_verified\":false,\"exact_replay_established\":false}}\n";
    return out.str();
}

// Declared content identity stays separate from generation/path/extent identity.
// Both manifests have already been acquired; no attachment is observed here.
[[nodiscard]] inline std::string captureBundleComparisonJson(
    const CaptureInspection& first, const CaptureInspection& second,
    const std::filesystem::path& firstManifest, const std::filesystem::path& secondManifest) {
    namespace json = capture_json_detail;
    const auto comparison = compareCaptureBundleDeclarations(first, second);
    std::ostringstream out;
    out.exceptions(std::ios::badbit | std::ios::failbit);
    out.imbue(std::locale::classic());
    out << "{\"schema\":\"epic-capture-bundle-comparison-v1\","
           "\"scope\":\"declared-attachment-content\",\"reference_encoding\":\"hex-bytes\","
           "\"artifact_files_probed\":false,\"first\":";
    const auto input = [&](const CaptureInspection& report, const std::filesystem::path& path) {
        const auto* doc = inspectedBundleV2(report);
        out << "{\"assessment\":\"" << captureBundleComparisonInputCode(report) << "\",\"manifest_hex\":";
        json::bytes(out, std::string_view(path.string()));
        out << ",\"generation_hex\":";
        if (doc) json::bytes(out, std::string_view(doc->generation));
        else out << "null";
        out << ",\"width\":";
        json::identity(out, doc ? std::optional<std::uint64_t>{doc->width} : std::nullopt);
        out << ",\"height\":";
        json::identity(out, doc ? std::optional<std::uint64_t>{doc->height} : std::nullopt);
        out << '}';
    };
    input(first, firstManifest);
    out << ",\"second\":";
    input(second, secondManifest);
    out << ",\"attachment_content\":\"" << captureBundleContentChangeCode(comparison.attachmentContent)
        << "\",\"generation_label_changed\":";
    json::boolean(out, comparison.generationLabelChanged);
    out << ",\"extent_changed\":";
    json::boolean(out, comparison.extentChanged);
    out << ",\"attachments\":[";
    const auto attachment = [&](const CaptureInspection& report, std::size_t index) {
        const auto* doc = inspectedBundleV2(report);
        const auto* entry = doc && doc->entries[index].present ? &doc->entries[index] : nullptr;
        if (!entry) { out << "null"; return; }
        out << "{\"name_hex\":";
        json::bytes(out, std::string_view(entry->name));
        out << ",\"declared_size\":\"" << entry->size
            << "\",\"declared_sha256\":\"" << captureV2Hex(entry->sha256) << "\"}";
    };
    for (std::size_t i = 0; i < kCaptureV2Roles; ++i) {
        if (i) out << ',';
        out << "{\"role\":\"" << kCaptureV2RoleNames[i] << "\",\"change\":\""
            << captureBundleContentChangeCode(comparison.attachments[i].content)
            << "\",\"reference_name_changed\":";
        json::boolean(out, comparison.attachments[i].referenceNameChanged);
        out << ",\"first\":";
        attachment(first, i);
        out << ",\"second\":";
        attachment(second, i);
        out << '}';
    }
    out << "],\"verification\":{\"artifact_contents_verified\":false,\"hashes_verified\":false,"
           "\"restoration_verified\":false,\"presentation_verified\":false,\"exact_replay_established\":false}}\n";
    return out.str();
}

} // namespace epi_sculpt
