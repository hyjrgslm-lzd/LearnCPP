# 08 模块 F：内存模型与 memory_order

## 模块目标

模块 E 让你会用原子（atomic）做 load / store / RMW（read-modify-write，读-改-写），但回避了一个最硬的问题：**多个原子操作、以及它们旁边那些普通变量的读写，在不同线程看来到底是什么顺序？** 单线程里你默认“代码怎么写就怎么执行”，但编译器和 CPU 为了性能会重排（reorder）指令、把写缓存在本核里延迟可见——只要不改变**单线程**可观测行为就允许。一旦多线程并发读写同一批数据，这些重排就会暴露成“别的线程看到的顺序和我写的顺序不一样”的诡异现象。

C++ 的**内存模型（memory model）**就是这套并发可见性与顺序的精确规则；`std::memory_order` 则是你给每个原子操作贴的“顺序强度标签”，用来告诉编译器和 CPU：这次操作需要多强的顺序保证。标签选弱了，程序有数据竞争（data race）甚至读到垃圾；选强了，凭空付出屏障开销拖慢性能。本模块要把这套规则讲到你**能用 happens-before（先行于）链条做推理**，而不是靠“在我机器上跑对了”这种运气——尤其因为 x86 是强内存模型，很多错误内存序在 x86 上会“碰巧正确”，换到 ARM/POWER 或开高优化就崩，这正是内存序 bug 最阴险之处。

本模块定位很明确：它是**无锁编程的语言地基**。后面模块 G（无锁数据结构）、I（安全内存回收）写的每一行 `compare_exchange`、每一个 release 发布、每一次 acquire 订阅，正确性都直接落在你这一模块建立的 happens-before 推理能力上。这是全套最偏理论的一章，**事实正确性高于一切**——宁可慢一点把每条规则的边界吃准。

## 模块完成标准

做完本模块，你至少要能稳定说清楚（can-do）：

- 能列出六种内存序——`memory_order_relaxed` / `consume` / `acquire` / `release` / `acq_rel` / `seq_cst`——并说出每种约束什么、不约束什么。
- 能精确定义三组关系：**sequenced-before（先序于，单线程内按求值顺序）**、**synchronizes-with（同步于，release 写被读到它的 acquire 读建立）**、**happens-before（先行于，前两者的传递闭包，还包含锁 unlock→lock、线程 create/join）**。
- 能用 release-acquire 配对，把“一个原子标志之前写的一整片非原子数据”安全发布给读到该标志的线程，并画出完整 happens-before 链。
- 能说清 **release sequence（释放序列）**：release 写之后由同线程后续 RMW、或其他线程 RMW 组成的序列，acquire 读到序列中任一值仍建立同步。
- 能说清 relaxed 的正确边界：只保证单变量的原子性与**修改顺序（modification order）**，不建立跨变量可见性——适合纯计数，不适合做标志位发布数据。
- 能说清 seq_cst 比 acquire/release 多给的东西：所有 seq_cst 操作的**单一全序（single total order）**，并知道它能修复 acquire/release 修不了的 **store-load 重排**（如 Dekker / IRIW）。
- 能用 `std::atomic_thread_fence`（acquire / release / seq_cst 栅栏）配合原子操作，把屏障从“每个变量”集中到“线程关键点”。
- 能给出**数据竞争（data race）= 未定义行为（UB）**的精确定义，并据此判断一段代码是否有 race。

---

## 练习 F-1：release-acquire 同步

> 代码目录：`exercises/F1_release_acquire/`

### 目标

用 `std::atomic<bool>` 的 **release 写 / acquire 读** 配对，安全发布一块**非原子（non-atomic）payload**：生产方先写好普通数据、再用 release 写把标志置真；消费方用 acquire 读自旋等标志，读到后再读那块普通数据，保证看到完整、最新的值。你要能**画出这条 happens-before 链**；并通过把 acquire 换成 relaxed，理解为何可见性会被破坏、为何那是 data race / UB。

### 前置理解

