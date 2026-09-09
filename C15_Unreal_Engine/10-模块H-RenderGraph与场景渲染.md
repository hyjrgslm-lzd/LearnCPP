# 10 模块 H：RenderGraph 与场景渲染（压轴）

## 模块目标

反向引用 `01-心智模型` 的 §5 **RDG = lazy graph**。

本模块是整门课程的压轴收束点。在模块 G 你已经用 `FRHICommandList` 以"命令录制"方式直接向 GPU 提交命令，亲身感受了手动 barrier、手动资源生命周期管理的繁琐。现在，我们把目光投向 UE 渲染管线真正的主舞台：**RenderDependencyGraph（RDG）**。

RDG 对 C++ 高手的最大惊喜，是它与 `stdexec` sender graph 在结构上高度同构：`AddPass` 如同 `then`，`Execute()` 如同 `start(op)`，`FRDGTextureRef` 如同 sender 图中的值槽，transient pool 如同 `operation_state` 的生命周期容器。这不是比喻的修辞，而是两套框架在"**描述工作 vs 执行工作**"这条分水岭上的真实对齐。

做完本模块，你应该能不看资料脱稿复述：

- **RDG 描述期（`AddPass` 阶段）vs 编译期（`Execute` 内部的 `Compile()`）vs 执行期（Pass 遍历）三阶段**，以及每个阶段做了什么
- `FRDGTextureRef` / `FRDGBufferRef` 与真实 RHI 资源（`FRHITexture` / `FRHIBuffer`）的本质区别——前者是描述期的**句柄**，后者是执行期才分配的 GPU 对象
- **RDG ↔ stdexec 六维对照表**（本模块核心教学杠杆）
- `FScene` / `FPrimitiveSceneProxy` / `FPrimitiveSceneInfo` 的双对象镜像关系，以及镜像同步的触发时机
- 如何手写一个最小 compute pass，让 RDG 自动推 transition barrier

**阶梯定位**：inspect（H1、H2）→ implement-own（H3）

**四固定问题**（每题必答）：  
- Q1：执行真正开始时刻？  
- Q2：生命周期拥有者？  
- Q3：涉及哪些 Named Thread？  
- Q4：涉及 UObject 时怎么对 GC 可见？

**RDG pass 图**（本模块全部题强制触发）：矩形节点为 pass，椭圆节点为资源，实线边为写入，虚线边为读取，标出每个资源的 transient lifetime 范围。

---

## 模块完成标准

做完本模块，你至少要能稳定说清楚以下六条：

1. `FRDGBuilder::AddPass()` 调用之后，GPU 命令有没有被录制？答案是否定的——lambda 被存储，延迟至 `Execute()` 才调用。
2. `FRDGBuilder::Execute()` 内部依次做了哪三件事：`Compile()`（依赖分析 + barrier 推导 + 资源生命周期计算）、资源分配（transient pool 实际分配显存）、pass 遍历（调用 lambda，录制 `FRHICommandList`）。
3. `FRDGTextureRef` 是什么：指向 `FRDGTexture` 对象的指针，该对象在描述期只是元数据，不持有 RHI 资源。真实的 `FRHITexture` 在 `Compile()` 之后才分配。
4. `FPrimitiveSceneProxy` 与 `UPrimitiveComponent` 的镜像关系：前者活在 RenderThread，后者活在 GameThread；`UPrimitiveComponent::CreateSceneProxy()` 在 GameThread 调用但代理对象本体在 RenderThread 使用。
5. RDG ↔ stdexec 类比表中的六个维度对应关系（见 H1）。
6. 手写最小 RDG compute pass 的完整骨架：`BEGIN_SHADER_PARAMETER_STRUCT`、`SHADER_PARAMETER_RDG_TEXTURE`、`SHADER_PARAMETER_RDG_TEXTURE_UAV`、`AddPass`、`FComputeShaderUtils::Dispatch`。

---

## 练习 H1：RDG 惰性图 inspect + stdexec 类比表（核心）

### 目标

深度阅读 `FRDGBuilder` 的构造、`AddPass`、`Execute`/`Compile` 四段源码，画出 RDG 三阶段时序，并完成本课程核心教学杠杆——**RDG ↔ stdexec 六维类比表**。在完成表之后，手动推导一个三 pass pipeline 在 RDG 下的 barrier 与 lifetime 结果。

### 前置理解

- 你已完成模块 G，理解 `FRHICommandList` 的命令录制模式：每调用一次 API 就立刻录制一条 GPU 命令。
- 你已完成 `C10_Execution` 模块 A–D，能说出"sender 是惰性蓝图，`start(op)` 才真正启动"这句话的含义。
- 你能接受本题是纯阅读 + 分析题，不要求写 UE 工程代码。目标是在心智模型层面建立 RDG 的完整框架。

### 必做任务

**任务 1：阅读三段源码并理解三阶段**

阅读以下三段源码（路径见本模块末"源码清单"）：

1. **`FRDGBuilder` 构造函数**（`RenderGraphBuilder.h` 第 63 行）  
   关注：构造期只初始化 allocator、内部状态、`RHICmdList` 引用。没有任何 GPU 命令产生。`FRDGBuilder` 在栈上分配，整个图的生命周期绑定到这个栈对象。

2. **`FRDGBuilder::AddPass()`**（`RenderGraphBuilder.h` 第 217–230 行）  
   关注：`AddPass` 是模板函数，接受一个 `ParameterStruct*`（声明该 pass 读/写哪些 RDG 资源）和一个 `ExecuteLambdaType&&`（真正的 GPU 命令 lambda）。  
   调用后做的事：把 pass 的元数据（名字、参数、flags、lambda）存入内部 `Passes` 数组。**lambda 不被立即调用**。  
   这一步等价于 `stdexec` 的 `then(sender, f)`：把变换描述附加到图上，但不执行。

3. **`FRDGBuilder::Execute()`**（`RenderGraphBuilder.cpp` 第 1755 行）和 **`FRDGBuilder::Compile()`**（第 1316 行）  
   关注 `Execute()` 的执行顺序：
   - 创建 epilogue pass（哨兵节点）
   - 调用 `Compile()`：
     - `SetupPassDependencies()`：分析每个 pass 的参数 struct，建立 producer/consumer 边，计算引用计数
     - pass culling（深度优先搜索，裁剪无输出的 pass，等价于 lazy sender 的"未被消费则不执行"）
     - barrier 推导：遍历所有资源的 producer/consumer 序列，在正确位置插入 `FRHITransition`
     - transient resource lifetime 计算：确定每个资源的 first-writer pass 和 last-reader pass
   - 资源分配：`GRDGTransientResourceAllocator` 实际分配 GPU 显存（此处 `FRDGTextureRef` 对应的 `FRHITexture` 才真正存在）
   - pass 遍历：按拓扑序调用每个 pass 的 `ExecuteLambda`，录制到 `FRHICommandList`

