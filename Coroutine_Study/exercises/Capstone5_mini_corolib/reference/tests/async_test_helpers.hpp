#pragma once

#include <barrier>
#include <coroutine>
#include <latch>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

namespace mini_ref_test {

struct async_threads {
    std::mutex mutex;
    std::vector<std::jthread> threads;

    template <typename F>
    void spawn(F&& f) {
        std::lock_guard lock{mutex};
        threads.emplace_back(std::forward<F>(f));
    }

    void join_all() {
        for (;;) {
            std::vector<std::jthread> batch;
            {
                std::lock_guard lock{mutex};
                batch.swap(threads);
            }
            if (batch.empty()) return;
            for (auto& thread : batch) {
                if (thread.joinable()) thread.join();
            }
        }
    }
};

struct resume_before_suspend_returns {
    async_threads& threads;

    bool await_ready() const noexcept { return false; }
    bool await_suspend(std::coroutine_handle<> h) {
        std::latch resumed{1};
        threads.spawn([h, &resumed] {
            h.resume();
            resumed.count_down();
        });
        resumed.wait();
        return true;
    }
    void await_resume() const noexcept {}
};

struct resume_after_release {
    async_threads& threads;
    std::latch& started;
    std::latch& release;

    bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<> h) {
        auto* release_ptr = &release;
        started.count_down();
        threads.spawn([h, release_ptr] {
            release_ptr->wait();
            h.resume();
        });
    }
    void await_resume() const noexcept {}
};

struct resume_on_barrier {
    async_threads& threads;
    std::latch& started;
    std::barrier<>& finish;

    bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<> h) {
        auto* finish_ptr = &finish;
        started.count_down();
        threads.spawn([h, finish_ptr] {
            finish_ptr->arrive_and_wait();
            h.resume();
        });
    }
    void await_resume() const noexcept {}
};

} // namespace mini_ref_test
