# L10 有界管线、背压、欠载与排空

解码和播放之间必须有缓冲，但缓冲不能无限长。无限队列会把内存、延迟和关闭时间都交给输入速度决定；桌面应用最后表现为 seek 后旧数据继续播放，关闭时还要等一长串无用工作。本章建立一个有界 PCM 管线模型：写入者只能填到容量上限，读取者可能遇到欠载，输入关闭后必须排空，取消时可以 flush 丢弃。

这里故意不用无锁队列。媒体课程需要先把容量、EOF、取消和部分写的契约讲清楚；无锁结构属于 C08/C13 的深入主题。一个 `mutex + deque` 足以表达本章问题，复杂度更低，状态更可审。

## 1. 四种状态不能混淆

空队列不等于 EOF。打开的空队列只是 underrun：消费者暂时没有数据，但生产者以后可能写入。EOF 必须同时满足“输入已经关闭”和“队列已经排空”。

部分写不等于失败。容量为 3 时写入 4 个 sample，接受前 3 个，返回 accepted=3，并记录背压。调用者可以稍后继续写剩余数据。若把部分写当作全量成功，尾部 sample 就丢了；若为了省事扩容，就失去有界性。

取消 flush 不等于正常 EOF。正常 EOF 保留已经接受的数据并排空；取消表示这些数据属于旧请求或关闭路径，应该清空队列并进入终止状态。

## 2. 最小协议

本章练习的 `BoundedPcmPipe` 用同步方法模拟 producer/consumer：

```cpp
write(samples) -> accepted_count
read(max_count) -> vector<int>
close_input()
cancel_flush()
eof()
```

它不是实时音频设备；它是可运行的协议模型。后续接真实 Qt/FFmpeg 时，音频 renderer 也会遇到类似问题：设备一次能接受的字节数可能小于一帧或一个 buffer，剩余部分必须保留；流尾到达不代表设备队列立刻排空。

练习分两层。第一层是非阻塞状态 API：`write/read/close_input/cancel_flush/eof`，用于精确验证容量、部分写、underrun 和 drain。第二层是有限等待 API：`write_wait/read_wait`，内部用同一把 mutex 和 `condition_variable` 等待“有空间/有数据/终止”。检查器启动真实 producer 线程，容量为 2，写入 4 个 sample；主线程观察 producer 停在容量边界，再由 consumer 读出两个 sample 释放空间，producer 才能完成。这证明本题不是普通 deque 题，也不把无界等待留给读者猜。

## 3. 背压

背压的意义是把“下游慢”反馈给上游。这里最小反馈是 `write()` 的返回值小于输入长度，并增加 `backpressure_count()`。真实应用可以选择暂停解码、降低预取、丢弃旧视频帧、或显示缓冲状态。无论选择哪种策略，容量边界必须可见，不能让队列静默增长。

## 4. 欠载

`read()` 在打开且为空的队列上返回空，并记录 underrun。它不阻塞，因为练习要有限结束；真实音频线程通常不能无限等 UI 或磁盘。阻塞模型也可以成立，但必须有外部超时/取消解除阻塞。本课所有自动检查都有进程外超时，内部停止标志不能代替外部监督。

## 5. 练习

位置：`C16_Desktop_Multimedia/exercises/L10_bounded_pipeline`。

Part：

1. 容量为 3 时写入 4 个 sample，只接受 3 个并记录背压。
2. FIFO 读取后释放容量，再写入新数据。
3. 打开空读记录 underrun，但不报告 EOF。
4. `close_input()` 后先排空已接受数据，再报告 EOF。
5. `cancel_flush()` 清空队列，拒绝后续写入，进入终止状态。

`bad` 变体是典型错误：无视容量，把所有输入塞进队列；`close_input()` 后立刻 EOF；取消不清空。检查器会用同一组输入拒绝它。

## 6. 解析

正确状态可以压成四个字段：`capacity_`、`queue_`、`closed_`、`cancelled_`。所有公开方法用同一个 mutex 保护，避免读到半更新状态。这个锁不是性能结论，只是教学模型；如果将来要优化，需要先有可复现的阶段测量证明锁是瓶颈。

EOF 判断写成：

```cpp
(closed_ || cancelled_) && queue_.empty()
```

这条表达式体现两个边界：正常关闭要 drain，取消要 flush。它也是检查器最重要的反向断言。

源码阅读可对照 [references/standards-and-implementations.md](../references/standards-and-implementations.md) 的 FFmpeg audio renderer 路径。阅读问题：`pushFrameToOutput` 遇到部分接受时如何保存剩余？`EndOfMedia`、buffer output、设备排空分别是哪些事件？
