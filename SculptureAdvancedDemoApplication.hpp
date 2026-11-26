#pragma once

#include "SculptureAdvancedDemoRunner.hpp"
#include <memory>

namespace epi_sculpt {

// -----------------------------------------------------------------------------
// Educational Annotation: ADVANCED OBJECT-ORIENTED SCULPTURE APPLICATION
// -----------------------------------------------------------------------------
// Extends the OOP pattern to the Advanced Rendering Pipeline.
// This abstract class provides a robust foundation for building interactive,
// audio-reactive, and volumetrically raymarched scenes, completely hiding the
// Vulkan and OpenCL synchronization complexities from the creative developer.
// -----------------------------------------------------------------------------

class SculptureAdvancedDemoApplication {
public:
    virtual ~SculptureAdvancedDemoApplication() = default;

    int run() {
        try {
            AdvancedDemoConfig config = configure();
            
            auto updateStateCallback = [this](
                GLFWwindow* window,
                mgfx::Engine& engine,
                SculpturePresentationDirector& presentation,
                float time,
                SculptureProgramParams& params) {
                
                float deltaSeconds = time - this->lastTime_;
                this->lastTime_ = time;
                this->window_ = window;
                this->engine_ = &engine;
                this->presentation_ = &presentation;
                
                if (!this->isSetup_) {
                    if (auto mesh = this->createMesh(engine)) {
                        presentation.setInstanceMesh(std::move(*mesh));
                    }
                    if (auto mat = this->createMaterial(engine); mat != mgfx::kInvalidMaterial) {
                        presentation.setInstanceMaterial(mat);
                    }
                    this->setup(engine, presentation);
                    this->isSetup_ = true;
                }
                
                this->updateState(time, deltaSeconds, params);
            };

            runAdvancedSculptureDemo(
                config, 
                initialParams(), 
                updateStateCallback
            );
            return 0;
        } catch (const std::exception& error) {
            std::fprintf(stderr, "[AdvancedDemoApplication] Fatal Error: %s\n", error.what());
            return 1;
        }
    }

protected:
    virtual std::optional<mgfx::vk::Mesh> createMesh(mgfx::Engine& engine) { return std::nullopt; }
    virtual mgfx::MaterialHandle createMaterial(mgfx::Engine& engine) { return mgfx::kInvalidMaterial; }

    virtual AdvancedDemoConfig configure() const = 0;
    virtual SculptureProgramParams initialParams() const { return {}; }
    virtual void setup(mgfx::Engine& engine, SculpturePresentationDirector& presentation) {}
    virtual void updateState(float time, float deltaSeconds, SculptureProgramParams& params) {}

    bool isKeyPressed(int glfwKeyCode) const noexcept {
        if (!window_) return false;
        return glfwGetKey(window_, glfwKeyCode) == GLFW_PRESS;
    }

    SculpturePresentationDirector* presentation() const noexcept { return presentation_; }

private:
    float lastTime_ = 0.0f;
    bool isSetup_ = false;
    GLFWwindow* window_ = nullptr;
    mgfx::Engine* engine_ = nullptr;
    SculpturePresentationDirector* presentation_ = nullptr;
};

} // namespace epi_sculpt
