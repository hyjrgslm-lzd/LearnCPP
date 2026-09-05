// =============================================================================
// rpc/scope.hpp —— co_spawn completion handler 的 in-flight 计数器
//
// 设计要点：
//   - 每次 co_spawn 前调用 on_spawn()；
//   - completion handler 中调用 on_complete()；
//   - shutdown/stop 之后 wait_empty()，证明没有后台协程遗留；
//   - 不使用 asio::detached，也不使用裸线程 detach。
// =============================================================================

#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>

namespace rpc {

class in_flight_scope {
    mutable std::mutex      mtx_;
    std::condition_variable cv_;
    std::atomic<int>        in_flight_{0};

public:
    void on_spawn() noexcept {
        in_flight_.fetch_add(1, std::memory_order_relaxed);
    }

    void on_complete() {
        if (in_flight_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            std::lock_guard<std::mutex> lk(mtx_);
            cv_.notify_all();
        }
    }

    void wait_empty() {
        std::unique_lock<std::mutex> lk(mtx_);
        cv_.wait(lk, [&] {
            return in_flight_.load(std::memory_order_acquire) == 0;
        });
    }

    int in_flight() const noexcept {
        return in_flight_.load(std::memory_order_acquire);
    }
};

} // namespace rpc
