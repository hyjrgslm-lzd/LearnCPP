# U01：异步日志队列、背压与关闭

先读 C05 的 [spdlog 同步前端](../../../C05_Data_Representation_Standard_Facilities/chapters/20-spdlog-frontend.md)，再回到 C08 的 [等待与有界队列](../../chapters/05-waiting-and-channels.md) 和 [取消与关闭](../../chapters/06-cancellation-and-shutdown.md)。C05 说明 logger、sink、格式化后端和错误处理；本题接管异步队列、worker、溢出、flush 请求和关闭排空。

本题使用固定源码：spdlog v1.17.0 commit `79524ddd08a4ec981b7fea76afd08ee05f83755d`，fmt v12.1.0 commit `407c905e45ad75fc29bf0f9bb7c5c2fd3475976f`。构建必须显式打开 `CONCURRENCY_STUDY_ENABLE_SPDLOG=ON`，并从 `CONCURRENCY_STUDY_SPDLOG_SOURCE_DIR` / `CONCURRENCY_STUDY_FMT_SOURCE_DIR` 指向源码目录；默认关闭，避免普通 C08 构建依赖 C05 的本地下载物。

## 问题

同步日志调用在调用线程里检查 level、格式化、写 sink。异步日志把已接受的 `log_msg_buffer` 放入 `details::thread_pool` 的 MPMC 有界队列，再由 worker 调用 `async_logger::backend_sink_it_`。调用返回不代表 sink 已写入；队列满时必须选策略；sink 异常发生在 worker；关闭时要先停 producer，再排空已接受消息和 flush 请求，最后 join worker。

不要用 `sleep` 猜测调度。本题的 checker 使用可控 sink：首条消息到达 sink 后卡在门闩上，主线程确定 worker 已经拿走首条消息，再向容量 1 的队列提交后续消息。这样稳定制造“worker 忙、队列满”的状态。所有卡住 worker 的场景都有 RAII 放闸，断言失败也不会把 worker 留在门闩里。

## 你要实现的接口

在 `src/student/async_logging_submission.hpp` 中完成 `async_logging_submission`：

- `make_pool(queue_size, workers)`：创建并返回 `std::shared_ptr<spdlog::details::thread_pool>`。
- `make_logger(sink, pool, policy)`：创建 `spdlog::async_logger`，接入给定 sink、pool 和 overflow policy，并把 level 设为 info。
- `log(logger, marked_payload)`：提交一条 info 日志。`marked_payload` 的 formatter 会记录格式化发生在哪个线程。
- `request_flush(logger)`：调用 async logger 的 flush 接口，只投递 flush 请求。
- `shutdown(logger, pool)`：释放 logger，再销毁 pool，让 pool 析构投递 terminate 并 join worker。

初始 Student 的 `complete=false`，`main.cpp` 直接安全退出并提示 Part 1-4 未完成。完成后再改为 `true`。

## Part 与答案

### Part 1：thread_pool 是拥有 worker 的运行时

操作位置：实现 `make_pool` 和 `make_logger`。合法结果：logger 可投递消息；pool 提前销毁后，`logger.info()` 和 `logger.flush()` 都不能入队，会走 `error_handler` 报告 `thread pool doesn't exist`。解释：async logger 保存的是 `weak_ptr<thread_pool>`，logger 不拥有 pool；真实系统要让 pool 活得至少和所有 async logger 一样久。

### Part 2：三种满队列策略

操作位置：`make_logger` 必须保留传入的 `async_overflow_policy`。合法结果：`block` 在满队列时卡住调用线程直到有空位；`overrun_oldest` 在容量满时覆盖最旧队列项，最终写出 `first, third`；`discard_new` 在容量满时丢弃新项，最终写出 `first, second`。解释：这三种策略分别把成本交给 producer、旧日志或新日志；调用方默认没有逐条失败返回，只能看 counter 或外部观测。

### Part 3：payload、格式化线程与 flush

操作位置：实现 `log` 和 `request_flush`。合法结果：`marked_payload` 的 formatter 在调用线程执行，sink 在 worker 线程执行；入队后修改原字符串，sink 仍看到旧文本，说明异步消息拥有已格式化 payload。`request_flush` 返回时 flush 还未完成；放开 sink 并 shutdown 后，队列中的 flush 才执行。解释：`flush()` 是同一队列里的请求，不是完成通知；非阻塞策略下，flush 请求本身也可能被丢弃。

### Part 4：错误与关闭

操作位置：实现 `shutdown`，并保证 handler 不抛未处理异常。合法结果：sink 抛 `spdlog_ex` 时进入 logger 的 `error_handler`；shutdown 后已接受消息和 flush 请求排空；pool 析构 join worker。解释：关闭顺序是停 producer、释放可能阻塞 worker 的 I/O 或门闩、释放 logger、销毁 pool。本题不覆盖 C11 的服务观测、远程日志确认或跨进程持久化协议。

## Bad variants

`validation/bad_overflow` 把 `overrun_oldest` 错接成 `discard_new`，同一 checker 必须用 `overrun counter` 拒绝它。

`validation/bad_flush` 漏掉 `request_flush`，同一 checker 必须用 `queued flush runs during drain` 拒绝它。

负例由 `expect_failure.cmake` 包装：bad 可执行程序必须退出 1，且输出 `check failed: ...` 中的指定诊断。这里不用 CTest 的 `WILL_FAIL`。

## 源码导读

固定源码阅读顺序：

1. `include/spdlog/async_logger.h`：`async_overflow_policy`、`thread_pool_` 是 `weak_ptr`，`sink_it_` 和 `flush_` 都只投递请求。
2. `include/spdlog/details/thread_pool.h`：队列项是 `async_msg`，它移动 `log_msg_buffer` 和 logger 的 `shared_ptr`。
3. `include/spdlog/details/thread_pool-inl.h`：`post_log`、`post_flush` 统一进入 `post_async_msg_`；析构投递 terminate 并 join。
4. `include/spdlog/details/mpmc_blocking_q.h`：`enqueue` 阻塞，`enqueue_nowait` 覆盖最旧项，`enqueue_if_have_room` 丢新项。
5. `include/spdlog/async_logger-inl.h`：worker 线程调用 `backend_sink_it_` / `backend_flush_`，sink 异常进入 `err_handler_`。

## 构建与运行

从 `C08_Concurrency/exercises` 执行：

```powershell
cmake -S . -B build/c08-logging-author -G "Visual Studio 18 2026" -A x64 -DCONCURRENCY_STUDY_ENABLE_SPDLOG=ON -DCONCURRENCY_STUDY_BUILD_REFERENCE=ON
cmake --build build/c08-logging-author --config Release --target U01_async_logging_checked
ctest --test-dir build/c08-logging-author -C Release -R U01_async_logging --output-on-failure
```

预期结果：`U01_async_logging_reference` 和 `U01_async_logging_good` 通过；两个 bad variant 被 `expect_failure.cmake` 成功拒绝。Starter 只有显式设置 `CONCURRENCY_STUDY_TEST_STARTERS=ON` 时才注册，初始状态会失败，提示 Part 1-4 未完成。
