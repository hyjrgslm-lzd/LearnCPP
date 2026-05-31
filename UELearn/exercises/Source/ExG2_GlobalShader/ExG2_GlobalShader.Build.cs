// 对应章节: ../../../09-模块G-RHI与着色器.md §练习 G-2
using UnrealBuildTool;
using System.IO;

public class ExG2_GlobalShader : ModuleRules
{
    public ExG2_GlobalShader(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        // G2 需要 Core + RenderCore（FGlobalShader / IMPLEMENT_GLOBAL_SHADER / AddShaderSourceDirectoryMapping）
        //       + RHI（FRHICommandListImmediate / FBufferRHIRef）
        //       + Renderer（FComputeShaderUtils::Dispatch）
        //       + Projects（FPaths::ProjectDir / IPluginManager — 着色器路径映射需要 Projects 模块）
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
