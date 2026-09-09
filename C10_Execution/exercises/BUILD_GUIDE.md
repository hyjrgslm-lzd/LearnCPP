# 构建指南

## 前置条件

- **CMake** >= 3.25
- **C++20 编译器**：GCC 12+、Clang 16+、MSVC 19.34+（VS 2022 17.4+）
- **Git**（FetchContent 需要用它拉取 stdexec）
- **网络连接**（首次配置时自动从 GitHub 拉取 stdexec；之后有缓存）

## 快速开始

### 方式 1：命令行（推荐）

```bash
cd exercises

# 配置（首次会自动拉取 stdexec，可能需要几分钟）
cmake --preset default

# 构建全部习题
cmake --build build

# 或者只构建某一道题
cmake --build build --target A1_two_executions
```

### 方式 2：Visual Studio

```bash
cd exercises
cmake --preset msvc
```

然后用 VS 打开 `build-msvc/C10_Execution.sln`；使用生成 `.slnx` 的新版生成器时，打开同名 `.slnx` 文件。

### 方式 3：Ninja

```bash
cd exercises
cmake --preset ninja
cmake --build build-ninja
```

### 方式 4：VS Code + CMake Tools

1. 用 VS Code 打开 `exercises/` 目录
2. CMake Tools 扩展会自动检测 CMakePresets.json
3. 选择 preset 后直接构建

## 离线模式

如果无法在线拉取 stdexec：

```bash
cd exercises/third_party
git clone https://github.com/NVIDIA/stdexec.git
```

然后编辑 `CMakeLists.txt`，注释掉方案 A，取消注释方案 B。

## 锁定 stdexec 版本

在 `CMakeLists.txt` 的 `FetchContent_Declare` 中把 `GIT_TAG main` 改为具体的 commit hash：

```cmake
FetchContent_Declare(
    stdexec
    GIT_REPOSITORY https://github.com/NVIDIA/stdexec.git
    GIT_TAG        abc123def456    # 替换为你想锁定的 commit
    GIT_SHALLOW    TRUE
)
```

## 编译器特殊说明

### MSVC

- 已在 CMakeLists.txt 中配置 `/Zc:__cplusplus` 和 `/utf-8`
- 如遇到 ICE（内部编译器错误），尝试升级到最新 VS 版本

### GCC

- 已自动启用 `-fcoroutines` 和 `-fconcepts-diagnostics-depth=10`（由 stdexec 配置）

### Clang

- 需要 Clang 16+ 以获得完整的 C++20 协程支持

## 目录结构

```
exercises/
├── CMakeLists.txt          # 顶层构建，FetchContent 拉取 stdexec
├── CMakePresets.json        # 预设配置（default / debug / msvc / ninja）
├── .gitignore
├── third_party/             # 离线 stdexec 放置位置
│   └── README.md
├── A1_two_executions/       # 每个习题一个子目录
│   ├── CMakeLists.txt       #   子项目构建
│   ├── main.cpp             #   代码填空模板
│   └── README.md            #   习题说明
├── A2_value_channel/
│   └── ...
└── ...（共 26 个习题子目录）
```

## 做题流程

1. 打开某道题的 `main.cpp`
2. 阅读同目录下的 `README.md` 了解题目要求
3. 搜索 `// TODO [必做]:` 标记，按提示填写代码
4. 构建并运行该题：`cmake --build build --target A1_two_executions && ./build/A1_two_executions`
5. 完成必做后，搜索 `// TODO [进阶]:` 做进阶任务
