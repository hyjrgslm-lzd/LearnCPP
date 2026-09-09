# SIMD 01：先把一个元素算对，再研究一批

本篇从 [numeric_kernels.hpp](../../exercises/include/concurrency_study/numeric_kernels.hpp) 的 `add_scalar` 开始。它是算法的普通循环基线，**不是保证没有向量指令的机器码**。完整入门 Reference 在 [K1](../../exercises/K1_simd_basics/solution.cpp)，综合数值检查在 [numeric_test.cpp](../../exercises/runtime_tests/numeric_test.cpp)。先完成[测量先修](../../chapters/12-measurement.md)，再比较性能。

## 1. 同时处理多个元素，需要什么独立性

任务是对两个长度相同的数组计算 `out[i]=a[i]+b[i]`。单个结果仅依赖同位置的两个输入，所以若 out 不覆盖任何输入，就没有跨迭代数据依赖。第 i 项和第 i+1 项交换执行顺序不会改变彼此的输入。

```cpp
inline void add_scalar(input a, input b, output out) {
    binary_shape(a, b, out);
    for (std::size_t i = 0; i < a.size(); ++i)
        out[i] = a[i] + b[i];
}
```

这就是实际函数的完整结构。binary_shape 检查三段长度，并拒绝输出与任意输入重叠；输入 a 和 b 互相重叠不构成问题，因为它们都是只读。空数组是合法操作，不访问任何元素。

为什么课程暂不支持原地写？若 out 恰好等于 a，逐位置更新可以设计成安全；但若 out 从 a+1 开始，第一轮覆盖的值可能是下一轮输入。标量从左到右执行和一次加载四个值会得到不同效果。把部分重叠排除在入口之外，能够给所有实现一份统一合同。需要原地接口时，应显式另加“完全相同起点允许、部分重叠拒绝”的规则及测试，不能把本版本默默扩大。

入口检查只能验证调用者提供的跨度之间的关系，不能验证悬空指针、虚假的跨度长度或已经结束的对象寿命。span 是一段借用视图，不会延长原数组寿命。数组必须存活到调用结束；若交给 worker，还必须存活到 join。

## 2. oracle 的职责是给答案，基线的职责是供比较

如果 scalar 和 SIMD 都把第七项漏掉，二者输出可能一致，仍然一起出错。因此检查器应尽量独立。K1 输入 `a[i]=i`、`b[i]=2i+1`，直接使用解析式 `3i+1` 检查；这些小整数恰好可表示，不需要把容差设成很宽来“兼容”错误。

长数组基准输入 `a[i]=i%17`、`b[i]=0.5`，期望值为 `(i%17)+0.5`。测试不从被测函数拷贝实现，而是在计时外逐个验证这个固定关系。异常或不一致导致失败退出。结果被完整读取，也帮助确保被测计算有可观察用途。

通用实数输入的 oracle 要处理自身精度。Windows MSVC 的 long double 可能与 double 具有相同精度，所以换个类型名字不足以制造高精度真值。点积采用补偿求和的独立实现，并配合解析输入；详见[精度篇](03-reductions-and-precision.md)。本篇加法不进行长归约，在严格配置、相同舍入环境下可以按每个运算的策略检查普通结果、NaN 分类和零符号。

## 3. 自动向量化首先受数据流约束

源代码里的普通循环并不要求编译器逐条生成 scalar 指令。它可能证明没有危险依赖，或在运行时增加别名检查，然后生成“批量主循环+短尾循环”。因此真实对照有三层：普通 C++ 循环、诊断确认的自动向量化版本、显式 SIMD 版本。三层可能最后生成非常相似的机器码。

循环中每轮调用不可见的外部函数、通过复杂指针追逐数据、读取前一轮刚写出的状态，都可能使自动向量化更难。例如 `x[i]=x[i-1]+a[i]` 有真实前缀依赖，不能因为写上编译器提示就变成独立映射；应该先选择 scan 算法。非标准 `restrict` 或 assume_aligned 也不能用来“修复”不成立的前提，向编译器作假的承诺可能产生未定义行为。

`map_value` 使用 `x*x+2`，各项独立；dot_scalar 的 sum 则有长依赖链。严格浮点配置下，编译器可能自动向量化前者，却不能任意改变后者的加法关联。这两种循环不能仅凭“都是 for”预测同样优化结果。

## 4. AoS 与 SoA：改变的是数据的邻接关系

