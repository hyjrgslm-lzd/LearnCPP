# 07：诊断、Sanitizer、fuzz 与构建成本

工具能帮你观察程序，但工具无报告不是正确性证明。C01 的诊断目标是建立一条可复现路径：先写清契约，再用最小输入验证契约，再用工具扩大观察范围。每条工具证据都要保存源码、命令、flags、输入、退出码、stdout/stderr、timeout 和环境边界。

## CTest 与检查程序

CTest 只负责运行检查并收集退出码、stdout/stderr、timeout 和标签。真正判断发生在检查程序里。课程检查用 `check(condition, message)`，Release 下仍生效；失败时输出诊断并 `std::exit(1)`。不要依赖 `assert`，因为 `NDEBUG` 可能关闭它。

一个好的负例必须先证明前置阶段成功。比如“链接失败才算通过”的测试，应先单独构建 object，确认编译已经过，再构建 link target 并匹配目标诊断。configure 失败、找不到编译器、timeout 都不是目标负例。否则测试只是在证明环境坏了。

`G1_diagnostics` 的 Reference parser 是安全路径：它只接受正好两个十进制数字。`"42"` 返回 `42`，`"00"` 返回 `0`；`"4x"`、`"7"`、空输入等无效输入抛出 `std::invalid_argument`。默认 CTest 只跑这条安全契约，不把故意 UB 放进普通课程通过条件。

## 静态分析

静态分析在编译前或编译中检查模式：未初始化、空指针路径、死代码、危险转换、潜在越界。它不运行程序，所以成本低、覆盖路径多；它也看不到所有运行时状态，尤其是输入相关的不变量。

课程的 static analyzer 验证分两类：

```text
clang++ --analyze -std=c++23 G1_diagnostics/reference/parser.cpp ...
clang++ --analyze -std=c++23 G1_diagnostics/static_analysis/null_state.cpp ...
```

第一条证明安全 parser 在当前 analyzer 下无诊断并能产出 analyzer 结果文件。第二条是观察样例：`definite_null_deref()` 明确解引用空指针，用来证明当前 analyzer 会产生 `core.NullDereference` 报告。不同 analyzer 版本可能给出不同报告，所以课程要求记录原始 stderr 和 plist，而不是只写“静态分析通过/失败”。

## ASan：安全样例通过，unsafe 专项失败

AddressSanitizer 在程序运行时插桩内存访问，能抓常见越界和 use-after-free。ASan 可以用于安全代码；安全代码在 ASan 下通过是有价值的回归信号。需要隔离的是故意触发 UB 的 fault 程序，因为它预期非零退出并输出 sanitizer 报告，不能混进默认安全 CTest。

本机 Clang 22 验证使用两条命令：

```text
clang++ -std=c++23 -fsanitize=address -g reference/parser.cpp checks/reference_check.cpp ... -o g1_asan_safe.exe
g1_asan_safe.exe

clang++ -std=c++23 -fsanitize=address -g unsafe/asan_fault.cpp -o g1_asan_fault.exe
g1_asan_fault.exe
```

第一条编译并运行安全 parser，退出码为 0。第二条源码只有显式 unsafe：

```cpp
int* values = new int[2]{1, 2};
int value = values[3];
```

验证脚本要求它非零退出，并在 stderr 中匹配 `ERROR: AddressSanitizer`、`heap-buffer-overflow` 和源码位置。这说明当前输入、当前二进制、当前 Clang ASan runtime 抓到了越界。它不说明所有 UB 都能被 ASan 抓到，也不说明没有报告的程序必然正确。

Windows 上还要验证运行时 DLL 可见。本机路径是：

```text
D:\VisualStudio2026\Installed\VC\Tools\Llvm\x64\lib\clang\22\lib\windows
```

验证脚本只在子进程 `PATH` 前置该目录，不改全局环境。不加运行时路径时，程序可能在启动阶段失败；那是 runtime 配置问题，不是 ASan 已报告代码问题。

## libFuzzer：有限输入，不伪装穷尽证明

libFuzzer 反复调用固定入口：

```cpp
extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size);
```

`G1_diagnostics/fuzz_parse.cpp` 把输入当作字节串传给真实 Reference parser。harness 不只看“有没有崩溃”：它先用独立 oracle 判断输入是否正好两个数字；有效输入必须返回同一数值，无效输入必须抛 `std::invalid_argument`。固定种子包括 `42`、`00`、`4x`、`7`，验证命令使用有限运行：

