# 练习 B-1：互斥量家族与 RAII 锁包装器

> 详尽版见 `../../03-模块B-互斥与锁.md` 的 练习 B-1。

## 目标

建立互斥量家族（`std::mutex` / `std::recursive_mutex` / `std::timed_mutex` / `std::shared_mutex`）与四种 RAII 锁包装器（`std::lock_guard` / `std::unique_lock` / `std::scoped_lock` / `std::shared_lock`）的选型直觉。

## 必做任务

1. 看懂「无锁自增」对照组为何结果错乱（数据竞争 / lost update）。
2. 用 `std::mutex` + `std::lock_guard` 保护计数器，验证结果恒等于期望值。
3. 用 `std::shared_mutex` + `std::shared_lock`（读）/ `std::lock_guard<std::shared_mutex>`（写）实现读多写少结构。
4. 用 `std::unique_lock` 在临界区内提前 `unlock()`，把耗时活移出锁外（`lock_guard` 做不到）。

## 进阶任务

1. `std::timed_mutex` 的 `try_lock_for` 限时获取，超时放弃。

## 选型口诀

单锁默认 `lock_guard`；要延迟/移动/配 condvar 用 `unique_lock`；多锁用 `scoped_lock`；读写锁读端 `shared_lock`、写端 `lock_guard<shared_mutex>`。

## 验收点（can-do）

- 你能展示「无锁错乱 vs lock_guard 恒正确」的对照输出。
- 你能解释 `shared_mutex` 下「N 读者并发 + 写者独占」为何无数据竞争。
- 你能各举一例说明何时 `lock_guard` 够用、何时必须用 `unique_lock`。

## 构建运行

```bash
cmake --build build-vs2026 --target B1_mutex_family --config Release
```

按 `// TODO [必做 N]:` / `// TODO [进阶 N]:` 填写后重新构建运行，对照文档自检。

## 对应官方参考

- cppreference：`std::mutex` / `std::shared_mutex` / `std::timed_mutex`
- cppreference：`std::lock_guard` / `std::unique_lock` / `std::shared_lock`
- 《C++ Concurrency in Action, 2nd ed.》(Williams) 第 3 章
