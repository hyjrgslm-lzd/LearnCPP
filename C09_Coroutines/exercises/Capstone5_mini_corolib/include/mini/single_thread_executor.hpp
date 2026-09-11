// =============================================================================
// mini/single_thread_executor.hpp —— 最小单线程调度器
//
// 对应文档：14-第三阶段结课-mini协程库实现.md  §"第七层"
//
// 设计要点：
//   - 内部一个 std::queue<std::function<void()>>（或 intrusive queue）；
//   - schedule() 返回 sender，connect 后把 receiver 注册到队列；
//   - run() 循环取出执行；
//   - stop() 设置停止标志。
// =============================================================================

#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <coroutine>
#include <stdexcept>

namespace mini {

class single_thread_executor {
    std::mutex                          mtx_;
    std::condition_variable             cv_;
    std::queue<std::function<void()>>   q_;
    std::atomic<bool>                   stop_{false};

public:
    // Non-blocking driver used by deterministic Student checks.
    bool run_one() {
        std::function<void()> job;
        {
            std::lock_guard lock(mtx_);
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
        void await_suspend(std::coroutine_handle<>) {
            // TODO: enqueue the continuation; do not resume inline.
            throw std::logic_error("TODO: implement mini executor schedule");
        }
        void await_resume() const noexcept {}
    };
    schedule_awaiter schedule() { return {*this}; }

    // 提交一个 callable
    void enqueue(std::function<void()> f) {
        {
            std::lock_guard<std::mutex> lk(mtx_);
            q_.push(std::move(f));
        }
        cv_.notify_one();
    }

    // 阻塞执行循环 —— 由调用线程驱动
    void run() {
        for (;;) {
            std::function<void()> job;
            {
                std::unique_lock<std::mutex> lk(mtx_);
                cv_.wait(lk, [&] { return stop_.load() || !q_.empty(); });
                if (stop_.load() && q_.empty()) return;
                job = std::move(q_.front());
                q_.pop();
            }
            job();
        }
    }

    void stop() {
        stop_.store(true);
        cv_.notify_all();
    }

    // TODO[必做]: get_scheduler() -> scheduler，scheduler.schedule() -> sender
    //  - sender 的 connect(receiver) 返回 op_state；
    //  - op_state.start() 把 receiver 投递到队列；
    //  - 队列消费时调用 set_value(receiver)。
};

} // namespace mini
