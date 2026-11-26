#pragma once
#include "SculptureProgramSchema.h"

namespace epi_sculpt {

// [EDUCATIONAL ANNOTATION: Plugin-based Architecture]
// By decoupling specific physical simulations (like Audio Resonance and 4D Foliation)
// from the core coordinator via this interface, the engine strictly adheres to the 
// Open-Closed Principle. New effects or AI-driven parameter directors can be injected
// dynamically without ever touching the main pipeline orchestration logic again.

class ISculptureCoordinatorPlugin {
public:
    virtual ~ISculptureCoordinatorPlugin() = default;

    // Called once per active simulation frame, before GPU dispatch.
    virtual void step(
        SculptureProgramParams& params, 
        float simulatedTime, 
        float deltaSeconds) = 0;
};

} // namespace epi_sculpt
