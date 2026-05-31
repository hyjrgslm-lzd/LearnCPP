// 对应章节: ../../../06-模块K-Actor-Component-World-Subsystem.md §练习 K-4
using UnrealBuildTool;

public class ExK4_CustomSubsystem : ModuleRules
{
    public ExK4_CustomSubsystem(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core",
            "CoreUObject",
            "Engine"
        });
    }
}
