# L11 customization 样章作者证据

2026-09-09。r1 初稿已经过非作者审查，结论为 ITERATE。本轮修复两个阻断：补齐“成员同名但不可调用时仍选择合法 ADL”的 checker 正例；补齐 `auto` 丢引用、忘记 `std::forward`、ADL 污染/递归风险、写死 `noexcept` 的前置可运行反例。r1 没有记录这些原问题观察，本轮明确补齐后再提交候选复验。

作者范围：

- `chapters/11-customization-points.md`
- `exercises/L11_customization/CMakeLists.txt`
- `exercises/L11_customization/README.md`
- `exercises/L11_customization/checks/read_value_checks.cpp`
- `exercises/L11_customization/observations/baseline.cpp`
- `exercises/L11_customization/observations/counterexamples.cpp`
- `exercises/L11_customization/src/student/read_value.hpp`
- `exercises/L11_customization/src/reference/read_value.hpp`
- `exercises/L11_customization/validation/good/read_value.hpp`
- `exercises/L11_customization/validation/bad/read_value.hpp`

## r2 验证

- `12-r2-configure.json`：`cmake -S C04_Generic_CompileTime_Reflection/exercises/L11_customization -B build/sample-author-r2 -G "Visual Studio 18 2026" -A x64` -> PASS。
- `14-r2-counterexamples-build.json`：`cmake --build build/sample-author-r2 --config Debug --target L11_customization_counterexamples` -> PASS。`13-r2-counterexamples-build.json` 保留了本轮第一次反例代码的失败记录，原因是坏 CPO 变量声明尚未进入模板定义上下文。
- `16-r2-counterexamples-run.json`：直接运行 `build/sample-author-r2/Debug/L11_customization_counterexamples.exe` -> PASS，输出 `L11_customization counterexamples observation OK`。`15-r2-counterexamples-ctest.json` 保留了 recorder 使用 `--contains` 包住通过 CTest 时拿不到 stdout 的失败记录；该 CTest 本身为 1/1 passed。
- `17-r2-build-debug.json`：`cmake --build build/sample-author-r2 --config Debug` -> PASS。
- `18-r2-ctest-debug.json`：`ctest --test-dir build/sample-author-r2 -C Debug --output-on-failure` -> PASS，覆盖 reference、validation_good、validation_bad_rejected、baseline、counterexamples。

Student-only r2 证据：

- `19-r2-student-configure.json`：使用 `-DGENERIC_STUDY_BUILD_REFERENCE=OFF -DGENERIC_STUDY_TEST_STUDENTS=ON` 配置 -> PASS。
- `20-r2-student-build-debug.json`：Student-only Debug build -> PASS。
- `21-r2-student-ctest-debug-expected-fail.json`：Student-only CTest 预期 exit 8，并匹配 `check failed: no-path object must not satisfy c04_readable` -> PASS。

## r1 修复历史保留

- `02-build-debug.json`：初始 bad 控制实现因 checker 无条件引用绑定而编译失败。
- `03-ctest-debug.json`：初始 bad 控制拒绝失败，因为 `BAD_DIAGNOSTIC` 已包含共享 `expect_failure.cmake` 自动添加的 `check failed:` 前缀。
- `07-student-build-debug.json`：初始 Student build 因同一类无条件 checker 绑定而编译失败。

## 已知边界

- 这是 L11 样章门，不代表 C04 其他章节、Release/ASan/frontier 或整课集成完成。
- Student 实现故意不完整，但独立可编译；checker 不包含 Reference，并会拒绝 Student。
- `counterexamples.cpp` 只做安全观察：不实际执行无限递归到栈溢出，不实际调用会触发 `std::terminate()` 的写死 `noexcept` wrapper。
