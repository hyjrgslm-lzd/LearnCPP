# L05 allocator 与 pmr

本练习把三层责任拆开：一段 storage 能不能放对象；对象生命期是否已经开始；容器向哪个 `memory_resource` 要 storage。不要把“地址可写”“allocator 返回了指针”和“对象已经构造好”混成一件事。

## Part 1：raw storage

暴露并实现一个单槽 `raw_slot<T>`。`construct` 必须先 `std::construct_at` 成功，再提交 engaged 状态；`destroy` 只在 engaged 时调用 `std::destroy_at`。checker 自带 throwing type，构造函数抛异常后，slot 仍为空，析构不能销毁未构造对象。

## Part 2：counting resource

暴露 `counting_resource : std::pmr::memory_resource`。`do_allocate` 转发 upstream 后记录次数和字节；`do_deallocate` 对称扣回；`do_is_equal` 使用对象身份。checker 自带 upstream 观测器，并用 `std::pmr::vector<std::pmr::string>` 验证嵌套元素也使用同一个资源。

## Part 3：有限 arena

暴露只增长的 `bounded_arena_resource`：用 `std::align` 满足每次请求的 alignment，空间不足抛 `std::bad_alloc` 且不改变 high-water，`do_deallocate` 什么也不做，`release()` 一次性复位。checker 用 64 字节对齐、超大请求和 release 后地址复用覆盖边界。

## Part 4：定长 pool

暴露固定块 `fixed_pool_resource`。分配只接受不超过 block size 且 alignment 不超过池能力的请求；释放块回到 free list；下一次同规格分配应复用地址。free-list 的 `node` 要用 `construct_at` 明确创建；把块交给用户前销毁 node，用户对象销毁后再归还块并重建 node。这个池只为教学服务，不承诺通用 STL allocator 性能。

## 解析

`allocator_traits::construct` 负责在 allocator 给出的 storage 上开始对象生命期；`memory_resource::allocate` 只返回字节 storage。`monotonic_buffer_resource` 的关键代价是单次 deallocate 不回收，直到 `release()` 或析构才整体释放。`unsynchronized_pool_resource` 管理多个尺寸池，但线程安全由名字直接说明：没有外部同步时不能跨线程共享。`run_allocation_probe(size_t)` 是本题的观察与 checker 控制入口，不是正式性能采样入口。B01 直接复用上述真实 resource，先用 `alloc heap ITEMS` 的上游计数和分阶段时间定位基线成本，再评估 arena/pool 等同契约对照；本题不预先声明性能收益。
