> 对应章节: ../../../08-模块F-并发与任务图.md §练习 F-2

## 目标

通过 `FFunctionGraphTask::CreateAndDispatchWhenReady` 把工作投递到 TaskGraph 的不同 Named Thread 和 worker pool，用 `FGraphEventRef` 建立 A → B → C 的显式依赖链，能说清 `ENamedThreads` 枚举里各个值的含义，理解"投递完成"与"执行开始"是两个不同时刻。

## 前置理解

- 已完成 ExF1_Runnable
- 理解 `ENamedThreads::Type` 是位域枚举：低 8 位线程索引，高位编码优先级和队列偏移
- 理解 `FGraphEventRef`（= `TRefCountPtr<FGraphEvent>`）是任务的"完成句柄"
- 源码：`Engine/Source/Runtime/Core/Public/Async/TaskGraphInterfaces.h`
  - `ENamedThreads` 第 54–208 行
  - `FTaskGraphInterface` 第 264–420 行
  - `FFunctionGraphTask` 第 1117–1135 行

## 必做任务

1. 用 `FFunctionGraphTask::CreateAndDispatchWhenReady` 建立 A → B → C 三任务依赖链：A 无前置（AnyNormalThread），B 前置 A（AnyHiPriThread），C 前置 B（GameThread）。每个 lambda 打印任务名、当前 ThreadId、`GetCurrentThreadIfKnown()` 值。
2. 用 `FTaskGraphInterface::Get().WaitUntilTaskCompletes(EventC, ENamedThreads::GameThread)` 等待链尾，验证 A→B→C 顺序依赖正确。
3. 额外创建一个投递到 `ENamedThreads::ActualRenderingThread` 的任务，打印其 ThreadId，与 GameThread ThreadId 对比（`GIsThreadedRendering=true` 时应不同）。
4. 在观察记录里写出 `ENamedThreads` 六类线程槽位的含义（GameThread / ActualRenderingThread / RHIThread / AnyNormal / AnyHiPri / AnyBackground）。
5. 画**线程时序图**：GameThread / HiPriWorker / NormalWorker 三轴，标出 A/B/C 投递与执行时间点及依赖箭头。

## 进阶任务

- 用 `TGraphTask<T>::CreateTask(Prerequisites).ConstructAndDispatchWhenReady(...)` 完整路径实现自定义任务类型（需实现 `GetDesiredThread()` + `DoTask()`）。
- 实验 `FGraphEventRef::DontCompleteUntil(other)` 延长任务完成时刻，体会与"作为前置条件"的语义差异。
- 阅读 `TaskGraphInterfaces.h` 第 264–350 行，理解 `QueueTask` 的 `ThreadToExecuteOn` vs `CurrentThreadIfKnown` 参数语义。

## 验收点

- [ ] A → B → C 顺序依赖正确，控制台日志按 A、B、C 顺序出现
- [ ] 各任务的 `GetCurrentThreadIfKnown()` 返回值与投递时指定的 `ENamedThreads` 一致
- [ ] ActualRenderingThread 任务的 ThreadId 在 `GIsThreadedRendering=true` 时与 GameThread 不同
- [ ] 能口头解释 `AnyNormalThreadNormalTask` 与 `GameThread` 在调度行为上的根本区别

## 观察点

- TaskGraph 里"Named Thread"不是 OS 意义的 named thread，而是调度器为特定角色保留的槽位。GameThread 的任务只有在 GameThread 运行 `ProcessThreadUntilIdle` 时才被消耗。
- `WaitUntilTaskCompletes` 在 GameThread 上等待时，内部会处理 GameThread 本地队列（防止"自我阻塞"死锁）。
- `FGraphEventRef` 的引用计数决定 `FGraphEvent` 对象生命周期；把它放入 `FGraphEventArray` 传给下一任务会暂时增加 ref count。

## 常见坑

- **投递到 GameThread 后忘记驱动排队**：`StartupModule` 里投递到 `GameThread` 的任务，GameThread 还没进入 main game loop，任务不会自动执行。需显式 `WaitUntilTaskCompletes`（内部驱动队列消耗）。
- **FGraphEventArray 持有悬空引用**：确保 `FGraphEventRef` 生命周期覆盖整个 `CreateAndDispatchWhenReady` 调用范围。
- **在 worker task 里调用 GameThread-only API**：`AActor`、`UActorComponent` 等方法内部有 `check(IsInGameThread())`，在 AnyThread lambda 里调用会崩溃。

## 提示

- 单个前置条件的重载（`TaskGraphInterfaces.h` 第 1146 行）：`CreateAndDispatchWhenReady(lambda, TStatId{}, EventA, thread)` 直接传 `FGraphEventRef` 即可，不需要手动构造 `FGraphEventArray`。

## 复盘问题

1. 真正开始执行的时刻？（TaskGraph 任务"投递完成"和"执行开始"是两个不同时刻，这对 StartupModule 里的代码有什么影响？）
2. 谁负责这对象的生命周期？（`TGraphTask<T>` 的内存由谁分配、释放？`FGraphEventRef` 引用计数何时归零？）
3. 涉及哪些 Named Thread？（列出四类：GameThread / ActualRenderingThread / RHIThread / AnyThread 池，说明各自适合投递什么工作）
4. 这对 GC 如何可见？（worker task lambda 捕获 `UObject*` 的风险，用 `TWeakObjectPtr` 安全引用的代码框架）
5. [本题专属] `ENamedThreads::GameThread_Local` 与 `ENamedThreads::GameThread` 的区别？什么场景下用 `_Local` 变种？

## 对应官方参考

- `Engine/Source/Runtime/Core/Public/Async/TaskGraphInterfaces.h`（ENamedThreads 第 54–208 行；FTaskGraphInterface 第 264–420 行；FFunctionGraphTask 第 1117–1135 行）
- `Engine/Source/Runtime/Core/Public/Async/Async.h`（高层 `Async()` 帮助函数）
