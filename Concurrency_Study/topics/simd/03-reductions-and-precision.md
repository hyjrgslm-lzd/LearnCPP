# SIMD 03：点积、水平归约和精度预算

加法数组有 N 个结果，点积只有一个结果：`sum(a[i]*b[i])`。向量化后，每个通道先产生部分和，最后还要合并。这个合并改变了加法关联，也让“结果与 scalar 完全相等”的直觉失效。本篇以 [K2/solution.cpp](../../exercises/K2_simd_dotproduct/solution.cpp)、[simd_kernels.hpp](../../exercises/include/concurrency_study/simd_kernels.hpp) 的 `dot_sse2/dot` 和 [numeric_test.cpp](../../exercises/runtime_tests/numeric_test.cpp) 为完整可运行材料。

## 1. 先拆开乘法精度与累加精度

输入为 float，不表示所有中间结果都必须是 float。`sum += double(a[i])*double(b[i])` 先提升再乘法；`sum += a[i]*b[i]` 则可能先得到 float 乘积。输入接近 float 最大有限值时，后者乘法可以溢出为 Inf，之后装进 double 也无济于事。

课程点积选择 binary32 输入、binary64 乘法与累加。在 IEEE binary32/binary64 环境中，每个有限 float 乘积的有效位数不超过 48，double 有 53 位，因此提升后乘积本身精确；两个 float 极端指数的乘积范围也能被 double 容纳。求和仍可能舍入，尤其当部分和相差很多数量级时。

`dot_scalar` 的普通顺序为从左到右累加。它是性能基线，不是数学实数的完全精确 oracle。本篇同时提供解析输入与独立补偿求和，避免在两个同样不精确的循环之间随意宣布一个为真值。

## 2. 四路部分和如何形成

SSE2 一次读四个 float。`_mm_cvtps_pd` 把低两个 float 转成两个 double；再通过 `_mm_movehl_ps` 取高两个转换。两个 double 向量累加器分别保留四条逻辑通道的和：

- 第一组第 0、1 通道累计下标 0/4/8… 与 1/5/9…。
- 第二组第 0、1 通道累计下标 2/6/10… 与 3/7/11…。

循环结束后，两向量相加，再把最终两个 double 相加，才形成一个标量结果。水平归约放在循环外，避免每个批次都横向合并并重新形成长依赖链；最后不足四项继续在 double 中累加。

```cpp
_mm_store_pd(lanes, _mm_add_pd(even, odd));
double sum = lanes[0] + lanes[1];
for (; i < a.size(); ++i)
    sum += double(a[i]) * double(b[i]);
```

这段真实代码表明，使用显式 SIMD 不一定意味着以 float 精度累加。xsimd 分支使用 `batch<double,sse2>`，先转换加载 float，再执行 double 运算和 `reduce_add`；原生 N5050 分支使用 `vec<double,4>` 与自由加载、reduce。不同后端的通道数和归约树可能不同，因此合同是数值误差要求，而不是硬编码某一棵树的比特结果。

## 3. 解析输入怎样揭示漏项和尾部

K2 使用 `a[i]=i+1`、`b[i]=0.5`，n 项点积为 `n*(n+1)/4`。测试 n 从 0 到 67，覆盖空输入、短于通道数、完整批次和各种余数。这些数都能精确表示，在该测试域可以要求严格相等。如果尾部漏掉三项，解析公式会立即失败，不依赖另一份循环也犯同样错误。

另一个输入 `{2^24,1,-2^24,0.5,-0.5}` 与全 1 相乘，结果为 1。若错误地用 float 顺序累计，1 很可能在大数处丢失；先提升 double 后，这组数据的各步都足够精确。主测试还包括大范围互逆幂次乘积和 float 最大有限值乘积，专门检查“先转 double 再乘”的事实。

这些检查能非常明确地定位类型提升、尾部和水平归约错误；但它们不会覆盖一般输入的舍入传播，所以还需要第二类测试。

## 4. 用运算量决定误差界，而不是拍一个相对容差

设真实点积为 s，乘积绝对值和为 `A=sum(abs(a_i*b_i))`，double 的单位舍入误差 `u=epsilon/2`。在舍入到最近、没有溢出/下溢破坏相对误差模型的条件下，一条最多 k 次相关舍入的计算路径，可以用 `gamma_k=k*u/(1-k*u)` 描述误差放大。顺序求和的经典尺度是 `gamma_(n-1)*A`；更平衡的树通常有更短路径，但不能只根据 SIMD 宽度就保证最优树。

