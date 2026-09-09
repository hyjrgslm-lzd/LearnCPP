# K2：宽精度点积与水平归约

本题 main.cpp 分类为 OBSERVATION：运行给定基线并进行本程序实际列出的观察/检查。退出成功只说明这些检查通过，不表示下面全部实现、推导或测量 Part 已完成；完整答案与更广检查见独立 solution.cpp。需要实现的 Part 请在自己的函数中完成后对照 Reference，不把运行答案视为完成作业。

完整连续正文：[课程正文](../../topics/simd/03-reductions-and-precision.md)。

main 的三项点积为 32；[solution.cpp](solution.cpp) 给出解析输入、尾部、抵消和 FMA 完整答案。普通循环与 SIMD 都采用 float 输入、double 乘法/累加；这份类型合同必须保持一致，才能比较时间与误差。

## 必做 Part 与答案

1. 在乘法前把两个输入提升为 double。double(a*b) 太晚，可能已经发生 float 溢出；double(a)*double(b) 才是本题要求。
2. 用通道部分和消除一条长依赖链，循环结束后水平归约。SSE2 从四 float 生成两组 double 通道；xsimd 使用 batch<double,sse2>；原生 N5050 使用 vec<double,4>。三种后端都是真实向量运算。
3. 覆盖 0..67 长度。输入 a[i]=i+1、b[i]=0.5，答案 n*(n+1)/4 可精确表示，直接严格检查，不用容差掩盖漏项。
4. 输入 2^24、1、-2^24、0.5、-0.5 与全 1，答案 1。此题验证提升精度，不把任意浮点误差都称为“正常末位差”。
5. 计算 (1+2^-13)(1-2^-13)-1。分开 float 舍入得到 0，std::fma 得到 -2^-26。Reference 使用中间 volatile float 固定一次舍入；这不是为性能循环添加 volatile。

## 更一般输入的验收

runtime_numeric_test 有独立补偿 oracle、大范围幂次、最大 float 乘积、抵消输入以及按 double 单位舍入误差构造的 gamma 界。接近零的目标用乘积绝对值和衡量误差尺度，不用一个统一相对容差。long double 在 MSVC 上不保证更宽，oracle 不依赖这个假设。

基准选择 dot_scalar/dot_sse2/dot_xsimd/dot_std_simd；每元素读取两个 float，逻辑读字节为 8n。scalar 源码是否自动向量化由诊断确认。快数学另开有限输入合同，不能用它验证本题严格 FMA/括号行为。

## 构建与运行

从 `C08_Concurrency/exercises` 执行：

```powershell
cmake -S K2_simd_dotproduct -B build/K2_simd_dotproduct -G "Visual Studio 18 2026" -A x64
cmake --build build/K2_simd_dotproduct --config Release
ctest --test-dir build/K2_simd_dotproduct -C Release --output-on-failure
```

C++ 默认 23，cs::check 在 Release 中保持有效。可选能力缺失不阻止普通基线；平台及标准事实的官方链接、完整推导见本题对应正文。
