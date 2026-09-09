# 练习 A-3：`co_await` 一个 `std::future`

先读 [模块 A 的 co_await 章节](../../02-模块A-三关键字与最小协程.md#a3)。future 的基础来自 [00 的 future 模型](../../00-预备知识-执行模型与标准库.md#future-model) 和 [async 执行策略](../../00-预备知识-执行模型与标准库.md#async-launch)。

本题给 `std::future<T>` 写用户命名空间包装：`co_await await_future(std::move(fut))`。包装入口放在自己的命名空间里，调用点显式表达“把这个 future 转成 awaitable”。

## Part 1：三方法

打开 [main.cpp](main.cpp)，实现 `future_awaiter<T>`：

- `await_ready()` 用 `fut_.wait_for(0s) == std::future_status::ready` 判断快路径。
- `await_suspend(h)` 把等待工作交给明确 owner 管理的 worker，future 完成后调用 `h.resume()`。
- `await_resume()` 调用 `future.get()` 或读取 worker 保存的结果；异常应在这里回到协程体。

`await_ready` 返回 true 时，`await_suspend` 不会被调用。`await_suspend` 被调用时，当前协程已经稳定暂停，参数 `h` 是非拥有 handle。

## Part 2：包装入口

保留用户命名空间包装：

```cpp
auto fut = std::async(std::launch::async, [] { return 42; });
int result = co_await await_future(std::move(fut));
```

这让调用点清楚表达“把 future 适配为 awaitable”。`std::future` 的 ready/valid/get 语义仍是标准 future 语义。

进阶时可以把函数返回的对象改成 `future_awaitable<T>`，再让这个包装类提供 `operator co_await()`。扩展点属于你的包装类型，不属于 `std::future`。

## Part 3：三条路径

参考实现 [solution.cpp](solution.cpp) 覆盖三条路径：

- `std::async(std::launch::async, ...)` 未 ready，进入 `await_suspend`，worker 等待结果并恢复协程。
- `std::promise` 先 `set_value`，future 已 ready，跳过 `await_suspend`。
- future 中保存异常，`await_resume` 重新抛出，最终由 `sync_wait` 传给 main。

未 ready 路径用启动信号保证确定性：producer 先等待 `waiting_started`，awaiter 进入 `await_suspend` 后设置该信号，再由 worker 等待 future 并恢复协程。reference 的 `await_future(fut, workers, state)` 多出的 `workers` 是线程 owner，`state` 是观测和结果状态；starter 的变化点是先完成 `await_future(std::move(fut))` 这个用户包装入口。

画时序图时，分别标出挂起线程、worker 线程、结果保存位置、恢复位置。

**答案解析：** 示例链可以画成：协程线程执行到 `co_await await_future(...)`，`await_ready()` 返回 false，`await_suspend(h)` 记录挂起线程并把 future/handle 交给 `worker_group`。worker 线程等待 future，保存 value 或 exception，再调用 `h.resume()`；协程恢复后执行 `await_resume()` 读取保存结果。ready future 的图要单独标成 `await_ready()==true -> await_resume()`，没有 worker 分支。

## 验收

- 正常路径返回 84。

  **答案解析：** reference 的正常路径让 future 产生 42，协程体 `co_await` 拿到值后再做一次计算得到 84。未 ready 时，worker 等 future 完成并保存值，再恢复协程；`await_resume()` 返回 42，后续乘 2 得到最终结果。这个结果验证了跨线程等待后值没有丢。

- ready future 直接进入 `await_resume`。

  **答案解析：** `std::promise` 先 `set_value(21)` 后再取 future 进入 awaiter，`fut_.wait_for(0s)` 返回 ready。`await_ready()` 为 true 时协程不挂起，`await_suspend()` 不会被调用，也不会启动 worker。结果直接由 `await_resume()` 的 `get()` 取出。

- 未 ready future 显示跨线程恢复。

  **答案解析：** reference 用 `waiting_started` 信号保证 awaiter 已进入未 ready 路径，再让 producer 设置 future 的值。`await_suspend(h)` 把等待交给 `worker_group`，worker 在线程里等到 future ready 后调用 `h.resume()`。因此挂起发生在启动协程的线程，恢复和 `await_resume()` 之后的代码会记录为 worker 线程。

- 异常能从 `future.get()` 经 `co_await` 传播到调用侧。

  **答案解析：** future 共享状态里保存异常时，`future.get()` 会重新抛出。awaiter 把这个异常带到 `await_resume()`，协程体在 `co_await await_future(...)` 这一行看到异常；若父协程不捕获，lazy task 的 promise 保存它，最后 `sync_wait` 再抛给 main。异常路径没有单独绕开协程结果通道。
