> 对应章节: ../../../08-模块F-并发与任务图.md §练习 F-4

## 目标

写出规范的 `ENQUEUE_RENDER_COMMAND` 代码，把"render command lambda 必须按值捕获"从知识点变成直觉；理解 render command queue 的 SPSC 模型；通过故意错误示范（引用捕获）与 `FlushRenderingCommands` 对比实验，感受悬垂引用的后果。F4 是通往模块 G（RHI 与着色器）的直接前置——模块 G 里所有 RHI 操作都通过 `ENQUEUE_RENDER_COMMAND` 从 GameThread 发起。

## 前置理解

- 已完成 ExF3_UETasks
- 理解 `ENQUEUE_RENDER_COMMAND` 宏展开：`DECLARE_RENDER_COMMAND_TAG` + `FRenderCommandDispatcher::Enqueue`（`RenderingThread.h` 第 1167 行）
- 理解"GameThread 写入命令（生产者）→ RenderThread 消耗命令（消费者）"的 SPSC 模型
- 理解 **RenderThread 通常滞后 GameThread 1 帧**，`ENQUEUE_RENDER_COMMAND` 后 lambda 不会立即执行

## 必做任务

1. 在 `StartupModule` 里，用正确的值捕获方式 `ENQUEUE_RENDER_COMMAND` 一个打印命令，观察：两条日志（GameThread 端 "Enqueued" + RenderThread 端 "执行"）的 ThreadId 是否不同，哪条先打印。
2. 实现"故意错误示范"注释块，在代码注释中清晰说明为何引用捕获 = 悬垂指针（栈帧已返回，lambda 在 RenderThread 执行时读到垃圾值）。
3. `FlushRenderingCommands` 对比实验：版本 A（不 Flush）vs 版本 B（Flush），观察打印时机和帧卡顿差异，在观察记录里写出"为何 Flush 在 Tick 里禁止但在模块卸载时合法"。
4. 画**线程时序图**（必做）：横轴时间，纵轴 GameThread / RenderThread，标出 Enqueue 点、RenderThread 消耗点、时间差（1 帧延迟）。

## 进阶任务

- 在 `TickComponent` 里每帧 Enqueue 一个命令，每个命令打印被 Enqueue 时的帧号与被执行时的时间戳，观察帧号与时间戳之间的延迟规律。
- 值捕获 `TSharedPtr<FMyData>`（非 UObject）进 render command lambda，观察 lambda 何时析构（即 GameThread 侧的原始 `TSharedPtr` 引用计数何时降到可析构的值）——确认 lambda 生命周期结束于 RenderThread 执行完该命令之后。
- 研究 `DECLARE_RENDER_COMMAND_TAG` 宏（第 240 行）展开产生的类型，以及 `FRenderCommandDispatcher::Enqueue<Tag>` 如何携带 stat 数据。

## 验收点

- [ ] 两条日志的 ThreadId 不同（`GIsThreadedRendering=true` 时）
- [ ] "[GameThread] Enqueued" 先于 "[RenderThread]" 出现
- [ ] FlushRenderingCommands 版本 B 能感知明显等待延迟
- [ ] 能背出：render command lambda 必须按值捕获，因为构造时 GameThread 栈帧执行完毕后局部变量已析构

## 观察点

- `ENQUEUE_RENDER_COMMAND` 的 lambda 被包装成任务放入 UE::Tasks::FPipe（render command pipe），"投递"在 GameThread；"执行"在 RenderThread 的 tick 里——两者之间可能跨越 0~2 帧。
- `FlushRenderingCommands()` 只等待 CPU 侧 RenderThread 排空命令队列，**不等待 GPU 完成**。等 GPU 完成需要 `FRHICommandListImmediate::BlockUntilGPUIdle()` 或 fence 机制（模块 G 内容）。
- SPSC 模型：GameThread 是唯一生产者，RenderThread 是唯一消费者，无需加锁，通过无锁 FIFO 实现高效跨线程通信。

## 常见坑

- **按引用捕获 GameThread 局部变量**：最高频线程安全 bug。栈帧返回后局部变量已析构，lambda 在 RenderThread 执行时读到悬空内存。永远按值捕获。
- **按值捕获裸 `UObject*`**：GC 不知道 lambda 持有此指针，可能在 GameThread 帧间回收该对象，lambda 执行时 UAF。正确做法：提前在 GameThread 提取 POD 数据值捕获，或用带引用计数的 RHI 资源句柄（`TRefCountPtr`）。
- **在 render command lambda 里再次 ENQUEUE**：设计为从 GameThread 调用，RenderThread 侧嵌套 ENQUEUE 行为未定义（可能死锁或命令顺序错乱）。

## 提示

- `ENQUEUE_RENDER_COMMAND` 宏后紧跟 lambda（不是函数调用括号），格式：
  ```cpp
  ENQUEUE_RENDER_COMMAND(TypeName)(
      [ValueCopy](FRHICommandListImmediate& RHICmdList) { ... }
  );
  ```
- `GIsThreadedRendering` 为 false 时（单线程渲染模式），lambda 在 GameThread 同步执行——这是 `ShouldExecuteOnRenderThread()` 宏的语义（`RenderingThread.h` 第 154 行）。

## 复盘问题

1. 真正开始执行的时刻？（a）`ENQUEUE_RENDER_COMMAND` 那行执行时？（b）当前 GameThread Tick 返回后？（c）RenderThread 排到此命令时？正确答案及其余两个为何不对？
2. 谁负责这对象的生命周期？（render command lambda 对象何时析构？捕获的值何时被释放？）
3. 涉及哪些 Named Thread？（Enqueue 在哪条线程？lambda 在哪条线程？`FRHICommandListImmediate` 命令最终由哪条线程提交给 GPU 驱动？）
4. 这对 GC 如何可见？（lambda 闭包内存 GC 不可扫描——对使用 UObject 资源有什么严格限制？两种合规解决方案？）
5. [本题专属] `ENQUEUE_RENDER_COMMAND` 与 `FFunctionGraphTask::CreateAndDispatchWhenReady(..., ActualRenderingThread)` 的区别：顺序保证、stat 支持、profiling 可见性三个维度分别如何？

## 对应官方参考

- `Engine/Source/Runtime/RenderCore/Public/RenderingThread.h`（`ENQUEUE_RENDER_COMMAND` 第 1167 行；`FlushRenderingCommands` 第 110 行；`ShouldExecuteOnRenderThread` 第 154 行；`DECLARE_RENDER_COMMAND_TAG` 第 240 行）
- `Engine/Source/Runtime/Core/Public/Tasks/Pipe.h`（render command pipe 底层实现基础）
