# C02 构建、实验与验证

先读[课程入口](../README.md)，再按对应题的 README 选择观察或实现任务。构建成功证明程序已经生成；Reference 检查通过证明参考实现满足已运行的检查；Student 检查通过才涉及学生实现。观察实验还需要完成预测、解释与扩展，不能把几个绿色测试当作学习完成。

## 环境与离线边界

核心要求 C++23、CMake 3.28 或更新版本。各题既可由本课顶层一起配置，也能在完整 LearnCPP checkout 内作为叶级工程独立配置。公共检查头只读复用 C01，因此不能把单个题目录复制到没有其余仓库文件的位置后声称仍是相同构建入口。

课程示例按 CMake 4.x、Visual Studio/MSVC 和 Clang 等现代工具链组织。这些是环境要求提示，不是所有 C++23/26/29 能力都可用的保证。默认不下载依赖、不安装组件、不改系统配置。实际标准模式、特性宏、编译/链接和运行分别记录；MSVC 预览模式与 ISO 固定文本之间也需要区分。

下列普通命令在 `C02_Objects_Lifetime_Ownership/exercises` 目录运行。Visual Studio 生成器可自动发现安装，普通 PowerShell 没有 `cl` 不等于没有 MSVC。机器安装的生成器名不同时使用对应名称，不在共享 preset 中写个人工具路径。

## Windows 核心 Debug 与 Release

```powershell
cmake --preset verify-core -G "Visual Studio 18 2026" -A x64
cmake --build --preset verify-core
ctest --preset verify-core

cmake --preset verify-debug -G "Visual Studio 18 2026" -A x64
cmake --build --preset verify-debug
ctest --preset verify-debug
```

VS 是多配置生成器，build/test preset 的 `configuration` 决定实际 Debug 或 Release；仅设置 `CMAKE_BUILD_TYPE` 不能代替它。两组目录独立，以便把当前源与实际配置匹配，避免把历史二进制误当当前结果。

Windows 嵌套编译负例应采用较短的 build 路径。过深的目录可能使 MSBuild 先因项目路径失败；这属于工具/路径失败，不能算命中题目期望的 C++ 诊断。原始失败应保留，另选短路径复验。

初始 Reference 默认打开，Student 测试默认关闭。公开验证用例是否额外启用，按题内 README 的真实选项和目标执行；它们验证检查器，不能冒充学生已经完成题目。

## Student 与单题构建

```powershell
cmake --preset student -G "Visual Studio 18 2026" -A x64
cmake --build --preset student
ctest --preset student -L student
```

初始实现型 Student 的最后一步应明确失败，因为安全占位还没有实现契约。不要修改完成标记、测试期望、Reference 或检查器来制造通过。先按题内 Part 修改指定学生文件，再重建对应目标和运行其检查。

从 LearnCPP 根目录独立配置一题，例如：

```powershell
cmake -S C02_Objects_Lifetime_Ownership/exercises/L07_raii -B build/learner-c02-l07 -G "Visual Studio 18 2026" -A x64 -DCORE_STUDY_TEST_STUDENTS=ON
cmake --build build/learner-c02-l07 --config Debug
ctest --test-dir build/learner-c02-l07 -C Debug --output-on-failure
```

Student-only 配置另设 `-DCORE_STUDY_BUILD_REFERENCE=OFF`。完整检查不仅查看退出码，还查看学生目标的构建依赖、预处理包含关系和实际源码接线，确认没有调用答案。合法完成体和代表性坏变体在隔离构建目录中验证，原 Student 学习文件保持独立。

## ASan 与危险诊断

`asan` preset 使用已安装 Clang 的 Ninja 路线，插桩普通安全程序，test preset只选择reference、observation、capability标签。故意错误的验证变体和unsafe诊断不混进安全路径；它们通过单独明确的命令验证。Windows 下需要 MSVC 的头文件/库环境；构建会按实际编译器把匹配的 ASan runtime 复制到程序输出目录，不修改系统 PATH。MSVC 与 LLVM 的 DLL 可能同名但导出不同，不能混用；缺少匹配文件时配置明确失败。

