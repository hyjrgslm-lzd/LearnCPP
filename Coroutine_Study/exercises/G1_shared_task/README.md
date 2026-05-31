# G-1 从 task 到 shared_task

对应文档：`09-模块G-symmetric_transfer与高级task.md` 「练习 G-1」。

## 目标

把 `lazy_task<T>` 升级为 `shared_task<T>`：允许多个协程同时 `co_await` 同一个
shared_task，每个等待者都能拿到结果（值拷贝）或异常。实现引用计数 + control_block
解耦、final_suspend 的多 awaiter 链表唤醒、并妥善处理 frame 销毁与计数之间的竞态。

## 必做任务

1. 阅读骨架——重点是 `control_block`（引用计数与帧解耦）、`promise_type::final_suspend`
   （前 N-1 个 .resume()，最后一个 symmetric transfer）、`awaiter`（intrusive list node）。
2. 跑通测试 1：拷贝构造增加引用计数，作用域结束统一释放。
3. 跑通测试 2：两个 waiter 协程同时 co_await 同一个 shared_task，slow_compute 的
   final_suspend 会唤醒它们俩。
4. 在笔记中画出引用计数从 1 → 3 → 0 的生命周期图。
5. 标注 3 处竞态窗口：(a) 多线程拷贝 → atomic add；(b) 完成与 await_ready 之间 →
   double-check；(c) 链表头插 vs final_suspend 遍历 → 单线程模式天然安全。

## 验收点

- 拷贝构造能正确增加引用计数；作用域结束时计数归零并销毁帧。
- 两个等待者都能拿到结果，且没有任何 handle 被 resume 两次。
- 你能解释为什么 `await_resume` 必须返回 **拷贝**而非 move。
- 你能列出 control_block 设计的 3 个理由（与 std::shared_ptr 同源）。

## 提示

- 不要试图把引用计数直接放在 promise 里——一旦 frame 销毁，计数信息也随之消失，
  无法做"最后一个 release 才 destroy"的判断。
- 如果只用 `std::list<coroutine_handle<>>` 也能跑通，但 intrusive list 更高效。
- 多线程版需要把 `waiters_head` 的头插改成 atomic CAS，参考 cppcoro 实现。
