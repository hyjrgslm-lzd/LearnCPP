# J1：伪共享、真共享与批量发布

本题 main.cpp 分类为 OBSERVATION：运行给定基线并进行本程序实际列出的观察/检查。退出成功只说明这些检查通过，不表示下面全部实现、推导或测量 Part 已完成；完整答案与更广检查见独立 solution.cpp。需要实现的 Part 请在自己的函数中完成后对照 Reference，不把运行答案视为完成作业。

完整连续正文：[课程正文](../../topics/performance/01-cache-layout.md)。

先读完整正文，再运行 main 的相邻计数器基线。四个版本的共用实现位于本题 [reference.hpp](reference.hpp)，[solution.cpp](solution.cpp) 是完整答案，性能驱动位于 [layout_bench.cpp](../benchmarks/layout_bench.cpp)。

## 必做 Part 与答案

1. 用 packed 与 padded 两种布局各完成两个 worker、每个 N 次事件。每个私有 atomic 最终必须分别等于 N，不能只检查两者之和。实现使用 relaxed 原子读改写，不会把循环改成一次普通最终赋值；relaxed 不消除缓存一致性代价。
2. 改用 shared，让两 worker 更新同一 atomic。最终为 2N，这是正常真共享对照。它不能通过 padding 消除逻辑热点；它也不保留逐线程明细，必须注明合同差异。
3. 每 worker 用局部变量累积 B 次才发布，并在退出前刷新余数。N=257、B=256 时每线程发布两次，总共四次，总数 514。Reference 覆盖 0、1、257、4097 和 B=0 拒绝路径。
4. 解释观测与线程收尾：主线程只在 join 后读取结果，worker 异常回传。任务包含线程创建时间，不依赖 sleep 证明两个线程重叠。

## 实验与解读

用统一 runner 选择 packed/padded/shared/batched，参数为 `--size 100000 --threads 2 --batch 256`。size 是每线程事件数，completed 是两线程合计；计时包含清零、创建、处理和 join，检查不计时。检查发布次数与总数，不要求某个倍数。B=1 时发布次数退回 2N，但代码结构不同，耗时不保证一致。

瞬时算法未发布量每 worker 最多 B：达到阈值之后、fetch_add 之前也可能被抢占。若要求实时逐事件可见，批量版不能作为同合同替换。普通自动存储期 local 已经由当前线程独占，不需要为短任务额外引入 thread_local 对象。

## 构建与运行

从 `C08_Concurrency/exercises` 执行：

```powershell
cmake -S J1_false_sharing -B build/J1_false_sharing -G "Visual Studio 18 2026" -A x64
cmake --build build/J1_false_sharing --config Release
ctest --test-dir build/J1_false_sharing -C Release --output-on-failure
```

C++ 默认 23，cs::check 在 Release 中保持有效。可选能力缺失不阻止普通基线；平台及标准事实的官方链接、完整推导见本题对应正文。
