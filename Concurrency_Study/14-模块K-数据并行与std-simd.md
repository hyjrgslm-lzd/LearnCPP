# 数据并行与 std::simd：迁移入口

本模块已迁移为 [15 SIMD](chapters/15-simd.md)，正文连续覆盖标量 oracle、AoS/SoA、自动诊断、显式向量、对齐、尾部、掩码、gather/scatter、水平归约、FMA、精度、ISA 回退与带宽。

- [K1 加法与安全尾部](exercises/K1_simd_basics/README.md)。
- [K2 点积与水平归约](exercises/K2_simd_dotproduct/README.md)。
- [K3 条件选择与特殊值](exercises/K3_simd_where_select/README.md)。

默认 C++23 不依赖 xsimd。x64 SSE2 提供真实显式实现，xsimd 按能力开启；原生 C++26 接口以 [N5050](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5050.pdf) 为固定基准，使用 std::simd::vec/basic_vec 与自由 load/store/select。旧 TS copy_from/copy_to/where 不再混写成这份标准接口。完整 API 对照见 [显式向量正文](topics/simd/02-explicit-vectors.md)。
