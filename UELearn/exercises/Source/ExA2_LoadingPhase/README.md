> 对应章节: ../../../02-模块A-构建系统与模块生命周期.md §练习 A-2

## 目标

通过亲手把同一个 Module 放到不同 `LoadingPhase`，用日志对照观察"在哪个阶段什么东西变得可用"，把 `ELoadingPhase` 的语义从"枚举值列表"变成"有真实感知的时间线"。重点区分 UObject 系统的就绪边界与 `FCoreDelegates::OnPostEngineInit` 的订阅有效性。

## 前置理解

- 已完成 ExA1_HelloModule，理解 `IMPLEMENT_MODULE` 与 `FModuleManager` 的关系
- 已读 `02-模块A.md §练习 A-2`，熟悉 `ELoadingPhase` 完整枚举（`ModuleDescriptor.h:24-60`）
- 理解 UObject 系统的就绪边界：`GUObjectArray` 初始化在 `PreLoadingScreen` 到 `Default` 之间某时刻完成

## 必做任务

1. 修改 `StartupModule` 中 `PrintCurrentPhase` 的参数，使其与 `.uproject` 里实际的 `LoadingPhase` 一致
2. 完成 TODO 2：订阅 `FCoreDelegates::OnPostEngineInit`，用 lambda 打印回调触发日志
3. 分别将 `.uproject` 里 `ExA2_LoadingPhase` 的 `LoadingPhase` 改为以下值，各启动一次 Editor，对照日志填写结论表：
   - `"PostConfigInit"`、`"PreLoadingScreen"`、`"Default"`、`"PostEngineInit"`
4. 完成 TODO 3：制作 4 行 × 2 列对照表（UObject 就绪状态 + OnPostEngineInit 订阅有效性）
5. 观察将 `LoadingPhase` 改为 `"None"` 后 `StartupModule` 是否被调用

## 进阶任务

- 阅读 `ModuleManager.cpp:1078`，理解 `bCanProcessNewlyLoadedObjects` 的就绪边界，找到 `StartProcessingNewlyLoadedObjects` 在 `LaunchEngineLoop.cpp` 里的调用时机
- 在 `DefaultEngine.ini` 加 `[Core.Log] LogModuleManager=Verbose`，观察每个 Module 加载过程的详细日志

## 验收点

- [ ] 你有一张完整的对照表，至少包含 `PostConfigInit` / `Default` / `PostEngineInit` 三行，每行都有实测结果
- [ ] 能说出为什么绝大多数游戏 Module 应该用 `Default`，而不是 `PostConfigInit`
- [ ] 能说出 `None` phase 和 `PostEngineInit` phase 的区别
- [ ] 能答出四个固定复盘问题

## 观察点

- `ELoadingPhase` 是启动时间线的分段标记，不是"线程优先级"或"依赖权重"；两个 Module 都是 `Default` phase 时，加载顺序由 `.uproject` 声明顺序和依赖图共同决定
- `FModuleManager::bCanProcessNewlyLoadedObjects`（`ModuleManager.cpp:205`）分隔了"Module 里的 UObject 是否可被 GC 处理"的边界；`PostConfigInit` 阶段该位为 `false` 是正常的
- `StartupModule` 在 GameThread 上同步运行；在其中做重度 IO 或阻塞操作会直接卡住引擎启动

## 常见坑

- 在过早的 phase（如 `PostConfigInit`）访问 UObject 系统：调用 `NewObject<T>()` 或 `GetDefault<T>()` 大概率崩溃，因为 `GUObjectArray` 还未初始化完成
- 混淆 `LoadingPhase`（运行时加载顺序）与 `.Build.cs` 依赖声明（编译期符号可见性）：改了 `LoadingPhase` 不需要重新跑 UBT；改了 `.Build.cs` 必须重新编译
- `None` phase 下期望 `StartupModule` 被调用：`None` 表示"永远不自动加载"，必须有其他 Module 通过 `FModuleManager::Get().LoadModuleChecked` 显式加载

## 提示

- "什么时候什么东西可用"最可靠的参考来源是 `Engine/Source/Runtime/Launch/Private/LaunchEngineLoop.cpp`（`FEngineLoop::PreInit` 和 `FEngineLoop::Init`）：找到 `LoadModulesForProject` 和 `LoadModulesForEnabledPlugins` 的调用点就能精确定位每个 phase 在启动时间线上的位置
- 输出日志过滤 `LogExA2` 快速定位本题日志

## 复盘问题

1. **真正开始执行的时刻**？`StartupModule()` 的调用时机由 `ELoadingPhase` 决定；对 `Default` phase，是 `FEngineLoop::Init()` 主阶段 GameThread 上的同步调用；引擎主循环每个 phase 都调用 `IProjectManager::LoadModulesForProject(phase)` 和 `IPluginManager::LoadModulesForEnabledPlugins(phase)`，最终落到 `FModuleManager::LoadModule`。
2. **谁负责这个模块的生命周期**？Module 实例由 `FModuleManager` 的 `TUniquePtr<IModuleInterface>` 持有，生命周期由 `FModuleManager` 单例管理到引擎关闭。
3. **涉及哪些命名线程**？`StartupModule` 全部在 **GameThread** 上；`FModuleManager::WarnIfItWasntSafeToLoadHere`（`ModuleManager.cpp:157-163`）明确检查 `IsInGameThread()`，非 GameThread 调用 `LoadModule` 会产生警告。
4. **这对 GC 如何可见**？本题 Module 实例不是 UObject，不涉及 GC；在 `StartupModule` 里测试 `UObject::StaticClass()` 只是为了探测 UObject 子系统的就绪状态，不代表 Module 本身与 GC 有关联。
5. **本题专属**：在 `PostConfigInit` 的 `StartupModule` 里调用 `GetMutableDefault<UMySettings>()` 是否安全？（答：不安全，`Default` phase 之前 CDO 不一定已创建；应等到 `Default` 或更晚。）

## 对应官方参考

- `Engine/Source/Runtime/Projects/Public/ModuleDescriptor.h:24-60`：`ELoadingPhase` 完整枚举，含每个阶段的注释说明
- `Engine/Source/Runtime/Projects/Public/ModuleDescriptor.h:245`：`FModuleDescriptor::LoadModulesForPhase` 静态函数声明
- `Engine/Source/Runtime/Projects/Public/Interfaces/IPluginManager.h:303`：`IPluginManager::LoadModulesForEnabledPlugins`
- `Engine/Source/Runtime/Core/Private/Modules/ModuleManager.cpp:1078`：`bCanProcessNewlyLoadedObjects` 在 `StartupModule` 前的使用
