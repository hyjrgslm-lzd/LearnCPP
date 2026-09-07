# F2：relaxed 的数值保证与发布边界

完整正文见[relaxed 的数值保证与发布边界](../../topics/atomics/05-relaxed-and-sc.md)。[main.cpp](main.cpp) 是安全、有限结束的 Starter；[solution.cpp](solution.cpp) 是含运行检查的 Reference。默认 C++23。

## Part 1：精确计数与完成同步

将 Starter 扩展为四个 worker，每人做 2000 次 relaxed fetch_add，同时维护独占的 local[t]。全部 get 后，检查共享计数为 8000，每个局部计数为 2000。完整实现为 `count_after_join()`。

答案：每个 RMW 基于同一计数器 MO 的紧邻前驱，不丢更新；最终完成同步使读取发生在工作结束之后。local 元素彼此独立且并发阶段不被主线程读取，无需 atomic。计数达到某值不能自动发布不相关的普通 payload。

## Part 2：有限 message-passing litmus

`message_passing(false)` 使用 atomic data 与 atomic flag。每轮清零后，发布者先写 data=1 再写 flag=1；接收者只读一次 flag 和 data，统计 (1,0)。再将 flag 改为 release/acquire，保持其他工作相同。

答案：relaxed 版本允许 (1,0)，但不要求出现。RA 版本若读到 flag=1，就有 data 写 HB data 读，排除旧值；Reference 检查 RA 统计为零。普通 payload 的缺序版本是 UB，与这里的原子旧值实验不同。

barrier 只界定轮次，不在两次被测访问中间。循环次数固定为 4000，既不等到某个结果出现才退出，也不使用 logger 给实验窗口添加同步。

## 结果记录

记录编译器、标准、轮数和两项统计；0/0 是可以接受的输出。不要由某台机器没有观察到旧值推出 relaxed 可以发布普通数据，也不要由 elapsed time 推出某内存序必然更快，本题不是性能排名。

## 构建与验收

从 `Concurrency_Study/exercises` 执行：

```powershell
cmake -S F2_relaxed_counter -B build/F2_relaxed_counter -G "Visual Studio 18 2026" -A x64
cmake --build build/F2_relaxed_counter --config Release
./build/F2_relaxed_counter/Release/F2_relaxed_counter.exe
ctest --test-dir build/F2_relaxed_counter -C Release --output-on-failure
```

VS2026 生成器需要 CMake 4.2 或更新版。CTest 运行 `F2_relaxed_counter_reference` 并设进程超时；cs::check 在 Release 仍有效。通过表示本次检查成功，不替代正文中的协议证明。规范链接与版本说明见对应正文。
