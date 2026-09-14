# 08 SIMD 与并行边界

先修：[测量边界](01-measurement.md)、浮点误差预算、AoS/SoA 与矩阵 shape。C08 已经提供真实可运行的 `numeric_kernels.hpp` 与 `simd_kernels.hpp`；C13 不重写 ISA 探测和线程池，只从数值、布局和报告边界解释如何消费这些接口。

SIMD 和多线程都把多个操作排在一起执行，但层级不同。SIMD 是一个线程内的多个通道；`std::execution::par` 是算法实现管理的执行资源；手写线程则由程序显式创建 worker。它们可以组合，也会带来不同的正确性前提。

## 1. 标量合同先于向量宽度

以 `add(a,b,out)` 为例，合同包含：

- 三个 span 长度相同。
- 输出不与任一输入重叠。
- 每个元素都写一次，包括不足 SIMD 宽度的尾部。
- 非有限值和零符号按浮点加法自然传播。

显式 SSE2 一次处理四个 float：

```cpp
for (; a.size() - i >= 4; i += 4) {
    auto va = _mm_loadu_ps(a.data() + i);
    auto vb = _mm_loadu_ps(b.data() + i);
    _mm_storeu_ps(out.data() + i, _mm_add_ps(va, vb));
}
for (; i < a.size(); ++i)
    out[i] = a[i] + b[i];
```

第二个循环不是“慢尾巴可以忽略”。业务长度是 `n`，不是四的倍数。L07 默认用 `257` 个元素，就是为了让漏尾部的实现失败。SSE2 的 unaligned load 只放宽 16 字节对齐，不放宽对象范围；多读一个 float 再用 mask 丢掉结果仍然越界。C08 的 `add_sse2_tail_mask` 先把 live tail 复制到四元素临时数组，再做向量操作，这是安全掩码演示，不是更快承诺。

## 2. 对齐是调用前提，不是优化器许愿

`_mm_load_ps` 要求 16 字节对齐。若数组首地址偏移一个 float，即使长度足够，也不能调用 aligned 版本。当前接口只有在后端为 SSE2 且三段地址都满足对齐时才允许 `aligned=true`。

`std::assume_aligned` 或编译器内建 assume 只是把前提交给优化器。它不会重新分配内存，也不会在运行时修正错位地址。课程代码先检查地址，再调用 aligned 指令；性能实验如果去掉检查，就必须由上游分配器、类型或协议证明同一前提。

## 3. xsimd 与原生 `<simd>` 的边界

C08 的可选 xsimd 分支固定为 `xsimd::batch<float, xsimd::sse2>`，避免编译选项悄悄把宽度提升到 AVX 后和 SSE2 结果混排。未启用 xsimd 时，默认 C++23 基线仍可运行。

原生 `<simd>` 由 `CS_HAS_STD_SIMD` 控制。本课把它当作独立能力：头文件存在、编译器支持 `-std:c++latest`、第三方 xsimd 可用，都不能证明 N5050 里的原生名字已经实现。若宏为 0，程序报告 SKIP，而不是把 xsimd 成功记成标准库成功。

## 4. 点积不是逐元素加法

点积把 `n` 个乘积归约成一个数。向量化后会形成多个通道部分和，最后水平归约；这改变加法括号。C08 的 `dot_sse2` 仍把 float 提升到 double 后乘法和累加，这能避免 float 乘积溢出，但求和顺序仍可能与标量不同。

L07 使用两组检查：

- `a[i]=i%17`、`b[i]=0.5`，解析真值为每 17 个元素 68，覆盖尾部和漏项。
- 正常测量行只输出当前观察，不声称某个归约顺序数值更优。

如果输入包含强抵消、NaN、Inf、subnormal 或业务单位容差，应回到浮点章节建立误差预算。不能用“SIMD 允许一点误差”替代合同。

## 5. `std::execution` 不是线程数 API

`std::execution::seq` 要求顺序策略；`unseq` 允许向量化和无序交错；`par` 允许并行；`par_unseq` 同时允许并行和向量化风格的无序执行。它们改变的是算法允许的执行方式和异常/副作用边界，不是“启动 N 个线程”的命令。

元素函数必须独立。不能在 `par_unseq` 的 lambda 里写共享计数器、做无同步日志、依赖访问顺序或抛出异常后期待和普通循环一样传播。浮点归约还会改变括号，因此 `reduce` 结果要用误差预算或稳定算法验证。

报告 `std::execution` 时：

| 字段 | 写法 |
|---|---|
| `threads` | 未观测实际线程数时写 `0` |
| `details` | 说明 `implementation-managed; actual worker count not observed` |
| 结论 | 只比较同输入同窗口的观察，不拿硬件线程数当证据 |

若确实要观测线程数，可以在独立实验中记录 thread id 集合；这会给元素函数增加同步或线程本地记录，已经是另一组工作负载。

## 6. 运行 L07

```powershell
cmake -S C13_Performance_Numerics_Data_Layout/exercises/L07_simd_parallel -B build/c13-l07 -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/c13-l07
ctest --test-dir build/c13-l07 --output-on-failure
.\build\c13-l07\C13_L07_simd_parallel.exe --size 257
```

默认输出 `simd_add`、`simd_dot`、`execution_map` 三组 CSV。`scalar`、`sse2`、`tail_mask` 在 x64 上应能运行；`xsimd` 和 `std_simd` 根据宏报告运行或 SKIP；执行策略在 `CS_HAS_PARALLEL_ALGORITHMS=1` 时运行，否则只保留 plain 基线。

这些行只说明接口消费、尾部、oracle 和计时边界正常。要写性能优越性，需要按测量章节做多进程样本，并把自动向量化报告或反汇编作为形态证据。

## 自测与解析

**n=257 为什么比 256 更适合短检查？** 257 会覆盖完整向量批次后的 1 个尾元素。只处理 `n/4*4` 的错误实现会失败。

**xsimd 可用就代表 `<simd>` 可用吗？** 不代表。二者是不同实现和接口，分别由 `CS_HAS_XSIMD` 与 `CS_HAS_STD_SIMD` 记录。

**`par` 比 plain 快，能写线程数等于 32 吗？** 不能。除非程序记录了实际参与线程，否则只知道实现可能并行。

**为什么点积不用逐位相同当通用条件？** 因为归约括号可能不同。解析小输入可以要求精确；一般输入要用运算相关误差预算。