考虑粒子记录 `particle { float x,y,z,mass; }`，任务只更新 `x += dt*mass`。AoS（结构体数组）把同一个粒子的四个字段放在一起；沿粒子编号访问 x 时，地址步长为整个记录的 sizeof。SoA（数组的结构）则把所有 x 放在一个数组、所有 mass 放在另一个数组，对这次操作需要的两列都可连续读取。

实际代码有 `update_aos` 和 `update_soa`，它们完成相同的 x 更新，y、z 和 mass 保持不变。主测试给三个记录 `{x=1,mass=2}`、`{x=2,mass=4}`、`{x=3,mass=6}`，dt=0.5，解析 x 为 2、4、6。性能驱动 `simd_bench --variant aos` 与 `--variant soa` 使用同一组 x、mass 和 dt=1，在计时外检查 x 及 AoS 未修改字段。

本机常见的四 float 记录大小为 16 字节。只用其中 x 与 mass 时，AoS 可能把不需要的 y、z 一起带入缓存；SoA 更容易形成连续向量加载。这里描述的是机制假设。若另一操作同时使用同粒子的全部字段，AoS 的局部性反而可能合适，转换 SoA 还有创建数组、重排和维护一致性的成本。基准将数据准备排除在内核计时外，若应用每次都临时转换布局，必须另计转换成本。

更复杂场景可以考虑 AoSoA：把固定数量粒子组织成一个小块，块内按字段分列。它可兼顾某些通道访问与块级组织，但需要额外布局合同。本课程已有 AoS/SoA 实际对照足以回答当前问题，不为尚未出现的访问模式增加第三套接口。

## 5. 为什么更连续也不意味着无限加速

单个加法至少读取 a、b 两个 float 并写一个 float，逻辑流量是每元素 12 字节，算术量只有一个加法，算术强度约为 1/12 FLOP/byte。把一次指令扩成四个或八个通道，并没有减少总输入输出字节数。

当数组驻留 L1 时，比较的可能主要是加载端口、执行单元和循环指令；当数组超出末级缓存，内存带宽可能占主导。即便显式向量化提高了计算吞吐，整体时间也可能几乎不变。应该跨规模记录结果，而不是只挑一个最能展示 SIMD 的规模。

SoA 也有类似边界：它可能减少该操作触及的无关字段，但如果后续操作马上需要 y、z，整个流水线的总流量才是决策依据。优化目标应是完整应用的有效工作，而不是某个孤立循环的漂亮数字。

## 6. 最小实验与检查顺序

```powershell
# 工作目录 C08_Concurrency/exercises
cmake -S K1_simd_basics -B build/k1 -G "Visual Studio 18 2026" -A x64
cmake --build build/k1 --config Release
ctest --test-dir build/k1 -C Release --output-on-failure
```

先运行 main 的五元素 scalar 例子，预测输出 `{3,5,7,9,11}`；再运行 Reference 覆盖长度 0 到 35，验证普通循环与所有可用后端；最后才通过 benchmarks 目标运行 `simd_bench`。普通循环不需要 xsimd，缺可选库时基线依然完整执行。

AoS/SoA 是表示变化的对照，输入输出业务语义相同；strict 与 fast-math 则可能连数值合同也变，必须分开报告。显式向量和尾部安全进入[下一篇](02-explicit-vectors.md)。

## 自测与答案

**scalar 与 oracle 是一个东西吗？** 不是。scalar 是可测实现，oracle 是独立确定正确结果的方法。本课优先使用解析真值与补偿计算，不只让两个实现互相比。

**scalar 测得和 SSE2 一样快，说明 SSE2 没执行吗？** 不能这样推断。普通循环可能已自动向量化，或者两者都受访存限制；需要诊断与实际指令证据。

**给数组首地址对齐，AoS 的 x 字段就连续了吗？** 没有。对齐改变起点约束，数组内部 x 的步长仍是 sizeof(particle)。改变邻接关系需要改变数据布局。

**为什么本题拒绝 out=a+1？** 部分重叠会形成跨迭代依赖，标量与批量执行不再满足同一简单映射合同。入口在写入前拒绝。

## 官方资料

- [N5050](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5050.pdf)：`[intro.execution]`、`[views.span]`、`[simd]`。
- [Microsoft 自动向量化](https://learn.microsoft.com/en-us/cpp/parallel/auto-parallelization-and-auto-vectorization?view=msvc-170)和 [GCC 优化选项](https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html)：普通循环是否向量化由具体构建诊断确认。
