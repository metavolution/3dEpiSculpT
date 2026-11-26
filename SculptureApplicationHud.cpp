#include "SculptureApplicationHud.h"
#include "SculptureApplication.h"
#include "SculptureHudWidgets.h"
#include "SculptureLearnTrace.h"
#include "ImplicitExternalImageTransport.h"
#include "SculptureMidiServer.h"
#include "Sculpture4DFoliation.h"
#include <cstdio>
#include <cstring>
#include <string>
#include <algorithm>

namespace epi_sculpt {

void formatSculptureApplicationHud(
    char* buffer, std::size_t size,
    const SculptureFramePlan& framePlan,
    const SculptureApplication& app) {
    const SculptureProgramParams& params = framePlan.params();
    const SculptureWorldState& worldState = framePlan.worldState();
    const SculpturePresetAsset& framePreset = app.presetAt(framePlan.presetIndex());
    const auto* capturedWorld = framePlan.capturedWorldRecipe();
    const auto& producerTelemetry = app.sculptureProducer_->telemetry();
    const auto& worldTelemetry = app.sculptureProducer_->worldTelemetry();
    const std::uint32_t requestedWorldBudget =
        framePlan.requestedWorldRecipe().instanceBudget;
    const std::uint32_t effectiveWorldBudget =
        framePlan.effectiveWorldRecipe().instanceBudget;
    const float frameWorldQualityScale = requestedWorldBudget > 0U
        ? static_cast<float>(effectiveWorldBudget)
            / static_cast<float>(requestedWorldBudget)
        : 1.0f;
    const WorldSnapshotDelivery worldDelivery = app.sculptureProducer_->worldDelivery();
    const WorldGenerationMetrics* worldMetrics = worldDelivery.metrics;
    const auto& bridgeMetrics = app.presentationBindings_->worldBridge().metrics();
    const float worldQueueMilliseconds = worldMetrics != nullptr
        ? worldMetrics->queueToDeliveryMilliseconds
        : WorldGenerationMetrics::kUnavailableMilliseconds;
    const float worldKernelMilliseconds = worldMetrics != nullptr
        ? worldMetrics->kernelMilliseconds
        : WorldGenerationMetrics::kUnavailableMilliseconds;
    const float worldReadbackMilliseconds = worldMetrics != nullptr
        ? worldMetrics->readbackMilliseconds
        : WorldGenerationMetrics::kUnavailableMilliseconds;
    const std::uint32_t worldReadbackBytes = worldMetrics != nullptr
        ? worldMetrics->readbackBytes : 0U;
    const float worldBridgeMilliseconds = bridgeMetrics.matches(worldDelivery.generation)
        ? bridgeMetrics.rebuildMilliseconds
        : WorldBridgeMetrics::kUnavailableMilliseconds;
    const WorldProfileSummary* profileSummary = app.worldProfileDriver_.summary();
    const WorldProfileAssessment profileAssessment = profileSummary != nullptr
        ? assessWorldProfile(*profileSummary) : WorldProfileAssessment{};
    const char* profileSessionState = app.worldProfileDriver_.stateName().data();
    const SculptureProductionDecision productionDecision = framePlan.production();
    const char* route = name(productionDecision.route).data();
    const char* routeFallback = productionDecision.dedicatedWorldFallback
        ? " [world fallback]" : "";
    const epi::FeedbackExternalSlotHealth canonicalInteropHealth =
        app.presentationBindings_->canonicalTransport()
            ? app.presentationBindings_->canonicalTransport()->health()
            : epi::FeedbackExternalSlotHealth{};
    const char* canonicalInteropState = app.canonicalExternalDeliveryEnabled_
        && app.presentationBindings_->canonicalTransport() ? "zero-copy"
        : app.presentationBindings_->canonicalTransport() ? "draining-for-rebuild"
        : app.canonicalExternalTransportSupported_ ? "host-staged/retry"
        : "host-staged";
    // implicit diagnostics
    const auto* implicitTransport = app.presentationBindings_->director().implicitExternalTransport();
    ImplicitExternalImageTransport::Diagnostics implicitDiag{};
    if (implicitTransport) {
        implicitDiag = implicitTransport->diagnostics();
    }
    if (!buffer || size == 0) return;
    // Formatting occurs before the current frame's native outcome. The trace
    // therefore uses one earlier finalized graph, never mixed with this plan.
    learn_mode::formatReceiptTrace(buffer, size,
        app.latestCausalExecutionGraph_ ? &*app.latestCausalExecutionGraph_ : nullptr);
    const std::size_t traceBytes = std::strlen(buffer);
    if (traceBytes >= size - 1U) return;
    const int baseWritten = std::snprintf(buffer + traceBytes, size - traceBytes,
                  "3dEpiSculpTMorph\nepiC %s -> %s -> mGFX/Vulkan\n"
                  "preset %zu/%zu: %s%s%s%s%s\n"
                  "math: %s%s\n"
                  "reduce: %s -> seed: %s -> color: %s\n"
                  "strategy: %s [scale: %s | sim: %.1fHz] | 3-plane recursive morph %.2f (%s)  iterations %u  latentW %.2f  speed %.2fx  %s  %s  %s  cam:%s  output:%s  audio:%s [Q:%.1f, R:%.2f]\n"
                  "CURRENT requested stages: [%s%s%s%s%s%s]  DIAGNOSTIC: %s\n"
                  "Drop .manifest to reopen captured configuration; field history restarts.\n"
                  "Relief contours %.2f levels/luminance (%s) | Alt+C toggle | Alt+[ coarser | Alt+] finer | Alt+Shift+] style | Alt+Shift+C authored\n"
                  "producer %s  interop %s/%s act%u reject(a%u/r%u) fb%u unavailable%u  cpu %.3f ms  cl %.3f ms  gen %u/%u  q%u  MIDI: %s (%lu rx)  OSC :%u (%lu rx)\n"
                  "temporal %s  source %u  %s\n"
                  "canonical interop %s  slots own%u wait%u cl%u ready%u read%u bad%u%s\n"
                  "reuse %u  submit-drop %u  %zu inst (L0:%zu L1:%zu L2:%zu cull:%zu) -> %zu draws  bad %zu  render %s\n"
                  "world gen %u  %zu inst / %zu batches / %zu dropped  quality %.0f%% budget %u/%u q %.3fms k %.3fms r %.3fms/%uB bridge %.3fms slots %u  kernel: %s\n"
                  "implicit transport: avail %u/%zu  in-flight %u  quarantine %lu  reject %lu  %s\n"
                  "profile %s samples %u assessment %s bottleneck %s %.0f%% -> %s\n"
                  "[1-9/0/-/=/[,]] preset [F1-7] learn [S/L] save/load state [M/T] tour/cam [Space] capture [Shift+Space] relief [F12] proj [F9] aux-res [Ctrl+F9] scene-res [Ctrl+Shift+F9] auto-res [KP +/-] speed [KP *] pause [Ctrl+<>] XFade [KP 0-9] cue [KP /] FX [Ctrl+4] 4D [Alt+4] Manifold [Alt+Arrows] Rotors",
                  app.producerBackend_.c_str(),
                  route,
                  framePlan.presetIndex() + 1U, app.presetLibrary_.entries().size(),
                  framePreset.title.c_str(),
                  capturedWorld ? " [captured world: " : "",
                  capturedWorld ? capturedWorld->presetId.c_str() : "",
                  capturedWorld ? "]" : "",
                  app.control_.sources().presetMorph().isActive() ? " (morphing)" : "",
                  getPipelineDescription(params).c_str(),
                  routeFallback,
                  name(params.reduction).data(),
                  name(params.seedType).data(),
                  name(params.colorEffect).data(),
                  name(app.pipeline_.renderStrategy()).data(),
                  name(params.auxiliaryResolutionScale).data(),
                  static_cast<double>(app.compactProducerCadence_.effectiveUpdateHz()),
                  static_cast<double>(worldState.morph),
                  name(app.control_.sources().presetMorph().envelopeConfig().easing).data(),
                  params.feedbackIterations,
                  static_cast<double>(params.latentW),
                  static_cast<double>(app.control_.runtime().playbackRate().value()),
                  app.control_.runtime().paused() ? "paused" : "running",
                  framePlan.presentation().cinematic ? "CINEMATIC" : "",
                  app.control_.sources().presetMorph().isPlaylistActive() ? (app.control_.sources().presetMorph().tourPlaybackMode() == TourPlaybackMode::ContinuousSpline ? "TOUR-SPLINE" : "TOUR-STEP") : "",
                  name(app.presentationBindings_->director().cameraMode()).data(),
                  params.outputMode == 0 ? "1D-KNOT" :
                  params.outputMode == 1 ? "3D-VOXEL" :
                  params.outputMode == 2 ? "2D-TORUS" : "2D-SPHERE",
                  app.appState_.audioEnabled ? (app.sculptureAudio_.hasActiveSignal() ? "ACTIVE" : "IDLE") : "OFF",
                  static_cast<double>(app.control_.audioModulation().metrics().resonanceQFactor),
                  static_cast<double>(app.control_.audioModulation().metrics().kuramotoOrderR),
                  learn_mode::enabled(params.stageMask, learn_mode::Stage::Transform) ? "T" : "-",
                  learn_mode::enabled(params.stageMask, learn_mode::Stage::Reduce) ? "R" : "-",
                  learn_mode::enabled(params.stageMask, learn_mode::Stage::FeedbackSample) ? "S" : "-",
                  learn_mode::enabled(params.stageMask, learn_mode::Stage::Color) ? "C" : "-",
                  learn_mode::enabled(params.stageMask, learn_mode::Stage::Seed) ? "W" : "-",
                  learn_mode::enabled(params.stageMask, learn_mode::Stage::Blend) ? "B" : "-",
                  learnModeStageDescription(params.stageMask).data(),
                  static_cast<double>(params.reliefContourDensity),
                  reliefContourStyleName(params.reliefContourStyle),
                  name(producerTelemetry.delivery).data(),
                  name(app.sculptureProducer_->interopState()).data(),
                  name(producerTelemetry.interop.lastEvent).data(),
                  producerTelemetry.interop.activationCount,
                  producerTelemetry.interop.acquireRejectionCount,
                  producerTelemetry.interop.releaseRejectionCount,
                  producerTelemetry.interop.fallbackTransitionCount,
                  producerTelemetry.interop.unavailableSlotCount,
                  static_cast<double>(app.sculptureProducer_->lastFrameMilliseconds()),
                  static_cast<double>(producerTelemetry.openClCompletionMilliseconds),
                  producerTelemetry.deliveredGeneration,
                  producerTelemetry.requestedGeneration,
                  producerTelemetry.pendingSlots,
                  app.midiServer_ && app.midiServer_->isRunning() ? std::string(app.midiServer_->mapper().profileName()).c_str() : "OFF",
                  app.midiServer_ ? static_cast<unsigned long>(app.midiServer_->messageCount()) : 0UL,
                  app.remoteServer_ ? app.remoteServer_->port() : 0U,
                  app.remoteServer_ ? static_cast<unsigned long>(app.remoteServer_->packetCount()) : 0UL,
                  producerTelemetry.temporalHistoryUsed ? "history" : "fresh",
                  producerTelemetry.temporalSourceGeneration,
                  params.temporalReset ? "RESET" : "",
                  canonicalInteropState,
                  canonicalInteropHealth.vulkanOwned,
                  canonicalInteropHealth.vulkanReleased,
                  canonicalInteropHealth.openClWriting,
                  canonicalInteropHealth.openClReleased,
                  canonicalInteropHealth.vulkanReading,
                  canonicalInteropHealth.unavailable,
                  canonicalInteropHealth.inFlightCount() > 0U ? " [in-flight]" : "",
                  producerTelemetry.staleFrameReuses,
                  producerTelemetry.droppedSubmissions,
                  app.presentationBindings_->compactBridge().instanceCount(),
                  app.presentationBindings_->compactBridge().lodStats().lod0Count,
                  app.presentationBindings_->compactBridge().lodStats().lod1Count,
                  app.presentationBindings_->compactBridge().lodStats().lod2Count,
                  app.presentationBindings_->compactBridge().lodStats().lodCulledCount,
                  app.presentationBindings_->compactBridge().batchCount(),
                  app.presentationBindings_->compactBridge().droppedSampleCount(),
                  renderPathName(app.presentationBindings_->compactBridge().renderPath()),
                  app.presentationBindings_->worldBridge().generation(),
                  app.presentationBindings_->worldBridge().instanceCount(),
                  app.presentationBindings_->worldBridge().activeBatchCount(),
                  app.presentationBindings_->worldBridge().droppedInstanceCount(),
                  static_cast<double>(frameWorldQualityScale * 100.0f),
                  effectiveWorldBudget,
                  requestedWorldBudget,
                  static_cast<double>(worldQueueMilliseconds),
                  static_cast<double>(worldKernelMilliseconds),
                  static_cast<double>(worldReadbackMilliseconds),
                  worldReadbackBytes,
                  static_cast<double>(worldBridgeMilliseconds),
                  worldTelemetry.pendingSlots,
                  std::string(app.sculptureProducer_->worldDiagnostic()).c_str(),
                  static_cast<unsigned int>(implicitTransport ? implicitDiag.slots.size() - implicitDiag.unavailableSlotCount() : 0U),
                  implicitTransport ? implicitDiag.slots.size() : 0U,
                  implicitDiag.inFlightSlotCount(),
                  implicitDiag.quarantineCount,
                  implicitDiag.rejectedTransitionCount,
                  app.presentationBindings_->director().implicitDiagnostic().data(),
                  profileSessionState,
                  profileSummary != nullptr ? profileSummary->acceptedGenerationCount : 0U,
                  name(profileAssessment.bottleneck).data(),
                  name(profileAssessment.status).data(),
                  static_cast<double>(profileAssessment.bottleneckFraction * 100.0f),
                  name(profileAssessment.recommendation).data());
    if (baseWritten < 0 || static_cast<std::size_t>(baseWritten) >= size - traceBytes) return;
    int offset = static_cast<int>(traceBytes + static_cast<std::size_t>(baseWritten));

    // snprintf returns the required length on truncation. Do not use that
    // value as an offset for the optional telemetry tail on a short buffer.
    if (offset < 0 || static_cast<std::size_t>(offset) >= size) return;

    const auto& composition = framePlan.composition();
    if (composition.hasHeldStages()) {
        const int written = std::snprintf(buffer + offset, size - offset,
            "\nAuthoring held: %s%s%s%s\n",
            composition.deckClock == SculptureCompositionAdmission::Held ? "deck clock; " : "",
            composition.deckMacros == SculptureCompositionAdmission::Held ? "deck macros; " : "",
            composition.programMacros == SculptureCompositionAdmission::Held ? "parameter macros; " : "",
            composition.manualRotors == SculptureCompositionAdmission::Held ? "manual/rotors; " : "");
        if (written < 0 || static_cast<std::size_t>(written) >= size - offset) return;
        offset += written;
    }

    if (framePlan.presentation().cinematic) {
        const auto count = app.control_.sources().sequence().keyframeCount();
        const int written = std::snprintf(buffer + offset, size - offset,
            "\nCinematic cue %zu/%zu  t=%.2fs  %s | Ctrl+PgUp/PgDn cue | Ctrl+Home first"
            " | Ctrl+Enter play/pause | Ctrl+Backspace stop\n",
            count != 0 ? app.control_.sources().sequence().currentIndex() + 1U : 0U, count,
            static_cast<double>(app.control_.sources().sequence().currentTimelineTime()),
            app.control_.runtime().paused() || !app.control_.sources().sequence().active() ? "held" : "playing");
        if (written < 0 || static_cast<std::size_t>(written) >= size - offset) return;
        offset += written;
    }

    if (worldTelemetry.pendingCompilations > 0 || producerTelemetry.pendingCompilations > 0) {
        int w2 = std::snprintf(buffer + offset, size - offset,
            "\n[Async JIT Compiler]\n"
            "   Pending Kernels : %u\n",
            worldTelemetry.pendingCompilations + producerTelemetry.pendingCompilations);
        if (w2 > 0 && static_cast<std::size_t>(w2) < size - offset) {
            offset += static_cast<std::size_t>(w2);
        }
    }
    if (producerTelemetry.programCacheSize > 0) {
        int w3 = std::snprintf(buffer + offset, size - offset,
            "\n[JIT Kernel Cache]\n"
            "   Cached Programs : %zu  |  Hits: %zu  |  Misses: %zu\n",
            producerTelemetry.programCacheSize,
            producerTelemetry.programCacheHits,
            producerTelemetry.programCacheMisses);
        if (w3 > 0 && static_cast<std::size_t>(w3) < size - offset) {
            offset += static_cast<std::size_t>(w3);
        }
    }

    const auto& govTelem = app.stagedGovernor().telemetry();
    int wGov = std::snprintf(buffer + offset, size - offset,
        "\n[Adaptive Compute Governor] %s\n"
        "   Auxiliary Tier  : %s (Savings: %.1fx)  |  Cadence: 1/%u ticks\n"
        "   GPU Latency     : %.2f ms (Smoothed: %.2f ms, Target: %.1f ms)\n",
        govTelem.isThrottled ? "THROTTLED" : "NOMINAL",
        name(govTelem.activeResolutionScale).data(),
        static_cast<double>(govTelem.effectiveComputeSavingsRatio),
        govTelem.activeTemporalCadence,
        static_cast<double>(govTelem.instantaneousLatencyMs),
        static_cast<double>(govTelem.smoothedLatencyMs),
        static_cast<double>(app.stagedGovernor().config().targetFrameTimeMs));
    if (wGov > 0 && static_cast<std::size_t>(wGov) < size - offset) {
        offset += static_cast<std::size_t>(wGov);
    }

    const auto& presentationScaleState = app.presentationScalePolicy_.state();
    const auto& renderGpu = app.presentationHost_->engine().lastRenderGpuTelemetry();
    const VkExtent2D sceneExtent0 = app.presentationHost_->engine().sceneRenderExtent(0U);
    const char* scaleRequestStatus = app.presentationHost_->engine().sceneTargetRequestBlocked()
        ? "  [request blocked]"
        : (app.presentationHost_->engine().sceneRenderScaleRequestPending()
            ? "  [request pending]" : "");
    int wPresentation = std::snprintf(buffer + offset, size - offset,
        "\n[Presentation Pixel Policy] %s  scale req/eff %.3f / %.3f  effective %ux%u  gen %llu\n"
        "   Vulkan graph   : %.2f ms envelope / %.2f ms pass-sum  |  submit %llu\n"
        "   CPU collect/plan/graph : %.2f / %.2f / %.2f ms (%s)\n"
        "   Target         : %.2f ms  |  auto decisions %llu%s\n",
        app.presentationScalePolicy_.autoEnabled() ? "AUTO" : "MANUAL",
        static_cast<double>(app.presentationHost_->engine().requestedSceneRenderScale()),
        static_cast<double>(app.presentationHost_->engine().effectiveSceneRenderScale()),
        sceneExtent0.width, sceneExtent0.height,
        static_cast<unsigned long long>(app.presentationHost_->engine().sceneTargetGeneration()),
        renderGpu.sampled ? renderGpu.graphMilliseconds : 0.0,
        renderGpu.sampled ? renderGpu.totalPassMilliseconds : 0.0,
        static_cast<unsigned long long>(
            renderGpu.sampled ? renderGpu.submissionSerial : 0U),
        renderGpu.cpuTiming.sampled ? renderGpu.cpuTiming.drawCollectionMilliseconds : 0.0,
        renderGpu.cpuTiming.sampled ? renderGpu.cpuTiming.renderPlanMilliseconds : 0.0,
        renderGpu.cpuTiming.sampled ? renderGpu.cpuTiming.graphRecordMilliseconds : 0.0,
        renderGpu.cpuTiming.sampled ? "sampled" : "unavailable",
        app.presentationScalePolicy_.config().targetGpuMilliseconds,
        static_cast<unsigned long long>(presentationScaleState.decisionCount),
        scaleRequestStatus);
    if (wPresentation > 0 && static_cast<std::size_t>(wPresentation) < size - offset) {
        offset += static_cast<std::size_t>(wPresentation);
    }

    if (params.enable4D != 0U) {
        const auto manifoldType = static_cast<Manifold4DType>((params.enable4D - 1U) % kManifold4DTypeCount);
        int w4D = std::snprintf(buffer + offset, size - offset,
            "\n[4D Foliation & SO(4) Rotor Kinematics]\n"
            "   Status       : ACTIVE  |  Manifold: %.*s\n"
            "   SO(4) Rotors : Alpha (L)=%.2f rad, Beta (R)=%.2f rad, Theta=%.2f rad\n"
            "   Spacetime W  : %.3f (Manual: %.2f)\n",
            static_cast<int>(name(manifoldType).size()), name(manifoldType).data(),
            static_cast<double>(params.rotorAlpha),
            static_cast<double>(params.rotorBeta),
            static_cast<double>(params.rotorTheta4D),
            static_cast<double>(params.latentW),
            static_cast<double>(app.control_.manualOverrides().latentW));
        if (w4D > 0 && static_cast<std::size_t>(w4D) < size - offset) {
            offset += static_cast<std::size_t>(w4D);
        }
    }

    if (app.isMorphSpaceActive()) {
        char morphBuf[512];
        MorphPadWidget::format(morphBuf, sizeof(morphBuf), app.morphSpace());
        int wMorph = std::snprintf(buffer + offset, size - offset,
            "\n%s\n",
            morphBuf);
        if (wMorph > 0 && static_cast<std::size_t>(wMorph) < size - offset) {
            offset += static_cast<std::size_t>(wMorph);
        }
    }

    if (app.isPerformanceDeckActive()) {
        char deckBuf[512];
        PerformanceDeckWidget::format(deckBuf, sizeof(deckBuf), app.performanceDeck());
        const bool downbeat = app.performanceDeck().isMusicalDownbeat();
        int wDeck = std::snprintf(buffer + offset, size - offset,
            "\n%s%s\n",
            downbeat ? "[*** 4/4 DOWNBEAT DROP ***]\n" : "",
            deckBuf);
        if (wDeck > 0 && static_cast<std::size_t>(wDeck) < size - offset) {
            offset += static_cast<std::size_t>(wDeck);
        }
    }

    const auto diagStage = learn_mode::diagnostic(params.stageMask);
    if (diagStage != learn_mode::DiagnosticStage::FullPipeline || params.stageMask != learn_mode::kAllStages) {
        char learnBuf[512];
        LearnModeWidget::format(learnBuf, sizeof(learnBuf), params.stageMask);
        const auto formula = learn_mode::diagnosticMathematicalFormula(diagStage);
        const auto explanation = learn_mode::diagnosticExplanation(diagStage);
        int wLearn = std::snprintf(buffer + offset, size - offset,
            "\n%s\n"
            "   Diagnostic    : %s\n"
            "   Formula       : %.*s\n"
            "   Physical Math : %.*s\n",
            learnBuf,
            learn_mode::diagnosticDescription(diagStage),
            static_cast<int>(formula.size()),
            formula.data(),
            static_cast<int>(explanation.size()),
            explanation.data());
        if (wLearn > 0 && static_cast<std::size_t>(wLearn) < size - offset) {
            offset += static_cast<std::size_t>(wLearn);
        }
    }

    const auto proj = app.resolveActiveReliefProjection();
    const bool isOverridden = app.activeReliefProjectionOverride().has_value();
    int wRelief = std::snprintf(buffer + offset, size - offset,
        "\n[2.5D Epimorphic Relief 3D-Print]\n"
        "   Projection  : %s [%s]\n"
        "   Metric & K  : %s\n",
        name(proj),
        isOverridden ? "MANUAL OVERRIDE (F12)" : "AUTO-RESOLVED",
        metricDescription(proj));
    if (wRelief > 0 && static_cast<std::size_t>(wRelief) < size - offset) {
        offset += static_cast<std::size_t>(wRelief);
    }
}
} // namespace epi_sculpt
