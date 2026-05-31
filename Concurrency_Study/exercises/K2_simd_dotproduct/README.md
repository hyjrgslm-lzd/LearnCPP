# 练习 K-2：SIMD 向量化点积

> 详尽版见 `../../14-模块K-数据并行与std-simd.md` 的 练习 K-2。

## 目标

把**点积（dot product）= Σ a[i]·b[i]** 向量化。比 K-1 多一个难点：结果是一个**标量**，而 SIMD 产出的是一**批通道值（lane）**。要学会**累加器向量化（vectorized accumulator）** + **水平归约（horizontal reduction）**，并用**相对容差**对照标量结果。

## 前置理解

- **累加器向量化**：用一个 `batch<float>` 当累加器 `acc`，主循环 `acc = acc + va*vb`（亦可 `xsimd::fma(va,vb,acc)`），相当于 `W` 路部分和并行累加、互不干扰。
- **水平归约**：循环结束后 `acc` 里是 `W` 个部分和，用 `xsimd::reduce_add(acc)` 横向加成一个标量（对应 `std::simd` 的 `reduce(v, std::plus<>{})`）。
- **尾部余数**：主循环外再用标量乘累加补齐不足一整批的元素。
- **浮点累加顺序**：SIMD 把求和分散到 `W` 条 lane 再归约，顺序与标量从左到右不同；浮点加法**不满足结合律**，故结果有**微小误差**。对照**必须用相对容差**，不能严格相等。

## 必做任务

1. `// TODO [必做 1]`：向量化乘累加主循环（向量累加器）。
2. `// TODO [必做 2]`：用 `reduce_add` 做水平归约成标量。
3. `// TODO [必做 3]`：标量补齐尾部余数；并与标量基准用相对容差比较、计时。

## 验收点

- SIMD 点积与标量点积的相对误差在容差内（本题 `1e-4`）。
- 能说清向量累加器、为什么最后要水平归约。
- 能解释为什么 SIMD 点积与标量点积结果**不应**严格相等（浮点结合律）。

## 对应官方参考

- 提案 [`P1928R15`](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p1928r15.pdf)（`std::simd::reduce`）
- cppreference [`<simd>`](https://en.cppreference.com/w/cpp/header/simd)
- [xsimd reduce 文档](https://xsimd.readthedocs.io/en/latest/api/reducer_index.html)

## 构建运行

```bash
cmake --build build-vs2026 --target K2_simd_dotproduct --config Release
./build-vs2026/K2_simd_dotproduct/Release/K2_simd_dotproduct.exe
```
