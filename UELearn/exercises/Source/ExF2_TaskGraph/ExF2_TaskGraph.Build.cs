// 对应章节: ../../../08-模块F-并发与任务图.md §练习 F-2
using UnrealBuildTool;

public class ExF2_TaskGraph : ModuleRules
{
    public ExF2_TaskGraph(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        // F2 只需 Core：FFunctionGraphTask / FTaskGraphInterface / ENamedThreads 均在 Core
        PublicDependencyModuleNames.AddRange(new string[] {
            "Core"
        });
    }
}
