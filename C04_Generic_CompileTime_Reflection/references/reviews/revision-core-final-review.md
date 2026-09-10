# C04 L07-L10 核心修正独立复验

日期：2026-09-10
身份：非作者复验
结论：FAIL

本轮只复验 L07-L10 `validation/good` 独立性、L10 Reference/diagnostics、chapter08/10 对应说明、Student 初态和 blind-core 记录边界。未审 F01、公共 helper 全量语义、全 C04 矩阵或 C06 并行变更。

## 通过项

- L07-L10 `validation/good` 已不再 include `src/reference`。证据：`references/validation/revision-core-final-review/07-good-no-reference-include.json`，`rg` 期望 exit 1，wrapper 判定 PASS。
- 当前 good 与 Reference SHA 均不同。证据：`references/validation/revision-core-final-review/15-summary-latest.json`。
- 最新独立配置与核心构建通过。证据：`09-configure-latest.json` 和 `10-build-core-latest.json` 均 PASS，构建目标为 `L07_packs_validation_good`、`L08_type_lists_validation_good`、`L09_tuple_validation_good`、`L10_constexpr_validation_good`、`L10_constexpr_reference`。
- Student 仍可编译但按 checker 文本失败。证据：`12-configure-student-latest.json`、`13-build-student-latest.json` 均 PASS；`14-ctest-student-expected-fail-latest.json` wrapper PASS，底层 CTest exit 8，4/4 student 失败，失败文本分别来自 L07/L08/L09/L10 checker。
- L10 的非法数字、空串、内嵌 NUL、符号、空白和 parameter-constant 诊断均通过 targeted CTest。证据：`11-ctest-good-l10-diagnostics-latest.json` 中这些 test Passed；对应 `_diag/*/evidence-1.json` 均为 configure PASS、control PASS、subject FAIL、runner verdict PASS。
- `revision-core-blind-review.md` 已如实区分初稿错误、v2 与 Reference 后置比较；`references/validation/revision-20260910/blind-core/post-review/repair.json` 记录 L10 good 从 `c727...` 到当前 `65ec...` 是事后技术修复，不冒称 blind。

## 阻断项

`L10_decimal_overflow_diagnostic` 仍未通过。最新 targeted CTest：

```text
ctest --test-dir C04_Generic_CompileTime_Reflection\exercises\build\revision-core-final-review -C Debug -R "^(L07_packs_validation_good|L08_type_lists_validation_good|L08_lazy_type_eager_bad|L09_tuple_validation_good|L10_constexpr_reference|L10_constexpr_validation_good|L10_.*_diagnostic)$" --output-on-failure
```

结果：13 项中 12 PASS、1 FAIL，失败项为 `L10_decimal_overflow_diagnostic`。记录文件：`references/validation/revision-core-final-review/11-ctest-good-l10-diagnostics-latest.json`。

失败不是预期的语义诊断。runner 输出：

- control PASS，compiler 为 `D:/VisualStudio2026/Installed/VC/Tools/MSVC/14.51.36231/bin/Hostx64/x64/cl.exe`。
- subject 编译非零，但输出为 MSVC `fatal error C1001: 内部编译器错误`，runner 判定 `FAIL: expected semantic diagnostic with a normal nonzero compiler exit`。
- 模板栈显示仍形成了 `decimal_parser<...,10,-2147483648,true>`，说明当前 `validation/good/constexpr_tools.hpp` 仍在溢出路径上形成了溢出的下一层模板实参，未满足“先拒绝溢出，再乘加”的门槛。

相关源码位置：

- `exercises/L10_constexpr/validation/good/constexpr_tools.hpp:42-45` 使用 `if constexpr (fits)`，但 MSVC 仍在递推表达式中形成 `Accumulator * 10 + digit` 的溢出模板实参。
- `exercises/L10_constexpr/src/reference/constexpr_tools.hpp:39-44` 的 Reference 是函数模板递归，先进入 `if constexpr (!fits)` 的拒绝分支，再只在 `else` 中递推；当前 Reference 自身 test PASS。

## 版本边界

`01-08` 是中间态证据。最新修复后必须以后续 `09-15` 为准；其中 `11-ctest-good-l10-diagnostics-latest.json` 是当前阻断证据。

本报告不把历史中间态失败冒算为当前失败；当前失败来自重新 configure 后的最新字节。也不把 MSVC ICE 当作期望拒绝通过，因为构建指南要求超时、基础设施错误和非语义诊断不能算正常拒绝。

## 建议

不要放行本核心修正。最小修复是让 `validation/good` 的 L10 overflow 路径采用 Reference 同类的函数模板递归结构，或用不会形成溢出模板实参的 checked 状态类型；修复后重跑 `09-15` 对应 latest 验证即可，不需要重跑 F01 或全课矩阵。
