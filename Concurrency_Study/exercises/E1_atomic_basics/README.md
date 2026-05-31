# 练习 E-1：原子操作基础

> 详尽版见 `../../07-模块E-原子操作基础.md` 的 练习 E-1。

## 目标

掌握 `std::atomic<T>`（原子类型，`<atomic>`，C++11）的核心操作：`load` / `store` / `exchange` 与读-改-写（read-modify-write，RMW）家族 `fetch_add/sub/and/or/xor`；亲眼看到非原子 `++` 在多线程下丢更新（lost update），再用 `fetch_add` 修复；区分运行期的 `is_lock_free()` 与编译期的 `is_always_lock_free`（C++17）；用 `std::atomic_flag`（C++11；`test_and_set`/`clear`，C++20 起加 `test()`）搭一个自旋锁（spinlock）雏形。

## 前置理解

- **原子（atomic）= 不可分割**：原子操作要么完整发生、要么没发生，中间状态不会被别的线程看到。普通 `++x` 是“读→加→写”三步，会被并发覆盖（数据竞争 data race，本身就是未定义行为 UB）。
- 默认内存序（memory order）是顺序一致（sequential consistency，`seq_cst`）——最强、最易推理。**为什么能用更弱的序换性能，留到模块 F**。
- `atomic<T>` 没有隐式转换：取值要 `.load()`，赋值要 `.store()`。RMW 操作（如 `fetch_add`）返回**旧值**。
- `is_always_lock_free` 是 `static constexpr`，可 `static_assert`；`is_lock_free()` 是运行期成员查询。
- `atomic_flag` 是标准保证“永远无锁”的最简旗标，用 `ATOMIC_FLAG_INIT` 初始化为清状态。

## 必做任务

1. `// TODO [必做 1]`：8 线程各自 `++` 同一计数器 10 万次。先用普通 `int` 看丢更新，再用 `atomic<int>::fetch_add(1)` 得到精确结果。
2. `// TODO [必做 2]`：演示 `load`/`store`/`exchange`（返回旧值）与位运算 RMW `fetch_or/and/xor`，并对比 `is_lock_free()` 与 `is_always_lock_free`。
3. `// TODO [必做 3]`：用 `atomic_flag::test_and_set`（返回“调用前是否已 set”）实现 `SpinLock::lock`，`clear` 实现 `unlock`，保护一个普通计数器。

## 进阶任务

- 思考自旋锁里加一次 `flag_.test()`（C++20，只读探测）能否减少缓存行写争用；体会自旋锁与 `std::mutex`（模块 B）的适用边界。

## 验收点

- 能说清“原子 = 不可分割”，并用数据证明非原子 `++` 丢更新、`fetch_add` 不丢。
- 能正确使用 `load/store/exchange` 与 RMW 家族，知道 RMW 返回旧值。
- 能区分 `is_lock_free()`（运行期）与 `is_always_lock_free`（编译期常量）。
- 能用 `atomic_flag` 写出正确的自旋锁雏形并解释 `test_and_set` 的返回语义。

## 对应官方参考

- cppreference [`std::atomic`](https://en.cppreference.com/w/cpp/atomic/atomic) / [`fetch_add`](https://en.cppreference.com/w/cpp/atomic/atomic/fetch_add) / [`is_always_lock_free`](https://en.cppreference.com/w/cpp/atomic/atomic/is_always_lock_free)
- cppreference [`std::atomic_flag`](https://en.cppreference.com/w/cpp/atomic/atomic_flag)
- 《C++ Concurrency in Action, 2nd ed.》(Williams) 第 5 章 5.2

## 构建运行

```bash
cmake --build build-vs2026 --target E1_atomic_basics --config Release
./build-vs2026/E1_atomic_basics/Release/E1_atomic_basics.exe
```
