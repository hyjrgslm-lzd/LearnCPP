# 02 模块 A：环境与首个 kernel

## 模块目标

这个模块只有一个目标：让你从"CUDA 是什么"走到"我能编译和运行一个 kernel"，并通过 PTX/SASS 和 sanitizer 看懂 GPU 真实做了什么。

你会先写最小 kernel，再学会用工具链查看编译产物和调试错误。重点不是"写复杂算法"，而是建立"代码 → 编译 → 硬件执行"的完整认知。

## 前置知识

- C++ 基础：指针、数组、结构体。
- 命令行基础：能在 VS 2026 或 CMake 里编译程序。
- git 基础：知道 clone / commit 的含义。
- **零 CUDA 假设**：如果是第一次接触 CUDA，这个模块会从最基础的语法开始。

## 模块完成标准

做完本模块，你至少要能说清楚：

1. `__global__`、`__device__`、`__host__` 分别修饰什么函数，编译到哪里。
2. kernel launch 时 `<<<gridDim, blockDim>>>` 里的两个数字分别控制什么。
3. `threadIdx` 和 `blockIdx` 的区别，以及如何把它们映射到一维数组索引。
4. CUDA runtime 和 PTX 的关系，以及 fatbin 里藏了什么。
5. `compute-sanitizer` 怎么定位越界访问和数据竞争。
6. 为什么 `printf` 从 device 有行数限制，以及如何避免丢失输出。

## 硬件与工具链要求

- **GPU**：任何 compute capability ≥ sm_60 的 GPU（Ampere 及以上推荐）。
- **宿主编译器**：Visual Studio 2026（MSVC 19.5x）。
- **CUDA Toolkit**：13.x 或更新。
- **CMake**：3.28+。
- **调试工具**：`compute-sanitizer`（CUDA Toolkit 随附）、Nsight Visual Studio Edition（可选但推荐）。

如果你的 GPU 只支持 sm_60–sm_70，模块 B 及以后涉及 `cp.async` 的题目需要 sm_80+。

---

## 练习 A1：hello_device

### 目标

写出第一个 kernel：单线程在 device 上打印 `blockIdx` 和 `threadIdx`。感受 host/device 代码分离，以及 kernel launch 的最小形式。

### 前置理解

- 你知道 C 的 `printf` 怎么用。
- 你接受 CUDA kernel 就是一个"在 GPU 上跑很多次"的函数。
- 你能区分 host 代码（在 CPU 上执行）和 device 代码（在 GPU 上执行）。

### 必做任务

1. 在 `exercises/A1_hello_device/main.cu` 中，用 `__global__` 修饰一个函数 `hello_kernel()`。
2. 在 kernel 内部，用 `printf` 打印 `"Device: blockIdx=(%d,%d,%d), threadIdx=(%d,%d,%d)\n"` 和对应值。
3. 在 `main()` 里调用 `hello_kernel<<<1, 1>>>()`（grid 和 block 都是 1D，大小都是 1）。
4. 用 `CUDA_CHECK(cudaDeviceSynchronize())` 等待 kernel 完成，再打印 host 侧完成信息。
5. 编译并运行，观察输出。
   ```cpp
   // TODO [必做] 在这里定义 __global__ hello_kernel()，内部调用 printf
   // TODO [必做] 在 main() 里 hello_kernel<<<1,1>>>() 并 sync
   ```

### 进阶任务

- 把 kernel 改成 `hello_kernel<<<2, 2>>>`（grid 2×2，block 2×2），观察有多少个线程实际运行。
- 增加一个 `__device__` 辅助函数，计算全局线程编号 `gid = blockIdx.x * blockDim.x + threadIdx.x`，在 kernel 里调用。
- 在 kernel 里增加一个 `__shared__` 数组，让同一 block 的线程写入各自的值，再用 `__syncthreads()` 读回并 print（体会 shared memory 的概念）。

### 验收点

- 程序编译通过，运行时没有 CUDA runtime error。
- `printf` 输出被完整捕获（没有因为 buffer limit 被截断）。
- 你能从输出中确认有多少个线程执行了 kernel（线程数 = gridDim × blockDim）。
- `cudaDeviceSynchronize()` 后输出了"kernel 完成"的消息。

### 观察点

