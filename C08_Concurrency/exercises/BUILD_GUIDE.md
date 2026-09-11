# 构建、运行与复现实验

所有相对命令默认在 `C08_Concurrency/exercises` 执行。先运行正确性检查，再运行性能实验；测试通过的范围和未验证平台由本机运行记录说明，不随课程源码提交。

## 1. 工具链与核心构建

本轮Windows验证显式使用已经安装的Python3.13.11，避免PATH中的3.10被误用来启动要求3.11+的benchmark工具：

```powershell
$C08Python = 'C:/Users/zhidan.li/AppData/Roaming/uv/python/cpython-3.13.11-windows-x86_64-none/python.exe'
& $C08Python --version
cmake --preset verify-core "-DPython3_EXECUTABLE=$C08Python"
```

其他机器选择自己的Python3.11+绝对路径，不需要复制本机用户名或改全局PATH。普通材料工具能在3.10运行，不代表benchmark runner也满足要求。新runner会在低版本入口清楚拒绝，摘要算法保持不变。

根CTest的两项工具检查要求Python3.10+，找不到解释器将直接停止配置，不能静默减少测试。PowerShell调用记录器时，`"-DPython3_EXECUTABLE=完整路径"`整个参数必须加引号；配置后核对缓存中的完整路径以及`runtime_benchmark_tools`、`runtime_materials`确实注册。只编译C++且不运行测试可显式设置`BUILD_TESTING=OFF`。

需要 CMake 3.28 或以上，以及满足实际课程用法的 C++23 编译器和标准库。Visual Studio 18 2026 生成器从 CMake 4.2 起提供。本机验收使用 CMake 4.2.3、MSVC 19.51.36256、Windows x64。

这里的 C++23 表示代码及 CMake target 的最低要求。CMake 4.2.3 在本机 MSVC 上将该要求映射为 /std:c++latest；实际选项以构建日志为准。若要明确限制到本机的 C++23 预览模式，可按专题验证命令使用 /std:c++23preview。是否启用原生 C++26 课程分支仍由独立探测和能力宏决定。

```powershell
cmake --preset verify-core
cmake --build --preset verify-core --parallel 4
ctest --preset verify-core
```

`verify-core` 构建 Starter、已有 Reference 和 runtime tests。默认不开启第三方下载、危险示例或长性能实验。CTest 为测试设置外部超时；超时属于失败。返回码 77 表示某个明确依赖条件不具备，应查看对应 SKIP 原因。

只构建 Starter：

```powershell
cmake --preset student
cmake --build --preset student --parallel 4
```

原有 `vs2026`、`vs2022`、`default`、`debug`、`ninja`、`ninja-debug` 入口保留。更换生成器或工具链时使用不同构建目录。机器有多个 Visual Studio 实例时，可显式指定本机路径：

```powershell
cmake --preset vs2026 -DCMAKE_GENERATOR_INSTANCE="实际安装目录"
```

## 2. 单题运行

以队列基线为例：

```powershell
cmake --build --preset verify-core --target Q0_queue_baseline Q0_queue_baseline_reference
./build/verify-core/Q0_queue_baseline/Release/Q0_queue_baseline.exe
ctest --test-dir build/verify-core -C Release -R "^Q0_queue_baseline_reference$" --output-on-failure
```

Visual Studio 是多配置生成器，程序通常位于题目目录下的 Release 或 Debug 子目录。Ninja 等单配置生成器通常没有该层目录。以上路径以实际构建输出为准。

也可独立配置某题：

```powershell
cmake -S Q0_queue_baseline -B build/q0 -G "Visual Studio 18 2026" -A x64
cmake --build build/q0 --config Release
ctest --test-dir build/q0 -C Release --output-on-failure
```

普通题的 Reference 使用 cs::check，Release 中不会因 NDEBUG 消失。程序仅编译成功或正常退出，不能自动说明全部学习目标已完成；应看该题实际检查的契约。

### 验证学生程序与验证参考答案

默认 CTest 注册的是 Reference。修改 main.cpp 后，应另外运行学生程序，不能把 Reference 通过当作学生代码通过。需要 CTest 的外部超时，可对单题开启学生测试，例如：

```powershell
cmake -S C2_bounded_queue_condvar -B build/student-C2 -DBUILD_TESTING=ON -DCONCURRENCY_STUDY_TEST_STARTERS=ON
cmake --build build/student-C2 --config Release --target C2_bounded_queue_condvar
ctest --test-dir build/student-C2 -C Release -R "^C2_bounded_queue_condvar_student$" --output-on-failure
```

每题的 CMake 显式声明 main 的用途，不扫描 TODO 或根据退出码猜测题型：

| 用途 | CTest 名称后缀 / label | 通过的含义 |
|---|---|---|
| IMPLEMENTATION：实现型 Starter | _student / student | 学生操作通过已实现的契约检查；未完成返回 1 |
| OBSERVATION：观察实验或基准驱动 | _observation / observation | 仅本次实际运行的检查通过，不代表全部 Part 或实验报告完成 |
| Reference：solution.cpp | _reference / reference | 仓库参考实现的检查通过，不代表学生代码通过 |

