# L07：SIMD 与执行策略观察

本题是 C13 的小型 consumer：直接调用 C08 的 `concurrency_study/simd_kernels.hpp` 和 `numeric_kernels.hpp`，用独立解析预期检查标量、SSE2、可选 xsimd、可选原生 `<simd>` 和 `std::execution`。通过只说明接口、尾部和 oracle 正常，不代表任何后端性能优越。

```powershell
cmake -S C13_Performance_Numerics_Data_Layout/exercises/L07_simd_parallel -B build/c13-l07 -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/c13-l07
ctest --test-dir build/c13-l07 --output-on-failure
.\build\c13-l07\C13_L07_simd_parallel.exe --size 257
```

`threads=0` 表示 `std::execution` 的实际 worker 数没有观测。`xsimd`、`std_simd` 和 parallel algorithms 缺能力时输出 SKIP；这不是失败，也不能改写成通过。