**任务 2：画出 RDG 三阶段时序图**

在笔记或草稿纸上画出以下三段时序，用横轴表示时间，纵轴标注"描述期 / 编译期 / 执行期"三条轨道：

```
描述期（FRDGBuilder 建图）
  AddPass("GBuffer")    ─── 存入 Passes[0]，lambda 未调用
  AddPass("Lighting")   ─── 存入 Passes[1]，lambda 未调用
  AddPass("PostProc")   ─── 存入 Passes[2]，lambda 未调用
         │
         ▼ Execute() 被调用
编译期（FRDGBuilder::Compile()）
  分析依赖边，裁剪无用 pass
  推导 barrier 插入位置
  计算资源 transient lifetime
  分配 GPU 显存（FRHITexture 在此刻实际存在）
         │
         ▼ pass 遍历
执行期（遍历 Passes，调用 lambda）
  Pass[0].Lambda(FRHICmdList)  ─── 录制 GBuffer draw
  barrier(GBuffer RT → SRV)   ─── RDG 自动插入
  Pass[1].Lambda(FRHICmdList)  ─── 录制 Lighting
  barrier(LightBuffer → SRV)  ─── RDG 自动插入
  Pass[2].Lambda(FRHICmdList)  ─── 录制 PostProc
```

**任务 3：完成 RDG ↔ stdexec 类比表（核心必做）**

严格按以下六维度填写，并在每行"说明"列写一句自己的理解：

| 维度 | stdexec sender graph | UE RDG | 说明 |
|---|---|---|---|
| **描述期** | `just(x)` / `then(s, f)` / `when_all(...)` 构造 sender 图 | `FRDGBuilder::AddPass(Name, Params, Flags, Lambda)` | 两者都只是在登记"做什么"，不产生任何实际工作 |
| **执行真正启动** | `start(op)` / `sync_wait(s)` | `FRDGBuilder::Execute()` | 执行启动前，sender/AddPass 的 lambda 均未被调用过 |
| **生命周期拥有者** | `operation_state`（由 `connect` 产生，拥有一次执行实例的所有状态）| `FRDGBuilder`（栈对象，持有 transient pool，pass 执行完毕即释放资源）| 两者都在"执行期"结束后允许资源被回收 |
| **编译期类型推导 / 依赖分析** | `completion_signatures`（编译期声明 value/error/stopped 类型，组合器据此推导整图类型）| `Compile()` 阶段：分析 `SHADER_PARAMETER_RDG_*` 字段，推导每个 pass 的 producer/consumer 关系，决定 barrier 插入位置 | stdexec 在编译期做类型推导；RDG 在运行时 `Compile()` 做依赖图分析——两者都是"在真正执行前做全图分析" |
| **图节点** | sender adaptor（`then` / `when_all` / 自定义 adaptor，每个都是图中一个节点）| `FRDGPass`（由 `AddPass` 创建，存储 lambda + 参数元数据 + pass flags）| 两者的"节点"都只是描述，不是执行实例 |
| **资源依赖 / 数据流** | `tag_invoke` / receiver environment（调度信息、stop_token 从 env 传播）；sender 产出的值沿图传播 | `FRDGTextureRef` / `FRDGBufferRef`（描述期的资源句柄，编码在 `SHADER_PARAMETER_RDG_TEXTURE` 字段里，RDG 据此建立 producer/consumer 边）| stdexec 的值流是 completion channel；RDG 的值流是 texture/buffer 的读写依赖 |

> **补充两行（进阶维度）**：

| 维度 | stdexec | UE RDG |
|---|---|---|
| **Pass culling / 惰性求值** | 未被 `sync_wait` / `start` 消费的 sender 图分支永远不执行 | `Compile()` 的 pass culling：输出未被后续 pass 或 `QueueTextureExtraction` 使用的 pass 被标记为 culled，`Execute()` 跳过其 lambda |
| **资源分配时机** | `operation_state` 在 `connect` 后分配，在 completion 后销毁 | transient texture/buffer 在 `Execute()` 内部、pass 的 first-writer 调用前分配，在 last-reader 执行后立即归还 transient pool |

**任务 4：手动推导三 pass pipeline 的 barrier + lifetime**

给定以下三 pass pipeline（结课项目 Cap1 的原型）：

```
Pass A ("Heightfield")
  写入 TextureA（heightfield 数据，格式 R32_FLOAT）
  读取：无

Pass B ("Normal")
  读取 TextureA（作为 SRV，生成法线贴图）
  写入 TextureB（normal map，格式 RGBA8）

Pass C ("Gamma")
  读取 TextureB（作为 SRV，gamma 矫正）
  写入 TextureC（output，格式 RGBA8）
```

手动推导（在笔记中写出）：

1. **每个资源的 transient lifetime**：  
   - TextureA：first-writer = Pass A，last-reader = Pass B → 在 Pass A 之前分配，在 Pass B 执行完后立即归还
   - TextureB：first-writer = Pass B，last-reader = Pass C → 在 Pass B 之前分配，在 Pass C 执行完后立即归还
   - TextureC：first-writer = Pass C，last-reader = 用户调用 `QueueTextureExtraction` 持有至帧结束

2. **自动推导的 barrier 位置**：
   - Pass A 执行后 → TextureA 状态从 `RenderTarget/UAVWrite` 转换为 `PixelShaderResource/SRV`（RDG 自动插入 transition barrier）
   - Pass B 执行后 → TextureB 状态从 `RenderTarget/UAVWrite` 转换为 `PixelShaderResource/SRV`
   - 传统 `FRHICommandList` 路径需要手动调用 `RHICmdList.Transition(...)` 约 4–6 行 × 2 处 = 8–12 行 barrier 代码
   - RDG 路径：0 行 barrier 代码

3. 绘制 RDG pass 图（矩形=pass，椭圆=资源，实线=写，虚线=读，标注 transient lifetime 括号范围）

### 进阶任务

- 阅读 `RenderGraphBuilder.cpp` 第 1316 行 `Compile()` 中的 `SetupPassDependencies()` 调用，理解引用计数如何被逐 pass 累加（`PassState.Texture->ReferenceCount += PassState.ReferenceCount`，第 1351 行）。这正是"资源 lifetime = first-writer 到 last-reader"的机制。
- 在 `Execute()` 中找到 `bCullPasses` 的判断分支，理解 pass culling 的触发条件（`GRDGCullPasses > 0`，可在引擎配置或 console variable 中观察）。
- 阅读 `RenderGraphDefinitions.h` 中 `ERDGPassFlags` 的定义，找到 `Compute`、`Raster`、`AsyncCompute`、`NeverCull` 四个 flag，理解它们对 `Compile()` 行为的影响。
- 对比：同样的三 pass pipeline，如果用传统 `FRHICommandList` 写，barrier 代码量是多少？统计行数，与 RDG 的 0 行对比。

