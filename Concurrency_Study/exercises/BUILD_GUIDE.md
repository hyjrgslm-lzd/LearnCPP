# 构建指南

> 主战场：**Windows + Visual Studio 2026（MSVC）**。阶段一、二全部题目纯标准库、零外部依赖、可离线构建；阶段三仅模块 K（xsimd）与模块 M（stdexec 桥接题）需联网拉依赖。

## 前置条件

- **CMake** ≥ 3.25
- **C++20 编译器**（基线）：
  - **MSVC**：VS 2022 17.10+ 或 VS 2026（**推荐**）。本套所有 C++20 并发原语（jthread / latch / barrier / semaphore / atomic_ref / atomic wait·notify / atomic\<shared_ptr\>）在此全部可用，且并行 STL 内置、无需 TBB。
  - GCC 12+ / Clang 16+ 亦可（见下方“跨编译器说明”）。
- **Git**：仅模块 K/M 需要（FetchContent 拉取 xsimd / stdexec）。

> 关于 C++26：`std::simd`、`std::hazard_pointer`、`std::rcu`、`std::execution`(senders) 截至 2026-05 主流编译器尚未实现。本套以这些**标准 API 为讲解目标**，但练习代码用当前可编译的**回退实现**（xsimd / 自带教学版 hazard_pointer·rcu / stdexec），并在文档中标注与标准的差异。

## 快速开始（VS2026 主路径）

```bash
cd exercises

# 配置（生成 build-vs2026/concurrency_study_exercises.sln）
cmake --preset vs2026

# 构建全部题目（Release）
cmake --build --preset vs2026

# 或只构建某一题
cmake --build build-vs2026 --target A1_jthread_lifecycle --config Release
```

也可直接用 VS 打开 `build-vs2026/concurrency_study_exercises.sln`，或用 VS / VS Code
“打开文件夹”直接加载 `exercises/`（CMakePresets.json 会被自动识别）。

## 其他生成器

```bash
# Ninja（需安装 ninja）
cmake --preset ninja && cmake --build build-ninja

# 自动检测
cmake --preset default && cmake --build build
```

## 单题独立打开

每道题目录都是一个可独立配置的 CMake 项目：用 VS / VS Code 直接打开
`exercises/K1_simd_basics/` 即可单独构建。依赖（xsimd/stdexec）通过 `cmake/*Setup.cmake`
中的 TARGET 守卫按需拉取，不会与顶层重复。

## 各阶段依赖一览

| 阶段 | 模块 | 外部依赖 | 离线可构建 |
|------|------|----------|:---:|
| 一 | A/B/C/D + Capstone1 | 无 | ✅ |
| 二 | E/F/G/H/I + Capstone2 | 无（hazard_pointer/rcu 用 `include/` 自带教学实现） | ✅ |
| 三 | J | 无 | ✅ |
| 三 | K | xsimd（FetchContent） | 需联网或 `third_party/` 预克隆 |
| 三 | L | MSVC 无 / GCC·Clang 需 TBB | MSVC ✅ |
| 三 | M1 | 无 | ✅ |
| 三 | M2 | stdexec（**默认关闭**） | 需开关 + 联网 |

### 模块 M2（std::execution 桥接）默认关闭

`M2_execution_bridge` 默认**不参与构建**。原因：它依赖 NVIDIA/stdexec（P2300 参考实现），
而 stdexec 的 CMake 会经 FetchContent 拖入重型的 rapids-cmake 构建依赖，首次配置耗时长且强依赖网络。
本套对 senders 仅作「桥接演示」，深入内容在 `Execution_Study\`。需要构建 M2 时显式开启：

```bash
cmake --preset vs2026 -DCONCURRENCY_STUDY_ENABLE_STDEXEC=ON
cmake --build build-vs2026 --target M2_execution_bridge --config Release
```

其余 41 个目标不受影响，默认即可构建。M2 的 `CMakeLists.txt` 已为 MSVC 追加 `/Zc:preprocessor`
（stdexec 头文件在 MSVC 下硬性要求符合标准的预处理器）。

## 跨编译器说明

### MSVC（推荐）
- 已配置 `/Zc:__cplusplus /utf-8 /EHsc`。
- 并行 STL（`std::execution::par`）内置，无需链接任何库。
- 若遇 ICE，升级到最新 VS。

### GCC
- 内存序题目建议配合 `-fsanitize=thread`（ThreadSanitizer）交叉验证无数据竞争。
- 模块 L 的并行算法需安装 TBB（`apt install libtbb-dev`），`cmake/TbbSetup.cmake` 会自动探测。
- 模块 J 链接期可能出现 `-Winterference-size` 告警，属预期（见文档模块 J）。

### Clang
- 需 Clang 16+ 获得完整 C++20 支持；ThreadSanitizer 同样可用。

## 离线模式

见 `third_party/README.md`。

## 做题流程

1. 打开某题的 `README.md` 了解要求，详尽版在仓库根目录对应的 `XX-模块X-*.md`。
2. 打开 `main.cpp`，搜索 `// TODO [必做 N]:` 按提示填写。
3. 构建并运行：`cmake --build build-vs2026 --target <题目名> --config Release` 后运行可执行文件。
4. 完成必做后做 `// TODO [进阶 N]:`。
5. 对照文档“验收点 / 观察点 / 复盘问题”自检。