- 一个 kernel launch（`<<<1,1>>>`）不等于"一个线程"——它是"启动参数"，告诉 GPU 要生成多少个线程。
- `printf` 从 device 输出时，可能被缓冲；`cudaDeviceSynchronize()` 才能保证之前的输出被 flush。
- `blockIdx` 和 `threadIdx` 都是 `uint3` 类型（3D 坐标），即使 kernel 是 1D，也可以访问 `.x` / `.y` / `.z`（后两个为 0）。

### 常见坑

1. **忘记 `cudaDeviceSynchronize()`**：kernel 是异步启动的，host 代码立刻继续执行。如果不 sync，可能 host 侧 printf 比 device 侧先输出，或者根本看不到 device 输出。
2. **`printf` 行数限制**：GPU printf buffer 有限（通常 1 MB），超过会丢失。这题输出少，但后续练习要注意。
3. **kernel 执行出错但没报错**：CUDA runtime 不会主动 throw exception。必须用 `CUDA_CHECK` 宏包裹每个 API 调用，或者用 `cudaGetLastError()` 显式查询。
4. **混淆 grid/block 维度**：`<<<gridDim, blockDim>>>` 里的两个数字分别代表 grid 大小和 block 大小，都可以是 1D/2D/3D（`dim3` 类型）。这题用标量（自动转为 dim3(x,1,1)）。
5. **shared memory 访问冲突**：如果多个线程同时写同一个 shared memory 地址，结果未定义。必须小心索引。
6. **没有在编译时指定 architecture**：如果 CMakeLists.txt 没设 `CMAKE_CUDA_ARCHITECTURES`，可能编译成 generic PTX，runtime JIT 导致变慢或失败。
7. **device 端指针不能在 host 上 dereference**：GPU 内存地址在 CPU 上没有意义。
8. **blockDim 过大导致 launch 失败**：一个 block 最多 1024 个线程（某些架构可能更少）。超过会报错。
9. **printf 格式字符串不匹配**：device 端 printf 比较严格，格式错误可能导致乱码或崩溃。
10. **没有 #include "common/cuda_check.cuh"**：要用 `CUDA_CHECK` 宏，必须 include 这个共享头文件。

### 提示

- `dim3` 是 CUDA 的 3D 坐标类型。`dim3(2)` 等同 `dim3(2, 1, 1)`。
- `blockIdx` 和 `threadIdx` 在 kernel 内部是内置变量，无需显式参数传入。
- 想看 device 端崩溃信息，用 `CUDA_CHECK(cudaGetLastError())` 在 launch 后立刻查询。
- 如果怀疑 kernel 没有执行，可以在 host 端和 device 端各打一条独特的 printf，对比输出顺序。

### 复盘问题

1. `__global__` 和 `__device__` 的区别是什么？哪个可以从 host 调用？
2. 为什么 `hello_kernel<<<1,1>>>()` 启动 1 个线程，而 `hello_kernel<<<1,32>>>()` 启动 32 个线程？
3. 如果启动 `<<<10, 32>>>`，总共有多少个线程？它们如何分组到不同的 block？
4. `cudaDeviceSynchronize()` 的作用是什么？如果不调用会怎样？
5. device 端 `printf` 和 host 端 `printf` 有什么区别（除了执行位置）？

### 对应官方参考

- CUDA C++ Programming Guide: https://docs.nvidia.com/cuda/cuda-c-programming-guide/
- CUDA Runtime API: https://docs.nvidia.com/cuda/cuda-runtime-api/（cudaDeviceSynchronize / cudaGetLastError）
- cuda-samples: https://github.com/NVIDIA/cuda-samples（查找 `vectorAdd` 等基础例子）

---

## 练习 A2：index_mapping

### 目标

掌握 1D/2D grid 和 block 的索引映射。学会把二维问题（如矩阵）分解成一维线程索引，反之亦然。

### 前置理解

- 完成了 A1。
- 你知道线性数组和二维矩阵的内存布局（行优先）。
- 接受 GPU 线程往往以 1D 线性方式分配，即使问题是 2D 的。

### 必做任务

1. 在 `exercises/A2_index_mapping/main.cu` 中，写一个 kernel `map_index_1d`，计算全局线程编号：
   ```cpp
   // TODO [必做] int idx = blockIdx.x * blockDim.x + threadIdx.x;
   // TODO [必做] 如果 idx < N，打印该线程的 idx
   ```
