#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <algorithm>
#include <cstdio>
#include <span>

#include "mgfx/Drawable.h"
#include "mgfx/math/Types.h"
#include "SculptureMorphSpace.h"
#include "SculpturePerformanceDeck.h"
#include "SculpturePerformanceTelemetry.h"
#include "SculptureLearnMode.h"
#include "SculptureStagedComputeGovernor.h"

namespace epi_sculpt {

// -----------------------------------------------------------------------------
// Educational Annotation: ZERO-OVERHEAD MODULAR ASCII TELEMETRY WIDGETS
// -----------------------------------------------------------------------------
// In high-performance GPU real-time visualization systems (OpenCL compute +
// Vulkan presentation), the user HUD is a critical instrumentation surface.
//
// Traditional graphical UI overlays (e.g. ImGui or heavy retained-mode UIs)
// introduce:
//   1. Substantial vertex/index buffer traffic and pipeline state switches.
//   2. Complex memory allocation patterns in the hot frame loop.
//   3. Tight platform coupling (event interception, mouse capture, font atlases).
//
// Modular ASCII widgets format directly into stack-allocated fixed-size buffers
// and are rasterized through a single instanced monospace text draw call.
// Each widget is an independent, pure visual synthesizer that maps complex
// internal state (multidimensional foliation manifolds, equal-power crossfaders,
// 6-stage pipeline decompositions, closed-loop governor headroom) into human-
// readable pedagogical instruments.
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// 1. Multidimensional 2D Morph Space Vector Synthesis Pad Widget
// -----------------------------------------------------------------------------
// Displays corner anchor presets, Shepard Inverse Distance Weighting (IDW)
// exponents, normalized dynamic weights, cursor position (X, Y), and autonomous
// motion kinematics (Lissajous, Brownian, Orbital, Bounce).
struct MorphPadWidget {
    static void format(
        char* buffer,
        std::size_t size,
        const SculptureMorphSpace& morphSpace
    ) noexcept {
        if (!buffer || size == 0) return;
        const float cx = morphSpace.cursorX();
        const float cy = morphSpace.cursorY();
        const auto weights = morphSpace.computeWeights();
        const float wSW = (weights.size() > 0) ? weights[0] : 0.0f;
        const float wSE = (weights.size() > 1) ? weights[1] : 0.0f;
        const float wNW = (weights.size() > 2) ? weights[2] : 0.0f;
        const float wNE = (weights.size() > 3) ? weights[3] : 0.0f;

        const char* nwTitle = morphSpace.node(2) ? morphSpace.node(2)->title.c_str() : "NW";
        const char* neTitle = morphSpace.node(3) ? morphSpace.node(3)->title.c_str() : "NE";
        const char* swTitle = morphSpace.node(0) ? morphSpace.node(0)->title.c_str() : "SW";
        const char* seTitle = morphSpace.node(1) ? morphSpace.node(1)->title.c_str() : "SE";

        const std::string_view bankTitle = morphSpace.activeBankTitle();
        std::snprintf(buffer, size,
            "┌─── 2D MORPH PAD [%.*s | %s | Spd: %.2fx | p: %.1f] ───┐\n"
            "│ NW: %-14.14s (%.2f)   NE: %-14.14s (%.2f) │\n"
            "│ Cursor: [%.2f, %.2f]                  │\n"
            "│ SW: %-14.14s (%.2f)   SE: %-14.14s (%.2f) │\n"
            "└──────────────────────────────────────┘",
            static_cast<int>(std::min<std::size_t>(bankTitle.size(), 16U)), bankTitle.data(),
            morphSpace.motionModeName().data(),
            morphSpace.motionSpeed(),
            morphSpace.shepardPower(),
            nwTitle, wNW,
            neTitle, wNE,
            cx, cy,
            swTitle, wSW,
            seTitle, wSE
        );
    }

    [[nodiscard]] static std::string formatToString(const SculptureMorphSpace& morphSpace) {
        char buf[512] = {0};
        format(buf, sizeof(buf), morphSpace);
        return std::string(buf);
    }

