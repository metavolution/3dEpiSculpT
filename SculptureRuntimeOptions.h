// Small, renderer-free command-line contract for the live application.
// Parsing here keeps service selection and startup routing out of the render
// loop, so future launchers share one predictable control-plane policy.
#pragma once

#include <cmath>
#include <array>
#include <charconv>
#include <cstdlib>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <mgfx/ProceduralGeometryRecipe.h>
#include "SculptureReliefContourSettings.h"
#include "SculptureCameraPose.h"
#include "SculptureFieldPresentationOptions.h"

namespace epi_sculpt {

enum class SculptureStartupSelection : unsigned char { Preset, World };

struct SculptureDesktopSize {
    int width;
    int height;
};

struct SculptureRuntimeOptions {
    mgfx::ProceduralGeometryRecipe volumeRecipe = mgfx::ProceduralGeometryRecipe::Field;
    SculptureFieldPresentationOptions fieldPresentation{};
    std::optional<std::string> startupSelection;
    SculptureStartupSelection startupSelectionKind =
        SculptureStartupSelection::Preset;
    std::optional<std::string> statePath;
    // Optional root of a relocatable epiC resource layout. When absent, the
    // application discovers roots beside the executable/current checkout and
    // only then falls back to compiled developer paths.
    std::optional<std::filesystem::path> resourceRoot;
    std::optional<std::string> profileWorld;
    std::optional<float> profileSettleSeconds;
    std::optional<float> profileSampleSeconds;
    // Independent internal scene-resolution control. Feature quality and
    // presentation cadence remain separate authorities.
    std::optional<float> sceneRenderScale;
    std::optional<float> reliefContourDensity;
    // Presentation profiling/adaptation are explicit opt-ins. Automatic scene
    // scaling consumes completed Vulkan graph telemetry; it is independent of
    // the staged mathematical-compute governor.
    bool enableGpuPassTiming = false;
    bool enableAutoSceneScale = false;
    std::optional<float> sceneGpuBudgetMilliseconds;
    bool enableRemoteControl = false;
    bool enableMidiControl = false;
    std::optional<std::string> midiProfile;
    bool hidden = false;
    bool noHud = false;
    bool desktop = false;
    bool hostStaged = false;
    std::optional<double> runSeconds;
    // One ordinary snapshot request at the run clock boundary. Capture
    // admission, originating-frame joins and persistence retain their owners.
    std::optional<double> captureSeconds;
    std::optional<std::uint32_t> pauseAfterGeneration;
    std::optional<SculptureDesktopSize> desktopSize;
    std::optional<SculptureCameraPose> cameraPose;

