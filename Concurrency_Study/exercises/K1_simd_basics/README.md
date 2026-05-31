# 练习 K-1：std::simd 基础（xsimd 回退）

> 详尽版见 `../../14-模块K-数据并行与std-simd.md` 的 练习 K-1。

## 目标

建立**数据并行（data parallelism）/ SIMD（Single Instruction, Multiple Data，单指令多数据）**的心智模型：一条向量指令同时对一“批”（batch / lane，通道）相邻数据做同样的算术。用 `xsimd::batch<float>` 把“两数组逐元素相加”向量化，并正确处理**尾部余数（remainder / tail）**，再用标量结果做对照、粗略计时。

## 前置理解

- SIMD 是**线程内**的并行，与本课程前面的“多线程任务并行”正交：它在一个线程里把吞吐放大 `batch::size` 倍。
- 主循环以 `batch::size` 个元素为一步：`load_unaligned` 取一批 → `+` 相加 → `store_unaligned` 写回。
- 数组长度通常不是 batch 宽度的整数倍，**最后不足一整批的元素必须用标量循环补齐**，否则漏算或越界。
- 标准目标是 C++26 `std::simd`（`<simd>`，提案 `P1928R15`），MSVC 尚未实现；本题用 header-only 的 xsimd 作回退（`batch<T>` ↔ `basic_vec<T>`）。

## 必做任务

1. `// TODO [必做 1]`：向量化主循环，以 `batch::size` 为步长 load/add/store。
2. `// TODO [必做 2]`：用标量循环补齐尾部余数；并与标量版逐元素对照（加法无累加顺序问题，可严格相等）、计时。

## 验收点

- SIMD 结果与标量结果逐元素一致（不一致数 = 0）。
- 能说清 `batch::size`、主循环步长、为什么必须处理尾部余数。
- 能说清 SIMD（线程内数据并行）与多线程（任务并行）的区别。

## 对应官方参考

- 提案 [`P1928R15`](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p1928r15.pdf)（`std::simd`）
- cppreference [`<simd>`](https://en.cppreference.com/w/cpp/header/simd)
- [xsimd 文档](https://xsimd.readthedocs.io/en/latest/)

## 构建运行

```bash
cmake --build build-vs2026 --target K1_simd_basics --config Release
./build-vs2026/K1_simd_basics/Release/K1_simd_basics.exe
```
