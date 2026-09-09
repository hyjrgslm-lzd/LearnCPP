# 本批交付清单

状态：本文件列出本批持久源码与教学文档用途；实际验证与非作者最终结论统一见 quality-report.md 及其链接的审查报告。

边界：清单来自当前 `git diff --name-only` 与 `git ls-files --others --exclude-standard`；排除 `build/`、`out/`、`.omx/`、`__pycache__/`、`.pyc`、用户既有 `CONTENT_REFACTORING_GUIDE.md` 与 `Coroutine_Study/exercises/P2_generator_basics/`。validation 原始日志、json/xml/stdout/stderr 按目录合并；独立审查报告逐文件列出。

## Root 导航与计划

- `LEARNCPP_GLOBAL_PLAN.md` — 全仓课程交付计划，记录本批 Engineering/Coroutine/Concurrency 汇合路线。
- `README.md` — 仓库根导航，加入本批课程入口与学习路径。

## Engineering_Study 新增课程源码与教学文档

### 课程入口与章节

- `Engineering_Study/README.md` — Engineering Study 总入口，串联章节、练习、验收资料。
- `Engineering_Study/chapters/00-build-and-debug.md` — 编译、调试与工具链入口章节。
- `Engineering_Study/chapters/01-translation-and-preprocessing.md` — 翻译阶段、预处理和头文件边界章节。
- `Engineering_Study/chapters/02-odr-and-symbols.md` — ODR、符号和链接错误章节。
- `Engineering_Study/chapters/03-object-files-and-static-libraries.md` — 目标文件、归档库和静态链接章节。
- `Engineering_Study/chapters/04-dynamic-libraries-and-runtime.md` — 动态库、导入库、加载器和运行期初始化章节。
- `Engineering_Study/chapters/05-abi-boundaries.md` — ABI 边界、C ABI、所有权和异常隔离章节。
- `Engineering_Study/chapters/06-cmake-and-dependencies.md` — CMake target、依赖传递和包消费章节。
- `Engineering_Study/chapters/07-diagnostics-and-build-cost.md` — 诊断链、fuzz/static analysis 和构建成本章节。
- `Engineering_Study/chapters/08-modules.md` — C++20 modules、分区、可见性和打包限制章节。
- `Engineering_Study/chapters/09-import-std.md` — MSVC import std 工具链门禁与证据链章节。
- `Engineering_Study/chapters/10-packaging-and-compatibility.md` — 安装、重定位、版本兼容和二进制包章节。

### 练习公共设施

- `Engineering_Study/exercises/BUILD_GUIDE.md` — Engineering Study 练习统一构建说明。
- `Engineering_Study/exercises/CMakeLists.txt` — Engineering Study 练习根 CMake 聚合入口。
- `Engineering_Study/exercises/CMakePresets.json` — Engineering Study 练习 CMake preset 配置。
- `Engineering_Study/exercises/cmake/StudySetup.cmake` — 练习公共 target 配置与 CTest 包装 helper。
- `Engineering_Study/exercises/cmake/check_selftest.cpp` — 公共 CMake 自检源，用于验证 check/helper 行为。
- `Engineering_Study/exercises/cmake/verify_check_failure.cmake` — 公共负向检查脚本，验证失败用例确实失败。
- `Engineering_Study/exercises/include/check.hpp` — 练习公共 check(bool, string_view) 断言辅助头。
- `Engineering_Study/exercises/tools/process_runner.py` — 练习验证脚本公共进程运行辅助。

### A1_build_debug

- `Engineering_Study/exercises/A1_build_debug/CMakeLists.txt` — A1 编译/调试观察练习 的 leaf target、Reference/Student 和 CTest 编排。
- `Engineering_Study/exercises/A1_build_debug/README.md` — A1 编译/调试观察练习 的教学任务、运行方式、验收与解析。
- `Engineering_Study/exercises/A1_build_debug/checks/observation_check.cpp` — A1 编译/调试观察练习 的独立检查程序，覆盖 Student/Reference 或负向验收。
- `Engineering_Study/exercises/A1_build_debug/checks/reference_check.cpp` — A1 编译/调试观察练习 的独立检查程序，覆盖 Student/Reference 或负向验收。
- `Engineering_Study/exercises/A1_build_debug/checks/student_check.cpp` — A1 编译/调试观察练习 的独立检查程序，覆盖 Student/Reference 或负向验收。
- `Engineering_Study/exercises/A1_build_debug/debugger/gdb_commands.txt` — A1 编译/调试观察练习 的调试器观察材料。
- `Engineering_Study/exercises/A1_build_debug/debugger/gdb_lesson.cpp` — A1 编译/调试观察练习 的调试器观察材料。
- `Engineering_Study/exercises/A1_build_debug/src/reference/debug_story.cpp` — A1 编译/调试观察练习 的参考实现或观察程序。
- `Engineering_Study/exercises/A1_build_debug/src/reference/debug_story.hpp` — A1 编译/调试观察练习 的参考实现或观察程序。
- `Engineering_Study/exercises/A1_build_debug/src/student/debug_story.cpp` — A1 编译/调试观察练习 的学生实现入口。
- `Engineering_Study/exercises/A1_build_debug/src/student/debug_story.hpp` — A1 编译/调试观察练习 的学生实现入口。

### B1_preprocessor

