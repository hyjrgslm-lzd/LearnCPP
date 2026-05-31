# 构建指南

本练习包不依赖任何外部库；所有代码只用标准库 `<ranges>` / `<algorithm>` / `<iterator>` 等。

## 前置条件

- **CMake** >= 3.28
- **C++26 编译器**（优先级从高到低）：
  - MSVC `cl.exe` 自 Visual Studio 18 2026（首选；预设 `vs2026`）
  - MSVC `cl.exe` 自 Visual Studio 17 2022 17.10+（回退；预设 `vs2022`，部分 C++26 特性可能缺失）
  - GCC 14+（C++26 子集支持）
  - Clang 18+（C++26 子集支持）

> C++26 是 C++23 的超集；文档中标注 `// C++23: ...` 的代码在 C++26 下直接可编。若本地工具链暂未完成 C++26 支持，可临时把 `CMAKE_CXX_STANDARD` 改回 23 或 20（见 `cmake/RangesSetup.cmake` 注释）。

## 快速开始

### 方式 1：Visual Studio 2026（Windows 首选）

```powershell
cd P:\C++Code\Ranges_Study\exercises
cmake --preset vs2026
cmake --build build-vs2026 --config Release
```

然后可用 VS 打开 `build-vs2026/ranges_exercises.sln` 做单题调试。

### 方式 2：Visual Studio 2022（回退）

```powershell
cmake --preset vs2022
cmake --build build-vs2022 --config Release
```

### 方式 3：Ninja（Linux / macOS / Windows）

```bash
cmake --preset ninja
cmake --build build-ninja
```

### 方式 4：Default（自动探测）

```bash
cmake --preset default
cmake --build build
```

### 只构建某一道题

```bash
cmake --build build-vs2026 --target A1_iota_view --config Release
./build-vs2026/A1_iota_view/Release/A1_iota_view.exe
```

## 做题流程

1. 先读章节 markdown（位于本目录上一级，如 `../02-模块A-视图工厂与惰性.md`）。
2. 打开对应题目的 `README.md`，看"目标 / 前置理解 / 验收点"。
3. 打开 `main.cpp`，搜索 `// TODO [必做]`，按提示补码。
4. `cmake --build build-vs2026 --target <题目名>` 编译并运行。
5. 做完必做后搜索 `// TODO [进阶]` 扩展。
6. 回到 README 逐条回答"复盘问题"。

## C++26 / vs2026 说明

- 顶层 `CMakeLists.txt` 全局设置 `set(CMAKE_CXX_STANDARD 26)`。
- MSVC 全局注入 `/Zc:__cplusplus /utf-8 /permissive-`。
- 若特定题目在 C++26 下触发编译器 ICE，临时在该题 `CMakeLists.txt` 手动回退：
  ```cmake
  set_property(TARGET <题目名> PROPERTY CXX_STANDARD 23)
  ```

## 目录结构

```
exercises/
├── CMakeLists.txt          # 顶层构建（31 条 add_subdirectory）
├── CMakePresets.json       # 预设：vs2026 / vs2022 / default / debug / ninja / ninja-debug
├── BUILD_GUIDE.md          # 本文件
├── README.md               # 练习包导览与索引
├── .gitignore
├── cmake/
│   └── RangesSetup.cmake   # 扩展位（当前 no-op）
├── 01_mental_model_warmup/ # 心智模型预热
├── A1_iota_view/ ... A3_repeat_cartesian/          # 模块 A
├── B1_*/ B2_*/ B3_*/       # 模块 B
├── C1_1_*/ ... C2_3_*/     # 模块 C1/C2
├── D1_*/ D2_*/ D3_*/       # 模块 D
├── CAPSTONE1_log_pipeline/ # 结课项目 1
├── E*/  F1_*/              # 模块 E/F
├── G*/  H*/                # 模块 G/H
├── CAPSTONE3_impl_source_reading/  # 结课项目 3
└── CAPSTONE4_mini_ranges/          # 结课项目 4
```

每个题目子目录都含三文件：`CMakeLists.txt` / `main.cpp` / `README.md`（结课项目 3/4 另有 `notes/` 与分层 `.hpp`）。
