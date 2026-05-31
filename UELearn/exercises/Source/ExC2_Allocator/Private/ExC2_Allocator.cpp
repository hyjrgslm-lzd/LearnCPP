// ============================================================
// 对应章节: ../../../04-模块C-内存-智能指针-委托.md §练习 C-2
// C++ 标准要求: C++20
// 本题目标: 追踪 FMemory::Malloc 调用链到 FMallocBinned2;
//           理解分桶策略 (小分配走池, 大分配走 OS);
//           演示 TInlineAllocator<8> 的堆分配时机;
//           用 LLM_SCOPE 和 INC_MEMORY_STAT_BY 观察内存统计
//
// 骨架阶段预期行为: Module 注册, 打印关键数字注释
// 完成后预期行为 (PIE 启动日志):
//   LogExC2: FMemory::Malloc(64) 调用成功, Ptr != nullptr
//   LogExC2: TInlineAllocator<8>: 8 元素无堆分配; 第 9 元素触发堆分配
//   LogExC2: LLM_SCOPE(ELLMTag::Meshes) 分配已标记 (需 -llm 启动参数)
// ============================================================

#include "ExC2_Allocator.h"
#include "Modules/ModuleManager.h"
#include "Math/Vector.h"
#include "Stats/Stats.h"

// 参考源码:
//   Engine/Source/Runtime/Core/Public/HAL/UnrealMemory.h
//     └── FMemory::Malloc(SIZE_T Count, uint32 Alignment) — 全局入口
//   Engine/Source/Runtime/Core/Public/HAL/MemoryBase.h
//     └── GMalloc 全局指针 (UE5.6 起移入 UE::Private, 请通过 FMemory::Malloc 调用)
//   Engine/Source/Runtime/Core/Public/HAL/MallocBinned2.h
//     └── UE_MB2_MAX_SMALL_POOL_SIZE = 32768 - 16 = 32752 字节 (小于此走分桶池)
//     └── UE_MB2_SMALL_POOL_COUNT    = 51      (51 个大小类, 减少碎片)
//     └── UE_MB2_LARGE_ALLOC         = 65536   (大分配对齐到 OS 页)
//   Engine/Source/Runtime/Core/Public/HAL/LowLevelMemTracker.h
//     └── LLM_SCOPE(ELLMTag::...) — RAII 追踪当前分配归属的内存类别
//     └── ENABLE_LOW_LEVEL_MEM_TRACKER: 非 Shipping 构建默认开启; Shipping 关闭

IMPLEMENT_MODULE(FDefaultModuleImpl, ExC2_Allocator);

DEFINE_LOG_CATEGORY_STATIC(LogExC2, Log, All);

// 头文件已用 DECLARE_MEMORY_STAT_EXTERN 声明, 此处用 DEFINE_STAT 提供定义
// 配对规则: EXTERN 放头, DEFINE_STAT 放 cpp, 避免多 cpp 重复定义
DEFINE_STAT(STAT_ExC2PluginMemory);

// ──────────────────────────────────────────────────────────
// 工具函数实现
// ──────────────────────────────────────────────────────────

void ExC2_TraceMallocChain()
{
    // ══════════════════════════════════════════════════════
    // TODO [必做] 1: 追踪 FMemory::Malloc 调用链
    //   void* Ptr = FMemory::Malloc(64);
    //   ensureMsgf(Ptr != nullptr, TEXT("Malloc(64) 不应失败"));
    //   FMemory::Free(Ptr);
    //
    // 调用链 (通过 IDE 跳转到定义确认):
    //   FMemory::Malloc(64)
    //     → UnrealMemory.h: FORCENOINLINE CORE_API void* Malloc(SIZE_T, uint32)
    //       → (LLM 追踪层) LLM_PLATFORM_SCOPE(...)
    //         → GMalloc->Malloc(64, DEFAULT_ALIGNMENT)
    //           → FMallocBinned2::Malloc(64, ...)
    //             → 64 < 32752 → 走小池 (Binned2 分桶逻辑)
    //             → 从对应大小类的内存页中取一个空闲块
    // ══════════════════════════════════════════════════════

    UE_LOG(LogExC2, Log, TEXT("ExC2_TraceMallocChain: 请完成 TODO [必做] 1"));
    UE_LOG(LogExC2, Log, TEXT("  关键数字: MAX_SMALL_POOL_SIZE=32752, SMALL_POOL_COUNT=51, LARGE_ALLOC=65536"));
}

