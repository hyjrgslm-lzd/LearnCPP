# 练习 D-2：eager vs lazy

对应正文：[06 模块 D](../../06-模块D-promise_type全解.md#d2)。

本练习只改变一个协议点：`initial_suspend()`。你要用日志证明，lazy 和 eager 的差别是协程体何时开始执行。

## 做题前先确认两种 awaiter

`std::suspend_always` 的 `await_ready()` 返回 `false`，所以初始挂起真正发生。调用协程函数会返回一个尚未执行协程体的 task。

`std::suspend_never` 的 `await_ready()` 返回 `true`，所以初始挂起被短路。协程体在调用表达式返回前开始执行，直到第一个后续挂起点或完成。

## Part 1：同一 task，切换 `initial_suspend`

Starter 使用模板参数或别名表达两种版本。你只需要让 lazy 版返回 `suspend_always`，eager 版返回 `suspend_never`，然后保留其余 promise hook 不变。

预期日志：

```text
lazy 创建前
lazy 创建后
lazy body

eager 创建前
eager body
eager 创建后
```

这说明 `get_return_object()` 总是在协程体前发生，但 `initial_suspend()` 决定返回给调用方之前是否继续进入 body。

## Part 2：eager 跑到哪里停

在 `multi_step()` 中先打印 `step 1`，再 `co_await async_sleep`。eager 创建后会立即打印 `step 1`，随后停在这个 awaiter 选择的挂起点。`step 2` 要等 awaiter 恢复当前协程后才出现。

这条观察很重要：eager 只跳过初始挂起；后续真实挂起点仍会让协程暂停。

## Part 3：`eager_start(lazy_task)` 的语义

`eager_start` 保留 lazy task 类型，但在返回前主动启动一次：

```cpp
template <class T>
lazy_task<T> eager_start(lazy_task<T> t) {
    if (auto h = t.handle()) h.resume();
    return t;
}
```

调用方收到的是已经启动过的 task。后续 `get()` 需要识别已启动状态，不能再次无条件 `resume()` 同一帧。Capstone5 reference 会把二次 `start()` 当作错误。

## 复盘

lazy 更适合组合器。`when_all` 可以先收集两个 task，接好结果槽、continuation 和取消上下文，再启动它们。eager 适合创建即启动的应用级操作，但启动发生得早，调用方还没机会注入 stop token 或调度上下文。

**答案解析：** lazy/eager 的差别只来自 `initial_suspend` 的 awaiter：`suspend_always` 让调用方先拿到 task，`suspend_never` 让协程体在调用表达式返回前开跑。组合器需要先建立 parent handle、结果槽和取消关系，再统一启动子 task，所以 lazy 更容易维持协议。`eager_start(lazy_task)` 虽然返回类型没变，但已经 resume 过一次，后续消费端必须知道不能二次启动。

## Reference

Reference 用同一 task 只切换 `initial_suspend`，断言 eager body 在创建表达式返回前执行。

```powershell
cmake -S . -B build/dg-lane -DCOROUTINE_STUDY_BUILD_REFERENCE=ON
cmake --build build/dg-lane --config Release --target D2_eager_vs_lazy D2_eager_vs_lazy_reference
ctest --test-dir build/dg-lane -C Release -R D2_eager_vs_lazy_reference
```
