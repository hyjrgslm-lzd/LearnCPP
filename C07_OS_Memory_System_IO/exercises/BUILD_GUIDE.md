# C07 构建与验证

核心 C++23、CMake 3.28+、Python 3.10+。Visual Studio 2026 生成器需要本机 CMake 4.2+；Linux 使用独立的 GCC/标准库，不用 Windows 的编译成功替代它。所有命令在本课 `exercises` 目录执行，除非特别注明。

## Windows

```powershell
cmake --preset verify-core
cmake --build --preset verify-core
ctest --preset verify-core

cmake --preset verify-debug
cmake --build --preset verify-debug
ctest --preset verify-debug
```

学习实验经过 `tools/run_test.py`，复用 C01 的进程外超时监督器。CTest 的超时覆盖外层运行器，运行器另设子进程超时和有限清理。stdout/stderr、原始退出码、timeout、cleanup_error 与解释后的 verdict 写入各 build 的 `records/<Config>/`。bad 需要精确 exit 1 和对应诊断；崩溃、启动失败或超时不能冒充预期反例。

监督器自检 `C07_runner_controls` 由 CTest 直接启动并设置 60 秒外部超时，避免“被测 runner 再包装自身校准”的递归依赖；自检内部对每个受控 child 另有超时。该项证据在 CTest/JUnit，学习实验另有 records JSON。每项实验有专用临时目录，失败进程退出后也由监督器回收文件，cleanup 失败仍是 FAIL。

单题不必配置整个课程，例如：

```powershell
cmake -S L01_handles -B build/leaf-L01 -G "Visual Studio 18 2026" -A x64
cmake --build build/leaf-L01 --config Release
ctest --test-dir build/leaf-L01 -C Release --output-on-failure
```

Windows IOCP 使用系统 SDK，不依赖 liburing。测试只操作自己创建的文件、pipe、模块和进程。若受限运行环境报告 `taskkill ... Access denied`，这是未完成清理的真实环境失败；不能修改 checker 接受它。应使用本任务已经授权、可管理自启实验进程的本机执行权限重新验证。

## Student 与检查器

默认 Reference 与检查器控制纳入构建，未完成 Student target 存在但不自动运行。学生编辑位置在每个题目 README；可以单独构建 `<单元>_student`。

```powershell
cmake --preset student
cmake --build --preset student
ctest --preset student
```

初始 Student 运行应受控失败，这不是课程参考实现失败，也不算学生作业完成。最终隔离审计需要以下额外步骤：在配置前于 build 根建立 `.cmake/api/v1/query/codemodel-v2` 空文件，配置时打开 `C07_STUDY_TRACE_INCLUDES=ON`；用 C02 的 `record_process.py` 记录一次 `cmake --build ... --clean-first --config Release --target <全部Student目标>`。随后运行 `tools/verify_students.py`，传入这份 include trace、build、config 和全新的 output 路径。

该工具调用原有 `audit_student.py`，核对实际 include、link/source/codemodel，并运行列出的全部 Student。MSVC `/showIncludes` 的中文/英文和 GCC/Clang `-H` 都受支持。它检查声明与接线，不能识别学生手抄答案；教学独立审查仍需遮住 Reference 尝试完成。

## Linux / WSL2

复用已建发行版 `LearnCPP-C08-Ubuntu-24.04`。环境创建与 C08 构建不属于 C07；C07 只使用自己的 `/root/learncpp-c07` 子目录。Windows 工作树继续位于 `F:\CPPTrain\LearnCPP`。

```powershell
wsl -d LearnCPP-C08-Ubuntu-24.04 --user root --cd /root -- bash
```

在 guest 内创建全新源码快照，再配置和运行。快照同时带上本课需要的 C01/C02 公共头和工具，保存源与副本的 SHA，拒绝覆盖已有快照；不复制 C08 的代码/构建产物。

```bash
python3 /mnt/f/CPPTrain/LearnCPP/C07_OS_Memory_System_IO/exercises/tools/snapshot_linux.py \
  --run-id linux-r1
cd /root/learncpp-c07/snapshots/linux-r1/C07_OS_Memory_System_IO/exercises
cmake --preset linux-core
cmake --build --preset linux-core
ctest --preset linux-core
```

`linux-debug`、`linux-student` 和 `linux-asan` 使用独立目录。使用 `findmnt -T .` 核对实际构建目录，必须是 guest ext4；不得把继承的 `/mnt/f` 目录误记成 Linux 原生文件系统。Linux 进程监督器在发行版内部启动 session/process group；Windows 侧仅作命令入口，杀 `wsl.exe` 不证明 guest 子进程已被回收。

## 固定 io_uring 组合

C09 既有要求为 liburing>=2.15；本课为了复现独立固定 2.15，而不是声称 C09 已经固定它。准备脚本从上游固定 tag 检查完整 commit，在 C07 的隔离前缀构建/安装用户态库，不安装系统包。

```bash
bash /mnt/f/CPPTrain/LearnCPP/C07_OS_Memory_System_IO/exercises/tools/prepare_uring.sh
# 回到本批源码快照的 exercises
cmake --preset linux-uring \
  -DC07_LIBURING_ROOT=/root/learncpp-c07/deps/install-liburing-2.15
cmake --build --preset linux-uring
ctest --preset linux-uring
```

Debug 对应 `linux-uring-debug`。该 guest 初始没有原生 pkg-config，指定 ROOT 时由 CMake 原生定位固定前缀的头与库并核对源码标识；不静默使用 Windows PATH 中同名程序。未指定 ROOT 时才使用 pkg-config 的精确 2.15 查询。实际头、库、源码/二进制指纹与 kernel 信息进入证据。

库存在、ring 可以创建、所需 opcode 可用及主体完成是不同检查。真正能力缺失可记录 `SKIP:` 与 exit 77；已具能力后的错误仍是 FAIL。选项 OFF 是未启用。缺库而准备失败是依赖阻断；整个 WSL 不可用是环境交接阻断。

## 检测器与实验

Windows `asan` 用 MSVC AddressSanitizer 和对应运行库；Linux `linux-asan` 用 GCC 的 ASan/UBSan。它们验证支持的内存访问路径，不能替代 fd/HANDLE 关闭、子进程回收、请求收束或跨进程同步的检查。危险 UB 反例不默认执行。

性能入口与正确性入口分开，见 [B01](B01_costs/README.md)。先定位再改变，原始失败和旧样本不覆盖。默认一轮预热、五次独立进程采样；Windows 和 WSL 分开报告。并行写作/构建时不运行正式性能采样。

最终验证必须用固定代码重新配置、构建并运行，而不是汇总过期作者日志。把本批运行器 JSON、CTest/JUnit、环境、include 审计和源文件 manifest 导回 `../references/validation/`；二进制、依赖和 CMake 缓存留在 build/guest。详细结果以 [质量报告](../references/quality-report.md) 为准。
