# 原子操作 01：一次不可分割，究竟是哪一次

本篇代码是 [E1 Starter](../../exercises/E1_atomic_basics/main.cpp) 和 [E1 Reference](../../exercises/E1_atomic_basics/solution.cpp)。从会使用 mutex 到开始使用 atomic，第一步不是选择最弱的内存序，而是确定你希望别的线程不能插入的操作边界。

## 1. 从计数需求推出 RMW

普通 `int` 的自增在源语言中包含对旧值的读取和对新值的修改。不能把它硬说成固定的三条机器指令：优化、指令集和寻址方式会改变实际指令。C++ 的核心问题是，无同步的并发冲突访问中至少一方非原子，并且彼此没有 happens-before，便可能构成数据竞争。数据竞争导致未定义行为，结果不是“只会少算一点”。

为了可控地研究丢更新，E1 使用原子对象拆分操作：

```cpp
const int old = count.load();
// 另一个线程可以在这里访问 count。
count.store(old + 1);
```

Reference 的 `split_increment()` 先让 worker 保存 0，再通过 promise 告知主线程已经读完。主线程也读到 0、写入 1，随后才允许 worker 写回自己算出的 1。结果必为 1，所有共享计数访问都是原子的，辅助同步只用来强制这次合法交错。这不是弱内存序实验：默认 seq_cst 已经足够强，但仍有两个独立操作。

换成 `count.fetch_add(1)` 后，一次读取、计算、修改在这个原子对象上组成 RMW。一个原子对象有自己的 modification order（修改顺序）；RMW 读取其在修改顺序中的紧邻前驱值。因此后一个自增基于前一个自增留下的值继续推进，不能有两个操作都以同一个前驱为依据而覆盖彼此。E1 的四个线程各执行 2000 次，全部完成后检查 8000。

这里的“不可分割”是语言层面的原子性承诺，不是在承诺所有处理器核心物理上同时看见更新，也不是在承诺每次调用只生成一条机器指令。

## 2. 按返回值理解接口

E1 的 `operations()` 每一步都检查旧值和新值，适合先预测再运行。

| 操作 | 对目标的动作 | 返回什么 |
|---|---|---|
| load | 原子读取 | 该次读取得到的值 |
| store(v) | 原子写入 v | void |
| exchange(v) | RMW，无条件换入 v | 换入之前的值 |
| fetch_add/sub | RMW，增减 | 修改之前的值 |
| fetch_or/and/xor | RMW，按位修改 | 修改之前的值 |
| ++a / a++ | RMW，加一 | 分别是本次更新的新值 / 旧值 |
| a += n | RMW，加 n | 本次更新的新值 |

后续单独调用 load 时可能已看见其他线程的新修改，不能把“fetch_add 的返回值加一”与“随后的 load”在并发下当成同一次观察。前者属于自己的操作，后者是另一个操作。

`atomic<T>` 有取值转换，`int n = atomic_int;` 可以调用隐式 load；也支持从 T 赋值。它本身不可复制，并不等于它不能转成 T。课程写显式 load/store，是为了让读者数清楚操作次数并看到内存序。

整型、指针、浮点特化的接口并非完全相同。位运算属于整型；atomic<bool> 没有整数 fetch_add。原子指针的 fetch_add 按元素推进，得到一个指针值不等于已证明该位置可解引用，更不延长对象寿命。普通 CAS 回调里的有符号运算也不会因为最终要提交到 atomic 就免除溢出风险；下一篇为乘法显式约束数值范围。

## 3. 初始化不是一次发布

本课程使用 `std::atomic<int> count{0};` 明确表达初值。对象必须先构造完成，再供其他线程访问。原子构造不会让“另一个线程正好同时使用这块尚未构造完成的存储”变得合法。析构同理：必须先结束所有访问。

C++20 起 `std::atomic_flag flag{};` 的默认构造把旗标初始化为 clear，旧代码常用 `ATOMIC_FLAG_INIT`。构造是建立对象；`clear(order)` 是对象已经存在后的一次原子清除操作，具有 store 的序约束。不要把构造、clear 与“重新创建对象”混为一谈。

