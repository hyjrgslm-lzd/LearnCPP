// 对应章节: ../../../05-模块D-UObject-反射-GC.md §练习 D-4
// 小节: 手写 mini mark-sweep GC (implement-own, 纯 C++, 不依赖 UObject)
#pragma once

#include "CoreMinimal.h"
// 注意: 本题故意不 include 任何 UObject/CoreUObject 头文件
// 所有类型均为纯 C++ struct/class, 使用 TArray 保持 UE 风格

// ══════════════════════════════════════════════════════
// 全局对象注册表 (模拟 GUObjectArray)
// 参考: Engine/.../CoreUObject/Private/UObject/GarbageCollection.cpp
//       GUObjectArray 是 FUObjectArray, 存储所有 UObject 的 FUObjectItem 数组
// ══════════════════════════════════════════════════════
// 在 .cpp 中定义: TArray<class FMiniObject*> GMiniObjectArray;
extern TArray<class FMiniObject*> GMiniObjectArray;

// ══════════════════════════════════════════════════════
// FMiniObject: 模拟 UObject 的最小骨架
// ══════════════════════════════════════════════════════
class EXD4_MINIMARKSWEEP_API FMiniObject
{
public:
    FString Name;

    // 模拟 EInternalObjectFlags::RootSet
    // 参考: UObjectBaseUtility.h:206 AddToRoot() → GUObjectArray.IndexToObject(...)->SetRootSet()
    bool bIsRootSet = false;

    // GC 标记位 (Mark phase 期间设置, 每次 GC 开始时清零)
    bool bReachable = false;

    // 模拟 UStruct::RefLink —— 只追踪持有 FMiniObject* 的字段
    // 等价于给成员指针加 UPROPERTY 标注
    // 参考: Engine/.../CoreUObject/Public/UObject/Class.h:533 (RefLink)
    TArray<FMiniObject**> TrackedRefs;

    explicit FMiniObject(FString InName);
    virtual ~FMiniObject();

    // 模拟 UObject::AddToRoot() —— 加入根集后 GC 不回收
    void AddToRoot()      { bIsRootSet = true;  }
    void RemoveFromRoot() { bIsRootSet = false; }

    // 手动注册一个需要 GC 追踪的引用字段 (等价于给成员加 UPROPERTY)
    void TrackRef(FMiniObject** RefField) { TrackedRefs.Add(RefField); }
    void UntrackRef(FMiniObject** RefField) { TrackedRefs.Remove(RefField); }
};

// ══════════════════════════════════════════════════════
// FMiniGC: 两阶段 mark-sweep
// 参考: Engine/.../CoreUObject/Private/UObject/GarbageCollection.cpp
//   Mark  → PerformReachabilityAnalysis (line 4528)
//   Sweep → GatherUnreachableObjects + CollectGarbage
// ══════════════════════════════════════════════════════
class EXD4_MINIMARKSWEEP_API FMiniGC
{
public:
    // === Mark Phase ===
    // 从 RootSet 出发 BFS 标记所有可达对象
    // 对应 UE: PerformReachabilityAnalysis (GarbageCollection.cpp:4528)
    static void Mark();

    // === Sweep Phase ===
    // 删除所有未标记对象, 留 null 槽再统一压缩
    // 对应 UE: CollectGarbage → ConditionalBeginDestroy → Purge (延迟析构)
    static void Sweep();

    // === Full GC Cycle ===
    static void Collect();

    // === 增量 Mark (进阶任务) ===
    // 每次处理最多 MaxObjectsPerStep 个对象, 模拟 UE 的 IncrementalReachability
    // 参考: GarbageCollection.cpp:4552 EGCOptions::IncrementalReachability
    static bool IncrementalMarkStep(int32 MaxObjectsPerStep);

private:
    // 增量 Mark 的工作队列 (跨帧保持状态)
    static TArray<FMiniObject*> IncrementalWorkList;
    static bool bIncrementalMarkInProgress;
};

// ══════════════════════════════════════════════════════
// 验证入口函数
// ══════════════════════════════════════════════════════
void RunMiniGCDemo();