- `Engineering_Study/exercises/B1_preprocessor/CMakeLists.txt` — B1 预处理实现练习 的 leaf target、Reference/Student 和 CTest 编排。
- `Engineering_Study/exercises/B1_preprocessor/README.md` — B1 预处理实现练习 的教学任务、运行方式、验收与解析。
- `Engineering_Study/exercises/B1_preprocessor/checks/reference_check.cpp` — B1 预处理实现练习 的独立检查程序，覆盖 Student/Reference 或负向验收。
- `Engineering_Study/exercises/B1_preprocessor/checks/student_check.cpp` — B1 预处理实现练习 的独立检查程序，覆盖 Student/Reference 或负向验收。
- `Engineering_Study/exercises/B1_preprocessor/cmake/check_preprocess.cmake` — B1 预处理实现练习 的 CMake 辅助脚本或 package config 模板。
- `Engineering_Study/exercises/B1_preprocessor/src/reference/generated_value.hpp` — B1 预处理实现练习 的参考实现或观察程序。
- `Engineering_Study/exercises/B1_preprocessor/src/reference/macro_a.cpp` — B1 预处理实现练习 的参考实现或观察程序。
- `Engineering_Study/exercises/B1_preprocessor/src/reference/macro_b.cpp` — B1 预处理实现练习 的参考实现或观察程序。
- `Engineering_Study/exercises/B1_preprocessor/src/reference/pragma_once_example.hpp` — B1 预处理实现练习 的参考实现或观察程序。
- `Engineering_Study/exercises/B1_preprocessor/src/student/student_value.cpp` — B1 预处理实现练习 的学生实现入口。
- `Engineering_Study/exercises/B1_preprocessor/src/student/student_value.hpp` — B1 预处理实现练习 的学生实现入口。

### C1_odr

