// 对应章节: ../../../10-模块H-RenderGraph与场景渲染.md §练习 H-2
using UnrealBuildTool;

public class ExH2_SceneProxy : ModuleRules
{
    public ExH2_SceneProxy(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core",
            "CoreUObject",
            "Engine",
            "Renderer",
            "RenderCore",
        });

        // 教学重点: GameThread 侧 UPrimitiveComponent 与
        //           RenderThread 侧 FPrimitiveSceneProxy 双对象镜像
        // Engine 提供 UPrimitiveComponent / AActor
        // Renderer 提供 FPrimitiveSceneProxy (PrimitiveSceneProxy.h 在 Engine 但实现在 Renderer)
        // RenderCore 提供 FMeshElementCollector / ERHIAccess 等渲染辅助类型
    }
}
