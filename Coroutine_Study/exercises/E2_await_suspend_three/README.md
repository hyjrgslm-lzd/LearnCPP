# 练习 E-2：`await_suspend` 三种返回值

对应正文：[07 模块 E](../../07-模块E-awaitable三层与co_await变换.md#e2)。

本练习只观察控制权。每个 awaiter 都要能回答两个问题：当前协程是否保持挂起；挂起后谁负责让它继续。

## Part 1：`void`

`awaiter_void::await_suspend(h)` 保存当前 handle 并返回 `void`。当前协程保持挂起，控制权回到调用 `resume()` 的外部代码。

随后 `main` 调用保存的 handle：

```cpp
awaiter_void::saved_handle.resume();
```

此时协程从 `await_resume()` 继续。若没有保存 handle，这个协程就没有恢复入口。

## Part 2：`bool true`

`bool true` 与 `void` 一样表示保持挂起。区别是返回类型给 awaiter 一个运行时选择机会。它要保存 handle，再返回 `true`。

日志应显示：第一次 `start()` 后停住，外部 `resume()` 后才打印 after-await。

## Part 3：`bool false`

`bool false` 表示不保持挂起。虽然 `await_ready()` 已经返回 `false`，但 `await_suspend()` 拿到 handle 后又发现可以立即继续，于是返回 `false`，协程马上执行 `await_resume()`。

这和 `await_ready()==true` 的差别在时机：`await_suspend` 已经拿到了当前协程 handle，可以基于登记后的共享状态做最终决定。

## Part 4：`coroutine_handle<>`

`awaiter_symmetric` 返回目标协程 handle。当前协程挂起后，`co_await` 变换恢复目标协程。

观察 A/B 两个协程时，先创建 B，取得 B 的 handle，再让 A 的 awaiter 返回它。日志应显示 A 在 await 点把控制权交给 B，B 运行后，A 的后续执行依赖 B 的完成路径恢复。

## 验收

- `void` 和 `bool true` 都需要外部恢复。

  **答案解析：** 这两种返回都表示当前协程保持挂起。awaiter 必须保存当前 handle，之后由 `main`、事件循环、回调或其他完成路径调用 `resume()`。E-2 的日志中，挂起后不会继续打印 after-await，直到外部恢复。
- `bool false` 立即继续到 `await_resume()`。

  **答案解析：** `await_ready()` 已经返回 false，所以当前协程进入了 `await_suspend()`；但 `await_suspend()` 返回 false 表示取消本次挂起。控制流立即回到当前协程并执行 `await_resume()`，适合表达“登记时发现结果已经可用”的同步完成场景。
- `coroutine_handle<>` 把控制权交给返回的目标 handle。

  **答案解析：** 返回 handle 的 `await_suspend` 表示当前协程挂起后，接下来恢复另一个协程。task 的 child/continuation 模型就靠这个机制连接：caller 挂起，child 运行，child 完成后再返回 caller。E-2 的 handle awaiter 用 `std::noop_coroutine()` 观察这条返回值语义。
- 你能标出每条日志由 main、awaiter、当前协程还是目标协程打印。

  **答案解析：** 日志归属能防止把“谁调用 `resume()`”和“协程恢复后执行哪里”混在一起。`await_suspend` 的日志来自 awaiter，外部恢复来自 main 或调度器，`await_resume` 与 after-await 来自当前协程，handle-return 的目标协程日志来自被返回的 handle。能标清执行者，就能判断控制权是否真的转交。

## Reference

Reference 断言 `void` 可自行 resume，`bool false` 立即继续，`bool true` 保持挂起，handle 返回值把控制权交给目标 handle。

```powershell
cmake -S . -B build/dg-lane -DCOROUTINE_STUDY_BUILD_REFERENCE=ON
cmake --build build/dg-lane --config Release --target E2_await_suspend_three E2_await_suspend_three_reference
ctest --test-dir build/dg-lane -C Release -R E2_await_suspend_three_reference
```
