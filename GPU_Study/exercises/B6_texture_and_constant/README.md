# 练习 B6：texture_and_constant

## 目标

`[B6-T01]` (main.cu:2) 练习 B6：texture_and_constant。
`[B6-T02]` (main.cu:3) 学会两个特化内存：`__constant__` 用于广播小数据（卷积核），纹理内存用于空间局部 2D 采样。
`[B6-T03]` (main.cu:7) 验收：constant 版卷积吞吐 > global 版（频繁访问同一核系数时）；纹理采样结果正确，双线性插值精度通过验收。

学会两个特化内存：`__constant__` 用于广播小数据（如卷积核），纹理内存用于空间局部性强的访问（如 2D 图像采样）。

## 前置理解

- 完成 B1–B5。
- 你知道 GPU 内存有多种类型，各有特化场景。

## 必做任务

`[B6-T06]` (main.cu:32) 常量内存：存储卷积核系数。
`[B6-T07]` (main.cu:34) TODO [必做] 步骤 1：在全局作用域声明 `__constant__` 变量。
`[B6-T08]` (main.cu:39) Kernel 1：用全局内存存卷积核（baseline）。
`[B6-T09]` (main.cu:41) TODO [必做] 步骤 4：实现 global memory 版本卷积对比。
`[B6-T11]` (main.cu:60) TODO [必做] `result += kernel_g[(ky+half)*KER_SZ+(kx+half)] * input[iy*width+ix];`。
`[B6-T12]` (main.cu:67) Kernel 2：用 `__constant__` 存卷积核（优化版）。
`[B6-T13]` (main.cu:69) TODO [必做] 步骤 2：实现 constant memory 版本卷积。
`[B6-T14]` (main.cu:81) TODO [必做] 与 global 版本相同，但用 `c_kernel[ky+half][kx+half]`。
`[B6-T15]` (main.cu:93) Kernel 3：纹理内存双线性插值采样。
`[B6-T16]` (main.cu:95) TODO [必做] 步骤 5：实现纹理采样 kernel。
`[B6-T18]` (main.cu:111) TODO [必做] `output[oy * out_w + ox] = tex2D<float>(texObj, u, v);`。
`[B6-T23]` (main.cu:153) TODO [必做] 步骤 1：用 `cudaMemcpyToSymbol` 把 h_kernel 复制到 c_kernel。
`[B6-T31]` (main.cu:200) TODO [必做] 步骤 5：创建 2D 纹理对象并采样。
`[B6-T40]` (main.cu:243) TODO [必做] 步骤 5：验证采样结果（与直接读取比较误差）。

1. 写一个卷积 kernel，需要一个 5×5 的卷积核。用 `__constant__` 存储核系数：

```cpp
// TODO [必做] __constant__ float c_kernel[5][5];
// TODO [必做] // 在 host 上初始化：
// TODO [必做] CUDA_CHECK(cudaMemcpyToSymbol(c_kernel, host_kernel, sizeof(host_kernel)));
// TODO [必做] // 在 kernel 中直接引用 c_kernel
```

2. 在 kernel 中，对每个输入像素点做卷积（需要访问周围 5×5 像素）：

```cpp
// TODO [必做] float result = 0.0f;
// TODO [必做] for (int i = -2; i <= 2; i++) {
//     for (int j = -2; j <= 2; j++) {
//         result += c_kernel[i+2][j+2] * input[(y+i)*width + (x+j)];
//     }
// }
```

3. 计算吞吐，记录性能。
4. 写一个对比版本，把卷积核存在 global memory，对比吞吐。constant 版本应该因为广播而更快。
5. 用 `cudaTextureObject_t` 创建一个纹理对象，用于 2D 采样：

```cpp
// TODO [必做] cudaChannelFormatDesc channelDesc = cudaCreateChannelDesc<float>();
// TODO [必做] cudaArray_t cu_array;
// TODO [必做] CUDA_CHECK(cudaMallocArray(&cu_array, &channelDesc, width, height));
// TODO [必做] // 创建纹理对象
// TODO [必做] struct cudaResourceDesc resDesc = {};
// TODO [必做] resDesc.res.array.array = cu_array;
// TODO [必做] cudaTextureObject_t texObj;
// TODO [必做] CUDA_CHECK(cudaCreateTextureObject(&texObj, &resDesc, &texDesc, NULL));
// TODO [必做] // 在 kernel 中用 tex2D<float>(texObj, u, v) 采样
```

6. 用纹理内存做双线性插值采样，对比全局内存采样的吞吐。

## 进阶任务

`[B6-T44]` (main.cu:282) TODO [进阶] 用常数内存存一个 LUT 表，加速查表计算。
`[B6-T45]` (main.cu:284) TODO [进阶] 在纹理采样中试用 border/repeat 模式。
`[B6-T46]` (main.cu:286) TODO [进阶] 对比 `cudaTextureObject_t` 和旧 texture reference API。

- 用常数内存存储一个 LUT（查表表），加速某个计算。
- 在纹理采样中加入 edge wrap modes（border / clamp / repeat），观察性能差异。
- 对比 `cudaTextureObject_t` 和老的 texture reference API（已废弃，但代码库中可能存在）。

