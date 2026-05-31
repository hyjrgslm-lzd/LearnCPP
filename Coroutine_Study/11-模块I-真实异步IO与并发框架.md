# 11 模块 I：真实异步 IO 与并发框架

## 模块目标

前面三个阶段和模块 H 让你掌握了协程的编译器变换、promise hook、awaiter 三层协议、symmetric transfer、以及协程与 sender-receiver 的桥接通道。模块 I 要带你走进生产环境的真实工程框架：

- 用 **Asio + awaitable** 搭建完整回声服务器，理解 `io_context` 的单线程多协程模型、`cancellation_slot` 的优雅关停、以及 `parallel_group` 的并发等待语义。
- 亲手为 **Linux io_uring** 和 **Windows IOCP** 写最小 awaiter，理解 OS kernel 队列与协程帧的协作模式、completion 唤醒模型、以及异步 API 与 awaiter 的最小耦合设计。
- 剖析 **Folly coro Task / SafeTask** 的编译期安全不变量：禁止悬空引用、强制 executor 注入、`NowTask` 的立即 await 约束、`async_closure + co_cleanup` 协议、以及 FIFO 公平 `CoMutex`。
- 用 **Boost.Cobalt channel / race / gather** 搭建生产者-消费者管道，理解协程间的对称转移通信、`channel` 的背压（backpressure）机制、以及单线程执行器下的死锁陷阱。

如果你不经过这四道题，协程对你来说就始终停留在"语法特性"层面——你无法用它在 Linux 上驱动 io_uring 做百万 QPS 的网络服务，也无法在 Windows IOCP 上用协程替代回调地狱，更无法自信地选择 Folly 或 Cobalt 作为团队的基础设施。

## 模块完成标准

做完本模块，你至少要能稳定说清楚：

- Asio 的 `co_spawn` 如何把协程绑定到 `io_context` 上，以及单线程多协程模型的调度循环如何复用 `io_context::run()`。
- `cancellation_slot` 与协程 `stop_token` 的关系——取消信号如何从外部注入到挂起的协程体中，以及 `co_await` 作为天然 cancellation point 的实现。
- `asio::experimental::parallel_group` 的并发语义：同时启动多个异步操作，任一完成后取消其余，这与 `when_all`/`when_any` 的本质差异。
- Linux io_uring 的提交完成模型：SQE 提交、CQE 轮询、以及如何在 awaiter 中将 `cqe->user_data` 映射回 `coroutine_handle` 完成 resume。
- Windows IOCP 的 overlapped 模型：`WSARecv` 投递 overlapped 请求、IOCP 线程从 `GetQueuedCompletionStatus` 取出结果、通过 `OVERLAPPED` 基类寻址 awaiter。
- `folly::coro::SafeTask` 为什么禁止 `co_await SafeTask{}` 产生悬空引用——编译期捕获的机制如何比文档约定更可靠。
- `folly::coro::NowTask` 的 `await_ready() = false` 且无法被 `co_await` 延迟的设计意图：它要求调用者在原地立即驱动。
- `folly::coro::CoMutex` 的 FIFO 公平性是如何实现的——等待者链表 + `await_suspend` 入队 + 逐个唤醒。
- `boost::cobalt::channel<T>` 如何在两个协程之间实现零调度器参与的对称转移通信——以及为什么这在单线程执行器下既是最大优势也是最大陷阱。
- 单线程执行器下的背压死锁模式：生产者写满 channel 后阻塞，消费者因为生产者占着线程而无法被调度去 drain channel。

## 使用建议

- 本模块是**工程模块**——允许并鼓励使用 stdexec、Asio、folly、Boost.Cobalt、liburing 等第三方依赖。推荐准备两个环境：Linux（带 liburing-dev + Asio + Folly + Boost）和 Windows（带 Asio + Folly + Boost + MSVC IOCP 支持）。
- 练习 I-2 提供 Linux io_uring 和 Windows IOCP 两份代码骨架，读者按本地环境选择其一即可。两份都读更好——理解两种 OS 异步模型的共性与差异本身就是学习目标。
- 练习 I-1（Asio echo server）是唯一要求完整可运行版本——启动后可用 `nc` 或 `telnet` 实测。I-3（Folly SafeTask）和 I-4（Cobalt channel）给出可编译骨架。
- 建议四道题各预留 2-3 小时。I-2 的 io_uring/IOCP awaiter 是最需要"调试心态"的题——第一次基本都会踩到 completion 丢失或 double-resume 的坑。
- 如果你对 Asio 的 `io_context` / `strand` / `cancellation_slot` 不熟，先读 Asio 官方 tutorial 前 3 章。
- 如果你对 io_uring 的 SQE/CQE 环形缓冲不熟，先读 `liburing` 的 `io_uring.pdf`（Jens Axboe）或 `lordoftheio_uring` 教程。

---

## 练习 I-1：Asio + awaitable

### 目标

用 Asio 的 `co_spawn` + `awaitable<T>` 搭建完整可运行的回声服务器，处理多客户端并发连接，用 `cancellation_slot` 实现优雅关停，并掌握 `io_context` 单线程多协程模型的调度语义。同时探索 `asio::as_tuple(use_awaitable)` 的无异常错误处理模式和 `asio::experimental::parallel_group` 的并发等待语义。

### 前置理解

- 你已经理解 C++20 协程的 `promise_type`、`co_await` 变换、以及 `coroutine_handle::resume()` 的基本用法。
- 你知道 Asio 的 `io_context` 本质上是一个事件循环：`io_context::run()` 在单线程中轮询 IO completion 并分发回调。
- 你知道 `co_spawn` 的内部工作：它把协程的 `initial_suspend` 之后的 resume 作为"回调"投递到 `io_context` 的 executor 上。
- Asio 的 `use_awaitable` completion token 将异步操作（如 `async_read_some`）的结果通过 `co_await` 返回。`asio::as_tuple(use_awaitable)` 变体将错误码和结果打包为 tuple，避免 try/catch 异常路径——这是生产级协程代码的标准写法。
- `cancellation_slot` 是 Asio 的取消机制：一个 `cancellation_signal` 通过 slot 注入到异步操作中，操作在下一个取消点（如 `co_await`）检查并终止。协程的 `stop_token` 和 Asio 的 `cancellation_slot` 是不同的类型系统——在练习中你会看到它们如何协作。
- `asio::experimental::parallel_group` 是 Asio 对"并发等待多个异步操作"的答案：它同时启动多个操作，任一个完成后可选取消其余（`wait_for_one`），或等待全部完成后汇合（`wait_for_all`）。

### 必做任务

1. **搭建最小的 `io_context` + 协程骨架**：定义一个返回 `asio::awaitable<void>` 的协程，在 `main` 中 `co_spawn` 启动它，然后跑 `io_context.run()`。

   ```cpp
   #include <asio.hpp>
   #include <asio/experimental/awaitable_operators.hpp>
   #include <iostream>
   #include <string>

   using asio::awaitable;
   using asio::co_spawn;
   using asio::detached;
   using asio::use_awaitable;
   using asio::ip::tcp;

   awaitable<void> echo_session(tcp::socket socket) {
       // ... 回声逻辑
   }

   awaitable<void> listener(tcp::acceptor& acceptor,
                            asio::cancellation_signal& cancel_signal) {
       // ... 监听循环 + 取消
   }

   int main() {
       asio::io_context ctx;
       asio::cancellation_signal cancel_signal;

       tcp::acceptor acceptor(ctx, tcp::endpoint(tcp::v4(), 12345));
       co_spawn(ctx, listener(acceptor, cancel_signal), detached);

       // 在另一个线程等待用户输入后关停
       std::jthread shutdown_thread([&] {
           std::cout << "Press Enter to shutdown..." << std::endl;
           std::cin.get();
           cancel_signal.emit(asio::cancellation_type::all);
       });

       ctx.run();
       shutdown_thread.join();
   }
   ```

2. **实现回声会话协程**：每次 `async_read_some` -> `async_write` 循环，直到对端关闭。

   ```cpp
   awaitable<void> echo_session(tcp::socket socket) {
       try {
           std::array<char, 1024> buf;
           for (;;) {
               // as_tuple(use_awaitable) 返回 tuple<error_code, size_t>
               // 无需 try/catch 处理普通错误（如 EOF）
               auto [ec, n] = co_await socket.async_read_some(
                   asio::buffer(buf),
                   asio::as_tuple(use_awaitable)
               );
               if (ec) {
                   if (ec != asio::error::eof) {
                       std::cerr << "read error: " << ec.message() << "\n";
                   }
                   co_return;  // 对端关闭
               }
               co_await asio::async_write(socket,
                   asio::buffer(buf, n),
                   use_awaitable
               );
           }
       } catch (const std::exception& e) {
           std::cerr << "session error: " << e.what() << "\n";
       }
   }
   ```

