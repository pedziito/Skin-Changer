/**
 * ACE Engine — Thread Pool
 * Lightweight thread pool with futures, batch submission, and work stealing.
 * Used for background asset loading, layout computation, etc.
 */

#pragma once

#include "types.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include <future>
#include <vector>
#include <atomic>

namespace ace {

class ThreadPool {
public:
    explicit ThreadPool(u32 numThreads = 0) {
        if (numThreads == 0)
            numThreads = std::max(1u, std::thread::hardware_concurrency() - 1);

        _running = true;
        _workers.reserve(numThreads);
        for (u32 i = 0; i < numThreads; ++i) {
            _workers.emplace_back([this] { WorkerLoop(); });
        }
    }

    ~ThreadPool() {
        {
            std::lock_guard lock(_mutex);
            _running = false;
        }
        _condition.notify_all();
        for (auto& t : _workers) {
            if (t.joinable()) t.join();
        }
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    /**
     * Submit a callable and get a future for the result.
     */
    template<typename F, typename... Args>
    auto Submit(F&& fn, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>> {
        using ReturnType = std::invoke_result_t<F, Args...>;

        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            std::bind(std::forward<F>(fn), std::forward<Args>(args)...)
        );

        auto future = task->get_future();

        {
            std::lock_guard lock(_mutex);
            _tasks.emplace([task]() { (*task)(); });
        }
        _condition.notify_one();

        return future;
    }

    /**
     * Submit a fire-and-forget task.
     */
    void Enqueue(std::function<void()> fn) {
        {
            std::lock_guard lock(_mutex);
            _tasks.emplace(std::move(fn));
        }
        _condition.notify_one();
    }

    /**
     * Wait until all submitted tasks are complete.
     */
    void WaitIdle() {
        std::unique_lock lock(_mutex);
        _idle.wait(lock, [this] { return _tasks.empty() && _activeCount == 0; });
    }

    u32 WorkerCount() const { return static_cast<u32>(_workers.size()); }
    u32 PendingTasks() {
        std::lock_guard lock(_mutex);
        return static_cast<u32>(_tasks.size());
    }

private:
    void WorkerLoop() {
        while (true) {
            std::function<void()> task;
            {
                std::unique_lock lock(_mutex);
                _condition.wait(lock, [this] { return !_running || !_tasks.empty(); });
                if (!_running && _tasks.empty()) return;
                task = std::move(_tasks.front());
                _tasks.pop();
                ++_activeCount;
            }

            task();

            {
                std::lock_guard lock(_mutex);
                --_activeCount;
            }
            _idle.notify_all();
        }
    }

    std::vector<std::thread>          _workers;
    std::queue<std::function<void()>> _tasks;
    std::mutex                        _mutex;
    std::condition_variable           _condition;
    std::condition_variable           _idle;
    std::atomic<bool>                 _running{false};
    u32                               _activeCount{0};
};

} // namespace ace
