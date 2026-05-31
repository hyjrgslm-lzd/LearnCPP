// 对应章节: ../../../07-模块E-资产与加载.md §练习 E-1
using UnrealBuildTool;

public class ExE1_Package : ModuleRules
{
    public ExE1_Package(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core",
            "CoreUObject"
        });
    }
}
