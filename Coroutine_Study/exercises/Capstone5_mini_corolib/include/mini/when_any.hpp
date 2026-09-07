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

namespace mini {

template <typename... Awaitables>
auto when_any(Awaitables&&... aws) {
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
    static_assert(sizeof...(Awaitables) >= 2, "when_any needs >= 2 args");
    return std::variant<int>{};  // 占位
}

} // namespace mini
