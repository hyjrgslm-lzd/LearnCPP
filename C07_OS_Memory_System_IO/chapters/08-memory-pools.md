# 08 arena 与定长池

arena 和 pool 都是分配策略，不是性能承诺。arena 适合“一批对象一起死”的场景：每次分配只向前推进 offset，单个 deallocate 不做事，`release()` 一次性复位。定长池适合同尺寸对象频繁释放再分配：释放块进入 free list，下一次同尺寸请求复用。

arena 的关键正确性是 alignment、容量和异常。`std::align` 会在当前指针和剩余空间上找满足 alignment 的位置；找不到就抛 `std::bad_alloc`。分配失败不能改变 offset。定长池的关键正确性是 block size、alignment 能力和 free list 不变量；超过能力的请求要拒绝，不能悄悄返回未对齐或太小的块。

最小 free list 形状：

```cpp
struct node { node* next; };
node* free = first_block;
void* allocate() { auto* p = free; free = free->next; return p; }
void deallocate(void* p) { static_cast<node*>(p)->next = free; free = static_cast<node*>(p); }
```

这段代码省略了 block 初始化、alignment、空链表检查和 C++ 对象生命期，不能直接当完整实现。课程实现中，free-list 节点进入空闲块时用 `construct_at` 创建；块分配给用户前销毁节点；用户对象销毁后再把块归还给池并重建节点。本课的实现只为机制教学服务；是否更快由 B01 用相同正确性版本先做分配次数、high-water 和复用计数，再做 1 次预热 + 5 次独立进程采样。没有定位证据前，不声明 pool 或 arena 比标准资源快。

[L05_pmr](../exercises/L05_pmr/README.md) 暴露真实 `counting_resource`、`bounded_arena_resource` 和 `fixed_pool_resource`，B01 直接复用这些类型。`run_allocation_probe(size_t)` 只服务 L05 观察与 checker；正式 B01 从 `alloc heap ITEMS` 开始，以每种资源的上游调用、峰值字节、地址复用和 setup/loop/cleanup 时间定位成本，再做相同工作量的资源对照。两者不能互相替代证据。
