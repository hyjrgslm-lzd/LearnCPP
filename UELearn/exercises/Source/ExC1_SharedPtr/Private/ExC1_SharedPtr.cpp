// ============================================================
// 对应章节: ../../../04-模块C-内存-智能指针-委托.md §练习 C-1
// C++ 标准要求: C++20
// 本题目标: 掌握 TSharedPtr/TWeakPtr/TUniquePtr 语义;
//           对比 ESPMode::ThreadSafe vs NotThreadSafe 的控制块差异;
//           对比 MakeShared (单次分配) vs MakeShareable (两次分配)
//
// 骨架阶段预期行为: Module 注册, 打印一条 Log
// 完成后预期行为 (PIE 启动日志):
//   LogExC1: ExC1 Module 启动
//   LogExC1: FMeshData("TestMesh", 100) 已创建, SharedRef=1
//   LogExC1: MakeShared 单次分配路径: TIntrusiveReferenceController
//   LogExC1: MakeShareable 双次分配路径: TReferenceController
//   LogExC1: 循环引用演示: A->B->A (观察析构是否被调用)
//   LogExC1: TWeakPtr::Pin() 正确打破循环引用
//   LogExC1: TUniquePtr move 后 Ptr1 == nullptr
// ============================================================

#include "ExC1_SharedPtr.h"
#include "Modules/ModuleManager.h"

// 参考源码:
//   Engine/Source/Runtime/Core/Public/Templates/SharedPointer.h
//   Engine/Source/Runtime/Core/Public/Templates/SharedPointerInternals.h
//     └── TReferenceControllerBase<Mode>: SharedReferenceCount 类型
//         ThreadSafe  → std::atomic<int32>  (每次 AddRef = _InterlockedIncrement)
//         NotThreadSafe → int32             (每次 AddRef = 裸 ++)
//     └── TIntrusiveReferenceController   (MakeShared 路径: 对象+控制块一次分配)
//     └── TReferenceController            (MakeShareable 路径: 控制块仅持有对象指针)

IMPLEMENT_MODULE(FDefaultModuleImpl, ExC1_SharedPtr);

DEFINE_LOG_CATEGORY_STATIC(LogExC1, Log, All);

// ──────────────────────────────────────────────────────────
// FMeshData 析构实现
// ──────────────────────────────────────────────────────────
FMeshData::~FMeshData()
{
    UE_LOG(LogExC1, Log, TEXT("~FMeshData: Name=%s VertexCount=%d"), *Name, VertexCount);
}

// ──────────────────────────────────────────────────────────
// FNode 析构实现
// ──────────────────────────────────────────────────────────
FNode::~FNode()
{
    UE_LOG(LogExC1, Log, TEXT("~FNode: %s"), *NodeName);
}

// ══════════════════════════════════════════════════════
// TODO [必做] 1: 用 MakeShared<FMeshData>(...) 创建 TSharedPtr<FMeshData>
//   TSharedPtr<FMeshData> Mesh = MakeShared<FMeshData>(100, TEXT("TestMesh"));
//   UE_LOG(LogExC1, Log, TEXT("VertexCount=%d"), Mesh->VertexCount);
// ══════════════════════════════════════════════════════

// ══════════════════════════════════════════════════════
// TODO [必做] 2: ESPMode 差异对比
//   TSharedPtr<FMeshData, ESPMode::ThreadSafe>    MeshTS  = MakeShared<FMeshData, ESPMode::ThreadSafe>(50, TEXT("TS"));
//   TSharedPtr<FMeshData, ESPMode::NotThreadSafe> MeshNTS = MakeShared<FMeshData>(50, TEXT("NTS"));
//   // 在注释里写出:
//   // ThreadSafe  路径: SharedReferenceCount 类型 = std::atomic<int32>
//   //                   AddRef → std::atomic<int32>::operator++ (映射到 _InterlockedIncrement)
//   // NotThreadSafe 路径: SharedReferenceCount 类型 = int32
//   //                     AddRef → 裸 ++, 无内存屏障
// ══════════════════════════════════════════════════════

