# 构建指南

## 前置条件

- CMake >= 3.28
- 支持 C++23 coroutine / `<generator>` 的编译器：MSVC 17.10+、GCC 14+、Clang 18+
- Git：仅在启用 `COROUTINE_STUDY_FETCH_DEPS=ON` 时需要
- 可选系统依赖：liburing 2.15、Folly v2026.08.31.00、Boost 1.92 Cobalt

## 快速开始

默认只配置 core 练习，不拉第三方依赖：

```bash
cd Coroutine_Study/exercises
cmake --preset student
cmake --build --preset student
```

带参考答案 target 的核心验证：

```bash
cmake --preset verify-core
cmake --build --preset verify-core
ctest --preset verify-core
```

旧 preset 仍可用：`default`、`debug`、`ninja`、`ninja-debug`、`vs2022`、`vs2026`。它们现在等价于 core 构建入口。

## 功能闸门

所有重依赖默认关闭，显式打开才纳入构建图；打开后如果依赖不存在，CMake 会直接失败。

| 选项 | 默认 | 作用 |
| --- | --- | --- |
| `COROUTINE_STUDY_BUILD_REFERENCE` | OFF | 构建 `<name>_reference` 参考 target |
| `COROUTINE_STUDY_FETCH_DEPS` | OFF | 允许 FetchContent 拉轻依赖 |
| `COROUTINE_STUDY_ENABLE_STDEXEC` | OFF | H1/H2/H3、Capstone5 |
| `COROUTINE_STUDY_ENABLE_ASIO` | OFF | I1、Capstone4 |
| `COROUTINE_STUDY_ENABLE_CPPCORO` | OFF | I5_cppcoro_patterns |
| `COROUTINE_STUDY_ENABLE_IO_URING` | OFF | Linux I2 |
| `COROUTINE_STUDY_ENABLE_IOCP` | OFF | Windows I2 |
| `COROUTINE_STUDY_ENABLE_FOLLY` | OFF | I3 heavy lane |
| `COROUTINE_STUDY_ENABLE_COBALT` | OFF | I4 heavy lane |
| `COROUTINE_STUDY_ENABLE_UNSAFE_DEMOS` | OFF | 有意展示 UB/危险生命周期的 demo |

`COROUTINE_STUDY_ENABLE_STAGE3` 只保留兼容：打开后会启用 light lane（stdexec、Asio、cppcoro、当前平台 IO）。Folly 和 Cobalt 仍需单独显式打开。

## 常用 preset

```bash
cmake --preset full-windows
cmake --build --preset full-windows

cmake --preset full-linux-light
cmake --build --preset full-linux-light

cmake --preset heavy-folly-linux
cmake --build --preset heavy-folly-linux

cmake --preset heavy-cobalt-linux
cmake --build --preset heavy-cobalt-linux
```

`full-windows`、`full-linux-light` 和 heavy preset 默认打开 `COROUTINE_STUDY_BUILD_REFERENCE=ON`，用于 CI 验证参考实现。

## 锁定依赖

轻依赖版本固定在 `cmake/ThirdPartySetup.cmake`：

| 依赖 | 固定版本 |
| --- | --- |
| stdexec | `nvhpc-26.05` |
| Asio | `asio-1-38-2` |
| cppcoro | `8642e98596a92be30a2b061d3ed306d959d3214e` |

cppcoro 不是纯 header-only 依赖；CMake 会接入它的真实 `cppcoro` target，再桥接为 `cppcoro::cppcoro`。

离线覆盖使用 CMake 标准变量：

```bash
cmake --preset full-linux-light \
  -DFETCHCONTENT_SOURCE_DIR_STDEXEC=/path/to/stdexec \
  -DFETCHCONTENT_SOURCE_DIR_ASIO=/path/to/asio \
  -DFETCHCONTENT_SOURCE_DIR_CPPCORO=/path/to/cppcoro
```

## Heavy 依赖

liburing 通过 pkg-config 查找，要求 2.15 或更高：

```bash
sudo apt-get install pkg-config liburing-dev
pkg-config --modversion liburing
```

Folly 使用外部安装，不由本课程 CMake 自动构建：

```bash
git clone --branch v2026.08.31.00 https://github.com/facebook/folly.git
cd folly
python3 ./build/fbcode_builder/getdeps.py install-system-deps --recursive
python3 ./build/fbcode_builder/getdeps.py build --allow-system-packages
```

然后把 Folly 安装前缀加入 `CMAKE_PREFIX_PATH`。

Boost.Cobalt 需要 Boost 1.92，并提供 `Boost::cobalt`：

```bash
cmake --preset heavy-cobalt-linux -DCMAKE_PREFIX_PATH=/path/to/boost-1.92
```

## 编译器说明

核心练习使用 C++23。需要 C++26 预览的题目应只对自己的 target 调用 `coroutine_study_enable_cxx26_preview(target)`，不要再全局添加 `/std:c++latest` 或 `-std=c++2c`。

MSVC 顶层只加 `/Zc:__cplusplus /utf-8 /Zc:preprocessor`。Clang/GCC 只在 target 明确需要 C++26 预览时加 `-std=c++2c`。
