#pragma once

#include <mgfx/PresentationDeviceHost.h>

namespace epi_sculpt {

// Compatibility vocabulary retained for the flagship application. The actual
// native lifetime owner now lives in mGFX so catalog experiments and the
// flagship share exactly one device-island reconstruction implementation.
using SculpturePresentationHostRecipe = mgfx::PresentationDeviceHostRecipe;
using SculpturePresentationRecoveryStatus = mgfx::PresentationDeviceRecoveryStatus;
using SculpturePresentationRecoveryResult = mgfx::PresentationDeviceRecoveryResult;
using SculpturePresentationHost = mgfx::PresentationDeviceHost;

} // namespace epi_sculpt
