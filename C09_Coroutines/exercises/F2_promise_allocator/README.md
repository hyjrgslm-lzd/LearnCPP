# F-2 自定义 promise allocator

对应正文：[08 模块 F](../../08-模块F-协程帧与allocator.md#f2)。

本练习让 promise 接管 coroutine state 的分配入口。目标是观察分配 size、地址和释放配对。

## Part 1：理解这个 pool 的限制

starter 的 pool 是教学用 bump/pool 结构。它能统计分配次数、释放次数和 size，可能不支持真正复用每个块。这个限制可以接受，因为本题只证明编译器是否调用 promise `operator new/delete`。

## Part 2：写正确签名

promise 中的分配函数签名要匹配：

```cpp
static void* operator new(std::size_t size);
static void operator delete(void* ptr, std::size_t size) noexcept;
```

如果签名写错，编译器可能走默认分配路径。先在分配函数里记录 size，确认它实际被调用，再做性能对比。

## Part 3：观察帧大小

运行 `tiny_coro` 和 `wider_coro`。更多跨挂起局部、更复杂局部对象、更多 awaiter 临时，通常会让 frame size 变大。具体数字由编译器决定。

记录时写成：

```text
tiny_coro: size = ...
wider_coro: size = ...
原因：wider_coro 有 ... 跨挂起状态
```

## Part 4：allocation failure

`get_return_object_on_allocation_failure` 只有配合 non-throwing allocation function 才走。若 `operator new` 失败时抛异常，这个 hook 不负责把异常变成空 task。

Reference 覆盖 nothrow 失败到空 task。实现时不要在空 task 上继续 `resume()` 或读取 promise。

## 验收

- 控制台能看到自定义 `operator new/delete` 的调用。

  **答案解析：** 协程需要动态分配 coroutine state 且未被 elide 时，编译器会先在 promise scope 查找 allocation function。F-2 的 `pool_task::promise_type::operator new` 记录分配次数和地址，`operator delete` 记录释放次数；看到这两个 hook 的输出，才说明本题的 promise allocator 路径被选中。
- 分配与释放计数配对。

  **答案解析：** 返回对象 owner 析构时会对挂起的 coroutine frame 调用 `destroy()`，随后走 promise scope 的 deallocation function。正常创建、运行、离开作用域后，alloc/free 计数应相等。reference 的 `allocs == frees` 断言证明没有漏释放或错误绕过 delete hook。
- `tiny_coro` 与 `wider_coro` 的 size 差异有解释。

  **答案解析：** 传入 `operator new(size)` 的 size 是当前编译器计算的 coroutine state 大小。更多跨挂起点局部、非平凡对象和 awaiter 临时通常会让 size 变大，但具体数字依赖工具链、优化级别和 ABI。答案应写观察到的数值和源码中导致差异的状态来源，不写固定标准值。
- 你能说明 promise allocator 比替换全局 new 更精确，因为它按 promise 类型定制。

  **答案解析：** promise scope 的 `operator new/delete` 只影响使用该 promise type 的协程帧分配，能把统计、池化或失败策略限定在某类 coroutine return object 上。替换全局 new 会影响整个进程的大量普通对象分配，噪声和副作用都更大。F-2 的 hook 放在 `pool_task::promise_type`，因此证据直接对应本题 task。

## Reference

Reference 覆盖 aligned allocation、delete 配对、nothrow allocation failure 到空 task。

```powershell
cmake -S . -B build/dg-lane -DCOROUTINE_STUDY_BUILD_REFERENCE=ON
cmake --build build/dg-lane --config Release --target F2_promise_allocator F2_promise_allocator_reference
ctest --test-dir build/dg-lane -C Release -R F2_promise_allocator_reference
```