### 验收点

- 你能不看资料，用自己的话说出"RDG 描述期 vs 执行期 = sender 惰性 vs start"这一条。
- 你填写的六维类比表每行都有实质内容，不是复制本文本。
- 你手动推导的三 pass pipeline 的 barrier 位置与 lifetime 计算结果是正确的。
- 你能指出 `FRDGTextureRef` 在描述期和执行期分别代表什么（描述期：元数据句柄；执行期：关联了真实 `FRHITexture`）。
- 你画出的 RDG pass 图包含 pass 节点、资源节点、读写边、transient lifetime 标注四个要素。

### 观察点

- RDG 的惰性和 sender 的惰性来源于相同的设计动机：只有收集到全图信息，才能做出最优的执行计划。`Execute()` / `start(op)` 之前不执行，是为了让"编译器"（`Compile()` / `connect+start` 之间的逻辑）看到完整图。
- pass culling 是 sender 图"未被消费不执行"的渲染版本。在大型渲染管线中，大量 pass（如 shadow map、debug visualization）在特定配置下会被整体 cull，零 GPU 开销。
- `FRDGTextureRef` 不是 `TSharedPtr<FRHITexture>`，也不是 `FRHITextureRef`。它是 `FRDGTexture*`，指向一个纯 CPU 元数据对象。在 `Execute()` 之前对它做任何 RHI 操作都是未定义行为——这是 RDG 中最常见的新手陷阱。

### 常见坑

- **在 `AddPass` 的 lambda 内部访问 `FRDGTextureRef->GetRHI()` 并传给外部**：lambda 在描述期被构造，`GetRHI()` 在执行期才有效；但如果你把 `GetRHI()` 的结果存到 lambda 外的变量里，在 `Execute()` 之后才用，则可能已经被 transient pool 回收。
- **把 `FRDGBuilder` 对象移出栈帧**：RDG allocator 的生命周期绑定到 `FRDGBuilder` 的存在。注释 `RenderGraphBuilder.h` 第 45 行明确说明"The builder should be created on the stack and executed prior to destruction"。
- **在 `AddPass` lambda 内捕获 GameThread 侧的 `UObject*`**：lambda 在 RenderThread 的 `Execute()` 阶段调用，此时 `UObject*` 可能已经被 GC 回收。必须按值捕获数据副本，或使用 `TWeakObjectPtr` 并在 lambda 内检查有效性（实际上 RenderThread 代码通常不直接操作 UObject，而是通过 `ENQUEUE_RENDER_COMMAND` 传入数据）。
- **`ERDGPassFlags` 选错**：把 compute pass 标记为 `ERDGPassFlags::Raster` 会导致 RDG 按 render pass 处理它，尝试创建 `FRHIRenderPassInfo`，通常导致 RHI 层断言失败。

### 复盘问题

- **Q1：执行真正开始时刻？**  
  在 `FRDGBuilder::Execute()` 被调用时，具体是在 `Compile()` 完成后的 pass 遍历阶段，每个 pass 的 lambda 才被调用并向 `FRHICommandList` 录制命令。`AddPass()` 不是执行开始点。

- **Q2：生命周期拥有者？**  
  `FRDGBuilder` 是整个图的生命周期拥有者。它持有 `FRDGAllocatorScope`（管理所有 pass 元数据和参数 struct 的 CPU 内存），以及 transient resource pool 的引用（管理 GPU 显存的借用生命周期）。`Execute()` 返回后，所有 transient 资源归还池，CPU 元数据随 `FRDGBuilder` 析构被释放。

- **Q3：涉及哪些 Named Thread？**  
  `FRDGBuilder::AddPass()` 和 `Execute()` 均在 **RenderThread** 上调用。`Execute()` 内部可能启动 `UE::Tasks` 并行任务（见 `ERDGBuilderFlags::Parallel` 模式），但主控制流仍在 RenderThread。最终的 GPU 命令通过 `FRHICommandList` 提交到 **RHIThread**。

- **Q4：涉及 UObject 时怎么对 GC 可见？**  
  RDG 层本身不操作 UObject。如果 pass lambda 需要用到原本来自 GameThread 的资源（例如 `UTexture2D` 的 `FTextureResource`），必须在 GameThread 侧通过 `ENQUEUE_RENDER_COMMAND` 按值传入 `FRHITexture*` 或 `IPooledRenderTarget*` 等 RHI 级别的句柄，然后用 `GraphBuilder.RegisterExternalTexture(...)` 注册为外部资源，不得在 RenderThread 持有原始 `UObject*`。

### 对应官方参考

- `Engine/Source/Runtime/RenderCore/Public/RenderGraphBuilder.h`，第 42–65 行（类注释、构造函数）
- `Engine/Source/Runtime/RenderCore/Public/RenderGraphBuilder.h`，第 200–230 行（`AddPass` 注释与声明）
- `Engine/Source/Runtime/RenderCore/Private/RenderGraphBuilder.cpp`，第 1316 行（`Compile()`）
- `Engine/Source/Runtime/RenderCore/Private/RenderGraphBuilder.cpp`，第 1755 行（`Execute()`）
- `Engine/Source/Runtime/RenderCore/Public/RenderGraphDefinitions.h`（`ERDGPassFlags`、`ERDGBarrierLocation`）

---

## 练习 H2：FScene 双对象镜像

### 目标

理解 UE 场景系统中最重要的一个结构性决策：**每个可渲染对象在 GameThread 和 RenderThread 各有一个独立副本**。GameThread 侧是 `UPrimitiveComponent`（UObject，GC 管理），RenderThread 侧是 `FPrimitiveSceneProxy`（非 UObject，手动生命周期）。两者通过受控的同步机制保持一致。

这是 Mindset Shift 3（线程分线一等公民）在渲染层的具体实例化。理解这个镜像模式，是读懂 `FScene`、`FSceneRenderer` 以及一切渲染逻辑的前提。

### 前置理解

- 你已完成模块 K，理解 `AActor` / `UActorComponent` 的生命周期钩子。
- 你已完成模块 F，理解 `ENQUEUE_RENDER_COMMAND` 的按值捕获语义。
- 你知道 `UPrimitiveComponent` 是 `UActorComponent` 的子类，负责场景中可渲染物体的几何、材质、Transform 等数据。

### 必做任务

**任务 1：理解 GameThread 与 RenderThread 双对象镜像**

