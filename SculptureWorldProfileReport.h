// Renderer-neutral reader and comparator for immutable world profile sidecars.
// It deliberately consumes only completed evidence; it never changes quality,
// generator parameters, or GPU ownership.
#pragma once

#include <cmath>
#include <fstream>
#include <iterator>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace epi_sculpt {

struct WorldProfileRecord {
    std::map<std::string, std::string, std::less<>> fields;

    [[nodiscard]] std::string_view value(std::string_view key) const noexcept {
        const auto found = fields.find(key);
        return found == fields.end() ? std::string_view{} : found->second;
    }

    [[nodiscard]] std::optional<double> milliseconds(std::string_view key) const {
        const std::string_view text = value(key);
        if (text.empty()) return std::nullopt;
        try {
            std::size_t consumed = 0U;
            const double parsed = std::stod(std::string(text), &consumed);
            return consumed == text.size() && std::isfinite(parsed) && parsed >= 0.0
                ? std::optional<double>(parsed) : std::nullopt;
        } catch (...) {
            return std::nullopt;
        }
    }
};

struct WorldProfileComparison {
    bool comparable = false;
    std::vector<std::string> incompatibilities;
    double kernelDeltaMilliseconds = 0.0;
    double readbackDeltaMilliseconds = 0.0;
    double bridgeDeltaMilliseconds = 0.0;
    double presentationSubmitCpuDeltaMilliseconds = 0.0;
};

class SculptureWorldProfileReport {
public:
    [[nodiscard]] static std::optional<WorldProfileRecord> readFile(
        const std::string& path, std::string* error = nullptr) {
        std::ifstream input(path, std::ios::binary);
        if (!input) {
            setError(error, "cannot read '" + path + "'");
            return std::nullopt;
        }
        std::ostringstream payload;
        payload << input.rdbuf();
        return parse(payload.str(), error);
    }

    [[nodiscard]] static std::optional<WorldProfileRecord> parse(
        std::string_view payload, std::string* error = nullptr) {
        WorldProfileRecord record;
        std::size_t begin = 0U;
        while (begin < payload.size()) {
            const std::size_t end = payload.find('\n', begin);
            std::string_view line = payload.substr(
                begin, end == std::string_view::npos ? payload.size() - begin : end - begin);
            begin = end == std::string_view::npos ? payload.size() : end + 1U;
            if (!line.empty() && line.back() == '\r') line.remove_suffix(1U);
            if (line.empty()) continue;
            const std::size_t separator = line.find('=');
            if (separator == std::string_view::npos || separator == 0U) {
                setError(error, "malformed profile line");
                return std::nullopt;
            }
            const auto [_, inserted] = record.fields.emplace(
                std::string(line.substr(0U, separator)), std::string(line.substr(separator + 1U)));
            if (!inserted) {
                setError(error, "duplicate profile key");
                return std::nullopt;
            }
        }
        const std::string_view format = record.value("format");
        if (format != "epic-world-profile-v1" && format != "epic-world-profile-v2"
            && format != "epic-world-profile-v3") {
            setError(error, "unsupported profile format");
            return std::nullopt;
        }
        for (const std::string_view required : kBaseRequiredFields) {
            if (record.value(required).empty()) {
                setError(error, "profile is missing '" + std::string(required) + "'");
                return std::nullopt;
            }
        }
        if (format == "epic-world-profile-v2" || format == "epic-world-profile-v3") {
            for (const std::string_view required : kV2RequiredFields) {
                if (record.value(required).empty()) {
                    setError(error, "profile is missing '" + std::string(required) + "'");
                    return std::nullopt;
                }
            }
        }
        if (format == "epic-world-profile-v3") {
            for (const std::string_view required : kV3RequiredFields) {
                if (record.value(required).empty()) {
                    setError(error, "profile is missing '" + std::string(required) + "'");
                    return std::nullopt;
                }
            }
        }
        return record;
    }

