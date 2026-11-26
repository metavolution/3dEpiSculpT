#pragma once

#include <future>
#include <vector>

namespace epi_sculpt::detail {

// Scope-bound completion for pooled tasks borrowing caller-owned data.
// Declare after the borrowed locals and a reserved future vector, before enqueue.
// Reserve enough handles up front: moving a future into that storage cannot
// allocate after the pool accepts its task. Do not move/clear the vector while
// this guard is alive.
//
// The caller uses get() in submission order to observe errors/results normally.
// If enqueue, get or result assembly throws, consume every remaining future
// before borrowed locals expire, preserving the exception already in flight.
// This guard owns completion only; it neither submits nor schedules work.
template<class Result>
class SculptureFutureDrain final {
public:
    explicit SculptureFutureDrain(std::vector<std::future<Result>>& futures) noexcept
        : futures_(futures) {}

    SculptureFutureDrain(const SculptureFutureDrain&) = delete;
    SculptureFutureDrain& operator=(const SculptureFutureDrain&) = delete;

    ~SculptureFutureDrain() noexcept {
        for (auto& future : futures_) {
            if (!future.valid()) continue;
            try {
                (void)future.get();
            } catch (...) {
                // Cleanup cannot replace the caller's original failure.
            }
        }
    }

private:
    std::vector<std::future<Result>>& futures_;
};

} // namespace epi_sculpt::detail
