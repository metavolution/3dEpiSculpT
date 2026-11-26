#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * F12 Quartic Spiral Pinwheel — Mode-1 (Spiral) showcase for GPU inner ID 32.
 *
 *      f(z) = 0.7·(z − 1)·(z + 1)·(z − i)·(z + i) = 0.7·(z⁴ − 1)
 *
 * COMPANION TO f12_quartic_compass_rose:
 *
 * The compass_rose preset reveals F12's four-basin attractor through Mode 4
 * (Spatial Wave) — basin-cohesive hue bands with rainbow lacework boundaries.
 * This preset reveals the SAME math through Mode 1 (Spiral) — radial depth
 * banding and angular sectors driven by `h_spiral = a·θ + r·log(|z|)`.
 *
 * Why a second Mode-1 preset around F12 is worth landing:
 *   - F12 is the only INNER_FUNCTIONS entry whose primary dynamic feature is
 *     true 4-fold rotational symmetry (Square gives 2-fold, Cube gives 3-fold,
 *     all others are non-rotational).  Mode 1's `angle → hue` channel maps
 *     each of F12's four basins to a distinct hue sector → the "4 petals"
 *     read explicitly as four colored sector blades, like a literal pinwheel.
 *   - Mode 1 (Spiral) is under-represented in the catalog (a small handful of
 *     presets vs Modes 3/4 dominance).  Spiral's strength is depth-revealing
 *     `log(r)` ring banding, which composes naturally with F12's Newton-style
 *     basin-of-attraction depth structure: each basin shows concentric "depth
 *     rings" of approach toward its root attractor.
 *
 * AFFINE GEOMETRY:
 *
 * π/8 (22.5°) PRE-TILT — half the natural 4-fold angle.  Aligning to the
 * cardinal axes (no rotation, like compass_rose) reads as a static compass;
 * tilting by 22.5° offsets the petals so the four sectors catch the spatial
 * gradient asymmetrically and read as rotating blades.  The effective
 * 4-fold symmetry is preserved (since π/8 is rationally compatible with the
 * 4-fold root structure under D₄ rotations).
 *
 * zn0 = (PRE·cos(π/8), PRE·sin(π/8)) with PRE=0.55: aggressive contraction
 * is non-negotiable for quartic growth (proven in compass_rose).  The 22.5°
 * tilt is the only difference from compass_rose's pre-affine.
 *
 * −1/φ rad (≈ −0.618, golden-ratio reciprocal) POST-COUNTER-ROTATION:
 * irrational counter-rotation per design-guide §10's "irrational post-
 * rotation catalog".  Successive depth layers (in Mode 1's log-radius hue
 * field) misalign by a quasiperiodic angle → the radial ring banding never
 * exactly repeats → maximum visible depth complexity.
 *
 * zn2 = (POST·cos(−1/φ), POST·sin(−1/φ)) with POST=0.78: same post-scale as
 * compass_rose, contraction product 0.55·0.78 = 0.429 (well-bounded for
 * unbounded quartic growth).
 *
 * COLOR MODE 1 — SPIRAL (angle + log-radius hue field):
 *
 *   h_spiral = spiral_angle_coeff · angle  +  spiral_radius_coeff · log(|t_z|)
 *
 *   spiral_angle_coeff = 1.20  → reveals all 4 D₄ sectors as distinct hues.
 *                                Lower than decagonal_spiral_abyss's 1.30
 *                                because 4 sectors are wider than 10 — a
 *                                lower coeff still gives clean separation.
 *   spiral_radius_coeff = 0.50 → concentric depth-ring banding around each
 *                                basin attractor.  Each basin shows ring
 *                                bands like a tree's growth rings, marking
 *                                successive iterations of approach.
 *
 *   Mode 1 ignores chroma_breath_* and hue_velocity_* params (they are set
 *   to 0 explicitly to make the design intent unambiguous).
 *
 *   luma_shape_depth = 0.22 → visible darkening near each basin floor and
 *   brightening at the basin rims, accentuating the four-petal structure.
 *
 * Pipeline: transform → reduce(torus) → sample → color(mode1) → seed → blend
 */
