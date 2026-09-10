# C04 revision final Student/good audit r2

日期：2026-09-10

## Verdict

PASS。

r2 审计覆盖 U01 empty schema 修复后的最小变更、U01 叶子 Debug/Release/Student 复验、最终 Student include/CTest 分类、最终 good include 审计和 good 审计器负控。未重跑整课 102/35/frontier/C05 矩阵，未重跑 benchmark。

## U01 empty schema fix

真实问题由 `empty-map-probe-before.json` 证明：空 schema 下普通 runtime `if` 之后仍会实例化 `mp_with_index<0>`，clang-cl 编译失败，错误落在 `mp_at_c<map, I::value>` 和 `mp_with_index<0>`。

源码审查结论：

- `U01_mp11/src/reference/mp11_schema_tools.hpp`：`visit_field_type` 先做越界检查，再用 `if constexpr (count > 0)` 包住 `mp_with_index<count>`。
- `U01_mp11/validation/good/mp11_schema_tools.hpp`：同样隔离空 schema 分派。
- `U01_mp11/validation/bad/mp11_schema_tools.hpp`：空 schema 分支安全抛 `std::out_of_range`；非空越界仍折回 0，保留负例焦点。
- `U01_mp11/checks/mp11_schema_checks.cpp`：新增 `EmptyRecord` schema，检查 `field_count_v == 0`、缺键为 `void`、空 schema dispatch 抛出且 visitor 0 次调用。
- `U01_mp11/README.md` 与 `chapters/23-mp11.md` 已说明空 schema 需要类型层 `if constexpr` 边界。

`empty-map-meta-build-r2.json` 已记录 root 元配置 Release 下 `U01_mp11_reference`、`U01_mp11_validation_good`、`U01_mp11_validation_bad` 三个目标 build PASS。

## U01 leaf revalidation

证据目录：`references/validation/revision-20260910/final-audits-u01-r2/`

- `01-u01-configure-default.json`：PASS
- `02-u01-build-debug.json`：PASS
- `03-u01-ctest-debug.json`：PASS，4/4：`U01_mp11_reference`、`U01_mp11_validation_good`、`U01_mp11_validation_bad_rejected`、`U01_mp11_missing_key_diagnostic`
- `04-u01-build-release.json`：PASS
- `05-u01-ctest-release.json`：PASS，4/4 同上
- `06-u01-configure-student.json`：PASS
- `07-u01-build-student-debug.json`：PASS
- `08-u01-ctest-student-debug-expected-fail.json`：record PASS，子进程 exit 8，命中 `schema_map_t<Person> must expose...`
- `09-u01-build-student-release.json`：PASS
- `10-u01-ctest-student-release-expected-fail.json`：record PASS，子进程 exit 8，命中同一 Student 占位检查

## r2 source binding

- Student r2 快照：`final-audits-student/07-code-before-r2.json`
- good r2 快照：`final-audits-good/10-code-before-r2.json`
- 两者均为 `count: 246`，`tree_sha256: d8ce18266a9ec4777fae9ecff3e34cc9574ad37c18d67895e2aaa68289befdc5`
- 当前回读：missing 0，mismatch 0

相对 r1 快照，变化文件恰为 5 个：

- `U01_mp11/checks/mp11_schema_checks.cpp`
- `U01_mp11/README.md`
- `U01_mp11/src/reference/mp11_schema_tools.hpp`
- `U01_mp11/validation/good/mp11_schema_tools.hpp`
- `U01_mp11/validation/bad/mp11_schema_tools.hpp`

`U01_mp11/src/student/mp11_schema_tools.hpp` SHA 未变；Student 初态仍是占位实现。

## Student final r2 audit

证据目录：`references/validation/revision-20260910/final-audits-student/`

- `08-configure-meta-student-r2.json`：PASS
- `09-student-targets-r2.json`：18 个 `*_student`
- `10-build-students-showincludes-r2.json`：PASS，显式 `--clean-first --target` 重建 18 个 Student 目标
- `11-audit-student-r2.json`：PASS，`actual_include_count: 2723`，Reference target/path/include 违规 0
- `12-build-student-all-r2.json`：PASS
- `13-ctest-meta-student-expected-fail-r2.json`：record PASS，子进程 `status: FAIL`，`exit_code: 8`，`expected_exit: 8`
- `13-ctest-meta-student-r2.junit.xml`：71 testcase，18 fail，53 pass

18 个失败目标仍恰为全部 Student 目标，且每个失败都有具体 `check failed:`：

- `L01_templates_student`：`quantity_value_t reads dependent value_type`
- `L02_deduction_student`：`by-value keeps plain int`
- `L03_forwarding_student`：`invoke_preserving must keep lvalue argument category`
- `L04_lookup_student`：`no-route object must not satisfy c04_inspectable`
- `L05_overload_student`：`member describe returns member_result`
- `L06_constraints_student`：`missing name is not field_like`
- `L07_packs_student`：`empty all fold identity must be true`
- `L08_type_lists_student`：`map_t applies a unary template to each element`
- `L09_tuple_student`：`tuple traversal must preserve left-to-right order`
- `L10_constexpr_student`：`decimal_value parses digits at compile time`
- `L11_customization_student`：`no-path object must not satisfy c04_readable`
- `P1_static_record_student`：`encoded fields cover the complete schema in declaration order`
- `A01_compiletime_values_student`：`sort orders literal keys`
- `A02_field_projection_student`：`project returns writable references in requested order`
- `A03_explicit_object_student`：`value preserves object identity and cvref category`
- `A04_type_pipelines_student`：`flatten recursively expands nested type_list elements`
- `A05_expression_templates_student`：`empty expression evaluation performs no element access`
- `U01_mp11_student`：`schema_map_t<Person> must expose the Person fields before the full Mp11 contract can run`

## good final r2 audit

证据目录：`references/validation/revision-20260910/final-audits-good/`

- `11-configure-meta-libs-r2.json`：PASS
- `12-good-targets-r2.json`：18 个 `*_validation_good`
- `13-build-good-showincludes-r2.json`：PASS，显式 `--clean-first --target` 重建 18 个 good 目标
- `14-audit-good-r2.json`：PASS，`actual_include_count: 2742`，`failures: []`

负控：

- `15-synthetic-reference-include-trace-r2.json`：基于真实 `13-build-good-showincludes-r2.json`，只追加一行 synthetic Reference include；不是课程实现中的真实泄漏。
- `16-audit-good-negative-control-r2-current.json`：FAIL，命中 `Reference in actual include trace: .../U01_mp11/src/reference/mp11_schema_tools.hpp`
- `17-audit-good-negative-control-run-r2.json`：record PASS，子进程 exit 1 符合预期

## Gaps and risks

- 本报告不重跑已冻结的整课 Debug/Release 102、meta 108、ASan 35、frontier 102 PASS/15 SKIP，也不重跑 C05 fmt/std 矩阵。
- 本报告不重跑正式 benchmark；U01 修复未触碰 benchmark driver 或依赖。
- Student/good include 审计证明当前 codemodel 输入和实际 include trace 无 Reference 依赖；不证明作者独立性或 plagiarism。

停止条件满足：U01 修复已独立复验，最终 Student/good r2 源码绑定和 include trace 已更新，18 Student 最终分类已确认。
