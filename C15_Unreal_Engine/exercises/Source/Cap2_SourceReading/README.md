# 结课项目 2：源码对照阅读笔记

> 对应章节: ../../../14-结课项目2-源码对照项目.md §结课项目 2
> UE 源码根目录: `G:\Unreal Engine\Source\UnrealEngine\Engine\Source\`

## 项目定位

结课项目 1 训练"能把 A-H+K 的能力组合成可运行系统"。本项目训练的是"带着已建立的心智模型回头读官方引擎实现，精确定位你的 mini 练习对应哪段真实代码"。

读法原则：不是"看完整个文件"，而是"带问题入场"。每次阅读只回答本文件规定的五个固定问题，其余细节留到后续专题深挖。

---

## 阅读清单（6 个源码文件）

| # | 源码文件（相对 Engine/Source/） | 对应模块 | 读法重点 |
|---|---|---|---|
| 1 | `Runtime/Core/Private/Modules/ModuleManager.cpp` | 模块 A | `LoadModule` → `LoadModuleWithFailureReason` → `StartupModule` 调用链 |
| 2 | `Runtime/CoreUObject/Public/UObject/Object.h` + `Private/UObject/Obj.cpp` | 模块 D | UObject 构造链、`FinishDestroy`、`CreateDefaultSubobject` 路径 |
| 3 | `Runtime/Engine/Private/Actor.cpp` | 模块 K | `UWorld::SpawnActor` 路径；Actor 生命周期五钩子顺序 |
| 4 | `Runtime/CoreUObject/Private/Serialization/AsyncLoading2.cpp` | 模块 E | `FAsyncLoadingThread2` 入口；`LoadPackageInternal` → 队列投递路径 |
| 5 | `Runtime/RenderCore/Public/RenderGraphBuilder.h` + `Private/RenderGraphBuilder.cpp` | 模块 H | 描述期（`AddPass`）↔ 编译期（`Compile`）↔ 执行期（`Execute`） |
| 6 | `Runtime/Renderer/Private/SceneRendering.cpp` | 模块 H | `FRendererModule::BeginRenderingViewFamilies` 骨架；`ENQUEUE_RENDER_COMMAND` 入口 |

**路径修正说明**：AsyncLoading2.cpp 位于 `Runtime/CoreUObject/Private/Serialization/`，不在 `Engine/Private/Streaming/`（后者路径不存在）。

---

## 每文件必答五问说明

每个源码文件都必须回答以下五个问题：

- **Q1 执行真正开始时刻**：这段代码在引擎启动序列或帧序列的哪个时机被调用？哪条线程？
- **Q2 生命周期拥有者**：核心对象由谁创建、由谁销毁？GC、FModuleManager、UWorld，还是 RDG Transient Pool？
- **Q3 涉及哪些 Named Thread**：调用链跨越了哪些 `ENamedThreads`？哪里有显式线程切换点？
- **Q4 涉及 UObject 时 GC 可见性**：相关 `UObject*` 是通过 `UPROPERTY`、`TObjectPtr`、Outer 链，还是 `AddToRoot` 保持 GC 可见的？
- **Q5 最像我哪个自写练习**：能对照到你哪道具体练习（如 A1、D3、K3）？差异最大的一点是什么？

---

## 文件 1：`Runtime/Core/Private/Modules/ModuleManager.cpp`

**对应模块 A — 构建系统与模块生命周期**

**关键入口路径**（阅读时用 Grep/IDE 跳转，不要线性通读）：

```
FModuleManager::LoadModule(InModuleName)
  → LoadModuleWithFailureReason(...)         // 真正执行加载的函数
      InitializeModule lambda                // 创建模块实例
        ModuleInfo->Module->StartupModule()  // 你的 IModuleInterface 被调用
        ModuleInfo->bIsReady = true
        ModulesChangedEvent.Broadcast(...)
