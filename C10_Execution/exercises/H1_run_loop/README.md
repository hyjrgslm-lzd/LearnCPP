# 练习 H-1：run_loop 调度器

H1 让你从零写一个最小可用的 `run_loop`：`schedule()` 返回 sender，`connect` 产生不可移动的 operation state，`start()` 把 operation state 入队，`run()` 在调用它的线程上按 FIFO 顺序取出并完成 receiver。重点不是“写一个线程池”，而是看清 scheduler sender 与 operation state 地址稳定性的关系。

## 你要实现什么

公开接口在本题 `solution.hpp` 中，命名空间为 `c10_h1`。Reference 现在把实现放在 `H1_run_loop/src/reference/solution.hpp`，不再依赖公共 runtime shortcut。你需要提供：

- `run_loop`：持有 intrusive FIFO 队列、`std::mutex`、`std::condition_variable`、关闭标志。
- `run_loop::schedule()`：返回一个 sender。这个 sender 的 operation state 继承一个队列节点，`start()` 把自己的地址交给 loop。
- `run_loop::run()`：阻塞等待 work；出队后在锁外调用节点的完成函数；`close()` 后仍要 drain 已入队 work，队列空后退出。
- `run_loop_closed`：`close()` 之后再 `schedule()` 的 work 不应悄悄丢失，checker 期待它走 error channel。
- `detached_receiver`：用于 checker 直接 connect/start 后让 run loop drain。

## Parts

1. 定义 queue node。node 自带 `next` 指针和一个 `execute()`/函数指针入口。它由 operation state 自身继承或包含，不能分配临时节点后复制 receiver。
2. 写 FIFO。`push_back` 保持提交顺序，`pop_front` 返回原始 node 指针。checker 会用 `when_all` 和多个 `then` 验证顺序。
3. 写 `schedule_sender`。`connect(sender, receiver)` 返回 non-movable operation state；operation state 保存 receiver 和 loop 指针。
4. 写 `start()`。它只做入队和通知，不直接 inline 完成 receiver；bad 版本正是因为 completion 不在 loop 线程上而被拒绝。
5. 写 `run()`/`close()`。`run()` 必须由消费线程调用；所有 queued completion 都应在这条线程上执行。`close()` 唤醒 `run()`，但不抢走已排队任务。

## checker 覆盖

checker 不再相信实现提供的 `operation_state_address_stable()` 布尔函数，而是自己构造 operation state 并静态检查它不可移动，再运行真实排队流程。主要断言：

- 三个 scheduled sender 按 FIFO 得到 `{1,2,3}`。
- completion 运行在调用 `run()` 的线程上，而不是 `start()` 调用线程。
- operation state 类型不可移动，地址能被 intrusive queue 安全持有。
- `close()` 会 drain 已入队任务。
- `close()` 之后再 schedule 会产生 `run_loop_closed` error。

## Reference / good / bad

- `src/reference/solution.hpp` 是教学 Reference，实现真实 intrusive FIFO run loop。
- `validation/good/solution.hpp` 是独立 good，也实现自己的 FIFO loop，不依赖 Reference。
- `validation/bad/solution.hpp` 可编译，但错误地 inline 完成或跑错线程；负例诊断是 `completion runs on loop thread`。

## 直接命令

```powershell
cmake -S C10_Execution/exercises -B build/c10-h1 -DC10_UNITS="H1_run_loop" -DFETCHCONTENT_SOURCE_DIR_STDEXEC=$env:STDEXEC_ROOT -DC10_STUDY_BUILD_REFERENCE=ON
cmake --build build/c10-h1 --config Debug --target H1_run_loop_reference H1_run_loop_validation_good H1_run_loop_validation_bad H1_run_loop_student
ctest --test-dir build/c10-h1 -C Debug --output-on-failure
```

Student 初态应该能编译，并通过 `c10::unfinished` 返回 exit 2。bad 应该编译成功但被行为测试拒绝。
