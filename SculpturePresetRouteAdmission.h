#pragma once

#include "SculpturePresetAsset.h"

namespace epi_sculpt {

// Source-level route admission only; native producer availability is resolved
// by the frame plan. A canonical feedback asset does not require a same-named
// dedicated world to exist, even when the world catalog recognizes its basename.
[[nodiscard]] inline bool hasImplementedPresetRoute(
    const SculpturePresetAsset& asset) noexcept {
    if (asset.presentationOwnership == SculpturePresentationOwnership::CanonicalFeedback)
        return true;
    const auto* world = findWorldCatalogEntry(asset.id);
    return world == nullptr || world->hasCompleteRoute();
}

} // namespace epi_sculpt
