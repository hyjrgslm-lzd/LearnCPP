# 练习 H6：cute_layout_and_tensor

## 1. 目标

`[H6-T01]` (main.cu:3) 练习 H6：cuTe Layout + Tensor + Copy_Atom + MMA_Atom mini-GEMM。
`[H6-T02]` (main.cu:5) 目标：学习 cuTe 四大概念（Layout、Tensor、Copy_Atom、MMA_Atom），手写一个 mini-GEMM kernel（不借助 Collective 层），体验如何用类型级 layout 抽象消除手工索引。对比 cuTe 版本与手工索引版本的代码行数与可读性。
`[H6-T03]` (main.cu:11) 编译要求：sm_80+，CUDA 13.x，CUTLASS 3.x headers（cute/ 子目录）。
`[H6-T04]` (main.cu:12) 依赖：`cutlass::headers`（由 CutlassSetup.cmake 提供）。

学习 cuTe 的四大核心概念（Layout、Tensor、Copy_Atom、MMA_Atom），用它们手写一个 mini-GEMM kernel（不借助 Collective 层），体验如何用类型级 layout 抽象消除复杂的手工索引。对比同样功能的手写 indexing 版本，感受 cuTe 的清晰性与零开销抽象。

## 2. 前置知识

- 完成 H1-H5
- 理解 shape、stride、layout 作为数据结构的概念
- 对 C++ template 元编程有基础认识
- CUTLASS 3.x headers 可用（cute/ 子目录）

## 3. 硬件要求

- sm_80+（MMA_Atom 的硬件支持）
- sm_90a（使用 SM90 系列 MMA_Atom 时）
- CUTLASS 3.x headers（由 `cutlass::headers` 别名提供）
- CUDA Toolkit 13.x+

## 4. 文件说明

| 文件 | 说明 |
|------|------|
| `main.cu` | 手工索引基线 + cuTe mini-GEMM + layout 演示 |
| `CMakeLists.txt` | CUDA_ARCHITECTURES = 80;86;89;90a，链接 `cutlass::headers` |
| `README.md` | 本文件 |

## 5. 必做任务

### 步骤 1 — cuTe Layout 基础

`[H6-T05]` (main.cu:17) cuTe 核心头文件。
`[H6-T06]` (main.cu:25) 练习公共头文件。
`[H6-T07]` (main.cu:43) 超参。
`[H6-T08]` (main.cu:44) `BM`：block tile M。
`[H6-T09]` (main.cu:45) `BN`：block tile N。
`[H6-T10]` (main.cu:46) `BK`：block tile K。

`cute::Layout = Shape × Stride`，描述多维索引到一维地址的映射。

```cpp
// 16×32 行主矩阵：stride = (32, 1)
auto layout_rm = make_layout(make_shape(16, 32), make_stride(32, 1));

// 访问 (i, j) 位置的线性地址 = i * 32 + j * 1
assert(layout_rm(3, 5) == 3 * 32 + 5);  // = 101

// 打印 layout（调试用）
print_layout(layout_rm);
```

编译期 shape（`Int<N>{}`）比运行时 int 效率更高：

```cpp
auto layout_ct = make_layout(
    make_shape(Int<16>{}, Int<32>{}),
    make_stride(Int<32>{}, Int<1>{}));
```

### 步骤 2 — cuTe Tensor

`[H6-T11]` (main.cu:51) 版本 1：手工索引 GEMM（基线，不使用 cuTe）。
`[H6-T12]` (main.cu:67) TODO [必做] 步骤 6（对比基线）：手工索引的 tiled GEMM。
`[H6-T13]` (main.cu:75) stub。
`[H6-T14]` (main.cu:78) 版本 2：cuTe mini-GEMM（核心学习目标）。
`[H6-T15]` (main.cu:97) `gA_ptr`：`[M x K]` 行主。
`[H6-T16]` (main.cu:98) `gB_ptr`：`[K x N]` 行主。
`[H6-T17]` (main.cu:99) `gC_ptr`：`[M x N]` 行主。
`[H6-T21]` (main.cu:138) TODO [必做] 步骤 2：Tensor 访问演示（调试用）。

`cute::Tensor = 指针 + Layout`，封装任意维度张量访问。

```cpp
// 包装 global memory 指针
auto gA = make_tensor(
    make_gmem_ptr(A_ptr),
    make_layout(make_shape(M, K), make_stride(K, 1)));

// 访问 (row, col) = A_ptr[row * K + col]
float val = gA(make_coord(row, col));

// 包装 shared memory 指针
__shared__ __half smem[BM * BK];
auto sA = make_tensor(
    make_smem_ptr(smem),
    make_layout(make_shape(Int<BM>{}, Int<BK>{}),
                make_stride(Int<BK>{}, Int<1>{})));
```

