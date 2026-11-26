#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Square (Minimal 3.5D): a Fresnel Zone Plate Made by Squaring a Ring Wave.
 *
 * Mathematics:
 * - T(z) = z^2. Generation n samples the seed at z^(2^n), whose modulus is
 *   r^(2^n). The WaveRing seed is sin(20 r - 5 t) e^(-2 r), radial, so the
 *   angle doubling of z^2 does nothing to it and generation n is
 *   sin(20 r^(2^n) - 5 t) e^(-2 r^(2^n)).
 * - Generation 1 has phase 20 r^2: quadratic in the radius, which is the
 *   phase of a Fresnel zone plate (the interference of a spherical and a
 *   plane wave). Its crests sit at r_k = sqrt((pi/2 + 2 pi k + 5 t) / 20):
 *   radii in proportion to sqrt k, so every zone has the same area, and
 *   they drift outward with time, as a hologram's fringes do when the
 *   reference phase is swept.
 * - Generations 2, 3, ... have phases 20 r^4, 20 r^8: ever steeper chirps,
 *   all crowding against the unit circle, the Julia set of z^2. Inside the
 *   circle every orbit falls to 0; outside it escapes and is clamped onto
 *   the border, where the ring wave is weak (e^-2 or less).
 *
 * Pipeline: Square with bare slots; Clamp; WaveRing at alpha 0.6; OKLab hue
 * step 1/4, so the zone plate (generation 1) and its squares (2, 3, 4)
 * each take a quarter of the hue circle. The picture is radially symmetric:
 * a lens of light whose zones tighten towards a rim.
 *
 * Fidelity:   identity
 * Cell:       Square / Clamp / WaveRing / OKLabColorRotation
 * Nearest:    square_fresnel_edge_diffraction (an edge, not a zone plate);
 *             gabor_zone_plate_hologram_lattice (z^2 folded on Torus). Here
 *             the bare map and a radial seed give the textbook zone plate.
 * Height:     age = passes; generation n is the r^(2^n) chirp.
 * Hue clock:  bounded; step 1/4 +- 0.01.
 * Fastest lane: 0.02 Hz
 * Status:     designed by derivation on 2026-11-17. Not built, rendered or
 *             measured.
 * @assert sing[0].period == 1 && sing[0].rho_abs <= 0.01
 */
inline Pattern square_minimal_fresnel_zone_plate_from_a_ring_wave() {
  Pattern p;
  p.name = "square_minimal_fresnel_zone_plate_from_a_ring_wave";
  p.inner = InnerFunction::Square;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.25f;
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.94f;
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

  p.seed.type = SeedType::WaveRing;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.55f;
  p.seed.hue_speed = 0.02f;
  p.seed.alpha = 0.6f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("zone_hue_drift",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.006f,
                            .amplitude = 0.01f,
                            .offset = 0.25f},
           control::ModTarget::HueRot)
      .add("relief_depth",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.02f,
                            .amplitude = 0.025f,
                            .offset = 0.05f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
