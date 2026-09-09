# 练习 F5：Nsight Visual Studio Edition 中的 kernel 断点与寄存器查看

## 目标

`[F5-T01]` (main.cu:2) 练习 F5：Nsight Visual Studio Edition（VSE）中的 kernel 断点与寄存器查看。
`[F5-T02]` (main.cu:3) 学习目标：2D Gaussian blur kernel（512x512）适合 IDE（Integrated Development Environment，集成开发环境）断点演示；在 Nsight VSE 中为 kernel 关键行设置断点；单步执行查看 warp / lane 级寄存器值（`$r0`、`$r1` ...）；观察 CUDA Warp Info / Lane Info 窗口。
`[F5-T03]` (main.cu:9) 编译命令（Debug 配置）。
`[F5-T04]` (main.cu:10) 运行命令。
`[F5-T05]` (main.cu:11) Nsight VSE 调试流程：用 Debug 配置编译（不使用 Release 优化）；在 main.cu 第约 90 行（`gaussian_blur` 内积循环）设置断点；菜单 `Extensions` -> `Nsight` -> `Start CUDA Debugging`；程序停在断点后，打开 CUDA Warp Info / Lane Info 窗口；查看当前 warp 的寄存器值，单步执行。

学会在 Nsight VSE 中为 kernel 设置断点、单步执行、以及查看 warp / lane 级的寄存器值。本练习使用 2D Gaussian blur kernel（512x512）作为调试目标，该 kernel 包含 smem 加载和卷积内积两个清晰阶段，适合断点观察。

## 前置理解

- 你用过 Visual Studio 的 CPU 调试功能（设断点、查看变量等）。
- 你理解 warp 和 lane 的概念。
- 你能接受 kernel 在调试模式下会显著变慢。

## 编译

**必须使用 Debug 配置**（Release 模式优化会破坏 source-level 调试）：

```bash
# CMake Debug 配置
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build build --config Debug --target F5_nsight_vse_kernel_breakpoint

# 或使用 ncu-profile preset（如果定义了）
cmake --build build --target F5_nsight_vse_kernel_breakpoint
```

在 Visual Studio 中：选择 `Debug` 配置（下拉菜单），不要使用 `Release`。

## Nsight VSE 调试步骤

### 步骤 1：安装 Nsight VSE 插件

Nsight Visual Studio Edition 通常随 CUDA Toolkit 一并安装。验证方式：

- 打开 Visual Studio -> 菜单栏应出现 `Extensions` -> `Nsight` 菜单
- 或查看 `扩展` -> `管理扩展` -> 搜索 "Nsight"

如未安装，从以下地址下载：
https://developer.nvidia.com/nsight-visual-studio-edition

### 步骤 2：设置 kernel 断点

1. 在 Visual Studio 中打开 `main.cu`。
2. 找到 `gaussian_blur` kernel 中卷积内积的 `acc +=` 那一行（约第 90 行，注释标注 `<-- recommended breakpoint line`）。
3. 点击左侧行号旁的灰色区域，设置断点（红色圆点出现）。

> 也可以设置在 smem 加载循环的第一行，观察 smem 填充过程。

### 步骤 3：启动 CUDA 调试

1. 菜单：`Extensions` -> `Nsight` -> `Start CUDA Debugging (Next-Gen)`
2. 或：`调试` -> `开始调试 (F5)` — 如果已配置 Nsight 调试器

程序将在 host 代码正常运行，当 kernel 被 launch 后，调试器会在 GPU 上命中断点。

### 步骤 4：观察 CUDA Warp Info 窗口

断点命中后：

1. 菜单：`Extensions` -> `Nsight` -> `Windows` -> `CUDA Warp Info`
   - 显示当前 block 内所有 warp 的状态
   - 可以看到哪些 warp 处于 Active / Inactive / Divergent 状态
2. 菜单：`Extensions` -> `Nsight` -> `Windows` -> `CUDA Lane Info`
   - 显示当前 warp 内每个 lane（0-31）的寄存器值
   - 可以看到 `acc` 对应的寄存器（例如 `$r4`）在每个 lane 上的值

### 步骤 5：单步执行

- `F10`（Step Over）：执行当前行，不进入函数
- `F11`（Step Into）：进入当前行的函数调用
- `F5`（Continue）：继续运行到下一个断点

在内积循环的 `acc += ...` 行单步执行，观察：
- Lane Info 中 `acc` 寄存器值随迭代递增
- 每个 lane 的 `acc` 值应该不同（因为各自负责不同的输出像素）

### 步骤 6：切换 warp 和 block