2. 在 `main()` 里用 `cudaMemcpy` 从 host 分配一个大小为 N=1024 的数组到 device。
3. 启动 kernel 为 `<<<32, 32>>>`（总 1024 个线程），传入 N=1024。
4. 用 `cudaMemcpy` 把结果复制回 host，验证每个线程都访问了对应的数组元素。
   ```cpp
   // TODO [必做] 声明 input 和 output 设备端数组，分别大小 N
   // TODO [必做] kernel 计算 int idx = ...; output[idx] = idx;
   ```
5. 再写一个 kernel `map_index_2d`，处理二维网格（如 32×32 矩阵）：
   ```cpp
   // TODO [必做] int row = blockIdx.y * blockDim.y + threadIdx.y;
   // TODO [必做] int col = blockIdx.x * blockDim.x + threadIdx.x;
   // TODO [必做] int idx = row * cols + col;（行优先）
   ```
6. 用 `<<<dim3(8,4), dim3(32,8)>>>` 启动（grid 8×4 block，block 32×8 thread），处理 256×32 矩阵。验证没有越界访问。
   ```cpp
   // TODO [必做] 检查 row < rows && col < cols，避免越界
   ```

### 进阶任务

- 写一个 `map_index_grid_stride` kernel，使用"grid stride loop"（loop over all elements）而不是"一线程一元素"。观察它如何处理 grid 比 data 小的情况。
- 对比一维和二维启动方式下，索引计算公式。写下两种方式的优缺点。
- 增加一个边界检查版本，处理 grid/block 过大导致索引超出 data size 的情况。

### 验收点

- 1D 版本：所有 1024 个元素被正确填充，没有越界或遗漏。
- 2D 版本：256×32 矩阵所有元素被正确映射，行列顺序无误。
- 输出数组与索引逐一核对，证明映射正确。
- 没有 CUDA runtime error。

### 观察点

- 索引映射公式 `idx = blockIdx.x * blockDim.x + threadIdx.x` 是一维线性化的核心。
- 二维映射需要同时处理 x/y（列/行），行优先 `idx = row * cols + col` 是标准约定。
- grid 和 block 的大小是独立选择的，不一定能完美覆盖数据。边界检查是必须的。
- "grid stride loop"（一个线程处理多个元素）可以消除对 grid size 的依赖，代价是每线程代码更复杂。

### 常见坑

1. **忘记边界检查**：如果 grid/block 大小不能整除数据大小，某些线程会越界。必须写 `if (idx < N)` 或 `if (row < rows && col < cols)`。
2. **行优先 vs 列优先混淆**：C/C++ 数组默认行优先。`matrix[row][col]` 对应 `flat[row * cols + col]`，不是 `row + col * rows`。
3. **blockDim 和 gridDim 位置颠倒**：`<<<gridDim, blockDim>>>` 顺序固定，反了会导致大量线程或少量线程。
4. **2D block 中只用了 x 维**：有时候 block 声明为 2D（`dim3(32, 8)`）但代码只用 `threadIdx.x`，会导致线程浪费。
5. **没有初始化数据**：host 侧的输入数据必须在 `memcpy` 前初始化，否则复制的是垃圾值。
6. **使用了 gridDim 而不是传入参数**：kernel 内部可以读 `gridDim`，但如果需要处理任意大小数据，最好显式传入 N。
7. **float vs int 精度丢失**：如果索引用了 float，转回 int 时可能丢失精度。
8. **共享变量竞争**：如果多个线程写同一个 global 数组元素，结果未定义。确保每线程写不同位置。

### 提示

- 索引计算 `idx = blockIdx.x * blockDim.x + threadIdx.x` 是 1D 线性化的标准公式，记住它。
- 对于 2D 问题，先确定行列大小，再选择 grid/block 形状使得覆盖整个矩阵。
- 用 `__host__ __device__` 修饰辅助函数，可在 host 和 device 都调用，便于验证。
- 测试时，用一个小数据（如 16×16）快速验证正确性，再扩到大数据。

### 复盘问题

1. 如果启动 `<<<dim3(2,3), dim3(32,16)>>>`，总共生成多少个线程？
2. 对于 1024 个元素，如何选择 grid 和 block 大小？有多少种选法？
3. 为什么说"grid stride loop"比"一线程一元素"更灵活？
4. 在二维索引映射中，哪个维度对应行，哪个对应列？如果搞反了会怎样？
5. 边界检查 `if (idx < N)` 为什么不能省略？

