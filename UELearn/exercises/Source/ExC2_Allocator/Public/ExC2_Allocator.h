// 对应章节: ../../../04-模块C-内存-智能指针-委托.md §练习 C-2
// 小节: FMemory::Malloc 调用链 / FMallocBinned2 分桶策略 / TInlineAllocator / LLM_SCOPE
#pragma once

#include "CoreMinimal.h"
#include "HAL/UnrealMemory.h"         // FMemory::Malloc / Free
#include "HAL/LowLevelMemTracker.h"   // LLM_SCOPE / LLM_SCOPE_BYNAME / ELLMTag
#include "Stats/Stats.h"              // DECLARE_MEMORY_STAT / INC_MEMORY_STAT_BY

// ══════════════════════════════════════════════════════
// 自定义内存统计宏 (在 stat Memory 面板可见)
// 参考: Engine/Source/Runtime/Core/Public/Stats/Stats.h
// ══════════════════════════════════════════════════════
DECLARE_MEMORY_STAT_EXTERN(TEXT("ExC2 Plugin Memory"), STAT_ExC2PluginMemory, STATGROUP_Memory, );

// ══════════════════════════════════════════════════════
// 演示 TInlineAllocator: 8 个 FVector 以内不分配堆内存
// 参考: Engine/Source/Runtime/Core/Public/Containers/ContainerAllocationPolicies.h
// ══════════════════════════════════════════════════════
using FSmallVertexArray = TArray<FVector, TInlineAllocator<8>>;

// ══════════════════════════════════════════════════════
// 工具函数声明
// ══════════════════════════════════════════════════════

// 追踪 FMemory::Malloc 调用链, 在日志中打印分配步骤
void ExC2_TraceMallocChain();

// 演示 TInlineAllocator 与普通 TArray 的分配行为差异
void ExC2_DemoInlineAllocator();

// 演示 LLM_SCOPE 标签 (仅在 ENABLE_LOW_LEVEL_MEM_TRACKER=1 的构建下可见)
void ExC2_DemoLLMScope();
