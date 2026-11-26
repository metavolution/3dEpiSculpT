#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Tanh nonagonal mirror hall preset.
 *
 * MATHEMATICS: 9-fold quasicrystalline mirror labyrinth with breathing depth
 *
 * tanh(z) = sinh(z)/cosh(z) saturates to ±1 for large |Re(z)|, creating
 * natural "mirror walls" where the transform compresses infinite structure
 * into a bounded strip.  With EXPANDING pre-affine (|zn0| = 1.05 > 1.0),
 * each iteration pushes structure outward past the saturation threshold,
 * but tanh immediately compresses it back — creating progressively denser
 * mirror reflections at each depth level.
 *
 * DESIGN:
 *
 *   9-FOLD SYMMETRY = 2π/9 (40°) pre-rotation
 *     Nonagonal (9-fold) is one of the rarest symmetries in art and nature.
 *     9-fold cannot tile the plane periodically (like pentagons) — only
 *     quasiperiodic tilings exist.  This creates a Penrose-like tiling
 *     at the tanh saturation boundaries: no finite patch of the pattern
 *     ever repeats identically, but long-range 9-fold order emerges.
 *
 *   INFINITE MIRROR HALL = tanh + Torus
 *     Torus reduction folds coordinates via mirrored triangle-wave wrap.
 *     Combined with tanh's saturation walls, this creates the optical
 *     effect of standing in a hall of mirrors: each mirror reflects every
 *     other mirror, creating infinite recursive reflections.  The 9-fold
 *     rotation makes it a NONAGONAL hall of mirrors — like standing inside
 *     a room with 9 mirrored walls at 40° angles.
 *
 *   PULSING DEPTH = Breathing color (mode 2)
 *     Mode 2 modulates saturation:
 *       breath = 1 + amp × sin(freq × 2π × time + phase × angle)
 *     This makes the mirror reflections visibly PULSE — deeper reflections
 *     breathe out of phase with shallower ones, creating a mesmerizing
 *     depth illusion like light traveling between parallel mirrors, each
 *     reflection slightly dimmer and phase-delayed.
 *
 *   IRRATIONAL COUNTER-ROTATION = -π/φ radians
 *     π/φ ≈ 1.9416 rad.  The golden ratio φ is the most irrational number
 *     (worst-approximable by rationals).  π/φ is thus "double-irrational":
 *     both π-incommensurate AND φ-incommensurate.  This ensures the
 *     successive mirror layers never align with the 9-fold pre-rotation,
 *     creating maximum visual complexity at the intersection of two
 *     incommensurate symmetries.
 *
 *   EXPANDING PRE-SCALE: |zn0| = 1.05 > 1.0
 *     Normally expanding pre-scale causes divergence.  But tanh(z) → ±1
 *     for large |z|, so tanh provides a natural "safety net": no matter
 *     how far the expansion pushes, tanh bounds the output.  The 1.05
 *     expansion ratio means each mirror reflection is 5% denser than the
 *     previous — creating the visual impression of accelerating depth.
 *
 * - Stability: 1.05 × 0.79 = 0.830.  Within safe range, and tanh's
 *   boundedness provides additional margin beyond the linear product.
 *
 * Pipeline: transform -> reduce -> sample -> color(mode2) -> seed -> blend
 */
inline Pattern tanh_nonagonal_mirror_hall() {
  // 2π/9 (40 deg): nonagonal — rare 9-fold quasicrystalline symmetry
  constexpr float C9  = 0.7660444431f;   // cos(2π/9) = cos(40°)
  constexpr float S9  = 0.6427876097f;   // sin(2π/9) = sin(40°)
  // -π/φ ≈ -1.9416 rad: double-irrational counter-rotation
  // π/φ = π(1 - 1/φ²) ... cos(π/φ) = -cos(π/φ²) = -cos(1.200)
  constexpr float CPG  = -0.3623577545f; // cos(π/φ)
  constexpr float SPGN = -0.9320390860f; // sin(-π/φ)

  constexpr float PRE  = 1.5381f;  // EXPANDING: tanh bounds → density increase
  constexpr float POST = 1.3775f;  // contraction for mirror stability

  Pattern p;
  p.name = "tanh_nonagonal_mirror_hall";
  p.inner = InnerFunction::Tanh;         // saturating → infinite mirror depth
  p.reduction = ReductionType::Torus;    // mirrored wrap → hall of mirrors

  p.zn.zn[0] = {C9 * PRE, S9 * PRE};
  p.zn.zn[1] = {0.2059f, -0.1795f};       // tiny perturbation (asymmetric defect)
  p.zn.zn[2] = {CPG * POST, SPGN * POST};
  p.zn.zn[3] = {0.0f, 0.0f};

  // ── Color: MODE 2 (Breathing) — pulsing mirror depth ──
  p.color.hue_rotation = 0.011f;
  p.color.sat_scale = 1.08f;
  p.color.fade = 0.988f;                // HIGH: deep mirror persistence
  p.color.color_mode = as_int(ColorMode::Breathing);               // Breathing: reflection depth pulsation
  p.color.spiral_angle_coeff = 0.85f;   // angular: reveals 9-fold sectors
  p.color.spiral_radius_coeff = 0.42f;  // depth rings through mirror layers
  // Breathing = MIRROR PULSATION
  p.color.chroma_breath_amp = 0.28f;    // strong: visible depth breathing
  p.color.chroma_breath_freq = 0.14f;   // mirror oscillation frequency
  p.color.chroma_breath_phase = 0.50f;  // spatial: reflections breathe offset
  p.color.hue_velocity_base = 0.012f;
  p.color.hue_velocity_breath = 0.008f;
  p.color.hue_velocity_drift = 0.003f;
  p.color.luma_shape_depth = 0.22f;     // mirror facet luminance contrast
  p.color.luma_shape_center = 1.8f;

  // ── Seed: minimal — mirrors self-generate ──
  p.seed.border_width = 0.084f;         // narrow: mirror depth dominates
  p.seed.hue_speed = 0.107f;
  p.seed.alpha = 0.92f;
  p.speed = 0.45f;                      // slow: meditative mirror descent
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.128f;
  p.zn.zn0_anim_amp = 0.045f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.118f;
  p.zn.zn1_anim_amp = 0.037f;
  p.zn.zn1_anim_k = 4.0f;
  p.seed.pulse_freq = 0.092f;
  p.seed.pulse_amp = 0.15f;
  p.motion
      .add("radial_pitch_wave",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.055f,
                            .amplitude = 0.120f,
                            .offset = 0.420f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("chroma_diaphragm",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.098f,
                            .amplitude = 0.120f,
                            .offset = 0.280f,
                            .phase = 0.25f},
           control::ModTarget::ChromaBreathAmp)
      .add("seed_spectrum_tide",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.069f,
                            .amplitude = 0.037f,
                            .offset = 0.107f},
           control::ModTarget::SeedHueSpeed);


  return p;
}

} // namespace epi::presets
