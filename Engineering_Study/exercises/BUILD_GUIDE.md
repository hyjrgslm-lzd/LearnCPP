# 构建、运行与复现实验

## 1. 先区分四个动作

`cmake` 配置并生成构建系统；`cmake --build` 调用实际构建工具；可执行文件运行程序；`ctest` 运行已注册检查。配置成功没有证明所有源文件能编译，构建成功没有证明运行时 DLL 能找到，CTest 成功也只覆盖实际执行的检查。

从本目录运行核心路径：

```powershell
cmake --preset verify-core
cmake --build --preset verify-core --parallel 4
ctest --preset verify-core

cmake --preset verify-debug
cmake --build --preset verify-debug --parallel 4
ctest --preset verify-debug
```

核心代码要求 C++23，CMake 最低3.28。Windows 可显式选择 `-G "Visual Studio 18 2026" -A x64`；其他版本选择本机真实安装的生成器，不复用旧生成器的 build 目录。普通 PowerShell 无 `cl` 不等于没有 MSVC：Visual Studio 生成器可发现安装。若有多个实例，配置时用 `-DCMAKE_GENERATOR_INSTANCE=实际安装目录`，不要把个人路径提交到共享 preset。

VS 是多配置生成器：构建时的 `--config Release` / `Debug` 才选择实际配置，不能只看 `CMAKE_BUILD_TYPE`。Ninja 是单配置生成器，配置阶段的构建类型生效。本课 preset 将两者的 build/test 配置配对；实际语言选项仍以日志为准，例如 MSVC 的 C++23 CMake feature 可能映射到预览选项。

## 2. Student、Reference 与负例

| 入口/标签 | 成功意味着什么 |
|---|---|
| reference | 独立参考实现通过已执行的语义检查 |
| observation | 当前观察程序/符号检查运行成功，不代表完成所有解释题 |
| student | 检查实际学生代码；未完成时应失败，不能修改完成标记冒充实现 |
| negative | 隔离编译/链接/运行失败符合规定阶段、符号或诊断；不是任意报错都通过 |
| capability | 指定工具与接口的真实能力检查，不能从选项OFF推断接口不可用 |

默认核心不注册未完成学生作业。要检查学生路径，使用：

```powershell
cmake --preset student
cmake --build --preset student --parallel 4
ctest --preset student
```

初始实现型 Student 失败是作业尚未完成，不是课程 Reference 的通过证据。把 `return 1` 改成 `return 0`、删除检查或调用 Reference 都没有实现任务。

短检查的外部超时为30秒；含编译、安装、消费的子工程为180秒。超时和清理失败都是真失败，不能因为这是 negative 标签就接受。故意 UB 的诊断程序默认关闭；正常核心使用安全版本和编译/链接反例。

公共 `check()` 失败时向stderr写诊断并正常退出1，Debug与Release都生效；它不会抛一个可能被误当成被测API异常的检查器异常。被测API自身的异常仍用明确的catch和后置检查验证。`engineering_check_success/failure` 两项自检分别证明正例可运行、反例是正常非零退出而非崩溃或超时。

## 3. 单个练习

以 ODR 单元为例，既可在课程顶层只构建指定目标，也可独立配置这个练习。不要把尚未构建的其他目标计入该次 CTest 验证：

```powershell
cmake -S C1_odr -B build/one-C1 -G "Visual Studio 18 2026" -A x64
cmake --build build/one-C1 --config Release --parallel 4
ctest --test-dir build/one-C1 -C Release --output-on-failure
```

负例各自生成到 build 子目录，不破坏上面的默认构建。单题所需的公共 CMake/check 仍位于本课程目录内；“独立配置”不表示可以只复制一份 `.cpp` 而丢掉它的头、检查器和构建文件。

## 4. Modules 与 import std

先在 **x64 Native Tools** 环境确认 `cl`、`link`、`rc`、`mt`、`ninja` 与 `cmake` 指向预期工具。这里只影响当前构建进程，不需要修改系统 PATH。named modules、`import std`、模块包的安装导出是三项不同验证。

```powershell
cmake --preset modules-msvc-ninja
cmake --build --preset modules-msvc-ninja --parallel 4
ctest --preset modules-msvc-ninja

cmake --preset import-std-msvc-ninja
cmake --build --preset import-std-msvc-ninja --parallel 4
ctest --preset import-std-msvc-ninja
```

本次 `import std` 实验锁定 CMake4.2.3 的门控，在首次发现 CXX 工具链之前设置。存在 `std.ixx` 不足以证明 CMake 消费成功，还要检查 `modules.json`、`CMAKE_CXX_COMPILER_IMPORT_STD`、`CXX_MODULE_STD` 和真实编译/链接/运行。更换 CMake 版本时先核对对应版本的实验说明，不照抄旧 UUID。[固定版本说明](https://raw.githubusercontent.com/Kitware/CMake/v4.2.3/Help/dev/experimental.rst)。

普通核心没有请求这些分支，不能把“未请求”写成它们的 PASS 或能力失败。真正失败时保留失败阶段和输出；不要以 `#include` 替换 `import std` 后报告库模块通过。直接编译器路径与 CMake 集成路径也分别记录。

## 5. 诊断、调试与成本

G1/G2 的 Python 工具需要 **Python 3.11 或更新版本**（只用标准库）；C++ 核心构建本身不依赖 Python。先确认所调用解释器的实际 `--version`，不要把同名 shim 或旧报告中的版本当作当前运行环境。本次机器默认 `python` 是3.10，而验证显式使用已安装的3.13.11；换机器应选择自己的有效解释器路径，不复制作者路径，也不修改系统默认解释器。

调试实验先使用 `-g -O0` 或 Debug 产物，在变量完成初始化之后观察值；未初始化位置的 debugger 输出不是程序缺陷证据。MinGW/GDB 观察的是该工具链生成的 Windows 程序，不证明 GDB 能解读另一编译器的全部 PDB 信息。

Clang ASan/libFuzzer 程序在 Windows 需要匹配的运行库 DLL。只把匹配 LLVM runtime 目录加入该子进程的 PATH，构建通过后仍要实际运行；装载失败、检测报告和测试逻辑失败分开记录。具体命令和目标见 [G1 诊断](G1_diagnostics/README.md)，不安装新的系统组件来制造通过。

成本实验见 [G2 构建成本](G2_build_cost/README.md)。先运行正确性检查，正式采样期间停止本任务其他构建与测量，保持输入、编译器、配置、并行度一致。一轮预热加五个独立进程样本；cold/clean、no-op、修改实现与修改公共头的区别不能混作同一组。不得从单次总耗时推出“某缓存/链接器一定是瓶颈”。

## 6. 包消费与平台边界

[J1](J1_package/README.md)只在工作区内安装到专用 prefix_A，然后复制到 prefix_B。consumer 从新 prefix 的 package config 和导出目标获取使用要求，不应引用原源码目录、原 build 或机器上偶然存在的同名包。动态库运行还要核对实际加载位置；能找到导入库不意味着能找到 DLL。

Windows 核心及可用专项实际运行。ELF/GCC/Linux 的代码条件分支和命令保留，但未跑不写成通过；标准语义、工具细节和本机观察分别说明。最终验证范围、环境限制和非作者批准以[质量报告](../references/quality-report.md)为准。
