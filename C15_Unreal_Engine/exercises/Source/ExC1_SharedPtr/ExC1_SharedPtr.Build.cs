// 对应章节: ../../../04-模块C-内存-智能指针-委托.md §练习 C-1
// 本题依赖: Core 即可 —— TSharedPtr/TWeakPtr/TUniquePtr 全部定义在 Core 模块
using UnrealBuildTool;

public class ExC1_SharedPtr : ModuleRules
{
    public ExC1_SharedPtr(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core"
            // CoreUObject/Engine 故意不加 —— 演示 TSharedPtr 与 GC 体系完全正交
            // 学员任务: 若要在 FMeshData 里存 UObject*, 需加 CoreUObject 并改为 UPROPERTY
        });

        // PrivateDependencyModuleNames: 本题不需要
    }
}
