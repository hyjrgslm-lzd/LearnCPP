# 原子等待：等的是值改变，不是每一次通知

本篇对应 [H3 Starter](../../exercises/H3_atomic_wait_notify/main.cpp) 与 [H3 Reference](../../exercises/H3_atomic_wait_notify/solution.cpp)。先掌握一次性等待，再区分“内部被唤醒”“函数对外返回”“业务事件发生”三个层次。把它们混为一谈，会同时误解伪唤醒、ABA 和 payload 发布。

## 1. 从忙等到 wait

一次性发布可以反复 acquire load ready，直到读到 true。正确性已经由发布关系证明，但没有工作可做时，持续轮询可能占用执行资源。C++20 的 atomic::wait(old, order) 让实现能够在值未变化时阻塞等待。

它的抽象步骤是：用给定 order 读取原子并比较值表示；若与 old 不同则返回；否则阻塞，因通知或内部伪唤醒恢复后，重新执行读取和比较。返回类型是 void，不返回使它退出的那个新值。一般需要值本身时要另做 load，并分析两次观察之间是否可能又发生修改。

H3 的 `one_shot(true)` 在读者中调用 `ready.wait(false, acquire)`。ready 只从 false 变到 true、不再回退，所以一次 wait 足以判定一次性谓词满足。主线程先写普通 payload=42，再 release store ready=true，随后 notify。读者对 payload 的检查异常由 future 回传。

`one_shot(false)` 使用 acquire load 与 yield 轮询，验证同一正确性契约。它没有计时，也不要求读者一定先阻塞、一定经过内核或一定消耗更多 CPU。标准库实现可能先短暂自旋再阻塞；仅凭调用了 wait 不能断言“零 CPU 消耗”或固定系统调用次数。

## 2. 内部伪唤醒不等于对外伪返回

条件变量 wait 可以返回后发现谓词仍为假，所以经典接口需要循环复查。atomic::wait 的上述内部循环已经确保：它对外返回必须依据一次观察到与 old 不同的值。不能把条件变量“可能伪返回”的说明原样移植过来。

这并不等于所有 atomic wait 都不需要外层循环。若业务谓词是“数量至少为 10”，wait(3) 因为值变成 4 而返回，业务条件仍不满足；需要 load 后判断，再等待新的旧值。如果有其他线程会把状态改回去，对外返回之后的另一次 load 也可能读到 old，这不否定 wait 返回前曾经观察到不同值。

区分两个断言：一是“wait 内部最终观察不同才返回”，二是“函数返回后，该对象永远保持不同”。标准提供前者，业务协议决定后者。H3 的一次性 ready 和有限递增 generation 具有单调条件，因此可以据此简化。

## 3. 通知不保存事件，也不发布数据

notify_one/all 使符合条件的原子等待操作解除阻塞；没有正在等待的线程时，它不积攒一张日后可消费的通知票据。它也不负责改变原子值。只 notify 而保持值等于 old，等待者即使在内部醒来，也会比较后继续等待。

因此正常顺序是先更新状态，再通知。若先 notify，等待者可能醒来后仍见旧值、再次阻塞；随后只改值而不通知，就不能依赖它及时醒来。若更新发生在等待者调用 wait 之前，只要状态仍不同，wait 自己的读取就可以直接返回，不需要把早先的通知“保存”下来。

payload 的发布仍依赖 release store/RMW 与 acquire load/wait 的同步来源。把 ready 的更新改成 relaxed，同时只保留 notify，并不能让普通 payload 可安全读取。若只是记录无关联数据的计数，relaxed 则可以成立；这与通知机制是分开的选择。

## 4. A→B→A 为什么无法靠 while 补救

假设读者保存 old=false，生产者把 bit 改成 true 又改回 false，读者直到此后才调用 wait(false)。它可以一直看不到中间的 true。wait 没有违约：当前值确实又等于 false，而它不是修改历史数据库。

在外面写 `while (!bit.load()) bit.wait(false);` 无法恢复已经丢失的信息。无论再读多少次，若后续永远是 false，仍然不知道中间是否发生过事件。把这段程序当成自动结束的测试也不合适，因为最后没有使它退出的新状态。