- `Engineering_Study/exercises/C1_odr/CMakeLists.txt` — C1 ODR 与符号练习 的 leaf target、Reference/Student 和 CTest 编排。
- `Engineering_Study/exercises/C1_odr/README.md` — C1 ODR 与符号练习 的教学任务、运行方式、验收与解析。
- `Engineering_Study/exercises/C1_odr/checks/reference_check.cpp` — C1 ODR 与符号练习 的独立检查程序，覆盖 Student/Reference 或负向验收。
- `Engineering_Study/exercises/C1_odr/checks/student_check.cpp` — C1 ODR 与符号练习 的独立检查程序，覆盖 Student/Reference 或负向验收。
- `Engineering_Study/exercises/C1_odr/cmake/expect_build_failure.cmake` — C1 ODR 与符号练习 的 CMake 辅助脚本或 package config 模板。
- `Engineering_Study/exercises/C1_odr/cmake/observe_preprocess.cmake` — C1 ODR 与符号练习 的 CMake 辅助脚本或 package config 模板。
- `Engineering_Study/exercises/C1_odr/cmake/observe_symbols.cmake` — C1 ODR 与符号练习 的 CMake 辅助脚本或 package config 模板。
- `Engineering_Study/exercises/C1_odr/negative/duplicate_header_definition/CMakeLists.txt` — C1 ODR 与符号练习 的 leaf target、Reference/Student 和 CTest 编排。
- `Engineering_Study/exercises/C1_odr/negative/duplicate_header_definition/caller_a.cpp` — C1 ODR 与符号练习 的故意失败样例，用于验证错误阶段和诊断边界。
- `Engineering_Study/exercises/C1_odr/negative/duplicate_header_definition/caller_b.cpp` — C1 ODR 与符号练习 的故意失败样例，用于验证错误阶段和诊断边界。
- `Engineering_Study/exercises/C1_odr/negative/duplicate_header_definition/lesson.hpp` — C1 ODR 与符号练习 的故意失败样例，用于验证错误阶段和诊断边界。
- `Engineering_Study/exercises/C1_odr/negative/duplicate_header_definition/main.cpp` — C1 ODR 与符号练习 的故意失败样例，用于验证错误阶段和诊断边界。
- `Engineering_Study/exercises/C1_odr/negative/include_guard_same_failure/CMakeLists.txt` — C1 ODR 与符号练习 的 leaf target、Reference/Student 和 CTest 编排。
- `Engineering_Study/exercises/C1_odr/negative/include_guard_same_failure/caller_a.cpp` — C1 ODR 与符号练习 的故意失败样例，用于验证错误阶段和诊断边界。
- `Engineering_Study/exercises/C1_odr/negative/include_guard_same_failure/caller_b.cpp` — C1 ODR 与符号练习 的故意失败样例，用于验证错误阶段和诊断边界。
- `Engineering_Study/exercises/C1_odr/negative/include_guard_same_failure/lesson.hpp` — C1 ODR 与符号练习 的故意失败样例，用于验证错误阶段和诊断边界。
- `Engineering_Study/exercises/C1_odr/negative/include_guard_same_failure/main.cpp` — C1 ODR 与符号练习 的故意失败样例，用于验证错误阶段和诊断边界。
- `Engineering_Study/exercises/C1_odr/observation/single_consumer/lesson.hpp` — C1 ODR 与符号练习 的观察样例，用于展示预处理、符号或布局证据。
- `Engineering_Study/exercises/C1_odr/observation/single_consumer/main.cpp` — C1 ODR 与符号练习 的观察样例，用于展示预处理、符号或布局证据。
- `Engineering_Study/exercises/C1_odr/observation/two_tu_symbols/lesson.hpp` — C1 ODR 与符号练习 的观察样例，用于展示预处理、符号或布局证据。
- `Engineering_Study/exercises/C1_odr/observation/two_tu_symbols/observer_a.cpp` — C1 ODR 与符号练习 的观察样例，用于展示预处理、符号或布局证据。
- `Engineering_Study/exercises/C1_odr/observation/two_tu_symbols/observer_b.cpp` — C1 ODR 与符号练习 的观察样例，用于展示预处理、符号或布局证据。
- `Engineering_Study/exercises/C1_odr/reference/inline_definition/caller_a.cpp` — C1 ODR 与符号练习 的参考实现或观察程序。
- `Engineering_Study/exercises/C1_odr/reference/inline_definition/caller_b.cpp` — C1 ODR 与符号练习 的参考实现或观察程序。
- `Engineering_Study/exercises/C1_odr/reference/inline_definition/lesson.hpp` — C1 ODR 与符号练习 的参考实现或观察程序。
- `Engineering_Study/exercises/C1_odr/reference/inline_definition/main.cpp` — C1 ODR 与符号练习 的参考实现或观察程序。
- `Engineering_Study/exercises/C1_odr/reference/split_definition/caller_a.cpp` — C1 ODR 与符号练习 的参考实现或观察程序。
- `Engineering_Study/exercises/C1_odr/reference/split_definition/caller_b.cpp` — C1 ODR 与符号练习 的参考实现或观察程序。
- `Engineering_Study/exercises/C1_odr/reference/split_definition/lesson.cpp` — C1 ODR 与符号练习 的参考实现或观察程序。
- `Engineering_Study/exercises/C1_odr/reference/split_definition/lesson.hpp` — C1 ODR 与符号练习 的参考实现或观察程序。
- `Engineering_Study/exercises/C1_odr/review_variants/negative_compile/CMakeLists.txt` — C1 ODR 与符号练习 的 leaf target、Reference/Student 和 CTest 编排。
- `Engineering_Study/exercises/C1_odr/review_variants/negative_compile/caller_a.cpp` — C1 ODR 与符号练习 的审查变体，用于防止伪通过和错误验收。
- `Engineering_Study/exercises/C1_odr/review_variants/negative_compile/caller_b.cpp` — C1 ODR 与符号练习 的审查变体，用于防止伪通过和错误验收。
- `Engineering_Study/exercises/C1_odr/review_variants/negative_compile/lesson.hpp` — C1 ODR 与符号练习 的审查变体，用于防止伪通过和错误验收。
- `Engineering_Study/exercises/C1_odr/review_variants/negative_compile/main.cpp` — C1 ODR 与符号练习 的审查变体，用于防止伪通过和错误验收。
- `Engineering_Study/exercises/C1_odr/review_variants/negative_missing_symbol/CMakeLists.txt` — C1 ODR 与符号练习 的 leaf target、Reference/Student 和 CTest 编排。
- `Engineering_Study/exercises/C1_odr/review_variants/negative_missing_symbol/caller_a.cpp` — C1 ODR 与符号练习 的审查变体，用于防止伪通过和错误验收。
- `Engineering_Study/exercises/C1_odr/review_variants/negative_missing_symbol/caller_b.cpp` — C1 ODR 与符号练习 的审查变体，用于防止伪通过和错误验收。
- `Engineering_Study/exercises/C1_odr/review_variants/negative_missing_symbol/lesson.hpp` — C1 ODR 与符号练习 的审查变体，用于防止伪通过和错误验收。
- `Engineering_Study/exercises/C1_odr/review_variants/negative_missing_symbol/main.cpp` — C1 ODR 与符号练习 的审查变体，用于防止伪通过和错误验收。
- `Engineering_Study/exercises/C1_odr/review_variants/student_bypass/CMakeLists.txt` — C1 ODR 与符号练习 的 leaf target、Reference/Student 和 CTest 编排。
- `Engineering_Study/exercises/C1_odr/review_variants/student_bypass/caller_a.cpp` — C1 ODR 与符号练习 的审查变体，用于防止伪通过和错误验收。
- `Engineering_Study/exercises/C1_odr/review_variants/student_bypass/caller_b.cpp` — C1 ODR 与符号练习 的审查变体，用于防止伪通过和错误验收。
- `Engineering_Study/exercises/C1_odr/review_variants/student_bypass/lesson.cpp` — C1 ODR 与符号练习 的审查变体，用于防止伪通过和错误验收。
- `Engineering_Study/exercises/C1_odr/review_variants/student_bypass/lesson.hpp` — C1 ODR 与符号练习 的审查变体，用于防止伪通过和错误验收。
- `Engineering_Study/exercises/C1_odr/review_variants/symbol_missing/CMakeLists.txt` — C1 ODR 与符号练习 的 leaf target、Reference/Student 和 CTest 编排。
- `Engineering_Study/exercises/C1_odr/review_variants/symbol_missing/lesson.hpp` — C1 ODR 与符号练习 的审查变体，用于防止伪通过和错误验收。
- `Engineering_Study/exercises/C1_odr/review_variants/symbol_missing/observer_a.cpp` — C1 ODR 与符号练习 的审查变体，用于防止伪通过和错误验收。
- `Engineering_Study/exercises/C1_odr/review_variants/symbol_missing/observer_b.cpp` — C1 ODR 与符号练习 的审查变体，用于防止伪通过和错误验收。
- `Engineering_Study/exercises/C1_odr/src/student/lesson.cpp` — C1 ODR 与符号练习 的学生实现入口。
- `Engineering_Study/exercises/C1_odr/src/student/lesson.hpp` — C1 ODR 与符号练习 的学生实现入口。
- `Engineering_Study/exercises/C1_odr/support/student_callers/caller_a.cpp` — C1 ODR 与符号练习 的学生检查支持源文件。
- `Engineering_Study/exercises/C1_odr/support/student_callers/caller_b.cpp` — C1 ODR 与符号练习 的学生检查支持源文件。

### C2_archive