```

**`IMPLEMENT_MODULE` 宏的两条路径**：
- 动态链接（默认）：DLL 内定义 `FModuleInitializerEntry` 全局对象，构造函数插入链表，`LoadModuleWithFailureReason` 通过 `FindModule` 遍历链表查工厂
- 单体链接（Monolithic）：宏定义 `FStaticallyLinkedModuleRegistrant<T>` 全局对象，注册到 `StaticallyLinkedModuleInitializers` map

### Q1 执行真正开始时刻

> **【学员填写】**
>
> 提示：`LoadModule` 函数在非 GameThread 调用时会返回空（行 921）。引擎启动序列从哪里触发第一次 `LoadModule`？

### Q2 生命周期拥有者

> **【学员填写】**
>
> 提示：模块实例保存在哪个数据结构里？`ShutdownModule` 什么时候被调用？`TUniquePtr` 析构时发生什么？

### Q3 涉及哪些 Named Thread

> **【学员填写】**
>
> 提示：`LoadModule` 对非 GameThread 调用有硬性排斥，`StartupModule` 内部可以向哪些线程投递命令？

### Q4 涉及 UObject 时 GC 可见性

> **【学员填写】**
>
> 提示：`FModuleManager` 本身不是 `UObject`，若模块在 `StartupModule` 中持有 `UObject*`，应该怎么保证 GC 可见？

### Q5 最像我哪个自写练习 + 差异

> **【学员填写】**
>
> 提示：你写的是 Module 实现端（`IMPLEMENT_MODULE` + `StartupModule` 日志），`ModuleManager.cpp` 展示的是管理端。差异最大的一点是什么？

### 与对应 mini 实现的 5 处对照点

> **【学员填写】**
>
> 对照 ExA1_HelloModule 的代码，找出以下 5 处对应关系（填写函数名或行为描述）：
>
> 1. `IMPLEMENT_MODULE(FDefaultModuleImpl, ExA1_HelloModule)` ↔ 官方的 ___________
> 2. `StartupModule()` 打印日志 ↔ 官方的 ___________
> 3. `ShutdownModule()` ↔ 官方的 ___________（析构时机）
> 4. `bIsReady` 标志 ↔ 你的代码里有没有等价物？___________
> 5. `ModulesChangedEvent.Broadcast` ↔ 你的代码里有没有等价的"模块就绪通知"机制？___________

---

## 文件 2：`Runtime/CoreUObject/Public/UObject/Object.h` + `Private/UObject/Obj.cpp`

**对应模块 D — UObject / 反射 / GC**

**关键路径**：

```
// 构造链
NewObject<T>() → FUObjectArray::AllocateUObjectIndex → UObject::UObject()
  → FObjectInitializer (线程局部栈顶)
      CreateDefaultSubobject → FObjectInitializer::CreateDefaultSubobject (Obj.cpp 191行)

// GC 析构链
ConditionalBeginDestroy() → BeginDestroy()          // Object.h 1183行
  IsReadyForFinishDestroy()                          // 异步轮询
ConditionalFinishDestroy() → FinishDestroy()         // Object.h 1186行
  RF_FinishDestroyed 标志置位 → GUObjectArray 回收内存
