// =============================================================================
// rpc/scope.hpp —— async_scope 等价物（结构化并发）
//
// 对应文档：13-第三阶段结课-RPC框架.md
//   §"Scope 拓扑" / §"必做任务 5 / 8"
//
// 设计要点：
//   - 所有 in-flight 协程必须由 scope 拥有；
//   - 析构前 on_empty() 必须返回，否则有泄漏 / 悬空风险；
//   - 不允许 detach。
//
// 推荐：直接 reuse stdexec::async_scope（如果你在工程中有 stage3 deps）。
// 这里给出最小骨架，便于不依赖 stdexec 时单独跑。
// =============================================================================

#pragma once

#include <atomic>
#include <condition_variable>
#include <coroutine>
#include <functional>
#include <mutex>
#include <vector>

namespace rpc {

class async_scope {
    std::mutex                           mtx_;
    std::condition_variable              cv_;
    std::atomic<int>                     in_flight_{0};

public:
    // TODO[必做]: spawn(task<void>) —— 将一个协程加入 scope，increment 计数；
    //              协程完成时 decrement 并 notify。
    // 实现思路：
    //   - 包一个 detach_helper(task) -> task<void>，内部 co_await 原 task，
    //     完成后调用 scope 的 on_done()；
    //   - 用 coroutine_handle::resume() 启动它（不要持有原 task 让它析构）。
    template <typename Task>
    void spawn(Task&& t) {
        ++in_flight_;
        // TODO: 启动 t、注册回调、完成时 on_done()。
        (void)t;
    }

    void on_done() {
        if (--in_flight_ == 0) {
            std::lock_guard<std::mutex> lk(mtx_);
            cv_.notify_all();
        }
    }

    // 阻塞直到 scope 清空
    void wait_empty() {
        std::unique_lock<std::mutex> lk(mtx_);
        cv_.wait(lk, [&] { return in_flight_.load() == 0; });
    }

    int in_flight() const noexcept { return in_flight_.load(); }

    ~async_scope() {
        // 紧急兜底：不要 detach，必须在析构前自行 wait_empty()
        // TODO[必做]: 在 Debug 构建中加 assert(in_flight_ == 0)
    }
};

} // namespace rpc