在 CUDA Warp Info 窗口中：
- 点击不同的 warp，切换当前观察的 warp
- 观察 `blockIdx.x / blockIdx.y / threadIdx.x / threadIdx.y` 的值
- 比较相邻 warp 的 `acc` 寄存器：应该反映对应像素的 Gaussian 加权和进度

### 步骤 7：查看 shared memory

在调试窗口 -> `Watch` 或 `Memory` 窗口中，输入 `smem` 可以查看当前 block 的 shared memory 内容：
- 加载阶段之后，smem 应该包含正确的像素值（含 halo）
- 与 `h_src` 对应区域的值对比，验证加载是否正确

## 必做任务

`[F5-T06]` (main.cu:23) 常量定义。
`[F5-T07]` (main.cu:24) 图像宽度（像素）。
`[F5-T08]` (main.cu:25) 图像高度（像素）。
`[F5-T09]` (main.cu:26) Gaussian 卷积半径（kernel size = 2*RADIUS+1 = 7）。
`[F5-T10]` (main.cu:27) tile 宽度（含 halo = TILE_W + 2*RADIUS）。
`[F5-T11]` (main.cu:28) tile 高度。
`[F5-T12]` (main.cu:31) 设备端：2D Gaussian blur kernel；使用 shared memory tiling 减少全局内存访问。
`[F5-T13]` (main.cu:42) TODO [必做-1] 完成 smem 加载与卷积内积。
`[F5-T14]` (main.cu:44) 输入图像，行优先，IMG_H x IMG_W。
`[F5-T15]` (main.cu:45) 输出图像。
`[F5-T16]` (main.cu:46) Gaussian kernel，(2R+1)^2 个系数。
`[F5-T17]` (main.cu:51) shared memory tile（含 halo 区域）。
`[F5-T18]` (main.cu:52) 大小 = `(TILE_W+2*radius)*(TILE_H+2*radius)`。
`[F5-T19]` (main.cu:59) 加载 shared memory tile（含边界 clamp）。
`[F5-T20]` (main.cu:60) TODO [必做-1] 将 `(smem_w x smem_h)` 区域从 `src` 加载到 `smem`，每个线程负责一个或多个 smem 元素（多次迭代覆盖 halo），边界 clamp 到 `[0, width-1] / [0, height-1]`。
`[F5-T21]` (main.cu:80) TODO [必做-2] 卷积内积——在此行设置断点，观察 `acc` 的寄存器变化。
`[F5-T31]` (main.cu:201) TODO [必做-2] 设置断点在 `gaussian_blur` kernel 卷积内积行，在 Nsight VSE 中启动 CUDA Debugging，当程序停在断点时：打开 CUDA Warp Info 窗口观察当前活跃 warp；打开 Lane Info 窗口查看 `acc` 寄存器的逐 lane 值；单步执行几次，观察 `acc` 随 k 累加的变化。
`[F5-T33]` (main.cu:218) TODO [必做-1] 完成 `gaussian_blur` kernel（加载 smem + 卷积内积）。
`[F5-T34]` (main.cu:219) TODO [必做-2] 在 Nsight VSE 中设置断点并进行 CUDA 调试，记录：断点命中时的 `block(x,y)` / warp / lane 信息；`acc` 寄存器的初始值和几次迭代后的值；smem 中像素值是否与 `h_src` 对应区域一致。
`[F5-T35]` (main.cu:223) TODO [必做-3] 尝试在不同 warp 间切换（CUDA Warp Info 窗口），比较寄存器差异。

## Kernel 结构说明

```
gaussian_blur kernel
|-- 阶段 1：加载 smem tile（含 halo，边界 clamp）  <- 可在此设第 1 个断点
|   `-- 每个线程负责加载一个或多个 smem 元素
|-- __syncthreads()
`-- 阶段 2：卷积内积                              <- 推荐断点行
    |-- for ky in [0, 2*radius+1):
    |   `-- for kx in [0, 2*radius+1):
    |           acc += smem[...] * kern[...]      <- acc 寄存器在此累积
    `-- dst[...] = acc
```

## 常见坑

1. 没有用 Debug 配置编译，导致无法进行 GPU 级调试（某些优化会导致编译后代码与源代码对应关系丢失）。
2. 在执行大 kernel 时设置了 halt，导致 GPU 被锁死（其它应用无法使用 GPU）；需要小心管理。
3. VSE 的 GPU 调试功能在某些 GPU 上不可用（需要特定的 Compute Capability 和驱动版本）；需要提前检查。
4. 混淆了"warp ID"与"block ID"，导致查看错误的寄存器。
5. 某些寄存器名称在不同架构上不同；文档中需要说明基于的架构。
6. 调试模式下 kernel 速度可能比正常慢 100 倍以上，耐心等待断点命中。

