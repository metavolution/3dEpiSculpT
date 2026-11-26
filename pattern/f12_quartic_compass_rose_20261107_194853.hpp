#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * F12 Quartic Compass Rose — showcase for GPU inner ID 32.
 *
 *      f(z) = 0.7·(z − 1)·(z + 1)·(z − i)·(z + i) = 0.7·(z⁴ − 1)
 *
 * MATHEMATICS: FOURTH-ROOT-OF-UNITY BASIN STRUCTURE
 *
 * The factorisation makes the geometry obvious: the polynomial vanishes at
 * the four 4th-roots of unity {+1, −1, +i, −i}, which sit at the cardinal
 * directions of the complex plane like the points of a *compass rose*.
 * Each root is a point of zero magnitude in f, around which the local
 * dynamics behave like a Newton-style basin attractor.
 *
 * Iterating f on a contracted grid produces FOUR INTERLOCKING BASINS — one
 * per cardinal — separated by a fractal Julia-style boundary.  The boundary
 * itself has 4-fold (D₄) symmetry: 90° rotational + reflection.  Every
 * starting iterate eventually classifies into one of the four basins (or
 * lives forever on the boundary fractal); the basin-labelling tessellates
 * the plane into four pinwheel sectors.
 *
 * Quartic growth makes f extremely sensitive to magnitude: |z| > ~1.4 sends
 * iterates rocketing past escape radius within 2–3 steps.  Aggressive zn0
 * contraction is mandatory to keep the visible field inside the basin
 * structure rather than in the diverging exterior.
 *
 * COLOR MODE 4 — SPATIAL WAVE:
 *
 *   hue_shift ← hue_time + spatial_wave_phase(z)
 *
 * The spatial-wave hue map assigns each spatial position a phase color
 * drawn from a wave field.  Against the four-basin attractor this reads
 * beautifully: each basin is naturally cohesive (iterates cluster), so
 * each basin acquires a *characteristic* hue band, while the fractal
 * boundary becomes a high-frequency phase-jump where wave bands cross —
 * a thin filigree of rainbow lacework outlining the compass arms.
 *
 * AFFINE GEOMETRY:
 *
 * zn0 = (0.55, 0.0): aggressive contraction, no rotation.  The contraction
 * is non-negotiable for quartic growth.  Zero rotation preserves the
 * native D₄ symmetry → all four arms read equally.
 *
 * zn1 = (0.0, 0.0): no translate.  We *want* the compass centered on the
 * basin separator (z = 0); off-center placement biases one quadrant.
 *
 * zn2 = (0.78, 0.05): post-contraction with mild +2.9° tilt.  The tilt
 * breaks perfect D₄ symmetry just enough to add visible angular drift to
 * the boundary fractal — without the tilt the pattern is statically
 * mirror-symmetric and reads as a frozen wallpaper.
 *
 * Pipeline: transform → reduce(torus) → sample → color(mode4) → seed → blend
 */
inline Pattern f12_quartic_compass_rose() {
  Pattern p;
  p.name = "f12_quartic_compass_rose";
  // Named-enum path: InnerFunction::F12 ↔ GPU id 32 (0.7·(z⁴−1))
  p.inner = InnerFunction::F12;
  p.extended_inner_id = -1;
  p.reduction = ReductionType::Torus;

  // Aggressive contraction, zero rotation: tame z⁴ growth, preserve D₄ axes
  p.zn.zn[0] = {1.4922f, 0.2408f};
  // No translate: keep compass center on the basin separator (origin)
  p.zn.zn[1] = {0.1826f, -0.1767f};
  // Mild post-contraction with +2.9° tilt: breaks static symmetry
  p.zn.zn[2] = {1.3721f, 0.0880f};
  p.zn.zn[3] = {0.0f,   0.0f};

  // ── Color: MODE 4 (Spatial Wave) — basin-cohesive hues, lacework rim ──
  p.color.hue_rotation      = 0.005f;   // slow base — let spatial wave dominate
  p.color.sat_scale         = 1.20f;    // vivid: each basin saturated
  p.color.fade              = 0.990f;   // medium persistence: lacework refreshes
  p.color.color_mode        = as_int(ColorMode::SpatialWave);        // Spatial Wave
  p.color.spiral_angle_coeff  = 1.30f;
  p.color.spiral_radius_coeff = 0.95f;  // log_r reads as concentric basin arcs
  p.color.chroma_breath_amp   = 0.22f;
  p.color.chroma_breath_freq  = 0.70f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base   = 0.36f;
  p.color.hue_velocity_breath = 0.10f;
  p.color.hue_velocity_drift  = 0.45f;  // moderate: spatial wave is the lead
  p.color.luma_shape_depth    = 0.24f;  // pronounced depth: basin floors recede
  p.color.luma_shape_center   = 1.5f;

  // ── Seed: fine border — the boundary lacework is the visual signature ──
  p.seed.border_width = 0.079f;
  p.seed.hue          = 0.42f;   // teal seed: contrasts with rotating wave hues
  p.seed.hue_speed    = 0.127f;
  p.seed.alpha        = 0.92f;

  p.speed = 0.34f;

  // ── Motion: chromatic saturation pulse — basins breathe in/out of colour ──
  // Four-basin D₄ structure already provides visual rhythm through the compass
  // arms; sat_pulse adds a perpendicular temporal rhythm (12 s) so the hue
  // intensity of all four basins crests and troughs in unison. Combined with
  // the preset's spatial wave hue map, it reads as each compass arm glowing
  // and fading like four synchronized bioluminescent elements.
  p.motion = epi::control::PatternMotion::saturation_pulse(1.20f, 0.06f, 12.0f);
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.182f;
  p.zn.zn0_anim_amp = 0.038f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.148f;
  p.zn.zn1_anim_amp = 0.039f;
  p.zn.zn1_anim_k = 3.0f;
  p.seed.pulse_freq = 0.093f;
  p.seed.pulse_amp = 0.15f;


  return p;
}

} // namespace epi::presets
