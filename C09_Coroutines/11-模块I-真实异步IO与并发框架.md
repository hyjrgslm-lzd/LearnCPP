# 11 模块 I：真实异步 IO 与并发框架

模块 I 把前面手写过的 `promise_type`、awaiter、调度器和结构化生命周期放进真实库。目标是从“用户协程提交一次等待”一路追到“OS 或框架 completion 恢复协程”，再回到业务层看结果、错误、取消和资源收束。

协程适合 I/O 的原因很具体：I/O 等待期间线程可以去处理别的 completion，协程帧保存局部状态，completion 到来后恢复到 `co_await` 后一行。CPU 密集工作仍要线程池、SIMD 或任务系统；协程只让等待可组合。

真实异步 I/O 的共同形状：

```text
user coroutine
  co_await read/write/timer/channel
    await_suspend(handle)
      submit request
      store identity token
      return to event loop

event loop / framework / kernel completion
  take completion
  restore awaiter or operation state
  store result/error/cancel state
  resume coroutine

await_resume()
  return value, error_code, exception, or stopped result
```

上游入口：

- Asio C++20 coroutine support：<https://www.boost.org/latest/doc/html/boost_asio/overview/composition/cpp20_coroutines.html>
- liburing manual：<https://www.man7.org/linux/man-pages/man3/io_uring_wait_cqe.3.html>
- Windows IOCP overview：<https://learn.microsoft.com/en-us/windows/win32/fileio/i-o-completion-ports>
- `GetQueuedCompletionStatus`：<https://learn.microsoft.com/en-us/windows/win32/api/ioapiset/nf-ioapiset-getqueuedcompletionstatus>
- Folly `Task.h`：<https://github.com/facebook/folly/blob/main/folly/coro/Task.h>
- Folly `SafeTask.h`：<https://github.com/facebook/folly/blob/main/folly/coro/safe/SafeTask.h>
- Boost.Cobalt 1.92：<https://www.boost.org/library/latest/cobalt/>
- cppcoro：<https://github.com/lewissbaker/cppcoro>

<a id="i1"></a>

## I-1：Asio awaitable echo

Asio 的核心抽象是 executor 和 completion token。`co_spawn(ctx, task, token)` 把一个 `awaitable` 协程放进 `io_context` 的 executor。`use_awaitable` 告诉 Asio：异步操作完成时恢复当前协程。`asio::as_tuple(use_awaitable)` 把错误也作为返回值带回来，适合 EOF、cancel 这种常规路径。

I1 reference 的控制流：

```text
main
  -> io_context
  -> acceptor binds port 0
  -> co_spawn listener(..., use_future)
  -> co_spawn client(..., use_future)
  -> ctx.run()

listener
  -> async_accept(bind_cancellation_slot(stop.slot(), as_tuple(use_awaitable)))
  -> co_spawn echo_session(socket, use_future)

client
  -> async_connect
  -> async_write(payload)
  -> async_read(reply)
  -> stop.emit(all)
  -> acceptor.cancel()
```

先预测：client 写入 `"coroutine-asio-echo"`，session 读到后原样写回，client 校验通过；之后 client 发 `cancellation_signal` 并 cancel acceptor，listener 的 pending `async_accept` 以 `operation_aborted` 返回，listener 自然 `co_return`。`use_future` 持有 listener/client/session 的完成结果，所以异常不会沉默。

关键 API 读法：

```cpp
auto [ec, socket] = co_await acceptor.async_accept(
    asio::bind_cancellation_slot(stop.slot(), asio::as_tuple(asio::use_awaitable)));

if (ec == asio::error::operation_aborted) {
    co_return; // 外部取消 accept，是正常停止路径。
}
```

`bind_cancellation_slot` 把外部 `cancellation_signal` 接到这一次 accept 操作。`as_tuple` 让取消变成 `ec`，于是 listener 可以自然退出。session 里的 read/write 也用 `as_tuple`，因为 EOF 是网络连接的正常完成方式。

重要观察：单线程 `io_context::run()` 下多个 session 是并发等待，执行上仍由一个线程逐个处理 handler。共享状态在同一个 event loop 上顺序访问通常不需要锁；换成多线程 run 以后要用 strand、锁或消息化设计。

<a id="i2"></a>

## I-2：io_uring / IOCP awaiter

