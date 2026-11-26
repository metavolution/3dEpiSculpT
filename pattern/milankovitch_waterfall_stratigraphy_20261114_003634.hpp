#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Recursive Memory: Milankovitch Stratigraphy, the Loop as a Recording
 * Medium.
 *
 * Scientific foundation:
 * - Milankovitch (1941): the Earth's climate is paced by three orbital
 *   cycles, precession (about 23 kyr), obliquity (41 kyr) and eccentricity
 *   (about 100 kyr). Hays, Imbrie and Shackleton (1976) found all three in
 *   the oxygen-isotope record of deep-sea cores. A core is a time series
 *   stored as depth: accumulation writes at the top, burial carries the
 *   record down, and diffusion blurs the oldest layers (in ice, isotopic
 *   diffusion erases annual layers with depth).
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Inner = Identity, zn0 = 1, zn3 = (0, 0.01), Reduce = Grid: T is a pure
 *   translation. Each frame every pixel copies the one 0.01 above it, and
 *   the row that would read beyond the top wraps into the seed band. The
 *   frame scrolls down 12.8 pixels per frame (at 2560) and takes 200 frames
 *   to cross: height is time, 3.3 s from top to bottom. No transform, no fold: the
 *   only thing on screen is the history of the seed.
 * - Motion writes the record. Three Sine lanes with periods in the ratio
 *   23 : 41 : 100 drive the seed hue (precession), the seed opacity
 *   (obliquity, read as layer brightness) and a small sideways drift
 *   (eccentricity), which shears the layers into slow chevrons the way ice
 *   flow tilts strata.
 * - Blur with depth is real, not drawn: a 12.8-pixel step is re-sampled
 *   bilinearly 200 times, and each re-sampling diffuses the image slightly.
 *   The oldest layers are the softest.
 * - Color = Classic. Every accumulating mode adds 0.01 |T| of hue per pass,
 *   which over 200 passes would bury the record under a rainbow;
 *   hue_rotation = -0.007 cancels most of it, leaving a slow drift of
 *   colour with depth on which the recorded cycles ride.
 * - What it is not: a climate model. The three sinusoids are the forcing
 *   alone, with no ice-sheet response and no 100 kyr nonlinearity.
 */
inline Pattern milankovitch_waterfall_stratigraphy() {
  constexpr float BURIAL_STEP = 0.01f;
  constexpr float PRECESSION_HZ = 0.60f;  // 23 kyr
  constexpr float OBLIQUITY_HZ = 0.3366f; // 41 kyr
  constexpr float ECCENTRICITY_HZ = 0.138f; // 100 kyr
  constexpr float SEED_HUE = 0.55f;
  constexpr float SEED_ALPHA = 0.75f;

  Pattern p;
  p.name = "milankovitch_waterfall_stratigraphy";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Grid;
  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.0f, BURIAL_STEP};

  p.color.hue_rotation = -0.007f; // cancels the built-in 0.01 |T| hue advance
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.99f;
  p.color.color_mode = as_int(ColorMode::Classic);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 0.0f;
  p.color.chroma_breath_amp = 0.0f;
  p.color.chroma_breath_freq = 0.1f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f; // bounded hue clock
  p.color.hue_velocity_breath = 0.0006f;
  p.color.hue_velocity_drift = 0.0002f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.0f;

  p.seed.type = SeedType::Simplex;
  p.seed.border_width = 0.06f;
  p.seed.hue = SEED_HUE;
  p.seed.hue_speed = 0.0f; // the lanes are the only clock
  p.seed.alpha = SEED_ALPHA;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 1.0f;

  p.motion
      .add("precession_23kyr_hue",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = PRECESSION_HZ,
                            .amplitude = 0.10f,
                            .offset = SEED_HUE},
           control::ModTarget::SeedHue)
      .add("obliquity_41kyr_accumulation",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = OBLIQUITY_HZ,
                            .amplitude = 0.20f,
                            .offset = SEED_ALPHA},
           control::ModTarget::SeedAlphaBase)
      .add("eccentricity_100kyr_shear",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = ECCENTRICITY_HZ,
                            .amplitude = 0.0012f,
                            .offset = 0.0f},
           control::ModTarget::Zn3Re);

  return p;
}

} // namespace epi::presets
