// 对应章节: ../../../10-模块H-RenderGraph与场景渲染.md §练习 H-1
using UnrealBuildTool;

public class ExH1_RDGDescribe : ModuleRules
{
    public ExH1_RDGDescribe(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core",
            "RenderCore",
            "RHI",
        });

        // 教学重点: 描述期 (AddPass) 与执行期 (Execute) 的分离
        // RenderGraph 模块提供 FRDGBuilder / FRDGTextureRef 等类型
        // RHI 提供 FRHICommandList / FRHICommandListImmediate
    }
}
