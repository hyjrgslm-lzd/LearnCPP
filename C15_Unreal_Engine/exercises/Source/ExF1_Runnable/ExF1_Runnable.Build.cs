// 对应章节: ../../../08-模块F-并发与任务图.md §练习 F-1
using UnrealBuildTool;

public class ExF1_Runnable : ModuleRules
{
    public ExF1_Runnable(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        // F1 只需 Core：FRunnable / FRunnableThread / FEvent / FThreadSafeBool 均在 Core
        PublicDependencyModuleNames.AddRange(new string[] {
            "Core"
        });
    }
}
