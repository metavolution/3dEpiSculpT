#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Minimal 3.5D: Ad Quadratum. The Mirror Triangle Folded onto Itself Twice.
 *
 * Mathematics:
 * - PareidoliaCathedral is the Torus mirror fold followed by a mirror in
 *   the anti-diagonal. Together they generate the reflection group *442
 *   (p4m): mirrors on the lines x = odd, y = odd, x + y = 0 mod 4 and
 *   x - y = 2 mod 4. Its chamber is the right isosceles triangle with
 *   corners (1,-1) and (-1,1), where four mirrors meet, and (1,1), where
 *   two do.
 * - T(z) = c + (1 + i)(z - c) with c = (1,-1) turns the plane 45 degrees
 *   about a fourfold corner and enlarges it by sqrt 2. It carries mirrors
 *   to mirrors and the translation lattice 4 Z[i] into itself, so
 *   fold(T(fold z)) = fold(T z): T is a well-defined map of the triangle
 *   onto itself, two to one. A Gaussian integer off the 45 degree
 *   directions, such as 2 + i, does not have this property.
 * - This is the halving of a right isosceles triangle into two similar
 *   ones, and the mason's rule ad quadratum: each square inscribed
 *   cornerwise in the last, at 1 / sqrt 2 the size.
 * - The seed's ring source at 0 has images under the group at every point
 *   with both coordinates even. Generation n therefore shows ring sources
 *   on a square lattice of spacing 2 / sqrt(2)^n, turned 45 degrees at
 *   each step: spacing 2, then 1.414 (the four edge midpoints), then 1,
 *   then 0.707, and so on down.
 * - zn0 = (1, 1) and zn3 = (-1, -1) are exact in floating point.
 *
 * Pipeline: Identity / PareidoliaCathedral / WaveRing. The geometry is
 * exact and fixed, and no zn lane is authored. Once the lattice spacing
 * falls below the ring wavelength (0.314, at generation 6) the sources
 * blend into a smooth ground, so the cascade ends by itself.
 *
 * Fidelity:   identity
 * Cell:       Identity / PareidoliaCathedral / WaveRing / DepthEcho
 * Nearest:    spatial_wave_cathedral_quintupling_kaleidoscope is z -> 5 z
 *             on the same fold, with no rotation and degree 25; this is
 *             the smallest self-cover of the triangle, degree 2, and it
 *             turns.
 * Height:     age = generation n, the number of halvings.
 * Hue clock:  bounded (hue_velocity_base = 0).
 * Fastest lane: 0.024 Hz
 * Status:     designed from source on 2026-11-15; the identity
 *             fold(T(fold z)) = fold(T z) was confirmed to 1e-15 in a
 *             separate arithmetic check; not built, rendered or measured.
 * @assert fold_exact && p_E >= 0.99 && lambda_med within 1 % of 0.3466
 */
inline Pattern identity_minimal_ad_quadratum_cathedral_fold() {
  Pattern p;
  p.name = "identity_minimal_ad_quadratum_cathedral_fold";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::PareidoliaCathedral;
  p.zn.zn[0] = {1.0f, 1.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {-1.0f, -1.0f};

  p.color.hue_rotation = 0.05f;
  p.color.sat_scale = 0.92f;
  p.color.fade = 0.82f;
  p.color.color_mode = as_int(ColorMode::DepthEcho);
  p.color.spiral_angle_coeff = 0.14f; // hue per unit of age
  p.color.spiral_radius_coeff = 0.0f;
  p.color.chroma_breath_amp = 0.15f; // saturation gained with age
  p.color.chroma_breath_freq = 0.08f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.03f;
  p.color.hue_velocity_drift = 0.0f; // 0 = hue from age only
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 1.0f;

  p.seed.type = SeedType::WaveRing;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.08f;
  p.seed.hue_speed = 0.02f;
  p.seed.alpha = 0.55f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("source_hue",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.009f,
                            .amplitude = 0.12f,
                            .offset = 0.08f},
           control::ModTarget::SeedHue)
      .add("age_hue_scale",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.013f,
                            .amplitude = 0.05f,
                            .offset = 0.14f},
           control::ModTarget::SpiralAngleCoeff)
      .add("relief_depth",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.024f,
                            .amplitude = 0.06f,
                            .offset = 0.15f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
