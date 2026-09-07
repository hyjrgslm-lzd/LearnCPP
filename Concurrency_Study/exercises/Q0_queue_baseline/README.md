# Q0：从一把锁开始理解并发队列

完整讲解见[队列基线](../../topics/queues/01-mutex-baseline.md)。

`main.cpp` 是预测、观察和重写接口的 Starter；`solution.cpp` 检查容量、FIFO、失败时输出保持不变，以及并发生产者的逐项完整性。

从 `Concurrency_Study/exercises` 执行：

```powershell
cmake -S Q0_queue_baseline -B build/q0 -G "Visual Studio 18 2026" -A x64
cmake --build build/q0 --config Release
ctest --test-dir build/q0 -C Release --output-on-failure
```

VS2026 生成器需要 CMake 4.2 或更新版本；其他生成器可使用 CMake 3.28 及以上。CTest 的进程超时覆盖运行和线程收尾。

## Part 与答案解析

1. 容量为 2，依次插入 10、20、30 时，第三次返回 false。随后的两次成功 pop 得到 10、20。失败操作没有插入 30，不能把“尝试提交”计入实际传输量。
2. 容量改为 1 后，插入 10 成功，20、30 都失败。消费者只读出 10。要保留后两个值，调用方必须在空间可用后重试，或改用带等待契约的接口。
3. 复制这个接口时，把检查和修改放在同一个临界区。两个分别安全的 `empty()` 与 `pop()` 不能组成原子的检查后取出操作。Reference 的顺序小测试验证具体返回值；并发测试验证每个生产者的序号保持顺序以及所有 ID 恰好出现一次。

这个测试还不覆盖多消费者的完整线性化历史；后续队列实验会增加专门的历史与暂停场景检查。
