// 对应章节: ../../../12-模块J-网络与Replication.md §练习 J-3
using UnrealBuildTool;

public class ExJ3_NetSerialize : ModuleRules
{
    public ExJ3_NetSerialize(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core",
            "CoreUObject",
            "Engine",
            "NetCore"
        });
    }
}
