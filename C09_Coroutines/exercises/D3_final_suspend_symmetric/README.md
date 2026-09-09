# 练习 D-3：final_suspend + symmetric transfer

对应正文：[06 模块 D](../../06-模块D-promise_type全解.md#d3)。

本练习关注 task 完成时的最后一步。协程 A 等待协程 B 时，B 完成后要恢复 A。你要比较两种写法：在 `final_suspend` 里直接 `.resume()`，以及从 `await_suspend` 返回 continuation handle。

## Part 1：返回 continuation handle

目标实现：

```cpp
std::coroutine_handle<> await_suspend(
    std::coroutine_handle<promise_type> h) noexcept {
    auto next = h.promise().continuation;
    return next ? next : std::noop_coroutine();
}
```

`await_ready()` 返回 `false`，所以 frame 停在 final suspend。返回 handle 表示当前协程完成后控制权交给 `next`。

## Part 2：对照直接 resume

对照版在 `final_suspend` 中写：

```cpp
if (continuation) continuation.resume();
```

这会在当前库代码的调用栈中恢复等待者。等待者如果马上完成，又会在自己的 `final_suspend` 中恢复更上一层。嵌套很深时，库层 `.resume()` 调用会线性增长。

## Part 3：1000 层链路实验

`chain(n)` 的最内层返回 0，每一层等待下一层并加 1。运行 10、100、1000 层，观察 symmetric 版能沿 continuation 链恢复到最外层。

要写下的控制流：

```text
leaf 完成
  -> leaf.final_suspend 返回 caller_1
  -> caller_1 await_resume 取得值，继续完成
  -> caller_1.final_suspend 返回 caller_2
  -> ...
```

标准保证的是返回 handle 的控制转交语义。机器级 tail call、恒定机器栈、具体汇编形状都要按编译器实测记录。

## 验收

- symmetric 版 `chain(1000).get()` 返回 1000。

  **答案解析：** 每层 `chain(n)` 都等待下一层，下一层完成时在 `final_suspend` 返回上一层 continuation。这样控制权沿 handle 链回到最外层，逐层执行 `await_resume()` 并加 1，最终得到 1000。reference 断言的是这条控制转交流程可跑通。
- continuation 为空时返回 `std::noop_coroutine()`。

  **答案解析：** `final_suspend.await_suspend` 必须返回一个可被恢复的 handle。没有等待者时返回空 handle 会导致恢复空 handle 的未定义行为；`std::noop_coroutine()` 是标准提供的无副作用目标。D-3 的 final awaiter 用它作为无 continuation 的稳定落点。
- 你能解释 `void`、`bool`、`coroutine_handle<>` 三种 `await_suspend` 返回值的差异。

  **答案解析：** `void` 表示保持挂起，恢复责任交给 awaiter 保存的外部入口；`bool true` 也是保持挂起，`bool false` 表示本次挂起取消并立即继续。`coroutine_handle<>` 表示当前协程挂起后恢复返回的目标 handle。D-3 使用第三种让完成协程把控制权交回 continuation。
- 你能指出 direct-resume 版增长的是库代码嵌套恢复调用。

  **答案解析：** direct-resume 在 `final_suspend` 的库代码里直接调用 `continuation.resume()`，等待者若马上完成，又会在自己的 final awaiter 里继续调用下一层 `resume()`。深链时增长的是这些库层恢复调用的嵌套深度。返回 handle 把下一个恢复目标交给协程恢复机制，标准保证控制转交语义，机器级栈表现仍需实测。

## Reference

Reference 构造 1000 层 await 链，验证 handle-return control transfer 能沿 continuation 链恢复。

```powershell
cmake -S . -B build/dg-lane -DCOROUTINE_STUDY_BUILD_REFERENCE=ON
cmake --build build/dg-lane --config Release --target D3_final_suspend_symmetric D3_final_suspend_symmetric_reference
ctest --test-dir build/dg-lane -C Release -R D3_final_suspend_symmetric_reference
```