    static void render(
        mgfx::Drawable& draw,
        const SculptureMorphSpace& morphSpace,
        const mgfx::math::vec2& position,
        float fontSize = 18.0f,
        const mgfx::math::vec4& color = mgfx::math::vec4(0.35f, 0.92f, 1.0f, 0.95f)
    ) noexcept {
        char buf[512];
        format(buf, sizeof(buf), morphSpace);
        draw.drawText(buf, position, fontSize, color);
    }
};

// -----------------------------------------------------------------------------
// 2. VJ Performance Deck Master Crossfader Console Widget
// -----------------------------------------------------------------------------
// Visualizes Deck A and Deck B active preset titles, analytical blending gains,
// 16-segment graphic slider bar with center detent, crossfade curve law, and
// beat-synced / downbeat-quantized transition progress.
//
// Math Invariant:
//   Equal-Power: gainA^2 + gainB^2 = 1.0 (constant acoustic/visual power)
//   Linear:      gainA   + gainB   = 1.0 (constant amplitude)
struct PerformanceDeckWidget {
    static void format(
        char* buffer,
        std::size_t size,
        const SculpturePerformanceDeck& deck
    ) noexcept {
        if (!buffer || size == 0) return;
        const float pos = deck.crossfadePosition();
        const auto [gainA, gainB] = SculpturePerformanceDeck::computeDeckGains(pos, deck.crossfadeCurve());

        // 16-segment crossfader visual bar
        constexpr int kBarWidth = 16;
        char bar[kBarWidth + 2];
        const int marker = std::clamp(static_cast<int>(pos * kBarWidth), 0, kBarWidth);
        for (int i = 0; i <= kBarWidth; ++i) {
            bar[i] = (i == marker) ? '|' : '=';
        }
        bar[kBarWidth + 1] = '\0';

        const char* curveStr = "Smooth";
        switch (deck.crossfadeCurve()) {
        case CrossfadeCurve::Linear: curveStr = "Linear"; break;
        case CrossfadeCurve::EqualPower: curveStr = "EqPower"; break;
        case CrossfadeCurve::Smoothstep: curveStr = "Smooth"; break;
        case CrossfadeCurve::SharpCut: curveStr = "Cut"; break;
        }

        const char* titleA = deck.deck(DeckId::DeckA).presetTitle.empty()
            ? "Deck A" : deck.deck(DeckId::DeckA).presetTitle.c_str();
        const char* titleB = deck.deck(DeckId::DeckB).presetTitle.empty()
            ? "Deck B" : deck.deck(DeckId::DeckB).presetTitle.c_str();

        char statusStr[64] = "MANUAL";
        if (deck.isPendingDownbeat()) {
            std::snprintf(statusStr, sizeof(statusStr), "WAIT DOWNBEAT");
        } else if (deck.isAutoCrossfading()) {
            std::snprintf(statusStr, sizeof(statusStr), "%s %u%%",
                deck.isBeatSyncCrossfade() ? "BEAT-FADE" : "AUTO-FADE",
                static_cast<unsigned>(deck.autoCrossfadeProgress() * 100.0f));
        }

        std::snprintf(buffer, size,
            "┌─── VJ PERFORMANCE DECK [BPM: %.1f | Bar %u.%u | %s] ───┐\n"
            "│ [A] %3u%% \"%-18.18s\"                         │\n"
            "│ X-FADE: [%s] Pos: %.2f (%-7s)      │\n"
            "│ [B] %3u%% \"%-18.18s\"                         │\n"
            "└───────────────────────────────────────────────────────┘",
            deck.bpm(),
            deck.currentBar(),
            deck.beatInBar(),
            statusStr,
            static_cast<unsigned>(gainA * 100.0f + 0.5f),
            titleA,
            bar,
            pos,
            curveStr,
            static_cast<unsigned>(gainB * 100.0f + 0.5f),
            titleB
        );
    }

    [[nodiscard]] static std::string formatToString(const SculpturePerformanceDeck& deck) {
        char buf[512] = {0};
        format(buf, sizeof(buf), deck);
        return std::string(buf);
    }

