// 对应章节: ../../../06-模块K-Actor-Component-World-Subsystem.md §练习 K-1
using UnrealBuildTool;

public class ExK1_HelloActor : ModuleRules
{
    public ExK1_HelloActor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core",
            "CoreUObject",
            "Engine"
        });
    }
}
