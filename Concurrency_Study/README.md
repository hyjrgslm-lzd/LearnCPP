# C++26 并发编程 / HPC 自学练习包

这是一套「通过亲手编码理解并发与高性能计算」的练习材料，不是一份背 API 的速查笔记。
它和同目录的 `Coroutine_Study\`（协程）、`Execution_Study\`（`std::execution` sender/receiver）
共享同一套教学骨架：**先建立心智模型，再分模块由浅入深地做题，每题都画图、跑代码、写复盘**。

---

## 1. 这套材料要解决什么问题

市面上讲 C++ 并发的资料，要么停在「`std::thread` + `std::mutex` 怎么用」的入门层，
要么直接跳进「无锁队列 / 内存序」的硬核论文，中间缺一条**可以自己走完的路**。
而 HPC（High-Performance Computing，高性能计算）相关的数据并行（SIMD）、并行算法、
缓存效应，往往又被单独拆到另一批资料里，与并发正确性割裂。

本套材料把这条路补全：从 `future`/`promise`/`mutex` 这样的基础设施，到原子操作与
`memory_order` 内存模型，再到 `condition_variable`/`latch`/`barrier`/`semaphore` 这类
成熟同步基建，最后落到伪共享、`std::simd`、并行 STL、工作窃取线程池这些 HPC 主题。
**每一层都让你先理解「为什么需要它、它解决了什么、用错会怎样」，再动手写。**

> 关于 `std::execution`（sender/receiver，P2300，C++26 纳入）：它是更上层的**结构化并发**
> 模型，已在 `Execution_Study\` 中深入讲解。本套**只在模块 M 做桥接**——展示它如何取代裸线程，
> 然后把你引导过去，不重复造轮子。

---

## 2. 三阶段定位

| 阶段 | 名字 | 你会获得的能力 |
|------|------|----------------|
| 一 | **并发基础（会用）** | 能安全地创建/管理线程、用锁和条件变量保护共享状态、用 future 拿异步结果，知道每个原语的适用边界与陷阱 |
| 二 | **原子与内存模型（懂底层）** | 能读懂并正确写出原子操作，理解六种 `memory_order` 与 happens-before，能实现简单无锁结构并讲清它为什么正确 |
| 三 | **HPC（工程价值）** | 能识别并消除伪共享、用 `std::simd` 做数据并行、用并行算法真正榨出加速、实现工作窃取线程池，并知道何时该上结构化并发 |

每阶段以一个**结课项目**收尾，把该阶段的原语综合到一个能跑、能测、能复盘的小系统里。

---

## 3. 你会得到什么

- **3 个阶段**，共 **13 个字母模块**（A–M）+ 3 个结课项目。
- **42 道练习**（含 3 个结课项目），每道都是一个可独立构建的 CMake 子项目。
- 每道练习配 **TODO 骨架式 `main.cpp`**：骨架本身可编译运行，你按 `// TODO [必做 N]:` 填空。
- 配套 `exercises/` 完整构建系统：CMake + Presets，**主战场 Windows + Visual Studio 2026（MSVC）**。

---

## 4. 阅读顺序

**先读 `01-心智模型.md`**（不含练习，是整套材料的概念地基），再按编号推进：

**阶段一 · 并发基础**
- `01-心智模型.md`
- `02-模块A-线程生命周期与jthread.md`
- `03-模块B-互斥与锁.md`
- `04-模块C-条件变量.md`
- `05-模块D-future与异步任务.md`
- `06-第一阶段结课-线程池与生产者消费者.md`

**阶段二 · 原子与内存模型**
- `07-模块E-原子操作基础.md`
- `08-模块F-内存模型与memory_order.md` ← 全套最硬核的一章
- `09-模块G-无锁数据结构.md`
- `10-模块H-高级同步原语.md`
- `11-模块I-安全内存回收.md`
- `12-第二阶段结课-无锁队列.md`

**阶段三 · HPC**
- `13-模块J-缓存与伪共享.md`
- `14-模块K-数据并行与std-simd.md`
- `15-模块L-并行算法与执行策略.md`
- `16-模块M-工作窃取与结构化并发桥接.md`
- `17-第三阶段结课-并行计算项目.md`
- `18-源码阅读路线.md`（附录）

---

## 5. 统一技术基线

- **语言标准**：基线 **C++20**。C++20 已经提供了本套绝大多数原语（`jthread`、`stop_token`、
  `latch`、`barrier`、`counting_semaphore`、`atomic_ref`、`atomic` 的 `wait`/`notify`、
  `atomic<shared_ptr>`）。个别题目按需提升标准或切换回退库。
