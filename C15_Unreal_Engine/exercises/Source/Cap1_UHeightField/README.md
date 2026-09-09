# 结课项目 1：UHeightField 运行时生成 + 异步流入 + GPU 后处理

> 对应章节: ../../../13-结课项目1-集成项目.md §结课项目 1
> UE 版本: 5.7 / C++ 基线: C++20

## 项目目标

把模块 A/B/C/D/K/E/F/G/H 已练会的每一块能力拼接成一个从数据生成到屏幕显示的完整纵向管线，在真实的 UE 多线程约束下跑通。这是全套课程里唯一要求你同时看见"九条线"的题目。

最终效果：在 PIE 内输入 Console 命令 `UELearn.Cap1.Run 256 128`，Output Log 出现 Step 1–10 完整日志序列，三张 PNG 落盘到 `Saved/Cap1/`。

## 贯穿覆盖表

| 模块 | 在本项目中的用途 |
|---|---|
| **A** | `Cap1_UHeightField.Build.cs` 声明五项依赖；理解 Public/Private 依赖的区别（RenderCore/RHI 在 Private 的原因） |
| **B** | `TArray<uint16>` 承载 CPU heightmap；`FString` 用于日志前缀格式化 `[Cap1][Step N]` |
| **C** | `TSharedPtr<FHeightFieldPreprocessResult, ESPMode::ThreadSafe>` 管理非 UObject 中间结果；跨线程值捕获传递 |
| **D** | `UHeightFieldAsset` 继承 `UObject`；`UPROPERTY` GC 可见性；CDO 与实例的区分；禁止裸 `new UObject` |
| **K** | `AHeightFieldViewer`（AActor）承载可视化；`UHeightFieldWorldSubsystem`（UWorldSubsystem）管理批处理队列 |
| **E** | `FSoftObjectPath` + `FStreamableManager::RequestAsyncLoad` 异步加载；`FStreamableHandle` 防止 GC 提前回收 |
| **F** | `UE::Tasks::Launch` 投递 CPU 预处理到 worker；`ENQUEUE_RENDER_COMMAND` 投递到 RenderThread；三次显式线程跨越 |
| **G** | `FRHITextureCreateDesc::Create2D` 创建 R32_FLOAT 纹理；`RHICmdList.LockTexture2D` + `UnlockTexture2D` 上传数据 |
| **H** | `FRDGBuilder` 三 pass 流水线：HeightField 采样 → 法线生成 → Gamma 校正；`BEGIN_SHADER_PARAMETER_STRUCT` 声明参数 |

## 必做任务

1. **（模块 A）** `Cap1_UHeightField.Build.cs`：Core/CoreUObject/Engine 放 `PublicDependencyModuleNames`，RenderCore/RHI/RenderGraph 放 `PrivateDependencyModuleNames`；能解释为何渲染依赖应放 Private

2. **（模块 D）** `UHeightFieldAsset`：继承 `UObject`，`UPROPERTY(EditAnywhere) int32 Size`、`int32 Resolution`、`TArray<uint16> Heights`；实现 `GenerateTestData` 用 sin/cos 生成渐变高度图

3. **（模块 C）** `FHeightFieldPreprocessResult`：普通 struct，不加 `UCLASS`；用 `MakeShared<FHeightFieldPreprocessResult, ESPMode::ThreadSafe>()` 创建；验证它不受 GC 管理

4. **（模块 K）** `UHeightFieldWorldSubsystem`：实现 `Initialize`（注册 Console 命令）、`Deinitialize`（注销命令、清理队列）、`OnWorldBeginPlay`（在此 SpawnActor，不要在 Initialize 里 SpawnActor）

5. **（模块 K）** `AHeightFieldViewer`：实现 `BeginPlay`（触发异步加载）、`EndPlay`（释放 LoadHandle）；成员 `TObjectPtr<UHeightFieldAsset> HeightFieldAsset` 加 `UPROPERTY`

6. **（模块 E）** 实现 `StartAsyncLoad`：用 `FStreamableManager::RequestAsyncLoad` 触发加载，保存 `TSharedPtr<FStreamableHandle>` 防加载被取消；回调里调用 `OnAssetLoaded`

7. **（模块 F — 线程跨越 1）** `OnAssetLoaded` 里用 `UE::Tasks::Launch` 投递 CPU 预处理到 worker；加注释 `// [Thread Crossing 1] GameThread → UE::Tasks worker`

8. **（模块 F — 线程跨越 2/3）** worker 完成后用 `AsyncTask(ENamedThreads::GameThread, ...)` 跳回 GameThread（跨越 2），再 `ENQUEUE_RENDER_COMMAND` 投递到 RenderThread（跨越 3）；加对应注释

9. **（模块 G）** RenderThread lambda 里：`FRHITextureCreateDesc::Create2D` + `RHICreateTexture`；`LockTexture2D` + `Memcpy` + `UnlockTexture2D` 上传 float 数组