在工具链环境就绪的进程中，运行：

```powershell
cmake --preset asan
cmake --build --preset asan
ctest --preset asan
```

默认 `CORE_STUDY_ENABLE_UNSAFE_DEMOS=OFF`。故意越界、释放后使用等程序只在明确启用的独立进程执行；正反诊断需要同时检查目标错误类别、源码位置和退出码。缺 DLL、启动失败、超时、无关崩溃不能当成检测到了预期错误。ASan 无报告也不证明程序没有所有种类的 UB。

独立工具链探测应绑定当前机器的编译器、运行库路径和 sanitizer 选项；其他机器按自己的已安装工具设置子进程环境。探测程序通过不等于完整 C02 已通过 ASan，也不能把缺工具写成代码正确。

## 前沿能力与其他平台

`frontier` 单独打开 `CORE_STUDY_ENABLE_FRONTIER`。实验逐项核对固定草案或 DR、编译器模式、头文件/宏、实际实例化、链接和运行。源码存在、宏存在、某一种语法被接受，都不是完整规范支持证明。

实验区分选项未开、编译器不支持、库不支持、运行依赖缺失、尚未验证以及真实通过。不支持时仍保留完整源码、命令、规范解释与预期；只有明确声明的能力测试才能按真实条件 SKIP，不能把核心错误转换成 SKIP。两条编译器路径结果不同时分别记录，不用 MSVC 的结果替代 Clang。

Linux/macOS 在具有相应 C++23 能力的既有编译器环境中可使用同一核心入口：

```sh
cmake -S C02_Objects_Lifetime_Ownership/exercises -B build/c02-release -DCMAKE_BUILD_TYPE=Release
cmake --build build/c02-release
ctest --test-dir build/c02-release --output-on-failure
```

上述为可复现命令规格，本次不把未在这些平台执行的结果写成 PASS。平台专项或前沿能力仍按具体题的条件和限制执行。

## 记录一次命令

课程自己的记录器只用 Python 3.10+ 标准库，复用 C01 的进程超时/清理实现。从 LearnCPP 根目录运行：

```powershell
python C02_Objects_Lifetime_Ownership/exercises/tools/record_process.py --output build/learner-command-001.json -- cmake --version
```

`--output` 必须是新文件；重跑使用新名字，不覆盖旧失败。预期失败还需 `--expect-exit` 和 `--contains` 指定精确退出与目标诊断。记录同时保留子进程原始状态和判定结果，不能把预期非零改写成子进程退出零。记录器脚本位于 `exercises/tools/record_process.py`，生成的运行记录属于本机产物，不提交到课程文档。

Windows 记录器只为自身及继承的验证子进程设置错误模式，把加载/进程故障留在退出状态与日志中，避免系统错误弹窗打断用户操作；不改变机器级错误处理设置。它不会把加载失败变成成功，也不替代匹配运行库的部署。

普通 CTest 默认限时30秒，构建/诊断子任务180秒。超时和不能确认的清理仍记 FAIL；只处理自己的实验子进程。Windows 上经 Python 启动含复杂引号的 `cmd /c` 时，优先把精确命令保存在可审查的 `.cmd` 中再调用，避免不同参数解析规则让实际执行偏离记录。

## 怎样解释观察和性能

对象构造/移动/分配计数说明当前类型、当前输入的操作路径。计数本身不能推出耗时加速；带日志的类型也不能直接代表平凡类型的优化结果。NRVO、具体布局、未指定求值顺序只检查允许结果，不硬编码某一次观察。

只有进入性能收益或方案取舍实验时，才按通用指引先建立可归因基线，再一轮预热、五次独立进程采样，保存全部输入、样本、版本和无收益结果。不得用不完整采样、失败样本或不可比契约计算加速比。
