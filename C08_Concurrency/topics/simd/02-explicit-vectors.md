# SIMD 02：整批加载、短尾与掩码的真实边界

本篇把[上一节](01-scalar-and-layout.md)的逐元素加法改成真实显式 SIMD。全部实现位于 [simd_kernels.hpp](../../exercises/include/concurrency_study/simd_kernels.hpp)，必做答案在 [K1](../../exercises/K1_simd_basics/solution.cpp) 与 [K3](../../exercises/K3_simd_where_select/solution.cpp)，边界检查在 [numeric_test.cpp](../../exercises/runtime_tests/numeric_test.cpp)。所有长度和重叠检查都在触碰输出前执行。

## 1. 四个通道不是四个线程

x64 基线 SSE2 支持把四个 float 放入 128 位寄存器。`_mm_add_ps` 对相应通道做加法，产生四个 float 结果；它不创建线程，也不为独立任务安排调度。主循环的具体代码是：

```cpp
for (; a.size() - i >= 4; i += 4) {
    const auto va = _mm_loadu_ps(a.data() + i);
    const auto vb = _mm_loadu_ps(b.data() + i);
    _mm_storeu_ps(out.data() + i, _mm_add_ps(va, vb));
}
for (; i < a.size(); ++i) out[i] = a[i] + b[i];
```

前提 i 从 0 开始且始终不超过长度，所以 `size-i` 不下溢，也避免 `i+4` 在极端长度下溢出检查之外的加法问题。主循环每次访问四个真实存在的对象，尾循环只处理剩余元素。n=0 完全不访问内存，n=3 完全由尾循环处理，n=5 由一批加一项完成。

在 x64 上，SSE2 是本执行文件选定的基础能力。宏 `CS_NUMERIC_SSE2` 仅在 x64 编译条件下为 1；其他平台不会编译或执行这一分支，available 返回 false。不要通过函数名猜测后端，基准的 sse2/xsimd/std_simd 是不同标签。

## 2. aligned 与 unaligned 的区别是前提

`_mm_load_ps/_mm_store_ps` 要求本例地址满足 16 字节对齐；带 u 的形式不要求这项 SIMD 对齐，但仍要求指向足够数量的合法 float 对象。unaligned 从来不意味着“可以越界”或“可以读没有构造的内存”。

对齐版本入口先检查 a、b、out 三个起点的地址余数，任一个不满足就抛出失败，不执行危险指令。主循环步长为四个 float，即 16 字节，因此首地址对齐足以保持每批对齐。`alignas(16) float a[8]` 可以提供这个前提；普通 `vector<float>` 的必要保证主要是 float 对齐，不能用某次地址碰巧为 16 的倍数推导一般保证。

测试同时覆盖对齐起点、偏移 1/2/3 个 float 的合法不对齐跨度、以及声称 aligned 却偏移一个元素的拒绝路径。`std::assume_aligned` 不会帮你分配或移动数据，只是给优化器一项前提；前提不成立时后果由调用者承担，所以本题先检查真实地址。

## 3. 掩码选择不会倒转已经发生的读取

设剩下三个元素，却先从末尾指针加载四个 float，再把最后一通道 mask 掉。第四个读取已经发生，mask 不能补救。这可能跨过对象边界，甚至落到未映射页；是否恰好没有崩溃与是否合法无关。

本题提供两种安全尾部。默认是 scalar 尾循环。`add_sse2_tail_mask` 则只把仍然有效的元素复制到四元素、16 字节对齐的临时数组，其他通道初始化为零；对临时数组做完整 SSE 加法，按 live 通道 mask 结果，再只复制 live 元素回 out。每一次向量访问都有四个真实对象，调用者数组没有多读多写。

这个尾部确实执行 SIMD 与掩码，但不声称 SSE2 有原生 fault-suppressing masked load。复制临时数组有开销，对于最多三个尾元素，scalar 可能更便宜。两种版本用于理解安全协议，应以实际任务测量决定，而不是觉得“代码全是向量”就更先进。

测试的输出前后放哨兵，检查没有额外写；输入采用紧贴分配结尾的数组，在 ASan 配置下帮助发现多读。哨兵本身只能检测写，不能证明没有读越界；ASan 的一次通过也不替代主循环和尾部的范围推导。

## 4. N5050 与 xsimd 的对应不是拼写替换

本课程的原生 C++26 固定对照是 [N5050](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5050.pdf)，命名空间是 `std::simd`，类型包括 `basic_vec<T,Abi>` 与 `vec<T,N>`。不要把旧 Parallelism TS 的 `std::experimental::simd` 成员 `copy_from/copy_to` 混入该接口，也不要将滚动 eel 中后续 C++29 修订当成本课程原生版本。

| 用途 | 本课 xsimd 13.2.0 | N5050 原生分支 |
|---|---|---|
| 四 float 类型 | `batch<float,sse2>` | `std::simd::vec<float,4>` |
| 读一个完整批次 | `V::load_unaligned(ptr)` | `std::simd::unchecked_load<V>(span)` |
| 写一个完整批次 | `v.store_unaligned(ptr)` | `std::simd::unchecked_store(v,span)` |
| 短尾 | scalar 或真实临时数组 | `partial_load/partial_store` |
| 条件选择 | `xsimd::select(mask,yes,no)` | `std::simd::select(mask,yes,no)` |
| 水平求和 | `xsimd::reduce_add(v)` | `std::simd::reduce(v)` |