```

### Q1 执行真正开始时刻

> **【学员填写】**
>
> 提示：`NewObject<T>()` / `SpawnActor<T>()` / CDO 构造（引擎初始化时）分别对应什么时刻？`BeginDestroy` 由谁在什么时候调用？

### Q2 生命周期拥有者

> **【学员填写】**
>
> 提示：`FUObjectArray` / `GUObjectArray` 是谁？GC 如何从根集出发沿 `UPROPERTY` 指针链遍历？`Outer` 链的作用是什么？

### Q3 涉及哪些 Named Thread

> **【学员填写】**
>
> 提示：构造在哪条线程？`BeginDestroy` / `FinishDestroy` 在哪条线程？`PostLoad`（异步加载包时）可能在哪里执行？

### Q4 涉及 UObject 时 GC 可见性

> **【学员填写】**
>
> 提示：`UPROPERTY` / `TObjectPtr<T>` / Outer 链 / `AddToRoot()` 四种方式各自的适用场景？

### Q5 最像我哪个自写练习 + 差异

> **【学员填写】**
>
> 提示：析构侧最像 D4（mini mark-sweep），`CreateDefaultSubobject` 路径最像 D2。官方实现里有 `IsReadyForFinishDestroy` 异步轮询，你的 mini GC 有吗？

### 与对应 mini 实现的 5 处对照点

> **【学员填写】**
>
> 对照 ExD4_MiniMarkSweep 和 ExD2_CDOSubobject，找出以下 5 处对应关系：
>
> 1. D4 的"标记阶段（从根集出发遍历）" ↔ 官方的 ___________
> 2. D4 的"清除阶段（回收未标记对象）" ↔ 官方的 `BeginDestroy` + `FinishDestroy` 两步；差异：___________
> 3. D2 的 `CreateDefaultSubobject<USceneComponent>(TEXT("Root"))` ↔ 官方的 ___________（线程局部栈关系）
> 4. D4 无"等待渲染资源释放"步骤 ↔ 官方的 `IsReadyForFinishDestroy` 异步轮询解决什么问题？___________
> 5. D3 的 `TFieldIterator<FProperty>` 遍历 ↔ 官方 GC 遍历 `UPROPERTY` 时用的是哪种机制？___________

---

## 文件 3：`Runtime/Engine/Private/Actor.cpp`

**对应模块 K — Actor / Component / World / Subsystem**

**`UWorld::SpawnActor` 完整路径**（关键函数在 `LevelActor.cpp` + `Actor.cpp`）：

```
UWorld::SpawnActor(Class, Transform, Params)       // LevelActor.cpp 455行
  → NewObject<AActor>(Level, Class, ...)
  → Actor->PostSpawnInitialize(...)                 // Actor.cpp 4249行
      RegisterAllComponents()
      FinishSpawning(...)                           // Actor.cpp 4347行
        PostActorConstruction()                     // Actor.cpp 4403行
          PreInitializeComponents()                 // 钩子 1
          InitializeComponents()
          PostInitializeComponents()                // 钩子 2 (Actor.cpp 6544行)
          DispatchBeginPlay()                       // 钩子 3 (Actor.cpp 4690行，条件触发)
```

**五钩子顺序**：PreInitializeComponents → PostInitializeComponents → BeginPlay → Tick → EndPlay → Destroyed → BeginDestroy

### Q1 执行真正开始时刻

> **【学员填写】**
>
> 提示：`UWorld::SpawnActor` 何时/何线程调用？`BeginPlay` 为何不总是在 `PostInitializeComponents` 后立即调用（`World->HasBegunPlay()` 的条件）？

### Q2 生命周期拥有者

> **【学员填写】**
>
> 提示：Actor 以 `UWorld → ULevel → TArray<AActor*> Actors`（`UPROPERTY`）这条链路维持 GC 可达性。`AActor::Destroy()` 调用后发生什么？

### Q3 涉及哪些 Named Thread

> **【学员填写】**
>
> 提示：Actor 生命周期全程在哪条线程？`RegisterAllComponents` 会向哪条线程投递命令？

### Q4 涉及 UObject 时 GC 可见性

> **【学员填写】**
>
> 提示：`OwnedComponents`（`TSet<UActorComponent*>`，带 `UPROPERTY`）是 GC 可达性的主链路。Actor 自身如何可达？

### Q5 最像我哪个自写练习 + 差异

> **【学员填写】**
>
> 提示：最像 K1（五钩子日志观察）和 K3（SpawnActor 路径阅读）。`bDeferConstruction` / 碰撞检测分支等条件控制在练习里感知到了吗？

### 与对应 mini 实现的 5 处对照点

> **【学员填写】**
>
> 对照 ExK1_HelloActor 和 ExK3_WorldLevel，找出以下 5 处对应关系：
>
> 1. K1 的 `BeginPlay()` 打印日志 ↔ 官方 `DispatchBeginPlay()` 的触发条件（行 4690）：___________
> 2. K1 的 `PrimaryActorTick.bCanEverTick = true` ↔ 官方把 Actor 加入 Tick 列表的时机：___________
> 3. K3 的 `GetWorld()->SpawnActor<T>()` ↔ 官方 `PostSpawnInitialize` 中 `RegisterAllComponents()` 做了什么？___________
> 4. K1 的 `EndPlay` ↔ 官方 `Destroyed()` 和 `BeginDestroy()` 的先后顺序：___________
> 5. K4 的 `UWorldSubsystem::Initialize` 与 `AHeightFieldViewer::BeginPlay` 的先后顺序 ↔ 官方 `PostActorConstruction` 里 `bRunBeginPlay` 的条件判断（行 4482）：___________

---

## 文件 4：`Runtime/CoreUObject/Private/Serialization/AsyncLoading2.cpp`

**对应模块 E — 资产与加载**

**核心类层次**：

```
FAsyncLoadingThread2          // 整个异步加载系统主控 (行 4147)
  FAsyncPackage2              // 一个"正在加载中的包"状态机 (行 3559)
    FEventLoadNode2           // 加载图中的节点 (有 barrier 计数) (行 2990)
  FGlobalImportStore          // 已导入对象的全局缓存 (行 1651)
  IoDispatcher（外部）        // 底层 I/O 线程，异步读取包数据
