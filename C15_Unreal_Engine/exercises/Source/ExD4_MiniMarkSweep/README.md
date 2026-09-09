> 对应章节: ../../../05-模块D-UObject-反射-GC.md §练习 D-4

## 目标

这是本课程第一次触底。自己手写一个最小的 mark-sweep GC（`FMiniGC`），把"为什么 `UPROPERTY` 缺失导致悬空"的底层机制彻底理解——不是凭记忆，而是因为你亲手写过它。做完本题后，你应该能画出 RootSet / 可达对象 / 不可达对象 / PropertyLink / GC Barrier 五元关系图，并用代码回答每一层。

## 前置理解

- 已完成 ExD1~ExD3，理解 PropertyLink 是反射遍历的基础，以及 UHT 如何把 `UPROPERTY` 变成 `RefLink` 节点。
- 已读 `01-心智模型.md` §4，理解 GC 的 mark-sweep 原理。
- 理解两套所有权：`UPROPERTY`/`TObjectPtr`（GC 可见）vs `TSharedPtr`/裸指针（GC 不可见）。

## 必做任务

1. 实现 `FMiniObject` 构造函数：创建时自动注册到 `GMiniObjectArray`（模拟 `NewObject` 注册到 `GUObjectArray`）。
2. 实现 `FMiniGC::Mark()`：清零所有对象的 `bReachable`；收集根集（`bIsRootSet==true`）；BFS 沿 `TrackedRefs` 传播可达性。
3. 实现 `FMiniGC::Sweep()`：删除所有 `bReachable==false` 的对象，留 null 槽后统一压缩数组。
4. 实现 `RunMiniGCDemo()`：创建 A→B→D, A→C 的对象图；第一次 GC 无回收；移除 A 对 C 的 `TrackRef`；第二次 GC 回收 C；`RemoveFromRoot` 后第三次 GC 回收 A/B/D。
5. 对照 UE 源码 `GarbageCollection.cpp:4528`（`PerformReachabilityAnalysis`）——理解其 `while(true)` 主循环、`IsSuspended()` 检查、`EGCOptions::IncrementalReachability` 标志。

## 进阶任务

- 实现 `FMiniGC::IncrementalMarkStep(int32 MaxObjectsPerStep)`：把 Mark phase 分步执行，模拟 UE 的增量可达性分析。在两次 `IncrementalMarkStep` 之间修改引用后，手动标记新引用目标为可达（模拟 `TObjectPtr` write barrier）。
- 实现 `FGCObject` 等价物：一个非 `FMiniObject` 的类，通过注册回调告诉 `FMiniGC` 自己持有的引用——模拟 `FGCObject::AddReferencedObjects` 接口。
- 实现对象集群（Cluster）模拟：把一组相互引用的 `FMiniObject` 打包成一个"集群"，集群整体作为 GC 单元——对应 UE 的 `CreateCluster` 机制。

## 验收点

- [ ] `RunMiniGCDemo()` 正确输出"第一次 GC 无回收，第二次 GC 回收 C，第三次 GC 回收 A/B/D"。
- [ ] 能用一句话解释为什么"裸 `UObject*` 成员不加 `UPROPERTY`"会导致 GC 回收你以为还活着的对象。
- [ ] 能说明 stop-the-world GC（`Collect()` 一次性完成）与增量 GC（`IncrementalMarkStep(N)` 分帧）的差别，以及为什么增量 GC 需要 write barrier。
- [ ] 能把"UObject 元素 / PropertyLink / RootSet / ReachableSet / UnreachableSet"画成一张关系图并标注五元素。
- [ ] 能答出四个固定问题。

## 观察点

**增量 GC 与 TObjectPtr GC Barrier 的关系**（理解 UE5 TObjectPtr 的核心）：

增量 GC 期间 Mark phase 分多帧运行。若 GameThread 在 GC 运行期间创建了一个新的 `TObjectPtr` 指向尚未被 GC 遍历到的对象，该对象可能被错误地清扫（"漏标"问题）。

`TObjectPtr` 的 `ConditionallyMarkAsReachable`（`ObjectPtr.h:306-329`）正是解决这个问题的 write barrier：每次赋值/构造 `TObjectPtr` 时，若增量 GC 正在运行，立即把目标对象标记为可达。

在 `FMiniGC` 增量实现中，等价逻辑是：若在 `IncrementalMarkStep` 调用之间修改了某个 `FMiniObject*` 引用，需手动调用 `Referenced->bReachable = true`（相当于 `UE::GC::MarkAsReachable`）。

