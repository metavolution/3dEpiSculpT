#pragma once

#include <algorithm>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>
#include <thread>
#include <future>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <type_traits>

namespace epi_sculpt {

// -----------------------------------------------------------------------------
// Educational Annotation: REUSABLE WORKERS FOR CPU REFERENCE/FALLBACK WORK
// -----------------------------------------------------------------------------
// Reusing workers avoids repeated thread creation. The queue is mutex-protected;
// packaged tasks report results/errors through futures. Callers still own task
// grouping and must settle borrowed inputs before those inputs leave scope.
// Startup either owns all requested workers or joins the partial set and throws.
// -----------------------------------------------------------------------------
class SculptureThreadPool {
public:
    explicit SculptureThreadPool(std::size_t numThreads) {
        numThreads = std::max<std::size_t>(1U, numThreads);
        workers_.reserve(numThreads);
        try {
            for (std::size_t i = 0; i < numThreads; ++i) {
                workers_.emplace_back([this] { runWorker(); });
            }
        } catch (...) {
            // A failed constructor does not run our destructor. Join while the
            // queue, mutex and condition variable are still alive, then retain
            // the original allocation/thread-launch error for the caller.
            stopAndJoin();
            throw;
        }
    }

    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>> {
        using return_type = std::invoke_result_t<F, Args...>;

        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        std::future<return_type> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            if (stop_) {
                throw std::runtime_error("enqueue on stopped ThreadPool");
            }
            tasks_.emplace([task]() { (*task)(); });
        }
        condition_.notify_all();
        return res;
    }

    ~SculptureThreadPool() {
        stopAndJoin();
    }

private:
    void runWorker() {
        for (;;) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(queueMutex_);
                condition_.wait(lock, [this] {
                    return stop_ || !tasks_.empty();
                });
                if (stop_ && tasks_.empty()) {
                    return;
                }
                task = std::move(tasks_.front());
                tasks_.pop();
            }
            task();
        }
    }

    // Called by the owning thread, never by one of this pool's workers.
    // Drain accepted tasks before joining; stop_ rejects further submission.
    void stopAndJoin() noexcept {
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            stop_ = true;
        }
        condition_.notify_all();
        for (std::thread& worker : workers_) {
            if (worker.joinable()) worker.join();
        }
    }

    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;

    std::mutex queueMutex_;
    std::condition_variable condition_;
    bool stop_ = false;
};

} // namespace epi_sculpt
