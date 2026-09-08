# ODR 样章独立复验

结论：APPROVE。上一轮 3 个 HIGH 已关闭；本轮未发现新的阻断。当前样章满足 `implementation-spec.md` 的样章门：单 TU 成功、第二 TU 重复符号、预处理和逐 object 外部符号证据、include guard/ODR/inline 边界、分离单定义和 inline 两种修复、原输入复验、学生完成体和坏变体拒绝都已验证。

## 本轮审查结果

Files Reviewed: 49

Total Issues: 0

By Severity:

- CRITICAL: 0
- HIGH: 0
- MEDIUM: 0
- LOW: 0

Recommendation: APPROVE

## 已关闭问题

1. `observe_symbols.cmake` 已改为分别选择 `observer_a` / `observer_b` object，并要求每个 object 的 `dumpbin` 输出匹配 `SECT... External ... lesson_value`。独立坏变体 `review_variants/symbol_missing` 被拒绝，错误指向 `observer_b.cpp.obj` 缺少 external `lesson_value` 定义。
2. `expect_build_failure.cmake` 已拆成先构建 `OBJECT_TARGET`、再构建 `LINK_TARGET`。编译期 `#error lesson_value_compile_should_not_pass` 被拒绝在 object 阶段；missing/unresolved symbol 在链接阶段失败但被拒绝为“not as a duplicate-symbol link failure”。
3. Student 已收窄为只改 `src/student/lesson.cpp`；caller 移到 `support/student_callers/`。`student_check.cpp` 直接检查 `lesson_value()`，再检查两条 caller 路径。`review_variants/student_bypass` 构建成功但运行失败，错误为 `student lesson_value must return 42`。

## 独立复验命令和结果

复验目录：`Engineering_Study/exercises/build/odr-independent-review/fix3c-rerun`

工具环境：CMake 4.2.3、MSVC 19.51.36256.0、VS 内置 Ninja 1.13.2。补跑的 `good-normal-configure` 与 `valid-student-normal-build` 未传 `CMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY`；configure 完成 compiler ABI 检测，随后真实构建、链接、运行 exe。作者旧证据中使用 STATIC_LIBRARY 只作为历史 configure 条件记录，不作为本轮通过前提。

```powershell
cmake -S Engineering_Study\exercises\C1_odr -B Engineering_Study\exercises\build\odr-independent-review\fix3c-rerun\good-normal-configure -G Ninja -DCMAKE_MAKE_PROGRAM="D:\VisualStudio2026\Installed\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe" -DCMAKE_CXX_COMPILER=cl.exe -DCMAKE_BUILD_TYPE=Release -DENGINEERING_STUDY_TEST_STUDENTS=ON
cmake --build Engineering_Study\exercises\build\odr-independent-review\fix3c-rerun\good-normal-configure --config Release
ctest --test-dir Engineering_Study\exercises\build\odr-independent-review\fix3c-rerun\good-normal-configure -LE student --output-on-failure
ctest --test-dir Engineering_Study\exercises\build\odr-independent-review\fix3c-rerun\good-normal-configure -L student --output-on-failure
```

结果：configure/build 通过；非 student 7/7 PASS；student starter 预期失败，输出 `student check failed: student lesson_value must return 42`。

```powershell
ctest --test-dir Engineering_Study\exercises\build\odr-independent-review\fix3c-rerun\good -L negative -V
```

结果：2/2 PASS。两个负例都先生成 object，再链接 `*_link.exe` 失败；输出包含 `lesson_value`、`LNK2005`、`LNK1169`，并报告 `negative case compiled objects, then failed at the expected duplicate-symbol link step`。

```powershell
cmake -S Engineering_Study\exercises\build\odr-independent-review\fix3c-rerun\valid-student-src\Engineering_Study\exercises\C1_odr -B Engineering_Study\exercises\build\odr-independent-review\fix3c-rerun\valid-student-normal-build -G Ninja -DCMAKE_MAKE_PROGRAM="D:\VisualStudio2026\Installed\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe" -DCMAKE_CXX_COMPILER=cl.exe -DCMAKE_BUILD_TYPE=Release -DENGINEERING_STUDY_TEST_STUDENTS=ON
cmake --build Engineering_Study\exercises\build\odr-independent-review\fix3c-rerun\valid-student-normal-build --target C1_odr_student --config Release
ctest --test-dir Engineering_Study\exercises\build\odr-independent-review\fix3c-rerun\valid-student-normal-build -L student --output-on-failure
```

