# I-3 Folly coro Task 与 SafeTask

对应文档：`11-模块I-真实异步IO与并发框架.md` 「练习 I-3」。

## Windows 默认跳过

`Folly` 实际上几乎不在 Windows 走得动（vcpkg + POSIX-isms），所以 CMakeLists 顶部
有 `if(WIN32) return() endif()`。Linux/macOS 上才会真正构建。

## 目标

把 `lazy_task` 升级为 Folly 风格的 `SafeTask`——通过：
1. 编译期 `static_assert` 禁止裸引用 / 顶层 const，把"悬空引用 Bug"从运行时拉到编译时。
2. 运行时一次性 await 断言，保护 fire-and-forget 语义。
3. Executor 注入（`co_via_if_async` / `co_reschedule_on_current_executor`）—— 显式
   优于隐式。
4. FIFO 公平 `CoMutex`——等待者链表 + `await_suspend` 入队 + 逐个唤醒。

## 必做任务

1. 阅读骨架：
   - `my_safe_task<T>` 中的两个 `static_assert`。
   - `my_safe_task::await_suspend` 中的 `compare_exchange_strong` 一次性断言。
   - `co_mutex` 的 FIFO 队列实现。
2. 跑通 3 个测试场景。
3. 解开测试 1 中两行注释（`my_safe_task<int&>` / `my_safe_task<const int>`），确认
   编译错误信息可读。
4. 列举 SafeTask 比 lazy_task 更安全的 3 个设计决策。

## 进阶任务

- 把 `std::queue<coroutine_handle>` 换成侵入式链表（节点嵌入协程帧），消除堆分配。
- 阅读 `folly::coro::NowTask` 源码，复述其"Now"设计意图（约束在同一语句中 co_await）。
- 实现 `co_cleanup` 协议——一个 RAII guard，析构时 co_await 注册的回调。

## Folly API 概念示意（不可直接编译）

骨架中有几段 `// 以下是 Folly API 概念示意——不可直接编译` 的代码段，仅供对照阅读。
真正的 Folly API 需链接 `folly/experimental/coro/*.h`。

## 验收点

- 解开 `my_safe_task<int&>` 的注释能产生清晰的 static_assert 错误信息。
- 同一个 SafeTask 被 co_await 两次会触发 abort（运行时一次性断言）。
- CoMutex 在单线程环境下严格按入队顺序授予锁。
- 你能讲清 Folly "把更多错误从运行时拉到编译时" 的设计哲学。

## 提示

- `compare_exchange_strong` 是原子一次性 flag 的标准做法。
- 单线程 CoMutex 的 unlock 直接 resume 下一个 waiter——多线程要加锁保护队列。
- Folly 的 `NowTask` 通过代码审查约束而非类型系统强制——理解为什么。
