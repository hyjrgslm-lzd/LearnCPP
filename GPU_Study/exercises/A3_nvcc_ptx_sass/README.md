# 练习 A3：nvcc_ptx_sass

## 目标

`[A3-T01]` (main.cu:2) 练习 A3：nvcc_ptx_sass。
`[A3-T02]` (main.cu:3) 用 `nvcc -ptx` 生成 PTX 中间代码，
`[A3-T03]` (main.cu:4) 用 `cuobjdump` 提取 SASS 机器码，
`[A3-T04]` (main.cu:5) 观察 fatbin 编译链路。
`[A3-T05]` (main.cu:6) （空行注释）

用 `nvcc -ptx` 生成 PTX 中间代码，用 `cuobjdump` 提取 SASS（机器码），观察编译链路。理解 fatbin 包含多个 cubin 的含义。

## 前置理解

- 完成了 A1/A2。
- 你知道编译器一般分为前端和后端。
- 接受 GPU 代码要经过多个编译阶段才能变成硬件执行。

## 必做任务

`[A3-T06]` (main.cu:7) 必做工作流（在编译完成后于命令行执行）。
`[A3-T07]` (main.cu:8) 工作流提示。
`[A3-T08]` (main.cu:9) 步骤 1：构建。
`[A3-T09]` (main.cu:10) `cmake --build . --config Release --target A3_nvcc_ptx_sass`。
`[A3-T10]` (main.cu:11) 步骤 2：生成 PTX。
`[A3-T11]` (main.cu:12) `nvcc -ptx main.cu -o main.ptx -arch=sm_80`。
`[A3-T12]` (main.cu:13) 步骤 3：从可执行文件提取 SASS。
`[A3-T13]` (main.cu:14) `cuobjdump --dump-sass A3_nvcc_ptx_sass.exe 2>nul | findstr /A 50 "fma_kernel"`。
`[A3-T14]` (main.cu:15) 步骤 4：带行号信息重新编译。
`[A3-T15]` (main.cu:16) `nvcc --generate-line-info -ptx main.cu -o main_lineinfo.ptx -arch=sm_80`。
`[A3-T16]` (main.cu:17) 步骤 5：对比 `main.ptx` 与 `main_lineinfo.ptx` 的文件大小。
`[A3-T20]` (main.cu:34) TODO [必做] 步骤 1：这就是你要观察 PTX/SASS 的 kernel，无需修改；先编译运行，再生成 PTX，再对比 SASS。

1. 在 `main.cu` 中已有一个简单的 `fma_kernel`（乘加运算）。编译为 Release 模式，生成可执行文件。
2. 编译并运行，确认程序输出正确。
3. 用命令行 `nvcc -ptx main.cu -o main.ptx -arch=sm_80` 生成 PTX 文本。

```
// TODO [必做] 执行：nvcc -ptx main.cu -o main.ptx -arch=sm_80
```

4. 打开 `main.ptx`，找到 `fma_kernel` 的 PTX 代码片段，抄下 10–20 行有代表性的指令（如 `mov`、`ld.global`、`st.global`、`fma.rn.f32` 等）。
5. 用 `cuobjdump --dump-sass A3_nvcc_ptx_sass.exe` 提取 SASS 机器码。

```
// TODO [必做] 执行：cuobjdump --dump-sass A3_nvcc_ptx_sass.exe | findstr /A 100 "fma_kernel"
```

6. 对比 PTX 和 SASS：PTX 是虚拟机指令（一条 PTX 指令可能变成多条 SASS），SASS 是实际硬件指令。找到一条 PTX 指令和它对应的若干 SASS 指令。
7. 再编译一次，加入 `--generate-line-info`，对比两次生成的 PTX 大小和复杂度。

```
// TODO [必做] 执行：nvcc --generate-line-info -ptx main.cu -o main_li.ptx -arch=sm_80
```

## 进阶任务

`[A3-T23]` (main.cu:51) 进阶：-O3 vs -O0 对比，观察 PTX 代码长度。
`[A3-T24]` (main.cu:53) TODO [进阶] 重新用 `nvcc -O0 -ptx` 与 `nvcc -O3 -ptx` 各生成一次，对比生成的 PTX 文件行数和指令种类。
`[A3-T25]` (main.cu:55) TODO [进阶] 用 `cuobjdump --dump-elf A3_nvcc_ptx_sass.exe` 查看 fatbin 中包含了哪些 compute capability 的 cubin。

- 在编译命令里加 `-O3`（最优化）vs 不加（`-O0`），对比 PTX 代码长度和复杂度。
- 用 `cuobjdump --dump-elf` 查看 fatbin 中存在了哪些 compute capability 的二进制。
- 查看 PTX 中的 `.target` 声明（如 `sm_80`、`sm_90a`），确认 JIT compile 的目标。

## 验收点