3. **实现带取消支持的监听循环**：

   ```cpp
   awaitable<void> listener(tcp::acceptor& acceptor,
                            asio::cancellation_signal& cancel_signal) {
       for (;;) {
           // 将 cancellation_slot 绑定到 accept 操作
           // 注意：bind_cancellation_slot 包装整个 completion token，
           // 而不是作为 async_accept 的第二个参数传入
           auto slot = cancel_signal.slot();
           auto [ec, socket] = co_await acceptor.async_accept(
               asio::bind_cancellation_slot(slot, asio::as_tuple(use_awaitable))
           );

           if (ec) {
               if (ec == asio::error::operation_aborted) {
                   std::cout << "Listener cancelled, shutting down.\n";
                   co_return;
               }
               std::cerr << "accept error: " << ec.message() << "\n";
               continue;
           }

           std::cout << "New connection from "
                     << socket.remote_endpoint() << "\n";

           // 为每个客户端启动协程（fire-and-forget，由 listener 持有生命周期）
           co_spawn(co_await asio::this_coro::executor,
                    echo_session(std::move(socket)),
                    detached);
       }
   }
   ```

   关键观察：`bind_cancellation_slot(slot, asio::as_tuple(use_awaitable))` 返回一个 completion token，包装了原始的 async_accept operation。当 `cancel_signal.emit()` 被调用时，token 内部的 slot 会触发 operation 的 abort，`async_accept` 以 `operation_aborted` 错误完成，协程被 resume 后检查 `ec` 并退出循环。

   **关于内置 cancellation_slot**：Asio awaitable 协程已有内置 cancellation_slot（由 `co_spawn` 注入并随 `co_await` 传播），协程内所有 async_xxx 自动感知该 slot。本练习中外部传入 `cancellation_signal` 仅用于演示手动绑定机制——生产代码更常见的做法是在 `co_spawn` 时绑定 slot，然后协程内部通过 `co_await asio::this_coro::cancellation_state` 查询取消状态（而非在每个异步操作上手动 bind）：

   ```cpp
   // 生产代码模式：co_spawn 时绑定 slot，listener 内部不操心 slot
   co_spawn(ctx, listener(acceptor),
            asio::bind_cancellation_slot(cancel_signal.slot(), detached));
   // listener 协程内的所有 co_await async_xxx 自动感知 slot
   ```

4. **验证多客户端并发 + 优雅关停**：
   - 启动服务器，用 `nc localhost 12345` 或 `telnet localhost 12345` 连接多个客户端。
   - 每个客户端发送任意文本，观察回声。
   - 按 Enter 触发 `cancel_signal.emit()`，观察所有连接优雅关闭。
   - 观察：所有 session 和 listener 都在同一个 `io_context::run()` 线程上执行——单线程多协程模型。

5. **实验 `parallel_group` 的并发等待语义**：

   ```cpp
   #include <asio/experimental/parallel_group.hpp>

   awaitable<void> parallel_echo(tcp::socket sock1, tcp::socket sock2) {
       using namespace asio::experimental::awaitable_operators;

       std::array<char, 1024> buf1, buf2;

       // 同时等待两个 socket 的可读事件
       // wait_for_one: 任一个完成就返回（其余继续或取消）
       // 解构形式以官方 Asio 文档（asio::experimental::parallel_group::async_wait）为准。
       // 当前 Asio 版本中 parallel_group 返回 std::tuple<std::array<std::size_t, N>, T0..., T1..., ...>，
       // 其中 T_i 是第 i 个 op 的 completion token 展开。as_tuple(use_awaitable) 将每个 op 展平为 (ec, n)，
       // 因此本处结构化绑定形式 [order, ec1, n1, ec2, n2] 对应 N=2 的展平结果。
       auto [order, ec1, n1, ec2, n2] = co_await parallel_group(
           sock1.async_read_some(asio::buffer(buf1),
                                 asio::as_tuple(use_awaitable)),
           sock2.async_read_some(asio::buffer(buf2),
                                 asio::as_tuple(use_awaitable))
       ).async_wait(
           asio::experimental::wait_for_one(),
           use_awaitable
       );

       // order[0] 是首先完成的操作索引（0 或 1）
       if (order[0] == 0 && !ec1) {
           // sock1 先有数据，回复它，并且可以取消 sock2 的等待
           co_await asio::async_write(sock1, asio::buffer(buf1, n1),
                                      use_awaitable);
       }
       // 注意：parallel_group 的 wait_for_one 语义是"任一完成后返回"，
       // 完成后兄弟操作可能还在运行——需要显式管理其余操作的取消。
   }
   ```

   关键观察：`parallel_group` 和 `when_all`/`when_any` 的语义差异——`parallel_group` 是 Asio 生态特有的，它理解 Asio 的 `cancellation_slot`，可以在内部取消未完成的操作。而 `when_all`/`when_any` 是 sender-receiver 体系的概念，不绑定特定的 IO 框架。

6. **在笔记中画出单线程多协程调度模型**：

   ```
   io_context::run()
     |
     |-- epoll_wait / IOCP GetQueuedCompletionStatus 返回就绪事件
     |
     |-- 找到对应的 completion handler (协程的 resume)
     |
     |-- handler.resume()  // 恢复挂起的协程
     |      |
     |      |-- 协程执行到下一个 co_await 点
     |      |-- 发起新的 async_read/write (投递到 OS)
     |      |-- 再次挂起，返回 io_context::run()
     |
     |-- 继续循环下一个事件
   ```

   核心推论：在单线程 `io_context` 模型下，任何两个协程都不会并发执行——它们被 `io_context::run()` 的循环串行化了。这意味着协程内的普通变量访问不需要加锁。

### 进阶任务

- 将单线程 `io_context` 替换为 `asio::thread_pool`（多线程），验证当多个 session 协程可能在不同线程上被 resume 时，需要在哪些地方加锁。对比单线程和多线程模型的复杂度差异。
- 实现 `co_spawn` + `asio::steady_timer` 的超时逻辑：如果客户端 30 秒不发送数据，自动关闭连接。推荐用 `asio::experimental::parallel_group` 实现"read 和 timer race"的模式。
- 用 `asio::strand` 序列化对共享数据的访问：两个 session 协程在同一个 `strand` 中运行时，即使底层是多线程 `io_context`，也能保证互斥。
- 在 `echo_session` 中注入 `cancellation_slot`，使得关停信号也能终止正在处理中的 session，而不仅仅是 listener。

### 验收点

- 你的回声服务器能处理多个并发客户端，每个客户端发什么就回什么。
- 按 Enter 后，所有正在等待的 accept 操作被取消，listener 返回，`io_context::run()` 退出。
- 你能解释 `io_context::run()` 的单线程多协程调度循环：为什么两个 session 协程不会并发执行，以及为什么协程内变量不需要锁。
- 你能画出一个协程从 `co_await async_read_some` 挂起到被 `io_context` resume 的完整路径。
- `asio::as_tuple(use_awaitable)` 和裸 `use_awaitable` 的差异你能一句话说清：前者返回 `tuple<error_code, T>` 需要手动检查 ec；后者在错误时抛异常。

### 观察点

- `co_spawn` 是协程进入 Asio 生态的入口——它把协程的 resume 动作注册为 `io_context` 的一个 completion handler。从此，这个协程的生命周期就交给了 `io_context`。
- `asio::as_tuple(use_awaitable)` 模式是生产级 Asio 协程代码的标准做法：普通错误（如对端关闭、超时）用 error_code 处理，真正的异常（如内存不足、协议错误）才走 try/catch。
- `cancellation_slot` 的传播路径需要显式绑定的每一步：`cancellation_signal` -> `slot()` -> `bind_cancellation_slot(slot, token)` -> 目标异步操作。如果中间任何一步没有传播 slot，取消信号就会丢失。不过 Asio awaitable 协程有内置 cancellation_slot（由 `co_spawn` 注入），生产代码通常将 slot 绑定在 `co_spawn` 入口而非每个操作上。
- `parallel_group` 的 `wait_for_one` 语义对应"race"模式——在网络编程中是实现超时的最自然选择。
- 单线程 `io_context` + 协程的组合让并发编程"感觉像顺序编程"——你写的是 `co_await op1(); co_await op2()`，但实际上 op1 挂起期间 op2 永远不会执行。这是结构化并发的协程表达。

### 常见坑

- `echo_session` 里使用 `co_spawn` 启动子协程时忘了指定 executor——子协程的 `initial_suspend` resume 发生在 `io_context` 之外（或未定义位置），导致子协程在不同的上下文中执行。
- `cancellation_signal` 在 `main` 栈上，但 `co_spawn` 的协程在 `io_context::run()` 后才正式启动——如果 `main` 在 `ctx.run()` 前退出，signal 被析构而协程还在用它的 slot。
- 对 `async_read_some` 返回的 `n == 0` 或 `ec == eof` 没有正确判断，导致回声循环永远不退出。
- `parallel_group` 中未完成的操作没有被显式取消——它们在后台继续等待，浪费资源且可能在析构时触发未定义行为。
- 以 `use_awaitable` 方式 co_await 的 socket 操作抛出异常后，socket 状态未清理（如 shutdown），导致资源泄漏。

### 提示