- `Engineering_Study/exercises/C2_archive/CMakeLists.txt` — C2 静态库归档练习 的 leaf target、Reference/Student 和 CTest 编排。
- `Engineering_Study/exercises/C2_archive/README.md` — C2 静态库归档练习 的教学任务、运行方式、验收与解析。
- `Engineering_Study/exercises/C2_archive/checks/reference_check.cpp` — C2 静态库归档练习 的独立检查程序，覆盖 Student/Reference 或负向验收。
- `Engineering_Study/exercises/C2_archive/checks/student_check.cpp` — C2 静态库归档练习 的独立检查程序，覆盖 Student/Reference 或负向验收。
- `Engineering_Study/exercises/C2_archive/cmake/check_symbols.cmake` — C2 静态库归档练习 的 CMake 辅助脚本或 package config 模板。
- `Engineering_Study/exercises/C2_archive/cmake/expect_link_failure.cmake` — C2 静态库归档练习 的 CMake 辅助脚本或 package config 模板。
- `Engineering_Study/exercises/C2_archive/negative/direct_objects/CMakeLists.txt` — C2 静态库归档练习 的 leaf target、Reference/Student 和 CTest 编排。
- `Engineering_Study/exercises/C2_archive/negative/direct_objects/main.cpp` — C2 静态库归档练习 的故意失败样例，用于验证错误阶段和诊断边界。
- `Engineering_Study/exercises/C2_archive/negative/direct_objects/unused_member.cpp` — C2 静态库归档练习 的故意失败样例，用于验证错误阶段和诊断边界。
- `Engineering_Study/exercises/C2_archive/negative/direct_objects/used_member.cpp` — C2 静态库归档练习 的故意失败样例，用于验证错误阶段和诊断边界。
- `Engineering_Study/exercises/C2_archive/negative/missing_definition/CMakeLists.txt` — C2 静态库归档练习 的 leaf target、Reference/Student 和 CTest 编排。
- `Engineering_Study/exercises/C2_archive/negative/missing_definition/caller.cpp` — C2 静态库归档练习 的故意失败样例，用于验证错误阶段和诊断边界。
- `Engineering_Study/exercises/C2_archive/negative/missing_definition/main.cpp` — C2 静态库归档练习 的故意失败样例，用于验证错误阶段和诊断边界。
- `Engineering_Study/exercises/C2_archive/src/reference/archive_value.hpp` — C2 静态库归档练习 的参考实现或观察程序。
- `Engineering_Study/exercises/C2_archive/src/reference/unused_member.cpp` — C2 静态库归档练习 的参考实现或观察程序。
- `Engineering_Study/exercises/C2_archive/src/reference/used_member.cpp` — C2 静态库归档练习 的参考实现或观察程序。
- `Engineering_Study/exercises/C2_archive/src/student/archive_value.cpp` — C2 静态库归档练习 的学生实现入口。
- `Engineering_Study/exercises/C2_archive/src/student/archive_value.hpp` — C2 静态库归档练习 的学生实现入口。

### D1_shared_library

- `Engineering_Study/exercises/D1_shared_library/CMakeLists.txt` — D1 动态库与加载器练习 的 leaf target、Reference/Student 和 CTest 编排。
- `Engineering_Study/exercises/D1_shared_library/README.md` — D1 动态库与加载器练习 的教学任务、运行方式、验收与解析。
- `Engineering_Study/exercises/D1_shared_library/checks/linked_check.cpp` — D1 动态库与加载器练习 的独立检查程序，覆盖 Student/Reference 或负向验收。
- `Engineering_Study/exercises/D1_shared_library/checks/loader_check.cpp` — D1 动态库与加载器练习 的独立检查程序，覆盖 Student/Reference 或负向验收。
- `Engineering_Study/exercises/D1_shared_library/cmake/expect_loader_failure.cmake` — D1 动态库与加载器练习 的 CMake 辅助脚本或 package config 模板。
- `Engineering_Study/exercises/D1_shared_library/cmake/run_missing_dll.cmake` — D1 动态库与加载器练习 的 CMake 辅助脚本或 package config 模板。
- `Engineering_Study/exercises/D1_shared_library/include/d1_runtime.h` — D1 动态库与加载器练习 的练习源文件。
- `Engineering_Study/exercises/D1_shared_library/src/runtime.cpp` — D1 动态库与加载器练习 的练习源文件。

### E1_abi

- `Engineering_Study/exercises/E1_abi/CMakeLists.txt` — E1 ABI 边界练习 的 leaf target、Reference/Student 和 CTest 编排。
- `Engineering_Study/exercises/E1_abi/README.md` — E1 ABI 边界练习 的教学任务、运行方式、验收与解析。
- `Engineering_Study/exercises/E1_abi/c_consumer/main.c` — E1 ABI 边界练习 的真实 C consumer，用于展示 extern C 边界。
- `Engineering_Study/exercises/E1_abi/checks/abi_contract_check.cpp` — E1 ABI 边界练习 的独立检查程序，覆盖 Student/Reference 或负向验收。
- `Engineering_Study/exercises/E1_abi/checks/bad_alloc_check.cpp` — E1 ABI 边界练习 的独立检查程序，覆盖 Student/Reference 或负向验收。
- `Engineering_Study/exercises/E1_abi/include/lesson_api.h` — E1 ABI 边界练习 的练习源文件。
- `Engineering_Study/exercises/E1_abi/observation/layout.cpp` — E1 ABI 边界练习 的观察样例，用于展示预处理、符号或布局证据。
- `Engineering_Study/exercises/E1_abi/src/reference/lesson_api.cpp` — E1 ABI 边界练习 的参考实现或观察程序。
- `Engineering_Study/exercises/E1_abi/src/student/lesson_api.cpp` — E1 ABI 边界练习 的学生实现入口。

### F1_cmake_targets

- `Engineering_Study/exercises/F1_cmake_targets/CMakeLists.txt` — F1 CMake target 实现练习 的 leaf target、Reference/Student 和 CTest 编排。
- `Engineering_Study/exercises/F1_cmake_targets/README.md` — F1 CMake target 实现练习 的教学任务、运行方式、验收与解析。
- `Engineering_Study/exercises/F1_cmake_targets/checks/reference_check.cpp` — F1 CMake target 实现练习 的独立检查程序，覆盖 Student/Reference 或负向验收。
- `Engineering_Study/exercises/F1_cmake_targets/checks/student_check.cpp` — F1 CMake target 实现练习 的独立检查程序，覆盖 Student/Reference 或负向验收。
- `Engineering_Study/exercises/F1_cmake_targets/reference/include/f1_math/math.hpp` — F1 CMake target 实现练习 的参考实现或观察程序。
- `Engineering_Study/exercises/F1_cmake_targets/reference/math.cpp` — F1 CMake target 实现练习 的参考实现或观察程序。
- `Engineering_Study/exercises/F1_cmake_targets/src/student/student.cpp` — F1 CMake target 实现练习 的学生实现入口。
- `Engineering_Study/exercises/F1_cmake_targets/src/student/student.hpp` — F1 CMake target 实现练习 的学生实现入口。

