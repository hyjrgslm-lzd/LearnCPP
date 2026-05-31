# I-1 Asio + awaitable 回声服务器

对应文档：`11-模块I-真实异步IO与并发框架.md` 「练习 I-1」。

## 目标

用 Asio 的 `co_spawn` + `awaitable<void>` 搭建完整可运行的回声服务器：
- 处理多客户端并发连接（单线程 `io_context`）。
- 用 `cancellation_signal` 实现 Enter 优雅关停。
- 用 `asio::as_tuple(use_awaitable)` 让 `(ec, n)` 取代 try/catch。
- 用 `bind_cancellation_slot(slot, asio::as_tuple(use_awaitable))` 包装整个
  completion token，把 cancel 信号注入 async 操作。

## 必做任务

1. 编译并运行 `I1_asio_echo`（默认端口 12345，可命令行覆盖）。
2. 用 `nc localhost 12345` 或 `telnet localhost 12345` 起 1-3 个客户端，验证回声。
3. 在终端按 Enter——所有连接优雅关闭，`io_context::run()` 退出。
4. 在笔记中画 `io_context::run()` 的单线程多协程调度循环。

## 内置 cancellation_slot 注解

Asio awaitable 协程已有内置 cancellation_slot（由 `co_spawn` 注入并随 `co_await`
传播）；生产代码模式是在 `co_spawn` 处一次绑定：

```cpp
co_spawn(ctx, listener(acceptor),
         asio::bind_cancellation_slot(cancel_signal.slot(), detached));
```

骨架中"在 `async_accept` 上手动 bind slot"是为了对照演示——两种模式都成立。

## 进阶任务

- 把单线程 `io_context` 换成 `asio::thread_pool`，找出哪里需要锁。
- 用 `parallel_group::wait_for_one` + `steady_timer` 实现 30s 空闲超时关闭连接。
- 在 `echo_session` 也注入 cancellation_slot，实现连级取消。

## 验收点

- 多客户端并发回声正确。
- Enter 关停后 listener 与所有 session 都退出。
- 你能解释 `as_tuple(use_awaitable)` vs 裸 `use_awaitable` 的差异。
- 你能说清单线程 `io_context` 模型下为什么协程内变量无需加锁。

## 提示

- Windows 需要 `target_link_libraries(... ws2_32 mswsock)`，CMakeLists 已加。
- 如果 `<asio.hpp>` 找不到，确认 Agent 1 的 `coroutine_study_link_stage3_deps` 已注入
  Asio 的 include 路径。
