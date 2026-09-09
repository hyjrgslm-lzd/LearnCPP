// ============================================================
// 对应章节: ../../../05-模块D-UObject-反射-GC.md §练习 D-4
// C++ 标准要求: C++20
// 本题目标: 手写 mini mark-sweep GC (implement-own, 纯 C++, 不依赖 UObject);
//           理解 RootSet / 可达性传播 / Sweep 顺序;
//           模拟增量 GC (IncrementalMarkStep) 与 write barrier 的关系
//
// 骨架阶段预期行为: Module 注册, GMiniObjectArray 初始化
// 完成后预期行为 (PIE 启动日志):
//   LogExD4: --- GC Round 1 (all reachable) ---
//   LogExD4: [MiniGC] === GC Cycle Start ===
//   LogExD4: [MiniGC] === GC Cycle End (4 objects alive) ===
//   LogExD4: --- GC Round 2 (C unreachable after losing UPROPERTY) ---
//   LogExD4: [MiniGC::Sweep] Collecting: C
//   LogExD4: [MiniGC] === GC Cycle End (3 objects alive) ===
//   LogExD4: --- GC Round 3 (cleanup) ---
//   LogExD4: [MiniGC::Sweep] Collecting: A, B, D
// ============================================================

#include "ExD4_MiniMarkSweep.h"
#include "Modules/ModuleManager.h"

// 参考源码:
//   Engine/Source/Runtime/CoreUObject/Public/UObject/UObjectBaseUtility.h:206
//     └── AddToRoot() → GUObjectArray.IndexToObject(InternalIndex)->SetRootSet()
//   Engine/Source/Runtime/CoreUObject/Private/UObject/GarbageCollection.cpp:4528
//     └── PerformReachabilityAnalysis: while(true) 主循环 + IsSuspended() 增量中断
//   Engine/Source/Runtime/CoreUObject/Private/UObject/GarbageCollection.cpp:634
//     └── GRoots (TSet<int32>) —— 根集
//   Engine/Source/Runtime/CoreUObject/Public/UObject/ObjectPtr.h:306
//     └── ConditionallyMarkAsReachable —— 增量 GC write barrier
//         增量 Mark 期间新建 TObjectPtr → 立即标记目标为可达, 防止漏标

IMPLEMENT_MODULE(FDefaultModuleImpl, ExD4_MiniMarkSweep);

DEFINE_LOG_CATEGORY_STATIC(LogExD4, Log, All);

// ──────────────────────────────────────────────────────────
// 全局对象注册表 (模拟 GUObjectArray)
// ──────────────────────────────────────────────────────────
TArray<FMiniObject*> GMiniObjectArray;

// ──────────────────────────────────────────────────────────
// FMiniGC 静态成员初始化
// ──────────────────────────────────────────────────────────
TArray<FMiniObject*> FMiniGC::IncrementalWorkList;
bool                 FMiniGC::bIncrementalMarkInProgress = false;

// ──────────────────────────────────────────────────────────
// FMiniObject 实现
// ──────────────────────────────────────────────────────────
FMiniObject::FMiniObject(FString InName)
    : Name(MoveTemp(InName))
{
    // 等价于 NewObject 后自动注册到 GUObjectArray
    GMiniObjectArray.Add(this);
    UE_LOG(LogExD4, Verbose, TEXT("[FMiniObject] 创建: %s (总数=%d)"),
        *Name, GMiniObjectArray.Num());
}

FMiniObject::~FMiniObject()
{
    UE_LOG(LogExD4, Verbose, TEXT("[FMiniObject] 析构: %s"), *Name);
    // 注意: 析构时不从 GMiniObjectArray 移除 (Sweep 阶段置 nullptr 后统一压缩)
    // 这模拟了 UE 的延迟清理机制: 先标记 RF_Garbage → 再 ConditionalBeginDestroy → 再 Purge
}