```

**入口调用链**：

```
LoadPackageAsync(PackagePath, Delegate)
  → FAsyncLoadingThread2::LoadPackageInternal(...)   // 行 4504; 实现 11541
    → FindOrInsertPackage(...)
      → FAsyncPackage2 进入事件驱动加载图
        IoDispatcher 异步读包数据
        反序列化 Exports → 创建 UObject 实例
        CompletionCallback 在 GameThread 端触发
```

### Q1 执行真正开始时刻

> **【学员填写】**
>
> 提示：`LoadPackageAsync` 在哪条线程调用？立即返回还是阻塞？实际加载由哪条专用线程驱动？`PostLoad` 回调在哪里执行？

### Q2 生命周期拥有者

> **【学员填写】**
>
> 提示：加载完成的 `UPackage` 由谁管理？`FAsyncPackage2` 等中间状态由谁管理？加载完成后如何释放？

### Q3 涉及哪些 Named Thread

> **【学员填写】**
>
> 提示：`LoadPackageAsync` 调用（GameThread）→ 实际加载（ALT，专用后台线程）→ I/O（IoDispatcher 线程）→ 回调（GameThread）。ALT 是 TaskGraph worker 吗？

### Q4 涉及 UObject 时 GC 可见性

> **【学员填写】**
>
> 提示：`FPackageReferencer` 在加载期间持有对 `UPackage` 的引用防止 GC 回收。加载完成后调用方如何维持可见性（`FStreamableHandle` / `TSoftObjectPtr`）？

### Q5 最像我哪个自写练习 + 差异

> **【学员填写】**
>
> 提示：最像 E3（`FStreamableManager::RequestAsyncLoad`）。E3 是面向用户的 Handle API，`AsyncLoading2.cpp` 展示的是最底层实现——IoDispatcher 驱动的事件节点状态机。差异最大的一点是？

### 与对应 mini 实现的 5 处对照点

> **【学员填写】**
>
> 对照 ExE3_Streamable，找出以下 5 处对应关系：
>
> 1. E3 的 `FStreamableManager::RequestAsyncLoad` ↔ 官方最终调用到 ___________
> 2. E3 的 `FStreamableHandle` ↔ 官方的 `FPackageReferencer` 的职责对比：___________
> 3. E3 的加载完成回调（GameThread）↔ 官方的 `ProcessLoadedPackagesFromGameThread`（行 9897）触发时机：___________
> 4. E3 你看到的是"用户 API 层"，`AsyncLoading2.cpp` 展示的是"底层状态机层"，中间还有哪一层？___________
> 5. 为什么 `AsyncLoading2.cpp` 里的 `FAsyncPackage2` 不是 `UObject`，而 `UPackage` 是？各自的生命周期管理方式：___________

---

## 文件 5：`Runtime/RenderCore/Public/RenderGraphBuilder.h` + `Private/RenderGraphBuilder.cpp`

**对应模块 H — RenderGraph 与场景渲染**

**三阶段架构**（与 stdexec sender 类比）：

```
描述期（AddPass 调用时）:
  FRDGBuilder::AddPass(Name, ParameterStruct, Flags, ExecuteLambda)
    → AddPassInternal(...)
      Pass 节点加入 Passes 数组
      执行 Lambda 不被调用（仅存储）

