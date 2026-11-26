#pragma once

#include "SculptureWorldNavigator.h"

namespace epi_sculpt {

// -----------------------------------------------------------------------------
// SculptureWorldActionDispatcher
// -----------------------------------------------------------------------------
// [EDUCATIONAL NOTE]
// Domain-Focused Action Command & Event-Bus Routing Service for SculptureWorldNavigator:
// Encapsulates the 70+ `NavigatorAction` command table (`dispatchAction`) and
// the strongly-typed `SculptureEventBus` listener subscriptions (`bindEventBus`),
// decoupling interactive UI/keyboard/timeline command routing from the core
// procedural world navigation and frame orchestration loop.
// -----------------------------------------------------------------------------
class SculptureWorldActionDispatcher {
public:
    static bool dispatchAction(
        SculptureWorldNavigator& navigator,
        NavigatorAction action,
        float value
    ) noexcept;

    static void bindEventBus(
        SculptureWorldNavigator& navigator,
        SculptureEventBus& eventBus
    );
};

} // namespace epi_sculpt