`volatile int` 仍然是普通非原子对象。volatile 用于语言规定的易变访问语义，不提供跨线程的互斥或 synchronizes-with。两个线程无同步地写 volatile int，不能因为它“每次都读内存”就获得安全性；本课程不运行这种 UB 示例。某些平台针对设备内存的扩展也不是可移植 C++ 线程同步协议。

## 4. 从 test-and-set 推出一把锁

E1 的 `spin_mutex` 用 `test_and_set(acquire)` 取锁。它同时把旗标设为 set 并返回旧状态。返回 false 的线程完成了 clear → set 转换，可以进入临界区；返回 true 的线程没有取得许可。

解锁调用 `clear(release)`。上一位持有者的普通数据写先于 release clear，下一位成功取得 false 的 acquire RMW 读取这次 clear，于是把同步链接上。临界区内普通计数器的写不再互相冲突。Reference 使用 lock_guard，确保退出作用域时释放锁。

看见 `flag.test(relaxed) == false` 只能说明一次观察，不是取得所有权。test-and-test-and-set 优化可以先做只读轮询，观察到 clear 后仍须通过 test_and_set 竞争。它可能减少反复写同一缓存行的流量，但有多少收益要测量；本篇不声称它总比 mutex 快。

atomic_flag 的原子状态操作有无锁保证，依靠它搭建的锁没有。若持锁线程被暂停，其他线程即使不断执行无锁 test_and_set，也无法进入临界区。因此“使用了无锁原子”不推出“算法无锁”，更不推出等待时间有界、公平或 wait-free。

## 5. 运行与结果边界

在 `C08_Concurrency/exercises` 中执行：

```powershell
cmake -S E1_atomic_basics -B build/e1 -G "Visual Studio 18 2026" -A x64
cmake --build build/e1 --config Release
./build/e1/Release/E1_atomic_basics.exe
ctest --test-dir build/e1 -C Release --output-on-failure
```

Reference 检查受控拆分自增结果 1、RMW 总数 8000、接口返回值、旗标状态及锁保护下的 8000。lock-free 查询结果随实现变化，false 也应正常通过。检查使用 Release 中仍有效的 cs::check。并发任务使用显式 launch::async，future::get 会把 worker 异常重新抛给主线程；程序不在观察窗口中写日志。

这组结果验证具体操作契约和运行历史。它不测性能，也不证明任意复合算法正确。针对“即使是 SC 的拆分增量也不原子”，[atomic_protocol_test.cpp](../../exercises/runtime_tests/atomic_protocol_test.cpp) 还枚举两线程四步操作的六种串行交错，让丢更新和保留两次更新都成为可检查的结果。

## 自测与答案

**把 load 和 store 都升级成 seq_cst 能修复丢更新吗？** 不能。顺序一致约束各个操作的排列，不能把两个操作合成一个；需要 RMW，或把整个复合过程放在同一临界区。

**exchange 与 store 的差别只是有没有返回值吗？** 不只是。exchange 是 RMW，读取紧邻前驱并产生修改；store 是写操作。release sequence 等规则也会区分两者，即使最后数值相同。

**为什么没有 static_assert(atomic<int>::is_always_lock_free)？** 本题测试的是原子语义，标准并不普遍要求 atomic<int> 始终无锁。强制这一断言会无故排除能正确实现课程实验的平台。

**yield 能证明另一个线程已经执行吗？** 不能。它是调度提示，不是同步边，也没有特定让出时长承诺。可控交错通过 promise 建立，锁内普通数据安全通过 acquire/release 证明。

## 规范与延伸

固定依据为 [N5050](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5050.pdf) 的 [atomics.types.operations]、[atomics.flag]、[atomics.lockfree] 和 [intro.races]；在线定位可用 [原子类型操作](https://eel.is/c++draft/atomics.types.operations)与[旗标](https://eel.is/c++draft/atomics.flag)，但在线页会滚动，版本差异以 N5050 为准。[P0883R2](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p0883r2.pdf) 是理解原子初始化改进的历史材料。
