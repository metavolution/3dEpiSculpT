#pragma once

#include "current_anchor_common.hpp"

namespace epi::presets {

// Frozen compatibility contract for the "current visuals" baseline.
// Keep this function stable so operators can always return to this look
// even while runtime defaults and future profiles evolve.
inline Pattern current_anchor_v1() {
  Pattern p;
  apply_frozen_current_contract_core(p);
  p.name = "current_anchor_v1";
  return p;
}

} // namespace epi::presets
