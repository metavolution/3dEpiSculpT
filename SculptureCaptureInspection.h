#pragma once

#include "SculptureCaptureManifest.h"
#include "SculptureCaptureIntegrity.h"

#include <array>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

namespace epi_sculpt {

// Derived inspection only: none of these values admit work or authenticate a
// presentation. Missing/malformed or multiply declared numeric facts and preset
// origins remain unknown; references retain the loader's last-key-wins semantics.
struct CaptureInspection {
    CaptureManifestReferences references;
    // These are recorded artifact names for inspection, not restore inputs.
    std::string png;
    std::string epi;
    std::string profile;
    std::string causal;
    std::string canonicalMode;
    // A single valid declaration describes starting recipe provenance, not the
    // full captured parameter state. Repeated declarations cannot establish it.
    std::optional<std::string> presetId;
    std::optional<std::string> presetRevision;
    // Canonical producer metadata describes an accepted field, not a snapshot.
    bool canonicalCaptureVersionRecorded = false;
    std::optional<std::uint64_t> canonicalCaptureVersion;
    std::optional<std::uint64_t> canonicalFieldWidth;
    std::optional<std::uint64_t> canonicalFieldHeight;
    std::optional<std::uint64_t> canonicalFieldGeneration;
    std::optional<std::uint64_t> canonicalFieldResetEpoch;
    std::optional<std::uint64_t> canonicalSubmissionSerial;
    std::optional<std::uint64_t> canonicalReadbackSubmissionSerial;
    std::optional<float> canonicalKernelTime;
    std::optional<float> canonicalAcceptedControlTime;
    std::optional<float> canonicalProductionDelta;
    std::optional<bool> outcomeKnown;
    std::optional<bool> presentationConfirmed;
    std::optional<std::uint64_t> presentationOutcomeCode;
    bool presentationOutcomeCodeRecorded = false;
    // Stage IDs are recorded selections; their interpretation belongs to the
    // producer's versioned component catalog, not the inspection tool.
    std::optional<std::uint64_t> innerId;
    std::optional<std::uint64_t> reductionId;
    std::optional<std::uint64_t> colorEffectId;
    std::optional<std::uint64_t> seedId;
    // Raw authored sculpture mask; GPU packing normalizes its diagnostic field.
    std::optional<std::uint64_t> sculptureStageMask;
    std::optional<std::uint64_t> frameSequence;
    std::optional<std::uint64_t> deviceGeneration;
    std::optional<std::uint64_t> surfaceGeneration;
    std::optional<std::uint64_t> captureSubmissionSerial;
    std::optional<std::uint64_t> selectedCompactGeneration;
    std::optional<std::uint64_t> selectedWorldGeneration;
    std::optional<std::uint64_t> selectedCanonicalGeneration;
    std::optional<std::uint64_t> selectedCanonicalResetEpoch;
    std::optional<std::uint64_t> selectedImplicitGeneration;
    // Recorded displayed values are distinct from the selection for capture.
    // Zero is the writer's no-confirmed-generation sentinel.
    std::optional<std::uint64_t> displayedCompactGeneration;
    std::optional<std::uint64_t> displayedWorldGeneration;
    std::optional<std::uint64_t> displayedCanonicalGeneration;
    std::optional<std::uint64_t> displayedImplicitGeneration;
    // Receipt sequences are recorded causal identifiers, not verification.
    std::optional<std::uint64_t> selectedCompactReceiptSequence;
    std::optional<std::uint64_t> selectedWorldReceiptSequence;
    std::optional<std::uint64_t> selectedCanonicalReceiptSequence;
    std::optional<std::uint64_t> selectedImplicitReceiptSequence;
    // A bounded read-model mask, not a persisted identity or admission decision.
    // Consumers enumerate static field names through forEachAmbiguousCaptureField.
    // Trailing placement preserves existing partial aggregate initializers.
    std::uint64_t ambiguousNumericFields = 0U;
    // Structure only. Never acquire attachments through this read model.
    // Keep the presence flag even when a later format declaration is different.
    bool bundleV2Declared = false;
    std::optional<CaptureV2Document> bundleV2;
};

// A valid bundle-v2 can carry the existing canonical-input metadata-v1 schema.
// Project that declaration through the same lexical rules as the actual v2
// reader, without granting restoration or attachment-probing authority.
[[nodiscard]] inline CanonicalCaptureDeclarationAssessment assessInspectedCanonicalDeclaration(
    const CaptureInspection& report) {
    if (!report.bundleV2) return assessCanonicalCaptureDeclaration(report.references);
    auto refs = report.references;
    refs.format = "epic-capture-bundle-v1";
    return assessCanonicalCaptureDeclaration(refs);
}

// A borrowed read-model view, consumed synchronously by an inspector. Manifest
// policy decides which name may be observed; this grants no restore authority.
enum class CaptureInspectionArtifact {
    World, Epimode, Volume, Image, Epi, Profile, Causal, CanonicalMode, CanonicalSeedImage,
};

struct CaptureInspectionReference {
    std::string_view name;
    bool mayProbe = false;
    bool ambiguous = false;
};

[[nodiscard]] inline CaptureInspectionReference captureInspectionReference(
    const CaptureInspection& report, CaptureInspectionArtifact artifact) noexcept {
    using Artifact = CaptureInspectionArtifact;
    const auto& refs = report.references;
    const auto admission = captureManifestRestoreAdmission(refs);
    const bool v1 = admission.format == CaptureManifestFormatAdmission::V1;
    switch (artifact) {
    case Artifact::World: return {refs.world, admission.admitted(), v1 && admission.worldDuplicate};
    case Artifact::Epimode: return {refs.epimode, admission.admitted(), v1 && admission.epimodeDuplicate};
    case Artifact::Volume: return {refs.volmem, admission.admitted(), v1 && admission.volmemDuplicate};
    case Artifact::Image: return {report.png, !report.bundleV2Declared && captureManifestSiblingName(report.png)};
    case Artifact::Epi: return {report.epi, !report.bundleV2Declared && captureManifestSiblingName(report.epi)};
    case Artifact::Profile: return {report.profile, !report.bundleV2Declared && captureManifestSiblingName(report.profile)};
    case Artifact::Causal: return {report.causal, !report.bundleV2Declared && captureManifestSiblingName(report.causal)};
    case Artifact::CanonicalMode:
        return {refs.canonicalMode,
            assessCanonicalCaptureDeclaration(refs) == CanonicalCaptureDeclarationAssessment::DeclaredV1,
            refs.canonicalModeCount > 1 || refs.canonicalCaptureVersionCount > 1};
    case Artifact::CanonicalSeedImage:
        return {refs.canonicalSeedImage,
            assessCanonicalCaptureDeclaration(refs) == CanonicalCaptureDeclarationAssessment::DeclaredV1
                && refs.canonicalSeedImageCount == 1U,
            refs.canonicalSeedImageCount > 1
                || (refs.canonicalSeedImageCount != 0 && refs.canonicalCaptureVersionCount > 1)};
    }
    return {};
}

// Derived reference deltas for tools/editors. Equal names in separate capture
// directories do not establish equal bytes, mathematical state or replay.
enum class CaptureDependencyChange {
    NotRecorded, Added, Removed, SameReference, ChangedReference, NotComparable,
};

struct CanonicalCaptureDependencyComparison {
    CanonicalCaptureDeclarationAssessment firstDeclaration;
    CanonicalCaptureDeclarationAssessment secondDeclaration;
    CaptureDependencyChange mode = CaptureDependencyChange::NotComparable;
    CaptureDependencyChange seedImage = CaptureDependencyChange::NotComparable;
};

[[nodiscard]] inline CanonicalCaptureDependencyComparison compareCanonicalCaptureDependencies(
    const CaptureInspection& first, const CaptureInspection& second) noexcept {
    using Assessment = CanonicalCaptureDeclarationAssessment;
    using Change = CaptureDependencyChange;
    CanonicalCaptureDependencyComparison result{
        assessCanonicalCaptureDeclaration(first.references),
        assessCanonicalCaptureDeclaration(second.references)};
    const auto comparable = [](Assessment declaration) {
        return declaration == Assessment::NotRecorded || declaration == Assessment::DeclaredV1;
    };
    // Rejected declarations are unknown, never an apparently removed input.
    if (!comparable(result.firstDeclaration) || !comparable(result.secondDeclaration)) return result;

    const auto compare = [&](CaptureInspectionArtifact artifact) {
        const auto a = captureInspectionReference(first, artifact).name;
        const auto b = captureInspectionReference(second, artifact).name;
        if (a.empty() && b.empty()) return Change::NotRecorded;
        if (a.empty()) return Change::Added;
        if (b.empty()) return Change::Removed;
        return a == b ? Change::SameReference : Change::ChangedReference;
    };
    result.mode = compare(CaptureInspectionArtifact::CanonicalMode);
    result.seedImage = compare(CaptureInspectionArtifact::CanonicalSeedImage);
    return result;
}

[[nodiscard]] inline const char* captureDependencyChangeDescription(CaptureDependencyChange change) noexcept {
    switch (change) {
    case CaptureDependencyChange::NotRecorded: return "not recorded";
    case CaptureDependencyChange::Added: return "added";
    case CaptureDependencyChange::Removed: return "removed";
    case CaptureDependencyChange::SameReference: return "same recorded reference";
    case CaptureDependencyChange::ChangedReference: return "changed recorded reference";
    case CaptureDependencyChange::NotComparable: return "not comparable";
    }
    return "not comparable";
}

enum class CaptureInspectionPresence {
    NotRecorded, Ambiguous, NotProbed, Missing, Inaccessible, RegularFile, Other,
};

// Metadata-only observation. In particular, do not resolve an arbitrary path
// first and discover afterwards that its declaration should have been rejected.
[[nodiscard]] inline CaptureInspectionPresence inspectCaptureReferencePresence(
    const std::filesystem::path& parent, CaptureInspectionReference reference) {
    using Presence = CaptureInspectionPresence;
    if (reference.ambiguous) return Presence::Ambiguous;
    if (reference.name.empty() || reference.name == "none") return Presence::NotRecorded;
    if (!reference.mayProbe) return Presence::NotProbed;
    std::error_code error;
    const auto status = std::filesystem::status(parent / std::filesystem::path(reference.name), error);
    if (error == std::errc::no_such_file_or_directory
        || status.type() == std::filesystem::file_type::not_found) return Presence::Missing;
    if (error) return Presence::Inaccessible;
    return std::filesystem::is_regular_file(status) ? Presence::RegularFile : Presence::Other;
}

[[nodiscard]] inline const char* captureInspectionPresenceCode(CaptureInspectionPresence presence) noexcept {
    using Presence = CaptureInspectionPresence;
    switch (presence) {
    case Presence::NotRecorded: return "not_recorded";
    case Presence::Ambiguous: return "ambiguous";
    case Presence::NotProbed: return "not_probed";
    case Presence::Missing: return "missing";
    case Presence::Inaccessible: return "inaccessible";
    case Presence::RegularFile: return "regular_file";
    case Presence::Other: return "other";
    }
    return "not_probed";
}

[[nodiscard]] inline std::optional<std::uint64_t> inspectCaptureUnsigned(std::string_view text) {
    if (text.empty()) return std::nullopt;
    std::uint64_t result = 0;
    const auto parsed = std::from_chars(text.data(), text.data() + text.size(), result);
    if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size()) return std::nullopt;
    return result;
}

