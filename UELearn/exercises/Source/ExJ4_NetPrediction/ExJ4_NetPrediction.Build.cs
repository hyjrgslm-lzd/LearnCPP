// 对应章节: ../../../12-模块J-网络与Replication.md §练习 J-4
using UnrealBuildTool;

public class ExJ4_NetPrediction : ModuleRules
{
    public ExJ4_NetPrediction(ReadOnlyTargetRules Target) : base(Target)
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
