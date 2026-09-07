# 原模块 F：内存模型与 memory_order（迁移入口）

完整正文已迁移到[第 09 章：内存模型](chapters/09-memory-model.md)与[第 10 章：发布与生命周期](chapters/10-publication-and-lifetime.md)。

- [逐边 HB 证明与 release sequence](topics/atomics/04-happens-before.md)：[F1](exercises/F1_release_acquire/README.md)。
- [合法内存序、relaxed、SC 及 consume 版本差异](topics/atomics/05-relaxed-and-sc.md)：[F2](exercises/F2_relaxed_counter/README.md)、[F3](exercises/F3_seqcst_fence/README.md)。
- [独立 fence 正文与三种同步桥](topics/atomics/06-fences.md)：F3 的独立 Reference 部分。
- [持续发布、槽位复用与不可变快照](topics/atomics/07-publication-and-lifetime.md)：[F4](exercises/F4_publish_pattern/README.md)、[I1](exercises/I1_atomic_shared_ptr/README.md)。
- [atomic wait、通知和 generation](topics/atomics/08-wait-and-generation.md)：[H3](exercises/H3_atomic_wait_notify/README.md)。

旧 F4 的“版本号发布后直接反复覆盖普通 payload”缺少读者完成确认，不能保护下一轮写。新 F4 明确改为 SPSC 确认交接；允许跳版的多读者配置改由 I1 不可变快照承担，两者契约不同。

C++20 后，release sequence 的后续部分只由 RMW 延续；同线程普通 store 不再延续。C++26 固定依据 N5050：consume 弃用并具有 acquire 的含义，但不与 acquire 枚举等值。所有 litmus 只检查禁止结果，允许结果不要求必然出现；默认路径不执行 UB。
