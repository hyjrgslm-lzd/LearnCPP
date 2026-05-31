> 对应章节: ../../../03-模块B-核心容器与字符串.md §练习 B-1

## 目标

通过一系列可测量的实验，把 UE 三大容器（`TArray`/`TMap`/`TSet`）在增删改过程中的内存语义和迭代器失效规律刻进直觉，重点理解与 STL 容器的差异——不是"会用 API"，而是"能预测一次操作后容器内存状态和已持有指针/迭代器的有效性"。

## 前置理解

- 已完成模块 A 三题，理解 `.Build.cs` 依赖声明（本题只需依赖 `Core`）
- 熟悉 `std::vector` 迭代器失效规则（`push_back` 触发重分配使所有迭代器失效）
- 熟悉 `std::unordered_map` rehash 后指针失效行为
- 理解 cache 局部性基本概念

## 必做任务

1. 完成 `DemoTArrayReserve` 中的 TODO 1：不 Reserve 直接 Add，对比地址是否改变，理解触发重分配的条件
2. 完成 `DemoRemoveAtVsSwap` 中的 TODO 2：写下 `RemoveAt`（O(N) 保序）vs `RemoveAtSwap`（O(1) 破序）的结论
3. 完成 TODO 3：在 ranged-for 中修改 TArray 大小，在 Debug 构建下观察 `TCheckedPointerIterator` 的 `ensureMsgf` 警告
4. 完成 TODO 4：演示 `TMap::FindOrAdd` 返回引用，在 rehash 后引用悬空
5. 完成 TODO 5：用 `FPlatformTime::Cycles64()` 对比 `TSet::Contains` 与 `TArray::Contains` 在 N=1000 时的耗时差

## 进阶任务

- 完成 TODO 进阶 1：`TInlineAllocator<16>` vs 默认堆分配的 micro-benchmark，测量 16 个元素求和耗时差
- 阅读 `SparseArray.h`：`TSparseArray` 的"稀疏"体现在哪里（删除后位置标记为空洞而非移位），以及为何能保证未删元素的指针稳定
- 搜索 `TChunkedArray` 使用场景，理解分块数组为何能保证指针稳定性

## 验收点

- [ ] 能不查文档口述 `RemoveAt` 和 `RemoveAtSwap` 的时间复杂度差异及适用场景
- [ ] 能解释 `TArray` 在 `Add` 触发重分配后，所有已持有的 `ElementType*` 指针都失效的原因
- [ ] 能解释 `TMap::FindOrAdd` 返回引用在 rehash 后失效的根本原因（underlying storage 重分配）
- [ ] 做过 `TSet` vs `TArray::Contains` 的耗时对比，能用数字说明哈希查找在 N=1000 时的优势
- [ ] 能答出四个固定复盘问题

## 观察点

- `TArray::RemoveAt`（`Array.h:2083`）通过 `DestructItems` + `FMemory::Memmove` 完成移位；`RemoveAtSwap`（`Array.h:2185`）用最后一个元素覆盖被删元素，再缩短 `ArrayNum`；两者都接受 `EAllowShrinking` 参数控制是否 `ReallocShrink`
- `TArray` 的增长策略是"几何 slack"（`CalculateSlackReserve`，`ContainerAllocationPolicies.h`），实际增长量大于请求量，减少 realloc 频率
- `TMap` 内部用 `TSet<TPair<KeyType, ValueType>>` 存储，`TSet` 内部用 `TSparseArray` + 哈希桶；rehash 时 `TSparseArray` 底层 storage 重分配，之前通过 `Find()` 取到的指针/引用失效——这与 `std::unordered_map` 标准保证的节点稳定性相反

## 常见坑

- 在 ranged-for 中修改 TArray 大小：Debug 构建下 `TCheckedPointerIterator`（开启条件：`TARRAY_RANGED_FOR_CHECKS`）会通过 `ensureMsgf` 报警，但 Shipping 构建静默产生未定义行为；安全做法：收集待删索引或用倒序删除
- 误以为 `TMap::FindOrAdd` 返回的引用在任何操作后都有效：引用仅在没有触发 rehash 的操作期间有效；操作完成后立刻让引用出作用域
- 混淆 `TArray::Num()` 和 `Max()`：`Num()` 是已有元素数，`Max()` 是已分配容量；`Reserve(N)` 只确保 `Max() >= N`，不改变 `Num()`

## 提示

- 测量 cycle 时，把测量体放在大循环里，只取循环总耗时再除以次数，减少单次调用开销的干扰
- `TInlineAllocator` micro-benchmark 受 CPU cache 状态影响大；建议两次测量前用足够大的"噪声数据"填充 L1，使 warm/cold 状态一致
- 阅读 `Array.h` 的 `TCheckedPointerIterator`（第 204 行附近）：理解 `TARRAY_RANGED_FOR_CHECKS` 宏在 Debug 和 Shipping 构建下的行为差异

## 复盘问题

1. **真正开始执行的时刻**？`TArray::Add` 触发的重分配在 GameThread 的 calling context 里同步发生，没有延迟执行；本题不涉及 RenderThread。
2. **谁负责这个生命周期**？所有容器对象以栈变量或局部对象形式存在；`TArray` 持有 heap storage 的所有权，析构时调用 `DestructItems` + 释放内存；`TInlineAllocator` 的 inline 部分与对象共同析构；不涉及 UObject/GC。
3. **涉及哪些命名线程**？本题全部在 GameThread（或按你如何驱动代码的 TaskGraph worker）；不涉及 RenderThread/RHIThread。
4. **这对 GC 如何可见**？本题不涉及 UObject；`TArray<int32>`、`TMap<FString, int32>` 都是纯 C++ 值类型，GC 不感知它们；如果容器存在于 UObject 成员里，必须加 `UPROPERTY`（模块 D 再讲）。
5. **本题专属**：为什么 `RemoveAtSwap` 的覆盖操作用 `MoveOrCopy` 而不是直接 `memcpy`？（答：因为元素类型可能有非平凡的析构函数或移动语义；直接 memcpy 会跳过析构和移动构造，导致资源泄漏或双重析构；必须通过 `MoveOrCopy` 正确处理析构和移动构造。）

## 对应官方参考

- `Engine/Source/Runtime/Core/Public/Containers/Array.h`：`RemoveAt:2083`、`RemoveAtSwap:2185`、`Reserve`、`TCheckedPointerIterator:204`
- `Engine/Source/Runtime/Core/Public/Containers/Map.h`：`FindOrAdd:365`
- `Engine/Source/Runtime/Core/Public/Containers/SparseArray.h`：`TSparseArray` 定义，稳定指针语义
- `Engine/Source/Runtime/Core/Public/Containers/ContainerAllocationPolicies.h`：`TInlineAllocator:1073`、`CalculateSlackReserve`
