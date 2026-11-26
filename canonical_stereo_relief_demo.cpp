#include "EpiFeedbackFieldProducer.h"
#include "SculptureLiveControls.hpp"
#include "SculptureModeProjection.h"
#include "SculptureRenderBridge.h"
#include "SculptureResourcePaths.h"
#include "SculptureStateRecorder.h"
#include "SculptureCapturePixels.h"
#include "SculptureVulkanDeviceIdentity.h"
#include "DesktopRunObservation.h"
#include "epi/io/bounded_text.hpp"
#include "mgfx/vk/FrameLoop.h"
#include "mgfx/Engine.h"
#include "mgfx/vk/Swapchain.h"

#include <charconv>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <limits>
#include <memory>
#include <stdexcept>

namespace {

epi::VisualMode loadReliefMode(const std::filesystem::path& path) {
    const auto text = epi::io::readBoundedText(path, 4U * 1024U * 1024U, "canonical mode file");
    // Sculpture JSON .epimode is a different input. Require an actual shared
    // transform-section line before decoding; a marker in a comment is not enough.
    const auto first = text.find_first_not_of(" \t\r\n");
    const bool hasTransform = text.starts_with("[transform]")
        || text.find("\n[transform]") != std::string::npos;
    if (first == std::string::npos || (text[first] != '#' && text[first] != '[')
        || !hasTransform) {
        throw std::invalid_argument("--mode requires shared VisualMode text with a [transform] section");
    }
    auto mode = epi::VisualMode::deserialize_strict(text);
    for (const auto z : {mode.zn0, mode.zn1, mode.zn2, mode.zn3}) {
        if (!std::isfinite(z.re) || !std::isfinite(z.im))
            throw std::invalid_argument("canonical mode requires finite affine coordinates");
    }
    if (!std::isfinite(mode.speed) || mode.speed <= 0.0f
        || !std::isfinite(mode.time_speed) || mode.time_speed < 0.0f || mode.time_speed > 16.0f) {
        throw std::invalid_argument("canonical mode requires positive finite speed and time_speed in 0..16");
    }
    return mode;
}

void resolveModeSeed(epi::VisualMode& mode, const std::filesystem::path& modePath,
                     const std::filesystem::path& resourceRoot) {
    if (mode.seed_type != static_cast<int>(epi::SeedType::Image)) return;
    std::filesystem::path seed(mode.seed_image_path);
    if (seed.empty()) throw std::invalid_argument("image-seed mode requires seed_image_path");
    if (seed.is_relative()) {
        const auto besideMode = modePath.parent_path() / seed;
        if (std::filesystem::is_regular_file(besideMode)) seed = besideMode;
        else if (!resourceRoot.empty() && std::filesystem::is_regular_file(resourceRoot / seed))
            seed = resourceRoot / seed;
        else throw std::invalid_argument("image seed was not found beside the mode or under the resource root");
    }
    if (!std::filesystem::is_regular_file(seed))
        throw std::invalid_argument("image-seed mode requires an existing seed_image_path");
    mode.seed_image_path = std::filesystem::absolute(seed).lexically_normal().string();
}

} // namespace