### F2_dependencies

- `Engineering_Study/exercises/F2_dependencies/CMakeLists.txt` — F2 find_package/依赖传播练习 的 leaf target、Reference/Student 和 CTest 编排。
- `Engineering_Study/exercises/F2_dependencies/README.md` — F2 find_package/依赖传播练习 的教学任务、运行方式、验收与解析。
- `Engineering_Study/exercises/F2_dependencies/checks/delegation_check.cpp` — F2 find_package/依赖传播练习 的独立检查程序，覆盖 Student/Reference 或负向验收。
- `Engineering_Study/exercises/F2_dependencies/checks/reference_check.cpp` — F2 find_package/依赖传播练习 的独立检查程序，覆盖 Student/Reference 或负向验收。
- `Engineering_Study/exercises/F2_dependencies/checks/spy_provider/include/f2_provider/provider.hpp` — F2 find_package/依赖传播练习 的 spy provider 检查夹具，验证学生代码是否经 target 依赖调用 provider。
- `Engineering_Study/exercises/F2_dependencies/checks/spy_provider/spy_provider.cpp` — F2 find_package/依赖传播练习 的 spy provider 检查夹具，验证学生代码是否经 target 依赖调用 provider。
- `Engineering_Study/exercises/F2_dependencies/checks/spy_provider/spy_provider.hpp` — F2 find_package/依赖传播练习 的 spy provider 检查夹具，验证学生代码是否经 target 依赖调用 provider。
- `Engineering_Study/exercises/F2_dependencies/checks/student_check.cpp` — F2 find_package/依赖传播练习 的独立检查程序，覆盖 Student/Reference 或负向验收。
- `Engineering_Study/exercises/F2_dependencies/provider_fixture/CMakeLists.txt` — F2 find_package/依赖传播练习 的 leaf target、Reference/Student 和 CTest 编排。
- `Engineering_Study/exercises/F2_dependencies/provider_fixture/LICENSE.txt` — F2 find_package/依赖传播练习 的本地 provider package 夹具。
- `Engineering_Study/exercises/F2_dependencies/provider_fixture/cmake/F2ProviderConfig.cmake.in` — F2 find_package/依赖传播练习 的 CMake 辅助脚本或 package config 模板。
- `Engineering_Study/exercises/F2_dependencies/provider_fixture/include/f2_provider/provider.hpp` — F2 find_package/依赖传播练习 的本地 provider package 夹具。
- `Engineering_Study/exercises/F2_dependencies/provider_fixture/src/provider.cpp` — F2 find_package/依赖传播练习 的本地 provider package 夹具。
- `Engineering_Study/exercises/F2_dependencies/reference/include/f2_provider/provider.hpp` — F2 find_package/依赖传播练习 的参考实现或观察程序。
- `Engineering_Study/exercises/F2_dependencies/reference/provider.cpp` — F2 find_package/依赖传播练习 的参考实现或观察程序。
- `Engineering_Study/exercises/F2_dependencies/src/student/student.cpp` — F2 find_package/依赖传播练习 的学生实现入口。
- `Engineering_Study/exercises/F2_dependencies/src/student/student.hpp` — F2 find_package/依赖传播练习 的学生实现入口。

### G1_diagnostics

- `Engineering_Study/exercises/G1_diagnostics/CMakeLists.txt` — G1 诊断与 fuzz 检查练习 的 leaf target、Reference/Student 和 CTest 编排。
- `Engineering_Study/exercises/G1_diagnostics/README.md` — G1 诊断与 fuzz 检查练习 的教学任务、运行方式、验收与解析。
- `Engineering_Study/exercises/G1_diagnostics/checks/parser_contract.hpp` — G1 诊断与 fuzz 检查练习 的独立检查程序，覆盖 Student/Reference 或负向验收。
- `Engineering_Study/exercises/G1_diagnostics/checks/reference_check.cpp` — G1 诊断与 fuzz 检查练习 的独立检查程序，覆盖 Student/Reference 或负向验收。
- `Engineering_Study/exercises/G1_diagnostics/checks/student_check.cpp` — G1 诊断与 fuzz 检查练习 的独立检查程序，覆盖 Student/Reference 或负向验收。
- `Engineering_Study/exercises/G1_diagnostics/fuzz_bad/always_42_parser.cpp` — G1 诊断与 fuzz 检查练习 的负向 fuzz 变体。
- `Engineering_Study/exercises/G1_diagnostics/fuzz_corpus/invalid_4x.txt` — G1 诊断与 fuzz 检查练习 的 fuzz 种子语料。
- `Engineering_Study/exercises/G1_diagnostics/fuzz_corpus/invalid_one_digit.txt` — G1 诊断与 fuzz 检查练习 的 fuzz 种子语料。
- `Engineering_Study/exercises/G1_diagnostics/fuzz_corpus/valid_00.txt` — G1 诊断与 fuzz 检查练习 的 fuzz 种子语料。
- `Engineering_Study/exercises/G1_diagnostics/fuzz_corpus/valid_42.txt` — G1 诊断与 fuzz 检查练习 的 fuzz 种子语料。
- `Engineering_Study/exercises/G1_diagnostics/fuzz_parse.cpp` — G1 诊断与 fuzz 检查练习 的练习源文件。
- `Engineering_Study/exercises/G1_diagnostics/reference/parser.cpp` — G1 诊断与 fuzz 检查练习 的参考实现或观察程序。
- `Engineering_Study/exercises/G1_diagnostics/reference/parser.hpp` — G1 诊断与 fuzz 检查练习 的参考实现或观察程序。
- `Engineering_Study/exercises/G1_diagnostics/scripts/verify_fuzz.py` — G1 诊断与 fuzz 检查练习 的练习源文件。
- `Engineering_Study/exercises/G1_diagnostics/src/student/parser.cpp` — G1 诊断与 fuzz 检查练习 的学生实现入口。
- `Engineering_Study/exercises/G1_diagnostics/src/student/parser.hpp` — G1 诊断与 fuzz 检查练习 的学生实现入口。
- `Engineering_Study/exercises/G1_diagnostics/static_analysis/README.md` — G1 诊断与 fuzz 检查练习 的教学任务、运行方式、验收与解析。
- `Engineering_Study/exercises/G1_diagnostics/static_analysis/null_state.cpp` — G1 诊断与 fuzz 检查练习 的静态分析观察材料。
- `Engineering_Study/exercises/G1_diagnostics/unsafe/asan_fault.cpp` — G1 诊断与 fuzz 检查练习 的 sanitizer 观察样例。

