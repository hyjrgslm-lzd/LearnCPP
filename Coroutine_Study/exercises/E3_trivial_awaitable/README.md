# 练习 E-3：Trivial Awaitable 与短路优化

## 目标

实现 `await_ready()` 直接返回 `true` 的 Trivial Awaitable，
观察编译器如何在挂起决策阶段完全消除挂起-恢复开销；
实现 P2786R0 风格的简化版 awaiter，理解 HALO 在 awaitable 层面的对应优化。

## 必做任务

1. 实现 `always_ready`：`await_ready` 恒为 true。
2. 实现 `conditional_ready`：`await_ready = !should_suspend`，
   观察 fast / slow path 的切换。
3. 实现 P2786 风格 `trivial_awaitable`：`await_resume` 返回 std::string。
4. 在 Clang 下用 `-O2 -Rpass=coroutine-elide` 观察是否触发 elide remark；
   在 Godbolt 上观察是否还产生 `call operator new`。
5. 对比 `always_ready` 与 `suspend_always` 的连续 co_await 次数下的开销。

## 验收点

- 你能让多次 `co_await always_ready{}` 在优化模式下不产生挂起开销。
- 你能解释 `std::suspend_never` 是 trivial awaitable 的最典型代表。
- 你能说明 P2786R0 想消除的是"显然不挂起场景下的样板"。
- 你理解 trivial awaitable + HALO 让"看起来是协程"的函数与普通函数等效。

## 提示

- debug / -O0 模式下不会出现 elide；务必至少 -O1。
- 对照 `await_ready=true` 时 `await_resume` 仍会被调用——它跳过的是 `await_suspend`。
- 当前标准下不能删除 `await_suspend`；P2786 还未被采纳。
