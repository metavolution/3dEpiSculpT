// Durable, human-readable output for completed world profile sessions.
#pragma once

#include "SculptureWorldProfile.h"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>

namespace epi_sculpt {

class SculptureWorldProfileSidecar {
public:
    // A completed run must retain its own evidence. The caller supplies the
    // session clock and sequence so this naming stays deterministic and does
    // not make the renderer or OpenCL producer own filesystem policy.
    [[nodiscard]] static std::string makeSessionPath(
        std::string_view directory,
        std::string_view canonicalPresetId,
        std::uint64_t unixMilliseconds,
        std::uint64_t sessionSequence) {
        std::string safePresetId;
        safePresetId.reserve(canonicalPresetId.size());
        for (const unsigned char character : canonicalPresetId) {
            const bool isAsciiAlphaNumeric = (character >= 'a' && character <= 'z')
                || (character >= 'A' && character <= 'Z')
                || (character >= '0' && character <= '9');
            safePresetId += (isAsciiAlphaNumeric || character == '-' || character == '_')
                ? static_cast<char>(character) : '_';
        }
        if (safePresetId.empty()) safePresetId = "world";

        std::filesystem::path path(directory.empty() ? "profiles" : directory);
        path /= safePresetId + "_" + std::to_string(unixMilliseconds)
            + "_" + std::to_string(sessionSequence) + ".profile";
        return path.string();
    }

    [[nodiscard]] static std::string serialize(const WorldProfileSummary& summary) {
        const WorldProfileAssessment assessment = assessWorldProfile(summary);
        std::ostringstream out;
        // v3 adds CPU-side presentation submission timing. Keep the marker
        // honest so offline tools never compare it as an older profile.
        out << "format=epic-world-profile-v3\n"
            << "profile_status=" << (summary.interrupted
                ? "interrupted_producer_unavailable"
                : summary.acceptedGenerationCount == 0U
                    ? "incomplete_no_samples" : "complete") << '\n'
            << "preset_id=" << summary.identity.canonicalPresetId << '\n'
            << "source_revision=" << summary.identity.sourceRevision << '\n'
            << "requested_generator="
            << worldGeneratorName(summary.identity.requestedRecipe.generator) << '\n'
            << "requested_primitive="
            << worldPrimitiveName(summary.identity.requestedRecipe.primitive) << '\n'
            << "requested_delivery="
            << deliveryName(summary.identity.requestedRecipe.delivery) << '\n'
            << "requested_budget=" << summary.identity.requestedRecipe.instanceBudget << '\n'
            << "requested_update_hz=" << summary.identity.requestedRecipe.generatorHz << '\n'
            << "requested_material_bands="
            << static_cast<unsigned int>(summary.identity.requestedRecipe.materialBandCount) << '\n'
            << "requested_uses_4d_projection="
            << (summary.identity.requestedRecipe.uses4DProjection ? "true" : "false") << '\n'
            << "requested_mirror_topology="
            << mirrorCellTopologyName(summary.identity.requestedRecipe.mirrorTopology) << '\n'
            << "effective_generator="
            << worldGeneratorName(summary.identity.effectiveRecipe.generator) << '\n'
            << "effective_primitive="
            << worldPrimitiveName(summary.identity.effectiveRecipe.primitive) << '\n'
            << "effective_delivery="
            << deliveryName(summary.identity.effectiveRecipe.delivery) << '\n'
            << "effective_budget=" << summary.identity.effectiveRecipe.instanceBudget << '\n'
            << "effective_update_hz=" << summary.identity.effectiveRecipe.generatorHz << '\n'
            << "effective_material_bands="
            << static_cast<unsigned int>(summary.identity.effectiveRecipe.materialBandCount) << '\n'
            << "effective_uses_4d_projection="
            << (summary.identity.effectiveRecipe.uses4DProjection ? "true" : "false") << '\n'
            << "effective_mirror_topology="
            << mirrorCellTopologyName(summary.identity.effectiveRecipe.mirrorTopology) << '\n'
            << "opencl_device=" << summary.identity.openClDeviceId << '\n'
            << "vulkan_device=" << summary.identity.vulkanDeviceId << '\n'
            << "quality_tier=" << summary.identity.qualityTier << '\n'
            << "view_configuration=" << summary.identity.viewConfiguration << '\n'
            << "accepted_generations=" << summary.acceptedGenerationCount << '\n'
            << "first_generation=" << summary.firstGeneration << '\n'
            << "last_generation=" << summary.lastGeneration << '\n'
            << "assessment_status=" << name(assessment.status) << '\n'
            << "bottleneck=" << name(assessment.bottleneck) << '\n'
            << "recommendation=" << name(assessment.recommendation) << '\n'
            << std::fixed << std::setprecision(3)
            << "direct_stage_mean_ms=" << assessment.directStageMeanMilliseconds << '\n'
            << "bottleneck_fraction=" << assessment.bottleneckFraction << '\n';
        appendTiming(out, "queue_to_delivery_ms", summary.queueToDelivery);
        appendTiming(out, "kernel_ms", summary.kernel);
        appendTiming(out, "readback_ms", summary.readback);
        appendTiming(out, "bridge_rebuild_ms", summary.bridgeRebuild);
        appendTiming(out, "presentation_submit_cpu_ms", summary.presentationSubmitCpu);
        return out.str();
    }

