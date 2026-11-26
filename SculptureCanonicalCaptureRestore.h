#pragma once

#include "SculptureCaptureManifest.h"
#include "epi/backend/seed_image_decode_status.hpp"
#include "epi/backend/seed_image_input.hpp"
#include "epi/mode/pattern_mirror.hpp"
#include "epi/mode/visual_mode.hpp"

#include <cmath>
#include <exception>
#include <filesystem>
#include <initializer_list>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace epi_sculpt {

namespace canonical_capture_detail {
// Producer-facing scalar fields serialized by the v1 writer. Use the codec to
// fill sentinel values: a truncated/empty document must not turn into defaults.
// This is admission of decoded values, not another text parser.
inline constexpr float epi::VisualMode::* kMathScalars[] = {
    &epi::VisualMode::zn0_anim_speed, &epi::VisualMode::zn0_anim_amp,
    &epi::VisualMode::zn1_anim_speed, &epi::VisualMode::zn1_anim_amp, &epi::VisualMode::zn1_anim_k,
    &epi::VisualMode::hue_rotation, &epi::VisualMode::sat_scale, &epi::VisualMode::fade,
    &epi::VisualMode::hue_speed, &epi::VisualMode::distance_scale, &epi::VisualMode::distance_amount,
    &epi::VisualMode::hsls_axis1, &epi::VisualMode::hsls_axis2,
    &epi::VisualMode::spiral_angle_coeff, &epi::VisualMode::spiral_radius_coeff,
    &epi::VisualMode::chroma_breath_amp, &epi::VisualMode::chroma_breath_freq,
    &epi::VisualMode::chroma_breath_phase, &epi::VisualMode::hue_velocity_base,
    &epi::VisualMode::hue_velocity_breath, &epi::VisualMode::hue_velocity_drift,
    &epi::VisualMode::luma_shape_depth, &epi::VisualMode::luma_shape_center,
    &epi::VisualMode::seed_border_width, &epi::VisualMode::seed_hue,
    &epi::VisualMode::seed_saturation, &epi::VisualMode::seed_alpha, &epi::VisualMode::seed_hue_speed,
    &epi::VisualMode::seed_pulse_freq, &epi::VisualMode::seed_pulse_amp,
    &epi::VisualMode::speed, &epi::VisualMode::t_delta, &epi::VisualMode::time_speed,
};

// Resolve in the capture's document context, then pin the selected file in the
// owned candidate. An optional encoded owner freezes the admitted bytes.
// An optional declared sibling selects exact document IO; absence keeps normal
// resource search. The cold owner resolves/validates privately and commits path
// and input only on success. The backend retains cache/float/upload ownership.
[[nodiscard]] epi::backend::seed_image_runtime::SeedImageDecodeStatus prepareSeedImageResource(
    epi::VisualMode& mode, const std::filesystem::path& manifest,
    epi::SeedImageInput::Owner* retained = nullptr,
    epi::SeedImageRetentionBudget* retentionBudget = nullptr,
    std::optional<std::string_view> documentImageSibling = std::nullopt);
} // namespace canonical_capture_detail

enum class CanonicalCaptureRestoreFailure {
    None, DeclarationRejected, AcquisitionFailed, InvalidInput, ImageSeedUnavailable,
};

struct CanonicalCaptureRestorePreparation {
    bool admitted = false;
    // Owned semantic/encoded input. No recursive field contents, clocks,
    // generation counters or native handles are restored by this preparation.
    std::optional<epi::VisualMode> mode;
    CanonicalCaptureRestoreFailure failure = CanonicalCaptureRestoreFailure::DeclarationRejected;
    CanonicalCaptureDeclarationAssessment declaration = CanonicalCaptureDeclarationAssessment::RejectedManifest;
    std::optional<epi::backend::seed_image_runtime::SeedImageDecodeStatus> imageDecodeStatus = std::nullopt;
    epi::SeedImageInput::Owner seedImageInput{};

    [[nodiscard]] const char* diagnosticCode() const noexcept {
        switch (failure) {
        case CanonicalCaptureRestoreFailure::None:
            return mode ? "prepared" : "not_recorded";
        case CanonicalCaptureRestoreFailure::DeclarationRejected:
            return canonicalCaptureDeclarationCode(declaration);
        case CanonicalCaptureRestoreFailure::AcquisitionFailed: return "input_acquisition_failed";
        case CanonicalCaptureRestoreFailure::InvalidInput: return "invalid_input";
        case CanonicalCaptureRestoreFailure::ImageSeedUnavailable: return "image_seed_unavailable";
        }
        return "invalid_input";
    }
};