- 三组关系，逐个吃准：
  - **sequenced-before（先序于）**：纯**单线程内**的概念，由语言的求值顺序定义。`a = 1;`（语句先）sequenced-before `b = 2;`（语句后）。它不跨线程。
  - **synchronizes-with（同步于）**：跨线程的同步点。当线程 B 的一个 **acquire 读**，**读到了**线程 A 的一个 **release 写**所写入的值时，A 的那次 release 写 **synchronizes-with** B 的那次 acquire 读。注意触发条件是“**读到了那个值**”——没读到就不成立。
  - **happens-before（先行于）**：sequenced-before 与 synchronizes-with 的**传递闭包**。它还囊括其他同步：互斥锁的 `unlock` happens-before 下一次 `lock`；线程创建（`std::thread` 构造）happens-before 新线程的起始；新线程的全部操作 happens-before 对它的 `join` 返回之后。
- 核心定理（本模块的“主定理”）：**若 A happens-before B，则 A 之前写的内容对 B 之后的读可见，且这之间没有数据竞争。**
- 把定理套到本题：生产方“写 a/b/note”sequenced-before“release 写 ready”；release 写 synchronizes-with 消费方“acquire 读到 ready==true”；acquire 读 sequenced-before“读 a/b/note”。传递起来：**生产方的全部写 happens-before 消费方的全部读**。于是消费方读非原子 payload 既安全（无 race）又看到完整值。
- **release 写**的语义：本线程在它之前的所有读写不会被重排到它之后，并为读到该值的 acquire 提供同步源。**acquire 读**的语义：本线程在它之后的所有读写不会被重排到它之前，且一旦读到 release 写的值就接上同步。两者必须**配对**才有意义：单独一个 release 写、或单独一个 acquire 读，都不构成同步。

### 必做任务

1. **release/acquire 安全发布非原子 payload**（对应 `// TODO [必做 1]`）：
   - 生产方 `producer()`：先写 `g_payload.a / b / note`（普通非原子变量），最后 `g_ready.store(true, std::memory_order_release)`。
   - 消费方 `consumer()`：`while(!g_ready.load(std::memory_order_acquire)) std::this_thread::yield();` 自旋等待；读到 true 后读 `g_payload` 并打印。
   - 在纸上（或注释里）**画出 happens-before 边**：写 a → 写 b → 写 note → store(release) ⟶(synchronizes-with) load(acquire)==true → 读 a/b/note。

2. 记录并回答：
   - synchronizes-with 是在“acquire 读执行”那一刻无条件建立的，还是必须“acquire 读到了那次 release 写的值”才建立？
   - 为什么生产方对非原子 `g_payload` 的写、消费方对它的读，**不**构成数据竞争？是哪条关系担保的？
   - 如果消费方在 `g_ready` 还是 false 时就去读 `g_payload`，会怎样？

### 进阶任务

- **把 acquire 换成 relaxed，讲清为何是 UB**（对应 `// TODO [进阶 1]`）：消费方改用 `g_ready.load(std::memory_order_relaxed)` 自旋。理论要点：relaxed 读**不**与生产方的 release 写建立 synchronizes-with，于是**不**产生 happens-before；消费方读非原子 `g_payload` 与生产方写它**没有先后关系** = data race = UB。即便 ready 读到了 true，payload 各字段也**不保证**已可见——编译器可能把 payload 写重排到 ready 写之后，弱内存模型 CPU 可能延迟让其他核看到 payload 写。
- 解释“为什么在我的 x86 机器上 relaxed 版常常也打印出正确数据”：x86 是强内存模型（TSO），普通 store 近似带 release、普通 load 近似带 acquire，store-store / load-load 不被硬件重排，所以 relaxed 在 x86 上**碰巧正确**。但**代码依然是 UB**：换 ARM/POWER、或开 `/O2` 让编译器重排，就可能读到 `a=0, b=0, note=""`。
- 思考：若把 `g_payload` 本身也声明成一堆 `std::atomic`，还需要 ready 的 release/acquire 吗？（提示：每个原子各自有序，但**跨原子之间**的相对顺序仍需 release/acquire 或 seq_cst 来锚定——把数据全做成 atomic 既贵又不解决“整片一致发布”的问题。）

### 验收点

- 你能用 release/acquire 配对安全发布一块非原子数据，并完整画出 happens-before 链。
- 你能准确陈述 synchronizes-with 的触发条件是“acquire 读到了 release 写的值”。
- 你能解释把 acquire 换成 relaxed 为何破坏可见性、为何构成 data race / UB。
- 你能解释“x86 上看不出错 ≠ 代码正确”，并坚持用 happens-before 推理而非平台表现下结论。

