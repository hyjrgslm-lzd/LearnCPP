# 内存模型 01：从读到一个标志，推到安全读取数据

本篇对应 [F1 Reference](../../exercises/F1_release_acquire/solution.cpp) 的 `direct_publication()` 与 `release_sequence()`。场景先限制为一次发布：发布者完成 payload 的全部初始化后不再改它；接收者取得发布许可后读取；所有线程结束后才销毁对象。这个限制使我们能先集中研究可见性，而不被下一轮覆盖和内存回收干扰。

## 1. 先找冲突，再找边

payload 是普通结构体，其中包含 int 和固定数组。发布者赋值与接收者复制访问相同内存位置，其中有写，因此需要检查是否有 happens-before 将冲突求值排序。不要先问“它们是不是在不同时间运行”：C++ 数据竞争的判断依赖语言关系，墙钟上的先后或人为延迟不是充分条件。

本篇使用三个关系。sequenced-before，简称 SB，是语言规定的同一线程求值先后。synchronizes-with，简称 SW，是满足同步操作规则时建立的关系，例如 release 操作被合适的 acquire 操作读取。happens-before，简称 HB，在本篇不使用旧 consume 的程序中，可沿 SB、SW 及其传递关系建立。

“两次操作都有 acquire/release 字样”还不够。必须在同一个原子对象上，acquire 的读取来源符合那次 release 或它所领导的 release sequence。标志值相等本身也不是读自关系的证明；两个写者都写 true 时，必须区分实际读到了哪一次写。

## 2. 给 F1 每个事件命名

`direct_publication()` 中令 W 为 `data = {7, {11,22,33}}`，S 为 `ready.store(true, release)`，L 为 wait 内部最终观察到 true 的 acquire load，R 为主线程 `observed = data`：

```text
发布线程： W ——SB——> S
                       │ SW：L 读取 S 写入的 true
接收线程：             L ——SB——> R
传递结果： W ——HB——> R
```

初始 ready 为 false，只有这一次写 true，接收者退出 wait 的读取来源因此明确。W HB R，且没有其他线程在发布后写 payload，所以 R 可读取该次完整赋值的内容。wait/notify 的等待机制不改变这条证明：SW 来自 release 写与 acquire 读，notify 本身不是 payload 的发布边。

Reference 特意在 producer.get() 之前复制 data。若先 get 再读，即使删掉 ready 的同步，future 的完成同步也可能使测试继续安全通过，掩盖本来要检查的协议。主线程保存 observed 后才 get，然后运行 cs::check。

若把 L 换成 relaxed 并保留普通 payload，现有 SW 消失，W 与 R 缺少排序，程序涉及数据竞争，不能说“只是有时读旧值”。默认练习不执行这个变体。想研究有定义的旧值观察，应到 F2，把 payload 也改成 atomic；这属于场景切换。

## 3. modification order 能做什么

所有对同一原子对象的修改都属于该对象的一条 modification order（MO）。load 不产生修改，但它从某个修改取得值。MO 不是所有原子对象的联合全序，也不等于 HB；在没有同步时，两个原子对象的观察可能不能拼成一条全局串行时间线。

原子的连贯性规则还约束 HB 与读值的关系。例如，同一线程先读到某个修改，再读取同一原子，第二次不能回退到 MO 中更早的修改；若写入数值本身会降低，数值下降并不违反这条规则。“不回退”指修改位置，不是数值大小。RMW 更进一步，读取自身修改在 MO 中的紧邻前驱。

这些约束解释了 relaxed 自增可以精确累计，却没有给任意别的普通变量自动加上 SW。分清“同一对象的连贯性”和“用它发布另一对象”，才能知道什么时候需要更强的序。

## 4. release sequence 是怎样接力的

F1 第二个实验有三种角色：发布者写 payload 并 release store phase=1；中继只做 relaxed CAS，将 1 改为 2；主线程 acquire 观察 phase=2 后读取 payload。

在 phase 的 MO 中，这次 store 是序列头 A，其后紧接成功 CAS B。成功 CAS 是 RMW，因此 B 位于 A 的 release sequence 内。接收者的 acquire 读 C 从 B 取得 2；依据同步规则，A SW C。所以：

