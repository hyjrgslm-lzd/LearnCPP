# 练习 J-3：协程调试与 tracing

知识讲解：[J3 对应章节](../../12-模块J-陷阱诊断与跨编译器.md#j3)。

对应主讲义：`12-模块J-陷阱诊断与跨编译器.md` 的 J-3。

协程挂起后，普通线程栈只显示当前 handler 或调度器位置；逻辑上的 `task_c -> task_b -> task_a` 链在协程帧和 continuation 中。`traced_awaitable` 的价值是把每个 `co_await` 的挂起、恢复、线程和 frame id 打出来。

reference 中的调用链：

```text
task_c()
  -> co_await traced(task_b(), "c.calls_b")
      task_b()
        -> co_await traced(task_a(), "b.calls_a")
            task_a()
              -> co_await traced(inline_resume_int{1}, "a.step1")
              -> co_await traced(inline_resume_int{2}, "a.step2")
        -> co_await traced(inline_resume_int{10}, "b.step2")
```

预测结果：`task_a` 得 3，`task_b` 得 13，`task_c` 得 26。Debug 构建会在 stderr 输出 `INIT`、`SUSPEND`、`RESUME`、`FINAL`；每个 frame 只显示 `coro#N`，不鼓励解引用 frame 地址。reference 结束时检查 registry 为空，说明已完成协程和提前丢弃的协程都注销了。

**答案解析：** `task_a` 两次 inline await 分别得到 1 和 2，所以返回 3；`task_b` 等到 `task_a` 后再加 10，得到 13；`task_c` 等到 `task_b` 后乘 2，得到 26。trace 的 frame id 是调试标识，帮助看逻辑链和恢复线程；registry 为空才说明 tracing 元数据跟随 frame 生命周期清理，没有留下悬空记录。

运行：

```powershell
cmake -S Coroutine_Study/exercises -B build/coroutine-j3 -DCOROUTINE_STUDY_BUILD_REFERENCE=ON
cmake --build build/coroutine-j3 --target J3_coroutine_tracing_reference
ctest --test-dir build/coroutine-j3 -R J3_coroutine_tracing_reference --output-on-failure
```

回读路径：先看 `Registry` 的 `set/mark/erase`，再看 `log_event` 如何产生 opaque id，然后看 `traced_awaitable::await_suspend/await_resume`。最后看 `traced_task::initial_suspend/final_suspend`，确认注册和注销都在 frame 生命周期内。

**答案解析：** `Registry` 是 frame id 和状态的外部索引，`set/mark/erase` 对应注册、状态变化和注销。`traced_awaitable` 记录每个等待点的 suspend/resume，`traced_task` 在 initial/final 边界包住整个协程生命周期；这两层合起来才能回答“谁在等谁、何时恢复、哪个 frame 已经结束”。

调试器实验：MSVC 用 Parallel Stacks 的 Tasks 视图；GDB 14+ 用 `info coroutines`。日志告诉你发生过什么，调试器告诉你暂停瞬间停在哪里。

**答案解析：** trace 日志是时间线证据，适合复盘已经发生的 await 链和耗时阶段。调试器是现场证据，适合在断点处查看当前 coroutine frame、continuation 和线程栈；两者结合能把逻辑调用链和物理执行线程分开。