// ──────────────────────────────────────────────────────────
// FMiniGC::Mark —— 从 RootSet 出发 BFS 标记所有可达对象
// ──────────────────────────────────────────────────────────
void FMiniGC::Mark()
{
    // ══════════════════════════════════════════════════════
    // TODO [必做] 2: 实现 Mark Phase
    //
    // // Step 1: 清除上一次 GC 的标记 (每次 GC 必须清零 bReachable)
    // for (FMiniObject* Obj : GMiniObjectArray)
    //     if (Obj) Obj->bReachable = false;
    //
    // // Step 2: 收集根集 (bIsRootSet == true 的对象)
    // // 对应: UE 的 GRoots (AddToRoot/RF_ClassDefaultObject/RF_RootSet 标志的对象)
    // TArray<FMiniObject*> WorkList;
    // for (FMiniObject* Obj : GMiniObjectArray)
    // {
    //     if (Obj && Obj->bIsRootSet)
    //     {
    //         Obj->bReachable = true;
    //         WorkList.Add(Obj);
    //     }
    // }
    //
    // // Step 3: BFS 可达性传播
    // // 对应: PerformReachabilityAnalysis (GarbageCollection.cpp:4528)
    // while (!WorkList.IsEmpty())
    // {
    //     FMiniObject* Current = WorkList.Pop();
    //
    //     // 遍历该对象注册的所有 GC 可见引用 (对应 UStruct::RefLink)
    //     for (FMiniObject** RefField : Current->TrackedRefs)
    //     {
    //         FMiniObject* Referenced = *RefField;
    //         if (Referenced && !Referenced->bReachable)
    //         {
    //             Referenced->bReachable = true;
    //             WorkList.Add(Referenced);
    //         }
    //     }
    // }
    // ══════════════════════════════════════════════════════

    UE_LOG(LogExD4, Log, TEXT("[FMiniGC::Mark] 请完成 TODO [必做] 2"));
}

// ──────────────────────────────────────────────────────────
// FMiniGC::Sweep —— 删除所有未标记对象
// ──────────────────────────────────────────────────────────
void FMiniGC::Sweep()
{
    // ══════════════════════════════════════════════════════
    // TODO [必做] 3: 实现 Sweep Phase
    //
    // for (FMiniObject*& Obj : GMiniObjectArray)
    // {
    //     if (Obj && !Obj->bReachable)
    //     {
    //         UE_LOG(LogExD4, Log, TEXT("[MiniGC::Sweep] Collecting: %s"), *Obj->Name);
    //         delete Obj;
    //         Obj = nullptr;  // 留 null 槽 (模拟 UE 的延迟 Purge)
    //     }
    // }
    //
    // // 压缩数组 (UE 实际用 FUObjectItem::IsValid() 标志跳过 null 槽, 不立即压缩)
    // GMiniObjectArray.RemoveAll([](FMiniObject* P){ return P == nullptr; });
    //
    // 注意顺序: 必须先 Mark 再 Sweep!
    //   若先 Sweep 再 Mark → 活跃对象被错误删除 → UB
    // ══════════════════════════════════════════════════════

    UE_LOG(LogExD4, Log, TEXT("[FMiniGC::Sweep] 请完成 TODO [必做] 3"));
}

// ──────────────────────────────────────────────────────────
// FMiniGC::Collect —— 完整 GC 周期
// ──────────────────────────────────────────────────────────
void FMiniGC::Collect()
{
    UE_LOG(LogExD4, Log, TEXT("[MiniGC] === GC Cycle Start ==="));
    Mark();
    Sweep();
    UE_LOG(LogExD4, Log, TEXT("[MiniGC] === GC Cycle End (%d objects alive) ==="),
        GMiniObjectArray.Num());
}

// ──────────────────────────────────────────────────────────
// FMiniGC::IncrementalMarkStep —— 增量 Mark (进阶任务)
// 对应: EGCOptions::IncrementalReachability (GarbageCollection.cpp:4552)
// ──────────────────────────────────────────────────────────
bool FMiniGC::IncrementalMarkStep(int32 MaxObjectsPerStep)
{
    // ══════════════════════════════════════════════════════
    // TODO [进阶] 1: 实现增量标记
    //
    // if (!bIncrementalMarkInProgress)
    // {
    //     // 初始化: 清标记, 收集根集
    //     for (FMiniObject* Obj : GMiniObjectArray)
    //         if (Obj) Obj->bReachable = false;
    //     IncrementalWorkList.Reset();
    //     for (FMiniObject* Obj : GMiniObjectArray)
    //     {
    //         if (Obj && Obj->bIsRootSet)
    //         {
    //             Obj->bReachable = true;
    //             IncrementalWorkList.Add(Obj);
    //         }
    //     }
    //     bIncrementalMarkInProgress = true;
    // }
    //
    // int32 Processed = 0;
    // while (!IncrementalWorkList.IsEmpty() && Processed < MaxObjectsPerStep)
    // {
    //     FMiniObject* Current = IncrementalWorkList.Pop();
    //     for (FMiniObject** RefField : Current->TrackedRefs)
    //     {
    //         FMiniObject* Referenced = *RefField;
    //         if (Referenced && !Referenced->bReachable)
    //         {
    //             Referenced->bReachable = true;
    //             IncrementalWorkList.Add(Referenced);
    //         }
    //     }
    //     ++Processed;
    // }
    //
    // if (IncrementalWorkList.IsEmpty())
    // {
    //     bIncrementalMarkInProgress = false;
    //     return true;  // Mark phase 完成
    // }
    // return false;  // 仍在进行中
    //
    // 增量 GC 与 TObjectPtr write barrier 的关系:
    //   在 IncrementalMarkStep 调用之间, 若 GameThread 修改了某个引用:
    //     新引用目标可能尚未被 GC 遍历到 → "漏标"问题
    //   TObjectPtr 的 ConditionallyMarkAsReachable (ObjectPtr.h:306) 是修复方案:
    //     每次赋值/构造 TObjectPtr 时, 若增量 GC 正在运行, 立即标记目标为可达
    //   在 FMiniGC 中等价: 在两次 IncrementalMarkStep 之间修改了引用,
    //     需手动调用 Referenced->bReachable = true (模拟 write barrier)
    // ══════════════════════════════════════════════════════

    return true;  // 骨架: 直接返回完成
}

