// Renderer-neutral CPU reference evaluator for the sculpture program.
//
// Authored mathematical state lives in SculptureProgramSchema.h. This header
// adds only the stateful CPU oracle/evaluator and its temporal working storage.
// Keeping the two layers separate lets preset/serialization tools remain truly
// headless without depending on the evaluator implementation.

#pragma once

#include "SculptureProgramSchema.h"

#include <memory>

namespace epi_sculpt {

// -----------------------------------------------------------------------------
// Educational Annotation: SculptureProgram (The Oracle)
// This is the CPU reference implementation of the visual math engine.
// The value schema is declared separately in SculptureProgramSchema.h; this class
// provides the deterministic evaluation path and CPU-owned temporal working state.
// -----------------------------------------------------------------------------
class SculptureProgram {
public:
    explicit SculptureProgram(SculptureProgramParams params = {});

    [[nodiscard]] const SculptureProgramParams& params() const noexcept;
    void setParams(const SculptureProgramParams& params) noexcept {
        params_ = params;
        params_.stageMask = learn_mode::normalize(params_.stageMask);
    }
    // Synchronous CPU evaluation: settle all submitted tasks before return or
    // unwind. Submission failure takes precedence; otherwise observe task errors
    // in submission order. Output may be partial on failure and must be discarded.
    // previousFrame and outSnapshot must be distinct; aliasing is rejected
    // before output or source-history mutation.
    void evaluate(float time, float morph, SculptureSnapshot& outSnapshot, const SculptureSnapshot* previousFrame = nullptr) const;
    [[nodiscard]] SculptureSnapshot evaluate(float time, float morph, const SculptureSnapshot* previousFrame = nullptr) const {
        SculptureSnapshot snap;
        evaluate(time, morph, snap, previousFrame);
        return snap;
    }

    // -------------------------------------------------------------------------
    // Stateful Temporal Cadence Governance
    // Evaluates simulation using fixed-timestep sub-stepping decoupled from
    // variable display framerates, ensuring deterministic dynamical orbits.
    // -------------------------------------------------------------------------
    void resetTemporalHistory() noexcept;
    [[nodiscard]] bool hasTemporalHistory() const noexcept;
    [[nodiscard]] const SculptureSnapshot* currentTemporalHistory() const noexcept;
    // Require finite wall time/morph/delta/feedback blend and, when active, fixed
    // timestep. Invalid input throws before touching history/clocks/output/reset.
    // Bootstrap/history replacement and each admitted fixed step commit on success.
    // Offered time remains pending on failure; retry with zero new delta after
    // discarding partial output. Earlier accepted steps remain committed even
    // if a later step or the final observation copy throws. Admitted explicit
    // resets still apply on evaluation failure; clear temporalReset before retry.
    // A successful history replacement may invalidate borrowed history pointers.
    void evaluateWithCadence(float wallTime, float morph, float deltaSeconds, SculptureSnapshot& outSnapshot);
    [[nodiscard]] SculptureSnapshot evaluateWithCadence(float wallTime, float morph, float deltaSeconds) {
        SculptureSnapshot snap;
        evaluateWithCadence(wallTime, morph, deltaSeconds, snap);
        return snap;
    }

    // -------------------------------------------------------------------------
    // Educational Annotation: Volumetric Continuous Field Evaluation
    // Step 7 introduces implicit field sampling. Rather than querying only
    // discrete parametric bead indices (1D knot / bead instances), these methods
    // evaluate the continuous scalar density field and normal gradients at arbitrary
    // 3D coordinates in [-1.0, 1.0]^3, enabling dual particle/volumetric raymarching.
    // -------------------------------------------------------------------------
    [[nodiscard]] float sampleDensityField(const SculptureVec3& pos, float time, float morph, const std::vector<float>& spatialBuffer = {}) const;
    [[nodiscard]] SculptureVec3 evaluateFieldGradient(const SculptureVec3& pos, float time, float morph, float eps = 0.005f, const std::vector<float>& spatialBuffer = {}) const;
    // Borrow an x-fastest cubic field; infer its axis by rounded cube root.
    // Empty/truncated inferred cubes or nonfinite coordinates return zero.
    // Finite coordinates clamp to [0,1]; admitted trailing samples are ignored.
    // Allocation-free observation: no field mutation, readback or cadence change.
    [[nodiscard]] static float trilinearSampleVoxelBuffer(const std::vector<float>& spatialBuffer, const SculptureVec3& normPos);
    [[nodiscard]] static std::uint32_t getAuxiliaryVoxelGridDimension(StageResolutionScale scale) noexcept;

private:
    SculptureProgramParams params_;
    mutable std::unique_ptr<SculptureSnapshot> temporalHistoryBuffer_;
    mutable std::unique_ptr<SculptureSnapshot> temporalScratchBuffer_;
    mutable float cadenceAccumulator_ = 0.0f;
    mutable float simulatedTime_ = 0.0f;
};

} // namespace epi_sculpt