I2 是模块 I 的“最小内核”。两个平台 API 不同，但 awaiter 模型相同：`await_suspend` 提交 OS 请求，并把“我是谁”塞进 OS 能在 completion 时还回来的位置。

Linux io_uring reference：

```text
read_awaiter.await_suspend
  -> io_uring_get_sqe
  -> io_uring_prep_read
  -> io_uring_sqe_set_data(sqe, this)
  -> io_uring_submit
  -> pending++

uring_loop.run
  -> io_uring_wait_cqe_timeout
  -> io_uring_cqe_get_data(cqe)
  -> base->result = cqe->res
  -> io_uring_cqe_seen
  -> pending--
  -> resume()
```

`io_uring_cqe_seen()` 要在 resume 前调用。恢复后的协程可能立刻提交下一次请求；如果旧 CQE 未消费，event loop 状态会变得难推理。

对应代码形状：

```cpp
bool await_suspend(std::coroutine_handle<> h) noexcept {
    continuation = h;
    auto* sqe = io_uring_get_sqe(&loop.ring);
    io_uring_prep_read(sqe, fd, data, size, offset);
    io_uring_sqe_set_data(sqe, static_cast<awaiter_base*>(this));
    ++loop.pending;
    return true;
}
```

`user_data` 里放的是 awaiter 身份。业务结果来自 CQE 的 `res` 字段，event loop 写入 awaiter 后，协程才从 `await_resume` 读到字节数。

I2 的四个 Part 要分清：

- read/recv：`await_suspend` 保存 continuation、提交一次真实 OS 请求，并只在请求已经 pending 后让协程挂起。
- complete：event loop 有有限 timeout，取 completion 后先复制 `res`、byte count 或 error，再消费 CQE/完成包。
- resume：只有 complete 阶段写完结果后才能恢复 coroutine；危险回调不能在还没建立 buffer/awaiter 生命周期时盲目 `resume()`。
- close：fd/socket、ring/port 和临时 buffer 都由外层 owner 活到 completion 之后；恢复后的协程可能立刻继续提交或退出，所以 close/drain 要发生在 pending 归零后。

Windows IOCP reference：

```text
recv_awaiter.await_suspend
  -> WSARecv(socket, ..., &ov, nullptr)
  -> pending++

iocp_loop.run
  -> GetQueuedCompletionStatus(port, ..., &ov, 5000)
  -> OVERLAPPED* 还原 awaiter_base*
  -> 写 transferred/error
  -> pending--
  -> resume()
```

`OVERLAPPED` 是 awaiter 的第一个成员，reference 用 `static_assert(offsetof(..., ov) == 0)` 固定还原前提。Microsoft 文档说明 `GetQueuedCompletionStatus` 成功时会返回完成包的信息；失败但 `lpOverlapped` 非空时，仍表示取到了一个失败的 I/O completion，需要用 `GetLastError()` 读取错误。reference 因此把超时 `ov == nullptr` 和失败 completion `ov != nullptr` 分开处理。

Windows 版的“身份”来自 `OVERLAPPED*`：

```cpp
struct awaiter_base {
    OVERLAPPED ov{};
    std::coroutine_handle<> continuation{};
    DWORD transferred = 0;
    DWORD error = 0;
};

auto* base = reinterpret_cast<awaiter_base*>(ov);
base->continuation.resume();
```

这个技巧要求 `ov` 位于对象首地址。生产代码可以用 `CONTAINING_RECORD` 风格恢复外层对象，但前提仍然是 layout 由你自己定义并保持稳定。

I2 的预期观察：Linux 输出 CQE `user_data` 恢复协程；Windows 输出 IOCP loopback bytes 恢复协程。两者证明“completion -> awaiter -> resume”链路；生产级 I/O 框架还需要批量提交、错误分类、取消、资源池和并发策略。

<a id="i3"></a>

## I-3：Folly safe coroutine

Folly 的价值在生命周期边界。`folly::coro::Task<T>` 是 lazy task，创建后尚未绑定 executor；await 或 `co_withExecutor` 后才启动。Folly `Task.h` 的注释说明 child task 会继承 awaiting task 的 executor，普通 `co_await expr` 会经 executor 相关适配保证恢复语义。

Safe coroutine 层把常见悬空引用问题变成类型约束。`SafeTask.h` 中的几个别名各自对应不同场景：

