// 对应章节: ../../../03-模块B-核心容器与字符串.md §练习 B-1
// C++ 标准要求: C++20
// 本题目标: 把 TArray/TMap/TSet 在增删改过程中的内存语义和迭代器失效规律刻进直觉
//
// 骨架阶段预期行为: DemoTArrayReserve 和 DemoRemoveAtVsSwap 打印结果
// 完成后预期行为（Editor 启动日志）:
//   LogExB1: [B1] Reserve 前地址: 0x...  Reserve 后地址: 0x...
//   LogExB1: [B1] Add 后地址是否改变: 0 (Reserve 已保证不重分配)
//   LogExB1: [B1] RemoveAt  结果: A C D E (保序)
//   LogExB1: [B1] RemoveAtSwap 结果: A E C D (破序，末尾元素填入)

#include "ExB1_TArrayTMap.h"
#include "Modules/ModuleManager.h"
#include "HAL/PlatformTime.h"

// ══════════════════════════════════════════════════════
// 独立日志分类
// ══════════════════════════════════════════════════════
DEFINE_LOG_CATEGORY_STATIC(LogExB1, Log, All);

IMPLEMENT_MODULE(FB1TArrayTMapModule, ExB1_TArrayTMap);

// ══════════════════════════════════════════════════════
// DemoTArrayReserve: 演示 Reserve 与地址稳定性
// TArray::Reserve 源码: Engine/Source/Runtime/Core/Public/Containers/Array.h
// ══════════════════════════════════════════════════════
void FB1TArrayTMapModule::DemoTArrayReserve()
{
    TArray<int32> Arr;
    // 填入 100 个元素，可能触发多次重分配
    for (int32 i = 0; i < 100; ++i)
    {
        Arr.Add(i);
    }
    const int32* P1 = Arr.GetData();
    UE_LOG(LogExB1, Log, TEXT("[B1] 100 个元素后 GetData 地址: %p"), P1);

    // Reserve 200 个位置后，后续 Add 不触发重分配
    Arr.Reserve(200);
    const int32* P2 = Arr.GetData();
    UE_LOG(LogExB1, Log, TEXT("[B1] Reserve(200) 后 GetData 地址: %p"), P2);

    // Add 一个元素，不超过 Reserve 容量，地址应不变
    Arr.Add(101);
    const int32* P3 = Arr.GetData();
    UE_LOG(LogExB1, Log, TEXT("[B1] Reserve 后 Add 一个元素，地址改变: %d"),
        P3 != P2 ? 1 : 0);

    // ══════════════════════════════════════════════════════
    // TODO [必做] 1: 不 Reserve 直接 Add，记录地址是否改变（验证触发重分配的条件）
    //   TArray<int32> Arr2;
    //   for (int32 i = 0; i < 100; ++i) { Arr2.Add(i); }
    //   const int32* PA = Arr2.GetData();
    //   Arr2.Add(101);
    //   const int32* PB = Arr2.GetData();
    //   UE_LOG(LogExB1, Log, TEXT("[B1] 未 Reserve 直接 Add，地址改变: %d"), PA != PB ? 1 : 0);
    // ══════════════════════════════════════════════════════
}

// ══════════════════════════════════════════════════════
// DemoRemoveAtVsSwap: 演示 RemoveAt 保序 vs RemoveAtSwap 破序
// RemoveAt 源码:     Array.h:2083
// RemoveAtSwap 源码: Array.h:2185
// ══════════════════════════════════════════════════════
void FB1TArrayTMapModule::DemoRemoveAtVsSwap()
{
    // --- RemoveAt 保序 ---
    TArray<FString> ArrA;
    ArrA.Add(TEXT("A")); ArrA.Add(TEXT("B")); ArrA.Add(TEXT("C"));
    ArrA.Add(TEXT("D")); ArrA.Add(TEXT("E"));
    ArrA.RemoveAt(1);  // 删除索引 1 ("B")，后续元素向前移位
    FString ResultA;
    for (const FString& S : ArrA) { ResultA += S + TEXT(" "); }
    UE_LOG(LogExB1, Log, TEXT("[B1] RemoveAt(1)  结果: %s (应为 A C D E)"), *ResultA);

    // --- RemoveAtSwap 破序 ---
    TArray<FString> ArrB;
    ArrB.Add(TEXT("A")); ArrB.Add(TEXT("B")); ArrB.Add(TEXT("C"));
    ArrB.Add(TEXT("D")); ArrB.Add(TEXT("E"));
    ArrB.RemoveAtSwap(1);  // 末尾 "E" 移到索引 1
    FString ResultB;
    for (const FString& S : ArrB) { ResultB += S + TEXT(" "); }
    UE_LOG(LogExB1, Log, TEXT("[B1] RemoveAtSwap(1) 结果: %s (应为 A E C D)"), *ResultB);

    // ══════════════════════════════════════════════════════
    // TODO [必做] 2: 写下结论
    //   RemoveAt: 保序，O(N) 移位（N = 被删元素后的元素数量）
    //   RemoveAtSwap: 破序，O(1)（只做一次覆盖 + ArrayNum 缩减）
    // ══════════════════════════════════════════════════════
}