## CUDA-GDB 等效流程（Linux）

如果使用 Linux 而非 Windows + VSE，可以用 `cuda-gdb`：

```bash
cuda-gdb ./F5_nsight_vse_kernel_breakpoint

# 在 cuda-gdb 中
(cuda-gdb) break gaussian_blur        # 在 kernel 函数入口设断点
(cuda-gdb) run                        # 启动
(cuda-gdb) cuda block 0,0 warp 0      # 切换到 block(0,0) warp 0
(cuda-gdb) info cuda threads          # 查看所有 CUDA 线程状态
(cuda-gdb) print $r4                  # 查看寄存器 r4（即 acc）
(cuda-gdb) step                       # 单步
(cuda-gdb) continue                   # 继续到下一断点
```

## 进阶任务

`[F5-T36]` (main.cu:225) TODO [进阶-1] 设置条件断点（当 `out_x==256 && out_y==256` 时停）。
`[F5-T37]` (main.cu:226) TODO [进阶-2] 用 CUDA-GDB（Linux）执行同等调试流程，对比命令行 vs GUI 体验。
`[F5-T38]` (main.cu:227) TODO [进阶-3] 修改 `h_src` 中某个像素值，在调试时观察 smem 加载是否正确反映。

- 尝试在不同的 warp 间切换，观察它们的寄存器值如何不同。
- 使用条件断点（当 `out_x==256 && out_y==256` 时停止）来定位特定像素的计算。
- 在 Linux + CUDA-GDB 执行同等流程，对比命令行 vs GUI 体验。

## 验收点

`[F5-T22]` (main.cu:99) Host：生成归一化 Gaussian kernel（sigma=1.0）。
`[F5-T23]` (main.cu:115) 归一化。
`[F5-T24]` (main.cu:120) `main` 入口。
`[F5-T25]` (main.cu:130) 分配内存。
`[F5-T26]` (main.cu:140) 生成测试图像（渐变 + 几个亮点，便于调试时肉眼验证）。
`[F5-T27]` (main.cu:144) 几个亮点。
`[F5-T28]` (main.cu:152) 生成 Gaussian kernel（sigma=1.0）。
`[F5-T29]` (main.cu:174) 启动 Gaussian blur kernel。
`[F5-T30]` (main.cu:184) shared memory 大小 = `(tile + 2*radius)^2 * sizeof(float)`。
`[F5-T32]` (main.cu:213) 结果验证（检查输出不全为 0）。
`[F5-T39]` (main.cu:230) 清理。
`[F5-T40]` (main.cu:240) 完成提示。

- 能够在 kernel 中设置断点并进入调试。
- 能够查看某个 warp 的寄存器值。
- 能够单步执行并观察 `acc` 寄存器随卷积内积循环的变化。
- 记录了调试过程（可以是文字描述或截图）。

## 复盘问题

1. 为什么 kernel 调试比 CPU 调试慢这么多？
2. 一次调试时只能看一个 warp / lane 的状态，意味着什么？如何调试多个 warp 的交互？
3. 寄存器值在调试过程中能修改吗？有什么应用场景？

## 对应官方参考

- Nsight Visual Studio Edition：https://docs.nvidia.com/nsight-visual-studio-edition/
- CUDA-GDB Debugging Guide：https://docs.nvidia.com/cuda/cuda-gdb/（Linux）

## 输出对照（printf / std::puts 原文）

- `[F5-T29]` (main.cu:177) 原文：`--- Gaussian blur kernel（512x512，radius=%d）---` -> 现：`--- Gaussian blur kernel (512x512, radius=%d) ---`
- 图像信息中文 `图像: %dx%d Gaussian radius=%d kernel size=%dx%d` -> 英文 `image: %dx%d Gaussian radius=%d kernel size=%dx%d`。
- 启动信息 `启动:` -> `launch:`，`Nsight VSE 断点建议：本文件第 ~90 行（卷积内积 acc += ... 处）` -> `Nsight VSE breakpoint hint: line ~90 in this file (acc += ... inside inner product)`。
- 时间信息 `时间=%.3f ms` -> `time=%.3f ms`。
- `[F5-T32]` (main.cu:215) 原文：`输出像素总和 = %.4f（stub 期望 0，完成后应 > 0）` -> 现：`output pixel sum = %.4f (stub expects 0; should be > 0 once complete)`。
- `[F5-T40]` (main.cu:243) 原文：`[F5] 完成。在 Nsight VSE 中用 Debug 配置编译，按 README.md 步骤调试 kernel。` -> 现：`[F5] done. Build with Debug config; follow README.md to debug the kernel in Nsight VSE.`
