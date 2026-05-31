// 对应章节: ../../../02-模块A-构建系统与模块生命周期.md §练习 A-1
using UnrealBuildTool;

public class ExA1_HelloModule : ModuleRules
{
    public ExA1_HelloModule(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core"
        });

        // PrivateDependencyModuleNames: A1 仅需 Core，无需其他模块
        // 进阶任务: 切换 PCHUsageMode.UseSharedPCHs 观察增量编译文件数量变化
    }
}