### G2_build_cost

- `Engineering_Study/exercises/G2_build_cost/CMakeLists.txt` — G2 构建成本测量实验 的 leaf target、Reference/Student 和 CTest 编排。
- `Engineering_Study/exercises/G2_build_cost/README.md` — G2 构建成本测量实验 的教学任务、运行方式、验收与解析。
- `Engineering_Study/exercises/G2_build_cost/reference/alpha.cpp` — G2 构建成本测量实验 的参考实现或观察程序。
- `Engineering_Study/exercises/G2_build_cost/reference/beta.cpp` — G2 构建成本测量实验 的参考实现或观察程序。
- `Engineering_Study/exercises/G2_build_cost/reference/common.cpp` — G2 构建成本测量实验 的参考实现或观察程序。
- `Engineering_Study/exercises/G2_build_cost/reference/gamma.cpp` — G2 构建成本测量实验 的参考实现或观察程序。
- `Engineering_Study/exercises/G2_build_cost/reference/include/g2_common/common.hpp` — G2 构建成本测量实验 的参考实现或观察程序。
- `Engineering_Study/exercises/G2_build_cost/reference/include/g2_common/heavy.hpp` — G2 构建成本测量实验 的参考实现或观察程序。
- `Engineering_Study/exercises/G2_build_cost/reference/main.cpp` — G2 构建成本测量实验 的参考实现或观察程序。
- `Engineering_Study/exercises/G2_build_cost/scripts/measure_build.ps1` — G2 构建成本测量实验 的练习源文件。
- `Engineering_Study/exercises/G2_build_cost/scripts/measure_build.py` — G2 构建成本测量实验 的练习源文件。

### H1_modules

- `Engineering_Study/exercises/H1_modules/CMakeLists.txt` — H1 C++ modules 观察实验 的 leaf target、Reference/Student 和 CTest 编排。
- `Engineering_Study/exercises/H1_modules/README.md` — H1 C++ modules 观察实验 的教学任务、运行方式、验收与解析。
- `Engineering_Study/exercises/H1_modules/cmake/run_package_consumer.cmake` — H1 C++ modules 观察实验 的 CMake 辅助脚本或 package config 模板。
- `Engineering_Study/exercises/H1_modules/header_baseline/geometry.hpp` — H1 C++ modules 观察实验 的头文件基线对照程序。
- `Engineering_Study/exercises/H1_modules/header_baseline/main.cpp` — H1 C++ modules 观察实验 的头文件基线对照程序。
- `Engineering_Study/exercises/H1_modules/negative/direct_name_hidden.cpp` — H1 C++ modules 观察实验 的故意失败样例，用于验证错误阶段和诊断边界。
- `Engineering_Study/exercises/H1_modules/package/consumer/CMakeLists.txt` — H1 C++ modules 观察实验 的 leaf target、Reference/Student 和 CTest 编排。
- `Engineering_Study/exercises/H1_modules/package/consumer/src/main.cpp` — H1 C++ modules 观察实验 的 modules package/consumer 安装消费样例。
- `Engineering_Study/exercises/H1_modules/package/library/CMakeLists.txt` — H1 C++ modules 观察实验 的 leaf target、Reference/Student 和 CTest 编排。
- `Engineering_Study/exercises/H1_modules/package/library/cmake/h1_geometry_moduleConfig.cmake.in` — H1 C++ modules 观察实验 的 CMake 辅助脚本或 package config 模板。
- `Engineering_Study/exercises/H1_modules/package/library/src/h1_geometry.ixx` — H1 C++ modules 观察实验 的 modules package/consumer 安装消费样例。
- `Engineering_Study/exercises/H1_modules/reference/fragments.ixx` — H1 C++ modules 观察实验 的参考实现或观察程序。
- `Engineering_Study/exercises/H1_modules/reference/fragments_main.cpp` — H1 C++ modules 观察实验 的参考实现或观察程序。
- `Engineering_Study/exercises/H1_modules/reference/geometry.cpp` — H1 C++ modules 观察实验 的参考实现或观察程序。
- `Engineering_Study/exercises/H1_modules/reference/geometry.ixx` — H1 C++ modules 观察实验 的参考实现或观察程序。
- `Engineering_Study/exercises/H1_modules/reference/geometry.partition.ixx` — H1 C++ modules 观察实验 的参考实现或观察程序。
- `Engineering_Study/exercises/H1_modules/reference/main.cpp` — H1 C++ modules 观察实验 的参考实现或观察程序。
- `Engineering_Study/exercises/H1_modules/reference/visibility.ixx` — H1 C++ modules 观察实验 的参考实现或观察程序。
- `Engineering_Study/exercises/H1_modules/reference/visibility_main.cpp` — H1 C++ modules 观察实验 的参考实现或观察程序。

### I1_import_std

