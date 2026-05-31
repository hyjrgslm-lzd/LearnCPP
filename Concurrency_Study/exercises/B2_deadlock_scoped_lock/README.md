# 练习 B-2：死锁与 scoped_lock 多锁原子获取

> 详尽版见 `../../03-模块B-互斥与锁.md` 的 练习 B-2。

## 目标

用两把 `std::mutex` 以相反顺序加锁制造真实死锁，再用 `std::scoped_lock` 一次性原子获取多锁消除死锁，理解修复原理是「破坏循环等待」而非「加超时」。

## 死锁四个必要条件（Coffman）

互斥 / 持有并等待 / 不可抢占 / 循环等待。破坏任意一个即可避免；`scoped_lock` 破坏「持有并等待 + 循环等待」。

## 必做任务

1. 写「相反顺序加锁」反例，观察死锁（程序永久挂起、日志停在等第二把锁）。
   - ⚠️ 代码里用 `kRunDeadlockDemo` 开关默认关闭。手动改 `true` 单独观察后请改回 `false`。
2. 用一行 `std::scoped_lock lk(mA, mB);`（CTAD）修复，验证永不死锁。
3. 在注释/此处说明修复原理：原子获取多锁，破坏循环等待，不是超时重试。

## 进阶任务

1. `std::lock(mA, mB)` + `std::lock_guard(m, std::adopt_lock)`（scoped_lock 的底层语法糖）。
2. 固定加锁顺序（永远先锁地址较小者）；对比与 scoped_lock 的取舍。
3. 最小层级锁 `hierarchical_mutex`（运行期断言加锁顺序，见文档与 Williams 3.2.5）。

## 验收点（can-do）

- 你能现场写出两锁反序死锁，并指出命中哪几个必要条件。
- 你能用 `scoped_lock` 一行修复，并解释原理是破坏循环等待。
- 你能说出至少两种避免多锁死锁的工程手段及适用边界。

## 构建运行

```bash
cmake --build build-vs2026 --target B2_deadlock_scoped_lock --config Release
```

## 对应官方参考

- cppreference：`std::scoped_lock` / `std::lock` / `std::adopt_lock`
- 《C++ Concurrency in Action, 2nd ed.》(Williams) 第 3.2.4 ~ 3.2.6
