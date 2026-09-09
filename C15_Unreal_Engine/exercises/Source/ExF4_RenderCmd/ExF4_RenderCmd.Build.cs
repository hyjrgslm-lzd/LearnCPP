// 对应章节: ../../../08-模块F-并发与任务图.md §练习 F-4
using UnrealBuildTool;

public class ExF4_RenderCmd : ModuleRules
{
    public ExF4_RenderCmd(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        // F4 需要 Core + RenderCore（ENQUEUE_RENDER_COMMAND / FlushRenderingCommands）+ RHI（FRHICommandListImmediate）
        PublicDependencyModuleNames.AddRange(new string[] {
            "Core",
            "RenderCore",
            "RHI"
        });
    }
}