```
GameThread 侧（UObject，GC 管理）   RenderThread 侧（非 UObject，FPrimitiveSceneProxy 拥有）
─────────────────────────────────   ──────────────────────────────────────────────────────────
AActor
  └─ UPrimitiveComponent           ←创建→  FPrimitiveSceneProxy
                                               └─ FPrimitiveSceneInfo（场景注册元数据）
        │ UPROPERTY 管理            不可直接访问 UObject；只接受值捕获的数据包
        │ GC 可见                  GC 不可见（非 UObject）；生命周期由 FScene 管理
        │ Tick 在 GameThread       GetDynamicMeshElements() 在 RenderThread
```

关键点：
- `FPrimitiveSceneProxy` **不继承 UObject**，不受 GC 管理。它的创建和销毁完全在 RenderThread 侧进行。
- `FPrimitiveSceneInfo` 是 `FPrimitiveSceneProxy` 在 `FScene` 中的注册记录，包含 index、bounds、LOD 信息等。
- `UPrimitiveComponent` 通过 `MarkRenderStateDirty()` 触发 RenderThread 侧的数据同步，不直接修改 `FPrimitiveSceneProxy`。

**任务 2：阅读 `UPrimitiveComponent::CreateSceneProxy()`**

在 UE 源码中，`CreateSceneProxy()` 是一个虚函数：

```cpp
// Engine/Source/Runtime/Engine/Public/PrimitiveSceneProxy.h 附近
// 实际定义在 UPrimitiveComponent（Components/PrimitiveComponent.h）
virtual FPrimitiveSceneProxy* CreateSceneProxy() { return nullptr; }
```

- 何时调用：`UPrimitiveComponent::RegisterComponent()` 后，引擎通过 `SendRenderState_Concurrent()` 在 GameThread 上调用 `CreateSceneProxy()`，然后通过 `ENQUEUE_RENDER_COMMAND` 将新建的 `FPrimitiveSceneProxy*` 发送到 RenderThread，由 `FScene::AddPrimitive()` 注册。
- 关键：`CreateSceneProxy()` 的**调用在 GameThread**，但返回的对象**只在 RenderThread 使用**。该对象不能包含 `UObject*` 裸引用，必须在构造时把所有需要的数据（材质、mesh 几何、Transform）以值的形式复制进来。

**任务 3：回答 FScene 的三层结构**

`FScene` 是 UE 场景在 RenderThread 侧的完整表示。它管理：

- `TSparseArray<FPrimitiveSceneInfo*> Primitives`：所有已注册的 primitive，以 Index 寻址
- `FLightSceneInfo` 数组：场景光源
- `FReflectionCaptureSceneProxy` 等：其他场景元素

当 `FSceneRenderer::Render()` 在 RenderThread 被调用时，它以 `FScene*` 为数据源，构建 `FRDGBuilder`，通过一系列 `AddPass()` 描述本帧的渲染管线，最后调用 `GraphBuilder.Execute()` 提交整帧。

**任务 4：观察 SceneViewExtension 机制（inspect）**

`ISceneViewExtension` 是 UE 的渲染管线扩展点，允许外部代码在不修改 `FSceneRenderer` 的情况下注入额外的渲染 pass：

```cpp
// SceneViewExtension.h 关键接口
virtual void PreRenderViewFamily_RenderThread(FRDGBuilder& GraphBuilder, FSceneViewFamily& InViewFamily) {}
virtual void PostRenderViewFamily_RenderThread(FRDGBuilder& GraphBuilder, FSceneViewFamily& InViewFamily) {}
```

注意：扩展接口直接接收 `FRDGBuilder&`，意味着扩展代码可以在同一个 `FRDGBuilder` 实例上 `AddPass()`，与主渲染 pass 共享 transient pool 和 barrier 推导。这是 RDG 惰性图的一个关键扩展性体现：图可以被多个代码路径增量构建，直到 `Execute()` 时才一次性编译执行。

**任务 5：理解 SceneProxy 在 LOD / 剔除中的角色（进阶）**

`FPrimitiveSceneProxy::GetViewRelevance(FSceneView*)` 是每帧 visibility 剔除中被调用的关键函数，返回 `FPrimitiveViewRelevance`，声明该 Primitive 在当前 View 下是否可见、使用哪种渲染路径（static / dynamic / translucent 等）。

`GetDynamicMeshElements(Views, ViewFamily, VisibilityMap, Collector)` 是 RenderThread 每帧从 SceneProxy 提取 mesh draw command 的接口。

### 进阶任务

- 阅读 `Engine/Source/Runtime/Engine/Public/PrimitiveSceneProxy.h`，找到 `GetViewRelevance`、`GetDynamicMeshElements` 两个关键虚函数的声明，理解它们的参数语义。
- 阅读 `ScenePrivate.h` 的 include 列表（第 27 行 `#include "PrimitiveSceneInfo.h"`），找到 `FPrimitiveSceneInfo` 的结构，理解它相对于 `FPrimitiveSceneProxy` 多出了哪些"场景注册"字段（OctreeData、PackedIndex、UniformBuffer 等）。
- 思考：如果你要实现一个自定义 Nanite mesh 的 SceneProxy，`GetDynamicMeshElements` 和 `FPrimitiveViewRelevance` 需要如何配合？

### 验收点

- 你能画出 GameThread/RenderThread 双对象镜像的关系图（不需要工具，文字 ASCII 图即可）。
- 你能说出 `CreateSceneProxy()` 的调用时机：`RegisterComponent()` 触发，但对象只在 RenderThread 使用。
- 你能解释为什么 `FPrimitiveSceneProxy` 不继承 `UObject`：它活在 RenderThread，GC 在 GameThread 运行，两者线程不兼容；RenderThread 代码不安全地使用 GC 管理的对象。
- 你能说出 `ISceneViewExtension` 的扩展点为什么接收 `FRDGBuilder&` 而不是其他接口。

### 观察点

- GameThread/RenderThread 双对象镜像是 UE 渲染架构的根基，几乎每一个渲染相关的系统都遵循这个模式：GameThread 持有"意图"，RenderThread 持有"执行状态"。
- `ENQUEUE_RENDER_COMMAND` 按值传入数据包的必要性，在这里得到了最清晰的体现：你不能让 RenderThread 直接访问 `UPrimitiveComponent` 的成员，因为 GC 随时可能在 GameThread 运行并修改或销毁这个对象。
- `FRDGBuilder` 的增量构建能力（多个调用方都可以往同一个 builder 上 `AddPass`）来源于 RDG 的惰性图设计：在 `Execute()` 之前，图是一个纯 CPU 数据结构，可以安全地并发读写（在 RenderThread 上）。

### 常见坑