- `value_task<T>`：参数和返回都是值语义，适合普通安全协程。
- `now_task<T>`：需要在创建它的 full-expression 内立即 await，减少“保存后再跑”导致的引用悬空。
- `closure_task<T>`：配合 `async_closure` 使用，处理闭包捕获和不那么结构化的启动场景。
- `co_cleanup_safe_task<T>`：用于 cleanup/scope 期间可能运行的协程工作。

I3 reference 只保留真实 Folly API 子集：`value_task<int>`、`now_task<int>`、executor-bound `Task`、`async_closure(bind::args{...}, ...)`、`co_cleanup_safe_task<int>`。预测结果分别是 42、7、worker thread 与 main 不同、closure 返回 6、cleanup 返回 9。本题在 Linux `heavy-folly-linux` 预设中启用；Windows 学习时沿 reference 源码阅读 API 和生命周期设计。

本机若没有 Folly，I3 仍保留完整 starter/reference 和阅读入口，但验证状态只能写“未验证/依赖缺失”。不能用空程序输出 0 代表通过；启用 starter 检查时，TODO 版本会调用 `value_task_todo`、`now_task_todo` 和 `task_todo`，初态应有限失败。

reference 中最值得读的片段：

```cpp
folly::coro::Task<std::thread::id> executor_bound_task() {
    co_await folly::coro::co_current_executor;
    co_return std::this_thread::get_id();
}

auto worker = folly::coro::blocking_wait(
    folly::coro::co_withExecutor(&executor, executor_bound_task()));
```

`co_withExecutor` 把 task 绑定到 executor，`co_current_executor` 用来确认当前协程确实运行在这个 executor 上。safe task 别名解决的是参数和捕获的生命周期问题；executor 绑定解决的是恢复位置问题，它们是两条独立边界。

`blocking_wait` 只适合 main/test bridge。把它放进真实 executor 线程里等待同一个 executor 的工作，很容易把唯一能运行 completion 的线程堵住。

<a id="i4"></a>

## I-4：Boost.Cobalt channel / gather / race

Cobalt 用 `promise`、`task`、`channel`、`gather`、`race` 提供协程原生的通信和组合。`channel<T>` 是线程局部协程通道：缓冲为空时读者挂起；缓冲满时写者挂起；读写双方可直接交接控制，能清楚观察 symmetric transfer 的工程价值。

I4 reference 在 Linux `heavy-cobalt-linux` 预设中启用；Windows 学习时沿 reference 源码阅读 channel/gather/race 的控制流。reference：

```text
cobalt::main co_main
  -> channel<int>{0}
  -> consumer(ch) 先创建
  -> gather(producer(ch), delay(1ms))
  -> producer 写 1,2,3
  -> consumer 读固定 3 次
  -> race(delay(30ms), delay(1ms))
```

先预测：zero-buffer channel 让 producer/consumer 交替推进，consumer 最终拿到 `{1,2,3}`。`race` 中 1ms 分支先完成，reference 期望 winner index 为 1。Cobalt 文档说明 `race` 对 void awaitable 返回 index；loser cancellation 规则按所用 Boost 版本文档确认。

`channel` 的最小使用形状：

```cpp
cobalt::promise<void> producer(cobalt::channel<int>& ch) {
    co_await ch.write(1);
    co_await ch.write(2);
}

cobalt::promise<std::vector<int>> consumer(cobalt::channel<int>& ch) {
    std::vector<int> values;
    values.push_back(co_await ch.read());
    values.push_back(co_await ch.read());
    co_return values;
}
```

写者和读者都通过 `co_await` 让出 executor。零缓冲通道因此能表达背压：消费者没准备好时，生产者挂起协程并释放执行权。

背压是 channel 的正常语义。channel 满时 producer 挂起并释放 executor；真正危险的是同步阻塞或 busy loop 占住唯一线程，让 consumer 没机会运行。

本机若没有 Boost.Cobalt，I4 同样只登记未验证，不改写成“已通过”。starter 检查会实际进入 `gather(producer(ch), consumer(ch))`；TODO 版本没有 channel 操作，只能有限失败。完成 producer/consumer 后，再增加 race/timeout 观察。

<a id="i5"></a>

## I-5：cppcoro patterns

cppcoro 是很好的对照物。它把你在 D-G 写过的机制包装成稳定 API：

