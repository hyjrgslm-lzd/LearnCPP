# 03 schedulers: 执行位置、环境与切换

scheduler 是执行资源入口，不是线程对象。`exec::static_thread_pool pool(4); auto sch = pool.get_scheduler();` 得到的是可复制的调度能力。它可以生成很多 `schedule(sch)` sender；每个 sender 连接后才有自己的 operation state。

## 身份与位置

同一个 scheduler 发出的多个任务可能跑在不同 worker 上，也可能因为任务很短只观察到一个 worker。反过来，观察到一个 thread id 也不能证明 scheduler 就是线程。B4 的 checker 用 `task_count = 0, 1, 3, 8, 17` 多组输入验证：请求数必须全部完成，非零任务必须离开 caller thread，零任务不能发明 worker 观察，scheduler copy 还能创建后续 work。

这比“打印 4 个不同线程才算通过”更可靠。线程池后端、任务长度、系统负载都可能影响观察分布；课程不能把硬件调度偶然现象写成协议。

## B4 的可复现执行资源

B4默认使用N个已安装的 `exec::single_thread_context`，每个提供一个run_loop scheduler，任务按索引选择上下文。Reference逐项用schedule/then/sync_wait；独立Good批量connect/start，并把latch、互斥量与稳定操作状态保留到完成和上下文收束。这样仍真实创建请求数量的执行线程，也能验证scheduler副本继续派生工作。

固定版本 `static_thread_pool` 的四 worker 短生命周期场景曾在 WSL 复查中出现挂起；缺少用户态回溯时不能把现象进一步归因到某个内部算法。B4 默认采用稳定的 `single_thread_context` 组，仍能验证 scheduler 能力目标；其它单元的 `static_thread_pool` 只声明各自实际验证范围。调度能力概念不依赖某一个线程池实现，案例也不替上游库证明所有交错正确。

## `starts_on`

`starts_on(sch, sender)` 把这段 sender 的开始放到 scheduler 上。B5 的 parse 阶段用 parse scheduler 开始；拿到真实解析的 `ParsedRecord` 后，用 `let_value` 返回一段新的 `starts_on(compute_sch, ...)`，让 compute 阶段整体在另一个 scheduler 上开始。

B5 的输入是 `name,value,raw`。checker 用多组 fresh input 验证 `normalized = raw / 100` 和 `score = value * normalized`，因此 `parse_record` 不能返回固定样本。

## `continues_on`

`continues_on(sch)` 表示当前 sender 完成后，后续 continuation 切到另一个 scheduler。B5 的第二版先在 parse scheduler 解析，然后 `continues_on(compute_sch)`，后面的 compute `then` 在 compute scheduler 上继续。

`starts_on` 更像“这段新 work 在哪里开始”；`continues_on` 更像“从这个点以后在哪里继续”。两者都把执行位置写进图，而不是在 lambda 里偷偷丢线程。

## `on` roundtrip

固定 stdexec `nvhpc-26.05` 的 `on(scheduler, sender)` 在外层环境提供旧 scheduler 时，会把 child sender 放到目标 scheduler 上运行，再把 completion 切回旧 scheduler。B5 以三段 thread id 检查这个 roundtrip：外层在 outer pool，inner 在 compute pool，后续 continuation 回到 outer pool。value 使用解析出的 input value 再加一，证明 roundtrip 没丢 value channel。

## 有界流水线

B6 把 0 到上限条记录分给三条逻辑分支，分支内复制记录并执行 `parse -> enrich`，最后切到 merge scheduler 汇总。`Report` 带 stage thread id 和 `completed_batches`。checker 注入 enrich hook，要求 hook 在 recorded enrich 资源上执行，并要求 parse/enrich thread 集合互不重叠，因此单 scheduler 假完成体会被拒绝。

B6 的返回值是一次性同步收束边界：`sync_wait` 返回后，已接受三批工作都完成，局部借用状态可以销毁。它不实现通用 close/reject，也不证明 shutdown 后拒绝新 work 的完整 run_loop 协议；H1 run_loop 和 P1 pipeline 会主讲 close、reject、drain 和 shutdown。
