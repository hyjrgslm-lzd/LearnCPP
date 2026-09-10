# C04 L11 样章非作者审查

结论：ITERATE。

审查时间：2026-09-09。审查范围为 `chapters/11-customization-points.md`、`exercises/L11_customization/**`（排除 build）、`references/validation/sample-author/summary.md`，并只读核对共享 `exercises/cmake/StudySetup.cmake`、`exercises/tools/compile_case.py`。Reference 在正文/Starter 初审后才打开对照。

## 版本绑定

以下 SHA256 排除 build 与本审查证据自身：

```text
d23def4e422db2bbb6206559404420ec72487ce713df588f207248a06f4d8e36  C04_Generic_CompileTime_Reflection/chapters/11-customization-points.md
a332d3cefc07dae2ee25d4288356fbfe35d5e3f1244657961e50f1f726d9c22e  C04_Generic_CompileTime_Reflection/exercises/L11_customization/CMakeLists.txt
86215e3555b6443c12d12a04f6944c00f1ea16503178f649aaf0bd96e8750ff8  C04_Generic_CompileTime_Reflection/exercises/L11_customization/README.md
6ff4f2cabe14e50d8af7ecc57da3aa43b2a825811531d57aa44ba28a17b9b9d8  C04_Generic_CompileTime_Reflection/exercises/L11_customization/checks/read_value_checks.cpp
272a71a21e79718cf3d48d33489bc4645d477691442beb3100d85146c3d1b294  C04_Generic_CompileTime_Reflection/exercises/L11_customization/observations/baseline.cpp
c0ddd034b6a008fe6755226c332fbeb6fb1675c515e74f3d8fad2df4b5224ee8  C04_Generic_CompileTime_Reflection/exercises/L11_customization/src/student/read_value.hpp
31cf2f65ebc71d474e2c78219627f4da12aab4aaa5cd805c65e8561cf1eba5f7  C04_Generic_CompileTime_Reflection/exercises/L11_customization/src/reference/read_value.hpp
31cf2f65ebc71d474e2c78219627f4da12aab4aaa5cd805c65e8561cf1eba5f7  C04_Generic_CompileTime_Reflection/exercises/L11_customization/validation/good/read_value.hpp
ad359bc499fee10e2e340a15404c459dfe714f442670fcf1e5c1b32fcc23144c  C04_Generic_CompileTime_Reflection/exercises/L11_customization/validation/bad/read_value.hpp
8ff3cdc2192b5b31f050ae25ae51ff78ce5bf773a33d5ad2640110869a688b9f  C04_Generic_CompileTime_Reflection/exercises/cmake/StudySetup.cmake
a62d8b82d1d17fe4a230f088d4a6c733521c8ad78e0531660d2f6601bb0a33d5  C04_Generic_CompileTime_Reflection/exercises/cmake/expect_failure.cmake
80c6025f85bf997ef1293226e63f5377b80ef7558759bc13b3256fd5f55a0644  C04_Generic_CompileTime_Reflection/exercises/tools/compile_case.py
bb3edd054ae16fc8b743b38d2c83860a6c27c59eeec2795f4445e3e27235a3ba  C04_Generic_CompileTime_Reflection/references/implementation-spec.md
971e2f48729fa198e41dab09b8dc8b859745661ad3f184c8e061e2a11ad5e6fc  CONTENT_REFACTORING_GUIDE.md
```

## 验证

- `01-configure.json`：`cmake -S C04_Generic_CompileTime_Reflection/exercises/L11_customization -B C04_Generic_CompileTime_Reflection/exercises/L11_customization/build/review-01 -G "Visual Studio 18 2026" -A x64`，PASS。
- `02-build-debug.json`：`cmake --build C04_Generic_CompileTime_Reflection/exercises/L11_customization/build/review-01 --config Debug`，PASS。
- `03-ctest-debug.json`：`ctest --test-dir C04_Generic_CompileTime_Reflection/exercises/L11_customization/build/review-01 -C Debug --output-on-failure`，PASS，4/4 tests passed：reference、validation_good、validation_bad_rejected、baseline。
- `04-student-configure.json`：Student-only configure with `-DGENERIC_STUDY_BUILD_REFERENCE=OFF -DGENERIC_STUDY_TEST_STUDENTS=ON`，PASS。
- `05-student-build-debug.json`：Student-only Debug build，PASS。
- `06-student-ctest-debug-expected-fail.json`：Student-only CTest expected exit 8 and expected text `check failed: no-path object must not satisfy c04_readable`，PASS。