编译期（Execute 内部）:
  FRDGBuilder::Compile()                   // RenderGraphBuilder.cpp 1316行
    Pass 依赖分析（SetupPassDependencies）
    Pass Culling（深度优先搜索）
    Barrier 计算（CompilePassBarriers）
    资源生命周期分析（transient 起止 pass）

执行期（Execute 末段）:
  FRDGBuilder::Execute()                   // RenderGraphBuilder.cpp 1755行
    WaitForParallelSetupTasks(...)
    Compile()
    AllocateTransientResources(...)        // transient 资源分配显存
    ExecutePasses(...)                     // 逐 pass 录制 RHI 命令
    BeginFlushResourcesRHI()              // 推送命令到 RHIThread
```

### Q1 执行真正开始时刻

> **【学员填写】**
>
> 提示：`FRDGBuilder::Execute()` 在哪条线程调用？`AddPass` 时 pass lambda 执行吗？`Execute` 内 Compile + 分配 + 录制才真正开始，这与 stdexec 的哪个概念类比？

### Q2 生命周期拥有者

> **【学员填写】**
>
> 提示：`FRDGBuilder` 是栈上对象（header 注释行 44）。Transient 资源由谁管理？`FRDGBuilder` 析构时若未调用 `Execute` 会发生什么？

### Q3 涉及哪些 Named Thread

> **【学员填写】**
>
> 提示：`AddPass` 和 `Execute` 都在哪条线程？`AddSetupTask` 投递到哪里？最终 GPU 命令通过什么提交到 RHIThread？

### Q4 涉及 UObject 时 GC 可见性

> **【学员填写】**
>
> 提示：RDG 中资源句柄是 `FRDGTextureRef` 等非 UObject 类型。若 pass lambda 内需要访问 `UTexture2D` 的底层数据，应该在哪条线程读取，为什么不能在 pass lambda 里持有 `UObject*`？

### Q5 最像我哪个自写练习 + 差异

> **【学员填写】**
>
> 提示：最像 H3（手写 RDG Pass）和 H1（RDG 惰性图与 stdexec 类比）。H3 只写了单个 pass，`RenderGraphBuilder.cpp` 的 `Execute` 内部展示了全图 Culling / barrier / transient 分配——H3 练习里完全透明的那些步骤。

### 与对应 mini 实现的 5 处对照点

> **【学员填写】**
>
> 对照 ExH3_ComputePass 和 ExH1_RDGDescribe，找出以下 5 处对应关系：
>
> 1. H3 的 `FRDGBuilder::AddPass(...)` ↔ 官方 `AddPassInternal` 里 pass lambda 存储在 ___________
> 2. H1 的"描述期不执行"类比 ↔ 官方 `Execute` 里 `Compile()` 做的第一件事（Pass Culling）的意义：___________
> 3. H3 的 `SHADER_PARAMETER_RDG_TEXTURE` ↔ 官方 `CompilePassBarriers` 如何利用这些声明自动插入 barrier：___________
> 4. H3 的 transient texture 在 pass 结束后自动回收 ↔ 官方 `AllocateTransientResources` 中"生命周期分析"的起止 pass 如何确定：___________
> 5. H3 的 `GraphBuilder.Execute()` ↔ 官方 `BeginFlushResourcesRHI()` 做了什么，与 RHIThread 的关系：___________

---

## 文件 6：`Runtime/Renderer/Private/SceneRendering.cpp`

**对应模块 H — RenderGraph 与场景渲染（RDG 调用入口视角）**

**`FRendererModule::BeginRenderingViewFamilies` 骨架**（行 5039）：

```
BeginRenderingViewFamilies(Canvas, ViewFamilies)      // 行 5039
  World->SendAllEndOfFrameUpdates()                   // GameThread 侧最后一次 Scene 数据更新
  ENQUEUE_RENDER_COMMAND(UpdateStateStream)(...)      // 向 RenderThread 投递 StateStream 更新
  Scene->IncrementFrameNumber()
  ENQUEUE_RENDER_COMMAND(BeginRenderingViewFamily)(...)
    → FSceneRenderer::CreateSceneRenderer(ViewFamily) // RenderThread 侧创建渲染器
    → SceneRenderer->Render(RHICmdList)
      // 内部构建 FRDGBuilder，AddPass 多次，最后 Execute()