- **在 `FPrimitiveSceneProxy` 的构造函数中存储 `UPrimitiveComponent*`**：这是错误的。Proxy 活在 RenderThread，Component 活在 GameThread，跨线程裸持有 `UObject*` 会在 GC 下产生悬空指针。应在构造时把必要数据（Transform、bounds、材质的 RHI 资源）按值复制进 Proxy，不保存 Component 指针。
- **修改 `FPrimitiveSceneProxy` 的数据而不通过 `MarkRenderStateDirty()`**：直接在 GameThread 修改 Proxy 的成员会产生 data race，因为 Proxy 可能同时在 RenderThread 被读取。正确做法是修改 Component 数据，调用 `MarkRenderStateDirty()`，引擎会在下一帧同步 Proxy。
- **在 `GetDynamicMeshElements` 中申请 `new` 内存**：`GetDynamicMeshElements` 每帧被调用，在其中用 `new` 分配内存会产生频繁的堆分配。UE 提供了 `FMeshElementCollector` 的局部分配器，应通过 `Collector.AllocateMesh()` 分配 `FMeshBatch`。

### 复盘问题

- **Q1：执行真正开始时刻？**  
  `FSceneRenderer::Render()` 由 RenderThread 在 `FRendererModule::BeginRenderingViewFamily` 处调用，每帧发生一次。`CreateSceneProxy()` 的调用时机是 Component 的 `RegisterComponent()` 后的第一次渲染状态同步，在 GameThread 上触发。

- **Q2：生命周期拥有者？**  
  `FPrimitiveSceneProxy` 由 `FScene` 拥有。`FScene::AddPrimitive()` 在 RenderThread 接管其所有权，`FScene::RemovePrimitive()` 触发其析构。`UPrimitiveComponent`（GameThread 侧镜像）由 GC 通过 `UPROPERTY` + Outer 链管理。

- **Q3：涉及哪些 Named Thread？**  
  `UPrimitiveComponent::CreateSceneProxy()` 在 **GameThread** 调用，返回的对象通过 `ENQUEUE_RENDER_COMMAND` 传递。`FScene::AddPrimitive()`、`FPrimitiveSceneProxy::GetDynamicMeshElements()` 在 **RenderThread** 调用。

- **Q4：涉及 UObject 时怎么对 GC 可见？**  
  `UPrimitiveComponent` 本身是 `UObject`，由 `UPROPERTY` + Actor Outer 链确保 GC 可见。`FPrimitiveSceneProxy` 不是 `UObject`，与 GC 无关，由 `FScene` 的 `TSparseArray<FPrimitiveSceneInfo*>` 手动管理生命周期。

### 对应官方参考

- `Engine/Source/Runtime/Engine/Public/PrimitiveSceneProxy.h`（`FPrimitiveSceneProxy` 类声明）
- `Engine/Source/Runtime/Renderer/Private/ScenePrivate.h`（`FScene` 结构，`#include "PrimitiveSceneInfo.h"` 等）
- `Engine/Source/Runtime/Renderer/Private/SceneRendering.cpp`（`FSceneRenderer` 构造与 Render 骨架）
- `Engine/Source/Runtime/Renderer/Private/DeferredShadingRenderer.cpp`（`RenderHzb`、`PostRenderOpaqueFX` 等 RDG 使用示例）

---

## 练习 H3：手写 RDG Pass（implement-own）

### 目标

从零写出一个最小 RDG compute pass：输入一个 `FRDGTextureRef`，输出另一个 `FRDGTextureRef`，让 RDG 自动推 transition barrier。这是本课程阶梯 implement-own 在渲染层的最终落点。

做完本题，你应该能完成以下脱稿复述：

> "RDG 描述期就是 sender 构图：我调用 `AddPass` 时只是在告诉 RDG '这个 pass 读 TextureA、写 TextureB'，不产生任何 GPU 命令。`Execute()` 时 RDG 才像 `start(op)` 一样启动整图——先 `Compile()` 推导 barrier，再遍历 pass 调用我的 lambda 录制 GPU 命令，barrier 全部自动插入，我写的 lambda 里一行 transition 代码都没有。"

### 前置理解

- 你已完成模块 G，能写出带 `BEGIN_SHADER_PARAMETER_STRUCT` 的 `FGlobalShader` compute shader，理解 `FComputeShaderUtils::Dispatch` 的用法。
- 你已完成 H1，能说出 RDG 三阶段和类比表六行。
- 你能接受本题不要求能在 UE 工程中实际编译运行——目标是写出结构正确、逻辑完整的代码骨架，理解每一行的必要性。

### 必做任务

**任务 1：定义 shader 参数 struct（代码生成点——UE 第二处 SHADER_PARAMETER 生成）**

```cpp
// 在你的 .h 文件中，在 ShaderModule 的 Public/ 目录
// 必须在 .generated.h 之前放置（如果这是 UObject 所在文件）

// BEGIN_SHADER_PARAMETER_STRUCT 是 UHT 之外的第二处代码生成：
// 它展开为一个 FParameters struct，其字段描述这个 pass 读写哪些资源
// RDG 通过反射这个 struct 来建立 producer/consumer 边
BEGIN_SHADER_PARAMETER_STRUCT(FMyComputePassParameters, )
    SHADER_PARAMETER_RDG_TEXTURE(Texture2D, InputTexture)        // 声明读取 InputTexture
    SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, OutputTexture) // 声明写入 OutputTexture
END_SHADER_PARAMETER_STRUCT()
```

关键说明：
- `SHADER_PARAMETER_RDG_TEXTURE` 声明一个 read-only SRV 绑定，RDG 将其 producer 关系记录为"OutputTexture 由某个先前 pass 写入，本 pass 为其 consumer"。
- `SHADER_PARAMETER_RDG_TEXTURE_UAV` 声明一个 UAV 写入绑定，RDG 将本 pass 记录为这个资源的 producer。
- 不需要手动写 `RHICmdList.Transition(...)`，RDG 的 `Compile()` 会根据这些声明自动推导并插入 barrier。

**任务 2：定义 compute shader 类**

```cpp
// 继承 FGlobalShader，声明 using FParameters = FMyComputePassParameters
class FMyComputeShader : public FGlobalShader
{
    DECLARE_GLOBAL_SHADER(FMyComputeShader);
    SHADER_USE_PARAMETER_STRUCT(FMyComputeShader, FGlobalShader);

    // 等价于 using FParameters = FMyComputePassParameters;
    using FParameters = FMyComputePassParameters;
};

// 在 .cpp 文件中注册 shader（静态初始化，禁止热重载后自动失效）
IMPLEMENT_GLOBAL_SHADER(FMyComputeShader, "/Game/Shaders/MyCompute.usf", "MainCS", SF_Compute);
```

**任务 3：在 RenderThread 函数中构建并 AddPass**

