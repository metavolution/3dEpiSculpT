// Application-owned live controls over the resource-free observation contract.
#pragma once
#include "SculptureReliefContourSettings.h"
#include <array>
#include <optional>

namespace epi_sculpt {
enum class ReliefContourAction { Toggle, Coarser, Finer, Inherit, CycleStyle };

// Application-owned observation controls. The authored baseline is refreshed
// during composition; input actions use that last composed value, even when
// several commands arrive before the next frame. No simulation clock is owned.
class SculptureReliefContourControl {
public:
    void setOverride(std::optional<float> density) noexcept {
        override_ = density ? std::optional<float>(resolveReliefContourDensity(*density))
                            : std::nullopt;
        remember(value());
    }

    [[nodiscard]] bool hasOverride() const noexcept {
        return override_.has_value() || styleOverride_.has_value();
    }
    [[nodiscard]] std::uint32_t style() const noexcept { return resolve(authored_).style; }
    [[nodiscard]] float value() const noexcept { return resolve(authored_).density; }
    [[nodiscard]] ReliefContourSettings resolve(ReliefContourSettings authored) const noexcept {
        const auto admitted = resolveReliefContourSettings(authored);
        return {override_.value_or(admitted.density), styleOverride_.value_or(admitted.style)};
    }
    ReliefContourSettings compose(ReliefContourSettings authored) noexcept {
        // Refresh both authored channels at the same owner-thread boundary.
        // Const save-time resolution must not replace this live input baseline.
        authored_ = resolveReliefContourSettings(authored);
        const auto selected = resolve(authored_);
        remember(selected.density);
        return selected;
    }

    void apply(ReliefContourAction action) noexcept {
        const float current = value();
        switch (action) {
        case ReliefContourAction::Inherit:
            override_.reset();
            styleOverride_.reset();
            remember(value());
            return;
        case ReliefContourAction::CycleStyle:
            styleOverride_ = static_cast<std::uint32_t>((style() + 1U) % kReliefContourStyleNames.size());
            if (current == 0.0f) setOverride(remembered_);
            return;
        case ReliefContourAction::Toggle:
            remember(current);
            setOverride(current > 0.0f ? 0.0f : remembered_);
            return;
        case ReliefContourAction::Finer:
            if (current == 0.0f) { setOverride(remembered_); return; }
            for (float density : kDensities) {
                if (density > current) { setOverride(density); return; }
            }
            return; // Saturation does not pin an inherited authored value.
        case ReliefContourAction::Coarser:
            if (current == 0.0f) return;
            remember(current);
            for (auto it = kDensities.rbegin(); it != kDensities.rend(); ++it) {
                if (*it < current) { setOverride(*it); return; }
            }
            setOverride(0.0f);
            return;
        }
    }

private:
    inline static constexpr std::array<float,8> kDensities{4,8,12,16,24,32,48,64};
    void remember(float density) noexcept {
        if (density > 0.0f) remembered_ = density;
    }
    std::optional<float> override_;
    std::optional<std::uint32_t> styleOverride_;
    ReliefContourSettings authored_{};
    float remembered_ = 12.0f;
};
} // namespace epi_sculpt
