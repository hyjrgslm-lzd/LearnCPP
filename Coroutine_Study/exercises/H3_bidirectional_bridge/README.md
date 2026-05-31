# H-3 双向桥接——my_task 同时是 sender 和 awaitable

对应文档：`10-模块H-协程与sender_receiver桥接.md` 「练习 H-3」。

## 目标

让自定义协程 task `my_task<T>` 同时具备两种身份：
1. **作为 sender**：能被 `stdexec::sync_wait`、`then`、`when_all` 消费。
2. **作为协程**：能被其它协程 `co_await`，且协程体内能 `co_await` 任意 stdexec sender。

实现完整的双向桥接——task 是 sender-receiver 生态和协程生态之间的通用货币。

## 必做任务

1. 阅读骨架：
   - `task_promise_base` 同时维护 `continuation`（协程侧）和 `external_receiver`（sender 侧）。
   - `final_suspend` 先通知 `external_receiver`，再 symmetric transfer 到 `continuation`。
   - `op_state::start` 注入 erased_receiver，把 continuation 设为 noop。
   - `await_transform(Sender)` 复用 H-1 的桥接路径。
2. 跑通 3 个场景：
   - A：`sync_wait(sender_side())` → 走 sender 协议。
   - B：`sync_wait(outer())`（其内 co_await inner()）→ 走 awaitable 协议。
   - C：`sync_wait(bridge_side())`（其内 co_await stdexec::just(42)）→ 走 await_transform。
3. 在笔记中画双向桥接的完整对象图。
4. 解释 `external_receiver_` 与 `continuation` 的语义差异，以及顺序为什么不能反。

## 进阶任务

- 实现 `my_task<void>`：偏特化 `task_promise_base<void>` + `return_void()` + completion_signatures
  改为 `set_value_t()`。
- 让 `my_task` 参与 `then`/`when_all` 组合链：`my_task<int>(...) | then([](int x){ return x*3; })`。
- 为 `get_env` 加上 scheduler / stop_token 转发。

## 验收点

- 三个场景都能跑通并产出正确结果（42 / 20 / 43）。
- 你能讲清 final_suspend 同时处理两种通知的微妙之处。
- `completion_signatures` 与实际 completion 一致——sender 路径下确实经过 set_value 而非 set_error。

## 提示

- erased_receiver 用 fn-ptr 类型擦除，避免把 Receiver 类型拖进 promise_type。
- `op_state::start` 中先设 `continuation = noop_coroutine()` 再 resume，保证 final_suspend
  不会跳到野指针。
- 如果遇到 "ambiguous overload of `await_transform`"，把 `await_transform(my_task<U>&)` 的
  重载从 `requires sender<>` 路径里排除（concepts 优先级）。
