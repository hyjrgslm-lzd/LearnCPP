# F1：直接发布与 release sequence

完整正文见[直接发布与 release sequence](../../topics/atomics/04-happens-before.md)。[main.cpp](main.cpp) 是安全、有限结束的 Starter；[solution.cpp](solution.cpp) 是含运行检查的 Reference。默认 C++23。

## Part 1：逐边证明普通 payload 的发布

从 Starter 的一个 int 扩展为 Reference 的 payload。发布者先赋值，再 ready.store(true,release)；主线程 acquire wait 后，在 producer.get 之前复制数据。检查 id=7、数组={11,22,33}。

答案：普通写 SB release store；wait 内部终止的 acquire load 读自唯一的 true 写，因此建立 SW；该 load SB 普通复制。传递得写 HB 读，且发布后没有其他写者。先 get 再读会引入另一条完成同步，可能掩盖 ready 协议的缺失。

## Part 2：加入只做 relaxed CAS 的中继

运行 `release_sequence()`：发布者写 payload 后 release store phase=1，中继只允许 CAS 1→2，接收者 acquire 读到 2 后检查 id=9、数组={4,5,6}。

答案：成功 CAS 是紧接 release 头的 RMW，接收者读它仍与 release 头同步。中继每次失败须重置 expected=1，否则失败回填 0 后可能错误完成 0→2。中继自己的普通写不会仅凭 relaxed CAS 自动发布。

## 反事实与答案

把中继改成普通 relaxed store，并不能继续沿用 C++20 后的 release sequence。即使由原发布线程执行普通 store，也会截断旧序列；另一个 release store 可以独立成为新的发布来源。

将接收者的 acquire 改成 relaxed 后，普通 payload 读取会失去本题依赖的同步，可能构成 UB；默认不运行这个错误变体。若要研究有定义的旧值结果，请运行 F2 的 atomic-only litmus。

Reference 用不抛异常的固定字段赋值，保证发布路径不会因分配失败漏发终态；等待不依赖 sleep。程序退出前取得所有 future，后台异常不被吞掉。

## 构建与验收

从 `Concurrency_Study/exercises` 执行：

```powershell
cmake -S F1_release_acquire -B build/F1_release_acquire -G "Visual Studio 18 2026" -A x64
cmake --build build/F1_release_acquire --config Release
./build/F1_release_acquire/Release/F1_release_acquire.exe
ctest --test-dir build/F1_release_acquire -C Release --output-on-failure
```

VS2026 生成器需要 CMake 4.2 或更新版。CTest 运行 `F1_release_acquire_reference` 并设进程超时；cs::check 在 Release 仍有效。通过表示本次检查成功，不替代正文中的协议证明。规范链接与版本说明见对应正文。
