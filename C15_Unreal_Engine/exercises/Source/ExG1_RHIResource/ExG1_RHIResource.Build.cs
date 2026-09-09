// 对应章节: ../../../09-模块G-RHI与着色器.md §练习 G-1
using UnrealBuildTool;

public class ExG1_RHIResource : ModuleRules
{
    public ExG1_RHIResource(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        // G1 需要 Core + RenderCore（ENQUEUE_RENDER_COMMAND）+ RHI（FBufferRHIRef / FRHICommandListImmediate / FRHIBufferCreateDesc）
        PublicDependencyModuleNames.AddRange(new string[] {
            "Core",
            "RenderCore",
            "RHI"
        });
    }
}