- 先用最小版本跑通：listener + 单 session echo，确认能回声。再加 cancellation 支持。再加多客户端测试。
- Asio 的 `this_coro::executor` 让你在协程体内获取当前绑定的 executor——不要手动传递 `io_context&`。
- `asio::as_tuple(use_awaitable)` 的返回类型：当操作产生 `T` 时返回 `std::tuple<asio::error_code, T>`；仅产生错误码时返回 `std::tuple<asio::error_code>`（单元素 tuple）。
- 如果 `parallel_group` 编译不过，检查是否正确 include 了 `<asio/experimental/parallel_group.hpp>` 和 `<asio/experimental/awaitable_operators.hpp>`。

### 复盘问题

- 单线程 `io_context` 多协程模型和 `std::execution::run_loop` 单线程事件循环在调度模型上有哪些结构相似性？
- 为什么 `cancellation_slot` 需要显式传递，而 `stop_token` 在 sender-receiver 体系中是自动通过 environment 传播的？你更偏好哪一种设计？
- 如果回声服务器的 session 数达到 10000 个，单线程 `io_context` 的瓶颈在什么地方？多线程 `io_context` 是自然的解决方案吗？
- `parallel_group::wait_for_one` 和标准 `when_any` 在取消语义上有什么本质差异？

### 对应官方参考

- Asio coroutine documentation: `https://think-async.com/Asio/asio-1.30.2/doc/asio/overview/composition/cpp20_coroutines.html`
- Asio `parallel_group` examples: `asio/src/examples/cpp20/coroutines/parallel_group.cpp`
- Asio `cancellation_slot`: `https://think-async.com/Asio/asio-1.30.2/doc/asio/overview/core/cancellation.html`
- Lewis Baker "C++ Coroutines and Asio" CppCon 2022

---

## 练习 I-2：io_uring (Linux) / IOCP (Windows) 双版本 awaiter

### 目标

为 Linux `io_uring` 和 Windows IOCP 两套底层异步 I/O 接口分别编写最小 awaiter，将 kernel completion 事件与协程的挂起/恢复打通。通过这道题理解 OS kernel 队列与协程帧的协作模式、completion 唤醒时机、以及异步 API 与 awaiter 的最小耦合设计。

### 前置理解

- 你已经理解 awaiter 的三方法协议：`await_ready()`、`await_suspend(coro_handle)`、`await_resume()`。`await_suspend` 中协程挂起，返回 void 将控制权交给调用者；后续某个时刻 `handle.resume()` 恢复协程。
- **Linux io_uring 模型**：内核和用户空间共享一对环形缓冲区——SQ（Submission Queue）和 CQ（Completion Queue）。用户将 SQE（Submission Queue Entry）写入 SQ，内核处理后生成 CQE（Completion Queue Entry）写入 CQ。用户可以轮询 CQ 或通过 `io_uring_enter` 阻塞等待。
- **Windows IOCP 模型**：用户通过 `CreateIoCompletionPort` 创建完成端口，将 socket/file handle 绑定到端口。调用 `WSARecv`/`WSASend` 并传入 `OVERLAPPED` 结构。工作线程调用 `GetQueuedCompletionStatus` 阻塞等待完成事件，取出 `OVERLAPPED` 指针定位到原始请求。
- 两套模型的核心共性：用户投递请求 -> 内核异步处理 -> 内核通知完成 -> 用户根据完成信息找到原始请求上下文 -> resume 等待者。avaiter 的任务就是把"找到原始请求上下文并 resume 等待者"这一步从回调/CQE/OVERLAPPED 翻译为 `coroutine_handle::resume()`。
- OS 异步 API 与 awaiter 的最小耦合原则：awaiter 不拥有 IO buffer 的生命周期（由协程帧管理）；awaiter 不负责 IO 的启动（由协程在 `await_suspend` 前或其中调用 OS API）；awaiter 只负责"等待完成"这一件事。

### 必做任务

#### A. Linux io_uring 版本

1. **设计 io_uring awaiter 的正确接口**（以下版本使用 `this` 指针 + 内置 `coroutine_handle` 成员，是正确的做法）：

   ```cpp
   #include <liburing.h>
   #include <coroutine>

   struct uring_read_awaiter {
       struct io_uring* ring_;
       int fd_;
       void* buf_;
       unsigned nbytes_;
       off_t offset_;

       std::coroutine_handle<> coro_;  // 由 await_suspend 设置
       struct io_uring_cqe* cqe_ = nullptr;

       bool await_ready() noexcept { return false; }

       void await_suspend(std::coroutine_handle<> h) noexcept {
           coro_ = h;
           auto sqe = io_uring_get_sqe(ring_);
           io_uring_prep_read(sqe, fd_, buf_, nbytes_, offset_);
           // user_data 设置为 this（awaiter 自身指针），
           // 这样事件循环可以从 CQE 找回 awaiter 并设置 cqe_ 成员
           io_uring_sqe_set_data(sqe, this);
           io_uring_submit(ring_);
       }

       int await_resume() noexcept {
           return cqe_->res;  // 对于 read: 读取字节数；<0 为 -errno
       }
   };
   ```

   关键细节：`io_uring_sqe_set_data(sqe, this)` 把 awaiter 自身指针编码进 `user_data`。当 kernel 完成 IO 后，CQE 的 `user_data` 字段原封不动地包含 `this` 指针——事件循环通过它找回 awaiter，设置 `cqe_` 成员，然后调用 `coro_.resume()`。这样构成了完整的回溯链：`CQE → awaiter → coroutine_handle → resume`。

2. **实现 io_uring 事件循环**：

   ```cpp
   #include <liburing.h>
   #include <coroutine>
   #include <functional>

   struct uring_loop {
       struct io_uring ring;
       bool running_ = true;

       uring_loop(unsigned entries = 256) {
           io_uring_queue_init(entries, &ring, 0);
       }

       ~uring_loop() {
           io_uring_queue_exit(&ring);
       }

       void run() {
           struct io_uring_cqe* cqe;
           while (running_) {
               // 等待至少一个 CQE 可用
               int ret = io_uring_wait_cqe(&ring, &cqe);
               if (ret < 0) {
                   // 错误处理
                   continue;
               }

               // 从 user_data 恢复 awaiter（this 指针）
               auto* awaiter = static_cast<uring_read_awaiter*>(
                   io_uring_cqe_get_data(cqe)
               );
               awaiter->cqe_ = cqe;

               io_uring_cqe_seen(&ring, cqe);
               awaiter->coro_.resume();  // 恢复协程 → await_resume 取出 cqe->res
           }
       }

       void stop() { running_ = false; }
   };
   ```

   这个设计实现了 OS 异步 API 与 awaiter 的最小耦合：`io_uring_prep_*` 填充 SQE（与协议无关），`sqe_set_data(this)` 建立了"完成事件 -> awaiter -> 协程"的回溯链。

端到端文件读取验证：

   ```cpp
   struct read_file_task {
       struct promise_type {
           read_file_task get_return_object() { return {}; }
           std::suspend_never initial_suspend() { return {}; }
           std::suspend_always final_suspend() noexcept { return {}; }
           void return_void() {}
           void unhandled_exception() {}
       };
   };

   read_file_task read_and_print(uring_loop& loop, const char* path) {
       int fd = open(path, O_RDONLY);
       if (fd < 0) co_return;

       char buf[4096];
       int n = co_await uring_read_awaiter{
           &loop.ring, fd, buf, sizeof(buf), 0
       };
       // 协程在此处挂起 → io_uring 处理 read → CQE 生成 →
       // 事件循环找到 awaiter → 设置 cqe_ → resume 协程 →
       // await_resume 返回 n

       if (n > 0) {
           write(STDOUT_FILENO, buf, n);
       }
       close(fd);
       co_return;
   }
   ```

#### B. Windows IOCP 版本

1. **设计 IOCP awaiter 的最小接口**：

   IOCP 的核心机制是 `OVERLAPPED` 结构——每个异步 IO 请求都关联一个 `OVERLAPPED`。`GetQueuedCompletionStatus` 返回时，可以取出与完成事件对应的 `OVERLAPPED*`。通过将 `OVERLAPPED` 嵌入 awaiter（利用 `OVERLAPPED` 作为基类或首个成员），可以在完成时从 `OVERLAPPED*` 反向寻址到 awaiter。

   ```cpp
   #include <windows.h>
   #include <coroutine>

   // 将 OVERLAPPED 嵌入 awaiter——利用 CONTAINING_RECORD 宏从 OVERLAPPED* 找回 awaiter*
   struct iocp_recv_awaiter {
       OVERLAPPED overlapped_{};  // 必须是标准布局，且 overlapped_ 是第一个成员
       SOCKET socket_;
       char* buf_;
       DWORD len_;
       DWORD transferred_ = 0;
       DWORD flags_ = 0;
       std::coroutine_handle<> coro_;

       iocp_recv_awaiter(SOCKET s, char* buf, DWORD len)
           : socket_(s), buf_(buf), len_(len) {
           memset(&overlapped_, 0, sizeof(overlapped_));
       }

       bool await_ready() noexcept { return false; }

       void await_suspend(std::coroutine_handle<> h) noexcept {
           coro_ = h;

           WSABUF wbuf{len_, buf_};
           DWORD flags = 0;
           int ret = WSARecv(socket_, &wbuf, 1, nullptr, &flags,
                             &overlapped_, nullptr);

           if (ret == SOCKET_ERROR) {
               int err = WSAGetLastError();
               if (err != WSA_IO_PENDING) {
                   // 真正的错误——可以在这里直接 resume 或存储 errno
                   // （简化处理：假设非 pending 即错误）
               }
               // WSA_IO_PENDING: 正常，等待 IOCP 完成
           }
           // 协程挂起——IOCP 线程会在完成时通过 overlapped_ 找回 awaiter 并 resume
       }

       int await_resume() noexcept {
           return static_cast<int>(transferred_);
       }
   };
   ```

