// =============================================================================
// mini/when_all.hpp —— 并发等待多个 awaitable
//
// 对应文档：14-第三阶段结课-mini协程库实现.md  §"第五层：when_all 与 when_any"
//
// 设计要点（决策 3 方案 A：fail-fast）：
//   - atomic<int> 计数：每个子 task 完成后 fetch_sub(1)；
//   - 结果存到 std::tuple<...>；
//   - 单个子 task error 立即取消其余并以 error 完成（决策 3 方案 A）；
//   - 完整版需要 completion_signatures 元函数推 tuple 类型 —— 本骨架仅占位。
//
// 用法：
//   auto [a, b] = co_await when_all(task_a(), task_b());
// =============================================================================

#pragma once

#include <atomic>
#include <coroutine>
#include <exception>
#include <optional>
#include <tuple>
#include <utility>

namespace mini {

// 简化骨架：仅支持两个 awaitable，T 同型。完整版用 std::tuple<Ts...> 推导。
template <typename A, typename B>
struct when_all_two {
    A a_;
    B b_;

    struct awaiter {
        when_all_two&                       parent_;
        std::coroutine_handle<>             caller_{};
        std::atomic<int>                    pending_{2};
        std::tuple<std::optional<int>, std::optional<int>> results_; // TODO: 泛化

        bool await_ready() noexcept { return false; }
        // TODO[必做]:
        //   await_suspend: 同时 connect/start 两个子 awaitable；
        //                   每个子的 receiver 在 set_value 时存入 tuple slot
        //                   并 fetch_sub(1)，count==0 时 resume(caller_)；
        //   await_resume:  从 tuple 取出 (a, b)。
        void await_suspend(std::coroutine_handle<> caller) {
            caller_ = caller;
            // TODO
        }
        auto await_resume() {
            // TODO[必做]: 返回 std::tuple<A_result, B_result>
            return std::make_tuple(*std::get<0>(results_),
                                   *std::get<1>(results_));
        }
    };

    awaiter operator co_await() && { return awaiter{*this}; }
};

template <typename... Awaitables>
auto when_all(Awaitables&&... aws) {
    // TODO[必做]: 完整泛化版本 —— 用 std::tuple<...> 存值，atomic count 收束。
    //            建议先写死 N=2 跑通，再用 std::index_sequence 泛化。
    static_assert(sizeof...(Awaitables) >= 2, "when_all needs >= 2 args");
    // 占位返回，仅保证模板可实例化
    return std::tuple<>{};
}

} // namespace mini
