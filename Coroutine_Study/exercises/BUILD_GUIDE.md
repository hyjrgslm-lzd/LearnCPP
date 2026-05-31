# 构建指南

## 前置条件

- **CMake** >= 3.28
- **C++26 编译器（协程 + `<generator>`）**：MSVC 17.10+（VS 2022 17.10+）/ Clang 18+ / GCC 14+
- **Git**（FetchContent 拉取 stdexec / asio 时使用）
- **网络连接**（首次配置 stage 3 时自动从 GitHub 拉取依赖；之后有缓存）

## 快速开始

### 方式 1：命令行（推荐）

```bash
cd exercises

# 配置（默认开启 stage3，首次需要拉取 stdexec / asio，可能需要几分钟）
cmake --preset default

# 构建全部习题
cmake --build build

# 或者只构建某一道题
cmake --build build --target A1_first_generator
```

### 方式 2：跳过 stage 3（更快的首次配置）

stage 3（模块 H/I/J + 两个第三阶段结课）依赖 stdexec / asio / liburing / folly / cobalt，
首次配置时间较长。如果你只想做 stage 1/2，可以禁用 stage 3：

```bash
cmake --preset default -DCOROUTINE_STUDY_ENABLE_STAGE3=OFF
cmake --build build
```

之后再开启时重新配置即可。

### 方式 3：Visual Studio

```bash
cd exercises
cmake --preset vs2022
```

然后用 VS 打开 `build-vs2022/coroutine_study_exercises.sln`。

### 方式 4：Ninja

```bash
cd exercises
cmake --preset ninja
cmake --build build-ninja
```

### 方式 5：VS Code + CMake Tools

1. 用 VS Code 打开 `exercises/` 目录
2. CMake Tools 扩展会自动检测 `CMakePresets.json`
3. 选择 preset 后直接构建

## 三阶段说明

| 阶段       | 模块                                     | 默认状态  | 备注                                                |
| ---------- | ---------------------------------------- | --------- | --------------------------------------------------- |
| Stage 1    | A / B / C + 第一阶段结课                 | 永远开启  | 仅依赖 C++23 `<generator>` 与随附的 `lazy_task.hpp` |
| Stage 2    | D / E / F / G                            | 永远开启  | 同上，无第三方依赖                                  |
| Stage 3    | H / I / J + Capstone 4 / Capstone 5      | 默认开启  | 需要 stdexec / asio / 等第三方依赖；首次配置较慢    |

> stage 3 中 `I3_folly_safe_task`（Folly 风格 SafeTask）和 `I4_cobalt_channel`（Boost.Cobalt channel）
> 仅在非 Windows 平台启用；`I2_io_uring_iocp` 跨平台——Linux 走 io_uring、Windows 走 IOCP。

## 离线模式

如果无法在线拉取依赖，可以把仓库手动放到 `third_party/` 目录下：

```bash
cd exercises/third_party
git clone https://github.com/NVIDIA/stdexec.git
git clone https://github.com/chriskohlhoff/asio.git
```

然后 `cmake/ThirdPartySetup.cmake` 的 `find_package` / 本地路径分支会优先生效。
详见 `third_party/README.md`。

## 代理（proxy）注意事项

stage 3 首次配置需经 `FetchContent` 从 GitHub 拉取 stdexec / asio。若在代理后，给 git/curl
设置代理环境变量即可：

```bash
export http_proxy=http://127.0.0.1:7897
export https_proxy=http://127.0.0.1:7897
```

> ⚠️ **不要同时设置大小写两份**（如同时 `http_proxy` 与 `HTTP_PROXY`）。Visual Studio
> 生成器在 `FetchContent` 的 populate 子构建里用 MSBuild 启动 git，其环境变量字典**大小写不敏感**，
> 同名两份会触发 `error MSB6001: 字典中已添加关键字`，导致配置失败。**只设小写一份**即可（git/curl 均认）。

## 锁定依赖版本

在 `cmake/ThirdPartySetup.cmake` 的 `FetchContent_Declare` 中把 `GIT_TAG main` 改为
具体的 commit hash：

```cmake
FetchContent_Declare(
    stdexec
    GIT_REPOSITORY https://github.com/NVIDIA/stdexec.git
    GIT_TAG        abc123def456    # 替换为你想锁定的 commit
    GIT_SHALLOW    TRUE
)
```

asio 同理。

## 编译器特殊说明

### MSVC

- 已在顶层 CMakeLists.txt 中配置 `/Zc:__cplusplus`、`/utf-8`、`/Zc:preprocessor`。
- 需要 17.10+ 才能稳定支持 C++23 `<generator>` 与 P2502R2 `co_yield std::ranges::elements_of(...)`。
- 如遇到 ICE（内部编译器错误），先升级到最新 VS 版本。

### GCC

- 需要 14+ 才能完整支持 `<generator>` 与 P2502 symmetric transfer。
- 协程相关 flag 已由 stdexec 子项目自动启用（stage 3 路径）。

### Clang

- 需要 18+ 才能稳定支持 `<generator>`。
- 注意：Clang 的 `<generator>` 实现依赖 libstdc++ 或 libc++ 的版本，必要时显式指定 `-stdlib=libc++` 并用最新 libc++。

## 目录结构

```
exercises/
├── CMakeLists.txt                  # 顶层构建（列出全部 33 题 + 5 结课）
├── CMakePresets.json               # 预设：default / debug / ninja / ninja-debug / vs2022 / vs2026
├── BUILD_GUIDE.md                  # 本文件
├── .gitignore
├── cmake/
│   └── ThirdPartySetup.cmake       # stage3 依赖（stdexec / asio / liburing / folly / cobalt）
├── include/
│   └── coroutine_study/
│       └── lazy_task.hpp           # 模块 A-2 起共用的 30 行最小 lazy_task<T>
├── third_party/                    # 离线依赖放置位置（可选）
│   └── README.md
├── A1_first_generator/             # 每道题一个子目录：CMakeLists.txt + main.cpp + README.md
│   ├── CMakeLists.txt
│   ├── main.cpp                    # 含 // TODO [必做] 与 // TODO [进阶]
│   └── README.md                   # 摘抄主文档的"目标 / 必做任务 / 验收点"
├── A2_co_return_lazy_task/
├── ...                              # 共 33 个习题 + 5 个结课子目录
```

## 做题流程

1. 先读对应模块的设计文档（如 `02-模块A-三关键字与最小协程.md`）。
2. 打开题目目录下的 `README.md`，对照"目标 / 必做任务 / 验收点"。
3. 打开 `main.cpp`，骨架代码已可编译；搜索 `// TODO [必做]:` 标记，按提示填写。
4. 构建并运行该题：
   ```bash
   cmake --build build --target A1_first_generator
   ./build/A1_first_generator/A1_first_generator        # Linux/macOS（单配置生成器）
   ./build/A1_first_generator/Release/A1_first_generator.exe   # MSVC（default preset，build/ 目录）
   # 若用 vs2026/vs2022 preset，构建目录改为 build-vs2026/build-vs2022：
   #   cmake --build build-vs2026 --config Release --target A1_first_generator
   #   ./build-vs2026/A1_first_generator/Release/A1_first_generator.exe
   ```
5. 完成必做后，搜索 `// TODO [进阶]:` 做进阶任务。
6. 跨文件复用 `lazy_task<T>` 时，只需 `#include <coroutine_study/lazy_task.hpp>`（顶层目录的 `include/` 已加入题目子项目的搜索路径）。