[[nodiscard]] inline std::optional<bool> inspectCaptureBool(std::string_view text) {
    if (text == "0") return false;
    if (text == "1") return true;
    return std::nullopt;
}

[[nodiscard]] inline std::optional<float> inspectCaptureFloat(std::string_view text) {
    if (text.empty()) return std::nullopt;
    float result = 0.0f;
    const auto parsed = std::from_chars(text.data(), text.data() + text.size(), result);
    if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size()
        || !std::isfinite(result)) return std::nullopt;
    return result;
}

// The v1 canonical recorder accepts only nonzero bounded field dimensions and
// a finite nonnegative production delta; do not present invalid values as data.
[[nodiscard]] inline std::optional<std::uint64_t> inspectCanonicalFieldExtent(std::string_view text) {
    const auto value = inspectCaptureUnsigned(text);
    if (!value || *value == 0U || *value > kMaxCanonicalCaptureFieldExtent) return std::nullopt;
    return value;
}

[[nodiscard]] inline std::optional<float> inspectCanonicalProductionDelta(std::string_view text) {
    const auto value = inspectCaptureFloat(text);
    if (!value || *value < 0.0f) return std::nullopt;
    return value;
}

[[nodiscard]] inline std::optional<std::uint64_t> inspectCaptureOutcomeCode(std::string_view text) {
    const auto value = inspectCaptureUnsigned(text);
    if (!value || *value > std::numeric_limits<std::uint32_t>::max()) return std::nullopt;
    return value;
}

