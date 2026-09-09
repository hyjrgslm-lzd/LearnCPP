# 练习 C-1：协作式取消

先读 [模块 C 的 stop_token 章节](../../04-模块C-取消与组合.md#c1)。本题只用显式函数参数传 `std::stop_token`，不改共享 `lazy_task` promise。

## Part 1：stop state

main 创建 `std::stop_source src`，把 `src.get_token()` 传给协程。另一个 `std::jthread` 延迟调用 `src.request_stop()`。

```text
stop_source request_stop()
  -> stop state 标记为 requested
  -> stop_token.stop_requested() 开始返回 true
```

请求停止不会强行杀掉协程。协程必须自己检查 token。

## Part 2：检查点

打开 [main.cpp](main.cpp)，在 `batch_process` 的 `co_await async_sleep{50ms}` 前后检查。starter 已把两个检查点写在对应位置，保留 TODO 方便你解释每一处负责的窗口：

```cpp
if (st.stop_requested()) co_return processed;
co_await async_sleep{50ms};
if (st.stop_requested()) co_return processed;
```

运行后观察已处理批次数应大于 0 且小于总批数。取消请求到达后，协程会在下一个检查点提前返回。

## Part 3：stopped 与异常对照

补 `batch_process_throwing`，同样在 `co_await` 前后检查 token，收到取消后 `throw task_cancelled{}`。对照两种调用侧：

- 返回已处理数量：普通值路径。
- 抛 `task_cancelled`：调用侧必须 `try/catch`。

这用于理解 stopped 和 error 的差异。实际库会把取消设计成独立完成通道；本题先用两种写法观察 API 形状。

## 验收

- `request_stop()` 后，协程在检查点响应。

  **答案解析：** `request_stop()` 设置共享 stop state，`batch_process` 在 `co_await async_sleep{50ms}` 前后检查 token，并在观察到请求时返回。延迟触发取消的示例可以得到部分已完成批次，具体数量取决于取消与执行的时序；启动前就请求停止可以得到 0，全部工作完成后再请求停止则保留全部结果。协作式响应的依据是源码中的显式检查点和对应返回路径。

- 返回版本能报告已完成批次数。

  **答案解析：** 返回版本收到取消后 `co_return processed`，把已经完成的批次数作为普通 value 交给 promise。`sync_wait` 取到的是一个正常结果，调用侧可以继续打印或统计已处理进度。这个路径适合“取消后仍要保留部分成果”的接口。

- 异常版本能被 `sync_wait` 重新抛给 main。

  **答案解析：** 异常版本在检查到 token 后抛 `task_cancelled{}`，该异常进入 task 的 `promise.unhandled_exception()`。最外层 `sync_wait` 消费 task 时会重新抛出，main 必须用 `try/catch` 处理。这个对照展示了把取消建模成 error 时调用侧会承担异常控制流。

- 你能说明 CPU 长循环如果不检查 token，就不会响应取消。

  **答案解析：** stop token 只是可查询的共享状态，CPU 长循环如果一直做计算、不 `co_await`、也不读取 `stop_requested()`，就没有机会观察取消请求。线程会继续跑到循环自然结束。要让取消及时生效，必须在循环边界或固定批次后加检查点。
