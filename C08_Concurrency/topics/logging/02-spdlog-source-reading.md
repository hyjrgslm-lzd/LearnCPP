# spdlog v1.17.0 异步源码导读

固定版本：spdlog commit `79524ddd08a4ec981b7fea76afd08ee05f83755d`，fmt commit `407c905e45ad75fc29bf0f9bb7c5c2fd3475976f`。C08 的构建通过 `exercises/cmake/SpdlogSetup.cmake` 显式指向源码目录，并在配置期检查 HEAD 与 tracked dirty 状态。

读 `include/spdlog/async_logger.h` 先看三个事实。第一，`async_overflow_policy` 只有 `block`、`overrun_oldest` 和 `discard_new`。第二，`async_logger` 保存的是 `std::weak_ptr<details::thread_pool>`。第三，`sink_it_` 和 `flush_` 都是虚函数覆盖，异步 logger 在这里把同步前端接到线程池。

读 `include/spdlog/async_logger-inl.h` 看运行边界。`sink_it_` 先 lock `thread_pool_`，成功后 `post_log(shared_from_this(), msg, overflow_policy_)`；失败则抛 `async log: thread pool doesn't exist anymore`，再由 `SPDLOG_TRY` 的外层机制转给错误处理。`flush_` 同理投递 flush 请求。worker 侧的 `backend_sink_it_` 遍历 sink，单个 sink 抛异常时进入 `err_handler_`，随后再判断 `flush_on`。

读 `include/spdlog/details/thread_pool.h` 看队列项。`async_msg` 继承 `log_msg_buffer`，不可复制、可移动，并带一个 `async_msg_type`。log 项持有 logger 的 `shared_ptr`，保证 worker 处理该条消息时 logger 对象还活着；flush 项同样带 logger 指针。terminate 项没有 logger。

读 `include/spdlog/details/thread_pool-inl.h` 看提交和销毁。`post_log` 与 `post_flush` 都进入 `post_async_msg_`，再按 overflow policy 调用队列。析构函数按 worker 数量投递 terminate，并 join 线程。这里的 terminate 用 block 策略，因此排在已经接受的普通消息之后。

读 `include/spdlog/details/mpmc_blocking_q.h` 看满队列行为。`enqueue` 等待 `!q_.full()`；`enqueue_nowait` 直接 `push_back`，底层 circular queue 满时覆盖最旧项并记录 overrun；`enqueue_if_have_room` 满时不 push，只增加 discard counter。flush 请求也是普通队列项，所以同样受这些策略影响。

最后回看 `include/spdlog/logger-inl.h`。同步 logger 的 `sink_it_` 直接调用 sink，异步 logger 的前端先生成消息再投递。这就是 C05 与 C08 的分界：格式化、level 和 sink 接口来自 C05；有界队列、worker、背压、重排和关闭由 C08 讲清楚。
