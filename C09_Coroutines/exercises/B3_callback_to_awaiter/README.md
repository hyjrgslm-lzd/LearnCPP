# 练习 B-3：把回调 API 包成 awaiter

先读 [模块 B 的回调 awaiter 章节](../../03-模块B-generator与task的使用.md#b3)。本题把一个 `async_add(a, b, callback)` 形状的 API 接到 `co_await`。

## Part 1：回调 API

打开 [main.cpp](main.cpp)，先确认模拟 API 的形状：

```cpp
async_add(workers, a, b, [](int value) {
    // 完成后被调用
});
```

`worker_group` 是后台线程 owner，负责保存 `std::jthread` 并在结束前 join。`async_add(workers, a, b, cb)` 把等待和回调提交到这个 owner 中；没有 owner 的后台线程不能安全持有 awaiter 指针或协程 handle。

## Part 2：awaiter 三方法

实现 `async_add_awaiter`：

- `await_ready()` 返回 false，表示这次操作默认异步完成。
- `await_suspend(h)` 调用 `async_add(workers_, a_, b_, ...)`，把当前协程的非拥有 handle 放进回调。
- 回调里先保存结果，再 `h.resume()`。
- `await_resume()` 返回保存的结果。

核心时序：

```text
协程线程 await_suspend
  -> async_add 提交 worker
worker 线程执行回调
  -> 写 result
  -> h.resume()
协程恢复
  -> await_resume 读 result
```

## Part 3：同步完成窗口

真实 API 可能在 `async_add` 返回前就调用回调。本题模拟的是延迟完成，所以 `await_ready()` 可以固定 false。复盘时仍要写清：生产代码通常要把已完成情况放进 ready 快路径，或让 `await_suspend` 返回 false，避免在 `await_suspend` 内重入恢复同一个协程。

starter 里单独保留 `async_add_immediate` 作为同步立即完成观察点。主线 awaiter 使用后台 `async_add`，同步完成窗口在这一 part 单独分析。

**答案解析：** 如果底层 API 可能同步调用回调，awaiter 不能简单固定 `await_ready()==false` 后在 `await_suspend` 里让回调重入恢复同一个协程。安全形状通常是：已完成结果让 `await_ready()` 返回 true，或让 `await_suspend()` 返回 false 表示当前调用链继续执行。B3 主线的 `async_add` 总是后台完成，所以固定 false 可用于教学；`async_add_immediate` 只用来说明生产代码要额外处理这个窗口。

## Part 4：结果同步与异常

参考实现 [solution.cpp](solution.cpp) 用 mutex 保存 result 和线程 ID。最小练习中，回调写 result 与 `h.resume()` 在同一 worker 线程中顺序发生；更通用的跨线程完成要用 mutex、atomic、事件队列或底层 API 的同步保证。

进阶可以把错误码转换成 `std::exception_ptr`，在 `await_resume()` 中重新抛出，让错误出现在协程体的 `co_await` 行。

## 验收

- `compute_with_callback(7, 5)` 最终返回 36。

  **答案解析：** `async_add` 的回调先算出 `7 + 5 = 12`，awaiter 保存 result 后恢复协程。`compute_with_callback` 从 `co_await` 得到 12，再继续执行后续计算，reference 最终检查 36。这个值证明回调结果确实通过 `await_resume()` 回到了协程体。

- 日志显示 `await_suspend` 与回调/恢复可能在不同线程。

  **答案解析：** `await_suspend` 在启动或等待 task 的线程里运行，随后 `async_add` 把工作提交给 `worker_group`。回调在 worker 线程执行，并在同一线程调用 `h.resume()`，所以恢复后的协程代码也可能接着跑在 worker 线程上。日志中的线程 ID 差异就是恢复位置由调用 `resume()` 的代码决定。

- 回调先写结果，后恢复协程。

  **答案解析：** 恢复协程后，下一步就是执行 `await_resume()` 读取 result。若先 `h.resume()` 再写 result，协程可能读到旧值或未初始化值。B3 的安全顺序是 worker 回调内先完成结果写入，再恢复 handle。

- 你能指出 awaiter 位于协程帧中，回调保存的 handle 是非拥有句柄。

  **答案解析：** `co_await async_add_awaiter{...}` 的 awaiter 需要跨挂起点存活，因此对象存放在当前协程帧里。回调捕获的 `h` 只是指向该帧的非拥有控制句柄，不能负责销毁帧。帧 owner 来自 task/`sync_wait` 契约；后台线程 owner 来自外层 `worker_group`。当前回调写 result、调用 `h.resume()` 后不再访问 awaiter，线程退出再由 `worker_group.join()` 收束。

## Student 检查

`main.cpp` 会运行 `compute_with_callback(workers, 7, 5)` 并检查结果：

| Part | 操作 | 本地检查 |
| --- | --- | --- |
| Part 1 | `async_add` 后台完成并调用 callback | 被 awaiter 的 `await_suspend` 调用 |
| Part 2 | awaiter 先写 result、后 resume | `co_await` 后继续计算得到 36 |
| Part 3 | 同步完成窗口 | 保持分析题；生产形状由答案解析说明 |
| Part 4 | 结果同步与异常 | 本地检查值流；异常扩展仍属进阶 |

完成前：result 没有通过 `await_resume()` 回到协程体时，最终值不会是 36。当前 Student PASS 只证明 Part 1/2/4 的值流和 owner 收束；Part 3 同步立即完成窗口仍是观察/解析题，没有把生产级 ready/return-false 处理计入实现 PASS。
