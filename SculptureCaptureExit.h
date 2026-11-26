#pragma once

#include <vulkan/vulkan_core.h>
#include <cstdio>
#include <exception>
#include <utility>

namespace epi_sculpt {

// Application exit policy only. FrameLoop retains buffer, callback and native
// completion ownership. Finalization is terminal, even when retirement fails.
class SculptureCaptureExit {
public:
    [[nodiscard]] bool started() const noexcept { return started_; }

    template<class Loop, class Wait>
    VkResult finish(bool normal, Loop& loop, Wait&& wait) noexcept {
        if (started_) return result_;
        started_ = true;
        if (normal) {
            result_ = loop.finishCaptures();
        } else {
            loop.cancelCaptures();
            try { result_ = std::forward<Wait>(wait)(); }
            catch (...) { result_ = VK_ERROR_UNKNOWN; }
        }
        return result_;
    }

    static void requireRetired(VkResult result) noexcept {
        if (result == VK_SUCCESS) return;
        std::fprintf(stderr, "Sculpture shutdown: Vulkan retirement failed (%d); stopping before resource destruction\n",
                     static_cast<int>(result));
        std::terminate();
    }

private:
    bool started_ = false;
    VkResult result_ = VK_NOT_READY;
};

} // namespace epi_sculpt
