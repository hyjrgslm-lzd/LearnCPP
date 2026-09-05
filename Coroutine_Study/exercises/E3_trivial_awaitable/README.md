# 练习 E-3：Trivial Awaitable 与短路优化

## 目标

实现 `await_ready()` 直接返回 `true` 的 Trivial Awaitable，证明标准要求的短路行为；再用优化 remark/汇编观察当前编译器是否消除分支，并与 HALO 的 frame allocation elision 分开取证。

## 必做任务

1. 实现 `always_ready`：`await_ready` 恒为 true。
2. 实现 `conditional_ready`：`await_ready = !should_suspend`；slow path 进入 bool-returning `await_suspend`，再返回 false，不引入外部调度器。
3. 实现标准三方法 `trivial_awaitable`：`await_resume` 返回 std::string，计数证明 `await_suspend` 未执行。
4. 在 Clang 下用 `-O2 -Rpass=coroutine-elide` 观察是否触发 elide remark；
   在 Godbolt 上观察是否还产生 `call operator new`。
5. 对比 `always_ready` 与 `suspend_always` 的连续 co_await 次数下的开销。

## 验收点

- 你能证明多次 `co_await always_ready{}` 不调用 `await_suspend`，并单独观察当前编译器的代码生成。
- 你能解释 `std::suspend_never` 是 trivial awaitable 的最典型代表。
- 你能区分 ready 短路、死代码消除与 HALO，不把 QoI 结果写成标准承诺。

## 提示

- debug / -O0 下通常保留更多代码；优化级别不是 HALO 或死代码消除的可移植保证。
- 对照 `await_ready=true` 时 `await_resume` 仍会被调用——它跳过的是 `await_suspend`。
- 当前标准下不能因 `await_ready` 恒真就删除 `await_suspend`。

## 本轮练习契约

Starter 要求证明 await_ready 短路。Reference 断言 await_ready=true 时 await_suspend 调用次数为 0，结果直接来自 await_resume。HALO 只能通过编译器诊断/反汇编观察，不能用耗时当作证明。

命令：``cmake -S . -B build/dg-lane -DCOROUTINE_STUDY_BUILD_REFERENCE=ON``，然后构建 ``E3_trivial_awaitable`` 与 ``E3_trivial_awaitable_reference``，再用 ``ctest -R E3_trivial_awaitable_reference`` 跑稳定验收。
