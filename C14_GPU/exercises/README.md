# C14_GPU / exercises

CUDA + OptiX + CUTLASS 渐进练习集合。

## 标准与工具链

| 项 | 值 |
|---|---|
| CMake | ≥ 3.28 |
| Host CXX | **C++26**（MSVC 19.5x 通过 `-std:c++latest` 注入；Clang/GCC 15+ 通过 `-std=c++26`） |
| CUDA | **C++20**（nvcc 13.x 主流） |
| CUDA 架构 | `80;86;89;90a;100a;120a`（Ampere/Ada/Hopper/Blackwell） |

C++26 由根 `CMakeLists.txt` 在 pre-`project()` 阶段为 MSVC 注入 `CMAKE_CXX26_STANDARD_COMPILE_OPTION`，所有子目录通过 `CMAKE_CXX_STANDARD 26` 自动继承，**无需在每个练习中重声明**。

## 构建

```bash
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release -j
```

Linux：

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

## 可选依赖

### OptiX（K1-K4、CAPSTONE2_C）

OptiX SDK 8.x，**手动安装**。`cmake/OptiXSetup.cmake` 按以下顺序搜索：

1. `-DOptiX_INSTALL_DIR=<path>`（最显式）
2. 环境变量 `$OPTIX_SDK_ROOT`
3. 环境变量 `$OPTIX_PATH`
4. Windows 默认 `C:/ProgramData/NVIDIA Corporation/OptiX SDK 8*`
5. Linux `/usr/local/optix`、`/opt/optix`、`$HOME/optix`

未找到时 K1-K4 与 CAPSTONE2_C 自动以 stub 模式编译，**不影响其他 target**。强制跳过：`-DGPU_STUDY_NO_OPTIX=ON`。

下载：<https://developer.nvidia.com/designworks/optix/download>

### CUTLASS

`cmake/CutlassSetup.cmake` 通过 `FetchContent` 自动拉取到 `third_party/cutlass-src/`，header-only 接入，跳过其自带 CMake。锁定 tag：`-DGPU_STUDY_CUTLASS_TAG=v3.6.0`。

## 目录迁移（随处复制）

整个 `exercises/` 目录可拷贝到任意路径（不同盘符 / 不同主机 / Windows ↔ Linux）。`CMakeLists.txt` 中**没有任何绝对路径硬编码**，所有外部依赖通过 `find_package` / 环境变量发现。

复制后**首次构建**：

```bash
# 1. 删除旧的构建缓存（含旧路径绝对引用）
rm -rf build/
rm -rf third_party/cutlass-subbuild/   # FetchContent stamp，路径绑定旧位置
# third_party/cutlass-src/ 可保留以省去重新下载

# 2. 重新 configure + build
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release -j
```

> Windows PowerShell 用 `Remove-Item -Recurse -Force build, third_party/cutlass-subbuild`。

## 可选开关

| 选项 | 默认 | 说明 |
|---|---|---|
| `GPU_STUDY_ENABLE_WARNINGS` | ON | 严格警告（MSVC `/W4`、GCC/Clang `-Wall -Wextra -Wpedantic`） |
| `GPU_STUDY_CUDA_WARNINGS` | OFF | CUDA host warning 转发（`-Xcompiler=-Wall`） |
| `GPU_STUDY_ENABLE_ASAN` | OFF | AddressSanitizer（仅 host，非 MSVC） |
| `GPU_STUDY_ENABLE_UBSAN` | OFF | UBSan（仅 host，非 MSVC） |
| `GPU_STUDY_LINEINFO` | OFF | nvcc `--generate-line-info`（Nsight Compute SASS 关联用） |
| `GPU_STUDY_FETCH_CUTLASS` | ON | 通过 FetchContent 拉取 CUTLASS |
| `GPU_STUDY_NO_OPTIX` | OFF | 强制跳过 OptiX 发现 |
