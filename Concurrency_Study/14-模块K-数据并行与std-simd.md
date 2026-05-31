# 14 模块 K：数据并行与 std::simd

## 模块目标

到这里为止，你掌握的所有并行手段——线程、`jthread`、线程池、`async`——都属于**任务并行（task parallelism）**：让多个**独立的执行流**各干各的活。本模块切换到正交的另一个维度：**数据并行（data parallelism）**，更具体地说是 **SIMD（Single Instruction, Multiple Data，单指令多数据）**——在**一个线程内部**，用一条机器指令同时对一“批”相邻数据做同样的算术，把吞吐放大若干倍。

这是高性能计算（High-Performance Computing，HPC）的核心模块。现代 CPU 的浮点峰值算力**绝大部分**藏在向量单元里（SSE / AVX / AVX-512 / NEON）；不会用 SIMD，等于把一台车的大部分气缸闲置。SIMD 与多线程**相乘**而非相加：8 核 × 8 路 AVX = 64 倍理论吞吐。

本模块用三道题建立 SIMD 的完整心智模型：

1. **基础数据并行模型**：`batch`（向量批）的加载/运算/存储，以及所有 SIMD 代码都绕不开的**尾部余数（remainder）**处理。
2. **向量化归约**：点积——把“一批 lane”最终**水平归约（horizontal reduction）**成一个标量，并直面**浮点累加顺序**带来的误差。
3. **掩码与条件运算**：SIMD 里没有逐 lane 的 `if`，改用**比较得掩码（mask）+ select 选值**的**分支无关（branch-free）**范式。

> ⚠️ **标准现状（务必读）**：C++26 标准化了 `std::simd`（头文件 `<simd>`，命名空间 `std::simd`，提案 `P1928R15`）。但截至 2026 年中，**MSVC（含 VS2026）尚未实现**，GCC 16 起仅部分实现（具体以各编译器文档为准）。因此本套练习用 **xsimd**——一个 header-only、跨平台（MSVC/GCC/Clang 全支持）、API 与 `std::simd` 概念高度对应的 SIMD 抽象库——作为**当前能在 Windows + VS2026 上真正编译运行的回退**。文档正文以 `std::simd` 标准 API 为讲解目标，并在每题与文末给出 **xsimd ↔ std::simd 差异映射表**。迁移到标准时，主要是改命名空间/类型名，思想完全一致。

## 模块完成标准

做完本模块，你至少要能稳定说清楚（can-do）：

- 你能用一句话区分**任务并行**（多个独立执行流，本课程前面所有模块）与**数据并行 / SIMD**（一个线程内一条指令处理一批数据），并说出两者**相乘**放大吞吐。
- 你能用 `batch`（≈ `std::simd` 的 `basic_vec`）写出向量化的逐元素运算：以 `batch::size` 为步长 load → 运算 → store，并**正确处理尾部余数**（不足一整批的元素用标量补齐）。
- 你能向量化一个**归约**（如点积）：用**向量累加器**并行累加 `W` 路部分和，再用 `reduce_add`（≈ `std::simd::reduce(+)`）做**水平归约**成标量，并解释为什么 SIMD 结果与标量结果**不应严格相等**（浮点加法不满足结合律）。
- 你能用**比较得掩码 + select**写出**分支无关**的条件运算（abs / clamp / relu），解释“两边都算再按掩码挑”为什么能避免分支预测失败，并把 `batch_bool` ↔ `basic_mask`、`select` ↔ `where/select` 对应起来。
- 你能说清 `std::simd`（C++26 标准目标）与 xsimd（当前可编译回退）的关系，并照差异映射表把 xsimd 代码迁移到 `std::simd`。

> 本模块在概念上独立于前面的锁/原子/无锁内容，但属于 HPC 主线。它与模块 J（缓存与伪共享）互补：J 讲“数据怎么摆才不互相拖累”，K 讲“摆好的数据怎么一次算一批”。

---

## 练习 K-1：std::simd 基础（xsimd 回退）

> 代码目录：`exercises/K1_simd_basics/`