namespace capture_inspection_detail {

template<typename T>
struct NumericField {
    std::string_view key;
    std::optional<T> CaptureInspection::* member;
    std::optional<T> (*parse)(std::string_view);
    bool CaptureInspection::* recorded = nullptr;
};

using UnsignedField = NumericField<std::uint64_t>;
using FloatField = NumericField<float>;
using BoolField = NumericField<bool>;

// One inventory owns each numeric field's name, parser and destination. In
// particular a producer generation must not inherit an outcome-code width.
inline constexpr std::array kUnsignedFields{
    UnsignedField{"canonical_feedback_capture_version", &CaptureInspection::canonicalCaptureVersion, inspectCaptureUnsigned, &CaptureInspection::canonicalCaptureVersionRecorded},
    UnsignedField{"canonical_feedback_field_width", &CaptureInspection::canonicalFieldWidth, inspectCanonicalFieldExtent},
    UnsignedField{"canonical_feedback_field_height", &CaptureInspection::canonicalFieldHeight, inspectCanonicalFieldExtent},
    UnsignedField{"canonical_feedback_field_generation", &CaptureInspection::canonicalFieldGeneration, inspectCaptureUnsigned},
    UnsignedField{"canonical_feedback_field_reset_epoch", &CaptureInspection::canonicalFieldResetEpoch, inspectCaptureUnsigned},
    UnsignedField{"canonical_feedback_submission_serial", &CaptureInspection::canonicalSubmissionSerial, inspectCaptureUnsigned},
    UnsignedField{"canonical_feedback_readback_submission_serial", &CaptureInspection::canonicalReadbackSubmissionSerial, inspectCaptureUnsigned},
    UnsignedField{"presentation_outcome", &CaptureInspection::presentationOutcomeCode, inspectCaptureOutcomeCode, &CaptureInspection::presentationOutcomeCodeRecorded},
    UnsignedField{"inner_id", &CaptureInspection::innerId, inspectCaptureOutcomeCode},
    UnsignedField{"reduction_id", &CaptureInspection::reductionId, inspectCaptureOutcomeCode},
    UnsignedField{"color_effect_id", &CaptureInspection::colorEffectId, inspectCaptureOutcomeCode},
    UnsignedField{"seed_id", &CaptureInspection::seedId, inspectCaptureOutcomeCode},
    UnsignedField{"sculpture_stage_mask", &CaptureInspection::sculptureStageMask, inspectCaptureOutcomeCode},
    UnsignedField{"presentation_frame_sequence", &CaptureInspection::frameSequence, inspectCaptureUnsigned},
    UnsignedField{"presentation_device_generation", &CaptureInspection::deviceGeneration, inspectCaptureUnsigned},
    UnsignedField{"presentation_surface_generation", &CaptureInspection::surfaceGeneration, inspectCaptureUnsigned},
    UnsignedField{"capture_submission_serial", &CaptureInspection::captureSubmissionSerial, inspectCaptureUnsigned},
    UnsignedField{"compact_generation", &CaptureInspection::displayedCompactGeneration, inspectCaptureOutcomeCode},
    UnsignedField{"world_generation", &CaptureInspection::displayedWorldGeneration, inspectCaptureOutcomeCode},
    UnsignedField{"canonical_feedback_generation", &CaptureInspection::displayedCanonicalGeneration, inspectCaptureUnsigned},
    UnsignedField{"implicit_generation", &CaptureInspection::displayedImplicitGeneration, inspectCaptureOutcomeCode},
    UnsignedField{"selected_compact_generation", &CaptureInspection::selectedCompactGeneration, inspectCaptureUnsigned},
    UnsignedField{"selected_world_generation", &CaptureInspection::selectedWorldGeneration, inspectCaptureUnsigned},
    UnsignedField{"selected_canonical_feedback_generation", &CaptureInspection::selectedCanonicalGeneration, inspectCaptureUnsigned},
    UnsignedField{"selected_canonical_feedback_reset_epoch", &CaptureInspection::selectedCanonicalResetEpoch, inspectCaptureUnsigned},
    UnsignedField{"selected_implicit_generation", &CaptureInspection::selectedImplicitGeneration, inspectCaptureUnsigned},
    UnsignedField{"selected_compact_production_receipt_sequence", &CaptureInspection::selectedCompactReceiptSequence, inspectCaptureUnsigned},
    UnsignedField{"selected_world_production_receipt_sequence", &CaptureInspection::selectedWorldReceiptSequence, inspectCaptureUnsigned},
    UnsignedField{"selected_canonical_feedback_production_receipt_sequence", &CaptureInspection::selectedCanonicalReceiptSequence, inspectCaptureUnsigned},
    UnsignedField{"selected_implicit_production_receipt_sequence", &CaptureInspection::selectedImplicitReceiptSequence, inspectCaptureUnsigned},
};

inline constexpr std::array kFloatFields{
    FloatField{"canonical_feedback_kernel_time", &CaptureInspection::canonicalKernelTime, inspectCaptureFloat},
    FloatField{"canonical_feedback_accepted_control_time", &CaptureInspection::canonicalAcceptedControlTime, inspectCaptureFloat},
    FloatField{"canonical_feedback_production_delta", &CaptureInspection::canonicalProductionDelta, inspectCanonicalProductionDelta},
};

inline constexpr std::array kBoolFields{
    BoolField{"presentation_outcome_known", &CaptureInspection::outcomeKnown, inspectCaptureBool},
    BoolField{"presentation_confirmed", &CaptureInspection::presentationConfirmed, inspectCaptureBool},
};

inline constexpr auto kNumericFieldCount = kUnsignedFields.size() + kFloatFields.size() + kBoolFields.size();
static_assert(kNumericFieldCount <= std::numeric_limits<std::uint64_t>::digits,
              "capture numeric ambiguity must fit its bounded mask");

template<typename Fields>
inline bool parseNumericField(std::string_view key, std::string_view text,
                              CaptureInspection& report, std::uint64_t& seen,
                              const Fields& fields, std::size_t offset) {
    for (std::size_t index = 0; index < fields.size(); ++index) {
        const auto& field = fields[index];
        if (key != field.key) continue;
        const auto bit = std::uint64_t{1} << (offset + index);
        if (field.recorded) report.*field.recorded = true;
        if (seen & bit) {
            // Once ambiguous, later valid values cannot heal or overwrite it.
            report.ambiguousNumericFields |= bit;
            (report.*field.member).reset();
        } else {
            seen |= bit; // Even an invalid first declaration counts as present.
            report.*field.member = field.parse(text);
        }
        return true;
    }
    return false;
}

} // namespace capture_inspection_detail