```cpp
// 此函数应在 RenderThread 上调用，例如在 ISceneViewExtension::PostRenderViewFamily_RenderThread 中
void AddMyComputePass(
    FRDGBuilder& GraphBuilder,          // 由调用者传入的 builder 实例
    FRDGTextureRef InputTexture,        // 描述期句柄：调用前某 pass 已写入
    FRDGTextureRef OutputTexture,       // 描述期句柄：本 pass 将写入
    const FIntPoint& TextureSize)
{
    // Step 1：分配参数 struct（生命周期绑定到 GraphBuilder）
    FMyComputePassParameters* Parameters = GraphBuilder.AllocParameters<FMyComputePassParameters>();
    Parameters->InputTexture  = InputTexture;
    Parameters->OutputTexture = GraphBuilder.CreateUAV(OutputTexture);  // 创建 UAV 视图

    // Step 2：获取 shader 实例（从全局 shader map 中取，不 new）
    TShaderMapRef<FMyComputeShader> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

    // Step 3：AddPass（描述期——此处不调用任何 RHI API，不产生 GPU 命令）
    GraphBuilder.AddPass(
        RDG_EVENT_NAME("MyComputePass"),    // GPU 调试名（仅在 RDG_EVENTS 启用时有开销）
        Parameters,                         // 参数 struct：RDG 用它建立 producer/consumer 边
        ERDGPassFlags::Compute,             // 标记这是一个 Compute pass（非 Raster）
        [ComputeShader, Parameters, TextureSize](FRHIComputeCommandList& RHICmdList)
        // ↑ lambda 在 Execute() 的 pass 遍历阶段才被调用
        // ↑ 此时 InputTexture 和 OutputTexture 已被 transient pool 分配了实际 GPU 内存
        // ↑ barrier（InputTexture 从 RT/UAV 状态转为 SRV 状态）已由 RDG 自动插入
        {
            // 设置 PSO（Pipeline State Object）
            FComputeShaderUtils::ValidateGroupCount(TextureSize, FComputeShaderUtils::kGroupSize2D);

            // 绑定参数并 Dispatch（RDG 保证此时参数中的 RHI 资源已经有效）
            FComputeShaderUtils::Dispatch(
                RHICmdList,
                ComputeShader,
                *Parameters,
                FComputeShaderUtils::GetGroupCount(TextureSize, FComputeShaderUtils::kGroupSize2D));
        });
    // AddPass 返回后，lambda 仍未执行。
    // 只有当调用者在外层调用 GraphBuilder.Execute() 时，才会：
    //   1. Compile()：分析此 pass 的 Parameters，推导 InputTexture → SRV 的 barrier 插入点
    //   2. 分配 transient 资源（如果 InputTexture/OutputTexture 是 transient 创建的）
    //   3. 调用此 lambda 录制 GPU 命令
}
```

**任务 4：创建 transient 资源并串联（完整 pipeline 骨架）**

```cpp
// 假设 SceneColorTexture 已存在（由先前 pass 写入）
// 现在创建 OutputTexture（transient，仅本 pass 使用）

FRDGTextureDesc OutputDesc = FRDGTextureDesc::Create2D(
    SceneColorTexture->Desc.Extent,   // 与输入同分辨率
    PF_FloatRGBA,                     // 格式
    FClearValueBinding::Black,
    // 关键：声明这个 texture 会被 UAV 写入（compute）和后续 SRV 读取
    TexCreate_UAV | TexCreate_ShaderResource);

FRDGTextureRef OutputTexture = GraphBuilder.CreateTexture(OutputDesc, TEXT("MyOutput"));
// CreateTexture 返回的是描述期句柄（FRDGTexture*），GPU 内存尚未分配

AddMyComputePass(GraphBuilder, SceneColorTexture, OutputTexture, SceneColorTexture->Desc.Extent);

// OutputTexture 现在可以作为下一个 pass 的输入
// 传统写法对比（伪代码，说明 RDG 省掉了什么）：
//   FRHITexture* OutputRHI = RHICreateTexture2D(...);         // 立即分配 GPU 内存
//   RHICmdList.Transition(SceneColor, ERHIAccess::SRVCompute); // 手动 barrier（约 4-6 行）
//   RHICmdList.SetComputeShader(ComputeShaderRHI);
//   RHICmdList.SetShaderTexture(ComputeShaderRHI, 0, SceneColor);
//   RHICmdList.SetUAV(ComputeShaderRHI, 0, OutputUAV);
//   RHICmdList.DispatchComputeShader(...);
//   RHICmdList.Transition(OutputRHI, ERHIAccess::SRVGraphics); // 手动 barrier（约 4-6 行）
//
// RDG 写法：零行 barrier 代码，资源声明在 SHADER_PARAMETER_STRUCT 中
```

**任务 5：传统 RHI vs RDG 行数对比（进阶核心）**

在笔记中完成以下对比表（以"单 compute pass，读一 texture，写一 texture"为例）：

| 步骤 | 传统 `FRHICommandList` 写法（行数）| RDG 写法（行数） |
|---|---|---|
| 资源分配 | 2–4 行（`RHICreateTexture2D` + flags + 错误检查）| 2 行（`CreateTexture` + `CreateUAV`，transient）|
| 进入 barrier（读取前 transition）| 4–6 行（`RHICmdList.Transition`，含访问掩码）| 0 行（SHADER_PARAMETER_RDG_TEXTURE 声明驱动）|
| Shader 绑定 | 6–10 行（SetComputeShader + SetTexture + SetUAV + 各 slot 绑定）| 3–4 行（`FComputeShaderUtils::Dispatch` 一行封装）|
| 退出 barrier（写完后 transition）| 4–6 行 | 0 行（RDG 推导下一 pass 的访问需求自动插入）|
| 资源释放 | 手动（或 TRefCountPtr 析构）| 自动（transient pool 归还）|
| **合计** | **约 18–26 行 barrier 相关代码** | **0 行 barrier 代码** |

这个对比是"为什么 UE 引入 RDG"最直观的答案。

### 进阶任务

- 为 `AddMyComputePass` 增加一个 `FRDGBufferRef OutputBuffer` 参数，让 compute shader 同时写入一个 structured buffer。观察 `SHADER_PARAMETER_RDG_BUFFER_UAV` 的用法，并推导 RDG 为这个 buffer 插入的 barrier 类型（`ERHIAccess::UAVCompute`）。
- 实现完整的三 pass pipeline（对应 H1 任务 4 的 heightfield→normal→gamma），串联三次 `AddPass`，共享同一个 `FRDGBuilder` 实例，验证中间 texture 可以 transient 分配。
- 阅读 `RenderGraphUtils.h` 中 `FComputeShaderUtils::Dispatch` 的实现，理解它如何从 `FParameters` 推导 SRV/UAV 绑定——这是 `BEGIN_SHADER_PARAMETER_STRUCT` 代码生成的运行时消费端。