```text
g1_real_parser_fuzz.exe -runs=16 -seed=20260908 -max_total_time=5 input-corpus
```

脚本还会链接一个代表性坏 parser：`parse_two_digits` 总是返回 `42`。同一个 harness 必须先输出 `G1_ORACLE_MISMATCH:`，再非零退出，证明失败来自 oracle 拒绝“忽略输入/忽略长度”的错误实现。验证脚本同时要求无 timeout、无 cleanup error；缺 DLL、找不到 exe、未带 marker 的任意访问违规或超时都不能冒充 PASS。原始输出中的 `Done 16 runs` 只证明这些固定种子和本轮生成输入没有触发 Reference parser 契约违规。它不能证明输入空间完整安全。若要把 fuzz 用作更强结论，必须保存 seed、runs、语料、二进制、flags、失败输入和 sanitizer 配置。当前 Windows 验证中 `-fsanitize=fuzzer,address` 遇到 MSVC STL annotation link mismatch，因此 fuzzer 正式证据使用 `-fsanitize=fuzzer`；ASan 由独立正反样例覆盖。

## `-ftime-trace` 与阶段定位

Clang `-ftime-trace` 生成 JSON trace，能看到前端解析、模板实例化、优化、后端代码生成等阶段耗时。它适合回答“时间花在哪里”，不直接回答“该怎么优化”。

验证命令：

```text
clang++ -std=c++23 -ftime-trace reference/common.cpp reference/main.cpp ... -o g2_trace.exe
```

成功后会生成类似：

```text
g2_trace.exe-common.json
g2_trace.exe-main.json
```

这些 JSON 是阶段定位证据。要比较改动，需要同口径采样：同一编译器、同一 build dir 策略、同一配置、同一输入、同一机器负载条件。并发构建中的总耗时不能直接写成某个头文件或某个优化选项的根因。

## 构建成本实验

构建成本至少分四个场景：

- clean build：清空 build dir 后完整 configure、编译、链接。
- no-op build：源码未变，检查构建系统判定是否无需动作。
- 改实现 `.cpp`：只应重编受影响源和链接相关产物。
- 改公共头：包含它的消费者都可能重编。

优化要先做基线定位，再做候选对照。PCH 和 LTO 是候选方案，不是默认更快。PCH 可能降低重复解析头的成本，也可能增加首轮构建成本和缓存复杂度。LTO 可能改善运行期优化，也可能显著增加链接时间。

`G2_build_cost/scripts/measure_build.py` 提供可重建驱动。它用 Python stdlib 和单调时钟，每个 variant/scenario/repetition 都创建独立 source/build 副本；`clean` 把 configure+build 纳入计时，`noop`、`implementation`、`public-header` 把准备成本放在计时外，只计目标 build。driver 保存每条命令的 stdout/stderr、exit、timeout、cleanup、源码和 exe 指纹、随机调度、重编 TU/link/no-op 观察、原始样本和 median/min/max/range。正式性能采样由 root 在所有作者静止后执行一轮预热、五次独立进程；本章预检只证明结构和 driver 可用，不提前宣称 PCH/LTO 更快。


一个可用 driver 必须拒绝坏数据。已有 output 目录不能覆盖；外部命令失败不能被记成 PASS；timeout 要终止自己启动的子进程树并记录 cleanup 结果；语义运行检查在计时外执行，避免把“程序坏了但构建很快”写进性能统计。

## 练习连接

`G1_diagnostics` 验证安全 parser、static analyzer、ASan 正反样例和 libFuzzer 有限运行。`G2_build_cost` 提供 baseline/PCH/LTO 三个 target 和预备测量驱动。学生要先读基线命令和依赖定位，再解释候选优化为什么可能改变某一阶段成本。

## 自测

- sanitizer 程序“编译通过”还缺哪一步才算工具可用？
- ASan 安全样例通过和 unsafe 样例失败分别证明什么？
- fuzz 运行 16 次通过能证明什么，不能证明什么？
- 为什么一次 no-op build 很快不能证明 PCH 有收益？

答案：还要运行并确认 runtime DLL、退出码和 sanitizer 报告路径；安全样例证明当前契约在插桩下通过，unsafe 失败证明故意越界被当前工具抓到；16 次通过只证明这 16 次没有触发可观察故障；PCH 影响特定阶段和输入，需要前后同口径样本。