**RF_RootSet 在 UE 中的位置**：

`AddToRoot()` 设置的是 `EInternalObjectFlags::RootSet`（在 `GUObjectArray` 的 `FUObjectItem` 上），而非 `EObjectFlags`（`RF_*` 前缀）里的标志。因此 `HasAnyFlags(RF_RootSet)` 在现代 UE 版本中不是正确检测方式，应使用 `IsRooted()`。

## 常见坑

- **Sweep 在 Mark 之前运行**：顺序必须是先 Mark 再 Sweep；先 Sweep 再 Mark 会删除所有对象。
- **增量 GC 漏标**：在 `IncrementalMarkStep` 调用之间添加了新引用，若不手动标记目标为可达，该对象会被 Sweep 回收（这就是 write barrier 的必要性）。
- **`bReachable` 忘记清零**：每次 GC 开始必须清零所有对象的 `bReachable`；忘记清零则对象永远可达，GC 永远无法回收任何对象。
- **Sweep 阶段用迭代器擦除**：直接用 `TArray` 迭代器在遍历中 `RemoveAt` 导致循环失效；应先置 null 再统一压缩。

## 提示

- UE 的 GC 不会立即调用析构函数，而是先标记 `RF_Garbage`，在 Sweep 阶段调用 `ConditionalBeginDestroy()`/`ConditionalFinishDestroy()`，最终在下一帧的 Purge 阶段才真正释放内存。本题的 `delete` 等价于简化的一步析构。
- `TrackRef(&MemberPtr)` 等价于给成员加 `UPROPERTY`；`UntrackRef(&MemberPtr)` 等价于去掉 `UPROPERTY`——这是本题最核心的类比。
- `GMiniObjectArray` 中的 null 槽在 UE 实际实现中是通过 `FUObjectItem::IsValid()` 标志跳过的，不像本题直接压缩数组。

## 复盘问题

1. 真正开始执行的时刻？（UE 的 `CollectGarbage` 在 GameThread 的帧间空隙被 `UEngine::ConditionalCollectGarbage` 触发；也可显式调用 `CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS)`。GC 绝不在帧执行期间中途介入普通代码。）
2. 谁负责这对象的生命周期？（`FMiniGC` 本身——等价于 UE 的 GC 系统；通过 `bIsRootSet` 和 `TrackedRefs` 链维持可达性，等价于 UE 的 `GRoots` + `PropertyLink/RefLink`。）
3. 涉及哪些命名线程？（UE 的 Mark phase 主要在 GameThread；`GAllowParallelGC=1` 时会并行到 Worker Thread Pool 加速标记，但 GC 启动和 Sweep 仍在 GameThread。）
4. 这对 GC 如何可见？（如果把 `FMiniObject* C` 存在一个未通过 `TrackRef` 注册的变量里，GC 不能看见它——等价于把 `UObject*` 存在无 `UPROPERTY` 的 `TArray` 里，GC 不遍历它，C 会被回收，访问 C 变成悬空指针。）
5. [本题专属] 为什么 UE 选择 mark-sweep 而不是引用计数（`TSharedPtr`）来管理 UObject？（引用计数无法处理循环引用，Actor→Component→Actor 是常见循环；mark-sweep 可以正确处理任意图结构；且 mark-sweep 与反射元数据（PropertyLink）天然结合，不需要在每个指针赋值处手动维护计数。代价是 stop-the-world 或增量 GC 的复杂性。）

## 对应官方参考

- `Engine/Source/Runtime/CoreUObject/Public/UObject/UObjectBaseUtility.h:206`（`AddToRoot()`/`RemoveFromRoot()`）
- `Engine/Source/Runtime/CoreUObject/Public/UObject/GarbageCollection.h:179`（`GIsGarbageCollecting`、`IsGarbageCollecting()`）
- `Engine/Source/Runtime/CoreUObject/Private/UObject/GarbageCollection.cpp:4528`（`PerformReachabilityAnalysis` 主循环）
- `Engine/Source/Runtime/CoreUObject/Private/UObject/GarbageCollection.cpp:634`（`GRoots`、`GGatherUnreachableObjectsState`）
- `Engine/Source/Runtime/CoreUObject/Public/UObject/ObjectPtr.h:306`（`ConditionallyMarkAsReachable`，增量 GC barrier）
- `Engine/Source/Runtime/CoreUObject/Public/UObject/UObjectGlobals.h:930`（`CollectGarbage` 声明）