// One OpenCL recursive history, one immutable delivery, two Vulkan views.
// This small baseline deliberately uses bounded host staging for transport;
// it never evaluates the feedback mathematics on the CPU.
int main(int argc, char** argv) try {
    using namespace epi_sculpt;
    std::string pattern = "exp_golden_phyllotaxis_cathedral";
    std::filesystem::path modePath;
    bool explicitPattern = false;
    unsigned fieldSize = 512;
    DesktopRunOptions runOptions;
    int windowWidth = 2560, windowHeight = 1440;
    bool stereo = true, validation = false, captureLast = false;
    for (int i = 1; i < argc; ++i) {
        const std::string_view arg(argv[i]);
        if (runOptions.parse(i, argc, argv)) continue;
        if (arg == "--help") {
            std::puts("canonical_stereo_relief_demo [--pattern NAME | --mode FILE] [--field-size 64..2048]\n"
                      "  [--mono] [--frames N] [--capture-last] [--validation]\n"
                      "  [--1440p] [--1080p] [--width PIXELS] [--height PIXELS]\n"
                      "  [--seconds S] [--watchdog S] [--warmup N] [--report PATH.jsonl]\n"
                      "  [--scenario basic] bounded pause/orbit/resume/capture/resize/SBS\n"
                      "P: mono/stereo; mouse: orbit/pan/dolly; WASD+E/C: move; Home: reset camera\n"
                      "Space: PNG + state; numpad */+/-: pause/speed; L: Learn Mode; H: help\n"
                      "--mode loads shared VisualMode text (including .canonical.epimode), up to 4 MiB.\n"
                      "Starts fresh feedback history; field/window sizes and controls remain host-owned.\n"
                      "OpenCL feedback -> staged shared field -> Vulkan stereo relief.");
            return 0;
        } else if (arg == "--mono") stereo = false;
        else if (arg == "--validation") validation = true;
        else if (arg == "--capture-last") captureLast = true;
        else if (arg == "--1440p") { windowWidth = 2560; windowHeight = 1440; }
        else if (arg == "--1080p") { windowWidth = 1920; windowHeight = 1080; }
        else if ((arg == "--width" || arg == "-W") && i + 1 < argc) {
            windowWidth = DesktopRunOptions::number<int>(argv[i + 1]);
            ++i;
        } else if ((arg == "--height" || arg == "-H") && i + 1 < argc) {
            windowHeight = DesktopRunOptions::number<int>(argv[i + 1]);
            ++i;
        } else if ((arg == "--pattern" || arg == "--mode" || arg == "--field-size") && i + 1 < argc) {
            const std::string_view value(argv[++i]);
            if (arg == "--pattern") {
                pattern = value;
                explicitPattern = true;
            } else if (arg == "--mode") {
                if (value.empty()) throw std::invalid_argument("--mode requires a file path");
                modePath = std::filesystem::path(value);
            } else {
                unsigned parsed = 0;
                const auto result = std::from_chars(value.data(), value.data() + value.size(), parsed);
                if (result.ec != std::errc{} || result.ptr != value.data() + value.size()
                    || (arg == "--field-size" && (parsed < 64 || parsed > 2048)))
                    throw std::runtime_error("invalid value for " + std::string(arg));
                fieldSize = parsed;
            }
        } else throw std::runtime_error("unknown or incomplete option: " + std::string(arg));
    }
    runOptions.validate();
    if (windowWidth < 64 || windowHeight < 64 || windowWidth > 16384 || windowHeight > 16384)
        throw std::invalid_argument("window dimensions must be 64..16384");
    if (explicitPattern && !modePath.empty())
        throw std::invalid_argument("choose --pattern or --mode, not both");
    DesktopRunObservation observation(runOptions);
    std::optional<epi::VisualMode> rootMode;
    if (modePath.empty()) {
        rootMode = mode_projection::fetchRootPatternMode(pattern);
        if (!rootMode) throw std::runtime_error("unknown root pattern: " + pattern);
    } else {
        rootMode = loadReliefMode(modePath);
        pattern = rootMode->preset_name.empty() ? "canonical-mode-file" : rootMode->preset_name;
        if (pattern.size() > 1024U || pattern.find_first_of("\r\n") != std::string::npos)
            throw std::invalid_argument("mode preset identity exceeds capture source limits");
    }
    const SculptureResourcePaths resources = SculptureResourceLocator::resolve(argv[0]);
    if (!modePath.empty()) resolveModeSeed(*rootMode, modePath, resources.root);
    SculptureProgramParams rootParams{};
    mode_projection::visualModeToSculptureParams(*rootMode, rootParams);
    mgfx::vk::Context context({.width = windowWidth, .height = windowHeight,
        .title = "epiC - canonical OpenCL stereo relief (1440p)", .enableValidation = validation,
        .shaderBinaryDirectory = resources.shaderBinaryDirectory});
    mgfx::vk::Swapchain swapchain(context);
    mgfx::vk::FrameLoop loop(context, swapchain);
    mgfx::Engine::Config config{};
    config.quality = mgfx::RenderQuality::basic();
    config.validateShaderContract = false;
    config.gpuPassTimingEnabled = !runOptions.report.empty();
    const std::string shaderSourceDirectory = resources.shaderSourceDirectory.string();
    const std::string fontPath = resources.fontPath.string();
    config.shaderSourceDir = shaderSourceDirectory.empty() ? nullptr : shaderSourceDirectory.c_str();
    config.text.fontPath = fontPath.empty() ? nullptr : fontPath.c_str();
    mgfx::Engine engine(context, loop, config);
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(context.device().physical(), &properties);
    observation.device(1, describeVulkanPhysicalDevice(context.device().physical()),
        properties.driverVersion, properties.apiVersion);
    observation.configuration("field-size", std::to_string(fieldSize));
    observation.configuration("semantic-source", modePath.empty() ? "registered-pattern" : "canonical-mode-file");
    if (!modePath.empty()) {
        observation.configuration("mode-file", modePath.string());
        observation.configuration("mode-load-policy", "producer-configuration; fresh-feedback-history; host-owned-extents");
    }
    observation.configuration("output-request", std::to_string(windowWidth) + "x" + std::to_string(windowHeight));
    observation.configuration("quality", "basic; host-staged; one canonical OpenCL field");
    engine.setAtmosphere(mgfx::pbr::Atmosphere{});
    // Drain submitted work before the following GPU owners are destroyed,
    // including exception exits.
    struct IdleGuard { mgfx::Engine& engine; ~IdleGuard() { engine.waitIdle(); } };
    EpiFeedbackFieldProducer::Config producerConfig{};
    producerConfig.width = producerConfig.height = producerConfig.kernelDimension = fieldSize;
    producerConfig.kernelPath = resources.canonicalFeedbackKernel.string();
    producerConfig.sourceStateId = pattern;
    EpiFeedbackFieldProducer producer(producerConfig);
    if (!producer.initialize()) throw std::runtime_error(producer.diagnostic());
    SculptureRenderBridge bridge;
    SculptureLiveControls controls(context.window().glfw());
    IdleGuard drain{engine};
    SculptureProgramParams params = rootParams;
    mgfx::Camera authored;
    authored.eye = {0.0f, 0.2f, 3.5f};
    authored.target = {0.0f, 0.0f, 0.0f};
    std::optional<EpiFeedbackFieldDelivery> field;
    std::optional<epi::VisualMode> selectedCanonicalMode;
    float selectedControlTime = 0.0f, selectedProductionDelta = 0.0f;
    std::uint64_t generation = 0;
    constexpr std::uint64_t resetEpoch = 1U;
    auto previous = std::chrono::steady_clock::now();
    const auto start = previous;
    unsigned frames = 0;
    std::uint64_t pausedGeneration = 0, resumedGeneration = 0;
    struct ArmedReliefCaptureEvidence {
        bool terminal = false;
        bool bound = false;
        std::uint64_t submissionSerial = 0U;
        std::uint64_t generation = 0U;
        SculptureWorldMode mode{};
        CanonicalFeedbackCaptureState canonical{};
        std::shared_ptr<DesktopCaptureReceipt> receipt{};
    };
    std::shared_ptr<ArmedReliefCaptureEvidence> armedCaptureEvidence;
    std::shared_ptr<DesktopCaptureReceipt> scenarioCaptureReceipt;
    bool scenarioCapture = false;
    auto captureObservation = std::make_shared<DesktopCaptureObservation>();
    int scenarioWidth = windowWidth, scenarioHeight = windowHeight;
    observation.start();
    while (!context.shouldClose() && observation.keepRunning()) {
        const auto frameStart = std::chrono::steady_clock::now();
        context.pollEvents();
        if (const auto action = observation.nextAction()) {
            switch (*action) {
            case DesktopScenarioAction::Pause:
                controls.setPaused(true); observation.event("pause", "applied"); break;
            case DesktopScenarioAction::Orbit:
                pausedGeneration = generation;
                authored.eye.x += 0.4f;
                observation.event("orbit-while-paused", "applied"); break;
            case DesktopScenarioAction::Resume:
                observation.event("paused-primary-ordinal", generation == pausedGeneration ? "observed" : "failed");
                resumedGeneration = generation;
                controls.setPaused(false); observation.event("resume", "applied"); break;
            case DesktopScenarioAction::Palette:
                observation.event("resumed-primary-progress", generation > resumedGeneration ? "observed" : "failed");
                observation.event("palette", "unsupported-by-relief-host"); break;
            case DesktopScenarioAction::Capture:
                scenarioCaptureReceipt = std::make_shared<DesktopCaptureReceipt>();
                scenarioCapture = true; observation.event("capture", "requested"); break;
            case DesktopScenarioAction::Resize:
                stereo = true;
                observation.event("SBS-for-odd-width", "applied");
                glfwGetWindowSize(context.window().glfw(), &scenarioWidth, &scenarioHeight);
                observation.requestResize(DesktopResizeStage::OddWidth, 1281, 721, 2,
                    1, loop.lastSubmittedSerial());
                glfwSetWindowSize(context.window().glfw(), 1281, 721);
                break;
            case DesktopScenarioAction::Restore:
                observation.requestResize(DesktopResizeStage::Restore, scenarioWidth, scenarioHeight,
                    stereo ? 2U : 1U, 1, loop.lastSubmittedSerial());
                glfwSetWindowSize(context.window().glfw(), scenarioWidth, scenarioHeight);
                break;
            case DesktopScenarioAction::Views:
                observation.finishResize(DesktopResizeStage::Restore);
                stereo = !stereo; observation.event("mono-stereo-toggle", "applied"); break;
            }
        }
        const auto now = std::chrono::steady_clock::now();
        const float dt = std::chrono::duration<float>(now - previous).count();
        previous = now;
        controls.beginFrame(dt, SculptureLiveControls::SimulationClockPolicy::HostCommitted);
        if (context.shouldClose()) break;
        if (controls.consumePresentationToggle()) stereo = !stereo;
        params = rootParams;
        controls.applyProgramOffsets(params);
        // Simulation runs once outside the view callback. Pause keeps the
        // existing generation while both cameras remain navigable. Resolve the
        // complete canonical mode only when production is actually due.
        if (!field || !controls.paused()) {
            const float productionDelta = controls.simulationDeltaSeconds();
            const auto nextControlTime = controls.proposeSimulationAdvance(productionDelta);
            if (!nextControlTime || generation == std::numeric_limits<std::uint64_t>::max()) {
                throw std::overflow_error("canonical relief production clock or generation exhausted");
            }
            const std::uint64_t nextGeneration = generation + 1U;
            epi::VisualMode canonicalMode =
                mode_projection::resolveCanonicalModeWithSculptureOverrides(
                    *rootMode, rootParams, params);
            auto production = producer.produce_result(
                canonicalMode, productionDelta, nextGeneration, resetEpoch);
            // A recursive swap commits mathematics even if later host staging
            // fails. Keep that fact before inspecting publication; this bounded
            // host terminates on either failure instead of retrying accepted work.
            if (!production.mathematicsAccepted) {
                throw std::runtime_error("canonical relief mathematics rejected: " + producer.diagnostic());
            }
            generation = nextGeneration;
            (void)controls.commitSimulationTime(*nextControlTime); // Preflighted before acceptance.
            if (!production.hasConsumableDelivery()
                || production.delivery->transport != epi::FeedbackFieldTransport::HostStaged) {
                throw std::runtime_error("canonical relief publication failed after accepting generation "
                    + std::to_string(generation) + ": " + producer.diagnostic());
            }
            const auto& selected = production.delivery->descriptor;
            if (selected.generation != generation || selected.resetEpoch != resetEpoch
                || selected.sourceStateId != pattern
                || selected.width != fieldSize || selected.height != fieldSize
                || !std::isfinite(selected.simulationTime)) {
                throw std::runtime_error("canonical relief publication does not match accepted generation "
                    + std::to_string(generation));
            }
            field = std::move(production.delivery);
            // Retain exactly the published field's input, including parameters
            // that have no sculpture projection. Moves reuse its owned strings;
            // serialization is deferred until a capture actually needs it.
            selectedCanonicalMode = std::move(canonicalMode);
            selectedControlTime = static_cast<float>(*nextControlTime);
            selectedProductionDelta = productionDelta;
        }
        const auto camera = controls.applyCamera(authored, dt);
        if ((controls.consumeCapture() || scenarioCapture
             || (captureLast && runOptions.frames && frames + 1 == runOptions.frames))
            && !armedCaptureEvidence) {
            const auto receipt = scenarioCapture ? scenarioCaptureReceipt : nullptr;
            scenarioCapture = false;
            auto evidence = std::make_shared<ArmedReliefCaptureEvidence>();
            evidence->receipt = receipt;
            const auto admission = loop.requestCaptureOutcome(
                [evidence, captureObservation](const mgfx::vk::CaptureOutcome& result) {
                    ++captureObservation->finished;
                    if (evidence->receipt) {
                        evidence->receipt->finished = true;
                        evidence->receipt->submission = result.submissionSerial;
                    }
                    evidence->terminal = true;
                    if (result.status != mgfx::vk::CaptureStatus::Completed
                        || !result.submitted || result.submissionSerial == 0
                        || !evidence->bound
                        || result.submissionSerial != evidence->submissionSerial) {
                        return;
                    }
                    const auto paths = SculptureStateRecorder::generatePaths("snapshots", evidence->mode.presetName);
                    evidence->canonical.readbackSubmissionSerial = result.submissionSerial;
                    bool saved = false;
                    try {
                        epi_sculpt::withBgraCapturePixels(result.image, [&](const auto& image) {
                            saved = SculptureStateRecorder::captureFrameAndStateWithPaths(
                                image, evidence->mode, paths, nullptr, false, &evidence->canonical);
                        });
                    } catch (const std::exception& error) {
                        std::fprintf(stderr, "[CanonicalRelief] Capture conversion failed: %s\n", error.what());
                        return;
                    }
                    if (saved) {
                        ++captureObservation->persisted;
                        captureObservation->lastSubmission = result.submissionSerial;
                        ++captureObservation->qualifiedProvenance;
                        if (evidence->receipt) evidence->receipt->persisted = true;
                    }
                });
            if (admission == mgfx::vk::CaptureAdmission::Accepted) {
                ++captureObservation->admitted;
                if (receipt) {
                    receipt->admitted = true;
                    receipt->deviceGeneration = 1; // This host has one fixed logical-device lifetime.
                }
                armedCaptureEvidence = std::move(evidence);
            } else {
                observation.event("capture-admission", "failed");
            }
        }
        // Mono and stereo collect once through runFrameFamily. Reset this fact
        // per attempt so a skipped frame cannot inherit a previous collection.
        auto reliefOutcome = ReliefDrawOutcome::Deferred;
        const auto draw = [&](mgfx::Drawable& drawable) {
            reliefOutcome = bridge.drawRelief(
                drawable, mgfx::kInvalidMaterial, mgfx::math::mat4(1.0f), params, *field);
            if (!fontPath.empty()) {
                const auto hud = controls.hud(std::string("OPENCL SHARED FIELD | ")
                    + reliefDrawOutcomeName(reliefOutcome) + " | input gen " + std::to_string(generation)
                    + (stereo ? " | STEREO" : " | MONO"), pattern);
                drawable.drawText(hud.c_str(), {12.0f, 12.0f}, 14.0f);
                // The HUD above is desktop-global; these annotations use each
                // selected view's own output pixels, including odd SBS widths.
                const auto& views = drawable.frameViews();
                for (std::uint32_t view = 0; view < views.count && view < views.views.size(); ++view) {
                    const auto extent = views.views[view].outputExtent;
                    const std::string label = "VIEW " + std::to_string(view) + " | " +
                        std::to_string(extent.width) + "x" + std::to_string(extent.height);
                    (void)drawable.drawTextForView(view, label, {12.0f, 42.0f}, 14.0f,
                                                  mgfx::math::vec4(0.55f, 0.9f, 1.0f, 1.0f));
                }
            }
        };
        mgfx::vk::FrameOutcome outcome;
        if (stereo) {
            const auto right = glm::normalize(glm::cross(camera.target - camera.eye, camera.up));
            auto leftEye = camera, rightEye = camera;
            // Parallel eye axes avoid toe-in vertical disparity.
            leftEye.eye -= right * 0.032f; leftEye.target -= right * 0.032f;
            rightEye.eye += right * 0.032f; rightEye.target += right * 0.032f;
            outcome = engine.runStereoFrame(leftEye, rightEye, draw);
        } else outcome = engine.runFrame(camera, draw);
        if (armedCaptureEvidence) {
            if (armedCaptureEvidence->terminal) {
                armedCaptureEvidence.reset();
            } else if (mgfx::vk::frameOutcomeSubmitted(outcome)) {
                const std::uint64_t submittedSerial = loop.lastSubmittedSerial();
                // A submitted HUD-only frame is not evidence that this field
                // was selected. The capture callback also checks this serial.
                if (reliefOutcome == ReliefDrawOutcome::Collected && submittedSerial != 0U
                    && selectedCanonicalMode) {
                    try {
                        SculptureWorldState state{};
                        // The producer's kernel clock may differ from accepted
                        // host control elapsed time because the mode owns time policy.
                        state.time = field->descriptor.simulationTime;
                        state.hasCameraOverride = true;
                        for (unsigned j = 0; j < 3; ++j) {
                            state.cameraEye[j] = camera.eye[j];
                            state.cameraTarget[j] = camera.target[j];
                            state.cameraUp[j] = camera.up[j];
                        }
                        armedCaptureEvidence->mode = SculptureStateRecorder::makeCaptureWorldMode(
                            state, pattern, "canonical-stereo-v1", params,
                            {"transport: host-staged",
                             stereo ? "views: 2" : "views: 1",
                             "capture_submission_serial=" + std::to_string(submittedSerial),
                             "capture_feedback_generation=" + std::to_string(generation)});
                        // WorldMode v21 retains the same full-width identity as
                        // the selected publication and its canonical sidecar.
                        armedCaptureEvidence->mode.canonicalFeedbackGeneration =
                            field->descriptor.generation;
                        armedCaptureEvidence->mode.canonicalFeedbackResetEpoch = field->descriptor.resetEpoch;
                        armedCaptureEvidence->mode.selectedCanonicalFeedbackGeneration = field->descriptor.generation;
                        armedCaptureEvidence->mode.selectedCanonicalFeedbackResetEpoch = field->descriptor.resetEpoch;
                        auto& canonical = armedCaptureEvidence->canonical;
                        canonical.serializedMode = selectedCanonicalMode->serialize();
                        canonical.sourceStateId = field->descriptor.sourceStateId;
                        canonical.generation = field->descriptor.generation;
                        canonical.resetEpoch = field->descriptor.resetEpoch;
                        canonical.fieldWidth = field->descriptor.width;
                        canonical.fieldHeight = field->descriptor.height;
                        canonical.kernelTime = field->descriptor.simulationTime;
                        canonical.acceptedControlTime = selectedControlTime;
                        canonical.productionDelta = selectedProductionDelta;
                        canonical.submissionSerial = submittedSerial;
                        armedCaptureEvidence->submissionSerial = submittedSerial;
                        armedCaptureEvidence->generation = generation;
                        armedCaptureEvidence->bound = true;
                    } catch (const std::exception& error) {
                        // Capture preparation is observational. Failure leaves its
                        // evidence unbound; it must not discard accepted mathematics.
                        std::fprintf(stderr, "[canonical-relief] capture state preparation failed: %s\n", error.what());
                    }
                }
                armedCaptureEvidence.reset();
            }
        }
        observation.frame(outcome,
            std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - frameStart).count(),
            1, loop.lastSubmittedSerial(), pattern, generation, false, engine.lastRenderGpuTelemetry());
        if (observation.resizePending()) {
            int width = 0, height = 0, pixelWidth = 0, pixelHeight = 0;
            glfwGetWindowSize(context.window().glfw(), &width, &height);
            glfwGetFramebufferSize(context.window().glfw(), &pixelWidth, &pixelHeight);
            observation.resizeFrame(outcome, 1, loop.lastSubmittedSerial(),
                width, height, pixelWidth, pixelHeight, engine.lastRenderSubmissionObservation());
        }
        ++frames;
    }
    engine.waitIdle();
    if (loop.finishCaptures() != VK_SUCCESS) observation.event("capture-retirement", "failed");
    if (runOptions.basicScenario)
        observation.scenarioCaptureResult(scenarioCaptureReceipt.get());
    observation.configuration("capture-last-submission", std::to_string(captureObservation->lastSubmission));
    observation.configuration("capture-qualified-provenance-count", std::to_string(captureObservation->qualifiedProvenance));
    observation.configuration("capture-provenance", "submitted-frame-bound semantic state; exact submission serial and generation joined");
    const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    std::fprintf(stderr, "[canonical-relief] frames=%u generations=%llu seconds=%.3f fps=%.2f transport=host-staged\n",
        frames, static_cast<unsigned long long>(generation), seconds, seconds > 0 ? frames / seconds : 0);
    return observation.finish() ? 0 : 1;
} catch (const std::exception& error) {
    std::fprintf(stderr, "[canonical-relief] %s\n", error.what());
    return 1;
}
