# C10 构建、作业与验证

## 构建会证明什么

配置成功只表示生成了项目；构建成功只表示指定代码能编译链接。实现型题目由同一个main.cpp实际调用选中的solution.hpp，运行后的行为检查才决定本题的结果。运行型Student初态返回2并报告UNFINISHED；纯类型算法可以按题面和manifest的精确诊断返回1，仍是未完成失败；Reference/good返回0；bad必须在指定检查处返回1。不要把任何非零退出都当作成功拒绝。

核心依赖是固定stdexec，本课默认配置不访问网络，不安装工具，不进入会隐式下载其他组件的上游CMake。先提供完整且干净的本地checkout：tag nvhpc-26.05，SHA 6d7ad689f4d4831c5136e4abe1c601f9a3b64e43。可复用机器上已有的该版本；不要把另一门课的私人build目录硬编码进提交的preset。

## Windows：从一题开始

需要CMake3.28+、Python3.10+、Git和支持所选模式的C++编译器；本机使用VS18 2026/MSVC19.51。在仓库根执行，下面的路径应换成自己的固定依赖checkout：

```powershell
$env:STDEXEC_ROOT = 'F:/deps/stdexec'
cmake -S C10_Execution/exercises -B build/c10 -G 'Visual Studio 18 2026' -A x64 -DFETCHCONTENT_SOURCE_DIR_STDEXEC=$env:STDEXEC_ROOT
cmake --build build/c10 --config Debug
ctest --test-dir build/c10 -C Debug --output-on-failure
```

构建学生题名会构建对应的_student目标，但Student默认不参加成功矩阵，避免把未完成作业当作课程实现：

```powershell
cmake --build build/c10 --config Debug --target G1_my_then
```

可执行文件的精确位置见build根unit-manifest.json中的target名称及CMake File API实际artifact，VS通常放在对应单元的Debug/Release目录。完成前返回2是正常的作业初态，不是Reference通过。

单题也可以作为独立工程：

```powershell
cmake -S C10_Execution/exercises/G1_my_then -B build/c10-g1 -G 'Visual Studio 18 2026' -A x64 -DFETCHCONTENT_SOURCE_DIR_STDEXEC=$env:STDEXEC_ROOT -DC10_STUDY_BUILD_REFERENCE=OFF
cmake --build build/c10-g1 --config Debug --target G1_my_then_student
```

在exercises目录可用preset：先设置STDEXEC_ROOT，再依次运行`cmake --preset windows`、`cmake --build --preset windows-debug`、`ctest --preset windows-debug`。Release改为windows-release。不存在旧指南中的msvc preset。

## Linux/WSL：原生运行

复用已有工具链，在guest独立目录配置、构建和运行。正式测量使用ext4内的源码/构建快照，不混用其他课程build。GCC13主路径C++23；Clang18可用C++26模式，但其宿主库不一定具有全部C++23设施，按单元probe判定。

```sh
export STDEXEC_ROOT=/root/learncpp-c10/deps/stdexec
cmake -S C10_Execution/exercises -B build/c10-linux -G Ninja -DCMAKE_CXX_COMPILER=g++ -DCMAKE_BUILD_TYPE=Debug -DFETCHCONTENT_SOURCE_DIR_STDEXEC="$STDEXEC_ROOT"
cmake --build build/c10-linux
ctest --test-dir build/c10-linux --output-on-failure
```

完整 Linux 矩阵还包括 I1/P1 的真实 io_uring。复用已有 liburing 2.15 前缀，另开构建目录并在上述 configure 追加：

```sh
-DC10_STUDY_ENABLE_IO_URING=ON -DC10_LIBURING_ROOT=/root/learncpp-c07/deps/install-liburing-2.15
```

前缀必须带有 `c07-source-commit.txt`，内容为固定源码 SHA `d41bf9220ec39277ff235379e9089d9e0fd6c2a5`。省略开关时 I/O 单元不会进入默认目标，配置明确显示 NOT_ENABLED；这不代表 I/O 验证通过。单题 I1/P1 同样需要这两个参数。

