// 对应章节: ../../../04-模块C-内存-智能指针-委托.md §练习 C-1
// 小节: TSharedPtr / TWeakPtr / TUniquePtr 语义 —— ESPMode 差异与分配模型
#pragma once

#include "CoreMinimal.h"
// 注意: 此模块故意不 include UObject 相关头 —— 演示 TSharedPtr 与 GC 完全正交

// ══════════════════════════════════════════════════════
// 非 UObject 结构体: 代表一块网格数据 (纯 C++ 所有权路径)
// 学员任务: 可以在此加更多成员, 但不能改为 USTRUCT/UObject
// ══════════════════════════════════════════════════════
struct FMeshData
{
    int32   VertexCount = 0;
    FString Name;

    FMeshData() = default;
    FMeshData(int32 InVertexCount, FString InName)
        : VertexCount(InVertexCount), Name(MoveTemp(InName)) {}

    // 析构时打印日志, 帮助观察引用计数归零时机
    ~FMeshData();
};

// ══════════════════════════════════════════════════════
// 演示循环引用的节点类型
// TODO [必做] 4: 先用 TSharedPtr<FNode> Next 制造环, 再改为 TWeakPtr<FNode> Next 打破
// ══════════════════════════════════════════════════════
struct FNode
{
    FString NodeName;

    // TODO [必做] 4: 改为 TWeakPtr<FNode> Next 以打破循环引用
    TSharedPtr<FNode> Next;

    explicit FNode(FString InName) : NodeName(MoveTemp(InName)) {}
    ~FNode();
};

// ══════════════════════════════════════════════════════
// 演示 TSharedFromThis
// TODO [进阶] 1: 继承 TSharedFromThis<FSharedSelf> 后可在成员函数里调用 AsShared()
// ══════════════════════════════════════════════════════
struct FSharedSelf : public TSharedFromThis<FSharedSelf>
{
    FString Tag;
    explicit FSharedSelf(FString InTag) : Tag(MoveTemp(InTag)) {}

    // TODO [进阶] 1: 实现 GetSelf(), 返回 AsShared() (TSharedRef<FSharedSelf>)
    // TSharedRef<FSharedSelf> GetSelf() { return AsShared(); }
};