### 验收点

- 你写出的代码骨架中，`AddPass` 的 lambda 内没有任何 `RHICmdList.Transition(...)` 调用，barrier 完全由参数 struct 声明驱动。
- 你能解释 `GraphBuilder.AllocParameters<FMyComputePassParameters>()` 分配的对象生命周期为何绑定到 `GraphBuilder`（而不是调用栈帧）：因为 lambda 在 `Execute()` 才调用，届时调用栈帧已销毁，参数 struct 必须由 RDG 持有。
- 你能说出为什么 `GraphBuilder.CreateTexture()` 不立即分配 GPU 内存，而 `RHICreateTexture2D()` 立即分配。
- 你的传统 vs RDG 行数对比表数字合理（barrier 代码 > 15 行 vs 0 行）。
- 你完成了对 RDG 描述期 vs 执行期的脱稿复述（见题目开头的复述段落）。

### 观察点

- `BEGIN_SHADER_PARAMETER_STRUCT` 是 UE 的**第二处代码生成**（第一处是 UHT 的 `.generated.h`）。它不依赖 UHT，而是通过 C++ 宏展开在编译期生成类型元数据，让 RDG 在运行时能反射参数 struct 的字段，自动建立资源依赖图。模块 G 已经介绍过其结构；在 H3 中你真正依赖它的"RDG 可见性"来驱动 barrier 推导。
- `ERDGPassFlags::Compute` vs `ERDGPassFlags::AsyncCompute` 的区别：前者在主 graphics pipeline 顺序执行，后者被 RDG 调度到 async compute pipeline 并行执行。选错 flag 不会编译报错，但会导致 GPU 时序错误（async compute pass 不能依赖 graphics pass 的输出，除非有显式 fence）。
- RDG 的 `NeverCull` flag：如果一个 pass 的输出没有被任何后续 pass 或 `QueueTextureExtraction` 引用，RDG 默认会 cull 掉它。如果你需要强制执行（例如 UAV 写入显存但不读回 GPU），需要添加 `ERDGPassFlags::NeverCull`。

### 常见坑

- **`Parameters->OutputTexture = OutputTexture`（直接赋 FRDGTextureRef，没有 `CreateUAV`）**：`SHADER_PARAMETER_RDG_TEXTURE_UAV` 字段类型是 `FRDGTextureUAVRef`，不能直接赋 `FRDGTextureRef`。必须先 `GraphBuilder.CreateUAV(OutputTexture)` 获取 `FRDGTextureUAVRef`。
- **在 lambda 外部调用 `Parameters->InputTexture->GetRHI()`**：在 `AddPass` 调用时，`FRDGTextureRef` 对应的 RHI 资源尚未存在（`Execute()` 还没跑）。只有在 lambda 内部（即 `Execute()` 期间）才能访问 `GetRHI()`。
- **`AllocParameters` 在栈上分配**：`FMyComputePassParameters params; Parameters = &params; GraphBuilder.AddPass(...)` 是错误写法。lambda 延迟执行时 `params` 已析构。必须用 `GraphBuilder.AllocParameters<>()` 分配到 RDG 持有的 arena 上。
- **`ERDGPassFlags` 选 `Raster` 做 compute 工作**：RDG 会为 `Raster` pass 创建 render pass（`FRHIRenderPassInfo`），在 compute-only pass 上触发断言。
- **忘记在 `TexCreate_*` flags 中加 `TexCreate_UAV`**：如果 `CreateTexture` 时没有声明 UAV flag，后续 `CreateUAV(OutputTexture)` 会在 RDG 验证层报错（仅 `RDG_ENABLE_DEBUG` 为真时）。

### 复盘问题

- **Q1：执行真正开始时刻？**  
  `AddPass` 的 lambda 在 `FRDGBuilder::Execute()` 的 pass 遍历阶段才被调用，具体是 `Compile()` 完成后的 pass-by-pass 迭代中。`AddPass` 调用本身只是描述，不执行任何 GPU 命令。

- **Q2：生命周期拥有者？**  
  `GraphBuilder.AllocParameters<>()` 分配的参数 struct，生命周期由 `FRDGBuilder`（栈对象）拥有，绑定到 `Execute()` 完成。`FRDGTextureRef` 指向的 `FRDGTexture` 元数据对象同样由 `FRDGBuilder` 持有。transient GPU 显存由 `GRDGTransientResourceAllocator` 持有，last-reader pass 执行后立即归还。

- **Q3：涉及哪些 Named Thread？**  
  整个 `AddMyComputePass` 函数（包括 `AddPass` 调用）在 **RenderThread** 上执行。`Execute()` 也在 **RenderThread** 上执行（可能启动并行 task 录制命令，但控制流主线仍是 RenderThread）。GPU 命令最终由 **RHIThread** 提交给 GPU 驱动。

- **Q4：涉及 UObject 时怎么对 GC 可见？**  
  本题的 compute pass 层面完全不涉及 UObject。如果需要使用来自 GameThread 的 `UTexture2D`，应通过 `UTexture2D::Resource->TextureRHI`（`FTextureResource` 是非 UObject，线程安全）以 `RegisterExternalTexture` 的形式注入 RDG，不得在 RenderThread 持有 `UObject*`。

### 对应官方参考

- `Engine/Source/Runtime/RenderCore/Public/RenderGraphBuilder.h`，第 200–230 行（`AddPass` 详细注释）
- `Engine/Source/Runtime/RenderCore/Public/RenderGraphUtils.h`（`FComputeShaderUtils::Dispatch`、`GetGroupCount`）
- `Engine/Source/Runtime/RenderCore/Public/RenderGraphResources.h`（`FRDGTextureDesc::Create2D`、`FRDGSubresourceState`）
- `Engine/Source/Runtime/RenderCore/Private/RenderGraphBuilder.cpp`，第 1316 行（`Compile()`：资源依赖分析核心）
- `Engine/Source/Runtime/Renderer/Private/DeferredShadingRenderer.cpp`，第 595 行（`GraphBuilder.AddPass(RDG_EVENT_NAME("SetSceneTexturesUniformBuffer"), ...)` 真实用例）

---

## 做完本模块后你现在应该能说清楚什么

至少把下面六句话说顺，不借助任何资料：

1. **RDG 的描述期**：`FRDGBuilder::AddPass()` 只是在向图中添加一个节点（`FRDGPass`），存储 lambda 和参数 struct，不调用任何 RHI API，不产生任何 GPU 命令。

