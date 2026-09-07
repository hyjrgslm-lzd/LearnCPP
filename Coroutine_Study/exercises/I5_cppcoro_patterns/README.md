# I5 cppcoro patterns

知识讲解：[I5 对应章节](../../11-模块I-真实异步IO与并发框架.md#i5)。

对应主讲义：`11-模块I-真实异步IO与并发框架.md` 的 I-5。

这题把本仓库手写 runtime 的概念映射到 cppcoro。`task` 是 lazy 单消费者结果；`shared_task` 缓存结果供多次 await；`generator` 是同步 pull 序列；`sync_wait` 从 `main()` 启动顶层 awaitable；`when_all` 汇合兄弟任务；`static_thread_pool::schedule()` 是明确调度点；`cancellation_source/token/registration` 是显式取消通道。

reference 预测：

- `range(1,4)` 求和为 6，`range(4,7)` 求和为 15，`when_all` 汇合后总和 21。
  **答案解析：** `generator` 同步产出半开区间 `[first,last)`，所以 1+2+3=6，4+5+6=15。两个 `sum_range` 是 lazy `task`，由 `when_all` 在同一个组合点启动并汇合，外层 `demo` 返回 21。
- `cached_answer()` 可被 await 两次，两次都是 42。
  **答案解析：** `shared_task` 把完成结果缓存到共享状态，后续 awaiter 可以读取同一个结果。第一次 await 负责启动或等待完成，第二次 await 看到已完成状态后直接从缓存取得 42。
- `co_await pool.schedule()` 后线程应变化。
  **答案解析：** `static_thread_pool::schedule()` 是显式调度点，await 后 continuation 被安排到 pool worker 上恢复。reference 比较 await 前后的 `thread::id`，用线程变化证明调度发生。
- `request_cancellation()` 后 token 和 callback 都可观察。
  **答案解析：** `cancellation_source` 持有停止状态，`request_cancellation()` 把 token 切到已请求。`cancellation_registration` 注册的回调在请求后可见，reference 同时检查 `token.is_cancellation_requested()` 和 callback flag。

运行：

```powershell
cmake -S Coroutine_Study/exercises -B build/coroutine-i5 -DCOROUTINE_STUDY_ENABLE_CPPCORO=ON -DCOROUTINE_STUDY_FETCH_DEPS=ON -DCOROUTINE_STUDY_BUILD_REFERENCE=ON
cmake --build build/coroutine-i5 --target I5_cppcoro_patterns_reference
ctest --test-dir build/coroutine-i5 -R I5_cppcoro_patterns_reference --output-on-failure
```

回读代码按 API 对照：先看 `generator` 如何喂给 `sum_range`，再看 `when_all` 如何同时启动两个 `task`，然后看 `shared_task` 的复用、`schedule()` 的线程切换和 cancellation registration 的回调。

**答案解析：** 这些 API 分别对应本课程前面手写的机制：generator 是 pull 序列，task 是 lazy 单消费者结果，when_all 是 fan-out/fan-in，shared_task 是多 awaiter 共享结果，schedule 是恢复位置切换，cancellation registration 是停止请求回调。逐个映射后，就能把库 API 行为落回 promise、awaiter、shared state 和 scheduler 这些对象关系。