2. **实现 IOCP 事件循环**：

   ```cpp
   struct iocp_loop {
       HANDLE iocp_;
       bool running_ = true;

       iocp_loop() {
           iocp_ = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
       }

       ~iocp_loop() {
           CloseHandle(iocp_);
       }

       void bind(SOCKET s, ULONG_PTR completion_key = 0) {
           CreateIoCompletionPort(reinterpret_cast<HANDLE>(s), iocp_,
                                  completion_key, 0);
       }

       void run() {
           while (running_) {
               DWORD transferred;
               ULONG_PTR key;
               OVERLAPPED* ov;

               BOOL ok = GetQueuedCompletionStatus(
                   iocp_, &transferred, &key, &ov, INFINITE
               );

               if (!ov) {
                   // 退出信号或错误
                   continue;
               }

               // 从 OVERLAPPED* 恢复 awaiter*
               // 利用 overlapped_ 是 iocp_recv_awaiter 第一个成员的布局
               auto* awaiter = reinterpret_cast<iocp_recv_awaiter*>(ov);
               awaiter->transferred_ = transferred;

               awaiter->coro_.resume();
           }
       }

       void stop() { PostQueuedCompletionStatus(iocp_, 0, 0, nullptr); }
   };
   ```

   这里的关键技巧：`OVERLAPPED` 作为 `iocp_recv_awaiter` 的第一个成员，使得 `OVERLAPPED*` == `iocp_recv_awaiter*`（标准布局保证）。事件循环拿到 `OVERLAPPED*` 后直接 reinterpret_cast 回 awaiter，无需额外的查找表。

3. **对比设计：Linux io_uring vs Windows IOCP awaiter**：

   | 维度 | io_uring | IOCP |
   |------|----------|------|
   | 请求投递 | `io_uring_prep_*` + `io_uring_submit` | `WSARecv`/`WSASend` 传入 `OVERLAPPED*` |
   | 完成通知 | `io_uring_wait_cqe` → CQE | `GetQueuedCompletionStatus` → `OVERLAPPED*` |
   | awaiter 寻址 | `cqe->user_data`（显式存储的指针） | `OVERLAPPED*` == awaiter*（标准布局偏移） |
   | 事件循环 | 单线程轮询 CQ | 单线程阻塞 `GetQueuedCompletionStatus` |
   | 多操作类型 | `io_uring_prep_read`/`prep_write`/`prep_accept` 等 | `WSARecv`/`WSASend`/`AcceptEx` 等 |
   | 取消 | `io_uring_prep_cancel` 投递取消 SQE | `CancelIoEx` 或 `PostQueuedCompletionStatus` 特殊键 |

4. **在笔记中画出 kernel 队列与协程帧协作的时间线**：

   ```
   协程帧                    内核态                    事件循环线程
   -------                   ------                    -----------
   co_await awaiter
     |
     await_suspend:
       sqe_set_data(this)
       io_uring_submit  ----→  SQE 进入 SQ
     [协程挂起]                   |
                             内核处理 IO
                                |
                                CQE 进入 CQ ---------→  io_uring_wait_cqe 返回
                                                           |
                                                        awaiter = cqe->user_data
                                                        awaiter->cqe_ = cqe
                                                        awaiter->coro_.resume()
                                                            |
     await_resume ←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←
       返回 cqe_->res
     [协程继续]
   ```

### 进阶任务

- 为 io_uring awaiter 添加超时支持：额外的 `io_uring_prep_timeout` SQE + `IORING_OP_LINK_TIMEOUT` 链接标志。超时触发时，原始 IO 被自动取消，CQE 以 `-ECANCELED` 完成。
- 为 IOCP awaiter 改造为标准 C++ 布局：用 `static_assert(offsetof(iocp_recv_awaiter, overlapped_) == 0)` 验证布局以替代依赖 "第一个成员" 的隐式假设。
- 实现 io_uring 的 SQ polling 模式（`IORING_SETUP_SQPOLL`），让事件循环不需要每次 `io_uring_enter` 系统调用——kernel 线程持续轮询 SQ。对比 SQPOLL 模式和不使用 SQPOLL 模式的延迟差异。
- 在 Windows 上用 `Registered I/O` (RIO) 替换 IOCP + `WSARecv` 方案，对比 RIO 的更低延迟和更复杂的 buffer 管理模型。

### 验收点

- 你的 io_uring awaiter 能完成文件读取，且 await_resume 返回的字节数与预期一致。
- 你能说明 `sqe_set_data(this)` 和 `cqe->user_data` 如何构成"完成事件 -> awaiter -> 协程"的回溯链。
- 你能说明 IOCP 中 `OVERLAPPED*` == `awaiter*` 成立的前提条件（标准布局、overlapped_ 是第一个成员）。
- 你能画出 kernel IO 完成到协程 resume 的完整事件时间线。
- 你能解释为什么 OS 异步 API 与 awaiter 可以最小耦合——awaiter 只负责"等待"和"取出结果"，IO 的启动和 buffer 管理由协程体或上层库负责。

### 观察点

- io_uring 的 `sqe->user_data` 字段是 awaiter 和协程之间的**关键粘合剂**——它把内核不知晓的用户态"协程 identity"编码进 SQE，在 CQE 中原样返回。这是 uring 能从 kernel 找回用户态上下文的唯一通道。
- IOCP 的 `OVERLAPPED*` 寻址技巧与 uring 的 `user_data` 本质上是同一问题的两种解法：**如何从 completion 事件中找回原始的请求上下文**。io_uring 用显式的 `void*`，IOCP 用标准布局偏移。
- 两套模型的 awaiter 结构惊人地相似：都有一个"挂起前投递请求"的方法（`await_suspend`），都有一个"恢复后取出结果"的方法（`await_resume`），都依赖协程帧的稳定性来保证 awaiter 在挂起期间存活。
- 这种模式验证了一个原则：**awaiter 是异步 API 的统一适配层**——无论底层是 io_uring、IOCP、epoll、kqueue、还是 DIY 轮询循环，awaiter 的三方法协议总能把它们统一映射为 co_await 语法。

### 常见坑

- io_uring 的 `cqe->res` < 0 时不加判断直接当字节数使用——`-errno` 是负值，强转 size_t 后变成巨大的正数。
- `sqe_set_data` 设置了指针但事件循环忘记 `io_uring_cqe_seen`——CQE 永远留在 CQ 中导致环形缓冲区被填满、无法投递新请求。
- IOCP 版本中 `OVERLAPPED` 不是第一个成员——reinterpret_cast 出来的地址偏移错误，访问到垃圾数据或崩溃。
- 事件循环在 `resume()` 之前忘记读取 CQE 或设置 awaiter 的成员——`await_resume` 看到的 cqe 是空指针或垃圾值。
- `await_suspend` 中没有等待事件循环设置完 `cqe_` 就 resume 了别的协程——race condition 导致使用未初始化的 cqe 数据。
- io_uring 事件循环中没有处理 `cqe->res < 0` 的情况，把 IO 错误当成了成功值传递给上层。

### 提示

- 先实现 io_uring 版本——它比 IOCP 版本简单清晰，因为 io_uring 的 API 设计本身就是为现代异步模型做的。
- 事件循环的线程模型：io_uring 通常只需要一个线程（主线程）跑事件循环；IOCP 传统上会用线程池。对于学习目的，两者都用单线程跑就够。
- `io_uring_cqe_seen` 必须调用，不能跳过——它的作用是推进 CQ 的消费指针，让内核可以重用 CQE 条目。忘了调用会导致 CQ 的可用条目逐渐枯竭。
- 在 IOCP 版本中使用 `CONTAINING_RECORD` 宏（而非裸 reinterpret_cast）能更好地表达"从嵌套成员指针推导容器指针"的意图，也更安全。
- 这两份 awaiter 代码的总体量约 100-150 行——核心是理解 completion-to-resume 链，而不是堆代码量。

### 复盘问题

- io_uring 和 IOCP 两套模型的 awaiter 设计为什么如此相似？它们之间的相似性是否说明 awaiter 协议具有某种"OS 异步 API 通用适配器"的性质？
- 为什么 io_uring 需要显式的 `sqe_set_data(this)` 而 IOCP 不需要？如果 uring 也把 awaiter 嵌入 `sqe` 中会有什么问题？
- `io_uring_cqe_seen` 在事件循环中必须由用户代码显式调用的设计是否有其意图（给用户更多控制权）？如果内核自动推进 CQ 会有什么不好的后果？
- 如果把 io_uring 的 SQPOLL 模式和 IOCP 的线程池模式放一起比较，两者在"kernel 主动轮询"这个设计模式上有哪些共同点和根本差异？

### 对应官方参考

