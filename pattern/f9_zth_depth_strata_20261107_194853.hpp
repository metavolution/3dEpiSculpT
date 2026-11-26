#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * F9 z·tanh Depth Strata — Mode-5 (DepthEcho) showcase for GPU inner ID 29.
 *
 *      f(z) = 0.8·z·tanh(z)
 *
 * THIRD COMPANION to f9_zth_basin_pulse (Mode 3) and f9_zth_breathing_basin (Mode 2).
 * Together they form the F9 three-way color tour:
 *
 *   basin_pulse:     Mode 3 (Alive)     — velocity-driven hue: kinetic energy
 *   breathing_basin: Mode 2 (Breathing) — chroma envelope: slow inhale/exhale
 *   depth_strata:    Mode 5 (DepthEcho) — temporal strata: history made visible
 *
 * MATHEMATICS: DIFFERENTIAL CONVERGENCE → THREE-LEVEL CHROMATIC GEOLOGY
 *
 * f(z) = 0.8·z·tanh(z) factors into two dynamical regimes joined at the sigmoid:
 *
 *   BASIN FLOOR (|z| ≈ 0):  tanh(z) ≈ z, so f(z) ≈ 0.8·z² — super-stable fixed point.
 *       Iterates reach z=0 in 2–4 steps.  Per-pixel alpha c.w stays thin.
 *       Depth Echo reads this as the YOUNGEST, FRESHEST LAYER — the geological surface.
 *
 *   BASIN RIM (tanh transition zone):  convergence slows as tanh shifts from quadratic
 *       to linear.  c.w accumulates over 10–30 frames.  Reads as a MID-STRATA BAND —
 *       a sedimentary horizon between fresh surface and deep history.
 *
 *   TANH POLES (Im(z) = π/2 + nπ, filament zone):  tanh has branch-cut singularities
 *       that project as long-range filaments through the iterated field.  Iterates
 *       in the filament zone linger for 50+ frames.  c.w saturates to maximum depth.
 *       Depth Echo reads this as the OLDEST, DEEPEST STRATUM — glowing, saturated,
 *       hue-shifted far from the fresh basin floor.
 *
 * This INVERTED depth signature distinguishes F9 from the other DepthEcho presets:
 *   tanh_temporal_strata_echo: uniform tanh convergence → monotone gradient, periphery deep
 *   tp6_radiant_spiral:        exp growth → shell-banded depth, exterior deep
 *   f9_zth_depth_strata:       three DISCRETE strata — floor/rim/filament — each with
 *                              its own characteristic depth and chromatic band.
 *
 * COLOR MODE 5 — DEPTH ECHO:
 *
 *   h_spatial = hv_drift × (spiral_a × angle  +  spiral_r × log|z|)
 *   h_depth   = (1 − hv_drift) × spiral_a × c.w
 *   hue_shift = hue_time + h_spatial + h_depth + |z|
 *   brightness = 1 + chroma_breath_amp × c.w       ← old strata glow bright
 *   saturation += hv_base × c.w                    ← deep filaments saturate
 *
 * hv_drift = 0.15 (85% depth-driven): the depth channel is almost all of the hue.
 * The spatial component (15%) provides a weak angular anchor so the strata read as
 * spatially coherent rather than purely temporal noise.
 *
 * AFFINE GEOMETRY — CLOCKWISE MIRROR OF breathing_basin:
 *
 * breathing_basin tilts at +π/11 (counter-clockwise 16.4°).
 * This preset mirrors it at −π/11 (clockwise −16.4°) — both share the same 11-fold
 * angular resonance (π/11 prime-fold symmetry) but orbit the F9 attractor in opposite
 * angular directions.  When cycling via next/prev_companion_of, the tilt reversal
 * reads as the field "unwinding" in the opposite sense, making the transition feel
 * like a smooth rotation reversal rather than an arbitrary parameter jump.
 *
 * PRE = 0.96 (lighter than breathing_basin's 0.93): less contraction → iterates
 * linger longer in the transition zone → more alpha accumulates in the rim stratum.
 *
 * −1/φ² rad POST-ANGLE (≈ −0.382 rad, golden inverse-square, same as breathing_basin):
 * irrational post-rotation ensures the depth strata never phase-lock into a periodic
 * pattern — quasiperiodic stacking gives maximum visible geological complexity.
 *
 * Pipeline: transform → reduce(torus) → sample → color(mode5) → seed → blend
 */
inline Pattern f9_zth_depth_strata() {
  // -π/11 (clockwise 16.4°): mirror of breathing_basin's +π/11 counter-clockwise tilt
  constexpr float C11N   =  0.9594929736f; // cos(-π/11) = cos(π/11)
  constexpr float SN11N  = -0.2817325568f; // sin(-π/11) = -sin(π/11)

  // -1/φ² rad (≈ -0.382): golden inverse-square irrational post-angle
  constexpr float CR_INVPHI2  =  0.9279386812f; // cos(1/φ²)
  constexpr float SR_INVPHI2N = -0.3727282745f; // sin(-1/φ²) = -sin(1/φ²)

  constexpr float PRE  = 1.6061f; // lighter than breathing_basin's 0.93: more rim lingering
  constexpr float POST = 1.4763f; // same post-scale as breathing_basin

  Pattern p;
  p.name = "f9_zth_depth_strata";
  // Named-enum path: InnerFunction::F9 ↔ id 29 (0.8·z·tanh(z))
  p.inner = InnerFunction::F9;
  p.extended_inner_id = -1;
  p.reduction = ReductionType::Torus;

  // Clockwise -16.4° pre-tilt: opposite orbit direction from breathing_basin (+16.4°)
  p.zn.zn[0] = {C11N * PRE,  SN11N * PRE};
  // Upper-left translate: third quadrant from origin — different basin bias than siblings
  p.zn.zn[1] = {0.2368f, -0.1510f};
  // -1/φ² post-rotation: irrational → depth strata never phase-lock
  p.zn.zn[2] = {CR_INVPHI2 * POST, SR_INVPHI2N * POST};
  p.zn.zn[3] = {0.0f, 0.0f};

  // ── Color: MODE 5 (Depth Echo) — three-layer geology: floor / rim / filament ──
  p.color.hue_rotation        = 0.007f;  // slow drift: strata are the temporal signal
  p.color.sat_scale           = 1.14f;   // modest global lift: depth echo adds own saturation
  p.color.fade                = 0.982f;  // very high persistence: strata need many frames
  p.color.color_mode          = as_int(ColorMode::DepthEcho); // temporal stratification
  p.color.spiral_angle_coeff  = 1.10f;   // π/11 resonance: 11 angular strata bands
  p.color.spiral_radius_coeff = 0.55f;   // log_r rings: basin transition zone reads as rings
  p.color.chroma_breath_amp   = 0.18f;   // strong: old filament strata glow bright
  p.color.chroma_breath_freq  = 0.0f;    // not oscillatory in mode 5
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base   = 0.0f;   // depth → saturation: deep filaments saturate
  p.color.hue_velocity_breath = 0.0f;    // unused in mode 5
  p.color.hue_velocity_drift  = 0.15f;   // 15% spatial / 85% depth-driven hue
  // Kernel gates luma shaping on color_mode < 4; mode 5 uses its own brightness
  // via hv_base above — setting non-zero here would be silently ignored.
  p.color.luma_shape_depth    = 0.0f;
  p.color.luma_shape_center   = 0.0f;

  // ── Seed: cool anchor — contrasts with warm geological strata glow ──
  p.seed.border_width = 0.085f;
  p.seed.hue          = 0.65f;   // blue-violet seed: cold surface against warm deep echo
  p.seed.hue_speed    = 0.098f;
  p.seed.alpha        = 0.92f;

  p.speed = 0.22f;  // slow: alpha needs many frames to stratify into three visible layers
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.120f;
  p.zn.zn0_anim_amp = 0.050f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.134f;
  p.zn.zn1_anim_amp = 0.046f;
  p.zn.zn1_anim_k = 3.0f;
  p.seed.pulse_freq = 0.092f;
  p.seed.pulse_amp = 0.20f;
  p.motion
      .add("radial_pitch_wave",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.072f,
                            .amplitude = 0.154f,
                            .offset = 0.550f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("chroma_diaphragm",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.119f,
                            .amplitude = 0.103f,
                            .offset = 0.180f,
                            .phase = 0.25f},
           control::ModTarget::ChromaBreathAmp)
      .add("seed_spectrum_tide",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.054f,
                            .amplitude = 0.034f,
                            .offset = 0.098f},
           control::ModTarget::SeedHueSpeed);


  return p;
}

} // namespace epi::presets