### 对应官方参考

- CUDA C++ Programming Guide / Thread Hierarchy: https://docs.nvidia.com/cuda/cuda-c-programming-guide/
- cuda-samples `vectorAdd`: https://github.com/NVIDIA/cuda-samples

---

## 练习 A3：nvcc_ptx_sass

### 目标

用 `nvcc -ptx` 生成 PTX 中间代码，用 `cuobjdump` 提取 SASS（机器码），观察编译链路。理解 fatbin 包含多个 cubin 的含义。

### 前置理解

- 完成了 A1/A2。
- 你知道编译器一般分为前端和后端。
- 接受 GPU 代码要经过多个编译阶段才能变成硬件执行。

### 必做任务

1. 在 `exercises/A3_nvcc_ptx_sass/main.cu` 中，写一个简单 kernel（或复用 A1 的 hello_kernel）。
2. 编译为 Release 模式，生成可执行文件 `a3_nvcc_ptx_sass.exe`。
3. 用 CMakeLists.txt 或命令行 `nvcc -ptx main.cu -o main.ptx`，生成 PTX 文本。
   ```cpp
   // TODO [必做] 设置 CMAKE_BUILD_TYPE Release 并启用 CMake 目标 NVCC_PTXDUMP
   // 或手工执行 nvcc -ptx
   ```
4. 打开 `main.ptx`，找到 kernel 的 PTX 代码片段，抄下 10–20 行有代表性的指令（如 `mov`、`ld.global`、`st.global` 等）。记录它们的操作数和类型。
5. 用 `cuobjdump --dump-sass main.exe` 提取 SASS 机器码（仅针对 fatbin 中目标架构编译的部分）。
   ```cpp
   // TODO [必做] cuobjdump -xall main.exe | grep -A 50 "hello_kernel"（或类似命令）
   ```
6. 对比 PTX 和 SASS：PTX 是虚拟机指令（一条 PTX 指令可能变成多条 SASS），SASS 是实际硬件指令。找到一条 PTX 指令和它对应的若干 SASS 指令。
7. 再编译一次，加入 `--generate-line-info`，对比两次生成的 PTX 大小和复杂度（line info 会增加元数据）。
   ```cpp
   // TODO [必做] nvcc ... --generate-line-info ... 并观察文件大小变化
   ```

### 进阶任务

- 在编译命令里加 `-O3`（最优化）vs 不加（`-O0`），对比 PTX 代码长度和复杂度。
- 用 `cuobjdump --dump-elf` 查看 fatbin 中存在了哪些 compute capability 的二进制。
- 查看 PTX 中的 `.target` 声明（如 `sm_80`、`sm_90a`），确认 JIT compile 的目标。

### 验收点

- 成功生成 PTX 文件。
- 从 PTX 中识别出 kernel 的 function 声明和若干指令。
- 从 SASS 中找到对应的机器码段。
- 你能用一句话解释 PTX 和 SASS 的区别。
- 没有 compilation error。

### 观察点

- PTX 是中间表示（IR），目标无关，便于优化和分析。
- SASS 是目标相关的机器码，硬件直接执行。一条 PTX 指令常常扩展为多条 SASS 指令。
- fatbin 包含多个 cubin（compute binary），对应不同 compute capability。CUDA runtime 在 load 时选择最合适的。
- `--generate-line-info` 增加源代码行号关联，便于 debugger 和 profiler 定位，代价是文件变大。
- Release 优化级别会改变 PTX 的样子（指令重排、变量消除等），但语义相同。

### 常见坑

1. **cuobjdump 找不到可执行文件**：fatbin 嵌入到可执行文件里，需要可执行文件而不是 object file。
2. **SASS 输出极其冗长**：`cuobjdump --dump-sass` 通常产生数千行。用 grep 过滤 kernel 名字，再用 head 只看前面。
3. **PTX 中看不到某些 kernel**：如果 kernel 没有被任何函数调用，可能被编译器优化掉。确保 kernel 在 main 中被启动。
4. **fatbin 中缺少某个 architecture**：如果 CMake 的 `CMAKE_CUDA_ARCHITECTURES` 没有包含目标架构，fatbin 中不会有对应的 cubin。
5. **PTX JIT 太慢**：如果 fatbin 中没有目标架构的 cubin，GPU driver 会 JIT compile PTX。这比提前编译慢得多。
6. **不知道自己的 GPU compute capability**：可以用 `nvidia-smi -q` 或 `cudaGetDeviceProperties` 查询。
7. **混淆 architecture 和 code capability**：`sm_90a` 是 architecture（芯片型号），`90` 是 compute capability（计算能力等级）。
8. **nvcc 版本和 GPU 不兼容**：某些老版 nvcc 无法生成最新 GPU 的 SASS。用 `nvcc --version` 检查。