- `liburing` 官方仓库及示例: `https://github.com/axboe/liburing`（尤其 `examples/io_uring-cp.c`）
- Jens Axboe "Efficient IO with io_uring" PDF
- `lordoftheio_uring` 教程: `https://unixism.net/loti/`
- Microsoft IOCP 文档: `https://learn.microsoft.com/en-us/windows/win32/fileio/i-o-completion-ports`
- Lewis Baker "C++ Coroutines: Under the Covers" 中 awaiter 与 OS 回调交互模式

---

## 练习 I-3：Folly coro Task 与 SafeTask

### 目标

将模块 D 的 `lazy_task<T>` 升级为 Folly 风格的 `Task<T>` 和 `SafeTask<T>`，理解 Folly 对生产级协程框架的安全设计：编译期禁止悬空引用、强制 executor 注入、`NowTask` 的立即 await 约束、`async_closure + co_cleanup` 协议、以及 FIFO 公平 `CoMutex`。

### 前置理解

- 你已经完成模块 D（promise_type 基础）和模块 G（shared_task、when_all 等），能手写 task 的基本骨架。
- Folly 的 `folly::coro::Task<T>` 是 Meta 内部数百个服务的基础协程类型。它的设计经历了多年生产环境的打磨，尤其是以下安全特性：
  1. **模板参数必须是值类型或引用包装（`std::reference_wrapper`）** —— 禁止 `Task<const int&>` 这种裸引用导致的悬空问题。
  2. **Executor 必须显式注入**——协程不能"偷偷"跑到默认线程池上执行。
  3. **`SafeTask` 是 `Task` 的安全包装**——增加了"同一个 SafeTask 只能被 co_await 一次"的运行时断言，防止 spawn 的 fire-and-forget 语义越界。
- `folly::coro::NowTask`：它的 `await_ready()` 返回 `false`（始终挂起），但其设计意图是"必须在创建的同一语句中立即 `co_await`"——它不能被存储、传递、或延迟消费。这个约束通过代码审查模式而非类型系统强制执行。
- `async_closure`：Folly 建议使用名为 `Task<T> myClosure(Arg) co_async { ... }` 的宏风格来声明协程 lambda，避免 `[&]` 或 `[this]` 逃逸到 co_await 之后。
- `CoMutex`：Folly 的协程互斥锁，实现基于等待者链表 + FIFO 顺序唤醒——头节点在锁释放时被 resume，新等待者追加到尾部。

### 必做任务

1. **研究 Folly 的 `Task<T>` 模板约束，实现类似的编译期安全检查**：

   在 Folly 中，`Task<T>` 对 T 有三条约束：

   ```cpp
   // folly/experimental/coro/Task.h (简化版意图)
   template<typename T>
   class Task {
       static_assert(!std::is_reference_v<T>,
           "Task<T> requires T to be a value type. "
           "Use std::reference_wrapper or a pointer for references.");
       static_assert(!std::is_const_v<T>,
           "Task<T> requires T to be non-const. "
           "Top-level const is meaningless on a return-by-value type.");
   };
   ```

   在自己的练习代码中实现一个简化版 `my_safe_task<T>`：

   ```cpp
   #include <coroutine>
   #include <functional>  // std::reference_wrapper
   #include <type_traits>
   #include <cassert>

   template<typename T>
   struct my_safe_task {
       // 编译期拒绝裸引用
       static_assert(!std::is_reference_v<T>,
           "SafeTask does not allow T to be a reference. "
           "Use std::reference_wrapper<T> instead.");

       struct promise_type {
           T result_;
           std::exception_ptr exception_;
           std::coroutine_handle<> continuation_;
           bool awaited_ = false;  // 运行时安全：只允许一次 co_await
           // 注意：Folly 用更复杂的原子 + 状态机来做这件事
       };
       // ... connect / awaitable 接口
   };
   ```

   编译期约束的价值：`Task<const int&>` 如果被允许，协程 co_return 一个局部变量后，co_await 者在 stack 销毁后读取引用——这是隐蔽的 UB。编译期拒绝直接将 Bug 从运行时拉到编译时。

2. **实现 Executor 注入机制**：

   Folly 的协程不通过全局线程池隐式切换执行器。每个协程在创建时必须说明它将在哪个 Executor 上运行。一种典型模式是 `co_viaIfAsync`：

   ```cpp
   // 概念骨架：co_await 另一个 task 时如果它在不同 executor 上，
   // 需要显式切换回来
   template<typename Executor>
   auto co_via_if_async(Executor& ex) {
       struct via_awaiter {
           Executor* ex_;
           bool await_ready() { return ex_->running_in_this_thread(); }
           void await_suspend(std::coroutine_handle<> h) {
               ex_->add([h] { h.resume(); });
           }
           void await_resume() {}
       };
       return via_awaiter{&ex};
   }

   // 在协程中的使用：
   my_safe_task<int> compute(folly::CPUThreadPoolExecutor& cpu_ex,
                              folly::IOExecutor& io_ex) {
       // 在 CPU executor 上
       auto data = co_await fetch_data(io_ex);       // 子 task 在 IO executor 上
       co_await co_via_if_async(cpu_ex);              // 恢复到 CPU executor
       int result = heavy_compute(data);              // 保证在 CPU executor 上
       co_return result;
   }
   ```

   关键观察：Executor 注入不是魔法——它要求协程中每次跨 executor 调用后都显式用 `co_via_if_async` 或 `schedule_on` 拉回。这是 Folly 的设计哲学：显式优于隐式，可审计优于魔法。

3. **剖析"协程 lambda capture this"问题与 Folly 解法**：

   ```cpp
   // 危险的写法：
   struct MyClass {
       folly::coro::Task<int> fetch() { co_return 42; }

       folly::coro::Task<void> dangerous() {
           // lambda 捕获 this——但 co_await 后 this 可能已经悬空！
           auto task = some_async_op().then([this](auto&&) {
               return this->member_;  // UB if this is destroyed
           });
           co_await task;
       }
   };
   ```

   Folly 的解决方式——`async_closure` + `co_cleanup`：

   ```cpp
   // Folly 推荐的模式：把协程 lambda 文档化
   struct MyClass {
       int member_ = 10;

       folly::coro::Task<int> safe_co_closure() {
           // 使用 Folly 的 async_closure 模式：把 this 作为显式参数传入
           auto closure = [](MyClass* self) -> folly::coro::Task<int> {
               // self 由调用者保证生命周期
               co_return self->member_;
           };
           co_return co_await closure(this);
       }
   };
   ```

   进一步，Folly 的 `co_cleanup` 协议：

   ```cpp
   // folly::coro::co_cleanup 在协程取消或异常时保证执行清理逻辑
   folly::coro::Task<void> with_cleanup() {
       auto cleanup = folly::coro::co_cleanup([&]() -> folly::coro::Task<void> {
           // 无论协程以何种方式退出（正常/异常/取消），这里都会执行
           std::cout << "cleanup executed\n";
           co_return;
       });
       // ... 正常的协程体
       co_await some_io();
       // co_cleanup 的析构回调在协程退出时自动触发
   }
   ```

4. **实现 FIFO 公平 CoMutex**：

   ```cpp
   #include <coroutine>
   #include <queue>

   struct co_mutex {
       bool locked_ = false;
       std::queue<std::coroutine_handle<>> waiters_;

       struct lock_awaiter {
           co_mutex& mtx_;

           bool await_ready() noexcept {
               if (!mtx_.locked_) {
                   mtx_.locked_ = true;
                   return true;  // 不挂起，直接获取锁
               }
               return false;
           }

           void await_suspend(std::coroutine_handle<> h) noexcept {
               // 入队——FIFO 尾插
               mtx_.waiters_.push(h);
           }

           void await_resume() noexcept {
               // 被唤醒时，锁已经由前一个持有者释放
               // 当前协程已经是锁的持有者（在 unlock 中设置的 locked_ = true 在 resume 之前）
           }
       };

       lock_awaiter lock() { return lock_awaiter{*this}; }

       void unlock() {
           if (waiters_.empty()) {
               locked_ = false;
           } else {
               auto next = waiters_.front();
               waiters_.pop();
               // locked_ 保持 true——传给下一个等待者
               next.resume();  // 恢复下一个等待者，它会在 await_resume 中成为锁的持有者
           }
       }

       // RAII 风格的门闩
       struct scoped_lock {
           co_mutex* mtx_;
           scoped_lock(co_mutex& m) : mtx_(&m) {}
           ~scoped_lock() { if (mtx_) mtx_->unlock(); }
           scoped_lock(scoped_lock&&) = delete;
       };
   };
   ```

   关键观察：
   - FIFO 公平性通过 `std::queue` 保证——先 `await_suspend`（入队）的协程先被 `unlock` 唤醒。
   - `unlock` 中先设置 `locked_ = true` 再 resume 下一个等待者（在 `scoped_lock` 用法中不直接设；在 `unlock` 中直接 resume 代表"传递锁"）。
   - 在单线程执行器下，`unlock` 之后的 `next.resume()` 是安全的——下一个协程会在当前协程完全退出后才开始执行（通过 symmetric transfer 或直接在 unlock 中 resume）。

