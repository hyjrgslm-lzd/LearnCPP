// 对应章节: ../../../03-模块B-核心容器与字符串.md §练习 B-1
using UnrealBuildTool;

public class ExB1_TArrayTMap : ModuleRules
{
    public ExB1_TArrayTMap(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core"  // TArray/TMap/TSet/FString 均在 Core 模块
        });

        // PrivateDependencyModuleNames: B1 仅需 Core，无需其他模块
    }
}