inline Pattern f12_quartic_spiral_pinwheel() {
  // π/8 (22.5°) pre-tilt: half-tilted 4-fold compass, breaks static read
  constexpr float C8 = 0.9238795325f; // cos(π/8)
  constexpr float S8 = 0.3826834324f; // sin(π/8)

  // -1/φ rad (≈ -0.618): golden-ratio reciprocal, irrational counter-rotation
  constexpr float CR_INVPHI  = 0.8146601949f;  // cos(-1/φ) =  cos(1/φ)
  constexpr float SR_INVPHIN = -0.5797944747f; // sin(-1/φ) = -sin(1/φ)

  constexpr float PRE  = 0.55f; // aggressive: tame quartic growth (proven in compass_rose)
  constexpr float POST = 0.78f; // same post-scale as compass_rose

  Pattern p;
  p.name = "f12_quartic_spiral_pinwheel";
  // Named-enum path: InnerFunction::F12 ↔ GPU id 32 (0.7·(z⁴−1))
  p.inner = InnerFunction::F12;
  p.extended_inner_id = -1;
  p.reduction = ReductionType::Torus; // smooth wrap → no tile boundaries on the petals

  p.zn.zn[0] = {C8 * PRE, S8 * PRE};
  p.zn.zn[1] = {-0.003f, 0.005f}; // tiny perturbation: breaks perfect static symmetry
  p.zn.zn[2] = {CR_INVPHI * POST, SR_INVPHIN * POST};
  p.zn.zn[3] = {0.0f, 0.0f}; // keep pinwheel center on the basin separator

  // ── Color: MODE 1 (Spiral) — 4 sector hues + concentric depth rings ──
  p.color.hue_rotation        = 0.008f; // very slow base: let spiral hue dominate
  p.color.sat_scale           = 1.10f;  // vivid sectors
  p.color.fade                = 0.984f; // medium-long: depth rings remain visible
  p.color.color_mode          = as_int(ColorMode::Spiral);      // Spiral: angle → sector hues, log_r → rings
  p.color.spiral_angle_coeff  = 1.20f;  // STRONG: all 4 sectors read as distinct hues
  p.color.spiral_radius_coeff = 0.50f;  // concentric depth-ring banding per basin
  // Mode 1 ignores these — set to 0 to make intent explicit
  p.color.chroma_breath_amp   = 0.0f;
  p.color.chroma_breath_freq  = 0.0f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base   = 0.0f;
  p.color.hue_velocity_breath = 0.0f;
  p.color.hue_velocity_drift  = 0.0f;
  p.color.luma_shape_depth    = 0.22f; // basin floors recede / rims glow
  p.color.luma_shape_center   = 1.6f;

  // ── Seed: moderate-narrow border — basin attractor structure dominates ──
  p.seed.border_width = 0.014f;
  p.seed.hue          = 0.55f; // cyan: contrasts with warm spiral hues
  p.seed.hue_speed    = 0.05f; // slow drift
  p.seed.alpha        = 0.80f;

  p.speed = 0.42f; // slow contemplative: depth rings need time to layer
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.169f;
  p.zn.zn0_anim_amp = 0.048f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.159f;
  p.zn.zn1_anim_amp = 0.042f;
  p.zn.zn1_anim_k = 7.0f;
  p.seed.pulse_freq = 0.109f;
  p.seed.pulse_amp = 0.21f;
  p.motion
      .add("radial_pitch_wave",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.053f,
                            .amplitude = 0.140f,
                            .offset = 0.500f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("chroma_diaphragm",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.095f,
                            .amplitude = 0.050f,
                            .offset = 0.000f,
                            .phase = 0.25f},
           control::ModTarget::ChromaBreathAmp)
      .add("seed_spectrum_tide",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.058f,
                            .amplitude = 0.030f,
                            .offset = 0.050f},
           control::ModTarget::SeedHueSpeed);


  return p;
}

} // namespace epi::presets
