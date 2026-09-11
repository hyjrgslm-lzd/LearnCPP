# B4 scheduler capability

`scheduler` 不是线程对象。它是“把一段 work 安排到某类执行资源”的能力句柄。执行资源可以是 thread pool，也可以是由若干 `exec::single_thread_context` 组成的受控环境；`scheduler` 可复制；`schedule(sch)` 构造 sender；`connect/start` 后才有一次 operation。

## Part 1：有界输入

`run_scheduler_probe(thread_count, task_count)` 必须验证输入：线程数范围是 `[1, 64]`，任务数范围是 `[0, 1024]`。本题 checker 会用 `task_count = 0, 1, 3, 8, 17` 多组 fresh count，并检查上下界拒绝，不能把 8 写死。

## Part 2：调度任务

用 `schedule(sch)` 建立调度边界。Reference用组合与sync_wait逐项执行；独立Good为每个work item保存operation state，批量start后收束。完成后累计 `completed_tasks` 并记录 worker thread id。`task_count == 0` 时不能发明 worker 观察；`task_count > 0` 时 work 必须离开 caller thread。

## Part 3：scheduler copy

复制 scheduler 后继续 `schedule(copy)`。这证明 scheduler 是可复制调度能力，不是一次性 operation state。

## 解析

对象关系是 `pool -> scheduler -> sender -> operation_state`。sender 只是描述，operation state 才是一次已连接运行。后续 `starts_on/continues_on/on` 都是在图里放置这类调度边界。

本题选择 `single_thread_context` 组，是为了保留 scheduler 能力目标，同时避开固定 `static_thread_pool` 在短生命周期场景下的已知不稳定边界。这里不宣称已定位或修复上游线程池的全部内部问题。