    [[nodiscard]] static bool writeToFile(const std::string& path,
                                          const WorldProfileSummary& summary) {
        const std::filesystem::path destination(path);
        std::error_code error;
        if (!destination.parent_path().empty()) {
            std::filesystem::create_directories(destination.parent_path(), error);
            if (error) return false;
        }

        // Write beside the destination and rename only after the full payload
        // reaches the stream. A crash or disk error therefore preserves the
        // last complete profile instead of publishing a truncated measurement.
        const std::filesystem::path temporary = destination.string() + ".tmp";
        std::ofstream out(temporary, std::ios::binary | std::ios::trunc);
        if (!out) return false;
        out << serialize(summary);
        out.close();
        if (!out) {
            std::filesystem::remove(temporary, error);
            return false;
        }
        std::filesystem::rename(temporary, destination, error);
        if (!error) return true;
        std::filesystem::remove(temporary, error);
        return false;
    }

private:
    [[nodiscard]] static constexpr std::string_view deliveryName(
        WorldRecipeDelivery delivery) noexcept {
        return delivery == WorldRecipeDelivery::DedicatedGeneratorRequired
            ? "dedicated_generator_required"
            : "existing_compact_path";
    }

    static void appendTiming(std::ostringstream& out, const char* name,
                             const WorldProfileTiming& timing) {
        out << name << ".samples=" << timing.sampleCount << '\n'
            << std::fixed << std::setprecision(3)
            << name << ".min=" << timing.minimumMilliseconds << '\n'
            << name << ".mean=" << timing.meanMilliseconds << '\n'
            << name << ".max=" << timing.maximumMilliseconds << '\n';
    }

    static void appendTimingJson(std::ostringstream& out, const char* name,
                                 const WorldProfileTiming& timing) {
        out << "\"" << name << "\": {"
            << "\"samples\": " << timing.sampleCount << ", "
            << std::fixed << std::setprecision(3)
            << "\"min_ms\": " << timing.minimumMilliseconds << ", "
            << "\"mean_ms\": " << timing.meanMilliseconds << ", "
            << "\"max_ms\": " << timing.maximumMilliseconds
            << "}";
    }

