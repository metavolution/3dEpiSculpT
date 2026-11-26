#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Magnetic Dipole (Advanced 3.5D): from Four Golden Lakes down a
 * Period-Doubling Cascade to the Lattes Map.
 *
 * Mathematics:
 * - f_t(u) = t / (1 - u^2), t real. Infinity is a critical point and maps
 *   to the other one, 0, so the family has a single free critical orbit,
 *   0 -> t -> t / (1 - t^2) -> ..., and it stays on the real line.
 * - t = phi (golden ratio): 1 - phi^2 = -phi, so
 *       0 -> phi -> -1 -> infinity -> 0,
 *   a superattracting 4-cycle through both critical points. Four basins
 *   of f^4 with one common boundary: four lakes of Wada. (t = 1 / phi
 *   gives another, 0 -> 1/phi -> 1 -> infinity; t = 1 is the three lakes.)
 * - t = sqrt 2: 0 -> sqrt 2 -> -sqrt 2 -> -sqrt 2. Both critical points
 *   land on a repelling fixed point of multiplier -4 = (1 + i)^4. The map
 *   is postcritically finite with no attracting cycle: a Lattes map, the
 *   quotient of multiplication by 1 + i on the square torus, whose Julia
 *   set is the whole sphere.
 * - In between, on 41 sampled values of t, the critical orbit found an
 *   attracting cycle of period 4 from about 1.50 up to phi, period 8 near
 *   1.48 to 1.50, period 16 near 1.465, and further down mostly none,
 *   with windows (12 near 1.455, 6 near 1.424). The real line carries the
 *   interval map u -> t / (1 - u^2) and this is its cascade; the Lattes
 *   point is its fully chaotic end, as c = -2 is for u^2 + c.
 *
 * Pipeline: MagneticDipole with zn0 = 2.4 i, zn2 = -t / 4.8 and
 * zn3 = t / 4.8 gives T(z) = f_t(2.4 z) / 2.4 exactly for every t, so two
 * Sine lanes in antiphase carry t along a straight line in parameter
 * space with no approximation: t = 1.5161 + 0.1019 sin, turning at phi
 * and at sqrt 2. A Sine lane lingers at its ends, so the picture rests on
 * the four lakes and on the Lattes foam. Clamp, Simplex and an OKLab hue
 * step of 1/4: at t = phi each lake has one hue, terraced every four
 * passes. As t falls the lakes ring, split in two at the doubling, and
 * break up; at sqrt 2 nothing bounds an interior and exit discs of every
 * size cover the frame.
 *
 * Fidelity:   identity for every lane value
 * Cell:       MagneticDipole / Clamp / Simplex / OKLabColorRotation
 * Nearest:    lattes_torus_sphere_shadow holds the Lattes map still (in
 *             another normalisation) and wobbles off it by 2 %;
 *             spatial_wave_feigenbaum_julia_cascade doubles in u^2 + c
 *             and ends at a dendrite. This path ends on the whole sphere.
 * Height:     age = passes since first exit.
 * Hue clock:  bounded; step 1/4.
 * Fastest lane: 0.02 Hz
 * Status:     designed from source on 2026-11-16. The slot arithmetic was
 *             checked at t = sqrt 2, 1.5161 and phi (error 2e-16). At
 *             t = phi on a 300 x 300 grid every pixel was lit and
 *             (k - 1) mod 4 matched the lake for all of them at this
 *             frame and border. The periods quoted are from a scan in
 *             double precision and are not a proof of the cascade's
 *             order. Not built, rendered or measured.
 * @assert sing[0].period == 4 && p_X >= 0.99
 */
inline Pattern magnetic_dipole_golden_four_lakes_cascade_to_lattes() {
  constexpr float MID = 0.31585912f;   // ((sqrt 2 + phi) / 2) / 4.8
  constexpr float SWING = 0.02123129f; // ((phi - sqrt 2) / 2) / 4.8
  constexpr float SWEEP_HZ = 0.004f;
  Pattern p;
  p.name = "magnetic_dipole_golden_four_lakes_cascade_to_lattes";
  p.inner = InnerFunction::MagneticDipole;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {0.0f, 2.4f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {-MID, 0.0f};
  p.zn.zn[3] = {MID, 0.0f};

  p.color.hue_rotation = 0.25f; // one lake per quarter turn
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.93f;
  p.color.color_mode = as_int(ColorMode::OKLabColorRotation);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 0.0f;
  p.color.chroma_breath_amp = 0.05f;
  p.color.chroma_breath_freq = 0.1f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.0006f;
  p.color.hue_velocity_drift = 0.0002f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.0f;

  p.seed.type = SeedType::Simplex;
  p.seed.border_width = 0.1f;
  p.seed.hue = 0.12f;
  p.seed.hue_speed = 0.03f;
  p.seed.alpha = 1.0f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("t_post", // t rises: zn2 = -t / 4.8 falls
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = SWEEP_HZ,
                            .amplitude = -SWING,
                            .offset = -MID},
           control::ModTarget::Zn2Re)
      .add("t_shift",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = SWEEP_HZ,
                            .amplitude = SWING,
                            .offset = MID},
           control::ModTarget::Zn3Re)
      .add("relief_depth",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.02f,
                            .amplitude = 0.025f,
                            .offset = 0.05f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
