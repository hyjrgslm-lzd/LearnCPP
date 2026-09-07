# 练习 E-3：Trivial Awaitable 与短路优化

对应正文：[07 模块 E](../../07-模块E-awaitable三层与co_await变换.md#e3)。

本练习把三件事分开：ready 短路是标准语义，挂起分支消除是优化器结果，HALO 是 coroutine state 分配优化。

## Part 1：恒 ready awaiter

实现 `always_ready`：

```cpp
bool await_ready() noexcept { return true; }
void await_suspend(std::coroutine_handle<>) noexcept { ++suspend_calls; }
int await_resume() noexcept { return 7; }
```

运行后预期 `await_resume` 的值被使用，`suspend_calls == 0`。这证明 `await_ready()==true` 时不求值 `await_suspend()`。

## Part 2：条件 ready awaiter

`conditional_ready` 用运行时字段决定：

```cpp
bool await_ready() const noexcept { return !should_suspend; }
bool await_suspend(std::coroutine_handle<>) noexcept { return false; }
```

`should_suspend=false` 时直接走 ready fast path。`should_suspend=true` 时进入 `await_suspend`，但返回 `false`，立即继续。两个路径都不长期挂起，但能证明 ready 判断与 bool-returning suspend 判断发生在不同阶段。

## Part 3：三方法仍需良构

即使 `await_suspend` 运行时不会调用，表达式仍要在语义分析中良构。不要因为 ready 恒真就删除 `await_suspend` 或把签名写错。

## Part 4：观察优化，不当作语义

用 Clang remark、GCC dump、MSVC 反汇编或 Godbolt 看两件事：

- 挂起分支是否被删除。
- coroutine frame allocation 是否被 HALO 省略。

它们都依赖编译器、优化级别和代码形状。计数器证明标准行为，汇编证明当前实现结果。

## Reference

Reference 断言 `await_ready=true` 时 `await_suspend` 调用次数为 0，结果直接来自 `await_resume`。HALO 只能通过编译器诊断/反汇编观察。

**答案解析：** `await_ready()` 返回 true 是标准语义上的 ready 短路，运行计数器应显示 `await_suspend` 没有被调用，但 `await_resume()` 仍交付 7。挂起分支是否被优化器删除、coroutine frame allocation 是否被 HALO 省略，都属于当前工具链实现观察。记录时用计数器证明控制流，用 remark/dump/汇编证明优化，别用时间差当唯一证据。

```powershell
cmake -S . -B build/dg-lane -DCOROUTINE_STUDY_BUILD_REFERENCE=ON
cmake --build build/dg-lane --config Release --target E3_trivial_awaitable E3_trivial_awaitable_reference
ctest --test-dir build/dg-lane -C Release -R E3_trivial_awaitable_reference
```