- `Engineering_Study/exercises/I1_import_std/CMakeLists.txt` — I1 import std 工具链实验 的 leaf target、Reference/Student 和 CTest 编排。
- `Engineering_Study/exercises/I1_import_std/README.md` — I1 import std 工具链实验 的教学任务、运行方式、验收与解析。
- `Engineering_Study/exercises/I1_import_std/direct-cl/direct_import_std.cmd` — I1 import std 工具链实验 的 MSVC cl 直连 import std 演示脚本。
- `Engineering_Study/exercises/I1_import_std/reference/main.cpp` — I1 import std 工具链实验 的参考实现或观察程序。

### J1_package

- `Engineering_Study/exercises/J1_package/CMakeLists.txt` — J1 包安装与重定位练习 的 leaf target、Reference/Student 和 CTest 编排。
- `Engineering_Study/exercises/J1_package/README.md` — J1 包安装与重定位练习 的教学任务、运行方式、验收与解析。
- `Engineering_Study/exercises/J1_package/cmake/LessonPackageConfig.cmake.in` — J1 包安装与重定位练习 的 CMake 辅助脚本或 package config 模板。
- `Engineering_Study/exercises/J1_package/cmake/run_package_roundtrip.cmake` — J1 包安装与重定位练习 的 CMake 辅助脚本或 package config 模板。
- `Engineering_Study/exercises/J1_package/consumer/CMakeLists.txt` — J1 包安装与重定位练习 的 leaf target、Reference/Student 和 CTest 编排。
- `Engineering_Study/exercises/J1_package/consumer/main.cpp` — J1 包安装与重定位练习 的安装后 find_package consumer。

### Engineering references

- `Engineering_Study/references/coverage.md` — Engineering Study 覆盖矩阵与边界说明。
- `Engineering_Study/references/implementation-spec.md` — Engineering Study 实施规格与冻结约束。
- `Engineering_Study/references/quality-report.md` — Engineering Study 质量报告，汇总作者验证与已知边界。
- `Engineering_Study/references/standards-and-toolchains.md` — 标准、编译器和工具链依据说明。
- `Engineering_Study/references/validation/freeze_snapshot.py` — Engineering Study 交付冻结快照脚本源码，用于记录可复核文件状态。
- `Engineering_Study/references/validation/run_integration.py` — Engineering Study 集成验证脚本源码，用于复现 configure/build/CTest 检查链。

## Coroutine_Study 修改与新增资料

- `Coroutine_Study/14-第三阶段结课-mini协程库实现.md` — Coroutine Study 教学正文更新。
- `Coroutine_Study/README.md` — Coroutine Study 总入口更新，衔接课程状态与验证资料。
- `Coroutine_Study/exercises/BUILD_GUIDE.md` — Coroutine 练习构建说明更新。
- `Coroutine_Study/exercises/CMakeLists.txt` — Coroutine 练习 CMake 聚合入口更新。
- `Coroutine_Study/exercises/CMakePresets.json` — Coroutine 练习 preset 更新。
- `Coroutine_Study/exercises/Capstone4_rpc_framework/CMakeLists.txt` — Coroutine Capstone4 RPC 框架练习修订或验证入口。
- `Coroutine_Study/exercises/Capstone4_rpc_framework/README.md` — Coroutine Capstone4 RPC 框架练习修订或验证入口。
- `Coroutine_Study/exercises/Capstone4_rpc_framework/src/main.cpp` — Coroutine Capstone4 RPC 框架练习修订或验证入口。
- `Coroutine_Study/exercises/Capstone5_mini_corolib/CMakeLists.txt` — Coroutine Capstone5 mini_corolib 生命周期/错误通道修订或验证入口。
- `Coroutine_Study/exercises/Capstone5_mini_corolib/README.md` — Coroutine Capstone5 mini_corolib 生命周期/错误通道修订或验证入口。
- `Coroutine_Study/exercises/Capstone5_mini_corolib/include/mini/as_awaitable.hpp` — Coroutine Capstone5 mini_corolib 生命周期/错误通道修订或验证入口。
- `Coroutine_Study/exercises/Capstone5_mini_corolib/include/mini/async_scope.hpp` — Coroutine Capstone5 mini_corolib 生命周期/错误通道修订或验证入口。
- `Coroutine_Study/exercises/Capstone5_mini_corolib/include/mini/sync_wait.hpp` — Coroutine Capstone5 mini_corolib 生命周期/错误通道修订或验证入口。
- `Coroutine_Study/exercises/Capstone5_mini_corolib/include/mini/when_all.hpp` — Coroutine Capstone5 mini_corolib 生命周期/错误通道修订或验证入口。
- `Coroutine_Study/exercises/Capstone5_mini_corolib/tests/as_awaitable_test.cpp` — Coroutine Capstone5 mini_corolib 生命周期/错误通道修订或验证入口。
- `Coroutine_Study/exercises/Capstone5_mini_corolib/tests/generator_test.cpp` — Coroutine Capstone5 mini_corolib 生命周期/错误通道修订或验证入口。
- `Coroutine_Study/exercises/Capstone5_mini_corolib/tests/scope_test.cpp` — Coroutine Capstone5 mini_corolib 生命周期/错误通道修订或验证入口。
- `Coroutine_Study/exercises/Capstone5_mini_corolib/tests/sync_wait_test.cpp` — Coroutine Capstone5 mini_corolib 生命周期/错误通道修订或验证入口。
- `Coroutine_Study/exercises/Capstone5_mini_corolib/tests/task_test.cpp` — Coroutine Capstone5 mini_corolib 生命周期/错误通道修订或验证入口。
- `Coroutine_Study/exercises/Capstone5_mini_corolib/tests/when_all_test.cpp` — Coroutine Capstone5 mini_corolib 生命周期/错误通道修订或验证入口。
- `Coroutine_Study/exercises/H2_std_execution_task/CMakeLists.txt` — Coroutine 练习 CMake 聚合入口更新。
- `Coroutine_Study/exercises/J1_eight_pitfalls/CMakeLists.txt` — Coroutine 练习 CMake 聚合入口更新。
- `Coroutine_Study/exercises/cmake/AddExercise.cmake` — Coroutine 练习 CMake 或运行时验证接线更新。
- `Coroutine_Study/exercises/runtime_tests/CMakeLists.txt` — Coroutine 练习 CMake 聚合入口更新。
- `Coroutine_Study/references/coverage.md` — Coroutine Study 覆盖矩阵。
- `Coroutine_Study/references/quality-report.md` — Coroutine Study 质量报告。