    [[nodiscard]] static SculptureRuntimeOptions parse(int argc,
                                                        char** argv) {
        SculptureRuntimeOptions options{};
        if (argv == nullptr) return options;
        std::optional<std::array<float, 3>> cameraEye;
        std::optional<std::array<float, 3>> cameraTarget;

        for (int index = 1; index < argc; ++index) {
            const std::string_view argument = argv[index] != nullptr
                ? std::string_view(argv[index]) : std::string_view{};
            if (argument == "--pause-after-generation") {
                if (index + 1 >= argc || argv[index + 1] == nullptr)
                    throw std::invalid_argument("--pause-after-generation requires an integer in [1, 4294967295]");
                const std::string_view value(argv[++index]);
                std::uint32_t generation = 0U;
                const auto parsed = std::from_chars(value.data(), value.data() + value.size(), generation);
                if (parsed.ec != std::errc{} || parsed.ptr != value.data() + value.size() || generation == 0U)
                    throw std::invalid_argument("--pause-after-generation requires an integer in [1, 4294967295]");
                options.pauseAfterGeneration = generation;
            } else if (argument.starts_with("--pause-after-generation=")) {
                throw std::invalid_argument("use --pause-after-generation N");
            } else if (argument == "--field-palette" || argument == "--field-channel" ||
                argument == "--field-min" || argument == "--field-max" ||
                argument == "--field-density-scale" || argument == "--field-extinction-ratio" ||
                argument == "--field-anisotropy" || argument == "--field-pulse") {
                if (index + 1 >= argc || argv[index + 1] == nullptr)
                    throw std::invalid_argument("field presentation option requires a value");
                const std::string_view value(argv[++index]);
                auto& field = options.fieldPresentation;
                if (argument == "--field-palette") {
                    if (value == "blue-pink") field.palette = SculptureFieldPalette::BluePink;
                    else if (value == "ice") field.palette = SculptureFieldPalette::Ice;
                    else if (value == "ember") field.palette = SculptureFieldPalette::Ember;
                    else if (value == "mono") field.palette = SculptureFieldPalette::Mono;
                    else throw std::invalid_argument("unknown field palette");
                } else if (argument == "--field-channel") {
                    if (value == "x") field.channel = mgfx::ProceduralFieldChannel::X;
                    else if (value == "y") field.channel = mgfx::ProceduralFieldChannel::Y;
                    else if (value == "z") field.channel = mgfx::ProceduralFieldChannel::Z;
                    else if (value == "density") field.channel = mgfx::ProceduralFieldChannel::Density;
                    else throw std::invalid_argument("unknown field channel");
                } else {
                    const float number = parseFieldScalar(value);
                    if (argument == "--field-min" || argument == "--field-max") {
                        if (std::abs(number) > 1.0e6f) throw std::invalid_argument("field range endpoint out of bounds");
                        if (argument == "--field-min") field.minimum = number;
                        else field.maximum = number;
                    } else if (argument == "--field-density-scale") {
                        if (number < 0.0f || number > 16.0f) throw std::invalid_argument("field density scale out of bounds");
                        field.densityScale = number;
                    } else if (argument == "--field-extinction-ratio") {
                        if (number < 1.0f || number > 16.0f) throw std::invalid_argument("field extinction ratio out of bounds");
                        field.extinctionRatio = number;
                    } else if (argument == "--field-anisotropy") {
                        if (number < -0.95f || number > 0.95f) throw std::invalid_argument("field anisotropy out of bounds");
                        field.anisotropy = number;
                    } else {
                        if (number < 0.0f || number > 0.5f) throw std::invalid_argument("field pulse out of bounds");
                        field.pulse = number;
                    }
                }
            } else if (argument.starts_with("--field-palette=") || argument.starts_with("--field-channel=") ||
                       argument.starts_with("--field-min=") || argument.starts_with("--field-max=") ||
                       argument.starts_with("--field-density-scale=") || argument.starts_with("--field-extinction-ratio=") ||
                       argument.starts_with("--field-anisotropy=") || argument.starts_with("--field-pulse=")) {
                throw std::invalid_argument("field presentation options require separate values");
            } else if (argument == "--camera-eye" || argument == "--camera-target") {
                if (index + 1 >= argc || argv[index + 1] == nullptr)
                    throw std::invalid_argument("camera coordinates require X,Y,Z");
                const auto coordinates = parseCameraCoordinates(argv[++index]);
                if (argument == "--camera-eye") cameraEye = coordinates;
                else cameraTarget = coordinates;
            } else if (argument.starts_with("--camera-eye=") || argument.starts_with("--camera-target=")) {
                throw std::invalid_argument("use --camera-eye X,Y,Z and --camera-target X,Y,Z");
            } else if (argument == "--volume-recipe") {
                if (index + 1 >= argc || argv[index + 1] == nullptr)
                    throw std::invalid_argument("--volume-recipe requires a supported recipe name");
                const std::string_view value(argv[++index]);
                if (value == "field") options.volumeRecipe = mgfx::ProceduralGeometryRecipe::Field;
                else if (value == "epimorph-woodland") options.volumeRecipe = mgfx::ProceduralGeometryRecipe::EpiMorphWoodland;
                else if (value == "octonion-woodland") options.volumeRecipe = mgfx::ProceduralGeometryRecipe::OctonionWoodland;
                else if (value == "mandelbulb-woodland") options.volumeRecipe = mgfx::ProceduralGeometryRecipe::MandelbulbWoodland;
                else if (value == "dielectric-woodland") options.volumeRecipe = mgfx::ProceduralGeometryRecipe::DielectricWoodland;
                else throw std::invalid_argument("--volume-recipe requires a supported recipe name");
            } else if (argument.starts_with("--volume-recipe=")) {
                throw std::invalid_argument("use --volume-recipe NAME");
            } else if (argument == "--hidden") {
                options.hidden = true;
            } else if (argument == "--no-hud") {
                options.noHud = true;
            } else if (argument == "--desktop") {
                options.desktop = true;
            } else if (argument == "--host-staged") {
                options.hostStaged = true;
            } else if (argument == "--seconds" || argument == "--capture-at") {
                const bool capture = argument == "--capture-at";
                if (index + 1 >= argc || argv[index + 1] == nullptr)
                    throw std::invalid_argument(capture ? "--capture-at requires seconds in [0, 3600]"
                        : "--seconds requires a duration in [0.001, 3600] seconds");
                const std::string_view value(argv[++index]);
                double seconds = 0.0;
                const auto result = std::from_chars(value.data(), value.data() + value.size(), seconds);
                if (result.ec != std::errc{} || result.ptr != value.data() + value.size()
                    || !std::isfinite(seconds) || seconds < (capture ? 0.0 : 0.001) || seconds > 3600.0)
                    throw std::invalid_argument(capture ? "--capture-at requires seconds in [0, 3600]"
                        : "--seconds requires a duration in [0.001, 3600] seconds");
                if (capture) options.captureSeconds = seconds;
                else options.runSeconds = seconds;
            } else if (argument == "--size") {
                if (index + 1 >= argc || argv[index + 1] == nullptr)
                    throw std::invalid_argument("--size requires WIDTHxHEIGHT (each 64 to 8192)");
                const std::string_view value(argv[++index]);
                const auto separator = value.find('x');
                int width = 0, height = 0;
                if (separator == std::string_view::npos)
                    throw std::invalid_argument("--size requires WIDTHxHEIGHT (each 64 to 8192)");
                const auto w = std::from_chars(value.data(), value.data() + separator, width);
                const auto h = std::from_chars(value.data() + separator + 1, value.data() + value.size(), height);
                if (w.ec != std::errc{} || w.ptr != value.data() + separator
                    || h.ec != std::errc{} || h.ptr != value.data() + value.size()
                    || width < 64 || width > 8192 || height < 64 || height > 8192)
                    throw std::invalid_argument("--size requires WIDTHxHEIGHT (each 64 to 8192)");
                options.desktopSize = SculptureDesktopSize{width, height};
            } else if (argument.starts_with("--seconds=") || argument.starts_with("--size=")
                       || argument.starts_with("--hidden=") || argument.starts_with("--no-hud=") || argument.starts_with("--desktop=")
                       || argument.starts_with("--host-staged=") || argument.starts_with("--capture-at=")) {
                throw std::invalid_argument("use --seconds N, --capture-at N, --size WIDTHxHEIGHT, --hidden, --no-hud, --desktop or --host-staged");
            } else if (argument == "--enable-remote" || argument == "--enable-osc") {
                options.enableRemoteControl = true;
            } else if (argument == "--gpu-pass-timing") {
                options.enableGpuPassTiming = true;
            } else if (argument == "--auto-scene-scale") {
                options.enableAutoSceneScale = true;
                options.enableGpuPassTiming = true;
            } else if (argument == "--resource-root"
                       && index + 1 < argc && argv[index + 1] != nullptr) {
                const std::filesystem::path root(argv[++index]);
                if (!root.empty()) options.resourceRoot = root;
            } else if (argument == "--enable-midi") {
                options.enableMidiControl = true;
            } else if (argument == "--midi-profile" && index + 1 < argc && argv[index + 1] != nullptr) {
                options.enableMidiControl = true;
                options.midiProfile = std::string(argv[++index]);
            } else if ((argument == "--preset" || argument == "--world"
                        || argument == "--profile-world") && index + 1 < argc
                       && argv[index + 1] != nullptr) {
                std::string value(argv[++index]);
                if (argument == "--profile-world") {
                    options.profileWorld = std::move(value);
                } else {
                    options.startupSelection = std::move(value);
                    options.startupSelectionKind = argument == "--world"
                        ? SculptureStartupSelection::World
                        : SculptureStartupSelection::Preset;
                }
            } else if (argument == "--relief-contours"
                       && index + 1 < argc && argv[index + 1] != nullptr) {
                const char* text = argv[index + 1];
                if (std::string_view(text).starts_with("--")) continue;
                ++index;
                char* end = nullptr;
                const float density = std::strtof(text, &end);
                if (end != text && *end == '\0' && std::isfinite(density)
                    && density >= 0.0f && density <= kMaxReliefContourDensity) {
                    options.reliefContourDensity = density;
                }
            } else if (argument == "--scene-render-scale"
                       && index + 1 < argc && argv[index + 1] != nullptr) {
                const std::optional<float> scale = parseSceneRenderScale(argv[++index]);
                if (scale) options.sceneRenderScale = *scale;
            } else if (argument == "--scene-gpu-budget-ms"
                       && index + 1 < argc && argv[index + 1] != nullptr) {
                const std::optional<float> budget = parseGpuBudgetMilliseconds(argv[++index]);
                if (budget) options.sceneGpuBudgetMilliseconds = *budget;
            } else if ((argument == "--profile-settle-seconds"
                        || argument == "--profile-sample-seconds")
                       && index + 1 < argc && argv[index + 1] != nullptr) {
                const std::optional<float> seconds = parseSeconds(argv[++index]);
                if (!seconds) continue;
                if (argument == "--profile-settle-seconds") {
                    options.profileSettleSeconds = *seconds;
                } else if (*seconds > 0.0f) {
                    options.profileSampleSeconds = *seconds;
                }
            } else if (argument.ends_with(".world")
                       || argument.ends_with(".epimode")) {
                options.statePath = std::string(argument);
            }
        }
        const float fieldRange = options.fieldPresentation.maximum - options.fieldPresentation.minimum;
        if (!std::isfinite(fieldRange) || fieldRange < std::numeric_limits<float>::min())
            throw std::invalid_argument("field range must be positive and at least the smallest normal float");
        if (cameraEye.has_value() != cameraTarget.has_value())
            throw std::invalid_argument("--camera-eye and --camera-target must be supplied together");
        if (cameraEye) {
            const double dx = static_cast<double>((*cameraTarget)[0]) - (*cameraEye)[0];
            const double dy = static_cast<double>((*cameraTarget)[1]) - (*cameraEye)[1];
            const double dz = static_cast<double>((*cameraTarget)[2]) - (*cameraEye)[2];
            constexpr double kMinimumLengthSquared = 1.0e-8;
            if (dx * dx + dy * dy + dz * dz < kMinimumLengthSquared ||
                dx * dx + dz * dz < kMinimumLengthSquared)
                throw std::invalid_argument("camera direction must be nonzero and not vertical");
            options.cameraPose = SculptureCameraPose{*cameraEye, *cameraTarget};
        }
        if (options.hidden && !options.runSeconds)
            throw std::invalid_argument("--hidden requires --seconds to bound the session");
        if (options.captureSeconds && options.runSeconds
            && *options.captureSeconds >= *options.runSeconds)
            throw std::invalid_argument("--capture-at must precede --seconds");
        return options;
    }

private:
    [[nodiscard]] static float parseFieldScalar(std::string_view value) {
        if (value.empty()) throw std::invalid_argument("field presentation value is empty");
        float number = 0.0f;
        const auto parsed = std::from_chars(value.data(), value.data() + value.size(), number);
        if (parsed.ec != std::errc{} || parsed.ptr != value.data() + value.size() || !std::isfinite(number))
            throw std::invalid_argument("field presentation value must be a finite number");
        return number;
    }

