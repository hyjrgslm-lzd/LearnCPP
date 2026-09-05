# 11 模块 I：真实异步 IO 与并发框架

模块 I 的目标不是再学一种协程语法，而是把前面写过的 `promise_type`、awaiter、调度器和结构化生命周期放进真实库里观察。学完后要能从 OS completion 一直讲到业务组件：为什么协程适合 IO、如何被实现、什么时候该用框架、什么时候不该用。

本模块对应练习：

- I1：Asio `awaitable` echo。看 `co_spawn` 如何把协程帧交给 `io_context`。
- I2：Linux io_uring / Windows IOCP awaiter。看 kernel completion 如何找到 awaiter 并恢复协程。
- I3：Folly `Task` / `safe_task`。看大型工程如何把生命周期安全变成类型约束。
- I4：Boost.Cobalt `channel` / `race` / `gather`。看协程间通信、背压和 symmetric transfer。
- I5：cppcoro patterns。把手写 runtime 和库级抽象逐一对照。

## 1. 协程在现代 C++ 项目里的位置

协程最擅长的是“等待很多次，但每次等待期间线程不该被占住”的代码：

- 网络服务器：accept/read/write、RPC、HTTP、WebSocket、网关。
- 存储与文件 IO：io_uring、Windows overlapped IO、异步日志和异步刷盘。
- 游戏/客户端任务流：资源加载、脚本桥接、超时、取消、多个任务汇合。
- 基础设施库：scheduler、task、async scope、channel、timer、cancellation。

协程不擅长替代 CPU 并行。CPU 密集计算仍然要线程池、SIMD、任务调度和负载均衡。协程只是让“等待”可组合，不让计算自动变快。

从底层看，C++20 协程只规定语言变换：

1. 编译器把协程函数变成协程帧。
2. `promise_type` 决定返回对象、初始挂起、最终挂起、异常和返回值。
3. `co_await expr` 被变换为 awaiter 三方法协议。
4. `await_suspend` 决定当前协程是否挂起，以及之后由谁恢复。

标准没有规定事件循环、线程池、IOCP、epoll、io_uring、超时或取消。这些都由库完成。模块 I 学的就是“库如何接住语言层协议”。

## 2. 从 OS completion 到 coroutine resume

真实异步 IO 的共同结构很稳定：

```text
user coroutine
  co_await read_awaiter
    await_suspend(handle)
      submit OS async request
      store identity: awaiter* / coroutine_handle / OVERLAPPED*
      return to event loop

event loop
  wait for kernel completion
  restore identity
  store result/error into awaiter state
  mark OS completion consumed
  resume coroutine

awaiter.await_resume()
  return bytes or throw/return error_code
```

I2 的两个平台只是身份 token 不同：

- io_uring：`io_uring_sqe_set_data(sqe, this)`，CQE 里用 `io_uring_cqe_get_data(cqe)` 取回 awaiter。
- IOCP：把 `OVERLAPPED` 放在 awaiter 第一个成员，`GetQueuedCompletionStatus` 返回 `OVERLAPPED*` 后可还原为 awaiter。

严格点说，awaiter 必须活到 completion 到达。最常见安全做法是 awaiter 位于协程帧内，且一次 async request 对应一次挂起。同步失败也必须走同一条 completion 路径或立即恢复，不能漏减 pending，也不能二次 resume。CQE 必须 `io_uring_cqe_seen()`；IOCP 必须保留 `OVERLAPPED` 到 completion 被取走。

## 3. I1：Asio awaitable

Asio 把回调模型包成 completion token。`use_awaitable` 告诉 Asio：完成时不要调用用户 callback，而是恢复当前协程。

核心路径：

```cpp
auto listener_done = co_spawn(ctx, listener(acceptor, stop, sessions), asio::use_future);
ctx.run();
listener_done.get();
```

`co_spawn` 创建协程并把它绑定到 executor。reference 用 `use_future` 持有 listener、client 和每个 session 的完成结果；`ctx.run()` drain 后逐个 `get()`，因此异常和完成都可观察。之后每个 `async_*` 都在 `await_suspend` 投递操作；completion handler 负责恢复协程。单线程 `io_context::run()` 下，多个 session 是并发等待，不是并行执行。任意时刻只有一个 handler 在跑，所以协程局部状态不需要锁；换成多线程 `io_context` 后，共享状态就必须用 strand、锁或消息化设计。

错误处理建议：

- 常规 IO 结果用 `asio::as_tuple(use_awaitable)` 返回 `(error_code, value)`。
- 真正异常才走异常路径。
- 取消是常规路径。`cancellation_signal` 通过 `bind_cancellation_slot(slot, token)` 进入异步操作，通常表现为 `operation_aborted`。

I1 reference 不用固定端口，也不用 `nc`。它绑定 `port 0`，内部启动 client，验证 echo 后取消 pending accept。这让习题能进 CI。

## 4. I2：io_uring / IOCP awaiter

这一题要观察的是“框架最小内核”。不要一开始写完整 TCP server，先把一条请求跑通。

Linux 版最小不变量：

- `await_suspend` 获取 SQE，`io_uring_prep_read`，设置 `user_data`。
- `io_uring_submit` 成功后 pending 加一。
- event loop 用 timeout 等 CQE，取回 awaiter，写入 `cqe->res`。
- `io_uring_cqe_seen` 先于 `resume`，避免恢复后的协程马上提交下一次请求时 CQ 状态仍未消费。

Windows 版最小不变量：

