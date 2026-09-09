# 练习 A-2：`co_return` 与 lazy task

先读 [模块 A 的 co_return 章节](../../02-模块A-三关键字与最小协程.md#a2)。本题使用共享 [lazy_task.hpp](../include/coroutine_study/lazy_task.hpp)，不要复制旧版 task。

本题验证：调用协程函数只创建 `lazy_task` 和协程状态；`coroutine_study::sync_wait(std::move(task))` 才启动 root 协程；`co_return` 的值先进入 promise，再由消费者取走。

## Part 1：异步版本

打开 [main.cpp](main.cpp)，补全 `compute_async(int x)`：

```cpp
int step1 = x + 10;
int step2 = step1 * 2;
int step3 = step2 - 5;
co_return step3;
```

每一步前后都打印日志。运行前先预测：`auto task = compute_async(5);` 后不会出现 `[coro]` 日志。

**答案解析：** 预测结论是不会出现 `[coro]`。lazy task 的 `initial_suspend()` 先挂起，调用 `compute_async(5)` 时只完成协程帧、promise 和返回对象的创建。`[coro]` 日志要等 `sync_wait(std::move(task))` 恢复 root 协程后才会打印。

## Part 2：同步对照

补全 `compute_sync(int x)`，做同样三步。同步函数调用即执行，日志会出现在调用表达式内部；lazy task 的日志会集中出现在 `sync_wait(std::move(task))` 之后。

## Part 3：返回值与所有权

观察 `sync_wait` 返回值应为 25。解释这条路径：

```text
co_return step3
  -> promise.return_value(step3)
  -> final_suspend 通知等待者
  -> sync_wait 取出值
```

`lazy_task` 是 move-only。`coroutine_handle` 是非拥有句柄；真正拥有协程状态的是 task/runtime 契约。把 task 移进 `sync_wait` 表示消费这一次结果入口。

## 验收

- `auto task = compute_async(5)` 不执行协程体。

  **答案解析：** `compute_async` 的返回类型是 lazy task，promise 的 `initial_suspend()` 会先挂起。调用表达式只创建协程帧和 task 返回对象，协程体里的 `[coro]` 日志要等 task 被消费时才出现。这个现象证明 task 是惰性启动。

- `sync_wait(std::move(task))` 返回 25。

  **答案解析：** 三步计算是 `5 + 10 = 15`，`15 * 2 = 30`，`30 - 5 = 25`。`co_return step3` 把 25 存进 promise，`final_suspend` 通知等待者，`sync_wait` 完成等待后取出这个值。因此返回 25 同时验证了计算值和结果通道。

- 同步版本和 lazy task 版本日志时机不同。

  **答案解析：** `compute_sync(5)` 是普通函数，调用那一刻就执行三步并打印日志。`compute_async(5)` 创建后先不执行，日志集中出现在 `sync_wait(std::move(task))` 启动 root 协程之后。两组日志的差异就是 eager 普通调用和 lazy 协程调用的差异。

- 你能画出 initial suspend、三步计算、final suspend、取值的位置。

  **答案解析：** 示例链可以画成：`compute_async(5)` -> 创建帧和返回对象 -> `initial_suspend` 挂起 -> `sync_wait` 恢复 root -> 执行三步计算 -> `co_return 25` -> `final_suspend` -> `sync_wait` 取值。图里还要标出 task 是协程帧 owner，`sync_wait` 消费这个 owner，handle 只是恢复控制入口。