主测试采用保守 k=n+8，并给 oracle 舍入再留一份预算：`2*gamma_k*A`。当 k*u 接近 1 时这类界不再有用，所以代码先检查 `k*u<0.5`。所有生成的测试输入保证乘积与相关和处于可用指数范围；不会把有 Inf 的输入交给这个有限误差公式。

为什么用 A 而不是 abs(s)？正负项可能抵消，使 s 很小，但运算过程中处理的大数并没有变小。比如 `1e20,-1e20,1`，两种关联得到 1 或 0；相对于真值 1 的误差很大，而相对于绝对项总量却很小。`abs(got-ref)/(abs(ref)+1e-12)` 里的 1e-12 也不是通用答案，它会把输入单位和应用容差藏在一个没有解释的常量中。

GEMM 的 float 累积另用每输出元素的 `sum(abs(A[i,k]*B[k,j]))` 和 float 的 u，取涵盖乘与加的保守步数。它不会套用 dot 的 double 精度预算。按运算区分误差模型，是“同一测试文件里有几种浮点算法”时必须保持的边界。

## 5. 独立 oracle 也需要审查

`compensated_dot` 使用 long double 乘积并进行 Neumaier 风格补偿。每次把当前部分和与新乘积相加，再根据两者量级，把这次加法丢失的低位估计累积到补偿项中，最后返回 s+c。它的更新结构不同于 SIMD 通道累加，也不同于 scalar 简单折叠。

MSVC 上 long double 可能没有额外有效位，因此代码与正文明确不依赖“long double 必定更宽”。float 乘积能精确容纳于 double 的前提帮助此 oracle，补偿又减轻长求和的舍入损失；解析测试则提供不依赖浮点累积的额外真值。如果输入范围扩展到 double 乘法、极长向量或强病态数据，需要重新评估 oracle，必要时使用多精度或精确累加器。

一个保守误差界很宽时，算法即使数学上符合界，也可能无法满足业务精度。课程检查只说明满足本课合同；应用如果要求误差小于某个物理单位或统计置信要求，还需将业务容差叠加进去，不能拿“浮点本来就不精确”掩盖失效。

## 6. FMA 改变的是舍入次数

K2 的精确构造是 `x=1+2^-13`，`y=1-2^-13`，`z=-1`。实数结果为 `(1+q)(1-q)-1=-q^2=-2^-26`。单独 float 乘法会先把 `1-2^-26` 舍入为 1，再减 1 得到 0；`std::fma(x,y,z)` 将乘加作为一次最终舍入，得到可精确表示的 `-2^-26`。

Reference 用一次 volatile float 暂存强制乘法中间结果写回 float，检查 separate=0 和 fma=-2^-26。这个 volatile 用途是教学舍入边界，不是基准防优化手段。`a*b+c` 在某些编译模式中可能融合，不能仅凭源码写了两个运算符就认为一定发生两次舍入。

FMA 在许多情况下改善精度，但结果与分开运算可能不同。更少的舍入也不保证所有实际输入的绝对误差都单调更小，因为误差之间也可能抵消。若应用要求与旧版本逐位相同，应将是否允许融合纳入合同。SSE2 基线没有 FMA 指令，本课显式点积和 GEMM 默认不会以高 ISA 指令偷偷替代；宽 ISA 构建要另外检查。

## 7. NaN、无穷、零和 subnormal 的政策

默认严格测试要求 IEEE binary32/binary64、正常舍入环境，并在 x64 检查 FTZ/DAZ 未开启。FTZ 将某些 subnormal 结果冲刷为零，DAZ 将某些 subnormal 输入当作零，二者会改变本课默认数值合同。

| 输入 | absolute | clamp [-1,1] | relu `x>0?x:+0` |
|---|---|---|---|
| quiet NaN | NaN 分类 | NaN 分类 | +0 |
| +Inf / -Inf | +Inf | +1 / -1 | +Inf / +0 |
| -0 | +0 | -0 | +0 |
| +0 | +0 | +0 | +0 |
| 正/负最小 subnormal | 去符号 | 原值 | 正值 / +0 |

K3 的手写期望表和主测试都覆盖这些情况，比较零时额外检查 signbit；`-0==+0` 为真，单靠相等运算无法验证符号政策。NaN 只检查 isnan，不要求 payload 或符号位保持。输入不包含 signaling NaN，浮点异常标志也不属于当前跨后端一致性合同。

