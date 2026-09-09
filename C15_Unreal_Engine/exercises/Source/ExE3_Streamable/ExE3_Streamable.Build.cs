// 对应章节: ../../../07-模块E-资产与加载.md §练习 E-3
using UnrealBuildTool;

public class ExE3_Streamable : ModuleRules
{
    public ExE3_Streamable(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core",
            "CoreUObject",
            "Engine"
        });
    }
}
