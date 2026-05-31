// 对应章节: ../../../09-模块G-RHI与着色器.md §练习 G-3
using UnrealBuildTool;

public class ExG3_RHIPass : ModuleRules
{
    public ExG3_RHIPass(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        // G3 需要 Core + RenderCore（FGlobalShader / GFilterVertexDeclaration / AddShaderSourceDirectoryMapping）
        //       + RHI（FRHICommandListImmediate / BeginRenderPass / Transition / DrawPrimitive）
        //       + Renderer（TStaticDepthStencilState 等 RHI 静态状态工具）
        //       + Projects（FPaths::ProjectDir — 着色器路径映射）
        PublicDependencyModuleNames.AddRange(new string[] {
            "Core",
            "RenderCore",
            "RHI"
        });

        PrivateDependencyModuleNames.AddRange(new string[] {
            "Renderer",
            "Projects"
        });
    }
}
