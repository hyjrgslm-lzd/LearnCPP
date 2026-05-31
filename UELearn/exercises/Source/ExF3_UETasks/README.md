> 对应章节: ../../../08-模块F-并发与任务图.md §练习 F-3

## 目标

用 UE5 引入的 `UE::Tasks` 高层 API 完成三个并发模式：用带 Prerequisites 的 `Launch` 重载实现 Then 链（三阶段数据流水线）；用 `FPipe` 实现无锁串行化保护共享计数器；对照 stdexec 的对等概念，写出 4 行映射表，深刻理解"eager 投递"与"惰性 sender 描述"的本质差异。

## 前置理解

- 已完成 ExF2_TaskGraph（理解 TaskGraph worker pool）
- 理解 `UE::Tasks::Launch` 是 **eager**（立即投递），不是 stdexec 的惰性 sender
- 理解 `FPipe` 通过隐式前置条件实现 FIFO 串行，不是独立串行线程
- 源码：`Engine/Source/Runtime/Core/Public/Tasks/Task.h`（`TTask<T>`，`Launch` 第 266 行）
- 源码：`Engine/Source/Runtime/Core/Public/Tasks/Pipe.h`（`FPipe` 第 29 行，`WaitUntilEmpty` 第 62 行）

## 必做任务

1. 完成三阶段 Then 链（Stage1 → Stage2 → Stage3）：Stage1 返回 `TArray<int32>`，Stage2 对每个元素平方，Stage3 求和返回 `int64`。打印三个阶段各自的 ThreadId，观察是否在不同线程执行。
2. 用 `FPipe` 保护 `int64 SharedCounter`：创建 8 个并发任务，每个通过 `Pipe.Launch` 向 `SharedCounter` 累加 1000 次。验收：最终 `SharedCounter == 8000`，多次运行结果稳定（无数据竞争）。
3. 在 `FPipe` 析构前调用 `WaitUntilEmpty()`（否则 `check(!HasWork())` 崩溃）。
4. 在观察记录里填写 4 行对照表：`UE::Tasks::Launch` / `Then` / `FPipe::Launch` 各对应 stdexec 的哪个概念，并写出最关键差异（eager vs lazy）。
5. 画**线程时序图**：展示三阶段链在不同 worker 线程上的执行顺序及 Then 触发点。

## 进阶任务

- 用 `UE::Tasks::All` 等待 4 个并发素数计算任务（各自独立区间），汇总总素数个数并打印。
- 阅读 `TaskPrivate.h` 中 `FTaskBase::TryLaunch` 执行路径，理解任务从"创建"到"TaskGraph dispatch"的具体步骤，找到与 `FFunctionGraphTask` 汇合的底层路径。
- 测量 `FPipe::WaitUntilEmpty()` 延迟：向 pipe 投递 1000 个小任务，测量"最后一个任务投递"到"WaitUntilEmpty 返回"的耗时（用 `FPlatformTime::Cycles()`）。

## 验收点

- [ ] Stage3 求和结果数值正确（可预先手算）
- [ ] `FPipe` 保护的 `SharedCounter` 精确等于 8000，多次运行稳定
- [ ] `WaitUntilEmpty()` 在 `FPipe` 析构前调用，无 `check` 崩溃
- [ ] 对照表 4 行全部完成，能口头解释"Launch 不等于 just+schedule"

## 观察点

- `UE::Tasks::Launch` 立即投递（eager），任务一经 `Launch` 就进入调度队列，没有"描述期"概念——这是与 stdexec 最大语义差异。
- `FPipe` 的串行化通过隐式前置条件实现：新任务加入时自动将 pipe 里最后一个未完成任务设为前置条件，任务仍跑在 worker pool 上，只是同一时刻只有一个在跑。
- `TTask<T>::Wait()` 内部尝试在当前线程就地执行任务（retract and execute），减少真正的线程切换——与 stdexec `sync_wait` 的设计思路相似。

## 常见坑

- **FPipe 在 pipe 任务仍有工作时析构**：`check(!HasWork())` 触发崩溃。`WaitUntilEmpty()` 是必须调用的前置。
- **在 Then 里捕获 TTask 本身**：形成引用环，`FTaskBase` 永远无法释放。只在 lambda 里通过参数或值捕获计算结果，不捕获 `TTask` 对象。
- **在 worker task 里访问 UObject**：`UE::Tasks::Launch` 的 lambda 跑在 worker pool，不是 GameThread，调用 `GetWorld()` 等会触发 `IsInGameThread()` 断言。

## 提示

- Then 链实现：用带 Prerequisites 的 `Launch` 重载（`Task.h` 第 280 行），将前一阶段的 `TTask<T>` 作为 prerequisites 传入，在 lambda 内调用 `StageN.GetResult()` 获取结果。
- `FPipe::Launch` 签名（`Pipe.h` 第 62 行）：`Pipe.Launch(DebugName, lambda)` 即可，内部自动处理串行化。

## 复盘问题

1. 真正开始执行的时刻？（`UE::Tasks::Launch` 投递到"开始执行"经过哪些步骤？与 stdexec `start(op)` 的本质区别？）
2. 谁负责这对象的生命周期？（`TTask<T>` 值语义；底层 `FTaskBase` 由 ref count 管；Then lambda 值捕获 `TTask` 时 ref count 如何变化？）
3. 涉及哪些 Named Thread？（`UE::Tasks::Launch` 默认投递到哪类 Named Thread？能否直接投递到 `ENamedThreads::GameThread`？）
4. 这对 GC 如何可见？（`FPipe` 不是 UObject；pipe 任务体里 `TWeakObjectPtr::Pin()` 返回 nullptr 的条件？）
5. [本题专属] `FPipe` 的串行化与 mutex 保护共享变量的本质区别是什么？哪些场景下 FPipe 更合适？

## 对应官方参考

- `Engine/Source/Runtime/Core/Public/Tasks/Task.h`（`TTask<T>`，`Launch` 第 266 行，带 Prerequisites 重载第 280 行）
- `Engine/Source/Runtime/Core/Public/Tasks/Pipe.h`（`FPipe` 第 29 行，`WaitUntilEmpty` 第 42 行）
- `Engine/Source/Runtime/Core/Public/Tasks/TaskPrivate.h`（`FTaskBase`，`TryLaunch` 底层路径）
