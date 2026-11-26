#pragma once

#include "SculptureProgramSchema.h"
#include "SculptureExternalMemoryImport.h"
#include "SculptureOpenClContext.h"
#include "SculptureGpuAbi.h"
#include "SculptureWorldGpuAbi.h"
#include "SculptureOpenClHotSwap.h"
#include <CL/cl.h>
#include <CL/cl_ext.h>
#include <array>
#include <cstddef>
#include <memory>

namespace epi_sculpt {

class SculptureGridOpenClDispatcher {
public:
    SculptureGridOpenClDispatcher(cl_context context, cl_device_id device, cl_command_queue commandQueue);
    ~SculptureGridOpenClDispatcher();

    SculptureGridOpenClDispatcher(const SculptureGridOpenClDispatcher&) = delete;
    SculptureGridOpenClDispatcher& operator=(const SculptureGridOpenClDispatcher&) = delete;

    void initializeKernel(const char* sourceCode);
    void recompileKernelAsync(const char* source, const char* buildOptions = "");
    
    [[nodiscard]] bool importExternalOutput(std::uint32_t slotIndex, const SculptureOpenClImportSlot& importSlot);
    
    bool submitGridTransform(
        const SculptureProgramParams& params,
        float time,
        float morph,
        std::uint32_t gridResolution,
        std::uint32_t slotIndex);

private:
    cl_context context_ = nullptr;
    cl_device_id device_ = nullptr;
    cl_command_queue commandQueue_ = nullptr;
    std::unique_ptr<SculptureOpenClHotSwap> hotSwap_;

    struct Slot {
        cl_mem buffer = nullptr;
        std::size_t byteSize = 0;
        // OpenCL borrows these bytes until both upload events are terminal.
        gpu_abi::SculptureProgramParamsGpu params{};
        gpu_abi::SculptureFrameGpu frame{};
        std::array<cl_event, 2> uploads{};
    };
    // Fixed storage: queued host pointers must never move during a resize.
    std::array<Slot, kSculptureExternalSlotCount> slots_{};
    static bool retireUploads(Slot& slot) noexcept;

    cl_mem paramBuffer_ = nullptr;
    cl_mem frameBuffer_ = nullptr;
};

} // namespace epi_sculpt
