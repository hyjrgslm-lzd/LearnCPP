# F3：SC 禁止结果与三种 fence 桥

完整正文见[SC 禁止结果与三种 fence 桥](../../topics/atomics/06-fences.md)。[main.cpp](main.cpp) 是安全、有限结束的 Starter；[solution.cpp](solution.cpp) 是含运行检查的 Reference。默认 C++23。

先读[relaxed 与 SC](../../topics/atomics/05-relaxed-and-sc.md)，再读上方独立 fence 正文。Starter 是一次安全 SC store-buffering；Reference 为每个变体执行 4000 轮。

## Part 1：比较 relaxed、RA 与 SC

实现 `store_load` 和 `store_buffering`。两端分别先写自身原子为 1，再读对方。relaxed 及 release-store/acquire-load 的双零计数均允许为零或正数；SC 双零必须为零。

答案：RA 双零时读自初值，没有跨线程发布 SW。全部 SC 若双零，则要求 Sx<Ly<Sy<Lx<Sx，形成全序矛盾。不要从 RA 字样直接推导两次读必有一次观察对方。

## Part 2：两侧 SC fence

用 relaxed store，SC fence，relaxed load 的顺序替换两端 body。Reference 的 mode::sc_fence 验证双零为零。

答案：若双零，读与对方写的 coherence-ordered-before 关系，结合两侧 fence 的 HB 位置，会同时迫使 F0<F1 和 F1<F0。relaxed 访问本身并没有加入 SC 全序。删掉一侧 fence 后不能沿用这条证明。

## Part 3：release/acquire fence 发布

`fence_publication(0/1/2)` 分别验证 fence→atomic、atomic→fence、fence→fence 三种桥，全部读取普通 data=42。data 读取位于 producer.get 之前。

答案：指出 ready 是桥、最终读取来源是唯一的 true 写；分别建立 Frel SW acquire-load、release-store SW Facq、Frel SW Facq。fence 前后的放置方向决定哪些普通访问被覆盖；两个孤立 fence 不自动同步。

## 小模型与运行边界

[atomic_protocol_test.cpp](../runtime_tests/atomic_protocol_test.cpp) 枚举六种 SC 四步排列，检查结果恰为 01、10、11，同时验证 SC 拆分自增仍可丢更新。它不是完整 C++ 内存模型求解器。

从 exercises 可单独配置 runtime_tests，构建目标 `runtime_atomic_protocol_test`，CTest 用 `-R '^runtime_atomic_protocol_test$'` 限定运行。所有 litmus 断言在双方完成后执行，窗口中没有 logger 锁、sleep 或用于强迫特定输出的附加同步。

## 构建与验收

从 `Concurrency_Study/exercises` 执行：

```powershell
cmake -S F3_seqcst_fence -B build/F3_seqcst_fence -G "Visual Studio 18 2026" -A x64
cmake --build build/F3_seqcst_fence --config Release
./build/F3_seqcst_fence/Release/F3_seqcst_fence.exe
ctest --test-dir build/F3_seqcst_fence -C Release --output-on-failure
```

VS2026 生成器需要 CMake 4.2 或更新版。CTest 运行 `F3_seqcst_fence_reference` 并设进程超时；cs::check 在 Release 仍有效。通过表示本次检查成功，不替代正文中的协议证明。规范链接与版本说明见对应正文。
