# 07 矩阵乘、分块与 linalg

先修：第 02 章的浮点归约、第 05 章的 `span/mdspan` 布局契约，以及基本缓存模型。目标不是写一个生产 BLAS，而是从朴素正确实现进入分块复用，知道每一步改变了什么，哪些结论需要实测。

配套 [L03 练习](../exercises/L03_tiled_matrix/README.md) 只实现 row-major `double` GEMM：`C = A * B`。公共接口位于 [matrix.hpp](../exercises/include/c13/matrix.hpp)。

## 先写朴素正确版本

矩阵乘的数学定义是：

```text
C[i,j] = sum(A[i,k] * B[k,j]) for k in [0, ac)
```

朴素三重循环直接表达这个定义：

```cpp
for (std::size_t i = 0; i < ar; ++i)
    for (std::size_t j = 0; j < bc; ++j)
        for (std::size_t k = 0; k < ac; ++k)
            c[i, j] += a[i, k] * b[k, j];
```

为了减少重复读取 `C[i,j]`，实际代码也常写成局部 `sum` 后一次赋值。这个版本的正确性最容易推导：每个输出元素只依赖 A 的一行和 B 的一列。它也是检查分块版本的基线。

## 为什么朴素版本可能慢

row-major 中，`A[i,k]` 随 k 连续，`B[k,j]` 随 k 跨行跳转。对大矩阵，B 的访问可能反复把同一缓存层挤出。三重循环的总乘加次数仍是 `ar * ac * bc`，分块并不改变算法复杂度；它试图改变的是数据在缓存里的复用方式。

仅凭这段分析不能宣称分块一定更快。矩阵尺寸、cache、编译器、CPU 预取、向量化和线程都会影响结果。C13 的原则是：先有正确基线，再做同口径测量；总耗时差异若要解释成 cache 或 SIMD，需要 profile、计数器或受控对照支持。

## 分块改变循环空间

分块把 i、k、j 三个维度切成小块：

```cpp
for (std::size_t ii = 0; ii < rows; ii += tile)
    for (std::size_t kk = 0; kk < inner; kk += tile)
        for (std::size_t jj = 0; jj < cols; jj += tile)
            // 在 [ii,i_end) x [kk,k_end) x [jj,j_end) 内计算
```

块内仍然执行同样的乘加，只是顺序不同。因为顺序不同，浮点结果可能和朴素版本有微小差异；L03 使用小整数输入和精确结果检查，避免容差掩盖尾部漏算。后续随机大矩阵比较时，应使用明确误差预算。

## 尾块必须计算

真实尺寸很少刚好整除 tile。若 `rows = 3`、`inner = 5`、`cols = 4`、`tile = 2`，最后一行、最后一个 k 块、最后一组列都不是完整块。业务契约是完整矩阵乘，不能把 tile 当作尺寸前提。

公共实现用剩余长度计算终点：

```cpp
const auto i_end = ii + std::min(tile, rows - ii);
```

循环递增也使用剩余长度，避免 `ii + tile` 在接近 `std::size_t` 上限时发生无符号溢出。shape 检查已经拒绝不可能的存储长度；循环仍不应依赖溢出绕回来结束。

## 写入前拒绝错误输入

矩阵乘有几个必须先检查的条件：

- `ac == br`，否则内维不匹配。
- `a.size() == ar * ac`，`b.size() == br * bc`，输出大小等于 `ar * bc`。
- 这些乘法不能溢出 `std::size_t`。
- `tile > 0`，否则分块循环无法推进。
- 输出不能与输入重叠，除非另有 in-place 算法契约。

这些检查不是调试断言，Release 也要执行。错误输入若已经写了一半再抛异常，调用者会得到难以恢复的半成品。C13 的公共写入接口先完成所有拒绝条件，再清零输出并计算。

## linalg 与本课边界

C++26 已包含 `linalg` 相关工作，Kokkos stdBLAS 提供可读的参考实现入口。生产项目应优先考虑成熟 BLAS、Eigen、oneMKL、cuBLAS 或领域库，而不是把 L03 教学 GEMM 当基础设施。

本课实现仍有价值，因为它暴露了库接口也必须面对的契约：shape、layout、stride、别名、尾块、误差、执行资源和测量口径。读源码时，先找这些契约落在哪里，再看它如何把工作交给向量化、线程或设备后端。

## 练习映射

L03 的 Student 起点返回空向量。Reference 调用 `c13::matmul_tiled`，good 用独立朴素循环，bad 返回零输出。检查器先验证小矩阵精确结果，再验证 `3x5 * 5x4` 的非整块尾部，最后验证 shape、tile、溢出和重叠拒绝。

通过本章后，读者应该能说明：分块没有改变矩阵乘的数学定义；尾块为什么不能跳过；为什么无符号加法也要避免溢出；为什么教学实现与生产 linalg 库承担的范围不同。
