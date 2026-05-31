// =============================================================================
// mini/async_scope.hpp —— 结构化并发 scope
//
// 对应文档：14-第三阶段结课-mini协程库实现.md  §"第八层：async_scope 与 stop_token"
//
// 设计要点：
//   - spawn(sender)：在 scope 中启动一个任务，记录 operation_state；
//   - on_empty()：返回 sender，在 scope 变空时完成；
//   - 析构等待所有 in-flight；
//   - operation_state 必须以 owning 方式管理（unique_ptr 或类型擦除）。
//
// operation_state non-movable 约束（14-mini §"设计约束"）对本类的影响：
//   - 不能用 std::vector<op_state> —— 每次 push_back 会 move；
//   - 必须 std::vector<std::unique_ptr<op_state_base>> 或 intrusive list。
// =============================================================================

#pragma once

#include <atomic>
#include <condition_variable>
#include <coroutine>
#include <memory>
#include <mutex>
#include <vector>

namespace mini {

class async_scope {
    std::mutex                  mtx_;
    std::condition_variable     cv_;
    std::atomic<int>            in_flight_{0};

    struct op_state_base {
        virtual ~op_state_base() = default;
        virtual void start() = 0;
    };
    std::vector<std::unique_ptr<op_state_base>> states_;

public:
    // TODO[必做]: spawn(sender)：
    //   - 包一个 detach_helper 协程，内部 co_await std::move(sender)；
    //   - completed 时 on_done()；
    //   - 把 op_state 放入 states_。
    template <typename Sender>
    void spawn(Sender&&) {
        ++in_flight_;
        // TODO: connect/start 并存 unique_ptr<op_state_base>
    }

    void on_done() {
        if (--in_flight_ == 0) {
            std::lock_guard<std::mutex> lk(mtx_);
            cv_.notify_all();
        }
    }

    // TODO[必做]: on_empty() -> sender 形式（与 sync_wait 对接）
    void wait_empty() {
        std::unique_lock<std::mutex> lk(mtx_);
        cv_.wait(lk, [&] { return in_flight_.load() == 0; });
    }

    int in_flight() const noexcept { return in_flight_.load(); }

    ~async_scope() {
        // 兜底：如果还有 in-flight，阻塞等待 —— 14-mini §"常见坑"中的
        // "spawn 后 op_state 立即析构"必须避免。
        wait_empty();
    }
};

} // namespace mini