## 验收点

`[B6-T04]` (main.cu:23) 常量段。
`[B6-T05]` (main.cu:25) KER_SZ = 5x5 卷积核。
`[B6-T10]` (main.cu:55) 边界夹紧。
`[B6-T17]` (main.cu:106) 归一化坐标 [0,1]，CUDA 纹理归一化坐标中心在像素中心。
`[B6-T19]` (main.cu:117) 计时辅助。
`[B6-T20]` (main.cu:131) main 入口。
`[B6-T21]` (main.cu:140) 初始化卷积核（高斯近似）。
`[B6-T22]` (main.cu:148) 归一化（sum = 256）。
`[B6-T24]` (main.cu:155) 分配输入图像和输出。
`[B6-T25]` (main.cu:174) Part 1：global memory 卷积。
`[B6-T27]` (main.cu:185) Part 2：constant memory 卷积。
`[B6-T30]` (main.cu:198) Part 3：纹理对象采样。
`[B6-T32]` (main.cu:204) 3a：创建 CUDA array 并拷贝数据。
`[B6-T33]` (main.cu:213) 3b：资源描述符。
`[B6-T34]` (main.cu:218) 3c：纹理描述符（双线性插值 + 归一化坐标 + clamp 边界）。
`[B6-T35]` (main.cu:222) 双线性插值。
`[B6-T36]` (main.cu:224) 归一化坐标。
`[B6-T37]` (main.cu:227) 3d：创建纹理对象。
`[B6-T38]` (main.cu:233) 输出（相同大小，用于验证）。
`[B6-T42]` (main.cu:255) 3e：清理纹理资源。
`[B6-T43]` (main.cu:280) 进阶提示段。

- 三个版本（global、constant、texture）都编译通过。
- constant 版本吞吐显著高于 global（尤其在核数据被频繁访问时）。
- 纹理采样结果正确，最大误差在双线性插值允许范围内。
- 没有 CUDA runtime error。

## 观察点

- `__constant__` 内存很小（64 KB），但访问时在 L2 缓存中广播到所有 thread，适合核数据。
- 纹理内存有硬件 2D 缓存和插值器，适合空间局部 2D 访问。
- 常数内存和纹理内存都是只读的。
- 纹理对象（`cudaTextureObject_t`）比纹理引用更灵活，是现代 CUDA 的推荐方式。

## 常见坑

1. **常数内存太满**：超过 64 KB 会导致编译失败。检查所有 `__constant__` 的总大小。
2. **纹理数据未初始化**：必须用 `cudaMemcpy2DToArray` 或类似 API 把数据写入纹理。
3. **纹理坐标越界**：取决于 wrap mode。如果不设置，默认行为未定义。
4. **常数内存的初始化顺序**：`cudaMemcpyToSymbol` 必须在任何使用该数据的 kernel 启动前进行。
5. **纹理对象的生命周期**：必须显式 `cudaDestroyTextureObject` 和 `cudaFreeArray`，否则泄漏。

## 提示

- `__constant__` 变量必须在全局作用域声明（文件顶部，不能在函数内）。
- `cudaMemcpyToSymbol` 的第一个参数是 constant 变量，第二个是 host 端指针，第三个是字节数。
- 纹理采样函数是 `tex1D<T>`、`tex2D<T>`、`tex3D<T>`，返回值取决于纹理格式。
- 纹理对象创建时需要指定 `cudaTextureDesc`（filter mode、address mode、normalized coords 等）。

## 复盘问题

1. `__constant__` 为什么比 global 快？有什么限制？
2. 常数内存和编译期 `#define` 的区别？
3. 纹理内存的插值功能怎么用？对性能有什么影响？
4. 什么时候用常数内存，什么时候用纹理内存？
5. 如果数据需要频繁修改，能用常数或纹理内存吗？

## 对应官方参考

- CUDA C++ Programming Guide / Constant Memory: https://docs.nvidia.com/cuda/cuda-c-programming-guide/
- CUDA C++ Programming Guide / Texture Memory: https://docs.nvidia.com/cuda/cuda-c-programming-guide/
- cuda-samples `texture` examples: https://github.com/NVIDIA/cuda-samples

## 输出对照（printf / std::puts 原文）

- `[B6-T26]` (main.cu:182) 原文：`[B6] global mem 卷积吞吐: %.1f GB/s` -> 现：`[B6] global mem conv throughput:   %.1f GB/s`
- `[B6-T28]` (main.cu:192) 原文：`[B6] constant  卷积吞吐: %.1f GB/s` -> 现：`[B6] constant     conv throughput: %.1f GB/s`
- `[B6-T29]` (main.cu:194) 原文：`[B6] constant / global: %.2fx` -> 现：`[B6] constant / global ratio:      %.2fx`
- `[B6-T39]` (main.cu:240) 原文：`[B6] 纹理采样吞吐:       %.1f GB/s` -> 现：`[B6] texture sample throughput:    %.1f GB/s`
- `[B6-T41]` (main.cu:251) 原文：`[B6] 纹理采样最大误差:   %.6f (双线性插值允许微小误差)` -> 现：`[B6] texture sample max error:     %.6f (small bilinear interp error allowed)`
