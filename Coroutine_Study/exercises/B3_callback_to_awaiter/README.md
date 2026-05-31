# 练习 B-3：把回调 API 包成 awaiter

> 详尽版本见 `../../03-模块B-generator与task的使用.md` 的 `练习 B-3` 章节。
> 本 README 仅摘抄"目标 / 必做任务 / 验收点"。

## 目标

给定一个典型的老式回调异步 API（`void async_op(Args..., Callback)`），写出一个最小
awaiter，让它可以被 `co_await` 消费。这是协程工程化的"第一道坎"——把任何回调世界里的
异步操作，拉进协程世界的 `co_await`。

## 必做任务

1. 模拟回调式异步 API：`async_add(a, b, cb)` 在新线程 sleep 100ms 后回调 `cb(a+b)`。
2. 写 `AsyncAddAwaiter`：
   - `await_ready()` 返回 `false`。
   - `await_suspend(h)` 调用 `async_add(a, b, [this, h](int r){ result_ = r; h.resume(); });`
     **注意：先写 `result_`，后 `h.resume()`**——`resume` 充当内存屏障。
   - `await_resume()` 返回 `result_`。
3. 写协程 `compute_with_callback(x, y) -> lazy_task<int>`，`int sum = co_await AsyncAddAwaiter{x,y}; co_return sum*3;`。
4. main 中 `sync_wait` 取走结果。
5. 在 `await_suspend` / 回调 lambda / `await_resume` / 协程体内分别打印线程 ID，验证跨线程 resume。
6. 画 awaiter 三方法 + async_add + 回调 + resume 的时序图。

## 进阶任务

- 给 awaiter 加错误路径（回调里 20% 概率给错误码 -> 转 `exception_ptr` -> `await_resume` 重抛）。
- 通用 `callback_awaiter<AsyncFunc, Args...>` 模板。
- 替换为真实异步 API（Asio 的 `async_read`、Win32 `ReadFileEx`）。
- 加 `std::stop_token` 支持，注册 `stop_callback` 取消底层操作。

## 验收点

- 日志显示 `await_suspend` 在主线程，回调 lambda / `await_resume` / 协程恢复后的代码在异步线程。
- 你能解释为什么 `await_ready` 返回 `false`、为什么必须"先写 result 后 resume"。
- 你能画出完整的 awaiter + async_add + callback + resume 时序图。
- 你能说出真实库（Asio `awaitable<T>`、Folly `Future::to_task`）为什么要把这个模式封装起来。