10. **（模块 H）** 构建 `FRDGBuilder`，`AddPass` 三次（Pass1 HeightField 采样、Pass2 法线生成、Pass3 Gamma 校正），每 pass 用 `BEGIN_SHADER_PARAMETER_STRUCT`；`Execute()`；安排 Readback + PNG 落盘

## 进阶任务

- **网络扩展（串 J）**：`AHeightFieldViewer` 加 `bReplicates = true`，声明 `UPROPERTY(ReplicatedUsing=OnRep_HeightFieldAsset) TObjectPtr<UHeightFieldAsset>`，实现 Server 向 Client 同步 Asset 引用（参考 §八）
- **完整 RDG Shader**：为三个 pass 各写真实的 HLSL Global Shader（继承 `FGlobalShader`），在 `StartupModule` 里用 `AddShaderSourceDirectoryMapping` 注册 Shader 目录
- **AssetRegistry 注册**：将 `UHeightFieldAsset` 注册为 `PrimaryAssetType`，通过 `UAssetManager` 管理，演示与硬引用加载的区别

## 固定交付物

1. **三线程并行时序图（ASCII 或工具）**：三条泳道（GameThread / Worker / RenderThread），标出三次线程跨越点、`FStreamableHandle` 生命周期区间、`TSharedPtr<FHeightFieldPreprocessResult>` 跨线程存活区间
2. **RDG Pass 依赖图**：矩形 = pass，椭圆 = 纹理资源，标出 transient texture 生命周期（首次写入 pass → 最后读取 pass）
3. **对象生命周期图**：`UHeightFieldAsset`、`AHeightFieldViewer`、`UHeightFieldWorldSubsystem`、`TSharedPtr<FHeightFieldPreprocessResult>` 四个对象的完整弧
4. **模块依赖图**：Public/Private 依赖的节点图，说明为何 RenderCore/RHI 放 Private
5. **四固定问题书面回答**（见下方复盘问题 1-4）
6. **三张 PNG 截图**：`Saved/Cap1/pass1_heightfield.png`、`pass2_normal.png`、`pass3_gamma.png`，文件非空，三张图视觉上有可区分差异

## 验收点

- [ ] 编译通过，无 "Missing module dependency" 警告，`.generated.h` 正确生成
- [ ] PIE 内输入 `UELearn.Cap1.Run 256 128`，Output Log 出现 Step 1–10 完整日志序列，无乱序
- [ ] 三次线程跨越有代码注释 `// [Thread Crossing N]`，与时序图对应
- [ ] 约束全满足：`≥1 UPROPERTY 包装 UObject`、`≥1 TSharedPtr 非 UObject`、`≥1 AActor`、`≥1 UWorldSubsystem`、`禁 FlushRenderingCommands`、`禁裸 new UObject`、`禁裸 UObject* 成员`
- [ ] RDG 三 pass 流水线完整，每 pass 有独立 `BEGIN_SHADER_PARAMETER_STRUCT`，资源依赖通过 `FRDGTextureRef` 传递
- [ ] 三张 PNG 落盘，非空，视觉可区分
- [ ] 运行 5 次 Console 命令（累积 GC 压力），无 stale pointer crash
- [ ] 六件交付物齐备

## 观察点

- `UWorldSubsystem::Initialize` 与 `AHeightFieldViewer::BeginPlay` 的先后顺序：Subsystem::Initialize 更早（World 创建时），BeginPlay 更晚（World 进入 Play 后）——这决定了不能在 Initialize 里持有 Actor 引用
- `TSharedPtr<FHeightFieldPreprocessResult>` 的引用计数在跨线程传递时的变化：worker lambda 持有一份、ENQUEUE lambda 持有一份，两个 lambda 都执行完毕后引用计数才归零
- `FRDGBuilder` 是栈上对象，`AddPass` 时 lambda 不执行，`Execute()` 触发 Compile（全图依赖分析 + barrier 计算）+ 资源分配 + pass 录制——这与 stdexec 的 sender 惰性结构同构
- `ENQUEUE_RENDER_COMMAND` 的 lambda 值捕获要求：必须按值捕获，不能捕获 `UObject*` 裸指针（GC 可能在 RenderThread 执行之前就回收了 UObject）

## 常见坑

