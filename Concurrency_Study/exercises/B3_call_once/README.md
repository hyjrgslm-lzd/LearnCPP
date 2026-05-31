# 练习 B-3：call_once 一次性初始化

> 详尽版见 `../../03-模块B-互斥与锁.md` 的 练习 B-3。

## 目标

用 `std::call_once` + `std::once_flag` 实现线程安全的惰性初始化（lazy initialization），多线程并发触发验证「恰好只执行一次」，并与「函数内 static 局部变量（magic statics，C++11 起线程安全）」对比取舍。

## 必做任务

1. 用 `std::call_once(flag, fn)` 写惰性初始化 `get_resource()`。
2. 起 N 个线程并发触发，用 `std::atomic<int> init_count` 验证初始化体只执行一次（== 1）。
3. 用「函数内 `static` 局部变量」再实现一遍，验证构造函数也只调用一次；对比代码量与适用性。

## 进阶任务

1. 初始化第一次故意抛异常，观察 `once_flag` 不翻转、下次会重试（`call_once` 的重要保证）。
2. 写错误的 double-checked locking 反例，解释为何无原子/内存序时它是坏的（数据竞争）。
3. `std::call_once(flag, fn, arg1, arg2)` 带参初始化，对比 magic statics 的灵活性差异。

## 取舍

magic statics 写法最短、覆盖大多数单例场景；`call_once` 更通用（初始化目标非函数局部静态、需带运行期参数、需在非函数作用域控制时机时）。两者都替你正确处理了「只一次 + 其余线程等待」，所以**不要手写 double-checked locking**。

## 验收点（can-do）

- 你能展示「N 线程并发触发，初始化恰好一次」（计数器 == 1）。
- 你能说清 `call_once` 与 magic statics 各自适用场景并各举一例。
- 你能解释 `call_once` 初始化抛异常时为何不翻转 `once_flag`。

## 构建运行

```bash
cmake --build build-vs2026 --target B3_call_once --config Release
```

## 对应官方参考

- cppreference：`std::call_once` / `std::once_flag`
- cppreference：局部静态变量的线程安全初始化（storage duration）
- 《C++ Concurrency in Action, 2nd ed.》(Williams) 第 3.3.1
