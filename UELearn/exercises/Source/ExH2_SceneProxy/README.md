> 对应章节: ../../../10-模块H-RenderGraph与场景渲染.md §练习 H-2

## 目标

理解 UE 场景系统中 GameThread 与 RenderThread 的双对象镜像：`UPrimitiveComponent`（GC 管理）与 `FPrimitiveSceneProxy`（`FScene` 手动管理）。通过继承 `UPrimitiveComponent` 实现 `CreateSceneProxy()` 返回自定义 `FPrimitiveSceneProxy` 子类，观察创建与销毁的线程边界。

## 前置理解

- 已完成模块 K（`AActor` / `UActorComponent` 生命周期钩子）
- 已完成模块 F（`ENQUEUE_RENDER_COMMAND` 按值捕获语义）
- 知道 `UPrimitiveComponent` 是 `UActorComponent` 的子类

## 必做任务

1. 阅读 `FExH2SceneProxy` 构造函数，理解为何不持有 `UPrimitiveComponent*`（构造时按值复制所需数据）
2. 把 `UExH2PrimitiveComponent` 添加到 PIE 中的某个 Actor，观察 `CreateSceneProxy` 被调用的日志
3. 在 `GetViewRelevance` 中设置 `Result.bDynamicRelevance = true`，使 `GetDynamicMeshElements` 被触发
4. 向 `GetDynamicMeshElements` 的 `Collector` 提交一个最小 `FMeshBatch`（使用 `Collector.AllocateMesh()`）
5. 修改 `DebugLabel` 后调用 `MarkRenderStateDirty()`，观察 `CreateSceneProxy` 再次被调用

## 进阶任务

- 阅读 `PrimitiveSceneProxy.h` 第 1–80 行，找到 `GetViewRelevance` / `GetDynamicMeshElements` 两个虚函数的完整签名
- 阅读 `ScenePrivate.h` include 列表，找到 `FPrimitiveSceneInfo` 相对于 `FPrimitiveSceneProxy` 额外的场景注册字段
- 思考：如果直接在 GameThread 修改 Proxy 的成员会产生什么问题（data race）

## 验收点

- [ ] 能画出 GameThread/RenderThread 双对象镜像关系图（ASCII 即可）
- [ ] 能说出 `CreateSceneProxy()` 的调用时机：`RegisterComponent()` 触发，但对象只在 RenderThread 使用
- [ ] 能解释为什么 `FPrimitiveSceneProxy` 不继承 `UObject`：活在 RenderThread，与 GC 线程不兼容
- [ ] `GetDynamicMeshElements` 内没有使用 `new`/`delete`（使用 `Collector.AllocateMesh()`）
- [ ] PIE 中可在 Output Log 看到 GameThread / RenderThread 两侧的日志序列

## 观察点

- GameThread/RenderThread 双对象镜像是 UE 渲染架构的根基：GameThread 持有"意图"，RenderThread 持有"执行状态"
- `ENQUEUE_RENDER_COMMAND` 按值传入数据包的必要性在此题得到最清晰体现：GC 随时可能在 GameThread 运行，修改或销毁 UPrimitiveComponent
- `MarkRenderStateDirty()` 是 GameThread 通知引擎重建 Proxy 的标准接口，内部通过 `ENQUEUE_RENDER_COMMAND` 安全跨线程

## 常见坑

- **在 `FPrimitiveSceneProxy` 构造函数里存储 `UPrimitiveComponent*`**：Proxy 活在 RenderThread，GC 下 Component 随时可能被销毁，产生悬空指针；必须在构造时按值复制数据
- **直接在 GameThread 修改 `FPrimitiveSceneProxy` 的成员**：Proxy 可能同时在 RenderThread 被读取，产生 data race；应通过 `MarkRenderStateDirty()` 触发重建
- **在 `GetDynamicMeshElements` 里使用 `new`**：该函数每帧调用，`new` 造成频繁堆分配；应通过 `Collector.AllocateMesh()` 分配 `FMeshBatch`

## 提示

- 搜索引擎源码中 `CreateSceneProxy` 的实现（如 `UStaticMeshComponent::CreateSceneProxy`）可找到完整的 FMeshBatch 提交范例
- 用 UE Insights 的 CPU track 可观察 `CreateSceneProxy` 和 `GetDynamicMeshElements` 的线程归属

## 复盘问题（四固定问题 + 本题专属）

1. **执行真正开始时刻？** `FSceneRenderer::Render()` 在每帧 RenderThread 调用；`CreateSceneProxy()` 在 `RegisterComponent()` 后的首次渲染状态同步（GameThread）触发
2. **生命周期拥有者？** `FPrimitiveSceneProxy` 由 `FScene` 拥有（`AddPrimitive` 接管，`RemovePrimitive` 销毁）；`UPrimitiveComponent` 由 GC 通过 `UPROPERTY` + Outer 链管理
3. **涉及哪些 Named Thread？** `CreateSceneProxy()` 在 **GameThread** 调用；`GetDynamicMeshElements()` 在 **RenderThread** 调用
4. **涉及 UObject 时怎么对 GC 可见？** `UPrimitiveComponent` 是 UObject，由 `UPROPERTY` + Actor Outer 链保证 GC 可见；`FPrimitiveSceneProxy` 非 UObject，由 `FScene` 手动管理
5. **[本题专属]** `FPrimitiveSceneProxy` 为什么不继承 `UObject`？答：Proxy 活在 RenderThread，GC 在 GameThread 运行；跨线程持有 UObject* 会在 GC 下产生悬空指针风险，且 RenderThread 代码不应依赖 GC 扫描

## 对应官方参考

- `Engine/Source/Runtime/Engine/Public/PrimitiveSceneProxy.h`（`FPrimitiveSceneProxy` 类声明，`GetViewRelevance`，`GetDynamicMeshElements`）
- `Engine/Source/Runtime/Renderer/Private/ScenePrivate.h`（`FScene` 结构）
- `Engine/Source/Runtime/Engine/Classes/Components/PrimitiveComponent.h`（`CreateSceneProxy`，`MarkRenderStateDirty`）