// ──────────────────────────────────────────────────────────
// 验证入口函数
// ──────────────────────────────────────────────────────────
void RunMiniGCDemo()
{
    UE_LOG(LogExD4, Log, TEXT("=== RunMiniGCDemo 开始 ==="));

    // ══════════════════════════════════════════════════════
    // TODO [必做] 4: 完成验证主程序
    //
    // // 创建对象图: A→B→D, A→C
    // FMiniObject* A = new FMiniObject(TEXT("A"));
    // FMiniObject* B = new FMiniObject(TEXT("B"));
    // FMiniObject* C = new FMiniObject(TEXT("C"));
    // FMiniObject* D = new FMiniObject(TEXT("D"));
    //
    // // 模拟 UPROPERTY 引用链 (TrackRef = 给成员加 UPROPERTY)
    // A->TrackRef(&B);  // A 持有对 B 的 GC 可见引用
    // A->TrackRef(&C);  // A 持有对 C 的 GC 可见引用
    // B->TrackRef(&D);  // B 持有对 D 的 GC 可见引用
    //
    // // A 是根 (模拟 AddToRoot)
    // A->AddToRoot();
    //
    // // Round 1: A/B/C/D 全部可达, 无对象被回收
    // UE_LOG(LogExD4, Log, TEXT("--- GC Round 1 (all reachable) ---"));
    // FMiniGC::Collect();
    // ensureMsgf(GMiniObjectArray.Num() == 4, TEXT("Round 1 应有 4 个对象存活"));
    //
    // // 模拟"丢失 UPROPERTY": A 不再追踪 C
    // // 等价于去掉 C 的 UPROPERTY 标注 → GC 看不到 A→C 这条引用
    // A->UntrackRef(&C);
    // // C 仍在内存里, 但 GC 不可见 → 下次 GC 回收 C → 访问 C 是悬空指针 (UB)
    //
    // // Round 2: C 不可达被回收; A/B/D 可达
    // UE_LOG(LogExD4, Log, TEXT("--- GC Round 2 (C unreachable after losing UPROPERTY) ---"));
    // FMiniGC::Collect();
    // ensureMsgf(GMiniObjectArray.Num() == 3, TEXT("Round 2 应有 3 个对象存活 (C 已回收)"));
    // // 此后 C 指针已悬空! 不要访问 C
    //
    // // Round 3: 清理根, 所有对象不可达
    // UE_LOG(LogExD4, Log, TEXT("--- GC Round 3 (cleanup) ---"));
    // A->RemoveFromRoot();
    // FMiniGC::Collect();
    // ensureMsgf(GMiniObjectArray.Num() == 0, TEXT("Round 3 应有 0 个对象存活"));
    // ══════════════════════════════════════════════════════

    UE_LOG(LogExD4, Log, TEXT("请完成 TODO [必做] 4: RunMiniGCDemo 验证主程序"));
    UE_LOG(LogExD4, Log, TEXT("=== RunMiniGCDemo 结束 ==="));
}


// ---- 验证区 (完成 TODO 后把下列 ensure 搬入对应代码路径) ----
// // 验证 Mark 正确清零 bReachable
// // 验证 Sweep 正确删除不可达对象
// // 验证第三次 GC 后 GMiniObjectArray.Num() == 0
// UE_LOG(LogExD4, Log, TEXT("ExD4 验证通过"));
