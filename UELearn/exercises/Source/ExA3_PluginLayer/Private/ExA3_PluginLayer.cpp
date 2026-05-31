// 对应章节: ../../../02-模块A-构建系统与模块生命周期.md §练习 A-3
// C++ 标准要求: C++20
// 本题目标: 观察 Plugin/Module 层级关系，演示循环依赖打破与运行时动态加载解耦
//
// 骨架阶段预期行为: 打印已启用插件列表，观察 IPluginManager 接口用法
// 完成后预期行为（Editor 启动日志）:
//   LogExA3: [A3] StartupModule called
//   LogExA3: [A3] 已启用插件数量: N
//   LogExA3: [A3] Plugin: EnhancedInput, Modules: 1
//   LogExA3: [A3] 运行时获取服务成功 (如果服务方 Module 已加载)

#include "ExA3_PluginLayer.h"
#include "Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"

// ══════════════════════════════════════════════════════
// 独立日志分类
// ══════════════════════════════════════════════════════
DEFINE_LOG_CATEGORY_STATIC(LogExA3, Log, All);

IMPLEMENT_MODULE(FA3PluginLayerModule, ExA3_PluginLayer);

// ══════════════════════════════════════════════════════
// PrintEnabledPlugins: 遍历 IPluginManager，打印已启用插件信息
// IPluginManager::Get() 来自 Engine/Source/Runtime/Projects/Public/Interfaces/IPluginManager.h:649
// ══════════════════════════════════════════════════════
void FA3PluginLayerModule::PrintEnabledPlugins()
{
    TArray<TSharedRef<IPlugin>> EnabledPlugins = IPluginManager::Get().GetEnabledPlugins();
    UE_LOG(LogExA3, Log, TEXT("[A3] 已启用插件数量: %d"), EnabledPlugins.Num());

    for (const TSharedRef<IPlugin>& Plugin : EnabledPlugins)
    {
        const FPluginDescriptor& Desc = Plugin->GetDescriptor();
        UE_LOG(LogExA3, Log, TEXT("[A3] Plugin: %s, Modules: %d"),
            *Plugin->GetName(), Desc.Modules.Num());
    }
}

void FA3PluginLayerModule::StartupModule()
{
    UE_LOG(LogExA3, Log, TEXT("[A3] StartupModule called"));

    PrintEnabledPlugins();

    // ══════════════════════════════════════════════════════
    // TODO [必做] 1: 查询特定 Plugin 是否已挂载，并打印其 LoadingPhase
    //   TSharedPtr<IPlugin> FoundPlugin = IPluginManager::Get().FindPlugin(TEXT("EnhancedInput"));
    //   if (FoundPlugin.IsValid())
    //   {
    //       UE_LOG(LogExA3, Log, TEXT("[A3] EnhancedInput 挂载状态: %d"),
    //           FoundPlugin->IsEnabled() ? 1 : 0);
    //   }
    // ══════════════════════════════════════════════════════

    // ══════════════════════════════════════════════════════
    // TODO [必做] 2: 演示运行时动态加载解耦（不依赖编译期依赖）
    //   用 FModuleManager::Get().IsModuleLoaded 检查某模块，再用 LoadModuleChecked 加载
    //   例如观察 "Projects" 模块的加载状态:
    //   bool bProjectsLoaded = FModuleManager::Get().IsModuleLoaded(TEXT("Projects"));
    //   UE_LOG(LogExA3, Log, TEXT("[A3] Projects 模块已加载: %d"), bProjectsLoaded ? 1 : 0);
    // ══════════════════════════════════════════════════════

    // ══════════════════════════════════════════════════════
    // TODO [必做] 3: 观察 Project Module 与 Plugin Module 的加载顺序差异
    //   在 .uproject 里把本 Module 和某 Plugin Module 设为同一 LoadingPhase，
    //   用日志对比谁先被加载（Project Module 先，Plugin Module 后）
    // ══════════════════════════════════════════════════════

    // ══════════════════════════════════════════════════════
    // TODO [进阶] 1: 制造并观察循环依赖报错
    //   新建 ExA3_ModuleAlpha 和 ExA3_ModuleBeta，互相在 PublicDependencyModuleNames 里声明对方，
    //   跑 UBT GenerateProjectFiles，记录报错信息关键字
    // ══════════════════════════════════════════════════════

    // ══════════════════════════════════════════════════════
    // TODO [进阶] 2: interface class + 运行时加载解耦（版本 3 正确做法）
    //   新建 ExA3_ModuleInterfaces 模块，在其 Public/ 声明纯 C++ 接口 IA3BetaService，
    //   ExA3_ModuleBeta 实现该接口，ExA3_ModuleAlpha 在运行时通过 GetModuleChecked<> 获取服务，
    //   ExA3_ModuleAlpha.Build.cs 里只依赖 ExA3_ModuleInterfaces，不依赖 ExA3_ModuleBeta
    // ══════════════════════════════════════════════════════
}

void FA3PluginLayerModule::ShutdownModule()
{
    UE_LOG(LogExA3, Log, TEXT("[A3] ShutdownModule called"));
}


// ---- 验证区 (完成 TODO 后把下列行搬入对应代码路径) ----
// ensureMsgf(IPluginManager::Get().GetEnabledPlugins().Num() > 0,
//     TEXT("[A3] 至少应有一个已启用插件"));
// UE_LOG(LogExA3, Log, TEXT("[A3] Plugin/Module 层级关系观察完成"));