在exercises下也可使用linux、linux-debug、clang26、asan-linux、tsan-linux预设。ASan/TSan使用不同build，不同时开启；Windows ASan使用asan-windows-debug。检测器只证明实际执行分支中没有相应报告，不能替代生命周期协议推导。

## 编辑哪一个文件

- 实现型题目：只编辑本题src/student/solution.hpp，沿README逐Part完成。main.cpp和checks属于验证依据，Reference是独立答案。
- Reference：src/reference/solution.hpp；答案解释见题面和正文。Student改动不得改变Reference。
- 独立good/bad：validation/good与validation/bad用于验证检查器。good可以采用另一种正确算法；bad必须可编译并真实违反行为，不能靠退出/崩溃/改checker作弊。
- 观察型单元：起点可以完整可运行，代码检查与读者解释任务分别验收，不以程序通过代替所有练习完成。

C10_STUDY_STUDENT_ROOT仅供验证工具在临时目录注入独立完成体，布局为root/题号/solution.hpp。正式作业不需要它。Reference OFF仍必须能够配置和构建学生目标。

## 能力、超时与日志

C10_CXX_STANDARD明确选择请求模式，CXX_STANDARD_REQUIRED禁止静默降级。MSVC的26映射/std:c++latest；最终以真实argv和设施probe记录能力，不以CMake变量证明标准库已经实现。标准原生路径和stdexec参考实现分别检查。

CTest通过仓库已有的C07运行包装及C01进程监督器运行被测程序，records/<配置>保存完整输出、退出码和清理结果。内部截止时间之外另有CTest进程外超时。需要在WSL内原生运行监督器，不能把终止wsl.exe当作guest进程清理证明。

选项关闭是NOT_ENABLED；独立probe确认缺能力才是SKIP；依赖准备、主体编译、运行、超时或清理失败是FAIL/BLOCKED。真实I/O必须发生真实提交/完成，取消提交不是目标完成。当前nvc++缺失时nvexec完整内容保留但不能记GPU运行通过。

## 证据与复现

工具入口为tools/run_matrix.py、tools/verify_students.py、tools/audit_delivery.py，各用`--help`查看明确参数。matrix保存新run-id、环境、配置/构建/CTest命令；Student检查保存Reference OFF、真实产物、good/bad及include路径证据。导航审计只证明链接/登记，不证明教学质量。

Linux 完整自动矩阵使用 `run_matrix.py --compiler /usr/bin/g++ --io-uring --liburing-root <前缀> --stdexec-source <checkout>`；Student 注入工具接受同名编译器/I/O 参数，并以 `--build-dir` 指向刚生成的 manifest。所有证据输出用新目录，保留失败，不覆盖旧记录。`--unit` 可缩小范围；不指定则覆盖本次 manifest 的全部单元。

实际通过范围必须由本机重新配置、构建和运行产生的新记录说明；旧 build、旧截图、目录存在和作者自述不能替代复验。验证记录留在 build/validation 或单独归档，不提交到课程源码。

矩阵工具在Windows默认单任务、Linux默认两个并行构建任务运行，可用 `--jobs` 调整；构建目录位于仓库 `build/c10-matrix`，证据目录单独保存。Student注入同样使用短构建路径，避免MSBuild长路径错误；验证工具对子进程关闭MSBuild节点复用，不修改系统环境配置。

ASan与UBSan现在独立选择：`C10_STUDY_ENABLE_ASAN` 只启用地址检测；`C10_STUDY_ENABLE_UBSAN` 或矩阵 `--sanitizer ubsan` 单独验证未定义行为检测器。若某一检测器组合因 stdexec 常量表达式或编译器前端问题失败，不能把该失败说成另一个检测器不可用，也不能通过修改正确的调度图掩盖检测器差异。原始诊断留在本机验证目录。
