> 对应章节: ../../../04-模块C-内存-智能指针-委托.md §练习 C-2

## 目标

理解 `FMemory::Malloc` 这条调用链从全局入口到 `FMallocBinned2` 分桶策略的完整路径；掌握 `TInlineAllocator` 在 `TArray` 上的用法；用 `LLM_SCOPE` 和 `INC_MEMORY_STAT_BY` 观察内存分类统计。

## 前置理解

- 理解 `std::allocator` 的基本机制；本题重点是 UE 在此之上的分层结构（`FMemory` → `GMalloc` → `FMallocBinned2`）。
- 已读 `01-心智模型.md` §4 中关于 UE 内存管理的部分。
- UE5.6 起裸 `GMalloc` 被 `UE_DEPRECATED(5.6, ...)` 标记，新代码应始终用 `FMemory::Malloc/Free`。

## 必做任务

1. 追踪 `FMemory::Malloc(64)` → `GMalloc->Malloc(64, Alignment)` → `FMallocBinned2::Malloc(...)` 的调用链，用 IDE 跳转到定义确认每一级的真实入口。
2. 打开 `MallocBinned2.h`，记录三个关键宏的值：`UE_MB2_MAX_SMALL_POOL_SIZE`（32752）、`UE_MB2_SMALL_POOL_COUNT`（51）、`UE_MB2_LARGE_ALLOC`（65536），写出分桶逻辑的核心思路。
3. 用 `FSmallVertexArray`（`TArray<FVector, TInlineAllocator<8>>`）演示：8 个元素无堆分配；添加第 9 个触发堆分配。
4. 声明 `DECLARE_MEMORY_STAT`，用 `INC_MEMORY_STAT_BY` / `DEC_MEMORY_STAT_BY` 在 Editor 的 `stat Memory` 面板中观察自定义统计项。

## 进阶任务

- 用 `LLM_SCOPE(ELLMTag::Meshes)` 包裹一段分配，在 Editor 控制台执行 `stat LLMFULL` 观察 "Meshes" 类目变化（需启动参数 `-llm`）。
- 用 `LLM_SCOPE_BYNAME(TEXT("ExC2CustomTag"))` 定义自定义标签，观察它在 `stat LLMFULL` 中归属的类目。
- 阅读 `LowLevelMemTracker.h` 顶部宏守卫逻辑（`ENABLE_LOW_LEVEL_MEM_TRACKER = LLM_ENABLED_IN_CONFIG && PLATFORM_SUPPORTS_LLM`），理解 Shipping 构建下 LLM 被关闭的含义。

## 验收点

- [ ] 能画出 `FMemory::Malloc(N)` → `GMalloc->Malloc(N, Alignment)` → `FMallocBinned2::Malloc(N, Alignment)` 的简短调用图。
- [ ] 能说出 Binned2 下小分配（< 32752 字节）走池、大分配（≥ 65536 字节）走 OS 的规则，以及 51 个桶的作用。
- [ ] `TInlineAllocator<8>` 示例跑通，8 个元素无堆分配、第 9 个触发分配的行为可通过注释或日志说明。
- [ ] `INC_MEMORY_STAT_BY` 宏在 `stat Memory` 面板可见，能答出四个固定问题。

## 观察点

- `FMemory` 并非类实例，而是一组静态方法的命名空间（`struct FMemory` with `static` functions）。它的价值在于把 LLM 追踪、AutoRTFM 支持和 `GMalloc` 派发统一封装。
- Binned 分配器的"桶"本质是大小类（size class）：同一大小类的分配共享一个内存页池，分配时只需从链表摘头，Free 时挂回链表——这是 Binned 对小对象的 Malloc/Free 比 `malloc(3)` 更快的关键。
- `TInlineAllocator` 是 UE 容器分配器定制化机制的一部分；`TFixedAllocator<N>` 是硬限容量版本（超过 N 个元素断言失败）。
- LLM 是运行时内存分类统计工具，不是 sanitizer。关闭 LLM（Shipping 默认）后分配元数据不存在，`stat LLMFULL` 无输出。

## 常见坑

- **UE5.6+ 后直接用裸 `GMalloc`**：应通过 `FMemory::Malloc/Free` 调用，裸 `GMalloc` 已被废弃。
- **在 Shipping 构建中依赖 `INC_MEMORY_STAT_BY` 统计结果**：Stats 在 Shipping 中默认剥离（`STATS=0`），相关宏展开为空操作。
- **`TInlineAllocator` 用于大元素**：内联存储占用栈帧，对大型结构体滥用会导致栈溢出。
- **`LLM_SCOPE` 范围不匹配**：LLM_SCOPE 是 RAII 对象；在异步任务或跨帧代码里错误使用会导致统计归错类别。

## 提示

- `FMemory.h` 即 `Engine/Source/Runtime/Core/Public/HAL/UnrealMemory.h`，是 `FMemory` 所在头文件。
- Binned2 的桶边界表在 `MallocBinned2.cpp` 的 `SmallBinSizesInternal` 数组里，共 51 项，从 16 字节到 32752 字节，间隔非均匀（小区间密，大区间稀）。
- `stat LLMFULL` 要求启动参数加 `-llm`（或 `DefaultEngine.ini` 配置），否则 LLM 不追踪。

## 复盘问题

1. 真正开始执行的时刻？（`FMemory::Malloc` 在调用点的当前线程执行；分配器内部可能持锁，并发调用安全但存在竞争。）
2. 谁负责这对象的生命周期？（调用 `FMemory::Free` 的代码；`FMemory::Malloc/Free` 是手动内存管理，没有 RAII 保证。）
3. 涉及哪些命名线程？（分配器本身线程安全；LLM tag 追踪是线程局部的（TLS），跨线程传递对象时 LLM 标签不跟随。）
4. 这对 GC 如何可见？（本题不涉及 UObject，是纯 C++ 分配路径；GC 完全不参与 `FMemory` 层。）
5. [本题专属] `FMallocBinned2` 与 `FMallocBinned`（初代）在锁粒度上的核心差异是什么？Binned3 又在哪方面进一步优化？

## 对应官方参考

- `Engine/Source/Runtime/Core/Public/HAL/UnrealMemory.h`（`FMemory` 静态方法集合）
- `Engine/Source/Runtime/Core/Public/HAL/MemoryBase.h`（`FMalloc` 基类，`GMalloc` 全局指针声明）
- `Engine/Source/Runtime/Core/Public/HAL/MallocBinned2.h`（`UE_MB2_MAX_SMALL_POOL_SIZE`、`UE_MB2_SMALL_POOL_COUNT`、`UE_MB2_LARGE_ALLOC`）
- `Engine/Source/Runtime/Core/Public/HAL/LowLevelMemTracker.h`（`LLM_SCOPE`、`LLM_SCOPE_BYNAME`、`ELLMTag`）
