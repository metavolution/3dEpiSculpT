#pragma once

#include "SculptureStereoRig.h"

#include <array>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <iomanip>
#include <limits>
#include <locale>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>

namespace epi_sculpt {

// Descriptive frozen input, not native projection/readback confirmation. The
// renderer may apply its own observation minima; these are requested values.
struct CaptureStereoIntent {
    bool enabled = false;
    SculptureStereoRig rig{}; // Meaningful only when stereo intent is enabled.

    [[nodiscard]] bool valid() const noexcept {
        return !enabled || (std::isfinite(rig.interpupillaryDistance)
            && rig.interpupillaryDistance >= 0.0f
            && std::isfinite(rig.focalConvergenceDistance) && rig.focalConvergenceDistance > 0.0f);
    }
};

enum class CaptureStereoProblem {
    None, DuplicateField, MissingField, UnsupportedVersion, InvalidBoolean,
    InvalidNumber, OutOfRange, UnexpectedMonoField,
};

enum class CaptureStereoField { None, Version, Enabled, Ipd, Convergence, Converged };

[[nodiscard]] constexpr const char* captureStereoFieldName(CaptureStereoField field) noexcept {
    switch (field) {
    case CaptureStereoField::None: return nullptr;
    case CaptureStereoField::Version: return "stereo_intent_version";
    case CaptureStereoField::Enabled: return "stereo_intent_enabled";
    case CaptureStereoField::Ipd: return "stereo_intent_ipd_m";
    case CaptureStereoField::Convergence: return "stereo_intent_convergence_m";
    case CaptureStereoField::Converged: return "stereo_intent_converged";
    }
    return nullptr;
}

struct CaptureStereoMetadata {
    bool recorded = false;
    std::optional<CaptureStereoIntent> intent;
    // One bounded explanation of the parser's rejection; never a text borrow or
    // an independent admission policy. Consumers still use value() for validity.
    CaptureStereoProblem problem = CaptureStereoProblem::None;
    CaptureStereoField problemField = CaptureStereoField::None;