### 目标

建立 SIMD 数据并行的心智模型：用 `xsimd::batch<float>`（向量批）把“两数组逐元素相加 `c[i]=a[i]+b[i]`”向量化——以 `batch::size` 个元素为一步加载、相加、写回；并**正确处理尾部余数（remainder）**。用标量版逐元素对照验证一致，再粗略计时。

### 前置理解

- **SIMD（单指令多数据）**：一条向量指令同时对一“批”（**batch**，由若干 **lane / 通道**组成）相邻数据做**同一种**算术。`batch<float>::size` 是该平台一批容纳的 `float` 数：SSE=4、AVX=8、AVX-512=16、NEON=4。它是**线程内**的并行，和多线程正交——8 核 × 8 路 = 64 倍理论吞吐。
- **load / 运算 / store 三步**：
  - `batch::load_unaligned(ptr)`：从 `ptr` 起读 `size` 个 `float` 进一个 batch（`load_aligned` 要求 `ptr` 按 SIMD 宽度对齐，更快但更挑剔，入门先用 unaligned）。
  - `va + vb`：逐通道相加，**一条向量指令**完成 `size` 个加法。
  - `vc.store_unaligned(ptr)`：把 batch 的 `size` 个结果写回。
- **尾部余数（remainder / tail）**：数组长度 `n` 通常不是 `size` 的整数倍。主循环只能覆盖前 `n - n%size` 个元素，**剩下 `n%size` 个不足一整批，必须用标量循环补齐**。这是所有 SIMD 代码的必修课，漏了就漏算或越界——本题特意取 `N=100003`（`% 4 = 3`）逼你处理它。
- **标准对照**：`std::simd` 里这就是 `std::simd::vec<float, N>`（别名）/ `basic_vec<float, Abi>`（主模板），`v = a + b`，`v.copy_from(ptr, flags)` / `v.copy_to(ptr, flags)` 做 load/store。差异映射见下表与文末。

### 必做任务

1. **向量化主循环**（`// TODO [必做 1]`）：以 `W = batch::size` 为步长，每步 `load_unaligned` 一批 `a`、一批 `b`，相加，`store_unaligned` 回 `c`。循环上界用 `i + W <= n` 保证不越界。
2. **尾部余数 + 对照 + 计时**（`// TODO [必做 2]`）：主循环后用标量循环补齐 `n%W` 个元素；与标量版**逐元素对照**（加法无累加顺序问题，可要求严格相等）；分别计时。

### 进阶任务

- **对齐 load/store**：用 `xsimd::aligned_allocator` 分配对齐内存，改用 `load_aligned` / `store_aligned`，对比是否更快。
- **看汇编**：用 `/FA` 或 godbolt 看主循环是否真的发出了 `addps`/`vaddps` 向量指令。
- **不同宽度**：在支持的机器上用编译开关切换 SSE/AVX，观察 `batch::size` 从 4 变 8、吞吐随之变化。

### 验收点

- SIMD 结果与标量结果逐元素一致（不一致数 = 0）。
- 你能说清 `batch::size`、主循环步长、为什么必须处理尾部余数。
- 你能区分 SIMD（线程内数据并行）与多线程（任务并行）。

### 观察点

- 程序打印的 `batch::size`：在多数 x64 机器上 xsimd 默认是 4（SSE）或 8（AVX）。
- `N % size != 0`：尾部余数确实存在，标量补齐后结果仍全一致。
- 计时：本题数据小、加法是 memory-bound，SIMD 与标量差距可能不明显——这是正常的，K-2 的 compute-bound 点积才看得出加速。

### 常见坑

- **忘了尾部余数**：只写主循环，最后几个元素没算或 `load` 越界（读到数组外）。务必补标量循环。
- **`load_aligned` 配未对齐指针**：直接崩或 UB。入门一律 `load_unaligned`；要对齐就用对齐分配器。
- **把 `batch::size` 当运行期变量**：它是**编译期常量**（`constexpr`），用它写步长、开数组都没问题。
- **以为 SIMD 一定更快**：memory-bound 的运算瓶颈在内存带宽，SIMD 帮不上多少。SIMD 的主场是 compute-bound。