    [[nodiscard]] static WorldProfileComparison compare(
        const WorldProfileRecord& baseline, const WorldProfileRecord& candidate) {
        WorldProfileComparison result;
        const std::string_view format = baseline.value("format");
        if (format != candidate.value("format")) {
            result.incompatibilities.emplace_back("format");
            return result;
        }
        const bool usesExpandedIdentity = format != "epic-world-profile-v1";
        const std::string_view* identityFields = usesExpandedIdentity
            ? kV2IdentityFields : kV1IdentityFields;
        const std::size_t identityFieldCount = usesExpandedIdentity
            ? std::size(kV2IdentityFields) : std::size(kV1IdentityFields);
        for (std::size_t index = 0U; index < identityFieldCount; ++index) {
            const std::string_view key = identityFields[index];
            if (baseline.value(key) != candidate.value(key)) {
                result.incompatibilities.emplace_back(key);
            }
        }
        if (baseline.value("profile_status") != "complete"
            || candidate.value("profile_status") != "complete") {
            result.incompatibilities.emplace_back("profile_status");
        }

        const auto baselineKernel = baseline.milliseconds("kernel_ms.mean");
        const auto candidateKernel = candidate.milliseconds("kernel_ms.mean");
        const auto baselineReadback = baseline.milliseconds("readback_ms.mean");
        const auto candidateReadback = candidate.milliseconds("readback_ms.mean");
        const auto baselineBridge = baseline.milliseconds("bridge_rebuild_ms.mean");
        const auto candidateBridge = candidate.milliseconds("bridge_rebuild_ms.mean");
        const auto baselinePresentation = baseline.milliseconds("presentation_submit_cpu_ms.mean");
        const auto candidatePresentation = candidate.milliseconds("presentation_submit_cpu_ms.mean");
        if (!baselineKernel || !candidateKernel || !baselineReadback || !candidateReadback
            || !baselineBridge || !candidateBridge
            || (format == "epic-world-profile-v3"
                && (!baselinePresentation || !candidatePresentation))) {
            result.incompatibilities.emplace_back("complete_stage_timings");
        }
        if (!result.incompatibilities.empty()) return result;

        result.comparable = true;
        result.kernelDeltaMilliseconds = *candidateKernel - *baselineKernel;
        result.readbackDeltaMilliseconds = *candidateReadback - *baselineReadback;
        result.bridgeDeltaMilliseconds = *candidateBridge - *baselineBridge;
        if (format == "epic-world-profile-v3") {
            result.presentationSubmitCpuDeltaMilliseconds =
                *candidatePresentation - *baselinePresentation;
        }
        return result;
    }

private:
    inline static constexpr std::string_view kBaseRequiredFields[] = {
        "format", "profile_status", "preset_id", "source_revision",
        "effective_budget", "effective_update_hz", "opencl_device", "vulkan_device",
        "quality_tier", "view_configuration",
    };
    inline static constexpr std::string_view kV2RequiredFields[] = {
        "effective_generator", "effective_primitive", "effective_delivery",
        "effective_material_bands", "effective_uses_4d_projection",
    };
    inline static constexpr std::string_view kV3RequiredFields[] = {
        "presentation_submit_cpu_ms.samples", "presentation_submit_cpu_ms.min",
        "presentation_submit_cpu_ms.mean", "presentation_submit_cpu_ms.max",
    };
    inline static constexpr std::string_view kV1IdentityFields[] = {
        "preset_id", "source_revision", "effective_budget", "effective_update_hz",
        "opencl_device", "vulkan_device", "quality_tier", "view_configuration",
    };
    inline static constexpr std::string_view kV2IdentityFields[] = {
        "preset_id", "source_revision", "effective_generator", "effective_primitive",
        "effective_delivery", "effective_budget", "effective_update_hz",
        "effective_material_bands", "effective_uses_4d_projection", "opencl_device",
        "vulkan_device", "quality_tier", "view_configuration",
    };

    static void setError(std::string* error, std::string message) {
        if (error != nullptr) *error = std::move(message);
    }
};

} // namespace epi_sculpt
