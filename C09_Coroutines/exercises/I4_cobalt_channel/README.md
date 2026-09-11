# I4 Boost.Cobalt channel

知识讲解：[I4 对应章节](../../11-模块I-真实异步IO与并发框架.md#i4)。

对应主讲义：`11-模块I-真实异步IO与并发框架.md` 的 I-4。

这题用 Boost.Cobalt 观察协程间通信。`channel<int>{0u}` 是零缓冲通道：没有 reader 时 writer 挂起，没有 writer 时 reader 挂起；双方匹配时能直接交接控制。这里能看到背压和 symmetric transfer 的实际形状。

starter 入口在 `main.cpp`。它使用 `channel<int>{0u}` 和 `channel_trace` 检查真实写读：producer 应写入变化值，consumer 应读满固定次数，最终 trace 要求 `writes=3`、`reads=3`、`sum=63`。只打印成功或直接返回 0 会因为 trace 不匹配失败。本机缺 Boost.Cobalt heavy 环境时保留源码阅读和 Linux heavy 预设验证，不伪造本机通过。

reference 链路：

```text
cobalt::main co_main
  -> channel<int>{0}
  -> consumer(ch)
  -> gather(producer(ch), delay(1ms))
  -> consumer 读出 1,2,3
  -> race(delay(30ms), delay(1ms))
```

预测：consumer 收到 `{1,2,3}`；`race` 的 1ms 分支获胜，winner index 为 1。Cobalt 文档说明 void awaitable 的 `race` 返回 index；loser cancellation 规则按所用 Boost 版本文档确认。

**答案解析：** `channel<int>{0u}` 没有缓冲槽，producer 的每次 `write` 都要等 consumer 的 `read` 匹配，所以值按 1、2、3 交替推进到 consumer。`race(delay(30ms), delay(1ms))` 中第二个 awaitable 更早完成，reference 检查 winner index 为 1；loser 如何取消或收束以当前 Boost.Cobalt 版本实现为准。

本题在 Linux `heavy-cobalt-linux` 预设中启用，需要 Boost 1.92 Cobalt。Windows 读者沿 `solution.cpp` 阅读 channel/gather/race 的控制流。

```bash
cd C09_Coroutines/exercises
cmake --preset heavy-cobalt-linux
cmake --build --preset heavy-cobalt-linux --target I4_cobalt_channel_reference
ctest --test-dir build/heavy-cobalt-linux -R I4_cobalt_channel_reference --output-on-failure
```

回读代码时看 `producer` 的三次 `write` 与 `consumer` 的三次 `read` 如何交替推进，再看 `gather` 等两个 awaitable 完成、`race` 只返回最先完成者。

**答案解析：** 背压发生在 `ch.write` 的 await 点：没有 reader 时 producer 挂起并释放 executor。`gather(producer, delay)` 等两条 awaitable 都完成后才继续，随后 `co_await c` 读取 consumer 的结果；`race` 则在第一个 awaitable 完成时返回 winner index。
