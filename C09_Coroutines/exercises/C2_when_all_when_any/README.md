# 练习 C-2：`when_all` / `when_any`

先读 [模块 C 的组合章节](../../04-模块C-取消与组合.md#c2)。本题用最小实现观察组合语义，不追求通用库。

## Part 1：三个模拟 task

打开 [main.cpp](main.cpp)，确认三个 fetch 都接收可选 `std::stop_token`，延迟不同：

- cache：50ms，返回 100。
- db：150ms，返回 200。
- remote：300ms，返回 300。

串行等待约等于三者相加；这个输入只用于观察现象。正确性检查不用耗时阈值，而是使用 gated child 证明所有分支先启动后释放。

## Part 2：`when_all`

把安全 TODO stub 改成真正组合器：先启动全部 child，再等待全部完成并 `co_return std::make_tuple(...)`。可以为每个 task 起 `std::jthread`，也可以用单线程事件循环/run loop fan-out；协程并发不等于多线程。

异常路径要保存第一个异常，并通过同一个 `std::stop_source` 请求其它 task 停止。main 已示例把 `all_src.get_token()` 传给 cache/db/remote；你完成并发版时要把这个 source 接进失败路径。请求取消后仍要等齐线程，避免 loser 继续访问资源。

## Part 3：`when_any`

超时模式由两个 task 竞速：

```cpp
when_any(fetch_remote(), timeout_after(200ms))
```

当前 starter 的 `when_any(stop_source, ta, tb)` 已把 source 参数摆到接口上；占位实现只等待第一条任务。完成时需要并行驱动两条任务，winner 写结果后调用 `stop_source.request_stop()`，再 join loser。本仓库共享 [runtime.hpp](../include/coroutine_study/runtime.hpp) 已提供二选一 helper `coroutine_study::when_any_cancel_join(stop_source, first, second)`，可先用它观察 winner/loser 收束。

## Part 4：改阈值观察

这组 200ms/400ms 输入只作为单独观察：timeout 和真实任务谁先完成。解释变化时不要只看返回值，要指出 stop 请求和 join 发生在哪里；不要把这组耗时观察当成当前 gated checker 的通过条件。

**答案解析：** 200ms 阈值小于 remote 的 300ms 延迟，所以 timeout 分支先完成，组合器记录 timeout marker 后请求 stop，并等待 remote 在检查点收束。400ms 阈值大于 remote 延迟，remote 先返回 300，组合器再请求 stop 让 timeout 分支退出或收束。两次实验的关键差异是 winner 改变，收尾动作仍是“请求 stop + join loser”。

## 验收

- `when_all` 返回 `(100, 200, 300)`，并且 gated child 全部启动后才释放。

  **答案解析：** cache/db/remote 的结果槽顺序保持 `(100, 200, 300)`。正确性门禁不看 300ms/500ms 耗时，而看受控 gate：每个 child 到达挂起点后记录 started；只有三个 child 都到达后才释放并允许完成。这样能证明组合器先 fan-out，再收束。

- `when_any` 能表达超时竞速。

  **答案解析：** 超时模式把真实任务和 `timeout_after(Nms)` 放进同一个竞速组合。N=200ms 时，remote 需要 300ms，timeout 先完成并返回超时标记；N=400ms 时，remote 会先返回 300。这个结果说明 winner 由完成时间决定，调用者不用把超时逻辑塞进 remote 自己的实现。

- loser 收到 stop 请求并被收束。

  **答案解析：** 当前 starter 的目标是 winner 写入结果后调用同一个 `stop_source.request_stop()`，再等待 loser 完成。共享 `runtime.hpp` 的 `when_any_cancel_join(stop_source, first, second)` 已提供这个二选一观察 helper：首个 value 或 exception 完成者成为 winner，helper 请求取消并 join 另一条任务。stop 只是请求，所以 loser 仍要在自己的检查点响应，组合器仍要等它收束。

- 子 task 抛异常时，整体传播异常，同时请求其它 task 停止。

  **答案解析：** `when_all` 中任一 child 失败时，组合器保存第一个异常，并通过共享 `stop_source` 通知其它 child 尽快停止。它不能立刻丢下其它线程或协程，因为 loser 可能还持有资源或 handle；必须 join 等齐后再把异常重新抛给父协程。这样异常传播和生命周期收束同时成立。

## Student 检查

`main.cpp` 现在用受控 gate 检查组合语义；耗时只保留为观察现象，不作为正确性判据：

| Part | 操作 | 本地检查 |
| --- | --- | --- |
| Part 1 | 三个 fetch 返回固定槽位 | `when_all` 返回 `(100, 200, 300)` |
| Part 2 | 并发 `when_all` | 三个 gated child 必须都到达挂起点后才释放；安全 TODO stub 会有限失败，避免旧串行占位挂死 |
| Part 3 | `when_any` 双分支竞速 | 两个 gated branch 必须都启动后才允许选择 winner；不检查线程 ID |
| Part 4 | 200ms/400ms 超时对照 | 作为单独观察输出保留，不能替代 gated 正确性检查 |

完成前：`when_all/when_any` 的 TODO stub 会有限失败；学生实现后，同一个 gate 可以接受单线程事件循环或多线程 fan-out。