### 提示

- `batch::size` 是 `static constexpr`，写 `constexpr std::size_t W = batch::size;` 即可。
- 主循环上界写 `i + W <= n`（而非 `i < n - W`）以避免 `n < W` 时无符号下溢。
- 用本仓库 `cs::logf(...)`（`concurrency_study/log.hpp`）打印结果与计时。

### 复盘问题

- SIMD 和多线程分别在哪个维度并行？为什么说它们“相乘”？
- `batch::size` 是什么决定的？为什么它是编译期常量？
- 不处理尾部余数会发生什么？怎么补？
- 为什么本题加法的 SIMD 加速可能不明显？

### 对应官方参考

- 提案 [`P1928R15`](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p1928r15.pdf)（`std::simd`）
- cppreference [`<simd>`](https://en.cppreference.com/w/cpp/header/simd)（`std::simd` / `basic_vec` / `vec`）
- [xsimd 文档](https://xsimd.readthedocs.io/en/latest/)

### xsimd ↔ std::simd 差异映射（K-1 用到的部分）

| 概念 / 操作 | 本题 xsimd（可编译回退） | C++26 std::simd（`<simd>`，标准目标） | 说明 |
|---|---|---|---|
| 向量批类型 | `xsimd::batch<float>` | `std::simd::basic_vec<float, Abi>` / 别名 `vec<float, N>` | 概念对应；xsimd 自动选平台最优 Abi |
| 批宽度 | `batch<float>::size`（`constexpr`） | `vec<float,N>::size()` / 模板参数 `N` | 均编译期已知 |
| 加载 | `batch::load_unaligned(p)` / `load_aligned(p)` | `v.copy_from(p, simd_flag_default)` / `simd_flag_aligned` | 标准用成员 + flags |
| 存储 | `b.store_unaligned(p)` / `store_aligned(p)` | `v.copy_to(p, flags)` | 同上 |
| 逐元素算术 | `va + vb`、`*`、`-`、`/` | 同样的 `+ - * /` 运算符 | 一致 |

---

## 练习 K-2：SIMD 向量化点积

> 代码目录：`exercises/K2_simd_dotproduct/`

### 目标

把**点积（dot product）= Σ a[i]·b[i]** 向量化。它比 K-1 多一个本质难点：结果是**一个标量**，而 SIMD 天然产出一**批 lane**。要学会**累加器向量化（vectorized accumulator）**（`W` 路部分和并行累加）+ **水平归约（horizontal reduction）**（把 `W` 个 lane 横向加成标量），并直面**浮点累加顺序**带来的误差——用**相对容差**而非严格相等做对照。

### 前置理解

- **难点：归约改变了数据形状**。逐元素运算（K-1）输入一批、输出一批，形状不变。归约要把 `n` 个数**坍缩成一个**。SIMD 没法一步坍缩，分两段走：
- **累加器向量化**：开一个 `batch<float> acc`（初值全 0），主循环 `acc = acc + va*vb`（亦可 `xsimd::fma(va, vb, acc)` 一条融合乘加指令）。这相当于把“第 0、W、2W… 个乘积”累进 `lane0`，“第 1、W+1… 个”累进 `lane1`……于是 `W` 路部分和**并行累加、互不干扰**。循环结束时 `acc` 里是 `W` 个部分和，**还不是最终答案**。
- **水平归约（horizontal reduction）**：用 `xsimd::reduce_add(acc)` 把 `acc` 的 `W` 个 lane **横向**加成一个标量。这一步对应 `std::simd` 的 `reduce(acc, std::plus<>{})`。它是 SIMD 里相对“贵”的操作（lane 间要互相搬运），所以**只在循环外做一次**，绝不在主循环里每步归约。
- **尾部余数**：归约结束后，把剩下 `n%W` 个元素的乘积用标量加到结果上。
- **浮点累加顺序（关键认知）**：标量版从左到右逐个累加；SIMD 版把求和拆成 `W` 路 + 一次水平归约，**求和顺序不同**。浮点加法**不满足结合律**（`(a+b)+c ≠ a+(b+c)` 在舍入意义下），所以两者结果会有**微小差异**。这不是 bug，是浮点的本性。因此**对照必须用相对容差**（本题 `1e-4`），用 `==` 判等几乎必然“失败”。

### 必做任务

1. **向量化乘累加主循环**（`// TODO [必做 1]`）：向量累加器 `acc += va*vb`。
2. **水平归约**（`// TODO [必做 2]`）：`reduce_add(acc)` 成标量。
3. **尾部余数 + 容差对照 + 计时**（`// TODO [必做 3]`）：标量补齐余数；与标量基准用**相对误差 < 容差**比较；计时（点积是 compute-bound，应能看到明显加速）。

### 进阶任务

- **FMA**：把 `acc + va*vb` 换成 `xsimd::fma(va, vb, acc)`，对比精度与速度（FMA 只舍入一次，精度更高）。
- **多累加器**：用 2~4 个独立 `acc` 交替累加（打破依赖链），观察是否进一步加速（隐藏 FMA 延迟）。
- **量化误差**：把 `N` 加到百万级，对比 SIMD 与标量（`float` 累加）的相对误差怎么变；再把标量累加器换成 `double` 看“真值”漂移。

### 验收点

- SIMD 点积与标量点积的**相对误差在容差内**（本题 `1e-4`）。
- 你能说清向量累加器、为什么最后要水平归约、为什么归约只做一次。
- 你能解释为什么 SIMD 点积与标量点积结果**不应**严格相等（浮点不满足结合律）。

### 观察点

- `[simd]` 耗时明显小于 `[scalar]`：点积是 compute-bound，SIMD 主场（本题实测约 4× @ SSE）。
- 相对误差很小但**未必为 0**（取决于数据；本题整齐数据可能恰好为 0，换成随机浮点就会看到非零误差）。
- 把容差改成 `0`（即严格相等）再跑随机数据：会偶发“失败”，亲手体会浮点结合律。

### 常见坑

- **在主循环里每步都 `reduce_add`**：把最贵的操作放进热循环，慢且没必要。归约**只在循环外一次**。
- **用 `==` 对照**：浮点累加顺序不同，几乎必然不等。必须相对容差。
- **忘了把尾部余数加进结果**：少加最后几项，结果偏小。
- **累加器忘了初始化为 0**：`batch` 默认未必清零，显式 `broadcast(0.0f)`。

### 提示

- `acc` 用 `xsimd::batch<float>::broadcast(0.0f)` 初始化。
- 容差比较：`std::abs(simd - ref) / (std::abs(ref) + 1e-12) < tol`。
- 标量基准用 `double` 累加，减小基准自身误差，让它更接近“真值”。

### 复盘问题

- 为什么归约比逐元素运算更难向量化？SIMD 分哪两段处理它？
- 水平归约为什么贵？为什么只能在循环外做一次？
- 为什么 SIMD 点积和标量点积结果不该严格相等？对照该怎么写？
- 多累加器为什么可能更快？（提示：依赖链 / 指令级并行）

### 对应官方参考

- 提案 [`P1928R15`](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p1928r15.pdf)（`std::simd::reduce`）
- cppreference [`<simd>`](https://en.cppreference.com/w/cpp/header/simd)
- [xsimd reduce 文档](https://xsimd.readthedocs.io/en/latest/api/reducer_index.html)

### xsimd ↔ std::simd 差异映射（K-2 用到的部分）

| 概念 / 操作 | 本题 xsimd（可编译回退） | C++26 std::simd（标准目标） | 说明 |
|---|---|---|---|
| 向量累加器 | `xsimd::batch<float> acc` | `std::simd::vec<float,N> acc` | 同为向量寄存器变量 |
| 乘累加 | `acc = acc + va*vb` / `xsimd::fma(va,vb,acc)` | `acc = acc + va*vb` / `std::simd::fma(...)` | 一致 |
| 广播初值 | `batch::broadcast(0.0f)` | `vec<float,N>{0.0f}`（broadcast 构造） | 标准用广播构造 |
| 水平求和 | `xsimd::reduce_add(acc)` | `std::simd::reduce(acc, std::plus<>{})` | 标准用通用 `reduce` + 二元函子 |

---

## 练习 K-3：SIMD select / where 掩码

> 代码目录：`exercises/K3_simd_where_select/`

### 目标

解决 SIMD 里的“`if` 怎么办”：向量一次处理 `W` 条 lane，但每条 lane 的条件可能不同（有的走 then、有的走 else），CPU**无法对一个向量分叉跳转**。改用**分支无关（branch-free）**范式——**比较得掩码（mask）+ select（条件选择）按掩码逐 lane 挑值**。用 `abs` / `clamp` / `relu` 三种条件运算练手，与标量逐元素对照。

### 前置理解

- **为什么不能逐 lane `if`**：一条向量指令对所有 lane 做**同一件事**。若 `lane0` 该取 `-x`、`lane1` 该取 `x`，没有“向量分叉跳转”这种指令。出路是**两个分支都算出来**，再按条件**逐 lane 挑选**。
- **比较得掩码（mask）**：`va < vb` 不返回单个 `bool`，而是返回一个**掩码** `xsimd::batch_bool<float>`——每条 lane 一个 bool（真/假）。它对应 `std::simd` 的 `basic_mask<T>`（别名 `mask<T,N>`）。
- **select（条件选择）**：`xsimd::select(mask, x, y)` 逐 lane——`mask` 为真取 `x`、为假取 `y`。**本质：then 分支 `x` 和 else 分支 `y` 两边都算，再按掩码挑**。没有真正的跳转，因此**没有分支预测失败**——对随机条件，分支无关版常常**比标量 `if` 更快**（标量 `if` 在随机条件下分支预测频繁失败，每次失败要清空流水线）。它对应 `std::simd` 的 `select` / `where` 表达式。
- **min/max 本就分支无关**：`xsimd::min` / `max` 是逐 lane 取小/取大的向量指令，天生无分支。clamp 到 `[lo,hi]` = `min(max(x,lo), hi)`，比两次 select 更直接。
- **三个例子**：
  - `abs(x)` = `select(x < 0, -x, x)`
  - `clamp(x,lo,hi)` = `min(max(x, lo), hi)`
  - `relu(x)` = `select(x > 0, x, 0)`（也可 `max(x,0)`；本题特意用 select 强调“条件赋值”这一通用模式）
- **对照可严格相等**：select/min/max 只是**挑选/比较**已有数值，不做累加，**不引入浮点结合律误差**，所以 SIMD 结果与标量结果可**逐元素严格相等**（与 K-2 的容差对照对比鲜明）。

### 必做任务

1. **abs**（`// TODO [必做 1]`）：`select(x<0, -x, x)`，比较得 `batch_bool`、`select` 选值。
2. **clamp**（`// TODO [必做 2]`）：`min(max(x,lo),hi)`（可另用两次 select 体会等价）。
3. **relu**（`// TODO [必做 3]`）：`select(x>0, x, 0)`；三者均与标量逐元素**严格相等**对照、计时。

### 进阶任务

- **where 写法**：`std::simd` 的 `where(mask, v) = expr` 是“只对掩码为真的 lane 赋值”的**条件写**。xsimd 中可用 `xsimd::select` 模拟（`v = select(mask, expr, v)`），实现“掩码内更新、掩码外保持”。
- **掩码归约**：用 `xsimd::any(mask)` / `all(mask)` / `count(mask)` 统计满足条件的 lane 数（对应 `std::simd` 的 `any_of`/`all_of`/`reduce_count`）。
- **分支 vs 无分支的代价对照**：把输入条件改成**高度可预测**（如全正）再测——此时标量 `if` 分支预测几乎不失败，可能反超；再改成**随机正负**，分支无关版优势重现。亲手体会“分支预测失败”的成本。

### 验收点

- 三种条件运算的 SIMD 结果与标量结果**逐元素完全一致**（不一致数 = 0）。
- 你能说清“为什么 SIMD 不能逐 lane 跳转”，以及“掩码 + select 如何替代 `if`”。
- 你能把 `batch_bool` ↔ `basic_mask`、`select` ↔ `where/select` 对应起来。

### 观察点

- 三种运算的 SIMD 结果与标量**严格相等**（不一致 = 0）——因为没有累加，无浮点结合律问题。
- `select` 的两个分支**都被求值**（`-vx` 和 `vx` 都算了）——这是分支无关的代价，也是它快的原因。
- 数据条件随机时（本题 `x` 正负交替），分支无关版的稳定性优于标量 `if`。

### 常见坑

- **想在 SIMD 里写 `if (lane...)`**：没有这种东西。条件一律走“掩码 + select / min / max”。
- **以为 select 只算了选中的那边**：两边都算了。若某分支有**副作用或越界风险**（如除以可能为 0 的数），要先把危险输入“消毒”再 select。
- **把 `batch_bool` 当普通 `batch` 用**：它是掩码类型，参与 `select` / `any` / `all`，不直接做算术。
- **用容差对照**：本题没有累加，应**严格相等**；用容差会掩盖真实 bug。

### 提示

- 比较结果直接喂给 `select`：`auto m = vx < batch::broadcast(0.0f); auto r = xsimd::select(m, -vx, vx);`。
- clamp 优先用 `min(max(...))`，比两次 select 简洁。
- 对照用 `!=` 计数不一致；select/min/max 不引入误差，严格相等是合理预期。

### 复盘问题

- 为什么 SIMD 里没有逐 lane 的 `if`？用什么替代？
- `select(mask, x, y)` 到底算了几个分支？这是优点还是缺点，何时是缺点？
- 为什么本题对照能用严格相等，而 K-2 点积不能？
- 什么情况下分支无关反而比标量 `if` 慢？（提示：分支高度可预测时）

### 对应官方参考

- 提案 [`P1928R15`](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p1928r15.pdf)（`where` / `select` / `mask`）
- cppreference [`<simd>`](https://en.cppreference.com/w/cpp/header/simd)（`basic_mask` / `mask` / `where` / `select`）
- [xsimd select 文档](https://xsimd.readthedocs.io/en/latest/api/cond_index.html)

### xsimd ↔ std::simd 差异映射（K-3 用到的部分）

| 概念 / 操作 | 本题 xsimd（可编译回退） | C++26 std::simd（标准目标） | 说明 |
|---|---|---|---|
| 掩码类型 | `xsimd::batch_bool<float>` | `std::simd::basic_mask<...>` / 别名 `mask<float,N>` | 每 lane 一 bool |
| 比较得掩码 | `va < vb`、`>`、`==` … | 同样的比较运算符 → `mask` | 一致 |
| 条件选择 | `xsimd::select(m, x, y)` | `std::simd::select(m, x, y)` | 同名 |
| 条件写 | `v = xsimd::select(m, expr, v)`（模拟） | `where(m, v) = expr` | 标准有专门的 where 表达式 |
| 逐 lane min/max | `xsimd::min/max(a, b)` | `std::simd::min/max(a, b)` | 同名，天生分支无关 |
| 掩码归约 | `xsimd::any/all/count(m)` | `std::simd::any_of/all_of/reduce_count(m)` | 命名不同，语义对应 |

---

## xsimd ↔ std::simd 总差异映射表

> 本套用 **xsimd**（header-only，支持 MSVC，当前**可编译运行的回退**）；标准目标是 **C++26 `std::simd`**（`<simd>`，`P1928R15`，MSVC 尚未实现）。两者概念高度对应，迁移主要是改类型名/命名空间。

| 概念 / 操作 | xsimd（当前回退） | C++26 std::simd（标准目标，`<simd>`） | 差异 / 说明 |
|---|---|---|---|
| 头文件 / 命名空间 | `<xsimd/xsimd.hpp>`，`xsimd::` | `<simd>`，`std::simd::` | 头与命名空间不同 |
| 向量批（主类型） | `xsimd::batch<T>` | `std::simd::basic_vec<T, Abi>` | 概念对应 |
| 向量批别名 | `xsimd::batch<T, Arch>` | `std::simd::vec<T, N>` | std 用元素数 `N`；xsimd 用架构标签 `Arch` |
| 掩码 | `xsimd::batch_bool<T>` | `std::simd::basic_mask<...>` / `mask<T,N>` | 每 lane 一 bool |
| 批宽度 | `batch<T>::size`（`constexpr`） | `vec<T,N>::size()` / 模板参 `N` | 均编译期已知 |
| 加载 | `batch::load_unaligned(p)` / `load_aligned(p)` | `v.copy_from(p, flags)`（`simd_flag_default`/`_aligned`） | std 用成员 + flags |
| 存储 | `b.store_unaligned(p)` / `store_aligned(p)` | `v.copy_to(p, flags)` | 同上 |
| 广播标量 | `batch::broadcast(x)` | `vec<T,N>{x}`（广播构造） | std 用构造 |
| 逐元素算术 | `+ - * /`、`xsimd::fma` | `+ - * /`、`std::simd::fma` | 一致 |
| 比较 → 掩码 | `a < b` → `batch_bool` | `a < b` → `mask` | 一致 |
| 条件选择 | `xsimd::select(m, a, b)` | `std::simd::select(m, a, b)` | 同名 |
| 条件写 | `v = select(m, expr, v)`（模拟） | `where(m, v) = expr` | std 有专门 where 表达式 |
| 逐 lane min/max | `xsimd::min/max` | `std::simd::min/max` | 同名 |
| 水平求和（归约） | `xsimd::reduce_add(b)` | `std::simd::reduce(v, std::plus<>{})` | std 用通用 `reduce`+函子 |
| 掩码归约 | `xsimd::any/all/count(m)` | `std::simd::any_of/all_of/reduce_count(m)` | 命名不同，语义对应 |

> 迁移要点：当 MSVC 等实现了 C++26 `<simd>` 后，把 `xsimd::batch<T>` 换成 `std::simd::vec<T,N>`、`load_unaligned/store_unaligned` 换成 `copy_from/copy_to(flags)`、`reduce_add` 换成 `reduce(.,std::plus<>{})`、`select` 基本同名、条件写改用 `where`。核心算法（主循环 + 尾部余数 + 向量累加器 + 水平归约 + 掩码 select）**一字不改**。

---

## 做完模块 K 之后，你现在应该能说清楚什么

至少把下面几句话说顺：

- **数据并行 vs 任务并行**：SIMD 是**一个线程内**用一条指令处理一批数据；多线程是多个独立执行流各干各的。两者正交、**相乘**放大吞吐（核数 × 向量宽度）。SIMD 是 HPC 浮点峰值算力的主要来源。
- **逐元素向量化模式**：以 `batch::size` 为步长 load → 运算 → store；末尾**必须处理尾部余数**（标量补齐），否则漏算/越界。
- **归约向量化**：用**向量累加器**并行累加 `W` 路部分和，循环外**一次**水平归约（`reduce_add` ≈ `std::simd::reduce(+)`）成标量；因浮点不满足结合律，SIMD 与标量结果**不应严格相等**，对照用相对容差。
- **分支无关条件运算**：SIMD 没有逐 lane `if`；用**比较得掩码 + select**（两边都算再按掩码挑）替代，避免分支预测失败；min/max 天生无分支；`batch_bool` ↔ `basic_mask`、`select` ↔ `where/select`。这类纯挑选运算不引入误差，对照可严格相等。
- **标准现状与迁移**：`std::simd`（`P1928R15`）已入 **C++26** 但 MSVC 等尚未实现；本套用 xsimd 建立直觉，迁移到标准只需照差异映射表换类型名/命名空间，算法不变。
