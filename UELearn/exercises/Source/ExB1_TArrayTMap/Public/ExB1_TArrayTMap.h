// 对应章节: ../../../03-模块B-核心容器与字符串.md §练习 B-1
// 小节: TArray / TMap / TSet 语义与迭代器失效
#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

// ══════════════════════════════════════════════════════
// FB1TArrayTMapModule: 演示 UE 三大容器在增删改过程中的内存语义
// 和迭代器失效规律；重点对比与 STL 容器的差异。
// ══════════════════════════════════════════════════════
class FB1TArrayTMapModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
    // 骨架中已实现：演示 TArray Reserve 与重分配地址变化
    static void DemoTArrayReserve();

    // 骨架中已实现：演示 RemoveAt vs RemoveAtSwap 保序/破序对比
    static void DemoRemoveAtVsSwap();

    // TODO [必做] 3: 演示 ranged-for 中修改 TArray 触发 TCheckedPointerIterator 警告
    // static void DemoIteratorInvalidation();

    // TODO [必做] 4: 演示 TMap FindOrAdd + rehash 后引用失效
    // static void DemoTMapFindOrAdd();

    // TODO [必做] 5: 演示 TSet Contains vs TArray Contains 性能对比
    // static void DemoTSetVsTArrayContains();
};