开启上述选项后，Q0 等观察入口注册为 `Q0_queue_baseline_observation`。长采样仍由基准 runner 单独运行，一次 observation 通过不覆盖多规模和重复测量。核心门禁默认不运行学生作业或观察入口；77 仅用于明确的能力缺失，不表示“作业没写”。学生应编辑独立作业位置，不能通过修改共享 Reference 来改变核对依据。

## 3. 固定版本依赖

完整 Windows 配置显式允许下载固定依赖，并加入基准目标：

```powershell
cmake --preset full-windows
cmake --build --preset full-windows --parallel 4
ctest --preset full-windows
```

完整 Windows preset 还开启 `CONCURRENCY_STUDY_TEST_FAST_MATH`：`numeric_fast_check` 用严格浮点选项编译独立判分单元，只将另一个内核单元按 fast 模式编译；两侧及链接都禁用 LTO/IPO，CTest 有 30 秒外部上限。这不是把整个数值 Reference 改成 fast 模式。其他配置默认关闭该专项，需要时可显式设置 `-DCONCURRENCY_STUDY_TEST_FAST_MATH=ON`，构建 `numeric_fast_check` 并用同名 CTest 过滤。具体单元、编译守卫和手工复现见[严格判分与 fast 内核](../topics/simd/fast-math-build.md)。

| 依赖 | 固定版本 | 固定源提交 |
|---|---|---|
| xsimd | 13.2.0 | `1f8dd9c8e162968d9b4ff0251c56d431b8777f36` |
| stdexec | nvhpc-26.05 | `6d7ad689f4d4831c5136e4abe1c601f9a3b64e43` |

下载源保存在相应 build 目录的 _deps 中。核心构建不依赖它们。已有源码可以通过 `CONCURRENCY_STUDY_XSIMD_SOURCE_DIR` 或 `CONCURRENCY_STUDY_STDEXEC_SOURCE_DIR` 提供；使用干净、位于固定提交的 Git checkout，配置会核验 include 的修改状态。已有 xsimd 安装仅接受 13.2.0，若无法验证源提交会在来源记录中明确标注。

需要代理时在当前终端配置本机所需代理后运行 CMake；课程不写入全局代理或 Git 设置。显式关闭下载且没有依赖时，配置给出缺失原因，不悄悄下载其他版本。

并行标准算法的支持由所用标准库决定。构建会查找可用 TBB 并实际编译、链接 policy 重载；不能只凭 Clang/GCC 名称推断后端。编译成功也不意味着每次运行一定使用多个线程。

## 4. 原生 C++26 验证

本轮同时增加 `native-cxx29` preset；它显式请求C++26和C++29两组设施。分别控制的选项为 `CONCURRENCY_STUDY_ENABLE_CXX26`、`CONCURRENCY_STUDY_ENABLE_CXX29`，可以只请求其中一组。每个能力的原始日志在构建目录 `capabilities/`；关闭时登记DISABLED，启用后的最小探测失败与主体失败分开。

```powershell
cmake --preset native-cxx29 "-DPython3_EXECUTABLE=$C08Python"
cmake --build --preset native-cxx29 --parallel 4
ctest --preset native-cxx29
```

F01/F02是两项C++29单元；F03的标准HP、RCU、sender分别构建、分别运行。教学模型、摘要和原生主体不是同一种验证。已有A2、E3及K系列的C++26分支仍按各自实际能力检查，不能仅凭核心通过推断扩展也通过。

```powershell
cmake --preset native-cxx26
cmake --build --preset native-cxx26 --parallel 4
ctest --preset native-cxx26
```

这个入口使用编译器的预览语言选项并分别探测 SIMD、senders、HP、RCU、atomic min/max 和 inplace stop。探测日志在 build/native-cxx26/capabilities。第三方 xsimd 或 stdexec 成功，不能让对应的标准能力宏变成 true。

MSVC 使用 /std:c++latest，GCC/Clang 使用 -std=c++2c；旧工具链不接受该选项时，使用核心 C++23 入口。标准 SIMD 的探测要求本课程采用的 N5050 接口，早期或部分接口可能未通过该完整门槛，需检查日志。

## 5. 性能实验与原始记录

无第三方依赖的基准：

```powershell
cmake --preset bench
cmake --build --preset bench --parallel 4
ctest --preset bench
```

各专题 README 列出实际支持的具体 variant 和参数。先运行该题的 Reference，再用统一 runner 采样。runner 需要 Python 3.11 或以上，所有依赖均来自 Python 标准库。

```powershell
python tools/run_benchmarks.py --help
python tools/test_tools.py
```

runner 接收一个 benchmark 可执行文件、一组先运行的 Reference、若干具体 variant，以及传给 C++ 程序的参数。下面是参数形状，EXE 与 REFERENCE 应替换为该专题构建输出，VARIANT 替换为该程序支持的具体版本：

```text
python tools/run_benchmarks.py --exe EXE --check REFERENCE --variant VARIANT --output build/results/new-run -- --size 100000
```

