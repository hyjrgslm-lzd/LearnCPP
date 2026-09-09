// 对应章节: ../../../08-模块F-并发与任务图.md §练习 F-3
using UnrealBuildTool;

public class ExF3_UETasks : ModuleRules
{
    public ExF3_UETasks(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        // F3 只需 Core：UE::Tasks::Launch / FPipe / TTask<T> 均在 Core
        PublicDependencyModuleNames.AddRange(new string[] {
            "Core"
        });
    }
}
