#pragma once

#include "base.hpp"

namespace epi::presets {

inline Pattern default_pattern() {
  Pattern p;
  p.name = "default";
  p.inner = InnerFunction::Square;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {0.97f, 0.03f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.color.hue_rotation = 0.02f;
  p.color.fade = 0.98f;
  p.seed.border_width = 0.1f;
  p.seed.hue_speed = 0.1f;
  return p;
}

} // namespace epi::presets
