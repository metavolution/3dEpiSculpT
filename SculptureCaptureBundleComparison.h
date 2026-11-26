#pragma once

#include "SculptureCaptureInspection.h"

#include <array>
#include <optional>

namespace epi_sculpt {

// Compare declarations, never files. Equal declared size/digest is useful for
// capture triage, but does not authenticate bytes, a simulation or exact replay.
enum class CaptureBundleContentChange { NotComparable, Absent, Added, Removed, Same, Different };

[[nodiscard]] inline const char* captureBundleContentChangeCode(CaptureBundleContentChange value) noexcept {
    switch (value) {
    case CaptureBundleContentChange::NotComparable: return "not_comparable";
    case CaptureBundleContentChange::Absent: return "absent";
    case CaptureBundleContentChange::Added: return "added";
    case CaptureBundleContentChange::Removed: return "removed";
    case CaptureBundleContentChange::Same: return "same_declared_content";
    case CaptureBundleContentChange::Different: return "changed_declared_content";
    }
    return "not_comparable";
}

[[nodiscard]] inline const CaptureV2Document* inspectedBundleV2(const CaptureInspection& report) noexcept {
    return report.bundleV2Declared && report.bundleV2 ? &*report.bundleV2 : nullptr;
}

[[nodiscard]] inline const char* captureBundleComparisonInputCode(const CaptureInspection& report) noexcept {
    if (!report.bundleV2Declared) return "not_v2";
    return inspectedBundleV2(report) ? "declared_v2" : "invalid_v2";
}

struct CaptureBundleAttachmentComparison {
    CaptureBundleContentChange content = CaptureBundleContentChange::NotComparable;
    std::optional<bool> referenceNameChanged;
};

struct CaptureBundleDeclarationComparison {
    CaptureBundleContentChange attachmentContent = CaptureBundleContentChange::NotComparable;
    // Generation labels and raster extents are separate facts. In particular,
    // renaming a capture generation does not change declared attachment bytes.
    std::optional<bool> generationLabelChanged;
    std::optional<bool> extentChanged;
    std::array<CaptureBundleAttachmentComparison, kCaptureV2Roles> attachments{};
};

[[nodiscard]] inline CaptureBundleDeclarationComparison compareCaptureBundleDeclarations(
    const CaptureInspection& first, const CaptureInspection& second) noexcept {
    CaptureBundleDeclarationComparison result;
    const auto* a = inspectedBundleV2(first);
    const auto* b = inspectedBundleV2(second);
    // An invalid/older bundle is unknown, not an apparently empty attachment set.
    if (!a || !b) return result;
    using Change = CaptureBundleContentChange;
    result.attachmentContent = Change::Same;
    result.generationLabelChanged = a->generation != b->generation;
    result.extentChanged = a->width != b->width || a->height != b->height;
    for (std::size_t i = 0; i < kCaptureV2Roles; ++i) {
        const auto& left = a->entries[i];
        const auto& right = b->entries[i];
        auto& row = result.attachments[i];
        if (!left.present && !right.present) row.content = Change::Absent;
        else if (!left.present) row.content = Change::Added;
        else if (!right.present) row.content = Change::Removed;
        else {
            row.referenceNameChanged = left.name != right.name;
            row.content = left.size == right.size && left.sha256 == right.sha256 ? Change::Same : Change::Different;
        }
        if (row.content == Change::Added || row.content == Change::Removed || row.content == Change::Different)
            result.attachmentContent = Change::Different;
    }
    return result;
}

} // namespace epi_sculpt
