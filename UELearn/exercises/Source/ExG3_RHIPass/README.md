> 对应章节: ../../../09-模块G-RHI与着色器.md §练习 G-3

## 目标

用传统 `FRHICommandList` 路径手写一个最小全屏 draw pass（vertex shader + pixel shader + PSO + render target set + draw call），刻意体会三大手工负担（barrier 管理、PSO 设置顺序、resource 生命周期），把这些"痛"形式化成一张表，为理解模块 H 的 RDG 动机建立切身体验。

## 前置理解

- 已完成 ExG2_GlobalShader（`FGlobalShader` + `IMPLEMENT_GLOBAL_SHADER`）
- 了解 GPU 资源状态转换（resource state / barrier）基本概念：同一块内存在 GPU 不同阶段有不同访问模式，切换时需插入 barrier
- 了解 PSO（Pipeline State Object）：把 VS / PS / rasterizer / blend / depth-stencil state 打包成不可变对象
- 了解 `FGraphicsPipelineStateInitializer`：UE 对 PSO 描述信息的封装
- 源码：`Engine/Source/Runtime/RHI/Public/RHICommandList.h`（`BeginRenderPass`、`Transition`、`DrawPrimitive`）
- 源码：`Engine/Source/Runtime/RHI/Public/RHIStaticStates.h`（`TStaticDepthStencilState`、`TStaticBlendState`、`TStaticRasterizerState`）

## 必做任务

1. 取消注释 `ExG3_RHIPass.cpp` 中 TODO 1：用 `FRHITextureCreateDesc::Create2D` 创建 256×256 RGBA8 render target texture（flags：`RenderTargetable | ShaderResource`），存入模块级 `GExG3RenderTarget`。
2. 取消注释 TODO 2（手工负担 1）：插入 `RHICmdList.Transition(FRHITransitionInfo(..., ERHIAccess::Unknown, ERHIAccess::RTV))`，理解"必须手动知道当前状态"的负担。
3. 取消注释 TODO 3（手工负担 2）：按顺序设置 PSO——`ApplyCachedRenderTargets` 先于 `SetGraphicsPipelineState`，填写 `FGraphicsPipelineStateInitializer`，调用 `RHICmdList.DrawPrimitive(0, 1, 1)`。
4. 取消注释 TODO 4（手工负担 3）：pass 结束后插入 `Transition(..., ERHIAccess::RTV, ERHIAccess::SRVGraphics)`。
5. 填写"手工负担总结表"（见 cpp 文件末尾注释），用自己的话描述每项手工负担的具体位置、遗漏后果、以及 RDG 如何自动化。这张表将在模块 H 第一道练习中直接复用。

## 进阶任务

- 故意省略步骤 2 中的第一个 barrier（Unknown→RTV），用 `-d3ddebug` 或 `-vulkandebug` 启动参数运行，记录验证层报错信息——理解"barrier 遗漏是静默错误还是立即崩溃"在不同平台的差异。
- 在 `SetGraphicsPipelineState` 之前打印 `GetTypeHash(PSOInit)`（或用 `stat PipelineStateCache` console 命令），观察 PSO cache hit/miss 率——理解为何需要缓存 PSO 而不是每帧重建。
- 在 `ExG3.usf` 的 `MainVS` 中实现全屏三角形（SV_VertexID 方案），无需 vertex buffer，专注于 pipeline state 和 barrier 练习。

## 验收点

- [ ] Draw pass 在 `-d3ddebug` 模式下无红色验证层报错
- [ ] 手工负担总结表已填完，内容具体（非模板复制粘贴）
- [ ] 能用一句话解释"为什么 PSO 不能每帧重建"
- [ ] 进阶：故意省略 barrier 后能记录到具体的验证层报错信息

## 观察点

- `TStaticDepthStencilState<false, CF_Always>::GetRHI()` 等静态 RHI 状态通过模板参数在编译期区分状态，运行期只创建一次并缓存——这是 UE 避免每帧重建渲染状态的另一层次优化，与 PSO cache 互补。
- `FGraphicsPipelineStateInitializer` 是值语义：填入 VS/PS/state 描述后调用 `SetGraphicsPipelineState`，内部通过 hash 查 cache；cache miss 时驱动层编译 PSO（开销大），cache hit 时直接绑定已有 PSO（开销小）。
- `BeginRenderPass`/`EndRenderPass` 必须配对；两者之间的命令假设 render target 处于 RTV 状态；结束后需要额外 barrier 才能作为 SRV 使用。

