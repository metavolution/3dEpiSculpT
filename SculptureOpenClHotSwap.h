#pragma once

#include <string>
#include <mutex>
#include <atomic>
#include <thread>
#include <utility>
#include <exception>
#include <condition_variable>
#include <optional>
#include <filesystem>
#include <CL/cl.h>
#include <cstdio>

namespace epi_sculpt {

class SculptureOpenClHotSwap {
    // Each candidate owns both references, including before publication and
    // when superseded without ever being acquired by the render thread.
    struct Handles {
        cl_program program = nullptr;
        cl_kernel kernel = nullptr;
        Handles() = default;
        Handles(const Handles&) = delete;
        Handles& operator=(const Handles&) = delete;
        ~Handles() { reset(); }
        void reset() noexcept {
            if (kernel) clReleaseKernel(kernel);
            if (program) clReleaseProgram(program);
            kernel = nullptr;
            program = nullptr;
        }
        void swap(Handles& other) noexcept {
            std::swap(program, other.program);
            std::swap(kernel, other.kernel);
        }
    };
public:
    SculptureOpenClHotSwap(
        cl_context context,
        cl_device_id device,
        std::string kernelName,
        std::filesystem::path includeDirectory = {})
        : context_(context), device_(device), kernelName_(std::move(kernelName)),
          includeDirectory_(std::move(includeDirectory)) {
#ifdef MGFX_EPI_RESOURCE_ROOT
        if (includeDirectory_.empty()) {
            const std::filesystem::path root(MGFX_EPI_RESOURCE_ROOT);
            if (std::filesystem::is_directory(root / "MODULES/3dEpiSculpT/kernels")) {
                includeDirectory_ = root / "MODULES/3dEpiSculpT/kernels";
            } else if (std::filesystem::is_directory(root / "mGFX.production/MODULES/3dEpiSculpT/kernels")) {
                includeDirectory_ = root / "mGFX.production/MODULES/3dEpiSculpT/kernels";
            } else {
                includeDirectory_ = root / "3dEpiSculpTMorph/kernels";
            }
        }
#endif
    }

    ~SculptureOpenClHotSwap() {
        {
            std::lock_guard<std::mutex> lock(requestMutex_);
            stopping_ = true;
            requested_.reset(); // Shutdown does not start another driver build.
        }
        requestCv_.notify_one();
        if (compileThread_.joinable()) compileThread_.join();
        // Member owners retire active AND unconsumed pending handles after join.
    }

    void requestAsyncCompile(const std::string& source, const std::string& buildOptions = "") {
        Request request{source, buildOptions}; // Copy before changing accepted state.
        {
            std::lock_guard<std::mutex> lock(requestMutex_);
            if (stopping_) return;
            // Start once, then reuse the worker. Requesting never joins a build.
            // A failed thread launch leaves both queue and compiling state intact.
            if (!compileThread_.joinable())
                compileThread_ = std::thread([this] { compilerLoop(); });
            requested_ = std::move(request);
            isCompiling_.store(true);
        }
        requestCv_.notify_one();
    }

    // Borrowed until the next acquisition/destruction; the existing dispatcher
    // serializes acquisition and argument binding on its submission thread.
    cl_kernel acquireActiveKernel() {
        std::lock_guard<std::mutex> lock(swapMutex_);
        if (pending_.kernel) {
            active_.swap(pending_);
            pending_.reset();
        }
        return active_.kernel;
    }

    bool isCompiling() const { return isCompiling_.load(); }

private:
    struct Request { std::string source, options; };

    void compilerLoop() {
        std::unique_lock<std::mutex> lock(requestMutex_);
        for (;;) {
            requestCv_.wait(lock, [this] { return stopping_ || requested_.has_value(); });
            if (stopping_) { isCompiling_.store(false); return; }
            Request request = std::move(*requested_);
            requested_.reset();
            lock.unlock();
            try { compileCandidate(request.source, request.options); }
            catch (const std::exception& error) {
                std::fprintf(stderr, "[SculptureOpenClHotSwap] Compile exception: %s\n", error.what());
            } catch (...) {
                std::fprintf(stderr, "[SculptureOpenClHotSwap] Unknown compile exception\n");
            }
            lock.lock();
            // One active build plus one replaceable latest request. Never report
            // idle between builds while a newer accepted edit is still queued.
            if (!requested_) isCompiling_.store(false);
        }
    }

    void compileCandidate(const std::string& source, const std::string& buildOptions) {
        Handles candidate;
        cl_int error = CL_SUCCESS;
        const char* src = source.c_str();
        const size_t length = source.size();
        candidate.program = clCreateProgramWithSource(context_, 1, &src, &length, &error);
        if (error != CL_SUCCESS || !candidate.program) {
            std::fprintf(stderr, "[SculptureOpenClHotSwap] Program creation failed: %d\n", error);
            return;
        }
        std::string defaultBuildOptions = "-cl-fast-relaxed-math";
        if (!includeDirectory_.empty()) {
            // OpenCL accepts quoted include paths; keep resource location a
            // constructor value rather than a developer-machine constant.
            defaultBuildOptions = "-I \"" + includeDirectory_.string()
                + "\" " + defaultBuildOptions;
        }
        const std::string& effectiveBuildOptions =
            buildOptions.empty() ? defaultBuildOptions : buildOptions;
        error = clBuildProgram(candidate.program, 1, &device_,
            effectiveBuildOptions.c_str(), nullptr, nullptr);
        if (error != CL_SUCCESS) {
            size_t size = 0;
            // Driver diagnostics are best effort and bounded; a failed query
            // must not turn an ordinary rejected edit into unbounded allocation.
            if (clGetProgramBuildInfo(candidate.program, device_, CL_PROGRAM_BUILD_LOG,
                    0, nullptr, &size) == CL_SUCCESS && size && size <= 1024 * 1024) {
                std::string log(size + 1, '\0');
                if (clGetProgramBuildInfo(candidate.program, device_, CL_PROGRAM_BUILD_LOG,
                        size, log.data(), nullptr) == CL_SUCCESS)
                    std::fprintf(stderr, "[SculptureOpenClHotSwap] Build failed for '%s': %s\n", kernelName_.c_str(), log.c_str());
            }
            std::fprintf(stderr, "[SculptureOpenClHotSwap] Build failed: %d\n", error);
            return;
        }
        candidate.kernel = clCreateKernel(candidate.program, kernelName_.c_str(), &error);
        if (error != CL_SUCCESS || !candidate.kernel) {
            std::fprintf(stderr, "[SculptureOpenClHotSwap] Kernel creation failed: %d\n", error);
            return;
        }
        {
            std::lock_guard<std::mutex> lock(swapMutex_);
            candidate.swap(pending_);
        }
        // Retire the superseded pending result outside the publication lock.
    }

    cl_context context_;
    cl_device_id device_;
    std::string kernelName_;
    std::filesystem::path includeDirectory_;
    std::thread compileThread_;
    std::atomic<bool> isCompiling_{false};
    std::mutex requestMutex_;
    std::condition_variable requestCv_;
    std::optional<Request> requested_;
    bool stopping_ = false;
    std::mutex swapMutex_;
    Handles pending_;
    Handles active_;
};

} // namespace epi_sculpt