加法另检查 Inf 与相反 Inf 产生 NaN、同号 -0 的相加和 subnormal 相加。点积的非有限输入不使用有限误差界，只对选定输入检查分类；一般混有非有限项的归约树如何产生分类，需要按应用另定义规则。

## 8. 快数学是另一个构建合同

MSVC `/fp:fast`、GCC/Clang `-ffast-math` 是编译器提供的非标准浮点优化配置，不是 C++ 标准库 API。它们可能允许重结合、融合以及关于 NaN/Inf/零符号的假设；不同编译器包含的子选项也不同。不能把 strict 的特殊值测试全部跳过后，仍说 fast 满足相同合同。

快数学实验必须把**被测代码**与**判分代码**分开。Neumaier 补偿中的 `(sum-next)+term` 正是依赖舍入顺序的表达式；若整个测试文件都按 fast 编译，优化器可能把补偿项化简掉，还可能根据“不会出现 NaN”的假设改写比较。这样的 oracle 不能继续被视为可靠参照，即便程序打印了通过。

现在 `verify-numeric.ps1 -FastMath` 编译两个独立单元：[fast_math_kernel.cpp](fast_math_kernel.cpp) 只包装同一份生产加法、点积与 GEMM 内核，以 `/fp:fast /GL-` 编译；[fast_math_check.cpp](fast_math_check.cpp) 包含输入生成、独立 oracle、误差界和比较，以 `/fp:strict /GL-` 编译。两者只共享 [fast_math_api.hpp](fast_math_api.hpp) 中的声明，严格单元不包含生产内核的 inline 定义，避免链接器从两种浮点模式的同名定义中选择一个。链接使用 `/LTCG:OFF /OPT:NOICF`，不允许跨单元优化侵蚀这个边界。

这套专门检查限定有限输入，不把 NaN、Inf、signed-zero 或 subnormal 政策作为 fast 内核的承诺。数组测试覆盖 0/短/尾长到 1003，数据量级受限，点积用严格补偿及 gamma 界；矩阵覆盖 0/1/3/5/17、三种块宽，用严格逐元素误差界。额外解析抵消输入验证结果为 5。所有被写输出先填错误哨兵，边界保留 canary。

判分代码本身也受检：`{float(1e20),1,-float(1e20)}` 的补偿结果必须保留 1；比较器必须拒绝注入的 NaN、Inf 和超出误差界的值，接受边界值；跳过整个加法写入必须失败。检查单元若意外用 fast 编译，预定义宏守卫会报错。常规 `numeric_test.cpp` 同样拒绝 fast 编译，不再通过一个宏跳过特殊值后假称可靠 oracle。

本机执行 `verify-numeric.ps1 -FastMath`，或加 `-AddressSanitizer` 检查两侧对象的内存访问；`-XsimdInclude` 可启用固定版本库。日志逐项记录严格/fast 编译命令和链接选项，并保存两份汇编。GCC/Clang 的对应做法是严格单元 `-fno-fast-math -ffp-contract=off -fno-lto`、内核单元 `-ffast-math -fno-lto`，最终链接也不启用 LTO 或全程序 fast-math；本批次实际验证的是 MSVC，不能把这组建议选项标成其他编译器已测。

## 自测与答案

**SIMD 点积比 scalar 差一个 bit，一定错误吗？** 先看输入和合同。精确小整数测试应完全相等；一般有限输入按独立 oracle 与运算相关误差界检查。不能一律容忍，也不能一律拒绝。

**FMA 是否等价于先乘后加？** 实数代数等价，但舍入位置不同。上述构造在 float 中得到 0 与 -2^-26，是可运行的反例。

**为什么最小 subnormal 要单列？** 普通相对误差模型在下溢区域需要额外处理，FTZ/DAZ 又可能改变行为。专门的分类和精确小值测试能防止普通随机数据遗漏它们。

**long double 为什么还不够成为 oracle？** 它可能与 double 同精度，简单累加仍会舍入。本课用解析数据和独立补偿算法交叉检查，并承认扩展输入域时需要升级 oracle。

## 资料

- [N5050](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5050.pdf)：`[simd.reductions]`、`[c.math]`、数值算法相关条款。
- [Microsoft /fp 说明](https://learn.microsoft.com/en-us/cpp/build/reference/fp-specify-floating-point-behavior?view=msvc-170)：严格、精确与快速配置及融合选项的边界。
- [Clang 浮点模型](https://clang.llvm.org/docs/UsersManual.html#floating-point-model)、[GCC 浮点优化选项](https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html)：跨工具链不能凭选项名字推断完全相同的保证。
