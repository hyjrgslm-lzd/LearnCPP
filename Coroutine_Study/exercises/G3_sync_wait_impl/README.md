# G-3 实现 sync_wait

对应文档：`09-模块G-symmetric_transfer与高级task.md` 「练习 G-3」。

## 目标

实现 `sync_wait`：在非协程上下文中驱动 task 到完成，把"协程世界"翻译为"同步阻塞
世界"。这是 main 函数能消费协程结果的标准入口。给两个版本：
1. **手动循环驱动版** —— 适合单线程全协程环境（asio io_context 内的协程链）。
2. **condvar + 独立线程驱动版** —— 适合协程可能在其它线程完成的场景。

## 必做任务

1. 阅读骨架：`sync_wait_simple` 和 `sync_wait_condvar` 已给出。
2. 跑通测试 1（成功路径）和测试 2（错误重抛）。
3. 跑通测试 3：condvar 版本——观察 driver 线程独立运行，主线程 wait。
4. 在笔记中回答：
   - 为什么 `main` 不能是协程？（从帧分配 / 启动 / 销毁三个角度）
   - `sync_wait` 在协程世界与同步世界之间承担什么角色？
   - 简易版 vs condvar 版分别适合什么场景？

## 进阶任务

- 实现 timeout 版本：`sync_wait_with_timeout(task, 5s)`，超时抛异常并销毁帧。
- 实现 spin-wait 版本：用 `std::atomic_flag` 替代 condvar，对比 CPU 使用率。
- 让 sync_wait 同时支持 `lazy_task<void>`（需偏特化或 if constexpr 路径）。

## 验收点

- 简易版能正确驱动 task 跑到完成并取出结果。
- 错误路径能正确把协程内异常重抛到 sync_wait 调用者。
- condvar 版能跨线程驱动 task。
- 你能解释 `sync_wait` 不能在协程内调用（会阻塞当前线程，浪费协程优势）。

## 提示

- 简易版只 30 行——不要在辅助设施上过度设计。
- 关键不是代码量，而是理解"协程挂起恢复 → 线程阻塞唤醒"的精确对应关系。
- condvar 版要小心 task 所有权——move 后由 driver 线程负责 destroy 帧。
