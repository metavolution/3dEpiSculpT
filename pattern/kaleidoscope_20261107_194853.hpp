#pragma once

#include "base.hpp"

namespace epi::presets {

inline Pattern kaleidoscope() {
  Pattern p;
  p.name = "kaleidoscope";
  p.inner = InnerFunction::Cube;
  p.reduction = ReductionType::Torus;

  p.zn.zn[0] = {1.3893f, 0.2242f};
  p.zn.zn[2] = {1.2585f, -0.1773f};

  p.color.hue_rotation = 0.015f;
  p.color.sat_scale = 1.12f;
  p.color.fade = 0.986f;

  p.seed.border_width = 0.08f;
  p.seed.hue_speed = 0.15f;

  // ── Motion layer: zoom breath + saturation pulse ───────────────────────
  //
  // The Cube inner function creates 3-fold rotational symmetry — visible
  // geometric structure that benefits from two complementary pulses at
  // incommensurate periods (20s vs 15s), producing a composite beat of
  // lcm(20,15)=60s that never feels mechanical:
  //
  //   zoom_pulse: triangle wave on zn0_re.  zn0 = {0.96+tri, 0.04}.
  //     At peaks (zn0_re=0.982) the pattern dilates slightly — the
  //     geometry "breathes out".  At troughs (zn0_re=0.938) it contracts.
  //     Triangle (not sine) gives a crisp reversal at each extreme,
  //     matching the hard geometric edges of the kaleidoscope.
  //
  //   sat_pulse: triangle wave on sat_scale.  Periodic chromatic spikes
  //     briefly saturate the palette — the geometry flares then settles.
  p.motion = control::PatternMotion::zoom_breath(
      /*zn0_centre=*/1.3893f, /*amp=*/0.022f, /*period_s=*/20.0f);
  p.motion.add("sat_pulse",
      control::ModLane{.wave         = control::Waveform::Triangle,
                       .frequency_hz = 1.0f / 15.0f,
                       .amplitude    = 0.055f,
                       .offset       = 1.12f},  // resting = preset sat_scale
      control::ModTarget::SatScale);
  p.zn.zn[1] = {0.2661f, -0.1913f};
  p.seed.alpha = 0.92f;
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.106f;
  p.zn.zn0_anim_amp = 0.048f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.156f;
  p.zn.zn1_anim_amp = 0.056f;
  p.zn.zn1_anim_k = 7.0f;
  p.seed.pulse_freq = 0.093f;
  p.seed.pulse_amp = 0.15f;


  return p;
}

} // namespace epi::presets
