# 练习 C-1：协作式取消

## 目标

让一个 `lazy_task` 接收 `std::stop_token`，在每个 `co_await` 点主动检查取消请求；
建立"取消是协作式的、发生在 co_await 点、stopped 不等于 error"的直觉。

## 必做任务

1. 给 `lazy_task<T>` 的 promise（或通过参数）注入 `stop_token`，
   在每次 `co_await` 之前/之后检查 `stop_token.stop_requested()`。
2. 写一个分批处理协程 `batch_process(token, N)`，每批 `co_await` 一段延迟，
   被取消时 `co_return` 已完成的批次数（走 stopped 路径）。
3. 在 `main` 中创建 `std::stop_source`，启动 task，
   稍后 `request_stop()`，观察协程在下一个 `co_await` 点提前返回。
4. 再写一个把取消映射成异常的对比版本，记录两种 API 的差异。

## 验收点

- 你能亲手触发取消，观察到协程在 `co_await` 点提前退出。
- 你能区分 stopped（有意终止）与 error（故障）。
- 你能解释为什么 `co_await` 点是天然的 cancellation point。
- 你能解释纯 CPU 循环不检查 token 时取消请求被完全忽略的原因。

## 提示

- `async_sleep` 已给出最小实现；真实工程应交给定时器线程驱动。
- `batch_process` 内部不要 `throw` 取消——`co_return processed` 让 stopped 路径自然成立。
- 进阶部分关注 `stop_callback`、stop_token 沿调用链向下传播。
