# C16 桌面应用与多媒体

桌面应用不只是调用控件，媒体播放也不只是调用 `play()`。本课以本地音视频审阅工具 **MediaWorkbench** 为主案例，逐步解释对象归属、事件分发、线程协作、视图更新、媒体时间和应用退出。读者应能从一个迟到的分析结果追到请求身份，从一次卡顿追到阻塞阶段，从一帧画面追到它的缓冲与时间戳。

主线使用 C++23、Qt 6.9.2 和 Qt Widgets；Qt Quick 是复用同一 C++ 模型的桥接单元。先读正文，再用练习检验推导。Reference 是解析依据，不能成为 Student 的实现依赖。

## 从哪里进入

- 工程基础：[C01](../C01_Build_Compile_Link/README.md) 的最小构建、链接和调试。
- 对象与接口：[C02](../C02_Objects_Lifetime_Ownership/README.md) 的 RAII、借用和生命周期，[C03](../C03_Type_Modeling_Interface_Design/README.md) 的不变量与错误通道。
- 数据与序列：[C05](../C05_Data_Representation_Standard_Facilities/README.md) 的字节、Unicode、时间与基础格式；[C06](../C06_Ranges/README.md) 的容器与失效。
- 到 L05 再补 [C08](../C08_Concurrency/README.md) 的线程结束、互斥和发布。学习本课不要求先完成协程或 Execution 全课。
- 系统 I/O 和性能归因分别由 [C07](../C07_OS_Memory_System_IO/README.md)、[C13](../C13_Performance_Numerics_Data_Layout/README.md) 主讲；本课展开 Qt 与媒体应用增加的责任。

环境、版本和运行入口见 [构建说明](exercises/BUILD_GUIDE.md)。第一次阅读从 [应用与事件循环](chapters/01-application-event-loop.md) 开始。

## 学习路线

| 阶段 | 连续正文 | 要回答的问题 |
|---|---|---|
| 应用与对象 | [01 应用](chapters/01-application-event-loop.md)、[02 所有权](chapters/02-qobject-ownership.md)、[03 元对象与连接](chapters/03-metaobject-connections.md)、[04 分发与重入](chapters/04-dispatch-reentrancy.md) | 谁拥有对象？槽实际在哪个线程执行？为什么一次调用尚未返回，状态却被再次修改？ |
| 桌面机制 | [05 worker 与关闭](chapters/05-worker-cancellation-lifetime.md)、[06 模型/视图](chapters/06-model-view.md)、[07 应用与会话](chapters/07-application-session.md)、[08 可访问性](chapters/08-accessibility.md) | 换源后如何拒绝旧结果？视图何时知道模型变了？保存失败和关窗如何收尾？ |
| 媒体表示与处理 | [09 PCM 与时间](chapters/09-pcm-time.md)、[10 有界管线](chapters/10-bounded-pipeline.md)、[11 音频处理](chapters/11-audio-processing.md)、[12 视频帧](chapters/12-video-frames.md) | sample 与 frame 有何区别？积压到哪里停止？stride 为什么不等于宽度乘像素大小？ |
| 同步与框架 | [13 媒体时钟](chapters/13-media-clock.md)、[14 Qt 播放](chapters/14-qt-playback.md)、[15 响应性](chapters/15-responsiveness.md) | PTS、实际到达时间和物理播放时间分别是什么？优化前怎样定位卡顿？ |
| 综合与桥接 | [16 MediaWorkbench](chapters/16-media-workbench.md)、[17 Quick 桥接](chapters/17-quick-bridge.md) | 如何把这些责任接成可交互、可取消、可保存并可退出的应用？ |

每个实现练习有独立 `student/solution.hpp`、`reference/solution.hpp`、`good/solution.hpp`、`bad/solution.hpp` 和共同检查器。观察实验与项目的入口、Part 和解析在对应正文说明；观察程序成功不等于预测、解释或作业已完成。

## 机制与证据的边界

本课既使用真实 Qt 对象、事件、模型、解码器和窗口，也通过独立小实验解释有界缓冲和同步决策。教学时钟的通过记录不能代替 Qt 内部调度的验证；解码音频回调也不等于扬声器发声时刻。设备、平台、解码和无窗口逻辑检查分别报告。

Qt 元对象由 `moc` 及运行时协作实现。它与 C++ 语言静态反射有不同的产生时机、对象模型和使用协议，不能直接互换；语言反射的主讲位置是 [C04](../C04_Generic_CompileTime_Reflection/README.md)。

完整的能力映射见 [覆盖与反向检查](references/coverage.md)，固定版本源码入口见 [规范与实现索引](references/standards-and-implementations.md)。这两份是教材导航，不是某次机器的验收报告。