    static void render(
        mgfx::Drawable& draw,
        const SculpturePerformanceDeck& deck,
        const mgfx::math::vec2& position,
        float fontSize = 18.0f,
        const mgfx::math::vec4& color = mgfx::math::vec4(1.0f, 0.82f, 0.35f, 0.95f)
    ) noexcept {
        char buf[512];
        format(buf, sizeof(buf), deck);
        draw.drawText(buf, position, fontSize, color);
    }
};

// -----------------------------------------------------------------------------
// 3. Educational Learn Mode: 6-Stage Pipeline Decomposition Widget
// -----------------------------------------------------------------------------
// Visualizes the canonical 6-stage mathematical flow of the epimorphism engine:
//   1. TRANSFORM -> 2. REDUCE -> 3. SAMPLE -> 4. COLOR -> 5. SEED -> 6. BLEND
// Clearly illustrates stage isolation, bypassing, and accumulative evaluation.
struct LearnModeWidget {
    static void format(
        char* buffer,
        std::size_t size,
        std::uint32_t stageMask
    ) noexcept {
        if (!buffer || size == 0) return;
        auto isStageActive = [stageMask](learn_mode::Stage s) -> bool {
            return (stageMask & (1U << static_cast<std::uint32_t>(s))) != 0U;
        };

        std::snprintf(buffer, size,
            "┌─── LEARN MODE: 6-STAGE PIPELINE DECOMPOSITION ───┐\n"
            "│ [%c] 1. TRANSFORM : z = M(zn0,z)+zn1 -> inner(z)     │\n"
            "│ [%c] 2. REDUCE    : z = torus_reduce(z)              │\n"
            "│ [%c] 3. SAMPLE    : frame = bilinear_sample(prev, z) │\n"
            "│ [%c] 4. COLOR     : hsls_rotate(frame)               │\n"
            "│ [%c] 5. SEED      : wca_seed(z_in)                   │\n"
            "│ [%c] 6. BLEND     : accum += mix(color, seed, a)     │\n"
            "└──────────────────────────────────────────────────┘",
            isStageActive(learn_mode::Stage::Transform) ? 'X' : ' ',
            isStageActive(learn_mode::Stage::Reduce) ? 'X' : ' ',
            isStageActive(learn_mode::Stage::FeedbackSample) ? 'X' : ' ',
            isStageActive(learn_mode::Stage::Color) ? 'X' : ' ',
            isStageActive(learn_mode::Stage::Seed) ? 'X' : ' ',
            isStageActive(learn_mode::Stage::Blend) ? 'X' : ' '
        );
    }

    [[nodiscard]] static std::string formatToString(std::uint32_t stageMask) {
        char buf[512] = {0};
        format(buf, sizeof(buf), stageMask);
        return std::string(buf);
    }

    static void render(
        mgfx::Drawable& draw,
        std::uint32_t stageMask,
        const mgfx::math::vec2& position,
        float fontSize = 18.0f,
        const mgfx::math::vec4& color = mgfx::math::vec4(0.40f, 0.95f, 0.65f, 0.95f)
    ) noexcept {
        char buf[512];
        format(buf, sizeof(buf), stageMask);
        draw.drawText(buf, position, fontSize, color);
    }
};

// -----------------------------------------------------------------------------
// 4. Closed-Loop Variable-Resolution Governor & Headroom Telemetry Widget
// -----------------------------------------------------------------------------
// Displays current resolution scale tier (Full, Half, Quarter, Eighth), frame
// pacing state, target time budget, actual frame interval, and signed headroom:
//   headroomMs = targetBudgetMs - frameIntervalMs
struct GovernorWidget {
    static void format(
        char* buffer,
        std::size_t size,
        const PerformanceTelemetry& telem,
        StageResolutionScale scale,
        bool autoScalingEnabled
    ) noexcept {
        if (!buffer || size == 0) return;
        const char* scaleName = "Full";
        switch (scale) {
        case StageResolutionScale::Full: scaleName = "Full (1:1)"; break;
        case StageResolutionScale::Half: scaleName = "Half (1:2)"; break;
        case StageResolutionScale::Quarter: scaleName = "Qtr (1:4)"; break;
        case StageResolutionScale::Eighth: scaleName = "1/8 (1:8)"; break;
        }

        std::snprintf(buffer, size,
            "┌─── VARIABLE-RESOLUTION GOVERNOR [%s] ───┐\n"
            "│ Tier: %-10s | Target: %4.1f FPS (%4.1fms)   │\n"
            "│ Frame: %4.1fms | Headroom: %+5.1fms            │\n"
            "│ Status: %-12s (Hitches: %u)             │\n"
            "└─────────────────────────────────────────────────┘",
            autoScalingEnabled ? "AUTO" : "MANUAL",
            scaleName,
            telem.targetFps,
            telem.targetBudgetMs,
            telem.currentFrameTimeMs,
            telem.headroomMs,
            telem.budgetStatus.c_str(),
            telem.hitchCount
        );
    }

    [[nodiscard]] static std::string formatToString(
        const PerformanceTelemetry& telem,
        StageResolutionScale scale,
        bool autoScalingEnabled
    ) {
        char buf[512] = {0};
        format(buf, sizeof(buf), telem, scale, autoScalingEnabled);
        return std::string(buf);
    }

    static void render(
        mgfx::Drawable& draw,
        const PerformanceTelemetry& telem,
        StageResolutionScale scale,
        bool autoScalingEnabled,
        const mgfx::math::vec2& position,
        float fontSize = 18.0f,
        const mgfx::math::vec4& color = mgfx::math::vec4(0.95f, 0.55f, 0.35f, 0.95f)
    ) noexcept {
        char buf[512];
        format(buf, sizeof(buf), telem, scale, autoScalingEnabled);
        draw.drawText(buf, position, fontSize, color);
    }
};

} // namespace epi_sculpt
