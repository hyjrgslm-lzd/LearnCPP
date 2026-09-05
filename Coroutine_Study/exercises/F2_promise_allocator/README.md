# F-2 自定义 promise allocator（P0912）

对应文档：`08-模块F-协程帧与allocator.md` 「练习 F-2」。

## 目标

实现一个最小 bump-pool，让 promise 通过重载 `operator new`/`operator delete` 把
协程帧的分配定向到这个 pool。通过统计分配次数和帧大小，建立"协程帧分配 = 可定制"
的直觉。再实现 `get_return_object_on_allocation_failure` 兜底。

## 必做任务

1. 阅读骨架中 `coroutine_pool`：理解 bump 分配的局限——不支持单点回收。
2. 在 `pool_task::promise_type` 中：
   - 重载 `static void* operator new(std::size_t)`。
   - 重载 `static void operator delete(void*, std::size_t)`。
   - 实现 `get_return_object_on_allocation_failure`。
3. 编译运行，观察控制台输出每次分配的 `size` 和地址。
4. 比较 `tiny_coro` 与 `wider_coro` 的帧大小差异，预测哪个会更大并验证。
5. 把 pool 替换为默认 `::operator new`，跑 10000 次创建/销毁，对比耗时。

## 验收点

- 控制台能看到自定义 `op new`/`op del` 的打印——证明编译器调用了你的重载。
- pool 统计正确反映了创建次数与总分配量。
- 你能说出 `get_return_object_on_allocation_failure` 的触发条件。
- 你能解释为什么 P0912 把 allocator 设计成 promise_type 的成员重载（而非 traits）。

## 提示

- `operator new` 签名必须严格是 `static void* operator new(std::size_t)`，否则编译器会
  静默 fallback 到默认 `::operator new`。
- 不要在 `op new` 里跑重逻辑（如 flush）——会破坏后续性能测量。
- 如果想让协程在异常路径上退出，先实现 `unhandled_exception()` 不要 `std::terminate`。

## 本轮练习契约

Starter 要求正确配对 promise operator new/delete。Reference 覆盖 aligned allocation、delete 配对、nothrow allocation failure 到空 task。要点：get_return_object_on_allocation_failure 必须和 non-throwing allocation function 一起使用。

命令：``cmake -S . -B build/dg-lane -DCOROUTINE_STUDY_BUILD_REFERENCE=ON``，然后构建 ``F2_promise_allocator`` 与 ``F2_promise_allocator_reference``，再用 ``ctest -R F2_promise_allocator_reference`` 跑稳定验收。
