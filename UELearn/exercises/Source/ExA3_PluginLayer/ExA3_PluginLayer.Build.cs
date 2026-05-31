// 对应章节: ../../../02-模块A-构建系统与模块生命周期.md §练习 A-3
using UnrealBuildTool;

public class ExA3_PluginLayer : ModuleRules
{
    public ExA3_PluginLayer(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core",
            "Projects"  // IPluginManager::Get().LoadModulesForEnabledPlugins / GetLastCompletedLoadingPhase
        });

        // PrivateDependencyModuleNames: 按需追加
        // 注意: 本题演示"运行时动态加载解耦"，不应在 .Build.cs 里声明对服务方 Module 的编译期依赖
        // DynamicallyLoadedModuleNames.Add("...") 仅告知 UBT 该 Module 会被运行时加载，不建立编译期依赖
    }
}
