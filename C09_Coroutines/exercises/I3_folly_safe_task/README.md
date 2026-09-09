# I3 Folly coro safe task

知识讲解：[I3 对应章节](../../11-模块I-真实异步IO与并发框架.md#i3)。

对应主讲义：`11-模块I-真实异步IO与并发框架.md` 的 I-3。

这题使用 Folly 的实际 API。普通 `folly::coro::Task<T>` 是 lazy task；safe coroutine 层在类型上限制参数、返回值和闭包捕获，减少协程挂起后引用悬空。

reference 覆盖：

- `value_task<int>`：值语义参数和返回，预测结果 42。
  **答案解析：** `value_task` 约束参数和返回走值语义，协程挂起后不会依赖已经离开作用域的引用参数。本题用它表达普通安全异步计算，结果由 task completion 返回 42。
- `now_task<int>`：要求在创建 full-expression 内立即 await，预测结果 7。
  **答案解析：** `now_task` 的安全性来自“创建后立刻 await”的使用约束，减少把含引用或临时依赖的协程对象保存到稍后运行的窗口。reference 只演示同步的安全形状，返回 7。
- executor-bound `Task`：经 `co_withExecutor` 在 worker 上运行，thread id 应不同。
  **答案解析：** `co_withExecutor(&executor, task)` 把 Folly task 绑定到指定 executor，`co_current_executor` 用来确认当前协程运行在这个 executor 上。线程不同说明恢复位置改变；共享数据是否安全仍要由 executor 串行性、锁或消息边界保证。
- `async_closure(bind::args{...}, ...)`：闭包安全包装，预测结果 6。
  **答案解析：** `async_closure` 把参数按库规则传入 closure task，针对 lambda 捕获跨挂起后悬空的问题。reference 的参数求和得到 6，重点是捕获/参数进入安全生命周期，而不是靠外部栈引用侥幸存活。
- `co_cleanup_safe_task<int>`：cleanup/scope 场景别名，预测结果 9。
  **答案解析：** cleanup safe task 用于清理或 scope 退出相关的协程工作，强调析构/收束阶段仍要满足生命周期约束。reference 返回 9，用一个小值路径验证别名可编译、可等待、可完成。

本题在 Linux `heavy-folly-linux` 预设中启用，需要 Folly v2026.08.31.00 兼容环境。Windows 读者沿 `solution.cpp` 阅读真实 API 和生命周期约束。

```bash
cd C09_Coroutines/exercises
cmake --preset heavy-folly-linux
cmake --build --preset heavy-folly-linux --target I3_folly_safe_task_reference
ctest --test-dir build/heavy-folly-linux -R I3_folly_safe_task_reference --output-on-failure
```

读代码时先看 `add_values` 为什么能返回 `value_task<int>`，再看 `co_withExecutor` 如何绑定 executor，最后看 `async_closure` 如何把参数传入 closure task。`blocking_wait` 只作为测试入口使用。

**答案解析：** `add_values` 的参数和值返回都满足 safe task 的值语义边界；`co_withExecutor` 解决恢复位置，和 safe task 解决生命周期是两条独立边。`blocking_wait` 把 async 世界接回测试 main，放进真实 executor 线程等待同一个 executor 时可能堵住 completion。
