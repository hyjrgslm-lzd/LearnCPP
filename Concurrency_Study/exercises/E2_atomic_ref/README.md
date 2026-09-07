# E2：对齐与原子访问阶段

完整正文见[对齐与原子访问阶段](../../topics/atomics/03-atomic-ref.md)。[main.cpp](main.cpp) 是安全、有限结束的 Starter；[solution.cpp](solution.cpp) 是含运行检查的 Reference。默认 C++23。

## Part 1：并发阶段只经 atomic_ref 访问

将 Starter 的单个 cell 扩展为四个 cell 的数组。四个 worker 各引用 data[2].value 并做 2000 次 relaxed fetch_add；全部结束后检查目标为 8000、其余元素为 0。Reference 在 main 的第一段完整实现。

答案：每个 cell 都按 required_alignment 对齐，value 是起始字段；多个 worker 的 ref 指向同一普通对象，RMW 保留每次更新。把 alignas 只加在 int 数组首地址上不普遍保证各元素符合更严格的对齐。

## Part 2：证明普通访问恢复点

Reference 检查所有元素地址，打印 required_alignment、alignof(int) 与运行期无锁属性。另开作用域复制 ref，通过 const 包装器 store(123)，通过另一个 ref.load 读取，作用域结束后再普通读字段。

答案：ref 不拥有目标，也不延长目标寿命。有任一 ref 存活时，目标所有访问都应经这些 ref；不是“只要没有同时写就可以随便普通读”。worker 局部 ref 在返回前销毁，全部 get 后才可恢复本题的普通访问。

## 复盘答案

vector 扩容可能移动目标使 ref 悬垂，erase/clear 也可能结束寿命；reserve 不能包办全部保护。同一个结构体的整体 ref 与它的成员 ref 不能同时重叠引用。若字段从始至终都是共享原子状态且能控制声明，直接 atomic 通常更容易维护。

这两个 Part 的数值、地址与访问阶段都在 Reference 中有检查；不尝试构造未对齐 ref 来“看会不会崩溃”。

## 构建与验收

从 `Concurrency_Study/exercises` 执行：

```powershell
cmake -S E2_atomic_ref -B build/E2_atomic_ref -G "Visual Studio 18 2026" -A x64
cmake --build build/E2_atomic_ref --config Release
./build/E2_atomic_ref/Release/E2_atomic_ref.exe
ctest --test-dir build/E2_atomic_ref -C Release --output-on-failure
```

VS2026 生成器需要 CMake 4.2 或更新版。CTest 运行 `E2_atomic_ref_reference` 并设进程超时；cs::check 在 Release 仍有效。通过表示本次检查成功，不替代正文中的协议证明。规范链接与版本说明见对应正文。
