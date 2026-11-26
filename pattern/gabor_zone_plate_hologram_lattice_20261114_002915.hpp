#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Spectral & Optical: Gabor Zone-Plate Hologram Lattice.
 *
 * Scientific foundation:
 * - Gabor's in-line hologram of a point source is a Fresnel zone plate, the
 *   intensity cos(a r^2): rings whose radii grow like sqrt(n). A hologram of
 *   several points is the superposition of their zone plates, and each plate
 *   acts as a lens that refocuses its own point.
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Seed = WaveRing, sin(20 r - 5 t) exp(-2 r): rings of constant spacing.
 *   The seed is a function of |z| and is not holomorphic, which is what makes
 *   circular zones possible in a pipeline whose transforms are all conformal.
 * - Inner = Square. The seed is sampled at T(z) ~ (zn0 z)^2, so the ring phase
 *   is 20 |zn0|^2 r^2: an exact travelling zone plate with r_n ~ sqrt(n).
 * - Reduce = Torus. The triangle fold returns to its origin at every lattice
 *   point T in 2Z[i], so each of those points carries its own ring system.
 *   Pulled back through the square they become zone plates centred on the
 *   points sqrt((2a + 2bi) / zn0^2): about forty plates arranged on the
 *   confocal-hyperbola grid of z^2, overlapping like the hologram of a point
 *   array.
 * - Second generation: zone plates of zone plates (phase ~ r^4), visible in
 *   the translucent gaps between rings. Fade = 0.94 keeps the third from
 *   aliasing.
 * - Color = Alive: the hue advance per pass is uniform in space at any
 *   instant, so each generation is tinted as a whole and the ring structure
 *   stays readable.
 * - Motion: Zn3 slides the source lattice, so plates drift, and pairs merge
 *   into one when a lattice point crosses the branch point at the centre.
 */
inline Pattern gabor_zone_plate_hologram_lattice() {
  Pattern p;
  p.name = "gabor_zone_plate_hologram_lattice";
  p.inner = InnerFunction::Square;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {1.5f, 0.15f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.0f; // unused by Alive
  p.color.sat_scale = 1.06f;
  p.color.fade = 0.94f;
  p.color.color_mode = as_int(ColorMode::Alive);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 0.0f;
  p.color.chroma_breath_amp = 0.0f;
  p.color.chroma_breath_freq = 0.1f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.004f;
  p.color.hue_velocity_breath = 0.002f;
  p.color.hue_velocity_drift = 0.001f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.0f;

  p.seed.type = SeedType::WaveRing;
  p.seed.border_width = 0.07f;
  p.seed.hue = 0.60f;
  p.seed.hue_speed = 0.07f;
  p.seed.alpha = 0.90f;
  p.seed.pulse_freq = 0.06f;
  p.seed.pulse_amp = 0.12f;
  p.speed = 0.8f;

  p.motion
      .add("source_lattice_x",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.019f,
                            .amplitude = 0.45f,
                            .offset = 0.0f},
           control::ModTarget::Zn3Re)
      .add("source_lattice_y",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.013f,
                            .amplitude = 0.45f,
                            .offset = 0.0f,
                            .phase = 0.25f},
           control::ModTarget::Zn3Im)
      .add("zone_shear",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.027f,
                            .amplitude = 0.10f,
                            .offset = 0.15f},
           control::ModTarget::Zn0Im);

  return p;
}

} // namespace epi::presets
