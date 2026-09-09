// 对应章节: ../../../10-模块H-RenderGraph与场景渲染.md §练习 H-3
using UnrealBuildTool;

public class ExH3_ComputePass : ModuleRules
{
    public ExH3_ComputePass(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core",
            "RenderCore",
            "RHI",
            "Projects",   // 提供 IPluginManager / AddShaderSourceDirectoryMapping
        });

        // 教学重点: 手写最小 RDG Compute Pass
        //   FExH3ComputeShader 继承 FGlobalShader
        //   FComputeShaderUtils::AddPass 一行完成 AddPass + Dispatch
        //   SHADER_PARAMETER_RDG_TEXTURE / UAV 宏驱动 barrier 零手写
        // Projects 依赖是为了 AddShaderSourceDirectoryMapping("/ExH3Shaders", ...)
    }
}
