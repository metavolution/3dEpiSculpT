/**
 * ============================================================================
 * EPI2 / 3dEpiSculpTMorph - SCULPTURE COORDINATOR EVENT ROUTER
 * ============================================================================
 *
 * [EDUCATIONAL NOTE: Decoupled Semantic Event Routing Domain Service]
 * ----------------------------------------------------------------------------
 * Isolates all `SculptureEventBus` subscription wiring (playback rate, learn
 * mode stage masks, 4D rotor nudges, multi-track choreography, file/HTML/mesh
 * export, timeline cues, sequence recording, and 3D volumetric field controls)
 * from the core `SculpturePipelineCoordinator` frame-tick state machine.
 * ============================================================================
 */

#pragma once

namespace epi_sculpt {

class SculpturePipelineCoordinator;
class SculptureEventBus;

class SculptureCoordinatorEventRouter {
public:
    static void bindAll(SculpturePipelineCoordinator& coordinator, SculptureEventBus& eventBus);

    static void bindPlaybackAndTourEvents(SculpturePipelineCoordinator& coordinator, SculptureEventBus& eventBus);
    static void bindPerformanceDeckEvents(SculpturePipelineCoordinator& coordinator, SculptureEventBus& eventBus);
    static void bindFileAndSequenceEvents(SculpturePipelineCoordinator& coordinator, SculptureEventBus& eventBus);
    static void bindModifierEvents(SculpturePipelineCoordinator& coordinator, SculptureEventBus& eventBus);
};

} // namespace epi_sculpt
