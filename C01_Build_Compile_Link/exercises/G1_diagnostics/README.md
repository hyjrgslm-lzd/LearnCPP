# 练习 G1：让诊断工具检查真实契约

先读[07 诊断与成本](../../chapters/07-diagnostics-and-build-cost.md)。本题分为安全实现练习、静态分析、ASan和fuzz观察。默认安全路径不执行故意UB。

`parse_two_digits`只接受恰好两个ASCII十进制数字，返回0到99；其他输入抛`std::invalid_argument`。本题不是通用Unicode/任意长度整数解析器。

## Part 1：完成独立的安全 Student

只编辑[src/student/parser.cpp](src/student/parser.cpp)。初始安全占位返回-1，位于合法结果域之外，首个checker会正常失败；它不索引空输入，也不把未实现伪装成有效解析。Reference在[reference/parser.cpp](reference/parser.cpp)，学生修改不会改变它。Reference和Student使用同一份契约检查：枚举全部100个两位ASCII数字，并覆盖空串、短输入、长输入、非数字、空白、符号和非ASCII数字样文本，拒绝只返回42的实现。

从LearnCPP根目录的x64 Native Tools环境运行：

```powershell
cmake -S C01_Build_Compile_Link/exercises/G1_diagnostics -B C01_Build_Compile_Link/exercises/build/learner-g1 -G Ninja -DCMAKE_BUILD_TYPE=Release -DENGINEERING_STUDY_TEST_STUDENTS=ON
cmake --build C01_Build_Compile_Link/exercises/build/learner-g1
ctest --test-dir C01_Build_Compile_Link/exercises/build/learner-g1 -L reference --output-on-failure
ctest --test-dir C01_Build_Compile_Link/exercises/build/learner-g1 -L student --output-on-failure
```

**解析：** 先检查长度，再检查两字符都在`'0'..'9'`，最后计算十位乘10加个位。直接读取`text[1]`再补长度判断已经太晚；string_view的逻辑边界和底层分配边界也不是同一件事，不能承诺ASan必然发现每次逻辑越界。检查器自身失败会打印诊断并退出1；被测parser的`invalid_argument`则由专门的catch验证，不混为一类。

## Part 2：静态分析与 ASan 分别证明什么

阅读`static_analysis/`和`unsafe/`，先预测告警或错误类别，再用本机可用的静态分析器或 sanitizer 运行对应目标。静态分析不运行程序；安全 parser 无告警和故意空指针样例产生目标诊断是两种证据。退出码0本身不能证明目标诊断命中。

ASan可以检查正常安全代码；故意越界程序才需要显式隔离。构建插桩程序后还必须运行，并检查匹配的LLVM runtime是否成功装载。没有ASan报告的DLL装载失败不是“发现内存错误”。故意越界的通过判据包括报告类别`heap-buffer-overflow`、源码位置和非零退出，不接受任意崩溃或timeout。具体当前命令及其环境见工具证据；不直接执行未经检查的unsafe代码来获得漂亮结果。

## Part 3：真实 parser 的有限 fuzz 与坏实现对照

需要Python3.11+、已安装Clang/libFuzzer及匹配的运行时。先在本机选择有效解释器和编译器路径；下方变量是需要设置的输入，不是要求复制作者机器路径。

```powershell
$StudyPython = '你的Python3.11以上解释器完整路径'
$StudyClang = '你的clang++.exe完整路径'
& $StudyPython --version
& $StudyClang --version
& $StudyPython C01_Build_Compile_Link/exercises/G1_diagnostics/scripts/verify_fuzz.py --clang $StudyClang --output C01_Build_Compile_Link/exercises/build/learner-g1-fuzz-001
```

output必须不存在，重复执行时选新目录。脚本复制固定seed到自己的输出目录，不向仓库`fuzz_corpus/`回写新语料。`fuzz_parse.cpp`调用真实parser；独立oracle对每个输入判断长度、字符及预期数值。种子包含42、00、4x、7。

**解析：** 正确parser应在有限运行内满足oracle；坏parser总返回42，必须先输出`G1_ORACLE_MISMATCH:`再非零退出。checker还检查没有timeout、runner错误或清理错误。缺DLL、不存在exe、未带marker的访问违规不能冒充“坏parser被oracle拒绝”。保留所有失败输入和实际命令，不能只写“fuzzer跑过”。

本机联合`-fsanitize=fuzzer,address`曾遇MSVC STL annotation链接不一致，当前正式fuzz证据使用`-fsanitize=fuzzer`，ASan由独立正反例覆盖；这不等于已验证联合模式。当前记录从summary中的575/577等对应快照进入，不把历史cmd的固定output当作可重复运行的通用入口。

## 必答问题与解析

- `"00"`为什么不可只靠`"42"`测试代替？恒定42实现能过后者，不能满足前者；本题checker枚举00到99，并验证多类非法输入都走`invalid_argument`拒绝路径。
- 16次fuzz通过能证明整个输入空间安全吗？不能，只说明此次seed和生成输入没有触发可观察契约违规。
- 工具无报告为什么不是完整证明？执行路径、插桩能力、库契约、输入和资源条件都有边界；应结合源码及明确契约分析。