// Decode the acquired snapshot, never a path. Unknown fields retain the shared
// codec's evolution policy. Missing producer values cannot silently become
// defaults when a JSON/projection file or truncated text is supplied instead.
[[nodiscard]] inline std::optional<epi::VisualMode> decodeCanonicalCaptureInput(
    const std::string& snapshot) {
    if (snapshot.empty() || snapshot.size() > kMaxCanonicalCaptureModeBytes
        || snapshot.find('\0') != std::string::npos) return std::nullopt;
    try {
        epi::VisualMode missing;
        const float nan = std::numeric_limits<float>::quiet_NaN();
        for (const auto member : canonical_capture_detail::kMathScalars) missing.*member = nan;
        missing.zn0 = missing.zn1 = missing.zn2 = missing.zn3 = {nan, nan};
        missing.kernel_dim = 0;
        missing.inner = static_cast<epi::InnerFunction>(-1);
        missing.reduction = static_cast<epi::ReductionType>(-1);
        missing.color_mode = missing.seed_type = -1;
        auto mode = epi::VisualMode::deserialize_strict(snapshot, &missing);
        // Admission of producer math, not a new parameter interpreter or a live
        // resource resize. Authored kernel_dim is not the captured field extent.
        if (mode.kernel_dim <= 0 || mode.speed <= 0.0f || mode.time_speed < 0.0f
            || mode.inner == missing.inner || mode.reduction == missing.reduction
            || mode.color_mode < 0 || mode.seed_type < 0)
            return std::nullopt;
        for (const auto z : {mode.zn0, mode.zn1, mode.zn2, mode.zn3}) {
            if (!std::isfinite(z.re) || !std::isfinite(z.im)) return std::nullopt;
        }
        for (const auto member : canonical_capture_detail::kMathScalars) {
            if (!std::isfinite(mode.*member)) return std::nullopt;
        }
        return mode;
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

[[nodiscard]] inline CanonicalCaptureRestorePreparation prepareCanonicalCaptureRestore(
    const CaptureManifestReferences& refs, const std::filesystem::path& manifest,
    epi::SeedImageRetentionBudget* retentionBudget = nullptr) {
    // Absence is supported: ordinary configuration captures have no canonical
    // input packet. An advertised packet must be complete and unambiguous.
    using Declaration = CanonicalCaptureDeclarationAssessment;
    using Failure = CanonicalCaptureRestoreFailure;
    const auto declaration = assessCanonicalCaptureDeclaration(refs);
    if (declaration == Declaration::NotRecorded)
        return {true, std::nullopt, Failure::None, declaration};
    if (declaration != Declaration::DeclaredV1)
        return {false, std::nullopt, Failure::DeclarationRejected, declaration};
    std::string snapshot;
    try {
        snapshot = epi::io::readBoundedText(
            manifest.parent_path() / refs.canonicalMode, kMaxCanonicalCaptureModeBytes,
            "canonical capture input");
    } catch (const std::exception&) {
        return {false, std::nullopt, Failure::AcquisitionFailed, declaration};
    }
    auto mode = decodeCanonicalCaptureInput(snapshot);
    if (!mode) return {false, std::nullopt, Failure::InvalidInput, declaration};
    if (refs.canonicalSeedImageCount != 0) {
        // Admission owns semantic binding; the cold image owner owns all path
        // resolution/acquisition. Keep this candidate untouched until success.
        if (epi::pattern_mirror::seed_type_from_mode_int(mode->seed_type) != epi::SeedType::Image
            || mode->seed_image_path != refs.canonicalSeedImage)
            return {false, std::nullopt, Failure::InvalidInput, declaration};
    }
    epi::SeedImageInput::Owner imageInput;
    const auto imageStatus = canonical_capture_detail::prepareSeedImageResource(
        *mode, manifest, &imageInput, retentionBudget,
        refs.canonicalSeedImageCount != 0
            ? std::optional<std::string_view>{refs.canonicalSeedImage} : std::nullopt);
    if (imageStatus != epi::backend::seed_image_runtime::SeedImageDecodeStatus::Decoded)
        return {false, std::nullopt, Failure::ImageSeedUnavailable, declaration, imageStatus};
    return {true, std::move(mode), Failure::None, declaration, std::nullopt, std::move(imageInput)};
}

// Decode the retained captured PNG with a bounded output allocation.
[[nodiscard]] bool validateCaptureV2Png(std::string_view bytes, std::uint32_t expectedWidth,
                                        std::uint32_t expectedHeight) noexcept;

// V2 uses only bytes retained by the bundle acquisition; it must not reopen a
// path after hash validation. Image decoding and reservation remain the seed owner.
[[nodiscard]] CanonicalCaptureRestorePreparation prepareCanonicalCaptureRestoreV2(
    const CaptureManifestReferences& refs, std::string_view modeBytes,
    std::string_view seedBytes, const std::filesystem::path& manifest,
    epi::SeedImageRetentionBudget* retentionBudget = nullptr);

} // namespace epi_sculpt