    [[nodiscard]] const CaptureStereoIntent* value() const noexcept {
        return recorded && intent && intent->valid() ? &*intent : nullptr;
    }
};

[[nodiscard]] inline const char* captureStereoMetadataCode(const CaptureStereoMetadata& metadata) noexcept {
    return !metadata.recorded ? "not_recorded" : metadata.value() ? "recorded" : "invalid";
}

// Null means no diagnostic applies (valid input or legacy absence). A manually
// assembled invalid value without parser details receives the generic fallback.
[[nodiscard]] inline const char* captureStereoDiagnosticCode(const CaptureStereoMetadata& metadata) noexcept {
    if (!metadata.recorded || metadata.value()) return nullptr;
    switch (metadata.problem) {
    case CaptureStereoProblem::None: return "invalid_value";
    case CaptureStereoProblem::DuplicateField: return "duplicate_field";
    case CaptureStereoProblem::MissingField: return "missing_field";
    case CaptureStereoProblem::UnsupportedVersion: return "unsupported_version";
    case CaptureStereoProblem::InvalidBoolean: return "invalid_boolean";
    case CaptureStereoProblem::InvalidNumber: return "invalid_number";
    case CaptureStereoProblem::OutOfRange: return "out_of_range";
    case CaptureStereoProblem::UnexpectedMonoField: return "unexpected_mono_field";
    }
    return "invalid_value";
}

enum class CaptureStereoChange { NotComparable, NotApplicable, Same, Different };

[[nodiscard]] inline const char* captureStereoChangeCode(CaptureStereoChange change) noexcept {
    switch (change) {
    case CaptureStereoChange::NotComparable: return "not_comparable";
    case CaptureStereoChange::NotApplicable: return "not_applicable";
    case CaptureStereoChange::Same: return "same";
    case CaptureStereoChange::Different: return "different";
    }
    return "not_comparable";
}

// A derived value comparison, with no resources, IO or restoration authority.
// Exact numerical float32 equality is intentional: signed zeros compare equal;
// this describes requested configuration, not whether two images look alike.
struct CaptureStereoComparison {
    CaptureStereoChange configuration = CaptureStereoChange::NotComparable;
    CaptureStereoChange enabled = CaptureStereoChange::NotComparable;
    CaptureStereoChange converged = CaptureStereoChange::NotComparable;
    CaptureStereoChange ipd = CaptureStereoChange::NotComparable;
    CaptureStereoChange convergenceDistance = CaptureStereoChange::NotComparable;
    std::optional<double> ipdDeltaMeters;
    std::optional<double> convergenceDeltaMeters;
};

[[nodiscard]] inline CaptureStereoComparison compareCaptureStereoIntent(
    const CaptureStereoMetadata& first, const CaptureStereoMetadata& second) noexcept {
    using Change = CaptureStereoChange;
    CaptureStereoComparison result;
    const auto* a = first.value();
    const auto* b = second.value();
    if (!a || !b) return result;
    result.enabled = a->enabled == b->enabled ? Change::Same : Change::Different;
    if (!a->enabled || !b->enabled) {
        result.configuration = result.enabled;
        result.converged = result.ipd = result.convergenceDistance = Change::NotApplicable;
        return result;
    }
    result.converged = a->rig.toeInConvergence == b->rig.toeInConvergence ? Change::Same : Change::Different;
    result.ipd = a->rig.interpupillaryDistance == b->rig.interpupillaryDistance ? Change::Same : Change::Different;
    result.convergenceDistance = a->rig.focalConvergenceDistance == b->rig.focalConvergenceDistance
        ? Change::Same : Change::Different;
    result.configuration = result.converged == Change::Same && result.ipd == Change::Same &&
        result.convergenceDistance == Change::Same ? Change::Same : Change::Different;
    // Widen before subtraction to avoid an additional float32 rounding step.
    // These deltas neither clamp nor apply the recorded observation settings.
    result.ipdDeltaMeters = double(b->rig.interpupillaryDistance) - double(a->rig.interpupillaryDistance);
    result.convergenceDeltaMeters = double(b->rig.focalConvergenceDistance) - double(a->rig.focalConvergenceDistance);
    return result;
}

// Prepare the entire optional block before appending it to a bounded manifest.
// Mono intent carries no fabricated eye parameters; legacy absence stays unknown.
[[nodiscard]] inline std::optional<std::string> serializeCaptureStereoIntent(const CaptureStereoIntent& intent) {
    if (!intent.valid()) return std::nullopt;
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << "stereo_intent_version=1\nstereo_intent_enabled=" << (intent.enabled ? 1 : 0) << '\n';
    if (intent.enabled) {
        out << std::setprecision(std::numeric_limits<float>::max_digits10)
            << "stereo_intent_ipd_m=" << intent.rig.interpupillaryDistance << '\n'
            << "stereo_intent_convergence_m=" << intent.rig.focalConvergenceDistance << '\n'
            << "stereo_intent_converged=" << (intent.rig.toeInConvergence ? 1 : 0) << '\n';
    }
    if (!out) return std::nullopt;
    return out.str();
}

// Called after complete manifest text admission. Duplicate, incomplete, future
// version or malformed declarations cannot become last-key-wins optical facts.
// This parser grants no restore authority and performs no IO. It retains only
// plain values; field views borrow the caller's admitted text during parsing.
[[nodiscard]] inline CaptureStereoMetadata parseCaptureStereoMetadata(std::string_view text) noexcept {
    constexpr std::array fieldIds{CaptureStereoField::Version, CaptureStereoField::Enabled,
        CaptureStereoField::Ipd, CaptureStereoField::Convergence, CaptureStereoField::Converged};
    struct Field { std::string_view value; std::size_t count = 0; };
    std::array<Field, fieldIds.size()> fields{};
    CaptureStereoMetadata result;
    const auto reject = [&](CaptureStereoProblem problem, std::size_t index) noexcept {
        result.problem = problem;
        result.problemField = fieldIds[index];
        return result;
    };
    while (!text.empty()) {
        const auto newline = text.find('\n');
        const auto line = text.substr(0, newline);
        const auto equals = line.find('=');
        if (equals != std::string_view::npos) {
            for (std::size_t index = 0; index < fieldIds.size(); ++index) {
                if (line.substr(0, equals) != captureStereoFieldName(fieldIds[index])) continue;
                result.recorded = true;
                fields[index].value = line.substr(equals + 1U);
                ++fields[index].count;
                break;
            }
        }
        if (newline == std::string_view::npos) break;
        text.remove_prefix(newline + 1U);
    }
    if (!result.recorded) return result;
    for (std::size_t index = 0; index < fields.size(); ++index)
        if (fields[index].count > 1U) return reject(CaptureStereoProblem::DuplicateField, index);
    if (fields[0].count != 1U) return reject(CaptureStereoProblem::MissingField, 0);
    if (fields[0].value != "1") return reject(CaptureStereoProblem::UnsupportedVersion, 0);
    if (fields[1].count != 1U) return reject(CaptureStereoProblem::MissingField, 1);
    const auto boolean = [](std::string_view value) -> std::optional<bool> {
        if (value == "0") return false;
        if (value == "1") return true;
        return std::nullopt;
    };
    const auto enabled = boolean(fields[1].value);
    if (!enabled) return reject(CaptureStereoProblem::InvalidBoolean, 1);
    if (!*enabled) {
        for (std::size_t index = 2; index < fields.size(); ++index)
            if (fields[index].count != 0U) return reject(CaptureStereoProblem::UnexpectedMonoField, index);
        result.intent = CaptureStereoIntent{};
        return result;
    }
    for (std::size_t index = 2; index < fields.size(); ++index)
        if (fields[index].count != 1U) return reject(CaptureStereoProblem::MissingField, index);
    const auto scalar = [](std::string_view value) -> std::optional<float> {
        if (value.empty()) return std::nullopt;
        float number = 0.0f;
        const auto parsed = std::from_chars(value.data(), value.data() + value.size(), number);
        if (parsed.ec != std::errc{} || parsed.ptr != value.data() + value.size() || !std::isfinite(number))
            return std::nullopt;
        return number;
    };
    const auto ipd = scalar(fields[2].value);
    if (!ipd) return reject(CaptureStereoProblem::InvalidNumber, 2);
    const auto convergence = scalar(fields[3].value);
    if (!convergence) return reject(CaptureStereoProblem::InvalidNumber, 3);
    const auto converged = boolean(fields[4].value);
    if (!converged) return reject(CaptureStereoProblem::InvalidBoolean, 4);
    if (*ipd < 0.0f) return reject(CaptureStereoProblem::OutOfRange, 2);
    if (*convergence <= 0.0f) return reject(CaptureStereoProblem::OutOfRange, 3);
    const CaptureStereoIntent intent{true, {*ipd, *convergence, *converged}};
    if (intent.valid()) result.intent = intent;
    return result;
}

} // namespace epi_sculpt