    [[nodiscard]] static std::string serializeJson(const WorldProfileSummary& summary) {
        const WorldProfileAssessment assessment = assessWorldProfile(summary);
        std::ostringstream out;
        out << "{\n"
            << "\"format\": \"epic-world-profile-v3\",\n"
            << "\"profile_status\": \"" << (summary.interrupted
                ? "interrupted_producer_unavailable"
                : summary.acceptedGenerationCount == 0U
                    ? "incomplete_no_samples" : "complete") << "\",\n"
            << "\"preset_id\": \"" << summary.identity.canonicalPresetId << "\",\n"
            << "\"source_revision\": \"" << summary.identity.sourceRevision << "\",\n"
            << "\"requested_generator\": \"" << worldGeneratorName(summary.identity.requestedRecipe.generator) << "\",\n"
            << "\"requested_primitive\": \"" << worldPrimitiveName(summary.identity.requestedRecipe.primitive) << "\",\n"
            << "\"requested_delivery\": \"" << deliveryName(summary.identity.requestedRecipe.delivery) << "\",\n"
            << "\"requested_budget\": " << summary.identity.requestedRecipe.instanceBudget << ",\n"
            << "\"requested_update_hz\": " << summary.identity.requestedRecipe.generatorHz << ",\n"
            << "\"requested_material_bands\": " << static_cast<unsigned int>(summary.identity.requestedRecipe.materialBandCount) << ",\n"
            << "\"requested_uses_4d_projection\": " << (summary.identity.requestedRecipe.uses4DProjection ? "true" : "false") << ",\n"
            << "\"requested_mirror_topology\": \"" << mirrorCellTopologyName(summary.identity.requestedRecipe.mirrorTopology) << "\",\n"
            << "\"effective_generator\": \"" << worldGeneratorName(summary.identity.effectiveRecipe.generator) << "\",\n"
            << "\"effective_primitive\": \"" << worldPrimitiveName(summary.identity.effectiveRecipe.primitive) << "\",\n"
            << "\"effective_delivery\": \"" << deliveryName(summary.identity.effectiveRecipe.delivery) << "\",\n"
            << "\"effective_budget\": " << summary.identity.effectiveRecipe.instanceBudget << ",\n"
            << "\"effective_update_hz\": " << summary.identity.effectiveRecipe.generatorHz << ",\n"
            << "\"effective_material_bands\": " << static_cast<unsigned int>(summary.identity.effectiveRecipe.materialBandCount) << ",\n"
            << "\"effective_uses_4d_projection\": " << (summary.identity.effectiveRecipe.uses4DProjection ? "true" : "false") << ",\n"
            << "\"effective_mirror_topology\": \"" << mirrorCellTopologyName(summary.identity.effectiveRecipe.mirrorTopology) << "\",\n"
            << "\"opencl_device\": \"" << summary.identity.openClDeviceId << "\",\n"
            << "\"vulkan_device\": \"" << summary.identity.vulkanDeviceId << "\",\n"
            << "\"quality_tier\": \"" << summary.identity.qualityTier << "\",\n"
            << "\"view_configuration\": \"" << summary.identity.viewConfiguration << "\",\n"
            << "\"accepted_generations\": " << summary.acceptedGenerationCount << ",\n"
            << "\"first_generation\": " << summary.firstGeneration << ",\n"
            << "\"last_generation\": " << summary.lastGeneration << ",\n"
            << "\"assessment_status\": \"" << name(assessment.status) << "\",\n"
            << "\"bottleneck\": \"" << name(assessment.bottleneck) << "\",\n"
            << "\"recommendation\": \"" << name(assessment.recommendation) << "\",\n"
            << std::fixed << std::setprecision(3)
            << "\"direct_stage_mean_ms\": " << assessment.directStageMeanMilliseconds << ",\n"
            << "\"bottleneck_fraction\": " << assessment.bottleneckFraction << ",\n";
        
        appendTimingJson(out, "queue_to_delivery", summary.queueToDelivery);
        out << ",\n";
        appendTimingJson(out, "kernel", summary.kernel);
        out << ",\n";
        appendTimingJson(out, "readback", summary.readback);
        out << ",\n";
        appendTimingJson(out, "bridge_rebuild", summary.bridgeRebuild);
        out << ",\n";
        appendTimingJson(out, "presentation_submit_cpu", summary.presentationSubmitCpu);
        out << "\n}\n";
        return out.str();
    }
};

} // namespace epi_sculpt