`[A3-T17]` (main.cu:30) 目标 kernel：做一些有意义的浮点运算。
`[A3-T18]` (main.cu:31) 让 PTX/SASS 中出现可读指令。
`[A3-T19]` (main.cu:32) （fma、ld、st 等）。
`[A3-T21]` (main.cu:45) `// fused multiply-add: c = a * b + c`。
`[A3-T22]` (main.cu:46) 在 PTX 中会产生 `fma.rn.f32` 指令。
`[A3-T26]` (main.cu:60) `main` 入口。
`[A3-T27]` (main.cu:64) 提示学生的工作流。
`[A3-T34]` (main.cu:82) 准备数据。
`[A3-T35]` (main.cu:83) 1M 元素。
`[A3-T36]` (main.cu:104) 启动 `fma_kernel`。
`[A3-T38]` (main.cu:127) 取回结果并做简单验证。
`[A3-T39]` (main.cu:130) 验证第 0 个元素：`c[0] = a[0]*b[0] + 1.0f = 0 + 1 = 1.0f`。
`[A3-T40]` (main.cu:138) 清理。
`[A3-T41]` (main.cu:146) 提醒学生下一步。

- 成功生成 PTX 文件。
- 从 PTX 中识别出 kernel 的 function 声明和若干指令。
- 从 SASS 中找到对应的机器码段。
- 你能用一句话解释 PTX 和 SASS 的区别。
- 没有 compilation error。

## 观察点

- PTX 是中间表示（IR），目标无关，便于优化和分析。
- SASS 是目标相关的机器码，硬件直接执行。一条 PTX 指令常常扩展为多条 SASS 指令。
- fatbin 包含多个 cubin（compute binary），对应不同 compute capability。CUDA runtime 在 load 时选择最合适的。
- `--generate-line-info` 增加源代码行号关联，便于 debugger 和 profiler 定位，代价是文件变大。
- Release 优化级别会改变 PTX 的样子（指令重排、变量消除等），但语义相同。

## 常见坑

1. **cuobjdump 找不到可执行文件**：fatbin 嵌入到可执行文件里，需要可执行文件而不是 object file。
2. **SASS 输出极其冗长**：`cuobjdump --dump-sass` 通常产生数千行。用 grep/findstr 过滤 kernel 名字，再用 head 只看前面。
3. **PTX 中看不到某些 kernel**：如果 kernel 没有被任何函数调用，可能被编译器优化掉。确保 kernel 在 main 中被启动。
4. **fatbin 中缺少某个 architecture**：如果 CMake 的 `CMAKE_CUDA_ARCHITECTURES` 没有包含目标架构，fatbin 中不会有对应的 cubin。

## 提示

- PTX 的寄存器通常用 `%rx` 表示（`%r0`, `%r1` 等），局部内存用 `[%sp+offset]`。
- SASS 中的指令往往用简写（`FFMA` 代表浮点乘加），具体定义见 GPU 架构手册。
- 如果只想看某个 kernel 的前 100 行 SASS，用管道：
  `cuobjdump --dump-sass a3.exe 2>nul | findstr /A 100 "Function : fma_kernel"`

## 复盘问题

1. PTX 和 SASS 分别是什么？为什么 GPU 需要这两层编译？
2. 一条 PTX 指令通常对应几条 SASS 指令？为什么？
3. fatbin 中为什么要包含多个 compute capability 的 cubin？
4. `--generate-line-info` 的作用是什么？它有什么成本吗？
5. 如果你的 GPU 是 sm_90a，但 fatbin 中只有 sm_80 的 cubin，会发生什么？

## 对应官方参考

- PTX ISA Reference: https://docs.nvidia.com/cuda/parallel-thread-execution/
- CUDA Compilation: https://docs.nvidia.com/cuda/cuda-c-programming-guide/（查找 compilation model）
- cuobjdump 用法: CUDA Toolkit 文档

## 输出对照（printf / std::puts 原文）

- `[A3-T28]` (main.cu:68) 原文："  [步骤 2] 编译完成后，在 build 目录执行：" -> 现："  [step 2] After building, in the build directory run:"
- `[A3-T29]` (main.cu:70) 原文："    nvcc -ptx main.cu -o main.ptx -arch=sm_80" -> 现：保持英文不变
- `[A3-T30]` (main.cu:72) 原文："  [步骤 4] 查看 SASS：" -> 现："  [step 4] View SASS:"
- `[A3-T31]` (main.cu:74) 原文："    cuobjdump --dump-sass A3_nvcc_ptx_sass.exe" -> 现：保持英文不变
- `[A3-T32]` (main.cu:76) 原文："  [步骤 7] 带行号：" -> 现："  [step 7] With line info:"
- `[A3-T33]` (main.cu:78) 原文："    nvcc --generate-line-info -ptx main.cu -o main_li.ptx -arch=sm_80" -> 现：保持英文不变
- `[A3-T37]` (main.cu:124) 原文："[A3] fma_kernel 耗时: %.3f ms" -> 现："[A3] fma_kernel elapsed: %.3f ms"
- `[A3-T42]` (main.cu:148) 原文："  TODO [必做] 步骤 3：打开 main.ptx，找到 fma_kernel" -> 现："  TODO [REQUIRED] step 3: open main.ptx, find the fma_kernel"
- `[A3-T43]` (main.cu:150) 原文："              的函数声明，摘录 10-20 行有代表性的指令。" -> 现："              function declaration, copy 10-20 representative lines."
- `[A3-T44]` (main.cu:152) 原文："  TODO [必做] 步骤 5：在 SASS 中找到对应指令段落。" -> 现："  TODO [REQUIRED] step 5: locate the matching block in the SASS dump."
- `[A3-T45]` (main.cu:154) 原文："  TODO [必做] 步骤 6：对比 PTX 和 SASS，写出区别。" -> 现："  TODO [REQUIRED] step 6: compare PTX vs SASS and write down the differences."