    [[nodiscard]] static std::array<float, 3> parseCameraCoordinates(std::string_view text) {
        std::array<float, 3> result{};
        for (std::size_t i = 0; i < result.size(); ++i) {
            const std::size_t separator = text.find(',');
            if ((i < 2 && separator == std::string_view::npos) ||
                (i == 2 && separator != std::string_view::npos))
                throw std::invalid_argument("camera coordinates require exactly three finite values");
            const auto component = i < 2 ? text.substr(0, separator) : text;
            if (component.empty())
                throw std::invalid_argument("camera coordinates require exactly three finite values");
            float value = 0.0f;
            const auto parsed = std::from_chars(component.data(), component.data() + component.size(), value);
            if (parsed.ec != std::errc{} ||
                parsed.ptr != component.data() + component.size() ||
                !std::isfinite(value) || std::abs(value) > 1.0e6f)
                throw std::invalid_argument("camera coordinates must be finite and within +/-1000000");
            result[i] = value;
            if (i < 2) text.remove_prefix(separator + 1);
        }
        return result;
    }

    [[nodiscard]] static std::optional<float> parseSceneRenderScale(const char* text) {
        if (text == nullptr || *text == '\0') return std::nullopt;
        char* end = nullptr;
        const float scale = std::strtof(text, &end);
        if (end == text || *end != '\0' || !std::isfinite(scale) ||
            scale < 0.25f || scale > 1.0f) {
            return std::nullopt;
        }
        return scale;
    }

    [[nodiscard]] static std::optional<float> parseGpuBudgetMilliseconds(const char* text) {
        if (text == nullptr || *text == '\0') return std::nullopt;
        char* end = nullptr;
        const float milliseconds = std::strtof(text, &end);
        if (end == text || *end != '\0' || !std::isfinite(milliseconds) ||
            milliseconds < 1.0f || milliseconds > 100.0f) {
            return std::nullopt;
        }
        return milliseconds;
    }

    [[nodiscard]] static std::optional<float> parseSeconds(const char* text) {
        if (text == nullptr || *text == '\0') return std::nullopt;
        char* end = nullptr;
        const float seconds = std::strtof(text, &end);
        if (end == text || *end != '\0' || !std::isfinite(seconds)
            || seconds < 0.0f) {
            return std::nullopt;
        }
        return seconds;
    }
};

} // namespace epi_sculpt