## 常见坑

- **`ApplyCachedRenderTargets` 必须先于填写其他 PSO 字段**：否则 PSO 的 render target format 字段为 unknown，每帧重建 PSO。
- **Shader parameter 设置必须在 `SetGraphicsPipelineState` 之后**：PSO 绑定决定 root signature（D3D12）或 pipeline layout（Vulkan），参数绑定槽位需在 PSO 已知时才能正确映射。
- **Barrier 状态跟踪**：Debug 模式下 UE RHI 层验证"当前状态"是否与实际匹配；Release 模式下不验证，错误的状态说明被忽略但 GPU 行为未定义。

## 提示

- 全屏三角形推荐用 SV_VertexID 方案（不需要 vertex buffer）：`uint VtxId 0→(-1,-1)`, `1→(3,-1)`, `2→(-1,3)`，仅需 `DrawPrimitive(0, 1, 1)`。
- `GFilterVertexDeclaration.VertexDeclarationRHI` 是引擎内置全屏四边形顶点格式（需 `ScreenRendering.h`，来自 `RenderCore` 模块）。

## 手工负担总结表（完成后填入）

| 手工负担 | 本题具体位置 | 遗漏后果 | RDG 如何自动化 |
|---|---|---|---|
| Barrier 管理 | TODO 2（Unknown→RTV）、TODO 4（RTV→SRV） | GPU 验证错误 / 数据竞争 | Execute 期分析 pass 读写声明，自动插入 barrier |
| PSO 设置顺序 | TODO 3 中 `ApplyCachedRenderTargets` 必须先于 `SetGraphicsPipelineState` | PSO 不稳定，每帧重建 | pass 描述期声明 RT 绑定，Execute 期按顺序自动设置 |
| Resource 生命周期 | `GExG3RenderTarget` 必须在 pass 执行期间保持引用计数 > 0 | use-after-free | RDG transient pool 把资源生命周期与 pass 生命周期绑定，pass 结束自动回收 |

> 这张表就是"模块 H 的第一道练习为什么要先回顾 G3"的原因。

## 复盘问题

1. 真正开始执行的时刻？（`RHICmdList.BeginRenderPass` 是命令录制（RenderThread）；实际 GPU 执行在 RHIThread 提交 CommandList 给驱动之后。两者之间存在约一帧的延迟。）
2. 谁负责这对象的生命周期？（`FTextureRHIRef` 由 `TRefCountPtr` 管理；PSO 对象由 `PipelineStateCache` 的 hash map 持有；`FGraphicsPipelineStateInitializer` 是栈上值。）
3. 涉及哪些 Named Thread？（GameThread 投递；RenderThread 录制（BeginRenderPass/Transition/DrawPrimitive）；RHIThread 提交并执行。G3 是本模块唯一覆盖三条 Named Thread 的练习。）
4. 这对 GC 如何可见？（传统 RHI draw pass 全程不涉及 UObject；若数据来源于 UStaticMeshComponent，需在 GameThread 提取数据时通过 UPROPERTY/TObjectPtr 确保 GC 可见，然后按值捕获进入 lambda。）
5. [本题专属] 对照模块 H 的 RDG：你手写的哪三行代码对应 RDG 的哪三个自动行为？（对照手工负担总结表回答）

## 对应官方参考

- `Engine/Source/Runtime/RHI/Public/RHICommandList.h`（`BeginRenderPass`、`EndRenderPass`、`Transition`、`DrawPrimitive`）
- `Engine/Source/Runtime/RHI/Public/RHIStaticStates.h`（`TStaticDepthStencilState`、`TStaticBlendState`、`TStaticRasterizerState`）
- `Engine/Source/Runtime/RenderCore/Public/PipelineCacheUtilities.h`（`FPipelineCacheFileFormatPSO`，PSO 预热缓存格式）
- 官方文档：[Render Dependency Graph](https://dev.epicgames.com/documentation/en-us/unreal-engine/render-dependency-graph-in-unreal-engine)（G3 结束后立即阅读，感受 RDG 动机）
