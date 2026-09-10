# frontier P2747/P3822 修订验证

范围：`chapters/14-splicing-generation.md`、`chapters/15-annotations-frontier.md`、`exercises/F01_frontier/**`、`references/standards-and-implementations.md`。

规范核对输入：

- N5050：C++26 draft，表22含`__cpp_constexpr 202406L`、`__cpp_constexpr_exceptions 202411L`、`__cpp_expansion_statements 202506L`、`__cpp_impl_reflection 202603L`、C++26类型/值包`__cpp_pack_indexing 202311L`。
- N5054：C++29 draft，表22含`__cpp_concepts 202606L`、`__cpp_pack_indexing 202606L`。
- P1306R5：`template for`展开语句，`break`跳出展开序列，`continue`进入下一个展开体；类型遍历经反射值域表达。
- P2747R2：constexpr placement new；placement storage约束和`__cpp_lib_constexpr_new`库宏。
- P3822R2：requires复合要求支持`noexcept(constant-expression)`；条件为`true`时要求表达式不抛，为`false`时关闭该要求，条件不能转`bool`时要求不满足。

证据：

| 文件 | 结果 |
|---|---|
| `frontier-configure-f01-p2747-p3822-02.json` | PASS；MSVC 19.51，新增`F01_HAS_CONSTEXPR_PLACEMENT_NEW`和`F01_HAS_CONDITIONAL_NOEXCEPT_REQUIREMENT`行为探测均为Failed |
| `frontier-build-f01-p2747-p3822-02.json` | PASS；F01专用Release构建成功 |
| `frontier-ctest-f01-p2747-p3822-02.json` | PASS；14/14 capability tests registered，0 failed，14 skipped |
| `frontier-build-f01-force-placement-02.json` | PASS证据；期望exit 1，强制进入P2747主体后编译失败，未被SKIP吞掉 |
| `frontier-build-f01-force-noexcept-02.json` | PASS证据；期望exit 1，强制进入P3822主体后编译失败，未被SKIP吞掉 |

本机没有把P2747标准负例跑成PASS/FAIL结论：正例能力探测未通过，所以`C04_P2747_NEGATIVE_WRONG_STORAGE`只作为源码入口保留。
