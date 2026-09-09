# C03 L07 author-a r2 evidence

## Scope

- 修复审查阻断：`checks/interface_checks.cpp` 现在对当前选中 `l07::Owner` 用 `requires` 检查 rvalue `.view()` 不可用。
- 新增 `validation/bad_rvalue/owner.hpp`：`snapshot()` 与 `replace_all()` 正确，只保留未禁用 rvalue `.view()` 的接口缺陷。
- `CMakeLists.txt` 新增 `L07_interfaces_validation_bad_rvalue_rejected`，复用同一 checker，以诊断 `rvalue view is rejected for selected implementation` 拒绝该独立负例。
- 保留 `L07_interfaces_rvalue_view_rejected` 作为 Reference 编译器诊断对照；它不再被表述为 Student/good 完成证明。

## Fresh Debug evidence

- `r2-debug-configure-core.txt`：exit 0。
- `r2-debug-build-core.txt`：exit 0。
- `r2-debug-ctest-core.txt`：5/5 passed。包含 Reference、validation_good、原 bad、Reference rvalue 编译负例、新 bad_rvalue。
- `r2-debug-configure-student.txt`：exit 0。
- `r2-debug-build-student.txt`：exit 0。
- `r2-debug-ctest-student.txt`：6 项中仅 `L07_interfaces_student` 失败，诊断为 `check failed: snapshot remains independent after owner mutation`；其他 5 项通过。
- `r2-debug-run-bad-rvalue.txt`：直接运行新负例 exit 1，诊断为 `check failed: rvalue view is rejected for selected implementation`。
- `r2-scope-diff-check-after-readme.txt`：exit 0。
- `r2-validation-include-scan-after-readme.txt`：exit 1，无命中；validation/checks 没有 include `src/reference` 或 `src/student`。

## Stop condition

L07 r2 已冻结，等待原非作者 critic 复验。未运行 P1，未触碰 L01/L02/L06。

