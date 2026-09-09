# 09 内存模型：用关系证明一次读取为什么合法

原子操作解决了“一个对象上的这次动作不可分割”，却还没有回答“读到就绪标志后，为什么可以读取普通数据”。内存模型为这个问题提供的是一组可组合的关系。它不是某种 CPU 的汇编操作手册，也不是把全部语句排成一条真实时间线。

从[逐边证明发布与 release sequence](../topics/atomics/04-happens-before.md)开始。它以 [F1](../exercises/F1_release_acquire/README.md) 的真实程序为主线，逐一确定线程内顺序、原子读取来源、跨线程同步及最终普通读写的 happens-before。先验证直接的 release/acquire，再加入一个 relaxed RMW 中继，观察为什么中继不必读取 payload 也能延续原发布者的同步关系。

然后读[relaxed、修改顺序和 SC 的范围](../topics/atomics/05-relaxed-and-sc.md)，完成 [F2](../exercises/F2_relaxed_counter/README.md) 与 [F3](../exercises/F3_seqcst_fence/README.md) 的前两部分。纯计数器不需要通过计数值发布另一个对象，relaxed RMW 加最终完成同步已经足够。相反，两个线程各自先写自己的原子、再读对方原子的 store-buffering 实验中，release/acquire 允许两边都读到零，因为在这个结果里没有任何读取得到了对方的 release。

最后单独学习[fence](../topics/atomics/06-fences.md)。fence 是独立的同步操作，但不是“随便放一道屏障，所有变量自动安全”。必须找到连接两个线程的原子对象，并证明读取确实取得指定写入的值。F3 Reference 提供三种发布桥接，以及双侧 SC fence 的 store-buffering 检查。

## 学习时始终分开的四件事

原子性约束某一次原子操作；modification order 排列同一个原子对象的修改；happens-before 为求值建立可传递的先行关系；SC 另为 seq_cst 操作及 fence 加上受约束的全序。这些概念有关联，但没有哪一个可以替代其余全部。

一个常见错误是先看测试结果，再反推标准承诺。relaxed litmus 连续几千次都没有出现旧值，仅说明本次执行没有观察到；不代表允许结果被标准禁止。相反，普通数据存在竞争的程序已经是 UB，不能给它规定“应出现旧值但不能崩溃”的验收条件。本章把故意无保证的实验数据也做成 atomic，保留有定义的观察范围。

另一个常见错误是用 logger 验证同步。logger 内部的锁可能给被测线程添加本来没有的同步关系。F2/F3 把 barrier 放在每轮实验窗口外，只在全部完成后打印；每轮 body 中只保留要研究的原子读写和所选 fence。

## 版本与完成标准

默认代码为 C++23，C++26 规范固定为 [N5050](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5050.pdf)。C++20 后 release sequence 的后续部分只有 RMW；N5050 的 consume 已弃用并按 acquire 的含义处理，枚举值仍有区别。不要把旧版的“同线程普通 store 也可延续 release sequence”或旧 consume 依赖链当作新代码前提。

完成本章后，应能为 F1 每一个普通 payload 读取指出唯一的发布来源和完整的三段关系；能说明 F2 最终总数检查为何仍需要完成同步；能为 F3 的 SC 禁止结果画出矛盾环，并解释少一侧 fence 时为什么不能沿用该证明。然后进入[发布与寿命](10-publication-and-lifetime.md)：同步已有内容，只是完整对象协议的一半。
