# 练习 C-2：when_all / when_any

## 目标

用 `when_all` 并行启动 3 个 task 并汇合结果，用 `when_any` 实现超时竞速模式，
建立并行组合与取消传播的直觉。

## 必做任务

1. 实现简化版 `when_all(t1, t2, t3)`：并行驱动 3 个 task，
   最后将结果合并成 `std::tuple<int, int, int>`。
2. 写 3 个延迟不同的模拟 fetch（cache/db/remote）。
3. 实现简化版 `when_any(t, timeout)`：
   返回第一个完成者，其余被取消。
4. 用 `when_any(fetch_remote(), timeout_after(Nms))` 做超时模式，
   观察阈值变化对结果的影响。
5. 画串行 / when_all / when_any 三种模式的时序图。

## 验收点

- 你能区分 `when_all`（汇合）与 `when_any`（竞速）的语义。
- 你能用 `when_any` 正确实现超时模式。
- 你能解释一个子 task 抛异常时，其余 task 应该如何被通知取消。
- 你能说明并行组合相对手写 mutex+cv 在表达力上的优势。

## 提示

- 学习版可以为每个 task 起一条 `std::jthread` 单独驱动 `.get()`。
- 超时 task 实现极简：`co_await async_sleep{N}; co_return timeout_marker;`
- when_any 的"取消未完成 task"可以用 `std::stop_source` 通知。

## Starter / Reference

- `main.cpp` 是练习骨架，保留 TODO 和串行占位，便于对比耗时。
- `solution.cpp` 是可运行参考实现，`ctest --preset verify-core -C Release -R C2_when_all_when_any_reference`
  会校验 `when_all` 并发汇合、`when_any` 超时胜出，以及 loser 收到取消并被 join。