结果：只把完成体副本的 `src/student/lesson.cpp` 从 `return 0;` 改为 `return 42;`，未改 caller/checker/reference；`C1_odr_student` 1/1 PASS。

坏变体复验：

- `review_variants/symbol_missing`：configure 后直接运行 `observe_symbols.cmake`，exit 1，拒绝原因是 `observer_b.cpp.obj` 没有 external `lesson_value` definition。
- `review_variants/negative_compile`：运行 `expect_build_failure.cmake`，exit 1，拒绝原因是 object compilation failed before expected link step，诊断为 `#error lesson_value_compile_should_not_pass`。
- `review_variants/negative_missing_symbol`：运行 `expect_build_failure.cmake`，exit 1，拒绝原因是链接失败不是 duplicate-symbol link failure；输出为 `LNK2019` / `LNK2001` / `LNK1120`。
- `review_variants/student_bypass`：configure/build 成功，CTest exit 1，拒绝原因是 `student lesson_value must return 42`。

## 教学与技术边界

正文仍保持正确边界：没有宣称所有 ODR 违规都必须诊断；没有把 `inline` 解释为强制机器码内联；没有把 `STATIC_LIBRARY` 或 PTY 当作样章工具链根因。inline variable 和模板实例化只讲到当前 ODR 样章所需边界，未替代后续 C04 深讲，符合当前样章任务。

练习闭环成立：学生说明只改 `src/student/lesson.cpp`；Reference 与 Student 分离；检查器直接消费学生实现和两条调用路径；负例不进入普通 build；Release 下 `check` 仍生效；负例拒绝 compile/configure/timeout/unresolved symbol 等无关失败。

## 未验证范围

- 未验证 GCC/Clang ELF 路径；本轮主验收环境是 Windows/MSVC。
- 未做 C01 其他章节、Modules/import std、ABI、包消费的最终整体验收。
- 未运行专用 `lsp_diagnostics`；当前执行面未提供该工具。C++ 诊断由 MSVC configure/build/CTest 覆盖，Markdown 由人工审查和 `rg` 静态检索覆盖。

## 文件指纹

`Engineering_Study/references/validation/odr/review-fix3c-source-fingerprints.sha256` 自身 SHA256：

```text
D8FB588811E815CCAA0CEBCD02CEC7D64709D63EA4699CBAC1A21D78B652CB0D
```

逐条比对结果：`review-fix3c-source-fingerprints.sha256` 中列出的全部源文件 hash 与当前工作区文件一致。关键文件：

```text
CCB514572FEDDEADCEB58941F23B961497EF3B7D078D69CB75CAAAF07542832C  Engineering_Study/chapters/02-odr-and-symbols.md
2435294ACB53AA48E635FC290F1B4F3F17363374B6708F81435BBA479F1FC83F  Engineering_Study/exercises/C1_odr/CMakeLists.txt
F161F9338DD96433218CC89C1A27D028A49A59FB32BAB0EC991B9F8E5662D0F4  Engineering_Study/exercises/C1_odr/README.md
D1D9C7F88F3833F7EE21ADEB681E0AA0AFC5B187F3E83B980105E60CC2941697  Engineering_Study/exercises/C1_odr/checks/student_check.cpp
D8EB99B5B85BD33FDC6E6562C2EC15FE511FB5F6BEFEDC821FC6E2C805476E2A  Engineering_Study/exercises/C1_odr/cmake/expect_build_failure.cmake
71F94890EEA0DD3F09DC8CECDEB95E1DD98B5E94710965B0B634C502287F8C23  Engineering_Study/exercises/C1_odr/cmake/observe_symbols.cmake
```
