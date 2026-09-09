> 对应章节: ../../../02-模块A-构建系统与模块生命周期.md §练习 A-1

## 目标

写出 UE 里最小可加载的 Module——不含任何 `UCLASS`，不触发 UHT，只有三件事：`.Build.cs`、`IModuleInterface` 的两个虚函数、`IMPLEMENT_MODULE` 宏。在引擎启动日志里找到 `StartupModule` 的打印，验证 Module 确实被加载了。

## 前置理解

- 已读 `01-心智模型.md` §Module-first，理解"代码归属 Module 才能被引擎感知"
- 理解 `IMPLEMENT_MODULE` 宏：展开后把初始化函数插入 `GFirstModuleInitializerEntry` 链表（`ModuleManager.cpp:98-119`）
- 理解 `Public/` vs `Private/` 的 include path 语义：`Public/` 头路径传递给消费方，`Private/` 只对本模块可见

## 必做任务

1. 补全 `StartupModule` 中的 TODO 1：用 `FModuleManager::Get().IsModuleLoaded` 验证本模块已加载并打印结果
2. 补全 TODO 2：加 `#if WITH_EDITOR` 守卫，打印 Editor-only 日志，理解 Shipping 构建中该段代码不会编译
3. 补全 `ShutdownModule` 中的 TODO 3：清理 StartupModule 中注册的任何资源或委托
4. 在 `C15_Unreal_Engine.uproject` 的 `Modules` 数组中确认 `ExA1_HelloModule` 已注册，`LoadingPhase` 为 `"Default"`
5. 启动 Editor，在输出日志搜索 `[A1]`，记录日志出现时机（在哪些 `LogInit` 条目之前/之后）

## 进阶任务

- 在 `Public/` 下新建 `IA1Service.h` 声明纯 C++ 接口，在 `Private/` 下新建实现；从外部 Module 尝试 include `Private/` 头，观察 UBT include path 报错
- 在 `.Build.cs` 里切换 `PCHUsage = PCHUsageMode.UseSharedPCHs`，对比首次增量编译的文件数量变化
- 重写 `SupportsDynamicReloading()` 返回 `false`，在 Editor 内触发 Live Coding，观察模块无法热重载

## 验收点

- [ ] `[A1] === ExA1_HelloModule StartupModule ===` 在 Editor 启动时出现，且只出现一次
- [ ] `[A1] ShutdownModule called` 在 Editor 关闭时出现
- [ ] 能说清 `IMPLEMENT_MODULE(FA1HelloModule, ExA1_HelloModule)` 中第二参数与 `.Build.cs` 文件名、`.uproject Modules[].Name` 三者必须完全一致的原因
- [ ] 能答出四个固定复盘问题

## 观察点

- `IMPLEMENT_MODULE` 不是魔法——它只是在静态初始化阶段往 `GFirstModuleInitializerEntry` 链表插入一个节点；`FModuleManager::LoadModuleWithFailureReason`（`ModuleManager.cpp:965`）遍历链表找到入口函数，再调用 `StartupModule()`
- `StartupModule` 在 `Default` phase 的调用时机：`FEngineLoop::Init()` 主阶段，配置文件已读完，渲染线程尚未完全启动
- UBT 不是 Makefile 包装——`Core.Build.cs` 里有真正的 C# `if/foreach`，可按平台动态增减依赖

## 常见坑

- `IMPLEMENT_MODULE` 第二参数拼写错误（如漏掉下划线）：引擎找不到初始化函数，报 `FileNotFound`，难以定位；三处命名（`.Build.cs` 文件名、宏第二参数、`.uproject Name`）必须完全一致
- 忘记在 `.uproject` 注册：Module 文件编译通过但 `StartupModule` 永远不被调用；检查方法：`FModuleManager::Get().IsModuleLoaded(TEXT("ExA1_HelloModule"))` 返回 `true` 才算真正加载
- `.Build.cs` 放到错误目录：必须和 `Public/`/`Private/` 目录处于同一层级（Module 根目录）

## 提示

- `Core.Build.cs`（`Engine/Source/Runtime/Core/Core.Build.cs`）是研究"真实 `.Build.cs` 长什么样"的最佳样本
- 输出日志过滤 `LogExA1` 可快速定位本题日志
- 如编译失败且报 `cannot open include file`，优先检查 `.Build.cs` 的依赖声明是否覆盖了所用头文件的 Module

## 复盘问题

1. **真正开始执行的时刻**？`StartupModule()` 在 `FModuleManager::LoadModuleWithFailureReason`（`ModuleManager.cpp:1087`）中被调用，对 `Default` phase 发生在 `FEngineLoop::Init()` 的主阶段，GameThread 上同步执行。
2. **谁负责这个模块的生命周期**？Module 实例由 `FModuleManager` 的 `TUniquePtr<IModuleInterface>` 持有（`ModuleManager.cpp:1066`），`FModuleManager` 本身是静态单例，在引擎关闭时通过 `TearDown()` 销毁。
3. **涉及哪些命名线程**？`StartupModule` 和 `ShutdownModule` 均在 **GameThread** 上同步调用；`WarnIfItWasntSafeToLoadHere`（`ModuleManager.cpp:159`）明确检查 `IsInGameThread()`。
4. **这对 GC 如何可见**？A1 不涉及 UObject，Module 实例本身由 `TUniquePtr` 管理，与 GC 完全无关；等模块 D 才引入两者的交叉点。
5. **本题专属**：`IMPLEMENT_MODULE` 宏做了哪两件事？①声明 `FA1HelloModule` 的静态初始化入口函数；②通过 `FModuleInitializerEntry` 把该函数注册到全局链表 `GFirstModuleInitializerEntry`。如果一个 Module 有多个 `.cpp` 文件，`IMPLEMENT_MODULE` 必须写在恰好一个 `.cpp` 里，否则链接期 `multiple definition` 报错。

## 对应官方参考

- `Engine/Source/Runtime/Core/Public/Modules/ModuleInterface.h`：`IModuleInterface` 完整定义（`StartupModule`、`ShutdownModule`、`SupportsDynamicReloading`、`SupportsAutomaticShutdown`）
- `Engine/Source/Runtime/Core/Public/Modules/ModuleManager.h`：`FModuleManager`、`FDefaultModuleImpl`、`IMPLEMENT_MODULE` 宏定义（第 933 行）
- `Engine/Source/Runtime/Core/Private/Modules/ModuleManager.cpp:98-119`：`GFirstModuleInitializerEntry` 链表与 `FModuleInitializerEntry` 构造
- `Engine/Source/Runtime/Core/Core.Build.cs`：真实引擎 Module 的 `.Build.cs` 样本
