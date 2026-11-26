#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Spatial Wave (Advanced 3.5D): A Piecewise Rotation and its Pentagon Mosaic.
 *
 * Mathematics:
 * - SymmetryBreak shifts the upper half-plane by 0.1 along the real axis
 *   and leaves the lower half alone. The shift is along the dividing line,
 *   so it is a bijection of the plane: a slip on a fault. With the affine
 *   slots,
 *       T(z) = L (z + d H(z)) + zn3,   L = e^{2 pi i / 5},
 *   where H is 1 above the fault and 0 below and d = 0.1 / zn0 = 0.5. T is
 *   a rotation by 72 degrees on each side, about two different centres. It
 *   is a piecewise isometry of the kind studied by Goetz: it stretches
 *   nothing, its entropy is zero, and all of its structure comes from the
 *   cut.
 * - The centres differ by d (-1/2 + (i/2) cot 36 deg) = (-0.25, 0.344).
 *   zn3 puts them at a = (0.125, -0.172) below the fault and b = -a above
 *   it, so at the centre of the lane swing T commutes with z -> -z.
 * - A point whose whole orbit stays below the fault lies in the
 *   intersection of five turned copies of the lower half-plane: a regular
 *   pentagon with centre a, inradius 0.172 and one side on the fault, from
 *   0 to 0.25. The same holds above with centre b and the side from -0.25
 *   to 0. The two pentagons meet at the origin, where the seed is centred.
 *   The two inradii always add up to 0.344 and tan 36 deg * 0.344 = 0.25,
 *   so the pentagons share a vertex wherever the centres are moved.
 * - On any periodic cell of period q, T^q is a rotation by a multiple of
 *   72 degrees or the identity, so T^(5q) is the identity there:
 *   generations n and n + 5q coincide and the cell holds a sharp, steady,
 *   fivefold arrangement of ring sources. The cells are separated by the
 *   pre-images of the fault, straight segments in five directions.
 * - The hue step is 1/5 of a turn per pass, so on the two pentagons the
 *   five sources keep five fixed hues.
 *
 * Pipeline: SymmetryBreak with zn0 = 0.2 and zn2 = 5 L; Clamp; WaveRing.
 * Every value of zn3 gives an exact piecewise isometry with the same
 * angle, so the lanes are free: two Sine lanes in phase move both centres
 * straight across the fault by +-0.08, and one pentagon grows (inradius
 * 0.252) as the other shrinks (0.092). The two pentagons stay within 0.6
 * of the origin and never leave the frame. Orbits that do leave the square
 * are pinned by Clamp and are outside the claim.
 *
 * Fidelity:   identity
 * Cell:       SymmetryBreak / Clamp / WaveRing / SpatialWave
 * Nearest:    spatial_wave_rotation_number_resonance_sweep is one rotation
 *             about one centre; op_art_symmetry_glitch uses the same inner
 *             as a visual glitch with no isometry or period stated.
 * Height:     age = number of passes; on a cell of period q it repeats with
 *             period 5q.
 * Hue clock:  bounded (hue_velocity_base = 0).
 * Fastest lane: 0.025 Hz
 * Status:     designed from source on 2026-11-15; not built, rendered or
 *             measured. Whether every orbit of this map is bounded is not
 *             claimed.
 * @assert p_N >= 0.99 && rot.q == 5
 */
inline Pattern spatial_wave_goetz_pentagon_piecewise_rotation_mosaic() {
  Pattern p;
  p.name = "spatial_wave_goetz_pentagon_piecewise_rotation_mosaic";
  p.inner = InnerFunction::SymmetryBreak;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {0.2f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.5450850f, 4.7552826f};     // 5 e^{2 pi i / 5}
  p.zn.zn[3] = {-0.07725424f, -0.23776413f}; // a (1 - L)

  p.color.hue_rotation = 0.2f; // one fifth of a turn per pass
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.94f;
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 1.0f;
  p.color.chroma_breath_amp = 0.02f;
  p.color.chroma_breath_freq = 0.08f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.01f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 1.0f;

  p.seed.type = SeedType::WaveRing;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.0f;
  p.seed.hue_speed = 0.02f;
  p.seed.alpha = 0.5f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("fault_offset_re", // 0.08 * 0.95105652
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.005f,
                            .amplitude = 0.07608452f,
                            .offset = -0.07725424f},
           control::ModTarget::Zn3Re)
      .add("fault_offset_im", // 0.08 * 0.69098301
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.005f,
                            .amplitude = 0.05527864f,
                            .offset = -0.23776413f},
           control::ModTarget::Zn3Im)
      .add("relief_depth",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.025f,
                            .amplitude = 0.012f,
                            .offset = 0.02f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