H3 的 `generation_history()` 用 armed 与 inspect 两个一次性门精确控制交错：读者先保存 bit=false 与 generation=0，告知已保存；生产者执行 bit=true、generation+1，再执行 bit=false、generation+1，最后允许读者检查。于是读者确定看到 bit 与 old_bit 相同，而 generation 与 old_generation 相差 2。它等待的是 generation，不对已经回退的 bit 执行可能永久阻塞的 wait。

这个实验的 inspect 门刻意增加了完成同步，以证明两次修改都位于观察之前。它研究的是信息保留，不能用它证明 generation 单独承担 payload 发布。一次性 payload 的发布已在另一个没有该门的实验中检查。

## 5. generation 能保留什么

generation_stream() 中生产者发布 1000 次递增，每次修改后 notify。读者保存 seen，wait(seen) 后重新 load current，累计 current-seen，再更新 seen。如果一次观察从 3 跳到 8，就累计 5，不把一次唤醒误算成一次事件。

这个程序只关心计数，所以 generation 的 RMW 与读取可以 relaxed；没有一个被反复覆盖、等待读者访问的普通 payload。更新单调且本题不回绕，读者对同一原子的后续读取不会回退到已观察修改之前，最终累计应为 1000。通知可能合并，读者也可能在生产全部完成后才开始执行；两种情况都不影响计数契约。

如果需要每个事件的具体内容，应使用队列、确认交接或其他存储协议。把每次 payload 覆盖在同一个普通缓冲，然后把 generation 递增，仍然重现 F4 的读写竞争。计数器保存了“发生多少次”，没有保存“每次是什么”。

generation 也有回绕边界。unsigned 的模运算本身有定义，但如果读者落后整整一个模周期，值会再次等于 seen，ABA 问题回来。实际系统应规定最大落后范围，选择足够宽的计数并证明不会在等待者持有旧值时绕回，或采用更强的事件存储协议；不能把“64 位很大”说成数学上绝不回绕。

## 6. 关闭、取消和对象寿命

本题退出由有限事件目标或不可回退的一次性终态决定。atomic::wait 本身没有 stop_token 参数，也没有超时重载；jthread 的停止请求不会自动解除它的等待。一般服务关闭时需要更新等待谓词的状态并通知，或者改用支持相应取消协议的等待原语。

通知者、等待者都必须在原子对象寿命内操作。不是“已经改值”就能立即销毁等待对象；仍要等待所有 wait 调用和通知访问结束。H3 的原子状态均在主线程栈上，future 完成后才离开作用域。

本题多个场景都只用一个读者，所以 notify_one 足够。多等待者共享一次性完成标志而全部都应继续时，通常需要 notify_all；否则那些已经阻塞且没有被唤醒的等待者不能仅靠值已经变化就保证完成。唤醒策略要与角色和退出契约匹配。

## 7. 运行与自测答案

在 `Concurrency_Study/exercises` 中：

```powershell
cmake -S H3_atomic_wait_notify -B build/h3 -G "Visual Studio 18 2026" -A x64
cmake --build build/h3 --config Release
ctest --test-dir build/h3 -C Release --output-on-failure
```

Reference 应通过 wait/spin 两种一次发布、受控两次修改及 1000 次 generation 累计。它不要求某次 wait 确实阻塞，不计数内核唤醒，也不检查哪个实现更快。不存在依赖 sleep 的顺序假设。

**只 notify 能否让 wait(old) 返回？** 值始终相等时不能据此对外返回；内部醒来后会重查。

**一次性 bool 为什么可以不用外层 while？** 状态单调 false→true，业务谓词恰好就是“与 false 不同”。若状态可回退或业务谓词更复杂，理由不再成立。

**wait 返回后 load 又等于 old，是否标准库伪返回？** 未必，可能在内部观察不同之后有其他线程把值改回。需要检查完整修改历史。

**generation 每次增加 1，是否保证每个消费者都收到每个 payload？** 不保证。它可以统计变化，但不替代逐项存储、消费确认和寿命管理。

规范依据为 [N5050](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5050.pdf) 的 [atomics.wait]、[atomics.types.operations] 与 [atomics.flag]；[P1135R6](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p1135r6.html) 提供 C++20 同步库设计背景。
