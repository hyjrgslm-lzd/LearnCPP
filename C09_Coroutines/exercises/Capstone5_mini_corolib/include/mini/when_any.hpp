// =============================================================================
// mini/when_any.hpp —— 竞速等待，首个成功 value 胜出，全部分支收束后完成
//
// 对应文档：14-第三阶段结课-mini协程库实现.md  §"第五层"
//
// 设计要点：
//   - winner_set 防止多个子 task 同时写入胜者；
//   - 第一个成功 set_value 的子 task 写入 std::variant<Ts...> 并 request_stop；
//   - loser 仍必须完成收束，operation 才能恢复等待者；
//   - 若所有分支都失败，await_resume 传播首个异常。
// =============================================================================

#pragma once

#include <atomic>
#include <coroutine>
#include <variant>
#include <utility>
#include <stdexcept>
#include <stop_token>
#include "mini/task.hpp"

namespace mini {

template <typename A, typename B>
struct when_any_operation {
    task<A> left;
    task<B> right;
    std::stop_source stop;
    bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<>) {
        // TODO: start both children, publish one winner, cancel and drain loser.
        throw std::logic_error("TODO: implement mini::when_any start/winner/drain");
    }
    std::variant<A, B> await_resume() {
        throw std::logic_error("TODO: implement mini::when_any result/error");
    }
};

template <typename A, typename B>
auto when_any(task<A> left, task<B> right, std::stop_source stop = {}) {
    // TODO[必做]: 完整实现：
    //   - 为每个子 awaitable 创建一个 receiver；
    //   - 第一个成功 set_value 的 receiver 抢占胜者；
    //   - 胜者把结果存进 std::variant<Ts...> 后 request_stop；
    //   - 其余 receiver/runner 仍要运行到完成并递减 remaining；
    //   - remaining==0 时 resume(caller)；
    //   - await_resume 有 winner 则返回，没有 winner 则抛首个异常。
    //
    // 与 when_all 的不同：when_any 的 value 会竞争 winner；error 只在没有
    // 任何成功 value 时成为最终结果。两者都要等待全部分支收束。
    // Mandatory contract is binary, non-void; variadic support is an extension.
    return when_any_operation<A, B>{std::move(left), std::move(right), std::move(stop)};
}

} // namespace mini
