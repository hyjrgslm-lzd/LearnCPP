# H3：等待值改变与保存事件历史

完整正文见[等待值改变与保存事件历史](../../topics/atomics/08-wait-and-generation.md)。[main.cpp](main.cpp) 是安全、有限结束的 Starter；[solution.cpp](solution.cpp) 是含运行检查的 Reference。默认 C++23。

## Part 1：一次性事件，分别用 wait 与轮询

Reference 的 `one_shot(true/false)` 实现相同的普通 payload 发布，检查 42。主线程先写 payload，再 release 改 ready，随后 notify；读者 acquire wait 或 load。

答案：发布来自 release/acquire，notify 不独立发布数据。ready 单调 false→true，业务条件正好是“不同于 false”，单次 wait 足够。对复杂谓词才需要外层循环继续判断；atomic wait 内部伪唤醒不意味着对外伪返回。

## Part 2：可控 ABA 与 generation

`generation_history()` 先让读者保存 old，再让生产者完成 false→true→false 和 generation 0→1→2，最后才允许观察。检查 bool 已与旧值相同，而 generation 增量是 2。

答案：多套 while 不能恢复被 bool 丢失的历史，wait(false) 甚至可能等不到未来变化；Reference 不运行这个可能永久阻塞的调用。inspect 门只证明受控时序，不能用该场景声称 generation 是 payload 的唯一发布边。

## Part 3：允许通知合并，保留事件总数

`generation_stream()` 发布 1000 次递增；读者累计 current-seen，最终检查 1000。它只处理计数，没有普通共享 payload，因此 relaxed 足够。

答案：一次观察可跨过多个 generation，唤醒次数不等于事件次数。有限计数器仍有回绕上限，需要规定最大落后范围；若每次事件携带不同内容，应另用队列或确认交接存储内容。

不要求 wait 实际进入内核、不宣称零 CPU、不要求忙等更慢。没有 sleep 证明时序；线程异常由 future 回传，所有等待结束后才销毁状态。

## 构建与验收

从 `Concurrency_Study/exercises` 执行：

```powershell
cmake -S H3_atomic_wait_notify -B build/H3_atomic_wait_notify -G "Visual Studio 18 2026" -A x64
cmake --build build/H3_atomic_wait_notify --config Release
./build/H3_atomic_wait_notify/Release/H3_atomic_wait_notify.exe
ctest --test-dir build/H3_atomic_wait_notify -C Release --output-on-failure
```

VS2026 生成器需要 CMake 4.2 或更新版。CTest 运行 `H3_atomic_wait_notify_reference` 并设进程超时；cs::check 在 Release 仍有效。通过表示本次检查成功，不替代正文中的协议证明。规范链接与版本说明见对应正文。
