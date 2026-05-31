// 对应章节: ../../../12-模块J-网络与Replication.md §练习 J-2
using UnrealBuildTool;

public class ExJ2_Replication : ModuleRules
{
    public ExJ2_Replication(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core",
            "CoreUObject",
            "Engine",
            "NetCore"
        });

        // PrivateDependencyModuleNames: 按需追加
    }
}