### 提示

- PTX 的寄存器通常用 `%rx` 表示（`%r0`, `%r1` 等），局部内存用 `[%sp+offset]`。
- SASS 中的指令往往用简写（`IADD` 代表整数加法），具体定义见 GPU 架构手册。
- `cuobjdump -xall` 会列出所有 kernel 及其对应的 SASS；加 `-c` 可以只看特定 kernel。
- 如果只想看某个 kernel 的前 100 行 SASS，用管道：`cuobjdump --dump-sass a3.exe 2>/dev/null | grep -A 100 "Function: hello_kernel"`。

### 复盘问题

1. PTX 和 SASS 分别是什么？为什么 GPU 需要这两层编译？
2. 一条 PTX 指令通常对应几条 SASS 指令？为什么？
3. fatbin 中为什么要包含多个 compute capability 的 cubin？
4. `--generate-line-info` 的作用是什么？它有什么成本吗？
5. 如果你的 GPU 是 sm_90a，但 fatbin 中只有 sm_80 的 cubin，会发生什么？

### 对应官方参考

- PTX ISA Reference: https://docs.nvidia.com/cuda/parallel-thread-execution/
- CUDA Compilation: https://docs.nvidia.com/cuda/cuda-c-programming-guide/（查找 compilation model）
- cuobjdump 用法: CUDA Toolkit 文档

---

## 练习 A4：sanitizer_debug

### 目标

用 `compute-sanitizer` 定位内存访问错误（out-of-bounds）和数据竞争（race condition）。学会用 Nsight VSE 在 kernel 中设置断点调试。

### 前置理解

- 完成了 A1/A2。
- 你知道内存越界和数据竞争是什么。
- 接受 GPU 编程中，不能像 CPU 那样随意用 debugger（但 Nsight VSE 提供了基础支持）。

### 必做任务

1. 在 `exercises/A4_sanitizer_debug/main.cu` 中，写一个 kernel `buggy_kernel`，故意造一个 out-of-bounds 访问：
   ```cpp
   // TODO [必做] __global__ void buggy_oob_kernel(int *data, int n) {
   //     int idx = blockIdx.x * blockDim.x + threadIdx.x;
   //     if (idx < n + 1) {  // 注意：n+1 会越界！
   //         data[idx] = idx;
   //     }
   // }
   ```
2. 在 `main()` 里分配一个大小 1000 的数组，启动 kernel（grid/block 使得 1001 个线程运行），触发越界。
3. 用 `compute-sanitizer memcheck ./a4_debug.exe` 运行程序，观察输出（会打印越界的线程编号和地址）。
   ```cpp
   // TODO [必做] 从 host 运行 compute-sanitizer memcheck
   ```
4. 修复代码：改成 `if (idx < n)`，再用 memcheck 验证没有错误。
5. 写另一个 kernel `buggy_race_kernel`，造成数据竞争：
   ```cpp
   // TODO [必做] __global__ void buggy_race_kernel(int *flag) {
   //     int idx = threadIdx.x;
   //     if (idx < 2) {
   //         for (int i = 0; i < 1000; i++) {
   //             flag[0]++;  // 注意：所有线程都写同一个地址！
   //         }
   //     }
   // }
   ```
6. 用 `compute-sanitizer racecheck ./a4_debug.exe` 运行，观察输出（会报告数据竞争）。
   ```cpp
   // TODO [必做] 从 host 运行 compute-sanitizer racecheck
   ```
7. 修复代码：用 `atomicAdd` 或加入 `__syncthreads()` 和 lock，再用 racecheck 验证。
8. （可选）在 Visual Studio 2026 中用 Nsight VSE，设置 kernel 断点，观察线程状态（需要 `-g` 编译标志）。

### 进阶任务

