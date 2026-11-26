#pragma once

#include <cstdint>
#include <string_view>

#include <epi/core/complex.hpp>

namespace epi_sculpt {

// -----------------------------------------------------------------------------
// Educational Annotation: Fractal Catalog
// -----------------------------------------------------------------------------
// Consolidates all dynamic geometry definitions into a single source of truth.
// This prevents drift between the OpenCL string generators and the CPU fallback
// implementations, simplifying the process of adding new mathematical formulas
// to a single unified registry.
class SculptureFractalCatalog {
public:
    static std::string_view getInnerName(std::uint32_t id) noexcept;
    static std::string_view getInnerClCode(std::uint32_t id) noexcept;
    
    static std::string_view getSeedName(std::uint32_t id) noexcept;
    static std::string_view getSeedClCode(std::uint32_t id) noexcept;

    static std::string_view getReductionName(std::uint32_t id) noexcept;
    static std::string_view getReductionClCode(std::uint32_t id) noexcept;

    static std::string_view getColorEffectName(std::uint32_t id) noexcept;
    static std::string_view getColorEffectClCode(std::uint32_t id) noexcept;

    [[nodiscard]] static epi::Complexf evaluateHomotopy(epi::Complexf zA, epi::Complexf zB, epi::Complexf zOrig, float t) noexcept;
};

} // namespace epi_sculpt
