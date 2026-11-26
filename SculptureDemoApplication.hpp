#pragma once

#include "SculptureDemoRunner.hpp"
#include "SculptureStateRecorder.h"
#include <memory>

namespace epi_sculpt {

// -----------------------------------------------------------------------------
// Educational Annotation: OBJECT-ORIENTED SCULPTURE APPLICATION FRAMEWORK
// -----------------------------------------------------------------------------
// This abstract base class provides a modular, fault-tolerant execution flow
// for developing interactive 3D mathematics demos. It replaces the legacy
// lambda-based `runSculptureDemo` function with a strict lifecycle interface.
//
// Lifecycle:
// 1. configure()   - Define window, camera, and environment settings.
// 2. setup()       - One-time initialization (meshes, choreographers).
// 3. updateState() - Per-frame logic (input, physics, morphing).
// 4. mapInstances()- Transforms OpenCL abstract math into 3D renderable primitives.
// -----------------------------------------------------------------------------

class SculptureDemoApplication {
public:
    virtual ~SculptureDemoApplication() = default;

    // Execution entry point
    int run() {
        try {
            DemoConfig config = configure();
            
            // We use the robust underlying runner, binding our OOP methods to its callbacks
            auto makeMeshCallback = [this](mgfx::Engine& e) { return this->createMesh(e); };
            
            auto updateInstancesCallback = [this](
                const gpu_abi::PackedSculptureSnapshot& snapshot,
                float time,
                const SculptureProgramParams& params,
                std::vector<mgfx::vk::InstanceData>& instances) {
                this->mapInstances(snapshot, time, params, instances);
            };
            
            auto updateStateCallback = [this](
                GLFWwindow* window,
                mgfx::Engine& engine,
                float time,
                SculptureProgramParams& params,
                mgfx::Camera& camera) {
                
                // Track internal delta time safely without static variables
                float deltaSeconds = time - this->lastTime_;
                this->lastTime_ = time;
                
                this->window_ = window;
                this->engine_ = &engine;
                
                // One-time deferred setup once engine is active
                if (!this->isSetup_) {
                    this->setup(engine);
                    this->isSetup_ = true;
                }
                
                this->updateState(time, deltaSeconds, params, camera);
            };

            runSculptureDemo(
                config, 
                makeMeshCallback, 
                createMaterial(), 
                initialParams(), 
                updateInstancesCallback, 
                updateStateCallback
            );
            return 0;
        } catch (const std::exception& error) {
            std::fprintf(stderr, "[SculptureDemoApplication] Fatal Error: %s\n", error.what());
            return 1;
        }
    }

protected:
    // --- Lifecycle Overrides ---

    // 1. Configure the demo environment (Camera, Lights, Atmosphere, Title)
    virtual DemoConfig configure() const = 0;

    // 2. Provide the initial program parameters to seed the OpenCL engine
    virtual SculptureProgramParams initialParams() const { return {}; }

    // 3. Create the primitive mesh (e.g., mgfx::vk::Mesh::makeCube)
    virtual mgfx::vk::Mesh createMesh(mgfx::Engine& engine) = 0;

    // 4. Create the PBR material properties for the instances
    virtual mgfx::pbr::Material createMaterial() const { return {}; }

    // 5. One-time setup routine (called on the first frame)
    virtual void setup(mgfx::Engine& engine) {}

    // 6. Per-frame state update (Morphing, User Input, Physics, Cinematic Camera)
    virtual void updateState(float time, float deltaSeconds, SculptureProgramParams& params, mgfx::Camera& camera) {
        // Fallback for backwards compatibility with legacy apps that don't override the new signature
        updateState(time, deltaSeconds, params);
    }
    
    // Legacy signature (deprecated, prefer the one with camera)
    virtual void updateState(float time, float deltaSeconds, SculptureProgramParams& params) {}

    // 7. Map the OpenCL abstract math snapshot into physical 3D instance transforms
    virtual void mapInstances(
        const gpu_abi::PackedSculptureSnapshot& snapshot,
        float displayTime,
        const SculptureProgramParams& params,
        std::vector<mgfx::vk::InstanceData>& instances) = 0;

    // --- Provided Utilities ---
    
    // Safely checks if a specific key is currently pressed
    bool isKeyPressed(int glfwKeyCode) const noexcept {
        if (!window_) return false;
        return glfwGetKey(window_, glfwKeyCode) == GLFW_PRESS;
    }
    
    // Captures the current frame state into a highly reproducible snapshot
    void requestFrameCapture(float time, const SculptureProgramParams& params, const std::string& label = "capture") {
        if (!engine_) return;
        SculptureWorldState state{};
        state.time = time;
        SculptureWorldMode mode = SculptureStateRecorder::makeCaptureWorldMode(
            state, label, "demo-v1", params, {});
        engine_->frameLoop().requestCapture([mode](const mgfx::vk::CaptureResult& res) {
            SculptureStateRecorder::captureFrameAndState(res, mode);
        });
    }

private:
    float lastTime_ = 0.0f;
    bool isSetup_ = false;
    GLFWwindow* window_ = nullptr;
    mgfx::Engine* engine_ = nullptr;
};

} // namespace epi_sculpt
