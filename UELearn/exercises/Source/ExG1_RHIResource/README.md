> 对应章节: ../../../09-模块G-RHI与着色器.md §练习 G-1

## 目标

在 RenderThread 中使用 `FRHICommandListImmediate` 和现代 `FRHIBufferCreateDesc` / `FRHITextureCreateDesc` API 创建 vertex buffer 与 texture；理解 RHI 资源的引用计数生命周期（`TRefCountPtr` → 删除队列 → 批量回收）；通过 `GDynamicRHI->GetName()` 直观感受 UE RHI 平台抽象层的分层结构。

## 前置理解

- 已完成 ExF4_RenderCmd（`ENQUEUE_RENDER_COMMAND` 值捕获语义）
- 理解 `TRefCountPtr<T>`：`AddRef`/`Release` 是原子操作；引用计数归零后进入删除队列（`MarkForDelete()`），在下一帧帧尾由 `FRHICommandListExecutor` 批量回收，**不是直接 `delete`**
- 了解 `FRHIResourceCreateInfo` 在 UE 5.6 已标记 deprecated，应使用 `FRHIBufferCreateDesc` / `FRHITextureCreateDesc`
- 源码：`Engine/Source/Runtime/RHI/Public/RHIResources.h`（`FRHIResource` 第 53 行，`FRHIBufferCreateDesc` 第 1416 行）
- 源码：`Engine/Source/Runtime/RHI/Public/RHICommandList.h`（`CreateBuffer` 第 800 行）

## 必做任务

1. 在 `ENQUEUE_RENDER_COMMAND` lambda 里，用 `FRHIBufferCreateDesc::CreateVertex(name, size)` 描述并用 `RHICmdList.CreateBuffer(desc)` 创建大小为 `3 * sizeof(FVector3f)` 的 vertex buffer，存入模块级 `FBufferRHIRef GExG1VertexBuffer`。
2. 同样在 lambda 里，用 `FRHITextureCreateDesc::Create2D(name, 64, 64, PF_R8G8B8A8)` 描述并用 `RHICmdList.CreateTexture(desc)` 创建 64×64 RGBA8 texture，存入 `FTextureRHIRef GExG1Texture`。
3. 在 lambda 末尾打印 `GExG1VertexBuffer->GetRefCount()` 和 `GDynamicRHI->GetName()`（观察当前 RHI 后端：D3D12 / Vulkan）。
4. 在 `ShutdownModule` 里先 `FlushRenderingCommands()`，再将两个 `FXxxRHIRef` 置为 `nullptr`，观察资源何时真正回收（非立即——进入删除队列）。

## 进阶任务

- 尝试在 GameThread 直接调用 `GDynamicRHI->RHICreateBuffer(...)` 绕过 CommandList，观察是否触发 "not on render thread" 类断言。
- 阅读 `FRHIResource::Release`（`RHIResources.h` 第 80–91 行），理解"引用计数归零 → `MarkForDelete()` → 批量回收"的流程，对比 `TSharedPtr` 的"引用计数归零 → 直接析构"。
- 阅读 `ERHIThreadMode`（`RHICommandList.h` 第 99–104 行）：`DedicatedThread` vs `Tasks` 两种 RHIThread 模式。

## 验收点

- [ ] `FBufferRHIRef` 在 lambda 内引用计数为 1（只有模块级变量持有）
- [ ] `GDynamicRHI->GetName()` 成功打印当前 RHI 后端名称
- [ ] 代码中没有手动 `delete` 任何 `FRHIResource` 子类实例
- [ ] 能用一句话解释"为什么不能在 GameThread 直接 `new FRHIBuffer`"

## 观察点

- `FRHIResource` 引用计数（`AtomicFlags`）是原子操作，`AddRef`/`Release` 线程安全；但回收路径与 `TSharedPtr` 完全不同——`TSharedPtr` 归零直接析构，`FRHIResource` 归零进删除队列。
- `FBufferRHIRef` 是 `TRefCountPtr<FRHIBuffer>` 的 typedef；`FTextureRHIRef` 是 `TRefCountPtr<FRHITexture>` 的 typedef。析构时调用 `Release()`，不是 `delete`。
- `FRHICommandListImmediate` 继承自 `FRHICommandList`。"Immediate" 的含义是"当前帧可以立即提交"，但在 `r.RHIThread.Enable=1` 时录制仍在 RenderThread，提交在 RHIThread。

## 常见坑

- **在 GameThread 对 `FRHIResource` 调用非 const 方法**：打破 RHI 线程安全约定，通常触发断言或未定义行为。
- **`FBufferRHIRef` 置 nullptr 后立即复用**：资源进入删除队列，不会立即回收；同帧再次访问 = use-after-free。
- **值捕获原则**：捕获 `FBufferRHIRef` 进 lambda 必须按值捕获（与 F4 原则完全相同）。

## 提示

- `FRHIBufferCreateDesc::CreateVertex` 有两个重载：`CreateVertex(name)` 不指定大小（后续用 `SetSize`），`CreateVertex(name, size)` 直接指定大小。推荐后者。
- 打印 `GetRefCount()` 要在 `UE_LOG` 里，不要用 `printf`（RenderThread 上 printf 线程安全性不保证）。

## 复盘问题

1. 真正开始执行的时刻？（GPU 资源创建在 `FDynamicRHI::RHICreateBuffer` 返回时完成——仍在 CPU 侧；GPU 实际读写更晚——在 CommandList Submit 阶段。）
2. 谁负责这对象的生命周期？（`FRHIBuffer`/`FRHITexture` 由 `FRHIResource` 引用计数管理，回收通过删除队列而非 RAII 直接析构。）
3. 涉及哪些 Named Thread？（`ENQUEUE_RENDER_COMMAND` lambda 在 RenderThread 执行；`RHICreateBuffer` 在 RenderThread 发出命令，在 RHIThread（若启用）最终提交到驱动。）
4. 这对 GC 如何可见？（`FRHIBuffer`/`FRHITexture` 是纯 C++ 对象，用 `TRefCountPtr` 管理，与 GC 完全分离。）
5. [本题专属] `FRHICommandListImmediate` 的"Immediate"究竟意味着什么？它与 `FRHICommandList` 的区别是什么？

## 对应官方参考

- `Engine/Source/Runtime/RHI/Public/RHIResources.h`（`FRHIResource` 第 53 行，`FRHIBuffer`，`FRHITexture`，`FRHIBufferCreateDesc` 第 1416 行）
- `Engine/Source/Runtime/RHI/Public/RHICommandList.h`（`FRHICommandListImmediate`，`CreateBuffer` 第 800 行，`ERHIThreadMode` 第 99 行）
- `Engine/Source/Runtime/RHI/Public/DynamicRHI.h`（`FDynamicRHI` 虚函数表）