### 观察点

- 正确版里：消费方一旦读到 `ready==true`，`g_payload` 的三个字段必然都是生产方写入的最新值——这不是巧合，是 happens-before 强制的。
- relaxed 版里：在 x86 上你很可能仍看到正确数据；这恰恰是内存序 bug 的伪装——“能跑”不等于“正确”。
- release 写之前的**所有**写（不只是某一个变量）都被这次发布带过去了——这正是“用一个标志发布一整片数据”的威力。

### 常见坑

- **只写了 release、读端用 relaxed**（或反之）：配对断裂，synchronizes-with 不成立，等于没同步。release 与 acquire 必须成对。
- **以为“acquire 读执行了”就建立同步**：不。必须**读到了**对应 release 写进去的值才建立。读到旧值不算。
- **用 x86 的表现给内存序“验收”**：x86 太宽容，会掩盖 relaxed/缺失同步的错误。要按标准推理，或在 ARM 上验证，或用 TSan/`-fsanitize=thread`。
- **把非原子数据的可见性寄希望于 `volatile`**：`volatile` 与多线程内存序**无关**，它不建立 happens-before，不能替代 atomic + memory_order。

### 提示

- 自旋等待用 `std::this_thread::yield()` 让出 CPU，避免忙等占满核（学习期足够；生产中用 `atomic::wait`/条件变量更好）。
- 让 consumer 先起跑、producer 稍后发布（代码里 `sleep_for(50ms)`），更能体现“消费方确实在 acquire 上等到了发布”。
- 用 `cs::logf(...)`（`concurrency_study/log.hpp`）打印带线程 id 与时间戳的日志，观察发布与读取的先后。

### 复盘问题

- sequenced-before、synchronizes-with、happens-before 三者各自的定义是什么？后者如何由前两者传递得到？
- “release 写 synchronizes-with acquire 读”的**确切**触发条件是什么？少了哪个条件就不成立？
- 把 acquire 换成 relaxed 后，哪条关系消失了？为什么这会让读 payload 变成 data race？
- 为什么 x86 上 relaxed 版常常“也对”？这说明了关于“测试能否证明内存序正确”的什么道理？

### 对应官方参考

