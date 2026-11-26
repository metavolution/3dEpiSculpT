#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * A second planar inversion folds the cyclide section into nested chambers. Repeated visits
 * to the tube contours crease the relief.
 *
 * Fidelity: A, analytic field. Meridian and parallel circles on an inverted torus section.
 * Cell: Invert / Torus / CyclidePencil. Seed width controls offset of the inverted torus.
 * Nearest: topo_cyclide_circle_pencil (curatorial comparison).
 * Height: Relief observer samples retained age and luminance.
 * Hue clock: Bounded oscillatory feedback clock. Source hue encodes the local observable.
 * Fastest lane: Per-field feedback; source geometry.
 * Status: Authored atlas preset.
 * @assert bounded_hue_clock
 */
inline Pattern relief35d_cyclide_inversion_vault() {
  Pattern p;
  p.name = "relief35d_cyclide_inversion_vault";
  p.inner = InnerFunction::Invert;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {0.16f, 0.22f};
  p.zn.zn[2] = {0.83f, -0.17f};
  p.zn.zn[3] = {-0.28f, 0.17f};

  p.seed.type = SeedType::CyclidePencil;
  p.seed.border_width = 0.39f;
  p.seed.hue = 0.03f;
  p.seed.alpha = 0.90f;
  p.seed.hue_speed = 0.0f;
  p.seed.pulse_amp = 0.0f;

  p.color.color_mode = as_int(ColorMode::Classic);
  p.color.fade = 0.75f;
  p.color.sat_scale = 1.0f;
  p.color.hue_rotation = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.hue_velocity_breath = 0.001f;
  p.color.chroma_breath_freq = 0.07f;
  p.relief_35d = true;
  p.relief_depth_range = 2.2f;
  return p;
}

} // namespace epi::presets
