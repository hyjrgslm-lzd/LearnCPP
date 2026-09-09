// 对应章节: ../../../14-结课项目2-源码对照项目.md
using UnrealBuildTool;

public class Cap2_SourceReading : ModuleRules
{
    public Cap2_SourceReading(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        // Cap2 以源码阅读和 .md 笔记为主体
        // 仅依赖 Core (骨架级, 最小占位)
        PublicDependencyModuleNames.AddRange(new string[] {
            "Core"
        });
    }
}