- socket 必须用 overlapped 模式创建。
- socket handle 必须绑定到 completion port。
- `OVERLAPPED` 的生命周期必须覆盖 `GetQueuedCompletionStatus`。
- 同步失败和异步完成都要进入一个统一的 result path。
- event loop 有硬 timeout，测试不能永久挂住。

两版 reference 都是自动验收：Windows 用 loopback socket，Linux 用 temp file read。它们不是生产 IO 框架，只是把“OS completion -> awaiter -> resume”这条链打穿。

## 5. I3：Folly safe coroutine

Folly 的重点不是“又一个 task”，而是大型代码库的协程安全边界。普通 `Task<T>` 是 lazy coroutine：创建时不跑，await/start 后绑定 executor 并执行。`folly::coro::safe` 进一步把常见生命周期坑编码到类型系统。

2026 系列 Folly 中应优先看这些头：

- `folly/coro/Task.h`
- `folly/coro/BlockingWait.h`
- `folly/coro/safe/SafeTask.h`
- `folly/coro/safe/NowTask.h`

关键规则：

- `value_task<T>` 适合只接收值语义参数的协程。
- `member_task<T>` 用于成员协程，但类需要显式 safe-alias 标注。
- `closure_task<T>` 配合 `async_closure`，避免 lambda capture 生命周期悬空。
- `now_task<T>` 强制在创建表达式内立即 await，减少“保存一个引用捕获协程以后再跑”的错误。
- `blocking_wait` 只适合 main/test bridge，不应在 executor 线程里调用，否则可能死锁。

I3 reference 保留一个可执行子集：`value_task<int>`、`now_task<int>`、executor-bound `Task`、`async_closure(bind::args{...}, ...)`、以及面向 cleanup/scope 场景的 `co_cleanup_safe_task<int>`。旧版手写 `my_safe_task` 模拟被删除，因为它会让学习者误以为 Folly 的安全只是几个 `static_assert`。

## 6. I4：Boost.Cobalt channel / race / gather

Cobalt 把协程通信做得很直观。`channel<T>` 类似 Go channel，但它强调同执行器内的直接上下文切换：有读者等待时，写者可以直接把控制交给读者，不必把恢复动作扔回 executor 队列。这就是 symmetric transfer 在工程库里的价值。

需要准确区分：

- `channel.write(v)`：缓冲未满可完成；缓冲满则挂起写者；已有读者时可直接交付。
- `channel.read()`：有数据则返回；无数据则挂起读者；已有写者时可直接接收。
- `gather(a, b, ...)`：启动并等待多个 awaitable 完成。
- `race(a, b, ...)`：等待最先完成的 awaitable。Boost.Cobalt 示例展示返回 winner index；不要在没有核对版本文档时断言所有版本都会自动取消 loser。

背压不是 bug，是异步系统的流量控制。channel 满时生产者挂起并释放 executor，所以单线程执行器通常不会因为“正确 co_await write”而死锁。真正危险的是混入同步阻塞或 busy loop：生产者占着唯一线程，消费者没有机会 drain。

I4 reference 使用 `cobalt::main co_main(int,char**)`，不使用旧宏猜测；读固定数量的数据，避免对 close/optional API 做版本猜测。

## 7. I5：cppcoro 对照

cppcoro 是理解协程库设计的好参照：

- `task<T>`：lazy、单消费者，接近本仓库前面实现的 `lazy_task`。
- `shared_task<T>`：结果可被多个 awaiter 共享。
- `generator<T>`：同步 pull 序列，观察 `co_yield` 和 iterator 协议。
- `sync_wait`：从非协程 `main` 启动顶层 async flow。
- `when_all`：结构化 fan-out/fan-in。
- `static_thread_pool::schedule()`：明确调度器切换点。
- `cancellation_source` / `cancellation_token` / `cancellation_registration`：显式取消通道和回调注册。

这道题的价值是闭环：前面你写过 frame、promise、awaiter、scheduler；这里你看到一个库如何把它们组成稳定 API。

## 8. 完成本模块后的闭环

你应该能回答四层问题：

- 是什么：C++ 协程是语言级可挂起函数；库把它连接到 executor、IO 和生命周期模型。
- 为什么：它让异步等待写成顺序控制流，同时保留非阻塞 IO 和结构化组合。
- 怎么实现：协程帧保存状态；awaiter 在 `await_suspend` 投递异步工作；completion 路径恢复 handle；`final_suspend` 负责 continuation/lifetime。
- 怎么用：在网络、RPC、文件 IO、异步资源加载、channel pipeline 和任务 scope 中使用；CPU 密集路径仍交给线程池/并行算法。

验收时不要只看“能编译”。还要能画出每题的时间线：

```text
submit -> suspend -> event loop/kernel completion -> result stored -> resume -> await_resume
```

如果这条线画不出来，说明只是会写语法，还没有真正理解协程。

## 参考来源

- Asio 1.38.2：C++20 coroutines、cancellation、`experimental::parallel_group` examples。
- liburing 2.15：SQE/CQE、`user_data`、`io_uring_wait_cqe_timeout`。
- Microsoft Windows IOCP：`CreateIoCompletionPort`、`GetQueuedCompletionStatus`、overlapped Winsock。
- Folly v2026.08.31.00：`folly/coro/Task.h`、`folly/coro/safe/SafeTask.h`。
- Boost 1.92 Cobalt：`cobalt::main`、`channel`、`race`、`gather`。
- andreasbuhr/cppcoro `8642e985`：`task`、`shared_task`、`generator`、`sync_wait`、`when_all`、`static_thread_pool`。