- **坑 1（模块 F）**：`ENQUEUE_RENDER_COMMAND` lambda 按引用捕获 GameThread 变量 → RenderThread 执行时栈帧已失效。对策：所有捕获按值，`TSharedPtr` 值捕获
- **坑 2（模块 D）**：`NewObject<UHeightFieldAsset>(nullptr)` 未 `AddToRoot` → GC 在异步加载期间提前回收。对策：`GetTransientPackage()` 作 Outer，或 `AddToRoot` + 加载完成后 `RemoveFromRoot`
- **坑 3（模块 H）**：RDG pass 的 `SHADER_PARAMETER_STRUCT` 遗漏 `RDG_TEXTURE` 宏 → RDG validation 报 `Missing resource`。对策：读取用 `SHADER_PARAMETER_RDG_TEXTURE`，写入用 `RENDER_TARGET_BINDING_SLOTS()`
- **坑 4（模块 H）**：PNG 截图全黑。原因 a：RT 格式不是 `PF_B8G8R8A8`；原因 b：Readback 在 `Execute()` 返回前调用。对策：见 §4.3 路径 A
- **坑 5（模块 K）**：在 `Initialize` 里 `SpawnActor` → World 未 BeginPlay，Level 未就绪，崩溃。对策：SpawnActor 移到 `OnWorldBeginPlay`
- **坑 6（模块 E）**：`FStreamableHandle` 未保存 → 加载被取消，回调不触发。对策：成员变量保存 handle，回调完成后 `Reset()`
- **坑 7（模块 F）**：`UE::Tasks::Launch` 的 lambda 捕获了 `this`（UObject）→ worker 线程访问 UObject 成员违反线程安全。对策：Launch 前提取好 POD 数据，按值传入 lambda

## 复盘问题（四固定问题 + 本项目专属）

1. **真正开始执行的时刻？** Console 命令 `UELearn.Cap1.Run` 触发时序：`IConsoleManager` 回调（GameThread）→ `NewObject<UHeightFieldAsset>` + `RequestAsyncLoad`（GameThread）→ CPU 预处理（`UE::Tasks::Launch` 后 worker 排到时）→ RHI 纹理上传（`ENQUEUE_RENDER_COMMAND` lambda 被 RenderThread 排到时）→ RDG Execute（RenderThread 调用 `FRDGBuilder::Execute()` 那一行）
2. **谁负责这些对象的生命周期？** `UHeightFieldAsset`（运行时）：`AddToRoot` → 加载完成后改由 `UPROPERTY TObjectPtr` 接管 → Actor 销毁时引用消失 → GC 回收。`AHeightFieldViewer`：`UWorld`（通过 `ULevel::Actors`）。`UHeightFieldWorldSubsystem`：Subsystem 框架自动管理，与 `UWorld` 同生命周期。`TSharedPtr<FHeightFieldPreprocessResult>`：引用计数归零（worker lambda + RenderThread lambda 均析构后）自动销毁
3. **涉及哪些命名线程？** `GameThread`（Asset 创建、RequestAsyncLoad、回调、Console 命令、PNG dump）；`TaskGraph worker pool`（CPU 预处理，由 `UE::Tasks::Launch` 投递）；`RenderThread`（RHI 上传、RDG AddPass × 3、Execute）；`RHIThread`（GPU 命令提交，由 RDG Execute 内部 `BeginFlushResourcesRHI` 触发，无需手动控制）
4. **涉及 UObject 时怎么对 GC 可见？** `UHeightFieldAsset`：`UPROPERTY TObjectPtr<UHeightFieldAsset>` + 正确 Outer（`GetTransientPackage()`）或 `AddToRoot`。`AHeightFieldViewer`：以 Level 为 Outer，通过 `ULevel::Actors`（`UPROPERTY`）可达。`UHeightFieldWorldSubsystem`：挂在 `UWorld::SubsystemCollection`，World 可达则 Subsystem 可达。`FHeightFieldPreprocessResult`：非 UObject，`TSharedPtr` 引用计数，GC 不追踪
5. **本项目专属：** 为什么 `TSharedPtr<FHeightFieldPreprocessResult>` 不能换成 `TObjectPtr<UHeightFieldPreprocessResult>`？即使把它改成 `UCLASS`，在 worker 线程里持有和访问它有什么问题？
6. **本项目专属：** `FRDGBuilder::Execute()` 调用之前，三个 pass 的 GPU 命令录制发生了吗？`FRDGTextureRef` 在 Execute 前指向真实的 `FRHITexture` 吗？

## 对应官方参考

- `Engine/Source/Runtime/RenderCore/Public/RenderGraphBuilder.h`：`FRDGBuilder` 栈上对象要求（44行注释）、`AddPass`、`Execute`
- `Engine/Source/Runtime/RHI/Public/RHICommandList.h`：`LockTexture2D`、`UnlockTexture2D`、`FRHITextureCreateDesc`
- `Engine/Source/Runtime/Engine/Classes/Engine/StreamableManager.h`：`RequestAsyncLoad`、`FStreamableHandle`
- `Engine/Source/Runtime/Engine/Public/Subsystems/WorldSubsystem.h`：`UWorldSubsystem` 生命周期钩子
- `Engine/Source/Runtime/Core/Public/Tasks/Task.h`：`UE::Tasks::Launch`
- `Engine/Source/Runtime/RenderCore/Public/RenderingThread.h`：`ENQUEUE_RENDER_COMMAND`
