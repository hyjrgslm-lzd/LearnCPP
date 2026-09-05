# 练习 D-2：eager vs lazy

## 目标

只改 `initial_suspend` 一行代码，把 lazy task 切换成 eager task，
观察两者执行时机的差异；再实现"基于 lazy 的混合策略"工厂 `eager_start`。

## 必做任务

1. 把 D-1 的 task 模板化：参数 `InitialSuspend` ∈ { suspend_always, suspend_never }。
2. 写 `lazy_compute / eager_compute`，在协程体首行打印日志，
   对比"body 开始"的时机。
3. 写一个含一次 `co_await` 的 multi_step eager，观察它是跑到第一个挂起点还是 `co_return`。
4. 实现 `eager_start(lazy_task)`：返回前先 resume 一次。

## 验收点

- 你能只改一行代码切换 lazy / eager 行为。
- 你能观察到 lazy 时协程体在 `get()` 调用后才执行。
- 你能解释 P3552 选择 lazy 的理由。
- 你能说明 eager 启动在 stop_token 还未注入时的安全风险。

## 提示

- task 的析构必须避免对已销毁帧 destroy；本骨架未做完整 done 检查，进阶时补。
- 测量耗时建议 `chrono::high_resolution_clock` + 多次平均。
- 不要混用 lazy / eager 同一份对象——状态语义不同。

## 本轮练习契约

Starter 要求记录事件顺序。Reference 用同一 task 只切换 initial_suspend：suspend_always 表示 lazy，suspend_never 表示 eager，并断言 eager body 在创建表达式返回前执行。学习目标：知道 task 组合通常偏 lazy，因为启动前还要接 continuation/cancellation/context。

命令：``cmake -S . -B build/dg-lane -DCOROUTINE_STUDY_BUILD_REFERENCE=ON``，然后构建 ``D2_eager_vs_lazy`` 与 ``D2_eager_vs_lazy_reference``，再用 ``ctest -R D2_eager_vs_lazy_reference`` 跑稳定验收。