// Synchronous diagnostic enumeration; names have static lifetime, and the raw
// bit positions are deliberately not an external JSON or capture-file contract.
template<typename Visitor>
inline void forEachAmbiguousCaptureField(const CaptureInspection& report, Visitor&& visitor) {
    std::size_t index = 0;
    const auto group = [&](const auto& fields) {
        for (const auto& field : fields) {
            const auto bit = std::uint64_t{1} << index++;
            if (report.ambiguousNumericFields & bit) visitor(field.key);
        }
    };
    group(capture_inspection_detail::kUnsignedFields);
    group(capture_inspection_detail::kFloatFields);
    group(capture_inspection_detail::kBoolFields);
}

[[nodiscard]] inline std::optional<CaptureInspection> inspectCaptureManifest(std::string_view text) {
    const auto manifestText = text;
    auto references = parseCaptureManifestReferences(text);
    if (!references) return std::nullopt;
    CaptureInspection result;
    result.references = std::move(*references);
    // Reuse the bounded decoder and declaration counts. A later valid value
    // cannot repair a duplicate, even if the first declaration was malformed.
    const auto& refs = result.references;
    if (refs.presetIdMetadataCount == 1) result.presetId = refs.presetIdMetadata;
    if (refs.presetRevisionMetadataCount == 1) result.presetRevision = refs.presetRevisionMetadata;
    std::uint64_t seenNumericFields = 0U;
    while (!text.empty()) {
        const auto newline = text.find('\n');
        const auto line = text.substr(0, newline);
        const auto equals = line.find('=');
        if (equals != std::string_view::npos) {
            const auto key = line.substr(0, equals);
            const auto value = line.substr(equals + 1);
            namespace fields = capture_inspection_detail;
            if (key == "format" && value == "epic-capture-bundle-v2") result.bundleV2Declared = true;
            else if (key == "png") result.png = value;
            else if (key == "epi") result.epi = value;
            else if (key == "profile") result.profile = value;
            else if (key == "causal") result.causal = value;
            else if (key == "canonical_feedback_mode") result.canonicalMode = value;
            else {
                // Reference/string policies stay separate; only the numeric
                // evidence inventory owns presence and sticky ambiguity here.
                static_cast<void>(fields::parseNumericField(key, value, result, seenNumericFields,
                        fields::kUnsignedFields, 0U)
                    || fields::parseNumericField(key, value, result, seenNumericFields,
                        fields::kFloatFields, fields::kUnsignedFields.size())
                    || fields::parseNumericField(key, value, result, seenNumericFields,
                        fields::kBoolFields, fields::kUnsignedFields.size() + fields::kFloatFields.size()));
            }
        }
        if (newline == std::string_view::npos) break;
        text.remove_prefix(newline + 1);
    }
    if (result.bundleV2Declared) result.bundleV2 = parseCaptureV2(manifestText);
    return result;
}

