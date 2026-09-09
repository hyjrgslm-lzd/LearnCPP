> 对应章节: ../../../02-模块A-构建系统与模块生命周期.md §练习 A-3

## 目标

理解 Plugin 是 Module 的容器，通过 `IPluginManager` 观察插件层加载时序；亲手制造循环依赖并观察 UBT 报错；用"interface class + 运行时动态加载"标准方案打破循环依赖，把"依赖声明是强制的"这条规则变成肌肉记忆。

## 前置理解

- 已完成 ExA1_HelloModule 和 ExA2_LoadingPhase
- 已读 `02-模块A.md §练习 A-3` 前置理解节，熟悉 `PublicDependency` vs `PrivateDependency` 的可见性语义
- 理解 UHT 只处理含 `UCLASS`/`UINTERFACE` 宏的头文件；纯 C++ 接口不触发 UHT，不需要 `.generated.h`

## 必做任务

1. 完成 TODO 1：用 `IPluginManager::Get().FindPlugin` 查询 `EnhancedInput` 插件，打印其挂载状态
2. 完成 TODO 2：用 `FModuleManager::Get().IsModuleLoaded` + `LoadModuleChecked` 演示运行时动态加载，不在 `.Build.cs` 里声明编译期依赖
3. 完成 TODO 3：对比 Project Module 与 Plugin Module 在同一 `LoadingPhase` 下的加载顺序（Project 先，Plugin 后）
4. 进阶任务 1：新建两个模块制造循环依赖，记录 UBT `GenerateProjectFiles` 的报错信息关键字
5. 进阶任务 2：用 interface class + 运行时加载完成版本 3 的解耦实现

## 进阶任务

- 用 `DynamicallyLoadedModuleNames.Add("...")` 替代依赖声明，观察 UBT 对这个字段的处理（只确保打包，不建立编译期依赖）
- 把"获取服务"代码从 `StartupModule` 移到 `FAutoConsoleCommand` 回调里，观察调用时序的灵活性
- 将纯 C++ 接口改为 `UINTERFACE` 形式，观察 UHT 生成 `.generated.h` 的触发条件，以及 `#include "*.generated.h"` 必须作为最后一个 include 的规则

## 验收点

- [ ] 能用日志证明 Project Module 先于同 phase 的 Plugin Module 加载
- [ ] 有循环依赖版本（版本 1）的 UBT 报错记录
- [ ] 有 `PrivateDependency` 传递性失败（版本 2）的编译错误记录
- [ ] 有 interface + 运行时加载版本（版本 3）的成功运行日志
- [ ] 能答出四个固定复盘问题

## 观察点

- `FModuleManager::GetModuleChecked<T>` 的类型参数 `T` 必须是 `IModuleInterface` 的子类，Module 实现类须继承 `T`（`class FMyModule : public IModuleInterface, public IMyModuleInterface`），这是"Module 对外暴露服务"的标准做法
- `PrivateDependencyModuleNames` 的传递性规则：私有依赖不传递给消费方；避免"编译传染病"是 UBT 依赖图的设计意图
- UHT 只处理含 UE 宏的头文件：纯 C++ 接口 `class IA3BetaService {}` 不含 `UINTERFACE`，UHT 完全不处理它，不需要 `.generated.h`

## 常见坑

- `FModuleManager::GetModuleChecked<T>` 在 Module 未加载时 crash：必须先用 `IsModuleLoaded` 检查，或用 `LoadModuleChecked` 直接加载
- 接口头放在服务方 Module 的 `Public/` 下（而不是中立 Module）：消费方仍需声明对服务方的编译期依赖，循环没有真正打破；接口必须放在一个中立的"只含接口"的 Module 里
- `UINTERFACE` + `.generated.h` 顺序错误：`#include "*.generated.h"` 必须是头文件里最后一个 `#include`，否则 UHT 报解析错误

## 提示

- UBT 循环依赖报错出现在"生成项目文件"（`GenerateProjectFiles`）阶段，不在 C++ 编译阶段；去 `Saved/Logs/UnrealBuildTool-*.log` 找完整错误
- `IPluginManager.h` 在 `Engine/Source/Runtime/Projects/Public/Interfaces/IPluginManager.h`，`Get()` 静态方法在第 649 行

## 复盘问题

1. **真正开始执行的时刻**？`FA3PluginLayerModule::StartupModule` 在 `Default` phase 的 GameThread 上同步执行；`PrintEnabledPlugins` 里对 `IPluginManager` 的调用发生在同一调用栈上。
2. **谁负责这个模块的生命周期**？Module 实例由 `FModuleManager` 的 `TUniquePtr<IModuleInterface>` 独占持有；运行时通过接口指针交互时，接口指针指向服务方 Module 实例，服务方由 `FModuleManager` 保证在 `ShutdownModule` 前存活。
3. **涉及哪些命名线程**？全部在 **GameThread**；`FModuleManager::LoadModuleChecked` 的注释明确：非 GameThread 调用会产生线程安全警告。
4. **这对 GC 如何可见**？本题不涉及 UObject；纯 C++ 接口 `IA3BetaService` 不继承 `UObject`，其实现类的生命周期由 `FModuleManager` 管理，与 GC 完全无关；进阶任务里改成 `UINTERFACE` 后，`UA3BetaService`（继承 `UInterface`，后者继承 `UObject`）会进入 GC 系统，但 Module 实例本身仍不是 UObject。
5. **本题专属**：`PublicDependencyModuleNames` 和 `PrivateDependencyModuleNames` 中，哪个会影响到消费方的 include path？（答：`PublicDependencyModuleNames` 的依赖 Module 头路径会传递给消费方；`PrivateDependencyModuleNames` 不传递。）

## 对应官方参考

- `Engine/Source/Runtime/Core/Public/Modules/ModuleManager.h:247-277`：`LoadModule`、`LoadModuleChecked`、`LoadModuleWithFailureReason` 完整签名
- `Engine/Source/Runtime/Projects/Public/Interfaces/IPluginManager.h:273`：`IPluginManager` 类定义，`Get():649`，`LoadModulesForEnabledPlugins():303`
- `Engine/Source/Runtime/Projects/Public/PluginDescriptor.h`：`FPluginDescriptor` 结构体（`Modules`、`SupportedTargetPlatforms`、`EnabledByDefault`）
- `Engine/Source/Runtime/Projects/Projects.Build.cs`：Plugin-module 的 `.Build.cs` 样本，含 `PublicDependency` 和 `PrivateDependency` 对照写法