- **主战场**：**Windows + Visual Studio 2026（MSVC）**。所有练习都以「能在 VS2026 上真正编译运行」为准。
  GCC/Clang 亦可（见 `exercises/BUILD_GUIDE.md`）。
- **关于 C++26 的诚实说明**：本套以多个 **C++26 标准 API 为讲解目标**，但其中几个截至 2026 年中
  主流编译器**尚未实现**。我们采用「**标准 API 为目标 + 当前可编译的回退实现**」策略
  （与 `Execution_Study\` 用 stdexec 代替 P2300 是同一做法），并在文档中明确标注差异：

  | C++26 特性 | 标准头文件 / 名字 | 现状 | 本套采用的可编译方案 |
  |---|---|---|---|
  | `std::simd`（P1928） | `<simd>`，`std::simd`，`basic_vec`/`vec`/`basic_mask`/`mask` | MSVC 未实现（GCC 16 起部分支持，以各编译器文档为准） | **xsimd**（header-only，MSVC 友好）+ 差异映射表 |
  | `std::hazard_pointer`（P2530） | `<hazard_pointer>`，`std::hazard_pointer` | 未实现 | `exercises/include/` 内**自带教学实现** |
  | `std::rcu`（P2545） | `<rcu>`，`std::rcu_domain` 等 | 未实现 | `exercises/include/` 内**自带教学实现** |
  | senders（P2300） | `<execution>`，`std::execution` | 未实现 | 仅模块 M 桥接，可选 **stdexec** |

  反过来，**阶段一、二的全部内容在 MSVC 上是纯标准、零外部依赖**的——这部分你学到的就是真正的标准 C++。
- **线程规则**：除非题目显式要求，优先用 `std::jthread` 而非 `std::thread`（自动 join + 内建取消）。
- **编码哲学**：先想清楚「谁拥有这块共享状态、谁在并发访问、用什么同步」，再写代码；
  能用值传递/不可变共享解决的，就不要引入可变共享状态。

---

## 6. 每题统一交付物

做完一道题，你应该留下四样东西：

1. **可运行的代码**：填完 TODO，构建通过，运行输出符合预期。
2. **一张并发结构草图**：画出「哪些线程、共享了什么、用什么同步、值/信号怎么流动」。
3. **5–10 行观察记录**：你跑出来看到了什么现象（尤其是和直觉不符的）。
4. **复盘结论**：用文档末尾「复盘问题」自问自答，把这道题的核心一句话说清楚。

---

## 7. 每题统一模板（10 小节）

每道练习在模块文档里都按这个固定结构展开，方便你对照：

```
### 目标          —— 一句话：这题训练什么
### 前置理解      —— 做题前你必须已经知道的概念
### 必做任务      —— 编号的填空步骤
### 进阶任务      —— 可选的加深挑战
### 验收点        —— 你能证明自己完成的判定标准
### 观察点        —— 做完应该注意到的现象
### 常见坑        —— 最易犯的错 + 根因
### 提示          —— 工具性建议
### 复盘问题      —— 概念层面的反思
### 对应官方参考  —— cppreference / 提案 / 博客
```

---

## 8. 推荐做题方法

1. **先画图再写码**：在 `main.cpp` 旁边先画好并发结构草图，标出共享状态与同步点。
2. **小步验证**：填一个 TODO 就构建运行一次，别攒着。
3. **故意制造竞争**：很多题让你先写一个「错」的版本观察数据竞争/死锁，再修正——别跳过这一步。
4. **用工具佐证**：内存序相关题目，在 GCC/Clang 下加 `-fsanitize=thread`（ThreadSanitizer）交叉验证。
5. **写复盘**：合上文档，用自己的话回答「复盘问题」。

---

## 9. 最终验收清单（学完你应该能做到）

**阶段一**
- 能解释 `std::thread` 与 `std::jthread` 的区别，并默认用后者。
- 能用 `stop_token` 实现协作式取消，知道它不是强制 kill。
- 能正确选用 `lock_guard`/`unique_lock`/`scoped_lock`/`shared_lock`，并说出死锁的四个必要条件。
- 能用 `condition_variable` + 谓词写出无虚假/丢失唤醒的等待，并解释为什么必须配谓词。
- 能用 `promise`/`future`/`packaged_task`/`async` 取异步结果，并说出 `std::async` 默认策略的陷阱。

**阶段二**
- 能说清六种 `memory_order` 各自的保证，画出 release-acquire 的 happens-before 边。
- 能写出正确的 CAS 循环，并解释 `compare_exchange_weak` 与 `strong` 的取舍。
- 能实现 Treiber 栈 / SPSC 队列，指出其中每个原子操作为什么用那个内存序。
- 能说明无锁结构的回收难题，并对比 `atomic<shared_ptr>` / hazard pointer / RCU 三种解法。
- 能用 `latch`/`barrier`/`semaphore`/`atomic::wait` 替代手写忙等。

**阶段三**
- 能用 `perf`/计时实测伪共享，并用 `alignas(hardware_destructive_interference_size)` 消除它。
- 能用 `std::simd`（或 xsimd 回退）写出向量化的点积/归约，并讲清 SIMD 的数据并行模型。
- 能判断一个算法用 `std::execution::par` 是否真能加速，知道并行的开销来自哪里。
- 能实现一个带工作窃取的线程池，并说出它相对单队列线程池的优势。

---

## 10. 术语速查表

| 术语 | 你在练习里会看到什么 | 你应该问自己的问题 |
|------|----------------------|---------------------|
| data race（数据竞争） | 两个线程无同步地访问同一非原子对象，至少一个写 | 这块内存有没有同步保护？ |
| happens-before | sequenced-before（单线程内）与 synchronizes-with（跨线程同步边：锁的 unlock→lock、release→acquire、线程 create/join 等）的传递闭包 | A 的效果对 B 可见吗？靠哪条同步边（锁 / 原子 / 线程操作）？ |
| `memory_order` | 按保证强度递增：`relaxed`/`consume`/`acquire`/`release`/`acq_rel`/`seq_cst` | 这个原子操作到底需要哪一级保证？ |
| CAS | `compare_exchange_weak/strong` | 失败时谁改了它？需要重读什么？ |
| ABA | 指针被改回原值但语义已变 | 我只比较了指针值，够吗？ |
| false sharing（伪共享） | 不同线程的变量挤在同一 cache line | 这两个热点变量在同一缓存行吗？ |
| SIMD / 数据并行 | `std::simd<float>` 一次算 N 个 | 这个循环每个元素独立吗？能向量化吗？ |
| work stealing（工作窃取） | 每个工作线程一个 deque，空了去偷 | 负载均衡了吗？任务粒度合适吗？ |

---

## 11. 一个非常重要的现实提醒

- **C++26 库特性的实现严重滞后于标准**。`std::simd`、`std::hazard_pointer`、`std::rcu`、
  senders 截至 2026 年中在 MSVC 上都还没有。本套用回退库让你**今天就能跑**，但请始终分清
  「标准 API 长什么样」与「回退库 API 长什么样」——每个相关模块都有差异对照表。
- **并发 bug 不可靠复现**。数据竞争可能跑一万次都「对」，第一万零一次崩。所以本套强调
  **用内存模型推理证明正确性**，而不是「跑过了就对了」。ThreadSanitizer 是你的朋友。
- **测出来的性能才算数**。HPC 部分的任何「优化」都必须用计时/profiler 佐证，不能凭感觉。

---

## 12. 参考资料入口

- **核心书籍**：Anthony Williams《C++ Concurrency in Action, 2nd ed.》（并发圣经）；
  Mara Bos《Rust Atomics and Locks》（讲内存模型的直觉极佳，跨语言通用）。
- **标准提案**：P1928（`std::simd`）、P2530（`hazard_pointer`）、P2545（`rcu`）、
  P2300（`std::execution`）。
- **权威文档**：cppreference 的 `<thread>`/`<atomic>`/`<execution>`/`<simd>` 页面；
  cppreference C++ 内存模型页（`std::memory_order`）。
- **演讲**：Herb Sutter "atomic<> Weapons"（内存模型经典）；Fedor Pikus "Lock-free Programming"。
- **参考实现/库**：folly（hazptr/RCU/MPMCQueue）、xsimd、NVIDIA/stdexec、moodycamel ConcurrentQueue。
- 每道练习末尾的「对应官方参考」会给出精确到该题的入口。

---

## 13. 配套代码怎么构建

详见 `exercises/BUILD_GUIDE.md`。最快路径（Windows / VS2026）：

```bash
cd exercises
cmake --preset vs2026
cmake --build --preset vs2026
```

阶段一、二零依赖、可离线构建；阶段三仅模块 K（xsimd）、模块 M（stdexec 桥接题）需联网拉依赖。

---

## 14. 最后一句

把这套练习当成**三层训练**：阶段一练「安全」，阶段二练「正确性证明」，阶段三练「性能」。
并发的难，不在于写出能跑的代码，而在于**证明它在所有交错下都对**，并且**还快**。慢慢来，画图，写复盘。