5. **评测 FIFO 公平性与性能**：设计一个并发实验——10 个协程竞争同一把 CoMutex，记录每个协程等待时间的分布。统计最小/最大/平均等待时间，并验证 FIFO 顺序（先到达的协程等待时间应小于后到达的）。

### 进阶任务

- 将 `co_mutex` 的 `std::queue` 替换为侵入式链表——消除 `std::queue` 的堆分配，将等待者的节点嵌入协程帧中。分析 intrusive 设计的性能优势和在协程场景下的适配性。
- 实现 `co_mutex` 的公平性保护：在不公平的调度器（多线程 thread_pool）下，FIFO 顺序可能被破坏——验证这一点并设计一种在单线程执行器上保持"虚拟顺序"的方案。
- 研究 `folly::coro::NowTask` 的源码实现——它的 `await_ready()` 返回 `false`，但 `await_suspend` 中的行为与普通 Task 的关键区别。讨论为什么 Folly 不在类型系统中强约束 NowTask 必须在同一语句中使用。
- 实现一个针对 `SafeTask` 的并发压力测试：多个协程 `co_await` 同一个 `SafeTask`——第二个 `co_await` 的运行时断言是否按预期触发？

### 验收点

- 你的 `my_safe_task<T&>` 或 `my_safe_task<const int&>` 在编译期产生清晰的 static_assert 错误信息，而不是运行时崩溃。
- 你能说明 Executor 注入为什么是"显式优于隐式"的体现——它迫使开发者在每次跨 executor 调用后思考"我现在在哪个线程上"。
- 你能解释 Folly 如何用 `co_cleanup` + `async_closure` 模式解决"协程 lambda capture this"的悬空问题。
- 你的 FIFO `co_mutex` 在各种并发场景下都能公平地按到达顺序授予锁。
- 你能列举至少三种 `folly::coro::Task` 在生产环境中比 `lazy_task<T>` 更安全的设计决策。

### 观察点

- Folly 的设计哲学：把更多错误从运行时拉到编译时（`static_assert` 拒绝裸引用），从隐式拉到显式（executor 必须传递），从"靠自觉"拉到"靠工具"（`SafeTask` 的一次性运行时断言）。
- `co_cleanup` 是协程世界的 RAII——在普通函数中析构函数处理资源释放，在协程中 `co_cleanup` 处理挂起点之前的清理。但 `co_cleanup` 的实现远比析构函数复杂——它本身也是一个协程。
- FIFO CoMutex 在单线程执行器下是最自然的公平锁实现——没有真正的并发，只需要维护正确的等待者队列和恢复顺序。
- "协程 lambda capture this"问题本质上是协程的生命周期模型和经典 C++ lambda 捕获语义的冲突——lambda 捕获发生在创建时（可能在线程 A），但 lambda 体执行在 co_await 后（可能在线程 B，且 this 已析构）。

### 常见坑

- `CoMutex::unlock()` 中先调用了 `next.resume()` 再设置 `locked_ = false`——被 resume 的协程在 `await_resume` 中无法判断它是否真正持有锁。
- `CoMutex::lock_awaiter::await_ready()` 中的 `locked_ = true` 没有配对——在单线程执行器下没问题，但后续若改为多线程则需要原子操作。
- 把 `Task<T&&>` 当作可接受右值引用的 Task——Folly 同样禁止（右值引用本质上也是引用），且在语义上 `Task` 的 `co_return` 按值返回，不存在"移动出临时对象"的场景。
- 忘记了 `SafeTask` 只能 co_await 一次——在 `while` 循环中反复 co_await 同一个 SafeTask 会触发运行时断言。
- `co_cleanup` 中访问的局部变量可能已经在 `co_cleanup` 注册之后、执行之前变了——需要区分"注册时的快照"和"执行时的当前值"。

### 提示

- 先对照 `folly/experimental/coro/Task.h` 源码读——注意 `Task<T>`（允许多次 co_await，类似 shared_task）和 `TaskWithExecutor<T>`（绑定 executor）之间的结构关系。
- `SafeTask` 的"一次性"约束本质上是对 fire-and-forget 语义的类型化——"一旦 co_await 了 SafeTask，它的生命周期就结束了，你不能让另一个协程也等待它"。
- `NowTask` 虽然名字是 Now，但它的 await_ready 返回 false——这看起来矛盾，但它的"Now"指的是"使用者必须立刻 co_await"，而不是"立刻完成"。
- FIFO CoMutex 在单线程环境下几乎就是一个 `std::queue<std::coroutine_handle<>>`——不要过度设计。

### 复盘问题

- 为什么 Folly 不直接在类型系统中禁止 `NowTask` 被存储到变量中（类似于 Rust 的 `must_use` + 生命周期约束），而是依赖代码审查？
- 如果 Folly 的 `Task<T>` 自动支持 executor propagation（像 P3552 task 那样通过 environment），`co_via_if_async` 这种显式模式还需要吗？讨论隐式 propagation 和显式控制的权衡。
- `CoMutex` 的 FIFO 公平性在多线程执行器下为什么不成立？如果在多线程下实现严格的 FIFO 公平锁，需要在哪些地方加锁/改顺序？
- `co_cleanup` 能替代传统的 try/catch + finally 块在协程中的作用吗？什么地方 `co_cleanup` 不如传统方式？

### 对应官方参考

- `folly/experimental/coro/Task.h`：Task 的核心实现
- `folly/experimental/coro/SafeTask.h`：SafeTask 的类型包装
- `folly/experimental/coro/NowTask.h`：NowTask 的实现
- `folly/experimental/coro/Mutex.h`：CoMutex 的完整实现
- `folly/experimental/coro/AsyncScope.h`：与 SafeTask 配合的 scope 管理
- Lewis Baker "Structured Concurrency" CppCon 2019 — Folly coro 的设计动机

---

## 练习 I-4：Boost.Cobalt channel + race + gather

### 目标

用 Boost.Cobalt 的 `channel<T>`、`race`、`gather` 搭建生产者-消费者管道，理解协程间通过 channel 对称转移通信的机制、背压引发的单线程执行器死锁模式、以及 `cobalt::main` 通过宏（`BOOST_COBALT_MAIN`）将 `int co_main(...)` 包装为协程入口的设计。

### 前置理解

- 你已经理解协程的 symmetric transfer：`await_suspend` 返回 `coroutine_handle` 时，编译器直接跳转到目标协程而不经过调用者栈帧——这就是协程间零开销上下文切换的基础。
- Boost.Cobalt 是 Boost 的协程库（之前称为 `boost::asio::experimental::awaitable_operators` 的精神继承者）。它的核心抽象：
  1. `cobalt::promise<T>`：一个无执行器的协程 promise，适合构建 channel 等底层原语。
  2. `cobalt::task<T>`：有执行器绑定的协程 task，类似于 Asio 的 `awaitable<T>` 或 Folly 的 `Task<T>`。
  3. `cobalt::channel<T>`：协程间通信的管道——写者 `co_await channel.write(val)` 挂起直到有读者或缓冲区有空位；读者 `co_await channel.read()` 挂起直到有数据。
  4. `cobalt::race(awaitable...)`：并发等待多个 awaitable，返回首先完成的那个的结果，其余被取消。
  5. `cobalt::gather(awaitable...)`：并发等待多个 awaitable，返回全部结果的集合。
  6. `cobalt::main`：通过宏 `BOOST_COBALT_MAIN` 将 `int co_main(...)` 包装为协程入口——内部自动创建 `io_context` 并驱动协程。它不是返回类型，而是一个宏生成的 thunk。
- channel 的 symmetric transfer 优化：当写者和读者在同一个执行器上时，`channel::write` 的 `await_suspend` 可以直接返回读者的 `coroutine_handle`（对称转移），读者被 resume 后消费数据，然后再通过对称转移回到写者。整个过程不经过执行器的调度队列——零开销。
- 背压（backpressure）问题：当 channel 缓冲区满时，写者挂起。在单线程执行器下，如果消费者没有机会被调度（因为写者占着线程），就形成了死锁——写者等消费者来 drain，消费者等写者释放线程。这是单线程异步系统中 channel 最常见的陷阱。

### 必做任务

