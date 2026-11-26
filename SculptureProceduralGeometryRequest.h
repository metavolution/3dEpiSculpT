#pragma once

#include "EpimorphicReliefGenerator.h"
#include "Sculpture4DPolychoronHomotopy.h"

#include <cstdint>

namespace epi_sculpt {

// A CPU geometry observation samples the coordinator's existing cadence. It is
// not an assertion that a corresponding OpenCL submission was accepted.
struct ProceduralGeometrySample {
    SculptureProgramParams params{};
    float time = 0.0f;
    float morph = 0.0f;

    [[nodiscard]] bool operator==(const ProceduralGeometrySample& other) const noexcept {
        // Reuse the schema's field-wise producer policy: presentation-only
        // contour edits do not change the sampled mathematical source.
        return equivalentProducerParams(params, other.params)
            && time == other.time && morph == other.morph;
    }
};

enum class ReliefSourceKind : std::uint8_t {
    Program,
    AuxiliaryTexture,
    VolumetricMemory
};

struct ReliefSourceIdentity {
    ReliefSourceKind kind = ReliefSourceKind::Program;
    // Owner-issued process-local content revision, never a world delivery ID.
    // Non-program sources leave programSample at its default value.
    std::uint64_t contentRevision = 0U;
    ProceduralGeometrySample programSample{};
    float volumeSliceRadius = 2.5f; // Coupler's sampling half-extent in world units.

    [[nodiscard]] bool operator==(const ReliefSourceIdentity& other) const noexcept {
        if (kind != other.kind) return false;
        if (kind == ReliefSourceKind::Program) return programSample == other.programSample;
        return contentRevision == other.contentRevision
            && (kind != ReliefSourceKind::VolumetricMemory
                || volumeSliceRadius == other.volumeSliceRadius);
    }
};

struct ReliefMeshRequest {
    EpimorphicReliefConfig config{};
    ReliefSourceIdentity source{};

    [[nodiscard]] bool operator==(const ReliefMeshRequest&) const noexcept = default;
};

struct PolychoronMeshRequest {
    PolychoronHomotopyConfig config{};
    float sampledTime = 0.0f;
    float worldScale = 1.0f; // Tube radius = 0.025 * worldScale.

    [[nodiscard]] bool operator==(const PolychoronMeshRequest&) const noexcept = default;
};

} // namespace epi_sculpt