### 步骤 3 — mini-GEMM kernel（核心）

`[H6-T18]` (main.cu:104) TODO [必做] 步骤 1：创建 global tensor。
`[H6-T19]` (main.cu:117) TODO [必做] 步骤 1（续）：切出当前 block 的 tile。
`[H6-T20]` (main.cu:125) TODO [必做] 步骤 1（续）：shared memory tensor。
`[H6-T22]` (main.cu:146) TODO [必做] 步骤 3：mini-GEMM kernel。
`[H6-T23]` (main.cu:179) stub：产生零输出。

```cpp
// 1. 创建 global tensor
auto gA = make_tensor(make_gmem_ptr(A), make_layout(make_shape(M, K), make_stride(K, 1)));
auto gB = make_tensor(make_gmem_ptr(B), make_layout(make_shape(K, N), make_stride(N, 1)));
auto gC = make_tensor(make_gmem_ptr(C), make_layout(make_shape(M, N), make_stride(N, 1)));

// 2. 切出当前 block 的 tile（local_tile）
auto blkA = local_tile(gA, make_shape(Int<BM>{}, Int<BK>{}), make_coord(blockIdx.y, _));
auto blkB = local_tile(gB, make_shape(Int<BK>{}, Int<BN>{}), make_coord(_, blockIdx.x));
auto blkC = local_tile(gC, make_shape(Int<BM>{}, Int<BN>{}), make_coord(blockIdx.y, blockIdx.x));

// 3. 创建 smem tensor（带 swizzle 消除 bank conflict）
auto sA = make_tensor(make_smem_ptr(smem_A), make_layout(...));

// 4. K tile 循环
for (int k = 0; k < size<2>(blkA); ++k) {
    // 搬运 global → smem
    copy(blkA(_, _, k), sA);  // 简化写法；实际需要分区
    __syncthreads();
    // 计算（gemm 函数）
    gemm(thr_mma, tAsA, tBsB, tCrC);
    __syncthreads();
}

// 5. 写回 global C
copy(tCrC, blkC);
```

### 步骤 4 — Copy_Atom 改写数据加载

`[H6-T24]` (main.cu:187) TODO [必做] 步骤 4：Copy_Atom 改写数据加载。

```cpp
// sm_80 cp.async（128-bit = 8×FP16）
using CopyAtom = Copy_Atom<SM80_CP_ASYNC_CACHEGLOBAL<sizeof(__half) * 8>, __half>;
TiledCopy tiled_copy = make_tiled_copy(
    CopyAtom{},
    Layout<Shape<_16, _2>, Stride<_2, _1>>{},  // 线程 layout
    Layout<Shape<_1,  _8>>{}                    // 每次搬运 8 元素
);
auto thr_copy = tiled_copy.get_slice(threadIdx.x);
auto tAgA = thr_copy.partition_S(blkA(_, _, k_tile));
auto tAsA = thr_copy.partition_D(sA);
copy(tiled_copy, tAgA, tAsA);
```

### 步骤 5 — MMA_Atom 改写计算

`[H6-T25]` (main.cu:196) TODO [必做] 步骤 5：MMA_Atom 改写计算。

```cpp
// sm_80 FP16 Tensor Core：m16n8k16，FP16 输入，FP32 累加
using MmaAtom = MMA_Atom<SM80_16x8x16_F32F16F16F32_TN>;
TiledMma tiled_mma = make_tiled_mma(
    MmaAtom{},
    Layout<Shape<_1, _1, _1>>{},   // warp layout（1 warp）
    Tile<_16, _16, _16>{}          // value tile
);
auto thr_mma = tiled_mma.get_slice(threadIdx.x);
auto tCsA = thr_mma.partition_A(sA);
auto tCsB = thr_mma.partition_B(sB);
auto tCrC = thr_mma.partition_fragment_C(blkC);
gemm(tiled_mma, tCsA, tCsB, tCrC);
```

### 步骤 6 — 代码对比

`[H6-T43]` (main.cu:330) TODO [必做] 步骤 6：代码对比统计。

| 版本 | 索引计算行数（估算） | 可读性 |
|------|---------------------|--------|
| 手工索引（H1 风格） | ~40 行 | 低（需理解 stride 公式）|
| cuTe 版本 | ~15-20 行 | 高（类型描述，无显式索引）|

## 6. 进阶任务