1. **搭建最小生产者/消费者管道**：

   ```cpp
   #include <boost/cobalt.hpp>
   #include <iostream>
   #include <string>

   namespace cobalt = boost::cobalt;

   // 生产者：生成 0..N-1 的序列并通过 channel 发送
   cobalt::task<void> producer(cobalt::channel<int>& ch, int N) {
       for (int i = 0; i < N; ++i) {
           std::cout << "producer sending: " << i << std::endl;
           co_await ch.write(i);
       }
       ch.close();  // 标记 channel 关闭——消费者会在 read 时收到 closed 信号
       co_return;
   }

   // 消费者：从 channel 读取并打印
   cobalt::task<void> consumer(cobalt::channel<int>& ch) {
       while (ch.is_open()) {
           auto result = co_await ch.read();
           if (result.has_value()) {
               std::cout << "consumer received: " << result.value() << std::endl;
               // 模拟处理耗时
               co_await cobalt::sleep(std::chrono::milliseconds(10));
           } else {
               // channel 已关闭且无剩余数据
               std::cout << "consumer done (channel closed)" << std::endl;
               break;
           }
       }
       co_return;
   }

   // cobalt::main 替代 int main()
   // 注意：Boost.Cobalt 当前版本中，main 协程入口使用 int co_main(...) 配合
   // BOOST_COBALT_MAIN 宏，而非返回类型 cobalt::main。
   // 以下写法为概念示意——具体 API 请核对正在使用的 Boost 版本。
   int co_main(int argc, char* argv[]) {
       // 创建有界 channel（容量为 2）
       cobalt::channel<int> ch{2};

       // 并发启动生产者和消费者
       // gather 同时运行两个 task，等待两者全部完成
       co_await cobalt::gather(
           producer(ch, 10),
           consumer(ch)
       );

       co_return 0;
   }
   ```

   关键观察：
   - `ch.write(i)` 在 channel 满时挂起生产者协程；`ch.read()` 在 channel 空时挂起消费者协程。
   - `cobalt::channel<int> ch{2}` 设置了容量为 2——当生产者发送 2 个元素后（消费者还没消费完），第 3 次 `write` 会挂起。
   - `ch.close()` 是优雅关闭的信号——消费者读到最后一个值后，再 `read` 会得到无值的 `std::optional` 或错误码（取决于 Cobalt 版本）。
   - `cobalt::main` 内部自动创建了 `io_context`，并在 `co_main` 返回后运行事件循环。

2. **观察 symmetric transfer 效果**：

   在 `channel<T>` 内部，`write` 方法的 `await_suspend` 大致形如：

   ```cpp
   // 概念化实现——Cobalt 实际代码更复杂
   template<typename T>
   auto channel<T>::write_awaiter::await_suspend(
       std::coroutine_handle<> writer_handle) noexcept
   {
       if (!readers_.empty()) {
           // 有等待的读者 → 直接通过 symmetric transfer 将数据交给读者
           auto reader = readers_.front();
           readers_.pop();
           data_ = value_;            // 将写入值移入存储
           reader.promise().result_ = std::move(data_);
           return reader;              // symmetric transfer: 直接跳转到读者
       } else if (buffer_.size() < capacity_) {
           // 缓冲区有空间 → 直接写入，不挂起写者
           buffer_.push(std::move(value_));
           return std::noop_coroutine(); // 不挂起
       } else {
           // 缓冲区满 → 写者入队，挂起
           writers_.push(writer_handle);
           // await_suspend 返回 void → 挂起写者
       }
   }
   ```

   关键观察：当有等待的读者时，`await_suspend` 返回读者的 handle，编译器执行 symmetric transfer——不经过调度器，直接从写者跳到读者。这是在单线程执行器下 channel 零开销通信的根基。

3. **用 `race` 实现超时模式**：

   ```cpp
   cobalt::task<void> consumer_with_timeout(cobalt::channel<int>& ch) {
       while (ch.is_open()) {
           // race: 同时等待 channel read 和 500ms 定时器
           // 优先完成的那个返回结果，另一个被取消
           auto result = co_await cobalt::race(
               ch.read(),
               cobalt::sleep(std::chrono::milliseconds(500))
           );

           // result 是 variant<read_result, ...>
           if (result.index() == 0) {
               auto opt = std::get<0>(result);
               if (opt.has_value()) {
                   std::cout << "received: " << opt.value() << std::endl;
               } else {
                   std::cout << "channel closed" << std::endl;
                   break;
               }
           } else {
               std::cout << "timeout! no data for 500ms" << std::endl;
               // continue waiting
           }
       }
       co_return;
   }
   ```

   注意：`cobalt::race` 的语义是"任一完成后取消其余"。当 channel read 先完成时，timer 被取消；当 timer 先到期时，channel read 操作被取消——读者的挂起被解除（可能以 `operation_aborted` 或类似错误完成）。

4. **用 `gather` 收集多个生产者的结果**：

   ```cpp
   cobalt::task<std::vector<int>> multi_producer() {
       cobalt::channel<int> ch{4};

       // 三个生产者并发写入
       auto prod1 = [&ch]() -> cobalt::task<void> {
           for (int i = 0; i < 5; ++i) co_await ch.write(i);
           co_return;
       };
       auto prod2 = [&ch]() -> cobalt::task<void> {
           for (int i = 10; i < 15; ++i) co_await ch.write(i);
           co_return;
       };
       auto prod3 = [&ch]() -> cobalt::task<void> {
           for (int i = 100; i < 105; ++i) co_await ch.write(i);
           ch.close();  // 最后一个生产者关闭 channel
           co_return;
       };

       // gather: 并发运行三个生产者 + 一个消费者
       // 注意：consumer 需要单独启动，因为 gather 收集的是 task 的返回值
       std::vector<int> collected;
       auto consumer_task = [&]() -> cobalt::task<void> {
           while (ch.is_open()) {
               auto r = co_await ch.read();
               if (r.has_value()) collected.push_back(r.value());
               else break;
           }
           co_return;
       };

       co_await cobalt::gather(
           prod1(), prod2(), prod3(), consumer_task()
       );

       co_return collected;  // 总共 15 个元素
   }
   ```

   `cobalt::gather` 的语义：并行启动所有传入的 task，等待全部完成后返回。结果通过 `co_await gather(...)` 获取，返回类型是一个编译期推导的 tuple——每个 task 的返回类型构成 tuple 的元素类型。

5. **背压死锁实验**——这是本练习最关键的观察点：

   ```cpp
   int co_main(int argc, char* argv[]) {
       cobalt::channel<int> ch{2};  // 容量仅为 2

       // 生产者发送 5 个元素，但 channel 只容纳 2 个
       auto prod = [&ch]() -> cobalt::task<void> {
           for (int i = 0; i < 5; ++i) {
               std::cout << "writing " << i << std::endl;
               co_await ch.write(i);  // 第 3 次 write 会挂起！
           }
           ch.close();
           co_return;
       };

       // 消费者：每次消费后 sleep 1 秒
       auto cons = [&ch]() -> cobalt::task<void> {
           co_await cobalt::sleep(std::chrono::seconds(2));  // 消费者延迟启动！
           while (ch.is_open()) {
               auto r = co_await ch.read();
               if (r.has_value()) {
                   std::cout << "read " << r.value() << std::endl;
                   co_await cobalt::sleep(std::chrono::seconds(1));
               } else break;
           }
           co_return;
       };

       // gather 并发运行——但在单线程执行器下，消费者延迟 2 秒启动意味着：
       // 生产者写入 2 个后 channel 满，第 3 次 write 挂起
       // 生产者挂起后控制权返回给执行器，但消费者还没到第一个 co_await 点！
       // → 死锁
       co_await cobalt::gather(prod(), cons());
       co_return 0;
   }
   ```

   分析死锁的成因：在 `gather(prod(), cons())` 中，两个 task 被并发启动。在单线程执行器下，它们交替执行。但 `cons` 的第一个操作是 `co_await sleep(2s)`——它注册了定时器后挂起。`prod` 写入 2 个元素填满 channel 后在第 3 次 `write` 挂起。两个协程都挂起了，事件循环等待 timer 到期来恢复消费者——timer 到期后消费者被 resume，开始读取 channel。这个实验实际上不会死锁（因为 Cobalt 的事件循环可以处理 timer——在这个具体例子里，timer 到期后消费者被调度就能 read）。

   真正的单线程背压死锁场景是：**生产者写入 channel 满后挂起，消费者需要在同一个线程上执行来 drain channel，但生产者占用着执行器（没有挂起而是忙于计算），消费者永远得不到调度**。或者更简单：生产者写入 channel 满后没有挂起而是 busy-waiting，独占线程。

   改造实验以触发真实死锁：

   ```cpp
   int co_main(int argc, char* argv[]) {
       cobalt::channel<int> ch{1};  // 容量 1——极度有限的缓冲

       // 消费者：必须先运行才能消费，但被延迟
       auto cons = [&ch]() -> cobalt::task<void> {
           co_await cobalt::sleep(std::chrono::seconds(1));  // 故意延迟
           auto r = co_await ch.read();
           std::cout << "finally read: " << r.value() << std::endl;
           co_return;
       };

       // 生产者：写满 channel 后挂起等消费者
       auto prod = [&ch]() -> cobalt::task<void> {
           co_await ch.write(1);       // OK——容量为 1
           co_await ch.write(2);       // 挂起——channel 满了，等待消费者
           // 永远不会到这里：消费者 1 秒后才启动，
           // 但生产者此时已经挂起，不会占用执行器
           // 1 秒后 timer 到期，消费者被 resume，读取数据，
           // 然后生产者被 resume
           std::cout << "wrote 2\n";
           ch.close();
           co_return;
       };

       co_await cobalt::gather(prod(), cons());
       co_return 0;
       // 这个例子实际上也不会死锁——
       // 生产者挂起后释放了执行器（回到事件循环），
       // 1 秒后 timer 到期，事件循环调度消费者，消费者 resume 后 read channel，
       // read 发现 channel 有等待的写者，通过 symmetric transfer 恢复写者。
       // 正确运行，不会死锁。
   }
   ```

   重要认识：**单线程执行器下的背压死锁不是 channel 本身的 bug，而是"消费者无法被调度"的情况**。当使用 `co_await channel.read()/write()` 的模式时，挂起的协程释放了执行器，所以不会死锁。真正的死锁发生在这个模式之外——例如混合了同步阻塞调用。

