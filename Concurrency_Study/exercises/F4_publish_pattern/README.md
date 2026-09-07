# F4：可复用单槽需要双向交接

完整正文见[可复用单槽需要双向交接](../../topics/atomics/07-publication-and-lifetime.md)。[main.cpp](main.cpp) 是安全、有限结束的 Starter；[solution.cpp](solution.cpp) 是含运行检查的 Reference。默认 C++23。

## Part 1：指出原版本号覆盖方案缺少的边

Starter 只安全地交接一次。若生产者继续覆盖 slot，读者即使 acquire 看见上一版，下一次写也可能与当前读竞争。不要把 Starter 直接包进生产者循环。

答案：现有链只给 Wv HB Rv，缺少 Rv HB W(v+1)。提高版本号序强度或读前后重查版本，都不能撤销已经发生的普通读写数据竞争。旧方案不作为可运行错误演示。

## Part 2：实现每版确认并完成 2000 次交接

Reference 使用 published 与 acknowledged 两个原子版本号。生产者 acquire 观察 ack==v-1 才写第 v 版，release 发布；消费者 acquire 接收，复制检查，release 确认。检查版本 1..2000 每版恰好一次，字段为 {v,2v,-v}。

答案：发布方向保证初始化可见；确认方向保证旧读先于槽位覆盖。两条链共同形成安全复用。容量 1、SPSC、固定次数，不承诺任意数量消费者、非阻塞调用或无锁进展。

## Part 3：异常与收尾

消费者累积检查结果，发送完全部确认后再 cs::check；主线程 consumer.get 接收失败。为何不在发现字段错误时立即抛出？因为生产者可能仍等待下一次确认，提前退出会留下等待者。

本题字段复制不抛异常。推广到 string、回调或外部资源时，需设计关闭/错误终态及唤醒，不能只复制这段循环。所有使用者结束后才销毁原子和 slot。

## 场景切换题答案

若读者允许跳版、只需某个自洽的配置快照，运行 I1 的不可变 atomic<shared_ptr<const T>>。它保留旧对象寿命，减少对“读完才可改同一个槽”的依赖，但引入分配和共享所有权成本，也不提供逐版送达承诺。不能作为同契约的无条件替换。

## 构建与验收

从 `Concurrency_Study/exercises` 执行：

```powershell
cmake -S F4_publish_pattern -B build/F4_publish_pattern -G "Visual Studio 18 2026" -A x64
cmake --build build/F4_publish_pattern --config Release
./build/F4_publish_pattern/Release/F4_publish_pattern.exe
ctest --test-dir build/F4_publish_pattern -C Release --output-on-failure
```

VS2026 生成器需要 CMake 4.2 或更新版。CTest 运行 `F4_publish_pattern_reference` 并设进程超时；cs::check 在 Release 仍有效。通过表示本次检查成功，不替代正文中的协议证明。规范链接与版本说明见对应正文。