2. **RDG 的编译期**：`Execute()` 内部先调用 `Compile()`，分析所有 pass 的 `SHADER_PARAMETER_RDG_*` 字段，建立资源的 producer/consumer 依赖图，推导 barrier 插入位置，计算 transient 资源的分配/回收时间点，执行 pass culling。

3. **RDG 的执行期**：`Compile()` 完成后，按拓扑序遍历所有未被 cull 的 pass，在每对相邻 pass 之间插入 RDG 推导的 barrier，然后调用 pass 的 lambda 向 `FRHICommandList` 录制 GPU 命令。

4. **RDG ↔ stdexec 类比的核心**：`AddPass` 如同 `then`，`Execute()` 如同 `start(op)`，`FRDGTextureRef` 如同 sender 图中的值槽（描述期不持有真实值），`FRDGBuilder`（transient pool）如同 `operation_state`，pass culling 如同惰性 sender 未被消费不执行。

5. **FScene 双对象镜像**：`UPrimitiveComponent` 活在 GameThread（GC 管理），`FPrimitiveSceneProxy` 活在 RenderThread（`FScene` 手动管理）；两者通过 `ENQUEUE_RENDER_COMMAND` 按值同步，`CreateSceneProxy()` 在 GameThread 调用，proxy 对象只在 RenderThread 使用。

6. **RDG 的零 barrier 成本**：传统 `FRHICommandList` 方式写一个有输入输出的 compute pass 需要 ~18–26 行 barrier 相关代码；RDG 方式将 barrier 描述内嵌在 `SHADER_PARAMETER_RDG_*` 宏中，调用层的 lambda 中 0 行 barrier 代码。

---

## 本模块覆盖的 UE 源码清单

| 路径 | 关键行 | 模块 H 对应点 |
|---|---|---|
| `Engine/Source/Runtime/RenderCore/Public/RenderGraphBuilder.h` | 第 42–65 行（类注释 + 构造函数）；第 200–230 行（`AddPass` 注释 + 三个重载声明）| H1：描述期；H3：AddPass 骨架 |
| `Engine/Source/Runtime/RenderCore/Private/RenderGraphBuilder.cpp` | 第 1316 行（`Compile()`：pass 依赖分析、barrier 推导、pass culling）；第 1755 行（`Execute()`：完整编译+执行流程）| H1：编译期与执行期三阶段分析 |
| `Engine/Source/Runtime/RenderCore/Public/RenderGraphResources.h` | 第 42–100 行（`FRDGProducerState`、`FRDGSubresourceState`、`ERDGBarrierLocation`）| H1：barrier 推导机制；H3：资源状态理解 |
| `Engine/Source/Runtime/RenderCore/Public/RenderGraphDefinitions.h` | 全文（`ERDGPassFlags`、`RDG_EVENTS`、调试宏定义）| H1：pass flags 语义；H3：`ERDGPassFlags::Compute` 选择 |
| `Engine/Source/Runtime/RenderCore/Public/RenderGraphUtils.h` | 第 50–80 行（`HasBeenProduced`、`GetIfProduced`、`GetLoadActionIfProduced`）| H3：判断资源是否已被生产 |
| `Engine/Source/Runtime/Engine/Public/PrimitiveSceneProxy.h` | 第 1–80 行（类前向声明、`FDesiredLODLevel`、`FPrimitiveSceneProxy` 骨架）| H2：双对象镜像结构 |
| `Engine/Source/Runtime/Renderer/Private/ScenePrivate.h` | 第 1–60 行（`FScene` include 树：`PrimitiveSceneInfo.h`、`GPUScene.h`、`SceneRendering.h`）| H2：`FScene` 全局结构 |
| `Engine/Source/Runtime/Renderer/Private/SceneRendering.cpp` | 第 1–60 行（`FSceneRenderer` include 列表）；第 2645 行（`FSceneRenderer` 构造函数）| H2：`FSceneRenderer` 框架 |
| `Engine/Source/Runtime/Renderer/Private/DeferredShadingRenderer.cpp` | 第 487 行（`RenderHzb` 使用 `GraphBuilder`）；第 595–605 行（`GraphBuilder.AddPass(...)` 真实用例）| H1/H3：RDG pass 在真实渲染管线中的形态 |

---

## 对后续模块的期待 + 整门课程收束语

本模块是整门课程 A-H 的压轴收束点。

模块 I（Slate/UMG）和模块 J（网络与 Replication）是正交于渲染主线的独立维度：Slate/UMG 处理 UI 组件树的描述与绘制，网络 Replication 处理跨机器的状态同步。两者都依赖模块 D（UObject/反射）作为基础，但与 RDG 渲染管线没有直接依赖关系。学有余力者可按兴趣顺序进入。

做完模块 A 到 H 的全部练习，此刻你站在的位置是：

**你理解了 UE 引擎内核的真实骨架。**

一句话把它钉住：

> **UE 引擎内核 = UObject 反射 + Module 组织 + RDG 延迟图 + Subsystem 层级单例。**

- **UObject 反射**（模块 D）：运行时可枚举的类型系统，是蓝图、序列化、网络 replication、GC 这四条主干的共同基础。没有反射，UE 就是一个普通的 C++ 程序。
- **Module 组织**（模块 A）：代码不是以文件为单位管理，而是以有 LoadingPhase 的 Module 为单位管理。依赖是显式的，生命周期由 `FModuleManager` 控制。这是 UE 能让数百人团队协作的工程组织基础。
- **RDG 延迟图**（本模块）：渲染不是"调用 API 立刻执行"，而是"描述完整帧的渲染意图，再由 Execute() 一次性编译优化执行"。这和 C++20 ranges 的惰性 view、stdexec 的 sender graph 是同一条设计哲学在三个不同领域的独立实例化。
- **Subsystem 层级单例**（模块 K + 01-心智模型 Shift 6）：UE 里没有裸全局变量充当单例，取而代之的是生命周期绑定到 Engine/GameInstance/World 三层的 Subsystem——"单例"的生命周期被框架层接管，在 PIE 多次开始/停止时行为正确。

从 `C10_Execution` 出发，经过 `C15_Unreal_Engine` A 到 H，你完成的不是一次 API 学习，而是一次**设计模式层的通觉**——你看到了"惰性求值"这条线索如何穿越 C++20 ranges（惰性 view）、P2300 stdexec（惰性 sender 图）、UE RDG（惰性渲染图），在三个完全不同的工程语境里做出了同一个设计选择：**先描述，后执行；全图可见，才能优化**。

这不是巧合，这是工业级 C++ 框架在面对"大规模、延迟绑定、依赖复杂的工作流"时收敛的共同解。

你现在能向任何人解释 UE 引擎内核的骨架了。

---

*所有 API 签名、源码路径及行号以本机 UE 5.7 release 分支（`G:\Unreal Engine\Source\UnrealEngine`）为准。*