- `task<T>`：lazy、单消费者结果。
- `shared_task<T>`：结果可被多个 awaiter 共享。
- `generator<T>`：同步 pull 序列。
- `sync_wait`：从普通 `main()` 启动顶层 awaitable。
- `when_all`：结构化 fan-out/fan-in。
- `static_thread_pool::schedule()`：明确线程池切换点。
- `cancellation_source/token/registration`：显式取消和回调注册。

I5 reference 先用 `generator` 算两个 range 的和，再用 `when_all` 汇合；随后两次 await 同一个 `shared_task`；再 `co_await pool.schedule()` 观察线程切换；最后 request cancellation 并通过 `cancellation_registration` 观察 callback。预期总和是 21。

对照代码：

```cpp
auto [a, b] = co_await cppcoro::when_all(sum_range(1, 4), sum_range(4, 7));

auto shared = cached_answer();
co_await shared;
co_await shared;

co_await pool.schedule();
```

`when_all` 让两个 task 在同一个组合点汇合；`shared_task` 说明结果可以缓存给多个 awaiter；`schedule()` 是显式调度切换点。每个 API 都能对应到你之前手写过的一个机制。

cppcoro 文档中 `sync_wait` 的定位很清楚：它在当前线程创建顶层协程并阻塞等待结果，适合从 `main()` 进入 async 世界。做完 I5 后，你应该能把本仓库的 `lazy_task/sync_wait/when_all/shared_task` 和 cppcoro 的对应类型逐个对上。

I1/I2/I5 的 starter 检查也消费真实实现：I1 做 250ms 有界 loopback echo；I2 要求 awaiter 不是 ready 快路径，后续 reference 再做真实 CQE/IOCP completion；I5 调用 generator/task/shared_task/cancel/scheduler TODO，初态观测值为 0 会被拒绝。观察型实验只声明它实际跑到的行为，不外推到生产库完整正确性。

## 模块 I 的闭环问题

完成 I1-I5 后，至少能回答这些问题：

- 一次 `co_await async_read` 的 handle 保存在哪里，completion 如何找到它？
  **答案解析：** 保存位置由框架决定：Asio 把 awaitable frame 和 handler/executor 绑定起来；io_uring 示例把 awaiter 指针放进 SQE `user_data`，CQE 再取回；IOCP 示例把 `OVERLAPPED` 嵌在 awaiter 首地址，completion 返回 `OVERLAPPED*` 后还原 awaiter。共同点是 completion 先找回 operation/awaiter 身份，写入结果，再恢复之前保存的 coroutine handle。
- 错误是通过 `error_code`、异常、`std::expected` 还是 stopped channel 传播？
  **答案解析：** I1 的 `as_tuple(use_awaitable)` 把 EOF/cancel 变成 `error_code` 返回，裸 `use_awaitable` 更常把错误抛成异常；Capstone4 RPC reference 对协议、超时、断连使用 `std::expected<..., error>`；H/stdexec 路径保留 value/error/stopped 三通道。读真实库时先看 completion token 或 sender completion signatures，那里决定错误进入哪条业务边。
- 取消是 cancel 当前 OS request、请求 stop token、发送 cancel frame，还是只移除本地 pending？
  **答案解析：** 四种都可能出现，语义边界不同。Asio accept 示例用 cancellation slot 和 `acceptor.cancel()` 取消当前操作；cppcoro/Folly/stdexec 通过 token 或 environment 表达停止请求；RPC 超时先移除本地 pending，再发送 `C|id` 让 server handler 在检查点停下。只移除 pending 能唤醒本地调用方，但远端工作是否停止要看协议或框架是否收到取消信号。
- 哪个对象拥有后台协程，shutdown 时如何证明它已经完成？
  **答案解析：** I1 用 `use_future` 保存 listener/client/session 的完成结果；RPC reference 用 `in_flight_` 覆盖 accept、connection、request、writer、client read loop，并在 shutdown 后断言归零；Folly/Cobalt/cppcoro 各自用 scope、task 或组合器表达拥有关系。证明完成要看 owner 的 join/future/scope/drain，而不能只看 socket close 或函数返回。
- scheduler affinity 保证恢复位置；它不自动提供数据竞争安全。
  **答案解析：** executor/scheduler 决定 continuation 被投递到哪里运行，比如 Asio 单线程 `io_context` 或 Folly `co_withExecutor`。数据竞争安全还要由 strand、锁、消息队列、线程局部约束或不可变数据保证；一旦多个线程同时 run 同一个上下文，原来依赖单线程顺序访问的共享状态就要重新审查。