```

**双对象镜像**：`UPrimitiveComponent`（GameThread）↔ `FPrimitiveSceneProxy`（RenderThread）—— `RegisterAllComponents` 时向 RenderThread 投递创建 Proxy 的命令，`FScene` 持有所有 Proxy。

### Q1 执行真正开始时刻

> **【学员填写】**
>
> 提示：`BeginRenderingViewFamilies` 在哪条线程的帧末执行？`ENQUEUE_RENDER_COMMAND` 之后实际渲染工作在哪条线程？通常滞后 GameThread 约几帧？

### Q2 生命周期拥有者

> **【学员填写】**
>
> 提示：`FScene` 由谁拥有？`FSceneRenderer` 由谁创建并销毁？`FPrimitiveSceneProxy` 由谁持有，何时被删除？

### Q3 涉及哪些 Named Thread

> **【学员填写】**
>
> 提示：`SendAllEndOfFrameUpdates`（GameThread）→ `ENQUEUE_RENDER_COMMAND`（投递）→ `Render`（RenderThread）→ `Execute`（RenderThread）→ 命令提交（RHIThread）。这是 UE 三线程模型最完整的一条路径。

### Q4 涉及 UObject 时 GC 可见性

> **【学员填写】**
>
> 提示：`FScene`、`FSceneRenderer`、`FPrimitiveSceneProxy` 均是纯 C++ 对象（非 UObject）。GameThread↔RenderThread 通信靠 `ENQUEUE_RENDER_COMMAND` 值捕获 POD 数据——为什么不能传 `UObject*`？

### Q5 最像我哪个自写练习 + 差异

> **【学员填写】**
>
> 提示：最像 H2（FScene 双对象镜像）和 F4（`ENQUEUE_RENDER_COMMAND` 值捕获实验）。H2 你看到了 Proxy 的创建，`SceneRendering.cpp` 展示了 Proxy 被 `FSceneRenderer::Render` 消费的过程——这是 Proxy 存在的目的。

### 与对应 mini 实现的 5 处对照点

> **【学员填写】**
>
> 对照 ExH2_SceneProxy 和 ExF4_RenderCmd，找出以下 5 处对应关系：
>
> 1. F4 的 `ENQUEUE_RENDER_COMMAND` 值捕获 ↔ 官方宏展开后生成 `TEnqueueUniqueRenderCommandType`，与 F4 的差异：___________
> 2. H2 的 `FPrimitiveSceneProxy` 创建时机 ↔ 官方 `RegisterAllComponents` 触发的具体函数名：___________
> 3. H2 的"双对象镜像"概念 ↔ 官方 `UPrimitiveComponent` 与 `FPrimitiveSceneProxy` 之间数据同步的机制（`ENQUEUE` vs 直接调用）：___________
> 4. F4 的 `check(IsInRenderingThread())` ↔ 官方 `SceneRenderer->Render` 开头是否有类似断言？目的是什么？___________
> 5. Cap1 的 `FRDGBuilder` 三 pass ↔ 官方 `FDeferredShadingSceneRenderer::Render` 内部构建 `FRDGBuilder` 的调用位置（在哪个文件，不在 SceneRendering.cpp）：___________

---

## 最终「一页综述」

> **【学员填写】**
>
> 合上教材和源码，用自己的话描述 UE 引擎内核的骨架（目标：约 500 字，覆盖以下四层）：
>
> **第一层：Module 组织**
> （填写：Module 是什么、`IMPLEMENT_MODULE` 宏做了什么、`FModuleManager` 如何管理生命周期、Plugin 与 Module 的关系）
>
> **第二层：UObject 反射与 GC**
> （填写：UHT 扫描什么产生什么、`NewObject<T>()` 做了什么、GC 标记-清除的两阶段、`TObjectPtr<T>` 的作用、`SpawnActor` 五钩子顺序）
>
> **第三层：三线程分线**
> （填写：GameThread / RenderThread / RHIThread 各负责什么、跨线程通信的唯一安全路径、`BeginRenderingViewFamilies` 是哪条线程的帧末入口、异步加载的第四条路径是什么）
>
> **第四层：RDG 延迟图 + Subsystem 层级单例**
> （填写：`AddPass` 为何不立即执行、`Execute` 内部三阶段、与 stdexec sender 的类比、Subsystem 如何解决"全局唯一对象的生命周期"问题、为何比裸全局单例安全）

---

## 验收点

- [ ] **6×5 对照表完整**：30 格（6 文件 × 5 问）均已填写，每格含具体函数名或机制描述，不以"见上"代替
- [ ] **mini 实现对照表**：6 个文件各列出 5 处对照点（共 30 处），每处标注官方文件位置（函数段落名），说明与 mini 练习的核心差异
- [ ] **一页综述**：四层（Module / UObject+GC / 三线程 / RDG+Subsystem）均有独立段落，约 500 字
- [ ] **路径修正确认**：`AsyncLoading2.cpp` 使用 `Runtime/CoreUObject/Private/Serialization/` 路径，已在阅读清单开头标注
- [ ] **源码引用真实性**：所有引用的函数名通过本机 UE 5.7 源码验证，无虚构

## 提示

- `ModuleManager.cpp` 超过 2000 行，重点只在 `LoadModuleWithFailureReason`（行 965）和 `InitializeModule` lambda（行 1061）
- `AsyncLoading2.cpp` 超过 5000 行，第一次阅读只需找 `FAsyncLoadingThread2` 类声明（行 4147）和 `LoadPackageInternal` 函数（行 4504）
- `RenderGraphBuilder.cpp` 建议先读 `Compile()`（行 1316）理解图分析，再读 `Execute()`（行 1755）的资源分配和 pass 录制段
- `SceneRendering.cpp` 的 `BeginRenderingViewFamilies`（行 5039）只有约 100 行，是六个文件中最易读的入口

## 常见坑

- `AsyncLoading2.cpp` 里的 `FAsyncPackage2` 不要和旧版 `FAsyncPackage`（`AsyncLoading.cpp`）混淆——两套系统并存，UE 5.x 默认走 AsyncLoading2
- `CreateDefaultSubobject` 只能在构造函数中调用（`FObjectInitializer` 栈存在的窗口期），读代码时若看到非构造函数里调用它，确认是否在 CDO 构造链中
- `FRDGBuilder` 必须在栈上创建，`AddPass` 后必须调用 `Execute()`，否则析构时触发断言
- 五钩子顺序中 `BeginPlay` 并非总在 `PostInitializeComponents` 后立即调用——`World->HasBegunPlay()` 为 false 时会延迟，注意 `bRunBeginPlay` 条件分支（`Actor.cpp` 行 4482）

## 对应官方参考

- `Engine/Source/Runtime/Core/Private/Modules/ModuleManager.cpp`（行 908–1096）
- `Engine/Source/Runtime/CoreUObject/Public/UObject/Object.h`（行 102–1186）
- `Engine/Source/Runtime/Engine/Private/Actor.cpp`（行 4249–4753）
- `Engine/Source/Runtime/CoreUObject/Private/Serialization/AsyncLoading2.cpp`（行 4147–11541）
- `Engine/Source/Runtime/RenderCore/Public/RenderGraphBuilder.h` + `Private/RenderGraphBuilder.cpp`（Compile 行 1316，Execute 行 1755）
- `Engine/Source/Runtime/Renderer/Private/SceneRendering.cpp`（行 5039）
