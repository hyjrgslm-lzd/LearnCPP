# 练习 D-3：final_suspend + symmetric transfer

## 目标

在 `final_suspend` 的 `await_suspend` 中返回 `coroutine_handle` 实现 symmetric transfer，
让协程完成时把控制权直接转交给等待者；对比"在 final_suspend 内 .resume()"的天然栈危险。

## 必做任务

1. 实现 `lazy_task_symmetric<T>`，其 `final_awaiter::await_suspend` 返回 `coroutine_handle<>`。
2. 实现对照版 `lazy_task_resume_chain<T>`，其 `final_awaiter::await_suspend` 返回 `void`
   且在内部直接 `.resume()` 等待者。
3. 写一个 N 层 `chain(n)` 协程，分别用两版跑 10 / 100 / 1000 层。
4. 画 symmetric transfer 控制流图：
   `A.final_suspend.await_suspend -> 返回 B.handle -> 框架 resume B -> A 帧已退栈`。

## 验收点

- symmetric 版能跑 1000 层嵌套不爆栈。
- 你能对比 `await_suspend` 的三种返回值（void / bool / coroutine_handle）。
- 你能解释为什么 `continuation` 为空时返回 `std::noop_coroutine()` 而不是空 handle。
- 你能说明 symmetric transfer 不是"性能优化"，而是"栈安全保证"。

## 提示

- 对照实验的 N 视平台调整：MSVC Debug 默认 1MB 栈，~100 层就可能崩。
- 进阶请把 `task::await_suspend` 也改写为 symmetric transfer 版本，
  让 caller -> task -> task -> ... 整条链都走 symmetric。
