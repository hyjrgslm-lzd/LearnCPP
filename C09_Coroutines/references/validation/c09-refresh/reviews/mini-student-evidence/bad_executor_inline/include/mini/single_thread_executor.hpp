#pragma once

#include <condition_variable>
#include <coroutine>
#include <functional>
#include <mutex>
#include <queue>

namespace mini {

class single_thread_executor {
    std::mutex mtx_;
    std::condition_variable cv_;
    std::queue<std::function<void()>> q_;
    bool stop_{};

public:
    bool run_one() {
        std::function<void()> job;
        {
            std::lock_guard lock{mtx_};
            if (q_.empty()) return false;
            job = std::move(q_.front());
            q_.pop();
        }
        job();
        return true;
    }

    struct schedule_awaiter {
        single_thread_executor& executor;
        bool await_ready() const noexcept { return false; }
        void await_suspend(std::coroutine_handle<> h) {
            (void)executor;
            h.resume();
        }
        void await_resume() const noexcept {}
    };

    schedule_awaiter schedule() { return {*this}; }

    void enqueue(std::function<void()> job) {
        {
            std::lock_guard lock{mtx_};
            q_.push(std::move(job));
        }
        cv_.notify_one();
    }

    void run() {
        for (;;) {
            std::function<void()> job;
            {
                std::unique_lock lock{mtx_};
                cv_.wait(lock, [&] { return stop_ || !q_.empty(); });
                if (stop_ && q_.empty()) return;
                job = std::move(q_.front());
                q_.pop();
            }
            job();
        }
    }

    void stop() {
        {
            std::lock_guard lock{mtx_};
            stop_ = true;
        }
        cv_.notify_all();
    }
};

} // namespace mini