// These stable persisted ordinals mirror the value-only FrameOutcome contract
// already used by SculptureSerializer: 4 is SubmittedPresented, 0..8 are known.
// A missing code is allowed for legacy observations; an explicitly malformed or
// future code cannot establish a coherent interpretation of confirmation.
enum class CapturePresentationAssessment { Unknown, Inconsistent, Confirmed, NotConfirmed };

[[nodiscard]] inline const char* capturePresentationAssessmentCode(
    CapturePresentationAssessment value) noexcept {
    switch (value) {
    case CapturePresentationAssessment::Unknown: return "unknown";
    case CapturePresentationAssessment::Inconsistent: return "inconsistent";
    case CapturePresentationAssessment::Confirmed: return "confirmed";
    case CapturePresentationAssessment::NotConfirmed: return "not_confirmed";
    }
    return "unknown";
}

[[nodiscard]] inline const char* capturePresentationAssessmentDescription(
    CapturePresentationAssessment value) noexcept {
    switch (value) {
    case CapturePresentationAssessment::Unknown: return "unknown";
    case CapturePresentationAssessment::Inconsistent: return "inconsistent";
    case CapturePresentationAssessment::Confirmed: return "confirmed";
    case CapturePresentationAssessment::NotConfirmed: return "not confirmed";
    }
    return "unknown";
}

