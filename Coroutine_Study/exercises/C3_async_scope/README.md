# 练习 C-3：async_scope 与生命周期

## 目标

用一个 `async_scope` 管理 N 个 fire-and-forget 协程任务，
确保 scope 析构前所有任务都已完成；通过故意写出超出 scope 引用的 UB 场景
和 `detach` 对比实验，理解结构化并发为什么是协程工程中必不可少的。

## 必做任务

1. 实现最小 `async_scope`：
   - `spawn(lazy_task<void>)`：纳入 scope 管理。
   - 析构时等齐所有未完成 task。
   - 可选：`on_empty()` 作为 awaiter。
2. 启动 N(>=5) 个延迟不同的 fire-and-forget task。
3. 在 scope 析构前后打印时间戳，验证等齐行为。
4. 故意构造 UB 场景（task 引用 scope 外部已销毁变量），
   分析 scope 的生命周期约束如何防范它。
5. 写一个 `detach(task)` 对比实验，记录两者的差异。

## 验收点

- scope 析构时自动等待所有 spawn 的 task 完成。
- 你能清晰区分 fire-and-forget 与"需要回收返回值"的两类 task。
- 你能解释为什么 `detach` 在工程中几乎总是错的。
- 你能解释 scope 与 `std::jthread` 析构 join 行为的相似之处。

## 提示

- 最小实现只需 `atomic<int> in_flight + condition_variable`。
- 析构循环等齐前必须保证所有 spawn 的 task 都通过某种方式被 resume。
- UB 场景仅用于观察，请勿在生产中复制。