void ExC2_DemoInlineAllocator()
{
    // ══════════════════════════════════════════════════════
    // TODO [必做] 3: TInlineAllocator<8> 演示
    //
    //   FSmallVertexArray SmallVerts;  // 定义在 .h: TArray<FVector, TInlineAllocator<8>>
    //
    //   // 添加 8 个顶点 —— 全部存储在栈内联缓冲区, 无堆分配
    //   for (int32 i = 0; i < 8; ++i)
    //       SmallVerts.Add(FVector(i, 0, 0));
    //   UE_LOG(LogExC2, Log, TEXT("8 元素: Num=%d (应无堆分配)"), SmallVerts.Num());
    //
    //   // 添加第 9 个 —— 超出内联容量, 触发堆分配并迁移所有元素
    //   SmallVerts.Add(FVector(8, 0, 0));
    //   UE_LOG(LogExC2, Log, TEXT("9 元素: Num=%d (已触发堆分配)"), SmallVerts.Num());
    //
    // 对比:
    //   TArray<FVector>              —— 第 1 个元素就分配堆内存 (默认扩容策略)
    //   TArray<FVector, TFixedAllocator<8>> —— 超过 8 个时断言失败 (不扩容)
    // ══════════════════════════════════════════════════════

    UE_LOG(LogExC2, Log, TEXT("ExC2_DemoInlineAllocator: 请完成 TODO [必做] 3"));
}

void ExC2_DemoLLMScope()
{
    // ══════════════════════════════════════════════════════
    // TODO [进阶] 1: LLM_SCOPE 演示
    //   {
    //       LLM_SCOPE(ELLMTag::Meshes);  // 此作用域内的所有分配归入 "Meshes" 类别
    //       void* MeshMem = FMemory::Malloc(1024);
    //       // 在 Editor 控制台执行 "stat LLMFULL" 观察 Meshes 类目变化
    //       // 注意: 需要启动参数 -llm, 否则 LLM 不追踪
    //       FMemory::Free(MeshMem);
    //   }
    //
    // TODO [进阶] 2: 自定义 LLM Tag
    //   {
    //       LLM_SCOPE_BYNAME(TEXT("ExC2CustomTag"));
    //       void* CustomMem = FMemory::Malloc(256);
    //       FMemory::Free(CustomMem);
    //   }
    //
    // 注意: ENABLE_LOW_LEVEL_MEM_TRACKER 在 Shipping 构建下默认为 0
    //   → LLM_SCOPE 宏展开为空操作
    //   → stat LLMFULL 无输出
    // ══════════════════════════════════════════════════════

    UE_LOG(LogExC2, Log, TEXT("ExC2_DemoLLMScope: 请完成 TODO [进阶] 1 (需 -llm 启动参数)"));

    // ══════════════════════════════════════════════════════
    // TODO [必做] 4: INC/DEC_MEMORY_STAT_BY 演示
    //   constexpr int32 AllocSize = 512;
    //   void* Ptr = FMemory::Malloc(AllocSize);
    //   INC_MEMORY_STAT_BY(STAT_ExC2PluginMemory, AllocSize);
    //   // 在 Editor 的 "stat Memory" 面板中可以看到 "ExC2 Plugin Memory" 条目
    //   FMemory::Free(Ptr);
    //   DEC_MEMORY_STAT_BY(STAT_ExC2PluginMemory, AllocSize);
    // ══════════════════════════════════════════════════════
}


// ---- 验证区 (完成 TODO 后把下列 ensure 搬入对应代码路径) ----
// void* TestPtr = FMemory::Malloc(64);
// ensureMsgf(TestPtr != nullptr, TEXT("FMemory::Malloc(64) 不应返回 nullptr"));
// FMemory::Free(TestPtr);
//
// FSmallVertexArray V;
// for(int32 i=0;i<8;++i) V.Add(FVector::ZeroVector);
// ensureMsgf(V.Num() == 8, TEXT("TInlineAllocator<8> 应容纳 8 元素"));
// UE_LOG(LogExC2, Log, TEXT("ExC2 验证通过"));
