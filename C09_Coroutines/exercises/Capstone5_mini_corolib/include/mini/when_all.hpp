// =============================================================================
// mini/when_all.hpp —— 并发等待多个 awaitable
//
// 对应文档：14-第三阶段结课-mini协程库实现.md  §"第五层：when_all 与 when_any"
//
// 设计要点（当前 reference：fail-delay，全分支收束后再处理首错）：
//   - remaining 计数：每个子 task 完成后递减；
//   - 结果存到 std::tuple<...>；
//   - 单个子 task error 记录首个 exception_ptr，其余分支继续收束；
//   - remaining 到 0 后恢复等待者，await_resume 再返回 tuple 或重抛首错；
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
#include <stdexcept>
#include <tuple>
#include <type_traits>
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
        //   await_suspend: 同时启动两个子 awaitable/runner；
        //                   每个 runner 完成时在 operation 状态中写入 tuple slot
        //                   或记录首个异常，并递减 pending；
        //                   pending==0 时恢复 caller_。
        //   await_resume:  若记录了异常则重抛，否则从 tuple 取出 (a, b)。
        void await_suspend(std::coroutine_handle<> caller) {
            caller_ = caller;
            // TODO
        }
        std::tuple<int, int> await_resume() {
            // TODO[必做]: 返回 std::tuple<A_result, B_result>
            throw std::logic_error{"TODO: implement mini::when_all"};
        }
    };

    awaiter operator co_await() && { return awaiter{*this}; }
};

template <typename... Awaitables>
auto when_all(Awaitables&&... aws) {
    // TODO[必做]: 完整泛化版本 —— 用 std::tuple<...> 存值，remaining 收束，
    //            fail-delay：全部分支完成后再传播首个异常。
    //            建议先写死 N=2 跑通，再用 std::index_sequence 泛化。
    static_assert(sizeof...(Awaitables) >= 2, "when_all needs >= 2 args");
    static_assert(sizeof...(Awaitables) == 2,
                  "starter skeleton only has the two-argument exercise shape");
    return when_all_two<std::decay_t<Awaitables>...>{std::forward<Awaitables>(aws)...};
}

} // namespace mini
