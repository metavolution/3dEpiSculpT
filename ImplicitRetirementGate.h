#pragma once

#include <vulkan/vulkan_core.h>
#include <utility>

namespace epi_sculpt {

// A returned retirement error ends this implicit session. There is deliberately
// no reset/retry API: a failed wait is not permission to replace live owners.
// This gates replacement only; it cannot establish safe device-loss destruction.
class ImplicitRetirementGate {
public:
    [[nodiscard]] bool stopped() const noexcept { return result_ != VK_SUCCESS; }
    [[nodiscard]] VkResult result() const noexcept { return result_; }

    template<class Wait, class Replace>
    bool replaceAfterRetirement(Wait&& wait, Replace&& replace) {
        if (stopped()) return false;
        result_ = std::forward<Wait>(wait)();
        if (stopped()) return false;
        return std::forward<Replace>(replace)();
    }

private:
    VkResult result_ = VK_SUCCESS;
};

} // namespace epi_sculpt
