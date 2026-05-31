// =============================================================================
// mini/when_any.hpp —— 竞速等待，第一个完成的 task 触发恢复
//
// 对应文档：14-第三阶段结课-mini协程库实现.md  §"第五层"
//
// 设计要点：
//   - atomic<bool> winner_set 防止多个子 task 同时声称胜出；
//   - 第一个 set_value 的子 task 唤醒等待者，其余子被请求停止（stop_token）；
//   - 与 stdexec::when_any 语义对齐。
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
    //   - 第一个进 set_value 的 receiver 用 atomic<bool>::exchange(true) 抢占胜者；
    //   - 胜者把结果存进 std::variant<Ts...> 后 resume(caller)；
    //   - 其余 receiver 看到 winner 已被抢则丢弃自己的结果。
    //
    // 与 when_all 的不同：error completion 也算"完成"——第一个完成（无论
    // value/error/stopped）即唤醒等待者。
    static_assert(sizeof...(Awaitables) >= 2, "when_any needs >= 2 args");
    return std::variant<int>{};  // 占位
}

} // namespace mini
