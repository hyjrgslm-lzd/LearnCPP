# C04 revision final Student/good audit

日期：2026-09-10

## Verdict

PASS。

本审计只覆盖最终冻结源码的 Student 失败形态、Student include 隔离、good validation include 隔离，以及 `audit_good.py` 的 Reference include 拒绝负控。它不证明作者从未看过 Reference，也不重新证明整课 Debug/Release/meta/ASan/frontier/C05 结果。

本报告完成后停止编译；后续正式 benchmark 可以独占构建资源。

## Student audit

- 源码快照：`references/validation/revision-20260910/final-audits-student/00-code-before.json`
  - `count`: 246
  - `tree_sha256`: `42db3333f402b7b0c3769116ab0b93e260fc172c3e458794afc3f51c4b8e14b5`
  - 当前回读：246/246 SHA 匹配，missing 0，mismatch 0
- 配置：`01-configure-meta-student.json`
  - `verdict`: PASS
  - command: `cmake --preset meta-student -B F:\CPPTrain\LearnCPP\build\c04-revision-student`
- 显式 Student 目标清单：`02-student-targets.json`
  - `count`: 18
- `/showIncludes` clean-first Student 构建：`03-build-students-showincludes.json`
  - `verdict`: PASS
  - command 显式列出 18 个 `*_student` 目标，含 `--clean-first`
- Student include 审计：`04-audit-student.json`
  - `verdict`: PASS
  - `actual_include_count`: 2723
  - Reference target/path/include 违规：0
- 普通 Student 全量构建补齐：`05-build-student-all.json`
  - `verdict`: PASS
- 完整 Student CTest：`06-ctest-meta-student-expected-fail.json` + `06-ctest-meta-student.junit.xml`
  - record `verdict`: PASS
  - 子进程 `status`: FAIL
  - `exit_code`: 8
  - `expected_exit`: 8
  - JUnit：71 testcase，18 fail，53 pass
  - stdout：18 个 fail 后均有具体 `check failed: ...` 文本

失败的 18 个目标恰为：

`L01_templates_student`, `L02_deduction_student`, `L03_forwarding_student`, `L04_lookup_student`, `L05_overload_student`, `L06_constraints_student`, `L07_packs_student`, `L08_type_lists_student`, `L09_tuple_student`, `L10_constexpr_student`, `L11_customization_student`, `P1_static_record_student`, `A01_compiletime_values_student`, `A02_field_projection_student`, `A03_explicit_object_student`, `A04_type_pipelines_student`, `A05_expression_templates_student`, `U01_mp11_student`。

## good audit

- 源码快照：`references/validation/revision-20260910/final-audits-good/00-code-before.json`
  - `count`: 246
  - `tree_sha256`: `42db3333f402b7b0c3769116ab0b93e260fc172c3e458794afc3f51c4b8e14b5`
  - 当前回读：246/246 SHA 匹配，missing 0，mismatch 0
- File API query：`build/c04-revision-good-audit/.cmake/api/v1/query/codemodel-v2`
  - query 在 configure 前创建
  - build dir 独立：`F:\CPPTrain\LearnCPP\build\c04-revision-good-audit`
- 配置：`01-configure-meta-libs.json`
  - `verdict`: PASS
  - command: `cmake --preset meta-libs -S C04_Generic_CompileTime_Reflection/exercises -B F:\CPPTrain\LearnCPP\build\c04-revision-good-audit`
- codemodel 提取 good 目标：`02-good-targets.json`
  - `count`: 18
  - 目标均为 `*_validation_good`
- `/showIncludes` clean-first good 构建：`03-build-good-showincludes.json`
  - `verdict`: PASS
  - command 显式列出 18 个 `*_validation_good` 目标，含 `--clean-first`
  - `CL=/showIncludes`、`VSLANG=1033` 只在本次 PowerShell 子进程块内设置，执行后恢复
- good include 审计：`04-audit-good.json`
  - `verdict`: PASS
  - `actual_include_count`: 2742
  - `failures`: []

good 目标 18 个：

`A01_compiletime_values_validation_good`, `A02_field_projection_validation_good`, `A03_explicit_object_validation_good`, `A04_type_pipelines_validation_good`, `A05_expression_templates_validation_good`, `L01_templates_validation_good`, `L02_deduction_validation_good`, `L03_forwarding_validation_good`, `L04_lookup_validation_good`, `L05_overload_validation_good`, `L06_constraints_validation_good`, `L07_packs_validation_good`, `L08_type_lists_validation_good`, `L09_tuple_validation_good`, `L10_constexpr_validation_good`, `L11_customization_validation_good`, `P1_static_record_validation_good`, `U01_mp11_validation_good`。

## good negative control

负控是 synthetic，不是课程实现中的真实 Reference 泄漏。

- synthetic trace：`05-synthetic-reference-include-trace.json`
  - 来源：复制本次真实 good trace `03-build-good-showincludes.json`
  - 变更：只追加一行 `Note: including file: F:/CPPTrain/LearnCPP/C04_Generic_CompileTime_Reflection/exercises/L10_constexpr/src/reference/constexpr_tools.hpp`
  - 文件内有 `synthetic_control` 字段说明其合成性质
- 首次负控审计：`06-audit-good-negative-control.json`
  - `verdict`: FAIL
  - failure: `Reference in actual include trace: .../src/reference/constexpr_tools.hpp`
- 首次外层记录：`07-audit-good-negative-control-run.json`
  - superseded
  - 原因：`audit_good.py` 返回 exit 1 且写出 FAIL JSON，但不把 failure 详情打印到 stdout；外层 record_process 的 `--contains "Reference in actual include trace"` 因此判 FAIL
- 补充负控审计：`08-audit-good-negative-control-r2.json`
  - `verdict`: FAIL
  - failure: `Reference in actual include trace: .../src/reference/constexpr_tools.hpp`
- 补充外层记录：`09-audit-good-negative-control-run.json`
  - record `verdict`: PASS
  - 子进程 `status`: FAIL
  - `exit_code`: 1
  - `expected_exit`: 1

## Gaps and risks

- 未重跑已冻结的整课 Debug/Release 102、meta 108、ASan 35、frontier 102 PASS/15 SKIP，未重跑 C05 fmt/std 两 backend Debug/Release 38。
- 本审计不做 plagiarism proof；只证明当前 codemodel 构建输入和实际 include trace 没有 Reference 依赖。
- `07-audit-good-negative-control-run.json` 是保留的失败外层记录，已由 `09-audit-good-negative-control-run.json` 取代；它不表示课程源码或 `audit_good.py` 失败。
