// 对应章节: ../../../07-模块E-资产与加载.md §练习 E-2
using UnrealBuildTool;

public class ExE2_AssetRegistry : ModuleRules
{
    public ExE2_AssetRegistry(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core",
            "CoreUObject",
            "AssetRegistry"
        });
    }
}