原生代码整批传入 `.subspan(i,4)`，短尾传入 `.subspan(i)`。N5050 的 unchecked_load 即使提供 mask，也仍要求范围长度至少达到向量 size；不能把短 span 和 mask 交给 unchecked 形式来规避这个前提。partial 形式才按提供范围和掩码决定有效访问，其余加载通道值初始化，partial_store 仅写对应有效位置。

公共 CMake 通过 `CS_HAS_STD_SIMD` 决定是否启用原生代码。头文件存在或启用 `-std=c++2c` 本身不证明这些确切名字已实现；能力探测必须编译接口。当前本机原生接口缺失，所以原生分支标为未运行验证。xsimd 则通过 `#if CS_HAS_XSIMD` 包含，未安装时默认 C++23 基线和 SSE2 不受影响。

本课把 xsimd 明确固定为 sse2 架构，避免默认架构随编译选项悄悄升成 AVX。非 x64 平台在本课包装中跳过 xsimd 的这条 SSE2 分支；这不代表 xsimd 库本身不支持其他架构，只是这里没有为其他架构提供并验证对应目标。

## 5. select 表达了选择，不等于短路求值

K3 有三个操作：absolute、clamp 到 [-1,1]、relu。比较生成每通道真假掩码，select 根据该掩码选取两个已形成的候选值。C++ 函数实参仍会先求值，所以 `select(x!=0,1/x,0)` 不能当作保证“零通道绝不会求倒数”的短路 if；除零和浮点异常策略需要另行设计。

SSE2 选择可写成 `(mask & yes) | (~mask & no)`，这是 `conditional` 中 choose 的实现。clamp 先选下界，再选上界；relu 用 `x>0` 的有序比较保留正数，否则选 +0。它们不使用一组未经审查的 min/max 指令替代，因为 NaN 操作数顺序和有符号零选择可能让 min/max 与语言条件表达式不同。

absolute 要特别注意 -0。`x<0 ? -x : x` 会保留 -0，因为 -0<0 为假；如果本题合同是数学绝对值的 +0，就应清除符号位或用正确的 abs。SSE2 实现清除符号位，xsimd 使用 abs，原生分支对零明确选择 +0。NaN 只承诺分类，不承诺 payload 或符号位；本题使用 quiet NaN，不研究 signaling NaN 的陷阱与异常标志。

这些行为的完整表格及检查见[下一篇](03-reductions-and-precision.md)，不能只在有限正数输入上通过后就声称条件代码完全等价。

## 6. gather/scatter 先定义索引合同

连续 load 的地址由 i 决定；gather 则由 index[i] 选择 source 元素，可能散布于多条缓存行。`gather` 先检查整个 index 范围，再读取，允许多个索引读同一元素。`scatter_unique` 按 index 写输出，**拒绝重复索引**，并且在任何写入之前检查越界和重复。

拒绝重复是课程选择，使每个输出位置最多有一个贡献。否则必须定义“最后一个”指哪个顺序，或改为求和/原子更新，随后处理舍入和冲突。不能把几个向量通道同时写同一地址的结果当作稳定的 scatter 语义。

参考输入 `{10,20,30,40}`、索引 `{3,0,2,1}`，gather 得 `{40,10,30,20}`，scatter 得 `{20,40,30,10}`。重复 `{0,1,0,3}` 在 gather 中合法，在 scatter 中抛错，输出保持原值。索引越界也在完整预检阶段失败。

SSE2 没有本例需要的通用硬件 gather/scatter 指令。因此 `gather_add_sse2` 明确先调用标量索引加载，再用真实向量加法处理 gathered 数组；绝不将这一步标成硬件 gather。N5050 还提供相应内存置换接口，查 `[simd.permute.memory]`；当前包装使用可检查的标量索引基线，并不宣称已经验证所有原生置换 API。

索引随机分散时，每条通道可能触及不同缓存行；一次 gather 指令也无法让这些缓存行凭空变成一次连续读取。优先问能否重排数据或索引、复用同一块数据，之后再研究更宽 ISA。

## 自测与完整答案

**n=5、W=4 时加载两批再屏蔽末尾三项合法吗？** 不合法，第二批访问超出对象。默认处理一批加一项；临时数组方案或原生 partial 形式才提供合法短尾。

**unaligned load 能读未构造对象吗？** 不能，它只放宽 SIMD 对齐前提，不放宽对象寿命与范围。

**为什么 clamp 没直接写 min(max(x,lo),hi)？** 需要先核对 NaN 与零符号语义。本课用两次选择表达已确定的合同，并逐特殊值检查。

**scatter 重复索引为什么在写入前拒绝？** 避免一半结果写回后才报错，也不依赖通道覆盖顺序。真要聚合重复贡献，应提供另一份明确的归约协议。

**N5050 unchecked_load 带 mask 能接受三元素范围给 vec<float,4> 吗？** 不能。范围长度前提仍成立，应使用 partial_load。

## 规范与实现资料

- [N5050](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5050.pdf)：`[simd.syn]`、`[simd.loadstore]`、`[simd.select]`、`[simd.permute.memory]`。
- [xsimd 官方类型文档](https://xsimd.readthedocs.io/en/latest/api/batch_index.html)和[选择操作文档](https://xsimd.readthedocs.io/en/latest/api/batch_manip.html)。本课编译版本固定 13.2.0，latest 页面仅辅助理解，最终以该版本源码和编译结果核对。
- [Intel 手册](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html)：MOVUPS/MOVAPS、ADDPS、CMPPS 等指令前提与行为。