`[H6-T44]` (main.cu:336) TODO [进阶] 用 cuTe 实现 split-K GEMM。
`[H6-T45]` (main.cu:340) TODO [进阶] 对比 cuTe 手写版与 CUTLASS Collective 自动生成版的 PTX/SASS。

- 用 cuTe 实现 split-K GEMM
- 对比 cuTe 手写版与 CUTLASS Collective 自动生成版的 PTX/SASS
- 尝试 Swizzle layout 消除 bank conflict，观察 Nsight Compute 中 bank conflict 计数变化

## 7. 验收标准

- mini-GEMM kernel 编译通过（需 CUTLASS headers 与 CUDA 13.x）
- 结果与参考实现逐元素一致（容差 1e-2）
- cuTe 版本的索引计算行数相比手工索引少 30-50%
- `print_layout` 输出正确映射手工验证（host 端调试）

## 8. 常见坑

| 坑 | 说明 |
|----|------|
| Layout rank 不匹配 | shape 与 stride 的维度数必须相等，否则编译报错 |
| swizzle 参数错误 | `Swizzle<B, M, S>` 参数不当会增加 bank conflict |
| Copy_Atom / MMA_Atom 类型不匹配 | Atom 的元素类型必须与 Tensor 的元素类型一致 |
| Tensor 生命周期 | smem Tensor 基于 `__shared__` 指针，kernel 结束后失效 |
| 编译期 shape 与运行时 shape 混用 | `local_tile` 的 tile shape 建议全用编译期 `Int<N>{}` |

## 9. cuTe 四大概念速查

```
Layout     = Shape × Stride    描述索引→地址映射（纯类型，零运行时开销）
Tensor     = gmem/smem ptr + Layout  封装多维访问，operator() 自动计算地址
Copy_Atom  = 硬件最小搬运单元（ldmatrix / cp.async / TMA）
MMA_Atom   = 硬件最小乘加单元（m16n8k16 / wgmma / etc.）
```

## 10. 复盘问题

1. cuTe Layout 相比手工索引计算的优势是什么？
2. Copy_Atom 与 MMA_Atom 如何组合成完整的 kernel？
3. swizzle 对 Layout 的影响是什么，如何选择 `Swizzle<B, M, S>` 的参数？

## 参考资料

- CUTLASS GitHub: `media/docs/cute/` 全系列文档
- CUTLASS `media/docs/cute/00_quickstart.md`
- CUTLASS `examples/cute_*` 系列示例
- NVIDIA Blog “CUTLASS 3.x and CuTe Deep Dive”

## 输出对照（printf / std::puts 原文）

- `[H6-T26]` (main.cu:208) CPU 参考实现。
- `[H6-T27]` (main.cu:245) cuTe Layout 演示（host 端，编译期）。
- `[H6-T28]` (main.cu:253) 原文：`── cuTe Layout 演示（host 端）──...` → 现：`-- cuTe Layout demo (host) --`
- `[H6-T29]` (main.cu:255) TODO [必做] 步骤 1（续）：在此取消注释以观察 layout 打印结果。
- `[H6-T30]` (main.cu:272) 原文：`(取消注释 demo_cute_layouts 中的代码可观察 layout 打印)` → 现：`(uncomment the body of demo_cute_layouts to see printed layouts)`
- `[H6-T31]` (main.cu:276) `main` 入口。
- `[H6-T32]` (main.cu:285) Layout 演示（纯 host 端）。
- `[H6-T33]` (main.cu:289) 原文：`问题规模：M=%d  N=%d  K=%d` → 现：`Problem size: M=%d  N=%d  K=%d`
- `[H6-T34]` (main.cu:292) 主机内存。
- `[H6-T35]` (main.cu:301) 原文：`正在计算 CPU 参考...` → 现：`Computing CPU reference...`
- `[H6-T36]` (main.cu:304) 设备内存。
- `[H6-T37]` (main.cu:319) 手工索引基线。
- `[H6-T38]` (main.cu:332) cuTe 版本。
- `[H6-T39]` (main.cu:345) 正确性检查。
- `[H6-T40]` (main.cu:347) 原文：`── 正确性检查（stub 阶段预期 FAIL）──` → 现：`-- Correctness check (FAIL expected at stub stage) --`
- `[H6-T41]` (main.cu:352) 性能汇总。
- `[H6-T42]` (main.cu:355) 原文：`── 性能汇总 ──...` → 现：`-- Performance summary --`
- `[H6-T46]` (main.cu:355) 原文：`[H6] 完成。` → 现：`[H6] done.`