`lsp_diagnostics` 与 `ast_grep_search` 在当前工具面不可用；已用 MSVC build/CTest 替代类型安全验证，并用 `rg` 扫描 hardcoded secret / silent fallback / SKIP / TODO 等模式，未发现安全项。

## 问题

[HIGH][实验/技术] `checks/read_value_checks.cpp:52` 定义的 `NeedsArgument` 只有不可调用成员，没有合法 ADL 备选；`checks/read_value_checks.cpp:127` 只断言它不可读。规格和正文要求“成员不可调用时才尝试 ADL”：`implementation-spec.md` 的公共接口要求合法 member 优先，否则合法 ADL；`11-customization-points.md:180` 和 `11-customization-points.md:196` 也明确要求先判定 member 不成立，再尝试 ADL。当前 checker 没有正向覆盖“存在同名但不可调用 member 时，仍能选择合法 ADL”的路径，会放过一类只处理“无 member 名称才走 ADL”的错误实现。

最小修复：在 fixture 中增加一个类型，例如 `MemberNeedsArgumentButAdl`，含 `int read_value(int) const` 和同 namespace 的 `int& read_value(MemberNeedsArgumentButAdl&) noexcept`；在 `type_contract_ok`、`static_assert(c04_readable<...&>)`、运行期引用身份、写回、`noexcept` 检查中覆盖该类型。bad/good/reference 路径复跑后记录新证据。

[HIGH][教学/实验] 样章承诺“只有当需求真的变化、原来的代码真的破坏契约时，才引入下一层泛型机制”（`11-customization-points.md:3`），通用指引要求样章包含朴素正确基线、问题推导、受控实验、答案和可运行代码。当前 `observations/baseline.cpp:14` 只运行正确已知类型基线，最终 checker 覆盖 Reference/good/bad，但没有把正文中用来推进的错误版本变成可运行/可诊断的最小实验，例如 `auto` 丢引用、缺少 `std::forward` 导致右值成员匹配错误、ADL 未隔离导致递归/候选污染、写死 `noexcept` 的异常后果。结果是读者能看到解释和最终答案，但看不到“改进前问题”如何被独立复现。

最小修复：补 2-4 个独立 observation 或 diagnostic compile case，覆盖正文实际用来推进的破坏契约点；每个 case 只证明一个点，并在 README/作者证据中链接命令和结果。不要把最终 Reference 通过当作这些前置问题的复现实验。

[LOW][报告一致性] `references/validation/sample-author/summary.md:1` 起为英文说明性记录。原始命令、日志、错误文本可以保留英文，但本仓库用户可见报告默认应为中文；后续收尾建议转中文，保留英文原始日志路径即可。

## 已通过项

- 教学正文没有发现明显未讲先用：`decltype(auto)`、转发引用、`std::forward`、ADL、poison pill、`requires`、hidden friend、member 优先、`noexcept(noexcept(expr))` 均在使用前解释。
- Reference 与 good 实现的核心表达式一致：member 分支 `std::forward<T>(object).read_value()`，ADL 分支在 detail namespace 中经 poison pill 隔离，返回 `decltype(auto)`，`noexcept` 来自实际表达式。
- Student 占位不包含 Reference，独立编译；在 Student-only CTest 中按预期失败，不是 SKIP 或完成标记作假。
- 普通测试通过 `c04_add_test` 默认 `TIMEOUT 30`；本次 configure/build/ctest 外部 recorder 分别使用 180/900/180 秒上限。

## 推荐

ITERATE。先补齐上述两个 HIGH 项并复跑 Reference/good/bad/Student-only 证据；两项关闭前，不建议放行样章作为 C04 后续批量专题模板。
