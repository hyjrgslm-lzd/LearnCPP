> 对应章节: ../../../08-模块F-并发与任务图.md §练习 F-1

## 目标

派生 `FRunnable`，用 `FRunnableThread::Create` 启动手工管理线程，用 `FEventRef`（RAII 包装的 `FEvent`）实现主线程与子线程之间的双向握手，亲身观察 GameThread ID 与 WorkerThread ID 的差异，把 UE 最底层并发原语与 C++ `std::thread` + `std::condition_variable` 做一对一映射。

## 前置理解

- 已读 `01-心智模型.md` §3 线程分线：GameThread / RenderThread / RHIThread 角色划分
- 理解 `FRunnable` 四函数语义：`Init()` 在新线程、`Run()` 主执行体、`Stop()` 外部通知退出、`Exit()` 清理
- 理解 `FEventRef` RAII：构造时 `GetSynchEventFromPool`，析构时 `ReturnSynchEventToPool`
- 源码：`Engine/Source/Runtime/Core/Public/HAL/Runnable.h`（`FRunnable` 类）
- 源码：`Engine/Source/Runtime/Core/Public/HAL/Event.h` 第 129 行（`FEventRef`）
- 源码：`Engine/Source/Runtime/Core/Public/HAL/RunnableThread.h` 第 44 行（`Create` 工厂）

## 必做任务

1. 将 `FExF1Worker` 中的 `bool bShouldStop` 替换为 `FThreadSafeBool bShouldStop`（`#include "HAL/ThreadSafeBool.h"`），在 `Stop()` 里置 `true`，在 `Run()` 循环里每轮检查，实现提前退出。
2. 用 `FRunnableThread::Create` 创建 worker 线程，传入正确的 `FExF1Worker*` 和线程名称，验证 `Init()` 确实在新线程上调用（打印 ThreadId 确认）。
3. 验证双向握手：ReadyEvent 和 DoneEvent 各触发一次，主线程两次 `Wait()` 均能正确返回。
4. 验证正确销毁顺序：`Kill(true)` 先于 `delete Thread`；故意颠倒顺序，记录崩溃现象。
5. 画**线程时序图**（纸/文档均可）：横轴时间，纵轴 GameThread / WorkerThread，标出 `Create`、两次 `Trigger/Wait`、`Kill` 点。

## 进阶任务

- 用 `FThreadSafeCounter` 记录 `Run()` 已处理的迭代数，主线程在等 DoneEvent 之前轮询进度并打印。
- 写等价的 `std::thread` + `std::atomic<bool>` + `std::condition_variable` 版本，逐行对比 UE API 与标准库 API 的映射关系，写入观察记录。
- 给 `FRunnableThread::Create` 传不同的 `EThreadPriority`（`TPri_AboveNormal` / `TPri_BelowNormal`），用 Unreal Insights 观察优先级变化。

## 验收点

- [ ] 控制台输出两个不同的 ThreadId（GameThread vs WorkerThread）
- [ ] 两次 FEvent 握手均正确工作，无死锁、无竞争
- [ ] `Kill(true)` 后 `delete Thread` 无崩溃；颠倒顺序能观察到崩溃或 UAF
- [ ] 进阶：`FThreadSafeBool` 提前退出能在指定迭代次数内停止
- [ ] 线程时序图完成，握手点清晰标出

## 观察点

- `FRunnable::Init()` 在**新线程**的上下文里调用，不是在 `FRunnableThread::Create` 所在线程——这与 `std::thread` 行为一致，传入的 callable 直接在新线程执行。
- `FEventRef` 的 AutoReset 模式（`EEventMode::AutoReset`）在一个等待线程被唤醒后自动复位——ManualReset 模式则持续触发，需手动 `Reset()`。
- `FRunnableThread::GetThreadID()` 与 `FPlatformTLS::GetCurrentThreadId()` 在新线程内部获取的值相同，可用来确认"代码真的在那条线程上跑"。

## 常见坑

- **Kill 与 delete 顺序颠倒**：先 `delete Thread` 再 `Kill` = UAF，OS 线程仍在运行但对象已释放。永远：先 `Kill(true)`，再 `delete`。
- **FEventRef 不可移动/复制**：不能把 `FEventRef` 存入容器或赋值给另一个 `FEventRef`，需传裸指针 `FEvent*` 给 worker。
- **Stop() 误解**：`Stop()` 通知退出标志，不强制终止。若 `Run()` 不检查标志，`Kill(true)` 会无限阻塞。

## 提示

- `FEventRef` 的 `Get()` 方法返回内部裸 `FEvent*`，传给 worker 时使用此指针。
- `FPlatformTLS::GetCurrentThreadId()` 在 Windows 上返回 `GetCurrentThreadId()` 的值。

## 复盘问题

1. 真正开始执行的时刻？（`FRunnable::Init()` 和 `Run()` 分别在哪条线程？`Create` 调用和 `Init()` 执行有何先后关系？）
2. 谁负责这对象的生命周期？（`FRunnable` 由谁管？`FRunnableThread` 由谁管？析构顺序约束？）
3. 涉及哪些 Named Thread？（F1 创建的线程是 `ENamedThreads` 体系里的 Named Thread 吗？）
4. 这对 GC 如何可见？（若 worker 需引用 UObject，最安全的传递方式是什么？）
5. [本题专属] `FEventRef` 的 AutoReset 与 ManualReset 在多个等待线程场景下行为有何不同？

## 对应官方参考

- `Engine/Source/Runtime/Core/Public/HAL/Runnable.h`（`FRunnable` 接口，Init/Run/Stop/Exit）
- `Engine/Source/Runtime/Core/Public/HAL/RunnableThread.h`（`FRunnableThread::Create` 第 44 行）
- `Engine/Source/Runtime/Core/Public/HAL/Event.h`（`FEvent` 第 20 行，`FEventRef` 第 129 行）
- `Engine/Source/Runtime/Core/Public/HAL/ThreadSafeBool.h`（`FThreadSafeBool`）
- `Engine/Source/Runtime/Core/Public/HAL/ThreadSafeCounter.h`（`FThreadSafeCounter`）
