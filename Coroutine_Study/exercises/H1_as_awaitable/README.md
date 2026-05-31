# H-1 as_awaitable 桥

对应文档：`10-模块H-协程与sender_receiver桥接.md` 「练习 H-1」。

## 目标

亲手实现 `as_awaitable(sender) -> awaitable` 的完整桥接通道。bridge_receiver 在
`set_value` / `set_error` / `set_stopped` 中分别 resume 协程，并在 `await_resume`
中把三种 completion 翻译回协程的 return / throw / 特殊标记。

## 必做任务

1. 阅读骨架的三个组件：
   - `bridge_receiver`：覆盖三条 completion channel，用具名 `emplace<value_type>` 等
     把结果存入共享 storage，然后 `coro_.resume()`。
   - `sender_awaitable`：包装 bridge_receiver 和 operation_state（用 `std::optional`
     延迟构造），实现 `await_ready/suspend/resume`。
   - `my_task::promise_type::await_transform(Sender)`：把 `co_await sender` 翻译为
     `as_awaitable(sender, handle)` 路径。
2. 跑通三个测试：value / error / stopped channel 都能正确路由。
3. 在笔记中画出 8 步完整桥接调用链：
   `co_await sender → await_transform → as_awaitable → sender_awaitable →
   await_suspend(connect+start) → 挂起 → set_xxx → resume → await_resume`。

## 进阶任务

- 从 `Sender::completion_signatures` 推导 `value_type`，支持多值（用 `std::tuple`）。
- 为 `bridge_receiver::get_env` 转发协程 promise 的环境（scheduler / stop_token）。
- 处理 sender 在 `start()` 中同步完成的情形——返回 `bool(true)` 或 symmetric transfer。
- 支持 `co_await` 一个 void 返回的 sender——`await_resume` 返回 `void`。

## 验收点

- `co_await stdexec::just(42)` 能正确返回 42。
- `co_await stdexec::just_error(...)` 能在 `await_resume` 中 rethrow 异常。
- `co_await stdexec::just_stopped()` 能进入 stopped 路径并抛特殊异常。
- 你能讲清 op_state 为何不能放在 await_suspend 的栈帧里——必须随 awaitable 同生命周期。

## 提示

- 起手用硬编码 `value_type = int`，跑通后再泛化。
- `std::optional<op_state_t>::emplace` 是延迟构造 op_state 的最简单方式。
- 在 `set_value` 中用 `emplace<value_type>` 而非 `emplace<1>`——variant 重排不会影响。
