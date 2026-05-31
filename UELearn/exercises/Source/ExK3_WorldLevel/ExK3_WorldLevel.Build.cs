// 对应章节: ../../../06-模块K-Actor-Component-World-Subsystem.md §练习 K-3
using UnrealBuildTool;

public class ExK3_WorldLevel : ModuleRules
{
    public ExK3_WorldLevel(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core",
            "CoreUObject",
            "Engine"
        });
    }
}