// ══════════════════════════════════════════════════════
// TODO [必做] 3: MakeShared vs MakeShareable 分配次数
//   // MakeShared:    走 TIntrusiveReferenceController —— 对象与控制块在同一块内存 (1次分配)
//   TSharedPtr<FMeshData> PtrA = MakeShared<FMeshData>(10, TEXT("A"));
//
//   // MakeShareable: 先 new FMeshData (1次分配), 再由 TSharedPtr 创建控制块 (1次分配) = 2次
//   TSharedPtr<FMeshData> PtrB = MakeShareable(new FMeshData(10, TEXT("B")));
//
//   // 结论: 优先使用 MakeShared, 节省一次堆分配, 提升 cache locality
// ══════════════════════════════════════════════════════

// ══════════════════════════════════════════════════════
// TODO [必做] 4: 循环引用场景
//   TSharedPtr<FNode> A = MakeShared<FNode>(TEXT("A"));
//   TSharedPtr<FNode> B = MakeShared<FNode>(TEXT("B"));
//   A->Next = B;  // A→B
//   B->Next = A;  // B→A  (循环! 析构永远不会调用)
//   // 观察: 出作用域后 ~FNode 不被调用 → 内存泄漏
//
//   // 修复: 将 FNode::Next 改为 TWeakPtr<FNode>
//   //   TWeakPtr<FNode> WB = B;
//   //   A->Next = WB.Pin();  // 错误示例 (Pin 返回 TSharedPtr 又成强引用)
//   //   // 正确做法: A->Next 改为 TWeakPtr, 访问前先 Pin() 检查有效性
// ══════════════════════════════════════════════════════

// ══════════════════════════════════════════════════════
// TODO [必做] 5: TUniquePtr move 语义
//   TUniquePtr<FMeshData> Ptr1 = MakeUnique<FMeshData>(20, TEXT("Unique"));
//   // TUniquePtr<FMeshData> Ptr2 = Ptr1;  // 编译失败: 不可复制
//   TUniquePtr<FMeshData> Ptr2 = MoveTemp(Ptr1);
//   check(!Ptr1.IsValid());  // Ptr1 移交后为 nullptr
//   UE_LOG(LogExC1, Log, TEXT("Ptr2 Name=%s"), *Ptr2->Name);
// ══════════════════════════════════════════════════════

// ══════════════════════════════════════════════════════
// TODO [进阶] 1: TSharedFromThis
//   // FSharedSelf 已继承 TSharedFromThis<FSharedSelf> (见 .h)
//   TSharedPtr<FSharedSelf> Owner = MakeShared<FSharedSelf>(TEXT("MySelf"));
//   TSharedRef<FSharedSelf> Ref   = Owner->AsShared();
//   // 注意: 必须先有一个 TSharedPtr 持有对象, AsShared() 才不会断言失败
//   // 错误示范: FSharedSelf StackObj(TEXT("X")); StackObj.AsShared(); // crash
// ══════════════════════════════════════════════════════

// ══════════════════════════════════════════════════════
// TODO [进阶] 2: 尝试把 UObject* 塞进 TSharedPtr (演示禁止操作)
//   // SharedPointer.h 的 Limitations 注释:
//   //   "Shared pointers are not compatible with Unreal objects (UObject classes)!"
//   // 原因:
//   //   1. GC 不可见 TSharedPtr 控制块, GC 回收后 TSharedPtr 持有悬空指针
//   //   2. UObject 析构由 GC mark-sweep 管理, TSharedPtr 引用计数归零会触发
//   //      双重析构 (UB)
//   // 正确做法: UPROPERTY() TObjectPtr<UObject> Ref; —— 走 GC 路径
// ══════════════════════════════════════════════════════


// ---- 验证区 (完成 TODO 后把下列 ensure 搬入对应代码路径) ----
// ensureMsgf(Mesh.IsValid(), TEXT("MakeShared 后指针应有效"));
// ensureMsgf(Mesh.GetSharedReferenceCount() == 1, TEXT("首次创建引用计数应为 1"));
// ensureMsgf(!Ptr1.IsValid(), TEXT("MoveTemp 后源指针应为 nullptr"));
// UE_LOG(LogExC1, Log, TEXT("ExC1 验证通过"));
