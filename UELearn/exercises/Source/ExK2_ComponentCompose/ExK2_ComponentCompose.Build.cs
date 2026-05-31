// 对应章节: ../../../06-模块K-Actor-Component-World-Subsystem.md §练习 K-2
using UnrealBuildTool;

public class ExK2_ComponentCompose : ModuleRules
{
    public ExK2_ComponentCompose(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core",
            "CoreUObject",
            "Engine"
        });
    }
}