- 在 buggy_race_kernel 中，不用 atomic，而是用 shared memory + `__syncthreads()` 来安全累加，再用 racecheck 验证没有数据竞争。
- 用 `compute-sanitizer synccheck` 检查同步错误（缺少 `__syncthreads` 导致的 race）。
- 修改 kernel 使得只有某些线程会竞争，观察 sanitizer 如何定位具体的线程对。

### 验收点

- `compute-sanitizer memcheck` 在修复前能检出 out-of-bounds 错误，修复后报告"no errors"。
- `compute-sanitizer racecheck` 在修复前能检出数据竞争，修复后报告"no errors"。
- 没有 compilation error。
- 代码能正确运行（不崩溃，输出正确）。

### 观察点

- `compute-sanitizer memcheck` 相对轻量，能快速定位越界。它会暂停访问越界的线程并报告。
- `compute-sanitizer racecheck` 需要动态追踪，性能开销大，但能定位并发写同一地址的所有线程对。
- Nsight VSE 的 kernel 调试功能有限（不像 CPU debugger 那么灵活），但足以观察线程状态和寄存器值。
- 大多数 GPU 缺陷来自"假设线程之间有序执行"或"忘记同步"，sanitizer 是快速抓住这些的工具。

### 常见坑

1. **compute-sanitizer 找不到可执行文件**：确保路径正确且文件编译了（不能跑 object file）。
2. **memcheck 输出太多，看不清**：用 `compute-sanitizer memcheck --report-api-errors all ./exe 2>&1 | head -50` 只看前面。
3. **racecheck 误报或漏报**：dynamic analysis 存在假正和假负。memcheck 更可信。
4. **编译时没有 debug 信息**：为了看到源代码行号，编译时加 `--generate-line-info` 或 `-g`。
5. **shared memory race 没被检出**：shared memory 的 race 有时候 racecheck 检不出，因为访问模式复杂。手工审查很重要。
6. **atomic 不是万能的**：某些竞争模式（如"先读后写"）用 atomic 也不对，需要 lock 或 CAS。
7. **kernel 在 sanitizer 下性能很差**：sanitizer 有 10-100 倍的开销。不要拿 sanitizer 测试的性能数据。
8. **忘记同步 device 和 host**：某些错误只在 `cudaDeviceSynchronize()` 后才会被 sanitizer 检出。

### 提示

- `atomicAdd(addr, val)` 是原子加法，保证多线程写同一个地址时不会丢失。对小数据特别有用。
- `__syncthreads()` 只同步 block 内的线程，不同 block 的线程无法用它同步。
- 如果想在 block 间同步，Hopper 有 `cluster.sync()`；其他架构需要用 kernel 外的 host 侧同步或 cooperative groups。
- sanitizer 的性能开销很大，通常只用来快速抓缺陷，不用来做性能测量。

### 复盘问题

1. `compute-sanitizer memcheck` 能检出哪些错误？`racecheck` 呢？
2. 为什么说 racecheck 的开销比 memcheck 大？
3. 修复数据竞争有哪几种办法？各有什么优缺点？
4. Nsight VSE 的 kernel 调试和 CPU 调试有什么根本区别？
5. 如果 sanitizer 没有检出某个缺陷，是不是说代码是对的？

### 对应官方参考

- compute-sanitizer: https://docs.nvidia.com/cuda/compute-sanitizer/
- Nsight Visual Studio Edition: https://docs.nvidia.com/nsight-visual-studio-edition/
- CUDA Runtime API: https://docs.nvidia.com/cuda/cuda-runtime-api/（atomic 函数）

---

## 做完本模块后应达到的水平

完成 A1–A4 后，你应该能：

1. 独立编写一个最小 CUDA kernel，用 `<<<grid, block>>>` 启动，观察 device 端输出。
2. 用 threadIdx/blockIdx 计算线程的全局编号，处理 1D 和 2D 数据。
3. 阅读 PTX 中间代码和 SASS 机器码，理解编译链路。
4. 用 `compute-sanitizer` 快速定位内存错误和竞争条件。
5. 编写任意大小数据的 kernel 而不担心越界或同步缺陷。
6. 解释 CUDA runtime 异步执行模型：为什么要 `cudaDeviceSynchronize()` 和 `CUDA_CHECK`。

接下来，模块 B 会深入内存层级、数据搬运、bank conflict 等硬件细节。