## Concurrency_Study 修改与新增资料

- `Concurrency_Study/README.md` — Concurrency Study 总入口，加入 C01 桥接与状态说明。
- `Concurrency_Study/references/coverage.md` — Concurrency Study 覆盖说明更新，标注 C01 桥接和非完整 C08/C13 边界。
- `Concurrency_Study/references/quality-report.md` — Concurrency Study 质量报告新增第 8 节，记录 C01 补充验收边界。
- `Concurrency_Study/references/standards-and-implementations.md` — Concurrency Study 标准/实现索引更新，加入 C++29/N5055 相关条目。

## 独立审查与最终审查报告

- `Concurrency_Study/references/validation/c01-supplement-independent-review.md` — 审查或验证报告，逐文件保留结论、依据和边界。
- `Coroutine_Study/references/validation/c01-independent-review.md` — 审查或验证报告，逐文件保留结论、依据和边界。
- `Engineering_Study/references/validation/binary-package-independent-review.md` — 审查或验证报告，逐文件保留结论、依据和边界。
- `Engineering_Study/references/validation/binary-package/author-validation-summary.md` — 审查或验证报告，逐文件保留结论、依据和边界。
- `Engineering_Study/references/validation/final-teaching-review.md` — 审查或验证报告，逐文件保留结论、依据和边界。
- `Engineering_Study/references/validation/final-technical-review.md` — 审查或验证报告，逐文件保留结论、依据和边界。
- `Engineering_Study/references/validation/foundations-independent-review.md` — 审查或验证报告，逐文件保留结论、依据和边界。
- `Engineering_Study/references/validation/odr-independent-review.md` — 审查或验证报告，逐文件保留结论、依据和边界。
- `Engineering_Study/references/validation/toolchain-independent-review.md` — 审查或验证报告，逐文件保留结论、依据和边界。

## 验证原始数据目录汇总

- `Engineering_Study/references/validation/toolchain/581-verify-g1-contract-controlled-failure.py` — 独立复验G1有效实现及三类坏变体的Debug/Release受控失败路径。
- `Engineering_Study/references/validation/toolchain/581-verify-g1-contract-controlled-failure.command.cmd` — 上述独立检查的本机实际调用。
- `Engineering_Study/references/validation/integration/recheck-g1.command.cmd` — G1最后检查头变更的原整合树Debug/Release定点复验命令。
- `Engineering_Study/references/validation/measurements/README.md` — 正式72轮测量表、分阶段统计、工作量解释与结论边界。
- `Engineering_Study/references/validation/measurements/g2-formal-r1/` — 正式测量参数/顺序/样本、实际调用、硬件记录及逐命令原始结果；构建副本与二进制仍在ignored build。
- `Engineering_Study/references/validation/snapshots/` — 测量前与最终交付的源码/证据SHA256清单，不把历史快照冒充当前版本。

- `Concurrency_Study/references/validation/c01-supplement/` — Concurrency C01 桥接补充 configure/build/CTest JSON/XML 证据；原始文件按目录合并，包含扩展名：.json, .xml。
- `Coroutine_Study/references/validation/c01-supplement/` — Coroutine C01 补充构建、CTest、正反变体和摘要证据；原始文件按目录合并，包含扩展名：.hpp, .txt。
- `Engineering_Study/references/validation/binary-package/` — D/E/J 二进制包链路作者验证 stdout/stderr、fingerprint 与 summary；原始文件按目录合并，包含扩展名：.sha256, .txt。
- `Engineering_Study/references/validation/environment/` — 本机 Ninja/CMake 环境探测证据；原始文件按目录合并，包含扩展名：.json。
- `Engineering_Study/references/validation/foundations/` — A/B/C 基础章节构建、CTest、负向检查与符号观察证据；原始文件按目录合并，包含扩展名：.cpp, .json, .sha256, .txt。
- `Engineering_Study/references/validation/integration/` — Engineering 集成验证命令、结果与原始输出；原始文件按目录合并，包含扩展名：.json, .txt, .xml。
- `Engineering_Study/references/validation/odr/` — C1 ODR 样章作者验证、审查变体和原始日志；原始文件按目录合并，包含扩展名：.json, .sha256, .txt。
- `Engineering_Study/references/validation/shared-tools/` — 公共 helper/check 自检证据；原始文件按目录合并，包含扩展名：.json, .sha256, .xml。
- `Engineering_Study/references/validation/toolchain-probe/` — MSVC/CMake/Ninja/toolchain 探测证据；原始文件按目录合并，包含扩展名：.cmd, .cpp, .in, .ixx, .json, .md, .txt。
- `Engineering_Study/references/validation/toolchain/` — F/G/H/I toolchain 与构建成本验证证据；原始文件按目录合并，包含扩展名：.cmd, .json, .md, .py, .txt, <no-ext>。

## 未在本清单中声明的内容

- 课程最终状态从质量报告及非作者终审记录读取；清单自身不替代验证。
- 未包含 `build/`、`out/`、`.omx/`、Python 缓存、用户既有 guide 和 P2 练习改动。
- 未把 validation 原始 stdout/stderr/json/xml 逐文件展开；这些证据按目录保留，方便最终用户定位。

