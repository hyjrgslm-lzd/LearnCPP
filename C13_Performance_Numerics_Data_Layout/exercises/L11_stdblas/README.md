# L11 stdBLAS 的真实 dot 与 matrix_product

先读 [矩阵与分块](../../chapters/07-matrix-tiling-linalg.md)、[源码与前沿](../../chapters/11-sources-and-frontier.md)。本目标使用固定 stdBLAS 与 Kokkos mdspan 参考实现，是独立可执行文件，不与原生 `<mdspan>` 混在同一翻译单元。

固定 stdBLAS 使用 `mdspan(i,j)`，因此目标启用上游 `MDSPAN_USE_PAREN_OPERATOR=1` 兼容开关。原生 C++23 主线仍使用 `view[i,j]`，两者不混淆。

从 `exercises` 开发终端运行：

```powershell
cmake --preset full-windows
cmake --build --preset full-windows --target c13_stdblas
ctest --preset full-windows -R '^c13_stdblas$'
```

单题可用 `-S L11_stdblas -B ../build/stdblas`，其余生成器及依赖开关与 L10 相同。

## 输入、契约和答案

`A` 是 2×3、`B` 是 3×2、输出是 2×2。按行优先解释固定数组，结果为 `[58,64;139,154]`。这组非方阵可暴露“误把另一矩阵的列数当行跨度”等错误。`C` 先填 -999，实际 `matrix_product` 必须覆盖它，不能沿用初始化零使漏写悄悄通过。

点积使用 `[1,2,3]` 与 `[4,5,6]`，实际 `dot(X,Y,0.0)` 得到 32。输出类型及初值进入归约语义，而不是只有两个输入向量的元素类型。

本例固定尺寸、不重叠、有限小值，满足算法前置条件。若将接口暴露给外部尺寸，应像 L03 一样先验证存储、形状和混叠；不能假设这份参考库会替外部输入完成所有运行时检查。

## 继续阅读

从 `include/experimental/linalg` 找到 `__p1673_bits/blas1_dot.hpp` 与 `blas3_matrix_product.hpp`。先识别输入类型、结果形状、默认执行策略映射，再找到 serial `inline_exec_t` 路径的实际循环。本课未启用 BLAS、TBB、Kokkos 后端；这证明真实接口可消费，不证明其性能达到专业 BLAS，也不证明本机具有原生 `<linalg>`。
