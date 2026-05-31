> 对应章节: ../../../04-模块C-内存-智能指针-委托.md §练习 C-1

## 目标

掌握 `TSharedPtr` / `TWeakPtr` / `TUniquePtr` 三件套的用法肌肉记忆；能从控制块层面说清 `ESPMode::ThreadSafe` 与 `NotThreadSafe` 的差异；能区分 `MakeShared`（单次分配）与 `MakeShareable`（两次分配）在 `SharedPointerInternals.h` 中的实现路径。

## 前置理解

- 已读 `01-心智模型.md` §4（GC ownership vs C++ ownership）：`TSharedPtr` 管理非 UObject 对象，GC 对其一无所知。
- 理解 C++ `std::shared_ptr` 的引用计数语义；本题重点是 UE 实现与 STL 实现在线程安全和分配模型上的差异。
- 已完成 ExA1_HelloModule，理解 UE Module 加载机制。

## 必做任务

1. 用 `MakeShared<FMeshData>(100, TEXT("TestMesh"))` 创建 `TSharedPtr<FMeshData>`，打印 `VertexCount` 和引用计数。
2. 声明两份指针，一份 `ESPMode::ThreadSafe`，一份 `NotThreadSafe`（默认），对照 `SharedPointerInternals.h` 的 `TReferenceControllerBase<Mode>` 写出 `SharedReferenceCount` 的类型差异。
3. 对比 `MakeShared<FMeshData>()` 与 `MakeShareable(new FMeshData())` 的分配次数，在注释里标注 `TIntrusiveReferenceController` vs `TReferenceController` 的路径。
4. 创建 `FNode` 循环引用（A→B→A），观察析构不被调用；将 `FNode::Next` 改为 `TWeakPtr<FNode>`，演示 `Pin()` 正确用法。
5. 用 `MakeUnique<FMeshData>()` 创建 `TUniquePtr`，尝试复制（预期编译失败），改用 `MoveTemp()` 转移，验证转移后源指针为 `nullptr`。

## 进阶任务

- 让 `FSharedSelf` 继承 `TSharedFromThis<FSharedSelf>`，在成员函数里调用 `AsShared()` 返回 `TSharedRef`，观察与直接构造新 `TSharedPtr` 的区别（不新增控制块）。
- 尝试把 `UObject*` 塞进 `TSharedPtr<UObject>`，记录 `SharedPointer.h` Limitations 注释的警告，写出两个层面的原因（GC 不可见引用计数；双重析构风险）。
- 对比 `TObjectPtr<T>` 与 `TSharedPtr<T>`：前者走 GC 路径，后者走 C++ 引用计数路径，两套体系完全正交。

## 验收点

- [ ] 编译通过，`LogExC1` 日志在 PIE Output Log 可见。
- [ ] 能用一句话解释 `ESPMode::ThreadSafe` 的控制块代价（`std::atomic<int32>` vs 裸 `int32`）。
- [ ] `MakeShared` 与 `MakeShareable` 分配次数差异可在注释中说清楚。
- [ ] 循环引用场景可复现，用 `TWeakPtr` 打破后析构顺序符合预期。
- [ ] 能给出"为什么 `TSharedPtr<UObject>` 是错误的"的书面解释（至少两个层面）。
- [ ] `TUniquePtr` move 转移演示可运行，能答出四个固定问题。

## 观察点

- `TSharedRef` 与 `TSharedPtr` 的核心差别：`TSharedRef` 构造时断言非空，所有解引用无需空检查，是"保证此对象存在"的类型表达。
- `TWeakPtr::Pin()` 在 `SharedReferenceCount == 0` 时返回空 `TSharedPtr`；控制块的 `WeakReferenceCount` 在强引用归零后仍存活，直到最后一个 `TWeakPtr` 销毁才真正释放。
- `TSharedFromThis` 的正确前提：对象必须已被某个 `TSharedPtr` 持有；纯栈对象上调用 `AsShared()` 会触发断言失败。

## 常见坑

- **把 `UObject*` 塞进 `TSharedPtr`**：GC 不知道控制块；GC 回收后 `TSharedPtr` 持有悬空指针。
- **多线程代码忘记 `ESPMode::ThreadSafe`**：默认 `NotThreadSafe` 下并发修改引用计数发生 data race。
- **循环引用导致隐形内存泄漏**：UE 的 `TSharedPtr` 无弱引用自动探测，需主动用 `TWeakPtr` 打破环。
- **栈对象上调用 `AsShared()`**：控制块不存在，断言失败。

## 提示

- `SharedPointerInternals.h` 的 `TReferenceControllerBase<Mode>` 是理解线程安全代价的最直接入口，重点看 `AddSharedReference()` 在两种 `constexpr if` 分支下的代码路径。
- `MakeShared` 走 `TIntrusiveReferenceController`，用 `TypeCompatibleBytes<ObjectType>` 内联存储对象内容，整个结构体只 `new` 一次。
- Editor 构建中 `TSharedPtr` 的转换路径会插入 `checkf(IsValid(), ...)` 检查；Shipping 构建中移除。

## 复盘问题

1. 真正开始执行的时刻？（`MakeShared`/`new` 发生在调用点的当前线程，通常是 GameThread；析构发生在最后一个持有者的析构点，无特定线程约束，这正是 `ThreadSafe` 模式必要的原因。）
2. 谁负责这对象的生命周期？（`TSharedPtr` 引用计数：最后一个强引用归零时析构；控制块额外存活直到所有弱引用也归零。）
3. 涉及哪些命名线程？（本题全在 GameThread；若跨线程共享需 `ESPMode::ThreadSafe`；GC 线程不参与 `TSharedPtr` 引用计数。）
4. 这对 GC 如何可见？（本题的 `FMeshData` 是普通 C++ 结构体，完全不参与 GC 体系——这是 C++ 所有权路径的完整示例。）
5. [本题专属] `TSharedPtr<T, ESPMode::ThreadSafe>` 的 `AddSharedReference()` 在 MSVC 上映射到哪个 Win32 原子 API？在 ARM 上又是什么？

## 对应官方参考

- `Engine/Source/Runtime/Core/Public/Templates/SharedPointer.h`（用户侧 API 注释，含 Tips/Limitations）
- `Engine/Source/Runtime/Core/Public/Templates/SharedPointerInternals.h`（`TReferenceControllerBase<Mode>`，两种计数类型的 `if constexpr` 分支；`TIntrusiveReferenceController` vs `TReferenceController`）
- `Engine/Source/Runtime/Core/Public/Templates/UniquePtr.h`（`TUniquePtr<T>`、`TDefaultDelete<T>`）