每个 C++ 子进程只运行一轮。默认每个 variant 先预热一轮，再五次独立进程采样，固定种子安排顺序。输出目录必须是新目录，避免覆盖已有记录。run.json 保存命令、源码与可执行文件摘要、工具链设置、原始 stdout/stderr、超时和 SKIP、每次计时及汇总；samples.csv 保存正式样本。

每个进程中同一 case 只能出现一次，每个 case 必须覆盖全部正式采样轮次；一轮重复输出五行不能冒充五次独立运行。部分 variant 缺条件时，结果保存为 PARTIAL_SKIP，默认返回 77；只有显式选择 --allow-partial 才允许在记录缺失范围的同时以 0 返回。失败样本不能借此选项通过。

程序自身报告的毫秒数与外部进程总时长分别记录；初始化、分配、线程创建、排空、回收是否在计时内，由各专题定义，并在同组比较中保持一致。CSV 的 completed 表示成功完成量。无实际硬件证据的 NUMA 远端试验不产生有效排名。

CSV 的 threads 为正数时表示程序报告的线程数；0 明确表示后端管理的实际线程数没有测量，不是“零线程执行”。例如标准 par 算法不能仅凭硬件线程提示数来填写其实际并发数。该标记与需要正线程数量的命令行参数不同，解释见每条 details。

## 6. 诊断与工具边界

普通路径不运行故意的 UB 或永久死锁。已提供的诊断资产必须显式开启或单独构建，并通过外部进程超时运行。ASan 检查地址访问，TSan 检查其支持环境中的数据竞争；它们不是内存序或进展保证的证明。

回收专题提供了[独立验证说明](../topics/reclamation/06-validation.md)，包括优化构建与 ASan 的实际范围。其他平台和工具的可用性以本机配置为准，未运行的检查不会被记录为通过。

## 7. Windows 与 WSL 验证矩阵

| 入口 | 环境与作用 |
|---|---|
| `verify-core` / `debug` | 核心Release/Debug；核心默认不下载依赖 |
| `full-windows` | 固定xsimd/stdexec、fast独立判分和基准；日志需另加SPDLOG选项 |
| `asan-windows` | MSVC RelWithDebInfo；地址检测、非增量链接、复制匹配ASan运行库 |
| `linux-core` / `linux-debug` | WSL GCC，Ninja，分别Release/Debug |
| `linux-asan` | WSL Clang18，RelWithDebInfo，ASan+UBSan |
| `linux-tsan` | WSL Clang18，RelWithDebInfo，单独TSan，不与ASan混用 |

`CONCURRENCY_STUDY_SANITIZER`接受`none/address/thread`。普通测试外部上限30秒，Sanitizer下的C++测试120秒；Python工具仍有30秒上限。MSVC的地址检测使用RelWithDebInfo，避免Debug默认的`/RTC1`冲突。Linux本课不使用module接口，因此关闭CMake模块扫描；Clang/libstdc++的运行时原子查询链接工具链提供的`atomic`库。

```powershell
cmake --preset asan-windows "-DPython3_EXECUTABLE=$C08Python"
cmake --build --preset asan-windows --parallel 4
ctest --preset asan-windows
```

Linux必须在guest中执行其预设，不从Windows生成器复用缓存：

```bash
# 已核对的guest源码快照内，进入C08_Concurrency/exercises
cmake --preset linux-core
cmake --build --preset linux-core --parallel 4
ctest --preset linux-core
```

其余Linux预设使用同样三步。专用镜像、快照、实际工具版本和TSan正常/竞争控制应记录在本机验证目录，不提交为课程文档。能力控制失败时先诊断环境；能够运行检测器之后出现的主体报告是失败，不能通过更改全局安全设置、压制报告或缩小源码快照来制造通过。多NUMA节点实验仍取决于实际拓扑；WSL不替代裸机性能证据。

新增日志单元默认关闭。打开时沿用已固定的fmt/spdlog源码；Windows可使用C05已有下载，Linux必须指定guest实际源码路径，不能直接复用Windows marker：

```powershell
cmake -S U01_async_logging -B build/u01 -G "Visual Studio 18 2026" -A x64 `
  -DCONCURRENCY_STUDY_ENABLE_SPDLOG=ON `
  "-DCONCURRENCY_STUDY_FMT_SOURCE_DIR=实际fmt12.1.0干净Git源码目录" `
  "-DCONCURRENCY_STUDY_SPDLOG_SOURCE_DIR=实际spdlog1.17.0干净Git源码目录"
cmake --build build/u01 --config Release --target U01_async_logging_checked
ctest --test-dir build/u01 -C Release -R U01_async_logging --output-on-failure
```

完整操作、Student作业位置与good/bad判分见[U01](U01_async_logging/README.md)。两个库的完整编译输入必须匹配固定commit且无跟踪修改；不能只校验头文件而允许src/CMake漂移。扩展不新增日志全局注册或机器级依赖。

队列计数是单独的`queue_diagnostics`目标，仅它带`CS_QUEUE_DIAGNOSTICS=1`。正式`queue_bench`不启用计数，诊断结果不能当成计时结果。运行入口与正式测量窗口约束见[队列验证与基准](../topics/queues/08-validation-and-benchmark.md)。

返回[课程入口](../README.md)。
