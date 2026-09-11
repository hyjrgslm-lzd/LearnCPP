# 本机编译与运行

本课程包含 34 道独立练习和 3 个项目。先编译预备单元与核心练习，再根据正在学习的框架开启相应依赖。

## 准备条件

- CMake 3.28 或更新版本。
- 能编译 C++23 的编译器与标准库；generator 练习需要标准库提供 `<generator>`。
- 启用依赖下载时，需要 Git 和相应网络访问。

编译器前端和标准库共同决定可用能力。例如 Clang 可以搭配不同标准库；遇到头文件或库类型缺失时，先确认实际使用的那组工具链。各练习的 CMake 文件声明了目标使用的语言版本。

## 编译当前课程

在 `C09_Coroutines/exercises` 执行：

```powershell
cmake --preset verify-core
cmake --build --preset verify-core
```

这会生成 Starter 与 Reference。普通练习的目标名与目录名相同，参考目标在其后加 `_reference`。例如只学习 future 基础时：

```powershell
cmake --build --preset verify-core --target P1_future_basics P1_future_basics_reference
```

只构建 Starter 可以使用 `student` 预设：

```powershell
cmake --preset student
cmake --build --preset student
```

现有的 `default`、`debug`、`ninja`、`ninja-debug`、`vs2022`、`vs2026` 入口仍可使用。首次配置一个构建目录时，选择本机已安装的 CMake 生成器；后续沿用该目录的生成器配置。

## 找到并运行程序

Visual Studio 使用多配置输出目录。默认 Release 构建的 P1 程序位于：

```powershell
./build/verify-core/P1_future_basics/Release/P1_future_basics.exe
./build/verify-core/P1_future_basics/Release/P1_future_basics_reference.exe
```

Ninja、Unix Makefiles 等单配置 CMake 生成器通常直接将程序放在对应的练习构建目录，例如 `build/verify-core/P1_future_basics/P1_future_basics_reference`。Windows 可执行文件带 `.exe`。以构建输出中显示的最终路径为准。

先运行 Starter，按正文预测并观察输出，然后逐步补全 TODO。Reference 提供完整结果；许多示例也用 `coroutine_study::check` 检查关键行为，失败时会指出观察条件。课程正文说明各个结果为什么出现。

`student` preset 会显式开启 `COROUTINE_STUDY_TEST_STARTERS=ON`。默认核心验证不注册未完成 starter，避免 `verify-core` 的 Reference 结果被学生 TODO 污染。

`ctest` 标签区分检查含义：

- `reference`：完整答案或课程运行时检查，应该通过。
- `starter`：学生起点检查。未完成 TODO 时可以失败，但必须安全、有限、明确失败，不能用 skip 文本返回 0；需要通过 `COROUTINE_STUDY_TEST_STARTERS=ON` 显式注册。
- `runtime`、`rpc`、`stdexec`：对应运行时、RPC 或可选 stdexec 覆盖。

默认自动测试有进程外 `TIMEOUT`。普通课程和 mini 库测试为 30 秒，RPC reference 为 60 秒；个别题目可按自身 CMake 保留更短限制。

项目的可执行目标名与目录结构见各项目 README：

- [异步小爬虫](Capstone1_async_crawler/README.md)
- [RPC 框架](Capstone4_rpc_framework/README.md)
- [mini 协程库](Capstone5_mini_corolib/README.md)

## 学到第三方框架时

选择与当前机器和当前主题相符的预设即可。Windows 上，现有 `full-windows` 配置包含 stdexec、Asio、cppcoro 与 IOCP：

```powershell
cmake --preset full-windows
cmake --build --preset full-windows --target I1_asio_echo I1_asio_echo_reference
```

Linux 的现有入口有 `full-linux-light`、`heavy-folly-linux`、`heavy-cobalt-linux`，分别用于相应平台与依赖。它们提供选择不同课程内容的构建入口。平台专有实现的背景和阅读方式在对应 I 模块讲义中展开。

也可以在本机预设基础上按主题选择构建内容：

| CMake 选项 | 纳入的内容 |
| --- | --- |
| `COROUTINE_STUDY_BUILD_REFERENCE` | 完整 Reference 目标 |
| `COROUTINE_STUDY_FETCH_DEPS` | 允许 CMake 下载固定版本的轻依赖 |
| `COROUTINE_STUDY_ENABLE_STDEXEC` | H1–H3 与 mini 库桥接扩展 |
| `COROUTINE_STUDY_ENABLE_ASIO` | I1 与 RPC 项目 |
| `COROUTINE_STUDY_ENABLE_CPPCORO` | I5 |
| `COROUTINE_STUDY_ENABLE_IOCP` | Windows I2 |
| `COROUTINE_STUDY_ENABLE_IO_URING` | Linux I2 |
| `COROUTINE_STUDY_LIBURING_ROOT` | 已有 liburing >= 2.15 安装前缀，使用其 include/lib，不安装系统组件 |
| `COROUTINE_STUDY_ENABLE_FOLLY` | Folly I3 |
| `COROUTINE_STUDY_ENABLE_COBALT` | Cobalt I4 |
| `COROUTINE_STUDY_ENABLE_UNSAFE_DEMOS` | J1 中明确说明的危险用法示例 |

