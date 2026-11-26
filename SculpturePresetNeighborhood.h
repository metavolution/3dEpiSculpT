#pragma once

#include "SculptureParameterRegistry.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

namespace epi_sculpt {

namespace preset_neighborhood_detail {

// One ordered vocabulary for metric eligibility and educational explanations.
// Returning false stops at the first field; report observers may visit them all.
template<class Observer>
[[nodiscard]] inline bool visitSelectors(
    const SculptureProgramParams& a, const SculptureProgramParams& b, Observer&& observe) {
    const auto field = [&](std::string_view name, auto x, auto y) {
        return observe(name, static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(y));
    };
    if (!field("inner", a.inner, b.inner) || !field("innerB", a.innerB, b.innerB)
        || !field("reduction", a.reduction, b.reduction)
        || !field("reductionB", a.reductionB, b.reductionB)
        || !field("seedType", a.seedType, b.seedType)
        || !field("seedTypeB", a.seedTypeB, b.seedTypeB)
        || !field("colorEffect", a.colorEffect, b.colorEffect)
        || !field("auxiliaryResolutionScale", a.auxiliaryResolutionScale, b.auxiliaryResolutionScale)
        || !field("temporalReset", a.temporalReset, b.temporalReset)) return false;
    for (const auto& parameter : kUintParams) {
        if (parameter.member != &SculptureProgramParams::feedbackIterations
            && !field(parameter.name, a.*parameter.member, b.*parameter.member)) return false;
    }
    return true;
}

} // namespace preset_neighborhood_detail

// One selector equality policy for admission and distance/report consumers.
// Numeric values (including iteration count) retain their independent policy.
[[nodiscard]] inline bool sameSpatialPresetSelectors(
    const SculptureProgramParams& a, const SculptureProgramParams& b) noexcept {
    return preset_neighborhood_detail::visitSelectors(a, b,
        [](std::string_view, std::uint32_t x, std::uint32_t y) noexcept { return x == y; });
}

namespace preset_neighborhood_detail {

// Enumerate the metric's numeric domain once. Each reduction decides whether to
// continue; finiteness guards need neither selector equality nor score arithmetic.
template<class Observer>
[[nodiscard]] inline bool visitNumericComponents(
    const SculptureProgramParams& a, const SculptureProgramParams& b, Observer&& observe) {
    for (const auto& field : kFloatParams) {
        if (!observe(field.name, static_cast<double>(a.*field.member),
                     static_cast<double>(b.*field.member), std::nullopt)) return false;
    }
    constexpr std::array affines{&SculptureProgramParams::pre, &SculptureProgramParams::post};
    constexpr std::array components{&ComplexAffine::scaleReal, &ComplexAffine::scaleImag,
                                     &ComplexAffine::offsetReal, &ComplexAffine::offsetImag};
    constexpr std::array names{
        "pre.scaleReal", "pre.scaleImag", "pre.offsetReal", "pre.offsetImag",
        "post.scaleReal", "post.scaleImag", "post.offsetReal", "post.offsetImag"};
    std::size_t affineIndex = 0;
    for (const auto affine : affines) {
        for (const auto component : components) {
            if (!observe(names[affineIndex++], static_cast<double>((a.*affine).*component),
                         static_cast<double>((b.*affine).*component), std::nullopt)) return false;
        }
    }
    for (std::size_t i = 0; i < a.audioSpectrum.size(); ++i) {
        if (!observe("audioSpectrum", static_cast<double>(a.audioSpectrum[i]),
                     static_cast<double>(b.audioSpectrum[i]), i)) return false;
    }
    return observe("feedbackIterations", static_cast<double>(a.feedbackIterations),
                   static_cast<double>(b.feedbackIterations), std::nullopt);
}

// One scoring reduction serves the fast scalar query and its educational report.
// Observers used here do not throw; all component order/names come from the visitor.
template<class Observer>
[[nodiscard]] inline std::optional<double> visitDistance(
    const SculptureProgramParams& a, const SculptureProgramParams& b,
    Observer&& observe) noexcept {
    if (!sameSpatialPresetSelectors(a, b)) return std::nullopt;

    double sum = 0.0;
    std::size_t count = 0;
    const auto add = [&](std::string_view name, double x, double y,
                         std::optional<std::size_t> component) noexcept {
        if (!std::isfinite(x) || !std::isfinite(y)) return false;
        // Symmetric relative L1: a large-unit parameter (e.g. film nanometers)
        // must not dominate merely because of its units. Double intermediates
        // also keep subtraction safe for opposite finite float extremes.
        const auto normalized = std::abs(x - y) / (1.0 + std::max(std::abs(x), std::abs(y)));
        sum += normalized;
        ++count;
        observe(name, component, x, y, normalized);
        return true;
    };
    if (!visitNumericComponents(a, b, add)) return std::nullopt;
    return sum / static_cast<double>(count);
}

} // namespace preset_neighborhood_detail

// Finiteness over exactly the metric's numeric components, independent of discrete
// selectors. This validates neither the whole schema nor native/canonical state.
[[nodiscard]] inline bool spatialPresetParametersFinite(const SculptureProgramParams& params) noexcept {
    return preset_neighborhood_detail::visitNumericComponents(params, params,
        [](std::string_view, double value, double, std::optional<std::size_t>) noexcept {
            return std::isfinite(value);
        });
}

[[nodiscard]] inline bool spatialPresetParametersFinite(
    const SculptureProgramParams& a, const SculptureProgramParams& b) noexcept {
    return preset_neighborhood_detail::visitNumericComponents(a, b,
        [](std::string_view, double x, double y, std::optional<std::size_t>) noexcept {
            return std::isfinite(x) && std::isfinite(y);
        });
}

// Inspection heuristic over decoded spatial parameters, not canonical fields or
// a proof that two presets can share a live morph/producer transaction.
[[nodiscard]] inline std::optional<double> spatialPresetDistance(
    const SculptureProgramParams& a, const SculptureProgramParams& b) noexcept {
    return preset_neighborhood_detail::visitDistance(a, b,
        [](std::string_view, std::optional<std::size_t>, double, double, double) noexcept {});
}

struct SpatialPresetDifference {
    std::string_view parameter; // Stable registry/literal name, never input storage.
    std::optional<std::size_t> component; // Array index, when present.
    double from = 0.0;
    double to = 0.0;
    double contribution = 0.0; // Contribution to the mean, not a percentage.
};

struct SpatialPresetDistanceReport {
    double distance = 0.0;
    std::size_t componentCount = 0;
    std::size_t changedCount = 0;
    std::array<SpatialPresetDifference, 3> differences{};
    std::size_t differenceCount = 0;
};

[[nodiscard]] inline std::optional<SpatialPresetDistanceReport> inspectSpatialPresetDistance(
    const SculptureProgramParams& a, const SculptureProgramParams& b) noexcept {
    SpatialPresetDistanceReport report;
    const auto distance = preset_neighborhood_detail::visitDistance(a, b,
        [&](std::string_view name, std::optional<std::size_t> component,
            double from, double to, double normalized) noexcept {
            ++report.componentCount;
            if (normalized == 0.0) return;
            ++report.changedCount;
            std::size_t position = 0;
            while (position < report.differenceCount
                   && report.differences[position].contribution >= normalized) ++position;
            if (position == report.differences.size()) return;
            const auto last = std::min(report.differenceCount, report.differences.size() - 1);
            for (auto index = last; index > position; --index)
                report.differences[index] = report.differences[index - 1];
            report.differences[position] = {name, component, from, to, normalized};
            report.differenceCount = std::min(report.differenceCount + 1, report.differences.size());
        });
    if (!distance) return std::nullopt;
    report.distance = *distance;
    for (std::size_t i = 0; i < report.differenceCount; ++i)
        report.differences[i].contribution /= static_cast<double>(report.componentCount);
    return report;
}

} // namespace epi_sculpt
