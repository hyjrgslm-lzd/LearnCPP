// 对应章节: ../../../04-模块C-内存-智能指针-委托.md §练习 C-2
// 本题依赖: Core —— FMemory::Malloc/Free, TInlineAllocator, LLM_SCOPE 均在 Core
using UnrealBuildTool;

public class ExC2_Allocator : ModuleRules
{
    public ExC2_Allocator(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core"
            // Stats 宏 (DECLARE_MEMORY_STAT / INC_MEMORY_STAT_BY) 也在 Core
            // LowLevelMemTracker (LLM_SCOPE) 在 Core/HAL/LowLevelMemTracker.h
        });
    }
}