[[nodiscard]] inline CapturePresentationAssessment assessCapturePresentation(
    const CaptureInspection& report) noexcept {
    using Assessment = CapturePresentationAssessment;
    if (!report.outcomeKnown || !report.presentationConfirmed) return Assessment::Unknown;
    if (!*report.outcomeKnown)
        return *report.presentationConfirmed ? Assessment::Inconsistent : Assessment::Unknown;
    if (report.presentationOutcomeCodeRecorded) {
        if (!report.presentationOutcomeCode || *report.presentationOutcomeCode > 8U)
            return Assessment::Unknown;
        if (*report.presentationConfirmed && *report.presentationOutcomeCode != 4U)
            return Assessment::Inconsistent;
    }
    return *report.presentationConfirmed ? Assessment::Confirmed : Assessment::NotConfirmed;
}

// This compares two recorded values from one capture. Matching numbers do not
// authenticate scanout or prove that captures from different sessions match.
enum class CaptureContributionAssessment {
    Unknown, NoSelection, SelectedOnly, DisplayedOnly, Different,
    MatchingConfirmed, MatchingUnconfirmed,
};

[[nodiscard]] inline const char* captureContributionAssessmentCode(
    CaptureContributionAssessment value) noexcept {
    switch (value) {
    case CaptureContributionAssessment::Unknown: return "unknown";
    case CaptureContributionAssessment::NoSelection: return "no_selection";
    case CaptureContributionAssessment::SelectedOnly: return "selected_only";
    case CaptureContributionAssessment::DisplayedOnly: return "displayed_only";
    case CaptureContributionAssessment::Different: return "different";
    case CaptureContributionAssessment::MatchingConfirmed: return "matching_confirmed";
    case CaptureContributionAssessment::MatchingUnconfirmed: return "matching_unconfirmed";
    }
    return "unknown";
}