### 进阶任务

- 用 Cobalt 的 `cobalt::promise`（无执行器的协程 promise）重写 channel 的原语层。`cobalt::promise` 的 `await_suspend` 返回 `void`（而不是 `coroutine_handle`），对比它和 `task<T>` 在 symmetric transfer 支持上的差异。
- 实现 multi-consumer channel：多个读者竞争同一 channel，每个数据项只投递到一个读者。实验不同消费者之间的公平性问题。
- 用 `cobalt::race` 实现一个带超时和重试的 channel 写操作——如果 channel 满，等待最多 1 秒后放弃写入。
- 将生产者/消费者管道扩展为 pipeline：3 个 stage，每个 stage 是一个协程，stage 之间用 channel 连接。数据流经 3 个 stage 后输出。测量吞吐量和延迟。
- 对比 `cobalt::gather` 和 `asio::experimental::parallel_group::wait_for_all` 的取消语义：`gather` 中一个子 task 失败时其他 task 会怎样？

### 验收点

- 你的生产者/消费者管道正确运行——生产者发送序列，消费者完整接收，channel 关闭后双方退出。
- 你能说出 `channel::write` 在"有等待的读者 / 缓冲区有空位 / 缓冲区满"三种情况下的挂起行为及其 symmetric transfer 优化。
- 你能用 `cobalt::race` 实现 channel read 的超时——超时后循环继续等待。
- 你能解释背压死锁的根本原因：在单线程执行器下，生产者填满 channel 后挂起（释放执行器）并不会死锁；死锁发生在消费者无法被调度的情况（例如消费者依赖一个永远不会被触发的外部事件）。
- 你能说明 `cobalt::main` 对比 `int main()` + 手动 `io_context.run()` 的优势：自动管理 io_context 生命周期、main 本身可以是协程直接用 co_await。

### 观察点

- `channel<T>` 的 symmetric transfer 优化是协程间通信的杀手级特性——它让你在单线程执行器下用 channel 传递数据的开销接近函数调用，因为不经过执行器的调度队列。
- `cobalt::race` 是比 `when_any` 更精细的竞争等待工具——它内置了对失败分支的自动取消，不需要手动管理未完成任务。
- 背压（backpressure）是异步系统中的"流量控制"——channel 的缓冲区大小决定了系统能承受的生产-消费速率差异。缓冲区太小导致频繁挂起（降低吞吐），缓冲区太大导致内存占用飙升（OOM 风险）。
- `cobalt::main` 是对"main 不能是协程"这一 C++ 限制的务实解决方案。在 C++26 之前，`int main()` 不能是协程；`cobalt::main` 通过宏展开为一个 thunk，在其中创建 io_context 并驱动协程。

### 常见坑

- 忘记调用 `ch.close()` 导致消费者永远阻塞在最后一次 `read` 上——即使所有生产者都已退出。channel 的关闭信号是区分"暂时无数据"和"永远无数据"的唯一方式。
- `race` 中使用 lambda 而非直接传递 awaitable——lambda 的返回值类型被错误推导为 `void` 而不是协程类型。需要显式指定返回类型。
- `gather` 中一个 task 抛出异常后，其他 task 的状态不确定——某些版本的 Cobalt 在第一个异常后取消剩余 task，某些版本等待全部完成后再传播第一个异常。具体行为需要查阅所用版本的文档。
- 在 `co_await` 前没有检查 channel 的状态——channel 关闭后 `write` 的行为是未定义的（某些实现直接崩溃，某些返回错误）。
- 在高吞吐场景下使用默认的 `channel<T>` 单元素容量——每个元素都需要一次完整的协程挂起/恢复周期，吞吐量严重受限于协程切换开销。

### 提示

- Channel 的容量是一个需要精心调优的参数——对于学习目的，容量 2-4 就够了。生产环境中根据"生产者速率 * 消费者延迟"来设置。
- `cobalt::race` 的结果类型是 `std::variant`——用 `std::holds_alternative` 或 `.index()` 判断哪个分支完成了。
- 如果 `cobalt::main` 无法编译，检查 Include 路径是否正确引入了 `<boost/cobalt.hpp>`。
- 这个练习的核心不在于写出多复杂的代码，而在于真正理解单线程执行器下 channel 的调度语义——多画几个时间线图比多写 200 行代码更有效。

### 复盘问题

- 为什么 `channel<T>` 在"有等待读者"时可以做 symmetric transfer，而在"缓冲区满且没有等待读者"时必须挂起写者？symmetric transfer 的前提条件是什么？
- `cobalt::race` 的"任一完成后取消其余"语义和 `when_any`（不取消其余）相比，哪种更安全？在什么场景下你宁愿用 `when_any` 而不是 `race`？
- 单线程执行器下的背压问题是否可以通过增加缓冲区大小来彻底解决？"无容量限制的 channel"是可行的设计吗？
- `cobalt::main` 隐藏了 io_context 的生命周期管理——这是一种好的抽象还是"魔法"？在什么情况下你会希望显式管理 io_context？

### 对应官方参考

- Boost.Cobalt 官方文档: `https://www.boost.org/doc/libs/master/libs/cobalt/doc/html/index.html`
- `boost/cobalt/channel.hpp`：channel 的完整实现
- `boost/cobalt/race.hpp`：race 的完整实现
- `boost/cobalt/gather.hpp`：gather 的完整实现
- `boost/cobalt/main.hpp`：cobalt::main 的宏定义
- Klemens Morgenstern "Coroutines and Channels" CppCon 2023 / meetingcpp 2023

---

## 做完模块 I 之后，你现在应该能说清楚什么

至少把下面几句话说顺：

- Asio `co_spawn` + `io_context` 的单线程多协程模型本质上是"事件循环串行化"：所有协程由 `io_context::run()` 的循环逐个驱动，任意两个协程不会并发执行，因此协程内变量无需加锁。`cancellation_slot` 通过转化为异步操作的错误码（`operation_aborted`）注入协程的 `co_await` 点，而 `asio::as_tuple(use_awaitable)` 让你用 `error_code` 而非 try/catch 处理这种常规取消。`parallel_group` 是 Asio 生态中的并发/竞争等待原语，其与 sender-receiver 的 `when_all`/`when_any` 的关键差异在于它原生理解 Asio 的 cancellation 传播。

- Linux io_uring 的 awaiter 把 `coroutine_handle` 或 `this` 指针编码进 `sqe->user_data`，kernel 完成时在 CQE 中原样返回，事件循环恢复了 awaiter 并 resume 协程。Windows IOCP 的 awaiter 通过将 `OVERLAPPED` 嵌入 awaiter 作为第一个成员，使 `OVERLAPPED*` 等同于 `awaiter*`，`GetQueuedCompletionStatus` 返回后直接 resume。两套模型的核心设计是同一个模式：**awaiter 是 OS completion 和协程 resume 之间的适配器**——`await_suspend` 投递请求，kernel 完成事件中被寻址的 awaiter 调用 `resume`，`await_resume` 取出结果。

- `folly::coro::Task<T>` / `SafeTask<T>` 通过三把锁把协程安全从"靠自觉"升级到"靠工具"：(1) 编译期 `static_assert` 禁止裸引用和顶层 const 的 T——把悬空引用 Bug 从运行时拉到编译时；(2) Executor 显式注入——`co_via_if_async` 或显式 schedule 确保协程的线程上下文可审计；(3) `SafeTask` 的一次性运行时断言——防止 fire-and-forget 的协程被意外二次消费。`CoMutex` 的 FIFO 公平性在单线程执行器下通过 `std::queue<std::coroutine_handle<>>` 自然实现——等待者按到达顺序入队，解锁时按顺序出队并 resume。

- `boost::cobalt::channel<T>` 是协程间对称转移通信的最佳展示：当写者和读者同时在同一个执行器上时，`write.await_suspend` 直接返回读者的 `coroutine_handle`，编译器执行 symmetric transfer 跳转到消费者——不经过调度器，开销接近函数调用。`cobalt::race` 提供有内置取消的"任一完成"语义，`cobalt::gather` 提供全量汇合的"全部完成"语义。单线程执行器下的背压死锁不是 channel 的缺陷，而是"生产者挂起后释放执行器"这一设计的边界条件——真正的死锁只发生在生产者不挂起而独占线程的场景。

- 至此，你已经可以独立完成一个基于 Asio/Cobalt/Folly 工程框架的真实异步 I/O 服务。你知道如何从 OS kernel completion 到协程 resume 打通全链路，如何在类型系统中编码协程的安全不变量，以及如何在单线程执行器下用 channel 构建低延迟的协程间通信管道。这四道题覆盖了现代 C++ 异步工程的三层基础设施：**OS I/O 层**（io_uring/IOCP）、**协程安全层**（Folly SafeTask）、**协程通信层**（Asio/Cobalt channel + parallel_group + race）。三层贯通之后，proposal 和源码对你而言就不再是黑盒了。
