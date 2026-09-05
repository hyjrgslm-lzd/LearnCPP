# G-2 实现 when_all<T...>

对应文档：`09-模块G-symmetric_transfer与高级task.md` 「练习 G-2」。

## 目标

实现 `when_all`：N 个子 task 先全部启动，再统一等待完成；最后一个完成
的子 task 唤醒等待者，用 `std::tuple` 汇合结果。错误处理采用 **fail-delay** 策略——
所有子 task 都跑完，若有任一异常就传播第一个。

## 必做任务

1. 主任务：实现可编译的 2-task 固定版 `when_all_2<T0, T1>`。骨架已给出，验证它能：
   - 成功汇合：`when_all_2(fetch_int(), fetch_string())` 返回 `tuple<int, string>`。
   - fail-delay 错误：第一个子 task 抛异常时，第二个子 task 仍跑完，最后传播第一个异常。
2. 在笔记里画出 when_all 的状态转换图：
   `remaining=N → 每个分支完成时 -1 → remaining=0 → waiter.resume() → await_resume 返回 tuple`。
3. 列出三种错误合并策略（fail-fast / fail-delay / aggregation）各自适合的场景。

## 进阶任务

- 用 `std::index_sequence_for<Tasks...>` 把索引 I 带入 fold expression，实现变参版本。
  骨架中给出了"设计示意——不可直接编译"的伪代码，用它指导实现。
- 实现 `when_any`：第一个完成的子 task 唤醒等待者，其余被取消。
- 把 run_loop/manual_event 版改为多线程并行（提交到线程池），观察 `remaining_` 必须用 atomic 的原因。

## 验收点

- `when_all_2(int, string)` 能正确汇合两个不同类型的结果。
- 任一子 task 失败时，错误被正确传播（fail-delay 策略）。
- 你能解释为什么 `remaining_` 在并行版本中必须是 `std::atomic<int>`。
- 你能画出计数从 N 递减到 0 的每一步含义。

## 提示

- 逐个 `drain` 只能证明顺序收束，是 starter 待改造起点，不能证明 `when_all` 并发；真正 fan-out 需要先启动所有分支，再统一等待完成。
- 真正的并行版需要 atomic 计数 + 多线程调度基础设施。
- `std::apply` + fold expression 是变参并行启动的简洁方式。

## 本轮练习契约

Starter 中的顺序 drain 是待改造起点；任务要求实现真正 fan-out/fan-in。Reference 用 `manual_event` 断言两个分支先全部启动、事件触发前都未完成、最后一个分支完成后恢复父协程，并验证异常在全部分支收束后传播；它不依赖 async 耗时测试。

命令：``cmake -S . -B build/dg-lane -DCOROUTINE_STUDY_BUILD_REFERENCE=ON``，然后构建 ``G2_when_all_impl`` 与 ``G2_when_all_impl_reference``，再用 ``ctest -R G2_when_all_impl_reference`` 跑稳定验收。
