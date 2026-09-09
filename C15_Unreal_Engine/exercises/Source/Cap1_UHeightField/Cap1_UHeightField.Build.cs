// 对应章节: ../../../13-结课项目1-集成项目.md
using UnrealBuildTool;

public class Cap1_UHeightField : ModuleRules
{
    public Cap1_UHeightField(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        // Public: 向下游模块暴露类型签名
        // Core/CoreUObject/Engine 放 Public, 因为头文件里引用了这些模块的类型
        PublicDependencyModuleNames.AddRange(new string[] {
            "Core",
            "CoreUObject",
            "Engine"
        });

        // Private: 仅内部使用的渲染实现细节
        // RenderCore/RHI/RenderGraph 放 Private: 渲染细节不应泄露到调用方
        // 参考: 13-结课项目1 §5.4 模块依赖图 — RenderCore/RHI 应在 PrivateDependency
        PrivateDependencyModuleNames.AddRange(new string[] {
            "RenderCore",   // FRDGBuilder, ENQUEUE_RENDER_COMMAND, FGlobalShader
            "RHI"           // FRHITexture2D, FRHICommandList, FRHITextureCreateDesc
        });

        // ══════════════════════════════════════════════════════
        // TODO [进阶]: 若需要 FStreamableManager 用 AssetRegistry 注册 PrimaryAssetType
        // 取消注释以下行:
        // PrivateDependencyModuleNames.Add("AssetRegistry");
        // ══════════════════════════════════════════════════════
    }
}