void FB1TArrayTMapModule::StartupModule()
{
    UE_LOG(LogExB1, Log, TEXT("[B1] StartupModule called"));

    DemoTArrayReserve();
    DemoRemoveAtVsSwap();

    // ══════════════════════════════════════════════════════
    // TODO [必做] 3: 演示 ranged-for 中修改 TArray（Debug 构建下 TCheckedPointerIterator 报警）
    //   TArray<int32> TestArr = {1, 2, 3, 4, 5};
    //   for (int32 Val : TestArr)
    //   {
    //       if (Val == 3) TestArr.Add(999);  // Debug 构建下触发 ensureMsgf
    //   }
    //   // 安全写法: 用索引循环 for (int32 i = 0; i < TestArr.Num(); ++i)
    // ══════════════════════════════════════════════════════

    // ══════════════════════════════════════════════════════
    // TODO [必做] 4: 演示 TMap FindOrAdd + rehash 后引用失效
    //   TMap<FString, int32> FreqMap;
    //   for (int32 i = 0; i < 100; ++i) FreqMap.Add(FString::Printf(TEXT("key%d"), i), i);
    //   int32& Ref = FreqMap.FindOrAdd(TEXT("key0"));
    //   Ref++;
    //   // 继续插入超过 bucket 数量的新条目，触发 rehash，Ref 悬空
    //   for (int32 i = 100; i < 300; ++i) FreqMap.Add(FString::Printf(TEXT("key%d"), i), i);
    //   // Ref 此时可能已悬空，不要再解引用
    //   UE_LOG(LogExB1, Log, TEXT("[B1] TMap rehash 后引用失效实验完成"));
    // ══════════════════════════════════════════════════════

    // ══════════════════════════════════════════════════════
    // TODO [必做] 5: 演示 TSet::Contains vs TArray::Contains 性能对比
    //   TSet<int32> MySet;  TArray<int32> MyArr;
    //   for (int32 i = 0; i < 1000; ++i) { MySet.Add(i); MyArr.Add(i); }
    //   uint64 T0 = FPlatformTime::Cycles64();
    //   for (int32 i = 0; i < 1000000; ++i) { volatile bool b = MySet.Contains(777); }
    //   uint64 T1 = FPlatformTime::Cycles64();
    //   for (int32 i = 0; i < 1000000; ++i) { volatile bool b = MyArr.Contains(777); }
    //   uint64 T2 = FPlatformTime::Cycles64();
    //   UE_LOG(LogExB1, Log, TEXT("[B1] TSet cycles: %llu  TArray cycles: %llu"),
    //       T1 - T0, T2 - T1);
    // ══════════════════════════════════════════════════════

    // ══════════════════════════════════════════════════════
    // TODO [进阶] 1: TInlineAllocator micro-benchmark
    //   TArray<int32> HeapArr;
    //   TArray<int32, TInlineAllocator<16>> InlineArr;
    //   各填 16 个 int32，用 FPlatformTime::Cycles64() 测量求和耗时（重复 1,000,000 次）
    // ══════════════════════════════════════════════════════
}

void FB1TArrayTMapModule::ShutdownModule()
{
    UE_LOG(LogExB1, Log, TEXT("[B1] ShutdownModule called"));
}


// ---- 验证区 (完成 TODO 后把下列行搬入对应代码路径) ----
// ensureMsgf(ArrA.Num() == 4, TEXT("RemoveAt 后应剩余 4 个元素"));
// ensureMsgf(ArrB.Num() == 4, TEXT("RemoveAtSwap 后应剩余 4 个元素"));
// UE_LOG(LogExB1, Log, TEXT("[B1] TSet Contains 比 TArray Contains 快约 N 倍（见实测数据）"));