- cppreference [`std::memory_order`](https://en.cppreference.com/w/cpp/atomic/memory_order)（Release-Acquire ordering 一节及其 message-passing 示例）
- 《C++ Concurrency in Action, 2nd ed.》(Anthony Williams) 第 5 章 5.3.1 / 5.3.2
- Herb Sutter, "atomic<> Weapons: The C++ Memory Model and Modern Hardware"
- Mara Bos, 《Rust Atomics and Locks》第 3 章（Release and Acquire；跨语言内存序直觉通用）

---

## 练习 F-2：relaxed 计数器

> 代码目录：`exercises/F2_relaxed_counter/`

### 目标

论证为什么纯计数器用 `memory_order_relaxed` 的 `fetch_add` 是**正确**的——计数只依赖**原子性（atomicity）**与**单变量的修改顺序（modification order）**，不需要任何跨变量的可见性顺序。再反向理解：用 relaxed 做“标志位发布数据”为何**错误**——relaxed 不建立 synchronizes-with / happens-before，两个独立变量的写在别的线程看来可被重排观测到。一句话边界：**relaxed 只管一个原子变量自己，不管不同变量之间的先后。**

### 前置理解

- **修改顺序（modification order）**：标准保证每个原子变量都有一条**全线程一致**的修改顺序（它自己的写按某个全序发生）。无论用哪种内存序，对同一个原子变量的所有写都串在这条序列上。
- **RMW 的原子性**：`fetch_add` 是一次不可分割的读-改-写。多个线程并发 `fetch_add(1)`，每次都基于修改顺序里“当前最新值”加一，**不会丢更新、不会撕裂**——这是 atomic 本身（任意内存序）就保证的。
- 把上面两点合起来：N 个线程各 +1 共 M 次，最终值精确等于 N×M，**与内存序强度无关**。计数器不关心“别的变量有没有可见”，所以 relaxed 既正确又最省（不插多余屏障）。
- relaxed 不提供的东西：跨变量的可见性顺序。它**不**建立 synchronizes-with，**不**产生 happens-before。所以“看到 flag 的新值 ⇒ 看到 data 的新值”这种发布语义，relaxed 给不了。

### 必做任务

1. **relaxed 计数求和正确**（对应 `// TODO [必做 1]`）：起 8 个线程，每个对一个 `std::atomic<long long>` 做 100000 次 `fetch_add(1, std::memory_order_relaxed)`。join 后验证结果精确等于 `8 × 100000 = 800000`。

2. 记录并回答：
   - relaxed 计数为什么不会丢更新？是“原子性”还是“修改顺序”在起作用，还是两者都要？
   - 把 `relaxed` 换成 `seq_cst`，结果会变吗？性能会变吗？说明你为何不该为计数器付 seq_cst 的钱。

### 进阶任务

- **relaxed 做标志位的可观测重排（反例）**（对应 `// TODO [进阶 1]`）：构造 message-passing——生产线程 relaxed 写 `data=1` 再 relaxed 写 `flag=1`；消费线程读到 `flag==1` 后 relaxed 读 `data`。统计“看到 flag=1 却读到 data=0”的次数；再用 release/acquire 版本（flag 写 release、读 acquire）作对照，理论上恒为 0。
- 解释 x86 上的现实：x86（TSO）不重排 store-store，所以 relaxed 版在 x86 上**可能统计为 0**（难复现重排）。这**不代表 relaxed 用作标志位是对的**——它在标准上没有任何 happens-before 保证，换 ARM/POWER 或编译器重排即暴露。结论必须建立在“relaxed 不建立 happens-before”这条规则上，而非某次运行的计数。
- 思考：什么样的“统计/计数”场景确实可以全程 relaxed？（提示：各线程只往同一个计数器累加、最终只在所有线程 join 后读一次结果——读取处的 join 已提供 happens-before。）

### 验收点

- 你能论证 relaxed 计数为何正确，并指出依赖的是原子性 + 修改顺序、而非跨变量顺序。
- 你能说清 relaxed 用作标志位为何错误：不建立 synchronizes-with / happens-before。
- 你能给出 relaxed 的适用边界一句话，并举出一个该用、一个不该用的例子。

### 观察点

- 计数版无论你把内存序调到多弱，结果都精确——这说明“正确性”和“顺序强度”在纯计数场景里**解耦**了。
- 反例版在 x86 上可能一次都观测不到 data=0，但这只是平台运气；release/acquire 对照版在任何平台都恒为 0，差别正是 happens-before 的有无。

### 常见坑

- **“relaxed 不安全所以从不用”**：错。relaxed 在纯计数/统计里既安全又最快，盲目升到 seq_cst 是白付屏障开销。
- **用 relaxed 写标志位发布数据**：本题头号反面教材。发布数据必须 release/acquire（或 seq_cst）。
- **以 x86 没复现重排来“证明”relaxed 标志位是对的**：x86 太宽容，复现不到不等于正确。
- **以为 relaxed 完全没有任何顺序**：它仍保证**同一个**原子变量的修改顺序与读取的单调性（不会读到比已见过的更旧的值）；它丢的是**跨变量**的顺序。

### 提示

- 计数：`std::atomic<long long> c{0}; c.fetch_add(1, std::memory_order_relaxed);`。
- 反例为高效复现，用常驻两线程 + 轮次票据（ticket）反复跑同一段 message-passing，而非每轮新建线程（建线程的开销会淹没要观测的重排）。代码里 `run_message_passing(...)` 即此结构。
- 对照组只需把 flag 的 store 换 `release`、load 换 `acquire`，其余不变，直观看出 happens-before 的作用。

### 复盘问题

- 修改顺序（modification order）是什么？它为什么足以让 relaxed 计数不丢更新？
- relaxed 与 seq_cst 在“纯计数求和”这件事上结果有差别吗？性能呢？你据此如何为计数器选序？
- 为什么 relaxed 不能用来“发布数据”？缺的是哪条关系？
- 你在反例里 x86 上观测到几次 data=0？这个数字能否用来判断 relaxed 标志位是否正确？为什么？

### 对应官方参考

- cppreference [`std::memory_order`](https://en.cppreference.com/w/cpp/atomic/memory_order)（Relaxed ordering；及 modification order 的定义）
- 《C++ Concurrency in Action, 2nd ed.》(Anthony Williams) 第 5 章 5.3.3（含 relaxed 与 fetch_add 计数示例）
- Mara Bos, 《Rust Atomics and Locks》第 3 章（Relaxed Ordering）

---

## 练习 F-3：seq_cst 与内存栅栏

> 代码目录：`exercises/F3_seqcst_fence/`

### 目标

复现经典的 **store-load 重排**现象（Dekker / store-buffer 模式）：两个线程各自“先 store 自己的标志、再 load 对方的标志”，在 relaxed 下**可能两边都读到对方的旧值（0）**。然后用 `memory_order_seq_cst` 修复，并理解为什么 **acquire/release 修不了它、只有 seq_cst 的单一全序能修**；最后用 `std::atomic_thread_fence(memory_order_seq_cst)` 替代“每操作 seq_cst”，达到同样效果而把屏障集中到关键点。

### 前置理解

- **store-load（StoreLoad）重排**：同一线程内“写 x，再读 y”，可能被其他线程观测成“先读 y（旧值），后写 x”。四种重排（StoreStore/StoreLoad/LoadLoad/LoadStore）里，**x86（TSO）唯一允许的就是 StoreLoad**——所以本现象在 x86 上能**真实复现**（不是只在 ARM 上）。
- **为什么 acquire/release 修不了**：release/acquire 约束的是 **message-passing 方向**——“A 在某原子上的 release 写”与“B 读到它的 acquire 读”之间的可见性。但本题是**对称的两个线程各写各的、各读对方**，问题出在“写自己 vs 读对方”的相对顺序（StoreLoad），release/acquire 语义并不禁止这种重排。
- **seq_cst 的额外保证**：`seq_cst` 操作除了各自带 acquire/release 语义，还**全部服从一条单一全序（single total order）**——存在一个所有线程一致认同的、把全部 seq_cst 操作排成一列的顺序。在这条全序里，两个线程的 store 必有先后；排在前的那个 store 之后，另一线程的 load 一定能看到它。于是“两边都读到 0”被全序禁止。
- **`std::atomic_thread_fence`**：独立的内存栅栏，不绑定到某个具体原子操作。
  - `fence(release)` ：把它之前的写“向后挡住”，配合后续某个 relaxed 原子写，等效一个 release 写。
  - `fence(acquire)` ：把它之后的读“向前挡住”，配合之前某个 relaxed 原子读，等效一个 acquire 读。
  - `fence(seq_cst)` ：一道**参与全局 seq_cst 全序**的全栅栏。在“写自己之后、读对方之前”插一道，可把本线程的 relaxed store 与后续 relaxed load 在全序上隔开，等效修复 store-load 重排。
- **consume 一句话**：`memory_order_consume` 本想提供比 acquire 更弱、只沿“数据依赖”传播的同步，但因难以正确实现，**主流编译器实践中一律把它当 acquire 实现**（提案中也长期建议暂勿使用）。知道有这回事即可，本模块不深入。

### 必做任务

1. **实现 store-load 模式并用 seq_cst 修复**（对应 `// TODO [必做 1]`）：
   - 两个线程：t0 写 `x=1` 再读 `y`；t1 写 `y=1` 再读 `x`。统计多轮中“t0 读到 y==0 且 t1 读到 x==0”（记为 `both==0`）的次数。
   - **relaxed 版**：store/load 全用 relaxed，`both==0` 次数应 **>0**（在 x86 上能复现 StoreLoad 重排）。
   - **seq_cst 版**：store/load 全用 seq_cst，`both==0` 次数应**恒为 0**（单一全序禁止）。

2. 记录并回答：
   - 为什么 relaxed 版会出现 both==0？用“写自己被推迟到读对方之后”解释它。
   - 为什么 seq_cst 版恒为 0？用“单一全序里两个 store 必有先后”解释它。
   - 为什么把 store/load 改成 acquire/release（而非 seq_cst）**修不好**这个？

### 进阶任务

- **用 atomic_thread_fence(seq_cst) 替代每操作 seq_cst**（对应 `// TODO [进阶 1]`）：store 与 load 改回 relaxed，在两者之间插一道 `std::atomic_thread_fence(std::memory_order_seq_cst)`。验证 `both==0` 同样恒为 0。体会：与其给每个变量都贴 seq_cst（处处屏障），不如在“写一批、读一批”的关键节插一道全栅栏（一次屏障覆盖多个操作）。
- 观察 relaxed 版的 `both==0` 计数有多大（本机上约占 ~40% 的轮次都能观测到）——这是 StoreLoad 重排在 x86 上**真实且高频**的铁证，不是教科书上的传说。
- 思考（选做）：IRIW（Independent Reads of Independent Writes）是另一个只有 seq_cst 能保证一致结论的经典场景——两个写者各写一个变量，两个读者以相反顺序观察，acquire/release 允许两读者得出矛盾的先后结论，seq_cst 的全序则强制一致。了解它属于“为什么有时非 seq_cst 不可”的同类问题。

### 验收点

- 你能复现 store-load 重排（relaxed 版出现 both==0），并说出它是四种重排里的哪一种、为何 x86 也会有。
- 你能用 seq_cst 修复，并用“单一全序”解释为何 both==0 不可能发生。
- 你能解释 acquire/release 为何修不了 store-load 重排（它管的是另一个方向）。
- 你能用 `atomic_thread_fence(seq_cst)` 替代每操作 seq_cst，并说清“把屏障集中到关键点”的取舍。

### 观察点

- relaxed 版的 both==0 计数在 x86 上往往相当可观（几十万轮里出现几十万次量级）——亲眼看到“先写后读被重排成先读后写”。
- seq_cst 版与 fence 版的 both==0 都恒为 0，且 seq_cst 通常比 relaxed 慢（多了屏障，如 x86 上的 `mfence` / `lock` 前缀）——正确性的代价是可量化的。

### 常见坑

- **以为 acquire/release 是“最强非 seq_cst，能修一切”**：修不了 store-load 重排。需要全序时只能 seq_cst（或 seq_cst fence）。
- **每个变量都贴 seq_cst 求稳**：能对，但常常过度，处处屏障拖慢全程。能用 release/acquire 的地方别用 seq_cst；需要全序的关键点用 fence 集中。
- **混用 fence 与原子操作时配错方向**：release fence 要配“其后的写”、acquire fence 要配“其前的读”。seq_cst fence 两边都挡且入全序，最不易错。
- **每轮新建线程来跑 litmus**：建/销线程开销巨大且让两线程难以真正同时冲进临界点，会让你**观测不到**本该出现的重排。要用常驻线程 + 紧凑同步（本题 `run_store_load` 即此结构）。

### 提示

- litmus 高效写法：起两个常驻 worker，用一个 `std::atomic<int> turn` 轮次票据“发车”，主线程每轮复位 x/y 后推进 turn，worker 抢着跑同一段 store-load，主线程等两者 done 后判 both==0。
- 把轮数设大（如 1e6）以稳定复现 relaxed 的重排；seq_cst/fence 版恒为 0 无需大轮数也成立。
- seq_cst store/load：`x.store(1, std::memory_order_seq_cst); y.load(std::memory_order_seq_cst);`。
- seq_cst fence：`x.store(1, relaxed); std::atomic_thread_fence(std::memory_order_seq_cst); y.load(relaxed);`，两线程**对称**地插。

### 复盘问题

- store-load 重排是什么？为什么连强内存模型的 x86 也允许它？
- seq_cst 比 acquire/release 多保证了什么？“单一全序”如何禁止 both==0？
- 为什么 acquire/release 修不了 store-load 重排？它约束的到底是哪种顺序？
- `atomic_thread_fence(seq_cst)` 如何替代每操作 seq_cst？相比给每个变量贴 seq_cst，它的好处是什么？

### 对应官方参考

- cppreference [`std::memory_order`](https://en.cppreference.com/w/cpp/atomic/memory_order)（Sequentially-consistent ordering）
- cppreference [`std::atomic_thread_fence`](https://en.cppreference.com/w/cpp/atomic/atomic_thread_fence)
- 《C++ Concurrency in Action, 2nd ed.》(Anthony Williams) 第 5 章 5.3.3（含 seq_cst 全序与 fence）
- Herb Sutter, "atomic<> Weapons"（讲透 SC 全序、StoreLoad 重排与硬件屏障）

---

## 练习 F-4：发布-订阅内存序模式

> 代码目录：`exercises/F4_publish_pattern/`

### 目标

把 F-1 的“一个 bool 标志发布一个 payload”推广为**通用发布-订阅模式**：单生产者反复发布**新版本**的一整块结构体/缓冲，用一个原子**版本号（version / sequence number）**作为发布点；消费者用 acquire 读版本号、读到新版本后再读数据。借此把 release-acquire 从“发布一次”升级为“持续发布”，并认清这正是**无锁（lock-free）数据结构的发布骨架**。

### 前置理解

- 被发布的 `g_data` 是**非原子**结构体；它的可见性**完全**由原子版本号 `g_version` 的 release/acquire 配对担保——和 F-1 同源，只是把“一次性 bool”换成“可递增的版本号”。
- **生产者不变式**：先写好新一版 `g_data` 的每个字段（这些写 sequenced-before 后面的 release 写），再 `g_version.store(v, release)` 发布。绝不能反过来（先发版本号再写数据）。
- **消费者不变式**（要能背出这条推理链）：`g_version.load(acquire)` 读到版本 V ⟹ 与生产者写 V 的那次 release 写 synchronizes-with ⟹ 生产者在那之前对 `g_data` 的全部写 happens-before 我随后对 `g_data` 的读 ⟹ 我读到的必是与版本 V 配套的**完整、自洽**数据。
- **release sequence（释放序列）**：一次 release 写之后，由“同一线程对该原子的后续写”以及“任意线程对该原子的 RMW”组成的最长连续序列，叫释放序列。一个 acquire 读只要读到了释放序列中的**任一**值，就与最初那次 release 写建立 synchronizes-with。这条规则在“版本号被多个 RMW 推进”“引用计数被多线程 fetch_sub”等场景里至关重要——它让“中间被别人 RMW 过”不破坏同步。本题单写者下版本号是普通 release `store`，但理解释放序列能让你把模式安全推广到多写者 RMW。
- 单写者下，读者可能**跳过中间版本**直接看到最新版（读得慢时）——这正常。我们要的是“**看到的那一版数据自洽完整**”，而非“每一版都被看到”。

### 必做任务

1. **版本号发布整片数据**（对应 `// TODO [必做 1]`）：
   - 生产者每轮：写好新版 `g_data`（`id` / `value` / `label`），再 `g_version.store(v, std::memory_order_release)`，v 从 1 递增到 N。
   - 多个消费者：各自维护 `last_seen`，循环 `g_version.load(std::memory_order_acquire)`；发现版本号变大就读 `g_data` 并校验自洽（`value == id*1000+id`、`label == "snapshot-v"+id`），更新 `last_seen`，直到追到第 N 版退出。
   - 验证所有消费者看到的每一版都自洽（打印 `自洽? 1`）。

2. 记录并回答：
   - 消费者读到版本 V 后，凭哪条 happens-before 推理断定“读到的 g_data 一定是 V 配套的完整数据”？
   - 如果生产者把顺序写反（先 `store(v, release)` 再写 `g_data`），会出什么问题？
   - 为什么消费者“跳过中间版本”不算 bug？模式真正承诺的是什么？

### 进阶任务

- **认知延伸到无锁结构**（无独立 TODO，写在复盘里）：把“发布版本号”替换成“发布一个**指向新数据的指针**”——生产者构造好新节点/新缓冲，再用 release 写把 `head`/`data_ptr` 指过去；读者 acquire 读指针后访问其指向的数据。这就是无锁单写多读结构的核心；再叠加 RCU（模块 I）或引用计数解决“旧数据何时能回收”，就是工业级 read-mostly 结构的做法。请用一段话写清这个推广路径。
- 思考：本题版本号用 `store(release)`；若改成多生产者、用 `fetch_add(release)` 推进版本号，释放序列规则如何保证消费者仍能与“最初那次发布”建立同步？

### 验收点

- 你能用一个原子版本号 release/acquire 持续发布整片非原子数据，消费者读到的版本与数据始终自洽。
- 你能完整复述消费者那条 happens-before 推理链。
- 你能说清这是无锁/单写多读结构的发布骨架，并说出“发布指针 + RCU/引用计数”是其自然延伸。
- 你能解释 release sequence 的作用，知道它让“中间被 RMW 推进”不破坏同步。

### 观察点

- 多个消费者各自追版本，可能有的逐版看到、有的跳版看到，但每一次读到的 `g_data` 都自洽——这是 happens-before 在“持续发布”下的稳定表现。
- 生产者每发一版，所有阻塞在 acquire 自旋上的消费者很快放行——发布点（一次 release 原子写）就是整片新数据的“可见性开关”。

### 常见坑

- **先发版本号再写数据**：彻底破坏发布语义，消费者会读到半成品。发布写**必须**最后做。
- **消费者用 relaxed 读版本号**：同 F-1 进阶，断了 synchronizes-with，读 `g_data` 变 data race。
- **以为消费者必须看到每一个版本**：单写者下允许跳版；模式承诺“看到的那版自洽”，不承诺“每版必达”。
- **把 `g_data` 误当线程安全**：它是普通非原子数据；它的安全**完全寄生**在版本号的 release/acquire 上，离开这个配对去读它就是 race。

### 提示

- 生产者：写完 `g_data` 三个字段后 `g_version.store(v, std::memory_order_release);`。
- 消费者：`long long v = g_version.load(std::memory_order_acquire); if (v > last_seen) { 读 g_data; 校验; last_seen = v; } else std::this_thread::yield();`。
- 用 `value == id*1000+id` 这种“可由 id 推出”的规律值做自洽校验，一眼看出是否读到配套数据。
- 用 `cs::logf` 打印每个消费者看到的版本与数据，观察“跳版/逐版”与“恒自洽”。

### 复盘问题

- 把“发布一次的 bool”升级成“持续发布的版本号”，release/acquire 的推理有变化吗？变的是什么、不变的是什么？
- 消费者读到版本 V 即可断定数据完整——这条结论依赖的 happens-before 链是怎样的？
- release sequence 是什么？它在“版本号被多线程 RMW 推进”时保证了什么？
- 这个模式如何一步步推广成无锁单写多读结构？还差哪一块（提示：旧数据回收）由后续哪个模块解决？

### 对应官方参考

- cppreference [`std::memory_order`](https://en.cppreference.com/w/cpp/atomic/memory_order)（Release-Acquire ordering；Release sequence 一节）
- 《C++ Concurrency in Action, 2nd ed.》(Anthony Williams) 第 5 章 5.3（release sequence）；第 7 章（无锁结构里的发布模式）
- Mara Bos, 《Rust Atomics and Locks》第 3 章（release/acquire 与发布模式）

---

## 做完模块 F 之后，你现在应该能说清楚什么

至少把下面几句话说顺：

- **六种内存序**：`relaxed`（只保单变量原子性 + 修改顺序）、`consume`（弱依赖序，实践中被当 acquire）、`acquire`（读端同步、其后读写不前移）、`release`（写端同步、其前读写不后移）、`acq_rel`（RMW 同时具备 acquire 与 release）、`seq_cst`（在 acquire/release 之上再加全局单一全序）。
- **三组关系**：sequenced-before（单线程内按求值）、synchronizes-with（release 写被读到它的 acquire 读建立）、happens-before（前两者传递闭包，含锁 unlock→lock、线程 create/join）；主定理是“A happens-before B ⟹ A 的写对 B 的读可见且无 race”。
- **release-acquire 发布**：A 对原子做 release 写、B 对同一原子 acquire 读**且读到了 A 写的值**，则 A 之前（sequenced-before）的所有写对 B 之后可见；这是用一个原子标志安全发布一整片非原子数据的基础。
- **release sequence**：release 写之后由同线程后续写、或任意线程 RMW 组成的释放序列，acquire 读到序列中任一值仍与最初那次 release 写建立同步。
- **relaxed 的边界**：只保单变量的原子性与修改顺序，不建立跨变量可见性顺序——纯计数对，做标志位发布数据错。
- **seq_cst 的价值**：提供所有 seq_cst 操作的单一全序，能修复 acquire/release 修不了的 store-load 重排（Dekker / IRIW）；代价是更强的屏障。
- **栅栏**：`std::atomic_thread_fence`（acquire / release / seq_cst）配合原子操作，可把屏障从“每个变量”集中到“线程关键点”。
- **数据竞争（data race）= UB**：两个线程对同一非原子内存的访问，至少一个是写，且它们之间没有 happens-before 关系——这就是数据竞争，按标准是未定义行为；避免它的唯一正道是用原子 + 正确内存序建立 happens-before，而**不是**靠“在 x86 上跑对了”。