```text
写 payload ——SB——> A: store(1, release)
                    │ phase 的 MO：A -> B: CAS(1,2,relaxed)
                    └————SW————> C: load(acquire)==2 ——SB——> 读 payload
```

注意并不是“B 的 relaxed 自动变成 acquire/release”。A 是 SW 的起点，中继只是在同一对象的修改顺序中延续该 release sequence。若中继在 B 前写了另一个普通对象，这个 relaxed B 不会自动发布中继自己的写；想把它也传过去，需独立证明同步，通常将 B 选为 release 或 acq_rel。

Reference 的中继每次失败都把 expected 重置为 1。因为契约只允许 1→2，不能读到初始 0 后直接执行 0→2。成功 CAS 返回后才 notify；即使主线程此前因 phase==0 阻塞，也会在通知后复查并观察终态。

## 5. C++20 的边界必须记清

C++20 起，release sequence 由 release 头和其后在 MO 中连续的 RMW 组成。普通 store 会截断先前的序列，即使它由同一个发布线程执行，也即使写的数值恰好相同。如下修改不能沿用刚才的证明：

```text
写 payload -> store(1, release) -> store(2, relaxed)
                                        ↑ acquire 只读到这次写，不能靠旧序列发布
```

如果第二次 store 自己也是 release，它可以成为新的同步来源，发布同线程在它之前的 payload 写；这是新的 release 操作自己的作用，不是普通 store 延续了原来的序列。另一个线程要发布不属于自己先前动作的数据，还需要先把 HB 接到本线程。

失败 CAS 只是 load，不会产生 MO 中的新修改，所以它本身既不是延续序列的 RMW，也不会作为一个 store 去截断序列。数值未变的成功 CAS 则仍是 RMW，必须与失败区别。

## 6. 运行和结果

在 `C08_Concurrency/exercises` 中：

```powershell
cmake -S F1_release_acquire -B build/f1 -G "Visual Studio 18 2026" -A x64
cmake --build build/f1 --config Release
ctest --test-dir build/f1 -C Release --output-on-failure
```

直接发布应得到 id=7 与 {11,22,33}；接力应得到 id=9 与 {4,5,6}。等待不依赖 sleep，日志发生在实验完成后。payload 用固定大小且不抛异常的赋值，使发布路径不可能因业务分配失败而漏发终态；后台任务的异常仍由 get 回传。完整应用若把构造换成可能失败的操作，还应加入失败状态和关闭通知。

有限实验只能支持具体检查，不能替代上述 MO 与 HB 推导。等待也没有固定完成时限的标准保证；课程 CTest 用进程超时检测卡住，而不是靠 sleep 假装证明线程已经前进。

## 自测与答案

**acquire 没有读到指定 release 的值，还能声称 SW 吗？** 不能仅凭这对操作声称；可能存在其他同步来源，但必须单独指出。初值 false 不发布尚未发生的 payload 初始化。

**发布者连续两次 store true，第二次是 relaxed，为什么布尔相等不够？** 接收者可能读自第二次写，而它截断了原序列；仅凭读值 true 无法区分来源。使用单次写或有明确版本来源的协议。

**改成 phase.fetch_add(1, relaxed) 就总安全了吗？** 不总是。必须证明该 RMW 在 release 之后并基于允许的状态执行。若从初值 0 提前加到 1，再被误认为完成发布，就没有期望的来源链。Reference 通过 CAS 的 1→2 前提避免这个问题。

**HB 已把初始化排在读取之前，能否随后反复改 payload？** 不能据此推出安全；下一次写与当前读之间仍缺排序。下一章通过确认或不可变快照处理这个问题。

## 规范入口

[N5050](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5050.pdf) 的 [intro.races]、[atomics.order] 与 [atomics.wait] 给出依据。[P0982R1](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0982r1.html) 说明 C++20 缩减 release sequence 的动机；旧描述不能覆盖新标准。[P3475R2](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p3475r2.pdf) 解释 consume 相关关系简化的背景。
