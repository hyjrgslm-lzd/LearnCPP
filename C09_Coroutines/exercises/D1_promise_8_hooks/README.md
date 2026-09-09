# 练习 D-1：从零写 `lazy_task<T>`

对应正文：[06 模块 D](../../06-模块D-promise_type全解.md#d1)。

本练习第一次要求你离开公共 `include/coroutine_study/lazy_task.hpp`，在本目录 `main.cpp` 中写出一个完整教学 task。每个 hook 都要对应生命周期中的一个可观察时刻。

## 做题前先确认这条链

调用 `compute()` 时，结果还不存在。编译器先分配 coroutine state，构造 promise，调用 `get_return_object()` 得到 `lazy_task`，再执行 `initial_suspend()`。当它返回 `std::suspend_always` 时，协程体第一行尚未执行，调用方已经拿到 task。

之后 `get()` 或 `co_await` 才启动协程体。`co_return` 调用 `return_value` 保存结果；未捕获异常调用 `unhandled_exception` 保存 `exception_ptr`；协程体结束后进入 `final_suspend`，frame 继续存在，等待 owner 读取结果并销毁。

## Part 1：创建返回对象

补 `get_return_object()`：

```cpp
return lazy_task{
    std::coroutine_handle<promise_type>::from_promise(*this)
};
```

这一步把 promise 地址转换成 handle。handle 是之后 `resume()`、`destroy()`、访问 promise 的唯一入口。此时协程体局部变量还没按源码执行，不能从这里读取业务结果。

## Part 2：选择 lazy 启动

`initial_suspend()` 返回 `std::suspend_always{}`。运行时你应看到：调用协程函数后，协程体日志尚未出现。直到 `get()` 调用 `resume()`，协程体才开始执行。

如果这里改成 `std::suspend_never{}`，D-2 会观察到 eager 行为。本题保持 lazy，因为 nested `co_await` 需要先把 continuation 接好再启动子 task。

## Part 3：保存值和异常

`return_value(T value)` 只负责把 `co_return` 的值移入 promise。`unhandled_exception()` 只负责保存 `std::current_exception()`。不要在 `unhandled_exception()` 里 `throw;`，否则错误会绕开本题设计的消费通道。

成功路径预期：

```text
compute() -> get() -> return_value(30) -> get() 读出 30
```

异常路径预期：

```text
faulty() -> get() -> unhandled_exception 保存 runtime_error -> get() rethrow
```

## Part 4：最终挂起与 continuation

`final_suspend()` 返回自定义 awaiter。它的 `await_ready()` 返回 `false`，保证 frame 停在 final suspend，结果还在 promise 中。`await_suspend()` 有 continuation 时返回它，没有 continuation 时返回 `std::noop_coroutine()`。

嵌套实验：

```cpp
lazy_task<int> inner() { co_return 42; }
lazy_task<int> outer() {
    int v = co_await inner();
    co_return v * 2;
}
```

`outer` 挂起时把自己的 handle 存进 `inner.promise().continuation`。`inner` 完成后，`final_suspend` 返回这个 handle，`outer` 才能从 `await_resume()` 取得 42 并继续。

## Part 5：分配 hook 的边界

`operator new/delete` 参与 coroutine state 分配释放。`get_return_object_on_allocation_failure` 只有在 promise 提供 non-throwing allocation function 且返回 null 时才走。它是分配失败兜底，属于条件路径。

## 验收

- `co_return x + y` 能从 `get()` 读出 30。

  **答案解析：** `co_return x + y` 会调用 `promise_type::return_value`，把计算结果存进 promise 的 `value`。`get()` 启动协程后读取同一个 coroutine state 中的 promise，所以能得到 30。reference 的 `value_task()` 断言覆盖这条成功路径。
- 协程体抛出的 `runtime_error` 能在 `get()` 中重新抛出。

  **答案解析：** 协程体未捕获异常进入 `unhandled_exception()`，教学实现保存 `std::current_exception()`，不在原抛出点直接泄漏到调用者。`get()` 恢复后检查 promise 的 `error` 并 `std::rethrow_exception`，所以异常在消费端重新出现。reference 的 `fail_task()` 验证这一点。
- `outer().get()` 返回 84。

  **答案解析：** `outer` 在 `co_await inner()` 时把自己的 handle 保存为 `inner` 的 continuation，并返回 `inner` 的 handle 让它运行。`inner` 在 `final_suspend` 返回 continuation，`outer` 恢复后从 `await_resume()` 取得 42，再乘 2 返回 84。这个断言验证的是 nested `co_await` 与 final-suspend continuation。
- 移动构造会清空源 handle，避免双重 `destroy()`。

  **答案解析：** coroutine frame 只能有一个 owner 负责销毁。移动构造用 `std::exchange(other.h_, {})` 转移 handle 并清空源对象，源对象析构时不会再 `destroy()` 同一 frame。否则两个 `lazy_task` 析构会重复销毁同一个挂起协程。
- 你能画出 `get_return_object -> initial_suspend -> get/resume -> return_value/unhandled_exception -> final_suspend -> destroy`。

  **答案解析：** 这条链就是协程从创建到销毁的协议顺序：返回对象先拿到 handle，lazy 初始挂起让调用方得到未启动 task，`get/resume` 推进函数体，结果或异常存入 promise，最终挂起保留 frame 给消费者读取，owner 最后 `destroy()` 清理。图里要标出 promise 和 handle，因为所有后续访问都通过它们发生。

## Reference

Reference 验证 value、`exception_ptr`、nested `co_await` 和 final-suspend continuation。所谓 hook 数量是教学分组；标准没有固定八个 hook 清单。allocation failure 需要 non-throwing allocation path。

```powershell
cmake -S . -B build/dg-lane -DCOROUTINE_STUDY_BUILD_REFERENCE=ON
cmake --build build/dg-lane --config Release --target D1_promise_8_hooks D1_promise_8_hooks_reference
ctest --test-dir build/dg-lane -C Release -R D1_promise_8_hooks_reference
```
