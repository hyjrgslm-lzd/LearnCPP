# G3：SPSC 环、双向交接与缓存下标

完整正文：[SPSC](../../topics/queues/03-spsc.md)。[queue_versions.hpp](../include/concurrency_study/queue_versions.hpp) 中 spsc_ring<T,false/true> 是两版真实实现，[solution.cpp](solution.cpp) 为 Reference。

## Part 1：容量与两条同步链

构造参数是可用容量，内部多分配一格。答案：空为 head==tail，满为 next(tail)==head；容量 2 能存两个元素，第三次 push 失败且不改变原数据。Reference 检查容量 1、2、7、64 的填满、拒绝、清空与 FIFO；容量 0 被拒绝。

生产者 release 发布 tail、消费者 acquire 读取，传递数据；消费者读完后 release 更新 head、生产者 acquire 读取，传递可复用空间。读自己独占写入的下标用 relaxed。赋值必须在发布/归还之前完成。每个下标只有一个合法写者，所以不需要 CAS；不能直接用于多生产者或多消费者。

## Part 2：完整序列与存储复用

每种容量、每个版本各传输 20003 个元素。Reference 记录唯一消费者的完整序列，核对 ID 集合与严格顺序。消费者记录在线程私有存储中，异常通过共享驱动取消并回传，不能只打印错误后仍返回 0。

T 要求可默认构造；默认构造只在队列建立时发生，可以失败。复制赋值和析构的 noexcept 由静态断言约束。只可移动类型不在这个接口范围，槽位 T 一直活到队列析构。

bool 保持合法：底层用 `unique_ptr<T[]>` 管理真实独立对象，避免 vector<bool> 压位导致不同槽位共享存储字。普通、缓存两版都在容量 1、2、7、64 下各做 20003 项 bool 回归，逐位检查非恒定序列、满空与复用。另用 const 赋值不抛而可变源赋值会抛的双重载类型验证两版出队均从 const 源复制。bool 回归不使用 vector<bool> 保存输出，也不把 ASan 通过当作 TSan 证据。

## Part 3：缓存对方下标

缓存版在即将跨过已确认边界时才重新 acquire 读取对方下标。旧缓存只少给许可，不允许越过未经确认的边界；因此两版在本课程失败观察契约下保持相同容量和成功 FIFO。Reference 同时检查两版，不把性能提升设为通过条件。

每次 try 的核心没有重试循环，但整个 while 重试传输依赖对方推进。is_lock_free 只报告 head/tail 原子，不证明任意 T 或任意平台的完整 wait-free。

## 构建

```powershell
# 从 Concurrency_Study/exercises 执行
cmake -S G3_spsc_ringbuffer -B build/g3 -G "Visual Studio 18 2026" -A x64
cmake --build build/g3 --config Release
ctest --test-dir build/g3 -C Release --output-on-failure
```

基准 variant 为 spsc 和 spsc-cached，必须 --producers 1 --consumers 1；可用容量与基线一致，具体外部采样见[验证与基准](../../topics/queues/08-validation-and-benchmark.md)。
