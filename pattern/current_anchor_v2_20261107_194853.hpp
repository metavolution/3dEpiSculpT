#pragma once

#include "current_anchor_common.hpp"

namespace epi::presets {

// Explicit next-generation compatibility contract slot.
// Starts intentionally identical to v1 so migration to selector support does
// not alter visuals unless operator explicitly chooses v2 behavior.
inline Pattern current_anchor_v2() {
  Pattern p;
  apply_frozen_current_contract_core(p);
  p.name = "current_anchor_v2";
  return p;
}

} // namespace epi::presets
