# I1 Asio awaitable echo

知识讲解：[I1 对应章节](../../11-模块I-真实异步IO与并发框架.md#i1)。

对应主讲义：`11-模块I-真实异步IO与并发框架.md` 的 I-1。

这题用 Asio 的真实 `awaitable` 写 echo。`co_spawn` 把协程交给 `io_context` executor；`use_awaitable` 把异步操作完成事件变成协程恢复；`as_tuple(use_awaitable)` 把 EOF/cancel 这种常规 I/O 结果作为 `(error_code, value)` 返回。

reference 链路：

```text
main -> co_spawn(listener, use_future) -> co_spawn(client, use_future) -> ctx.run()
listener -> async_accept(bind_cancellation_slot(...)) -> co_spawn(echo_session, use_future)
client -> async_connect -> async_write(payload) -> async_read(reply) -> stop.emit + acceptor.cancel
```

先预测：client 收到原样 echo；listener 的 pending accept 被取消后以 `operation_aborted` 结束；所有 future `get()` 都能观察完成。输出应包含 `loopback echo and accept cancellation observed`。

**答案解析：** client 通过 `async_write` 发出固定 payload，session 用同一个 socket `async_read_some` 后再 `async_write` 原样写回，所以读到的 reply 应等于 payload。client 完成后调用 `stop.emit(all)` 和 `acceptor.cancel()`，listener 的 `async_accept(bind_cancellation_slot(..., as_tuple(use_awaitable)))` 收到 `operation_aborted` 并正常 `co_return`。`use_future` 是后台协程的完成 owner，`get()` 能把异常重新暴露出来，也能证明 listener/client/session 都结束。

运行：

```powershell
cmake -S Coroutine_Study/exercises -B build/coroutine-i1 -DCOROUTINE_STUDY_ENABLE_ASIO=ON -DCOROUTINE_STUDY_FETCH_DEPS=ON -DCOROUTINE_STUDY_BUILD_REFERENCE=ON
cmake --build build/coroutine-i1 --target I1_asio_echo_reference
ctest --test-dir build/coroutine-i1 -R I1_asio_echo_reference --output-on-failure
```

回读代码时按这个顺序看：`main()` 的 future ownership，`listener()` 的 cancellation slot，`echo_session()` 的 `(read_ec, n)` 和 `(write_ec, written)`，最后看 client 为什么能安全关闭 socket 并取消 accept。

**答案解析：** `main()` 保存 listener/client/session future，因此 shutdown 不依赖 detached 后台路径。`listener()` 把 cancellation slot 绑定到 accept 操作，取消后 error_code 作为普通返回值处理；`echo_session()` 用 `as_tuple` 把 EOF/read/write 错误纳入循环出口。client 关闭自己的 socket 后再取消 acceptor，所有协程都有可观察完成点。