`COROUTINE_STUDY_ENABLE_STAGE3` 是兼容入口，会组合开启 stdexec、Asio、cppcoro 与当前平台 I/O。新配置直接选择对应主题的选项更容易看清依赖关系。

## 依赖来源

`cmake/ThirdPartySetup.cmake` 保存本课程的获取与查找方式：

| 依赖 | 仓库使用的版本或查找条件 |
| --- | --- |
| stdexec | `nvhpc-26.05` |
| Asio | `asio-1-38-2` |
| cppcoro | `8642e98596a92be30a2b061d3ed306d959d3214e` |
| liburing | 通过 pkg-config 查找，要求 2.15 或更新 |
| Folly | 使用本机安装提供的 CMake target |
| Boost.Cobalt | 使用本机 Boost 1.92 提供的 `Boost::cobalt` |

已有源码副本可以通过 CMake 的 `FETCHCONTENT_SOURCE_DIR_STDEXEC`、`FETCHCONTENT_SOURCE_DIR_ASIO`、`FETCHCONTENT_SOURCE_DIR_CPPCORO` 变量指定。已有安装可以通过 `CMAKE_PREFIX_PATH` 或相应包查找变量提供。cppcoro 会作为实际库目标参与链接。

需要 C++26 预览能力的具体目标使用仓库的 `coroutine_study_enable_cxx26_preview` 设置编译选项。H2 的 README 说明该题采用的 stdexec 类型及本机工具链条件。

## 已有本机依赖的可复验矩阵

以下命令从 LearnCPP 仓库根运行。本机 Windows 使用 VS 18 2026；脚本只在本课 `exercises/build/` 内写构建产物。`configure_windows.py --light` 核对 stdexec/Asio/cppcoro 的固定 SHA，并复用旧缓存中的 RAPIDS、CPM、execution.bs，避免上游 bootstrap 再次联网。其他机器需先按上节取得固定依赖，或通过 `--deps` 指向等价的本地缓存；该脚本不安装任何组件。

```powershell
python C09_Coroutines/exercises/tools/run_matrix.py windows
python C09_Coroutines/exercises/tools/run_matrix.py student
wsl -d LearnCPP-C08-Ubuntu-24.04 --cd /mnt/f/CPPTrain/LearnCPP -- python3 C09_Coroutines/exercises/tools/run_matrix.py linux
```

Windows 路线构建 Debug/Release 并运行完整已启用验证；Student 路线关闭 Reference，逐个运行注册的学生操作，区分已提供观察/基础实现与未完成起点的拒绝，回读编译依赖检查答案隔离。它是“课程起点交付验证”，不是要求读者完成作业后仍失败；读者日常验收直接运行自己的 `ctest -L starter`。

WSL 路线复用本机已有 liburing 2.15，运行 I/O 与可用 Reference。`<generator>` 独立实例化探针失败时，只排除相应四项；其他失败仍算 FAIL。标准 `std::execution::task` 的 H2 probe 与 stdexec 练习分离，缺能力返回 77/Skipped；probe 通过后主体错误不能降为 SKIP。

原始命令、输出、超时/清理结果、源码指纹保存到 `references/validation/c09-refresh/final/` 的时间戳目录；历史失败不覆盖。监督器复用 C07 的 `run_test.py` 与 C01 的 `process_runner.py`。单题默认 30 秒、RPC 60 秒，bad wrapper 留出清理时间。Reference 关闭时不注册教学 runtime 的 reference 测试；它们仍可从 `runtime_tests` 独立构建。

RPC 的 `answer_check` 是 public API 对应的 Reference-adapted 答版，`protocol_good_check` 仅在 protocol Part 使用独立实现。两者随 Reference 开关启用；学生起点不使用 WILL_FAIL。底层被检测的 bad 必须在规定退出码和具体诊断上被拒绝，超时、崩溃不算反例验收通过。

性能取证在其他构建/压力负载结束后单独运行 F3 的 `sample_f3.py`。耗时、计数与无插桩编译器诊断分别解释，不预设 HALO 或加速比。

返回 [课程入口](../README.md)。