[[nodiscard]] inline const char* captureContributionAssessmentDescription(
    CaptureContributionAssessment value) noexcept {
    switch (value) {
    case CaptureContributionAssessment::Unknown: return "unknown";
    case CaptureContributionAssessment::NoSelection: return "no generation recorded";
    case CaptureContributionAssessment::SelectedOnly: return "selected; no displayed generation recorded";
    case CaptureContributionAssessment::DisplayedOnly: return "displayed generation only";
    case CaptureContributionAssessment::Different: return "recorded generations differ";
    case CaptureContributionAssessment::MatchingConfirmed:
        return "matching generations; presentation reported confirmed";
    case CaptureContributionAssessment::MatchingUnconfirmed:
        return "matching generations; confirmation not established";
    }
    return "unknown";
}

enum class CaptureContributionRoute : std::size_t {
    Compact = 0, World, Canonical, Implicit,
};

// Descriptor table unifying the 4 recorded capture contribution routes across
// passive assessment, JSON serialization, and single/comparison text reporting.
struct CaptureContributionRouteDescriptor {
    using Member = std::optional<std::uint64_t> CaptureInspection::*;
    CaptureContributionRoute route;
    const char* name;
    const char* selectedGenerationLabel;
    const char* displayedGenerationLabel;
    const char* assessmentLabel;
    const char* receiptSequenceLabel;
    Member selected;
    Member displayed;
    Member receipt;
};

inline constexpr std::array<CaptureContributionRouteDescriptor, 4> kCaptureContributionRoutes{{
    {CaptureContributionRoute::Compact, "compact",
     "selected compact generation", "displayed compact generation",
     "compact contribution assessment", "compact production receipt sequence",
     &CaptureInspection::selectedCompactGeneration,
     &CaptureInspection::displayedCompactGeneration,
     &CaptureInspection::selectedCompactReceiptSequence},
    {CaptureContributionRoute::World, "world",
     "selected world generation", "displayed world generation",
     "world contribution assessment", "world production receipt sequence",
     &CaptureInspection::selectedWorldGeneration,
     &CaptureInspection::displayedWorldGeneration,
     &CaptureInspection::selectedWorldReceiptSequence},
    {CaptureContributionRoute::Canonical, "canonical",
     "selected canonical generation", "displayed canonical generation",
     "canonical contribution assessment", "canonical production receipt sequence",
     &CaptureInspection::selectedCanonicalGeneration,
     &CaptureInspection::displayedCanonicalGeneration,
     &CaptureInspection::selectedCanonicalReceiptSequence},
    {CaptureContributionRoute::Implicit, "implicit",
     "selected implicit generation", "displayed implicit generation",
     "implicit contribution assessment", "implicit production receipt sequence",
     &CaptureInspection::selectedImplicitGeneration,
     &CaptureInspection::displayedImplicitGeneration,
     &CaptureInspection::selectedImplicitReceiptSequence},
}};

[[nodiscard]] constexpr const CaptureContributionRouteDescriptor*
captureContributionRouteDescriptor(CaptureContributionRoute route) noexcept {
    const auto index = static_cast<std::size_t>(route);
    return index < kCaptureContributionRoutes.size() ? &kCaptureContributionRoutes[index] : nullptr;
}

[[nodiscard]] inline CaptureContributionAssessment assessCaptureContribution(
    const CaptureInspection& report, std::optional<std::uint64_t> selected,
    std::optional<std::uint64_t> displayed) noexcept {
    using Assessment = CaptureContributionAssessment;
    if (!selected || !displayed) return Assessment::Unknown;
    if (*selected == 0 && *displayed == 0) return Assessment::NoSelection;
    if (*displayed == 0) return Assessment::SelectedOnly;
    if (*selected == 0) return Assessment::DisplayedOnly;
    if (*selected != *displayed) return Assessment::Different;
    return assessCapturePresentation(report) == CapturePresentationAssessment::Confirmed
        ? Assessment::MatchingConfirmed : Assessment::MatchingUnconfirmed;
}

