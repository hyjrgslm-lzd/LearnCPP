# 练习 A-3：co_await 一个 std::future

> 详尽版本见 `../../02-模块A-三关键字与最小协程.md` 的 `练习 A-3` 章节。
> 本 README 仅摘抄"目标 / 必做任务 / 验收点"。

## 目标

给 `std::future<T>` 写一个最小 awaiter（`await_ready` / `await_suspend` / `await_resume`），
让 `std::future<T>` 可以被 `co_await` 消费。这是你第一次亲手实现 awaiter 协议。

## 必做任务

1. 写 `future_awaiter<T>`，实现三方法：
   - `await_ready()`：`future.wait_for(0s) == std::future_status::ready`。
   - `await_suspend(h)`：启动新线程做 `future.wait()`，完成后 `h.resume()`。
   - `await_resume()`：`return future.get();`。
2. 通过自由函数 `operator co_await(std::future<T>&&)` 让 `co_await fut` 能找到这个 awaiter。
3. 写一个测试协程 `test_future_await()` 返回 `lazy_task<int>`，内部 `co_await std::async(...)` 然后 `co_return result*2;`。
4. main 中用 `sync_wait` 取结果。
5. 在 `await_suspend` 与协程恢复后的代码各打印线程 ID，确认跨线程 resume。
6. 画一张 awaiter 三方法 + 异步操作 + h.resume 的时序图。

## 进阶任务

- 给 awaiter 加 `std::stop_token` 支持（取消走异常）。
- 直接给 `std::future<T>` 写 `operator co_await`（成员/特化），思考为什么不能改 `<future>` 源码。
- 把 `std::future<T>` 换成 `std::shared_future<T>`，观察 `get()` 的语义差异。

## 验收点

- 日志显示 `await_suspend` 与协程恢复发生在不同线程。
- 你能说清 awaiter 三方法各自被谁、何时调用。
- 你能解释为什么 `await_suspend` 中 `h.resume()` 是合法的（编译器保证此时帧已稳定）。
- 你能画出从 `co_await fut` 到拿到 `result` 的完整数据流图。

## Starter / Reference

- `main.cpp` 是练习骨架，保留 TODO 和最小可编译 awaiter。
- `solution.cpp` 是可运行参考实现，`ctest --preset verify-core -C Release -R A3_co_await_future_reference`
  会校验结果 84、`future.get()` 异常传播，以及 awaiter 三方法的数据流。
