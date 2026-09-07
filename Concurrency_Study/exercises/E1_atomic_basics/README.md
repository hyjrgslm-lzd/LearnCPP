# E1：原子性、RMW 与 atomic_flag

完整正文见[原子性、RMW 与 atomic_flag](../../topics/atomics/01-atomic-operations.md)。[main.cpp](main.cpp) 是安全、有限结束的 Starter；[solution.cpp](solution.cpp) 是含运行检查的 Reference。默认 C++23。

## Part 1：用有定义的交错解释丢更新

先预测 Starter 的两次 load、两次 store 为什么只得到 1。将相同交错放到两个线程，用 promise 控制“都先读旧值，再写回”，不能用 sleep。随后四个线程各做 2000 次 fetch_add，检查 8000。

答案在 Reference 的 `split_increment()`。即使两个 load/store 都是 SC，它们之间仍可插入其他操作；RMW 才把依赖旧值的更新合并。普通 int 无同步 ++ 是 UB，默认练习不执行它，也不对它的结果作数值承诺。

## Part 2：检查接口返回值

运行 `operations()` 前写下每一步的旧值与新值。10 先 store 为 42，exchange(7) 返回 42；fetch_add(3) 返回 7，fetch_sub(2) 返回 10，最后为 8。位运算 1→5→4→6，分别返回 1、5、4。前置 ++ 返回本次新值，后置 ++ 返回旧值。

`int implicit = value;` 是合法取值转换，等价于 load；不能复制 atomic 对象不等于不能取出 T。打印两种无锁查询即可，不要求 true。

## Part 3：保护一个普通计数器

实现 `spin_mutex` 并让 lock_guard 管理释放。Reference 的 `flag_lock()` 检查默认 clear、两次 test_and_set 返回 false/true、clear 后恢复 false，以及四线程各 2000 次普通自增。

答案：成功 acquire test_and_set 取得先前 release clear 写出的状态，建立跨临界区同步。只调用 test 看见空闲不算取锁。整个自旋锁依赖持有者释放，不具有无锁算法的进展保证；它是教学实现，不要求优于 mutex。

每个 Part 都在独立函数中实际检查，worker 异常由 future::get 回传，Release 不依赖 assert。

## 构建与验收

从 `Concurrency_Study/exercises` 执行：

```powershell
cmake -S E1_atomic_basics -B build/E1_atomic_basics -G "Visual Studio 18 2026" -A x64
cmake --build build/E1_atomic_basics --config Release
./build/E1_atomic_basics/Release/E1_atomic_basics.exe
ctest --test-dir build/E1_atomic_basics -C Release --output-on-failure
```

VS2026 生成器需要 CMake 4.2 或更新版。CTest 运行 `E1_atomic_basics_reference` 并设进程超时；cs::check 在 Release 仍有效。通过表示本次检查成功，不替代正文中的协议证明。规范链接与版本说明见对应正文。
