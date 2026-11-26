#include "ImplicitSequenceExporter.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <filesystem>

#include "epi/third_party/stb_image_write.h"

namespace epi_sculpt {
namespace {

float acesToneMap(float value) noexcept {
    const float x = std::max(value, 0.0f);
    return std::clamp(
        (x * (2.51f * x + 0.03f))
            / (x * (2.43f * x + 0.59f) + 0.14f),
        0.0f, 1.0f);
}

float linearToSrgb(float value) noexcept {
    const float x = std::clamp(value, 0.0f, 1.0f);
    return x <= 0.0031308f
        ? 12.92f * x
        : 1.055f * std::pow(x, 1.0f / 2.4f) - 0.055f;
}

} // namespace

ImplicitSequenceExporter::ImplicitSequenceExporter(ImplicitFrameProducer& producer, const ExportConfig& config)
    : producer_(producer), config_(config) {
}

void ImplicitSequenceExporter::runSequence(const SculptureProgramParams& initialParams) {
    std::filesystem::create_directories(config_.outputDirectory);
    std::cout << "[SequenceExporter] Rendering " << config_.frameCount 
              << " frames at " << config_.fps << " FPS to " << config_.outputDirectory << "\n";

    SculptureProgramParams currentParams = initialParams;
    const uint32_t width = producer_.width();
    const uint32_t height = producer_.height();

    std::vector<unsigned char> pixels(width * height * 3);

    for (std::uint32_t i = 0; i < config_.frameCount; ++i) {
        float t = static_cast<float>(i) / config_.fps;
        
        // Dynamic camera animation (orbiting)
        float radius = 3.5f + 0.5f * std::sin(t * 1.5f);
        float camX = radius * std::sin(t * 0.5f);
        float camZ = radius * std::cos(t * 0.5f);
        float camY = 1.0f + 0.5f * std::sin(t * 0.8f);

        producer_.setCamera(camX, camY, camZ, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);

        if (!producer_.update(
                currentParams, t, 0.0f, 1.0f / config_.fps, i + 1U)) {
            std::cerr << "[SequenceExporter] Frame " << i << " failed: " << producer_.diagnostic() << "\n";
            continue;
        }
        if (!producer_.waitForGeneration(i + 1U)) {
            std::cerr << "[SequenceExporter] Frame " << i
                      << " did not complete: " << producer_.diagnostic() << "\n";
            continue;
        }

        const auto& output = producer_.getOutput();
        if (output.color.empty() || output.color.size() < width * height * 4U) {
            std::cerr << "[SequenceExporter] Frame " << i << " has invalid color buffer\n";
            continue;
        }
        
        for (size_t p = 0; p < width * height; ++p) {
            float r = output.color[p * 4 + 0];
            float g = output.color[p * 4 + 1];
            float b = output.color[p * 4 + 2];
            
            // The producer is scene-linear HDR. PNG is display-referred, so
            // the offline path explicitly mirrors live ACES + sRGB encoding.
            r = linearToSrgb(acesToneMap(r));
            g = linearToSrgb(acesToneMap(g));
            b = linearToSrgb(acesToneMap(b));

            pixels[p * 3 + 0] = static_cast<unsigned char>(r * 255.0f);
            pixels[p * 3 + 1] = static_cast<unsigned char>(g * 255.0f);
            pixels[p * 3 + 2] = static_cast<unsigned char>(b * 255.0f);
        }

        std::ostringstream filename;
        filename << config_.outputDirectory << "/frame_" 
                 << std::setw(4) << std::setfill('0') << i << ".png";
                 
        stbi_write_png(filename.str().c_str(), width, height, 3, pixels.data(), width * 3);
        std::cout << "[SequenceExporter] Saved " << filename.str() << "\n";

        if (config_.exportGBufferChannels) {
            if (!output.depth.empty() && output.depth.size() >= width * height) {
                std::vector<unsigned char> depthPixels(width * height);
                float maxDepth = 1.0f;
                for (size_t p = 0; p < width * height; ++p) {
                    if (std::isfinite(output.depth[p]) && output.depth[p] < 99.0f) {
                        maxDepth = std::max(maxDepth, output.depth[p]);
                    }
                }
                for (size_t p = 0; p < width * height; ++p) {
                    const float d = std::clamp(output.depth[p] / maxDepth, 0.0f, 1.0f);
                    depthPixels[p] = static_cast<unsigned char>(d * 255.0f);
                }
                std::ostringstream depthFilename;
                depthFilename << config_.outputDirectory << "/frame_"
                              << std::setw(4) << std::setfill('0') << i << "_depth.png";
                stbi_write_png(depthFilename.str().c_str(), width, height, 1, depthPixels.data(), width);
            }
            if (!output.normal.empty() && output.normal.size() >= width * height * 4U) {
                std::vector<unsigned char> normalPixels(width * height * 3);
                for (size_t p = 0; p < width * height; ++p) {
                    const float nx = std::clamp(output.normal[p * 4 + 0] * 0.5f + 0.5f, 0.0f, 1.0f);
                    const float ny = std::clamp(output.normal[p * 4 + 1] * 0.5f + 0.5f, 0.0f, 1.0f);
                    const float nz = std::clamp(output.normal[p * 4 + 2] * 0.5f + 0.5f, 0.0f, 1.0f);
                    normalPixels[p * 3 + 0] = static_cast<unsigned char>(nx * 255.0f);
                    normalPixels[p * 3 + 1] = static_cast<unsigned char>(ny * 255.0f);
                    normalPixels[p * 3 + 2] = static_cast<unsigned char>(nz * 255.0f);
                }
                std::ostringstream normalFilename;
                normalFilename << config_.outputDirectory << "/frame_"
                               << std::setw(4) << std::setfill('0') << i << "_normal.png";
                stbi_write_png(normalFilename.str().c_str(), width, height, 3, normalPixels.data(), width * 3);
            }
            if (!output.velocity.empty() && output.velocity.size() >= width * height * 2U) {
                std::vector<unsigned char> velPixels(width * height * 3);
                for (size_t p = 0; p < width * height; ++p) {
                    const float vx = std::clamp(output.velocity[p * 2 + 0] * 0.5f + 0.5f, 0.0f, 1.0f);
                    const float vy = std::clamp(output.velocity[p * 2 + 1] * 0.5f + 0.5f, 0.0f, 1.0f);
                    velPixels[p * 3 + 0] = static_cast<unsigned char>(vx * 255.0f);
                    velPixels[p * 3 + 1] = static_cast<unsigned char>(vy * 255.0f);
                    velPixels[p * 3 + 2] = 128; // neutral
                }
                std::ostringstream velFilename;
                velFilename << config_.outputDirectory << "/frame_"
                            << std::setw(4) << std::setfill('0') << i << "_velocity.png";
                stbi_write_png(velFilename.str().c_str(), width, height, 3, velPixels.data(), width * 3);
            }
        }
    }
}

} // namespace epi_sculpt
