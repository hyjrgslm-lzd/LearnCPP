// =====================================================================
// 练习 C-3：async_scope 与生命周期收束
// 文档参考：Coroutine_Study/04-模块C-取消与组合.md  -> 练习 C-3
// 官方参考：
//   - P3149R4: async scope、spawn 与 counting scope
//   - P3296R1: 自动 join 的 let_async_scope adaptor
//   - Lewis Baker, "Structured Concurrency" CppCon 2019
//   - cppcoro async_scope.hpp 设计
// 学习要点：
//   1. scope.spawn(task) 把协程帧的拥有权交给 scope。
//   2. scope 析构必须等齐所有未完成的 task，禁止 detach。
//   3. 结构化并发的核心是"拥有关系"而不是"启动技巧"。
// =====================================================================
#include "coroutine_study/lazy_task.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <coroutine>
#include <iostream>
#include <mutex>
#include <print>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

struct async_sleep {
    std::chrono::milliseconds dur;
    bool await_ready() const noexcept { return dur <= 0ms; }
    void await_suspend(std::coroutine_handle<> h) const {
        std::this_thread::sleep_for(dur);
        h.resume();
    }
    void await_resume() const noexcept {}
};

// ---------------------------------------------------------------------
// 极简 async_scope：维护一个待完成 task 列表，析构时等齐。
// 实战中应使用成熟库实现；标准化方向见 P3149R4，异常安全 adaptor 见 P3296R1。
// ---------------------------------------------------------------------
class async_scope {
public:
    async_scope() = default;
    async_scope(const async_scope&) = delete;
    async_scope& operator=(const async_scope&) = delete;

    // TODO [必做 1]：实现 spawn(lazy_task<void>)
    //   - 增加 in_flight_ 计数
    //   - 起一条线程（或调度器任务）消费这个 task
    //   - task 完成时减少计数，notify 等待者
    void spawn(coroutine_study::lazy_task<void> task) {
        in_flight_.fetch_add(1, std::memory_order_relaxed);
        workers_.emplace_back([this, t = std::move(task)]() mutable {
            try {
                coroutine_study::sync_wait(std::move(t));
            } catch (...) { /* TODO：异常归属策略 */ }
            if (in_flight_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
                std::lock_guard<std::mutex> lk(mu_);
                cv_.notify_all();
            }
        });
    }

    // TODO [必做 2]：实现析构等齐语义
    //   wait until in_flight_ == 0，确保不会在 task 还没完成时 scope 已死。
    ~async_scope() {
        std::unique_lock<std::mutex> lk(mu_);
        cv_.wait(lk, [this] { return in_flight_.load(std::memory_order_acquire) == 0; });
    }

    // TODO [必做 3]：可选的 on_empty() awaiter，
    //   当所有 spawn 的 task 完成时 resume 等待者。
    //   提示：保存 coroutine_handle，在计数归零时 .resume()。

private:
    std::atomic<int> in_flight_{0};
    std::mutex mu_;
    std::condition_variable cv_;
    std::vector<std::jthread> workers_;
};

// ---------------------------------------------------------------------
// 反例：detach 启动 —— 不知道协程何时结束、被谁拥有。
// ---------------------------------------------------------------------
void detach(coroutine_study::lazy_task<void> task) {
    std::thread([t = std::move(task)]() mutable {
        try { coroutine_study::sync_wait(std::move(t)); } catch (...) {}
    }).detach();
}

int main() {
    std::println("===== 练习 C-3：async_scope =====\n");

    // ------------------ scope.spawn 演示 ------------------
    {
        auto t0 = std::chrono::steady_clock::now();
        {
            async_scope scope;
            for (int i = 0; i < 5; ++i) {
                scope.spawn([](int id) -> coroutine_study::lazy_task<void> {
                    co_await async_sleep{std::chrono::milliseconds{id * 50}};
                    std::println("  [task {}] done", id);
                }(i));
            }
            std::println("  scope 即将析构，开始等齐 ...");
        }
        auto dt = std::chrono::steady_clock::now() - t0;
        std::println("  scope 析构完成；总耗时 {}ms\n",
                     std::chrono::duration_cast<std::chrono::milliseconds>(dt).count());
    }

    // ------------------ detach 反例 ------------------
    // TODO [必做 4]：把上面同样的 5 个 task 改用 detach()，
    //   观察 main 退出时是否还有 task 没完成（行为依赖调度，可能丢日志）。
    //
    //   for (int i = 0; i < 5; ++i) {
    //       detach([](int id) -> coroutine_study::lazy_task<void> {
    //           co_await async_sleep{...};
    //           std::println("  [detached {}] done", id);
    //       }(i));
    //   }
    //   std::this_thread::sleep_for(10ms); // 故意不等齐

    // ------------------ UB 观察实验 ------------------
    // TODO [必做 5]：构造一个故意悬挂的场景：
    //   在内部块定义 std::string msg; spawn 一个 task 引用 msg；
    //   然后让 msg 离开作用域但 scope 还活着——分析为什么 scope 的
    //   生命周期设计应防范这种引用方向。
    //
    //   注：此实验用于观察，不要在生产代码中复现。

    // ------------------ 进阶 ------------------
    // TODO [进阶 1]：spawn_with_future(task) 返回可 co_await 的句柄。
    // TODO [进阶 2]：scope.request_stop() 给所有 spawn 的 task 发取消。
    // TODO [进阶 3]：实现 scope 嵌套：父 scope 析构时先等子 scope 收束。

    std::println("\n===== Done =====");
    return 0;
}
