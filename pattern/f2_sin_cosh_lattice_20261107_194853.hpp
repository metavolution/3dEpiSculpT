#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * F2 Sin-Cosh Lattice — showcase for GPU inner ID 22 (sin(z)·cosh(z)).
 *
 * MATHEMATICS: HYPERBOLIC-PERIODIC PRODUCT MAP
 *
 * The map f(z) = sin(z)·cosh(z) is the product of two entire functions with
 * complementary symmetry properties:
 *
 *   sin(z) = sin(x)cosh(y) + i·cos(x)sinh(y)   — periodic in x, sinh-growth in y
 *   cosh(z) = cosh(x)cos(y) + i·sinh(x)sin(y)  — even in x, periodic in y
 *
 * Their product creates a LATTICE of alternating basins: along the real axis
 * the map is purely real (sin(x)·cosh(x)), forming a monotone amplified sine.
 * Along the imaginary axis: sin(iy)·cosh(iy) = i·sinh(y)·cos(y) — a damped
 * oscillation with hyperbolic envelope.  The interaction of these two regimes
 * tiles the plane with diamond-shaped domains that warp under iteration.
 *
 * COLOR MODE 4 — SPATIAL WAVE:
 *
 *   hue_shift = hue_time + hv_drift × hv_base × sin(spiral_a × angle + spiral_r × log_r)
 *
 * The diamond lattice structure resonates with the sinusoidal spatial wave:
 * each diamond boundary aligns with a wave crest, producing iridescent moiré-
 * like shimmering as the lattice slowly rotates under the pre-affine tilt.
 *
 * AFFINE GEOMETRY:
 *
 * zn0 = (0.72, 0.22): |zn0| ≈ 0.753, arg ≈ 17°.  The strong contraction is
 * essential — sin(z)·cosh(z) grows as ~exp(|Im(z)|)/2 for large Im(z), so
 * without contraction iterates escape rapidly.  The 17° tilt rotates the
 * diamond lattice off-axis, breaking the natural ±45° alignment into an
 * asymmetric shear pattern.
 *
 * zn2 = (0.88, 0.04): gentle post-contraction with slight tilt.
 *
 * Pipeline: transform → reduce(torus) → sample → color(mode4) → seed → blend
 */
inline Pattern f2_sin_cosh_lattice() {
  Pattern p;
  p.name = "f2_sin_cosh_lattice";
  // Named-enum path: InnerFunction::F2 ↔ GPU id 22 (sin(z)·cosh(z))
  p.inner = InnerFunction::F2;
  p.extended_inner_id = -1;
  p.reduction = ReductionType::Torus;

  // Strong contraction + 17° tilt: prevents cosh-driven escape, tilts lattice
  p.zn.zn[0] = {1.3277f, 0.4057f};
  // Off-center translate: offsets lattice from origin → asymmetric diamond web
  p.zn.zn[1] = {-0.008f, 0.014f};
  // Mild post-contraction with slight tilt
  p.zn.zn[2] = {1.2752f, -0.1797f};
  p.zn.zn[3] = {0.0f,   0.0f};

  // ── Color: MODE 4 (Spatial Wave) — diamond lattice → moiré shimmer ──
  p.color.hue_rotation      = 0.007f;   // moderate base hue rotation
  p.color.sat_scale         = 1.15f;    // vivid: lattice deserves saturated color
  p.color.fade              = 0.990f;   // moderate trail
  p.color.color_mode        = as_int(ColorMode::SpatialWave);        // Spatial Wave
  p.color.spiral_angle_coeff  = 2.20f;  // strong angle term → wave aligns to diamonds
  p.color.spiral_radius_coeff = 0.80f;  // log_r rings reinforce lattice cells
  p.color.chroma_breath_amp   = 0.22f;  // subtle brightness pulse
  p.color.chroma_breath_freq  = 1.20f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base   = 0.50f;  // wave depth: strong spatial modulation
  p.color.hue_velocity_breath = 0.18f;  // oscillating wave amplitude
  p.color.hue_velocity_drift  = 0.60f;  // 60% spatial / 40% base
  p.color.luma_shape_depth    = 0.25f;
  p.color.luma_shape_center   = 1.5f;

  // ── Seed: wider border to feed the lattice boundaries ──
  p.seed.border_width = 0.087f;
  p.seed.hue          = 0.33f;
  p.seed.hue_speed    = 0.091f;
  p.seed.alpha        = 0.92f;

  p.speed = 0.42f;  // faster: lattice motion should be visible
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.103f;
  p.zn.zn0_anim_amp = 0.038f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.151f;
  p.zn.zn1_anim_amp = 0.040f;
  p.zn.zn1_anim_k = 3.0f;
  p.seed.pulse_freq = 0.124f;
  p.seed.pulse_amp = 0.18f;
  p.motion
      .add("radial_pitch_wave",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.057f,
                            .amplitude = 0.224f,
                            .offset = 0.800f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("chroma_diaphragm",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.088f,
                            .amplitude = 0.117f,
                            .offset = 0.220f,
                            .phase = 0.25f},
           control::ModTarget::ChromaBreathAmp)
      .add("seed_spectrum_tide",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.050f,
                            .amplitude = 0.032f,
                            .offset = 0.091f},
           control::ModTarget::SeedHueSpeed);


  return p;
}

} // namespace epi::presets
