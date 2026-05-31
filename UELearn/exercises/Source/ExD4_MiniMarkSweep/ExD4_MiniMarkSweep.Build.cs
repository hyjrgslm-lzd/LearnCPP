// 对应章节: ../../../05-模块D-UObject-反射-GC.md §练习 D-4
// 本题依赖: Core only —— 纯自研 mini mark-sweep, 不依赖 UObject 反射体系
// 这是 D 模块的 implement-own 层级习题: 手写 GC, 不使用任何 UE GC/UObject API
using UnrealBuildTool;

public class ExD4_MiniMarkSweep : ModuleRules
{
    public ExD4_MiniMarkSweep(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core"
            // 故意只依赖 Core —— 演示 GC 原理不需要 UObject 体系
            // 学员实验: 尝试加 CoreUObject, 观察 D4 的 mini-impl 与真实 GC 的层级差异
        });

        // 本题的 MiniObject / MiniGC 是纯 C++ struct/class
        // 使用 TArray (Core) 而非 std::vector, 保持 UE 风格
    }
}