[[nodiscard]] inline CaptureContributionAssessment assessCaptureRouteContribution(
    const CaptureInspection& report, CaptureContributionRoute route) noexcept {
    const auto* descriptor = captureContributionRouteDescriptor(route);
    return descriptor
        ? assessCaptureContribution(report, report.*(descriptor->selected), report.*(descriptor->displayed))
        : CaptureContributionAssessment::Unknown;
}

// Pure recorded-value consistency within one manifest's canonical v1 metadata.
// Matching numbers do not verify GPU field contents, scanout or exact replay.
enum class CanonicalRecordedIdentityAssessment {
    Unknown, Unavailable, Matching, Different,
};

[[nodiscard]] inline const char* canonicalRecordedIdentityAssessmentCode(
    CanonicalRecordedIdentityAssessment assessment) noexcept {
    using Assessment = CanonicalRecordedIdentityAssessment;
    switch (assessment) {
    case Assessment::Unknown: return "unknown";
    case Assessment::Unavailable: return "unavailable";
    case Assessment::Matching: return "matching";
    case Assessment::Different: return "different";
    }
    return "unknown";
}

[[nodiscard]] inline CanonicalRecordedIdentityAssessment assessCanonicalSelectionAndProducer(
    const CaptureInspection& report) noexcept {
    using Assessment = CanonicalRecordedIdentityAssessment;
    if (!report.canonicalCaptureVersion || *report.canonicalCaptureVersion != 1U
        || !report.selectedCanonicalGeneration || !report.selectedCanonicalResetEpoch
        || !report.canonicalFieldGeneration || !report.canonicalFieldResetEpoch) {
        return Assessment::Unknown;
    }
    if (*report.selectedCanonicalGeneration == 0U || *report.canonicalFieldGeneration == 0U) {
        return Assessment::Unavailable;
    }
    return (*report.selectedCanonicalGeneration == *report.canonicalFieldGeneration
            && *report.selectedCanonicalResetEpoch == *report.canonicalFieldResetEpoch)
        ? Assessment::Matching
        : Assessment::Different;
}

[[nodiscard]] inline CanonicalRecordedIdentityAssessment assessCanonicalSubmissionAndReadback(
    const CaptureInspection& report) noexcept {
    using Assessment = CanonicalRecordedIdentityAssessment;
    if (!report.canonicalCaptureVersion || *report.canonicalCaptureVersion != 1U
        || !report.canonicalSubmissionSerial || !report.canonicalReadbackSubmissionSerial) {
        return Assessment::Unknown;
    }
    if (*report.canonicalSubmissionSerial == 0U || *report.canonicalReadbackSubmissionSerial == 0U) {
        return Assessment::Unavailable;
    }
    return *report.canonicalSubmissionSerial == *report.canonicalReadbackSubmissionSerial
        ? Assessment::Matching
        : Assessment::Different;
}

// Joined canonical capture (SculptureStateRecorder::matchesWorldMode) binds the
// canonical production/readback serial to the frame's capture_submission_serial
// inside a valid non-zero presentation device/surface generation namespace.
[[nodiscard]] inline CanonicalRecordedIdentityAssessment assessCanonicalCaptureSubmissionJoin(
    const CaptureInspection& report) noexcept {
    using Assessment = CanonicalRecordedIdentityAssessment;
    if (assessCanonicalSubmissionAndReadback(report) != Assessment::Matching
        || !report.captureSubmissionSerial || !report.deviceGeneration || !report.surfaceGeneration) {
        return Assessment::Unknown;
    }
    if (*report.captureSubmissionSerial == 0U
        || *report.deviceGeneration == 0U || *report.surfaceGeneration == 0U) {
        return Assessment::Unavailable;
    }
    return *report.canonicalSubmissionSerial == *report.captureSubmissionSerial
        ? Assessment::Matching
        : Assessment::Different;
}

[[nodiscard]] inline std::optional<CaptureInspection> readCaptureInspection(const std::filesystem::path& path) {
    if (path.empty() || path.native().find(std::filesystem::path::value_type{})
            != std::filesystem::path::string_type::npos) return std::nullopt;
    try {
        return inspectCaptureManifest(epi::io::readBoundedText(path, kMaxCaptureManifestBytes, "capture manifest"));
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

} // namespace epi_sculpt
