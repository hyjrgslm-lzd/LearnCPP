# C04 A01 boundary independent review r2

日期：2026-09-10

结论：APPROVE for A01 boundary closure。上一轮 `revision-values-review.md` 的 MEDIUM 问题已关闭；本轮未重审 A03 未改部分。

## Scope

- `chapters/18-compiletime-values.md`
- `exercises/A01_compiletime_values/README.md`
- `exercises/A01_compiletime_values/CMakeLists.txt`
- `exercises/A01_compiletime_values/checks/compiletime_values_checks.cpp`
- `exercises/A01_compiletime_values/src/student/compiletime_values.hpp`
- `exercises/A01_compiletime_values/src/reference/compiletime_values.hpp`
- `exercises/A01_compiletime_values/validation/good/compiletime_values.hpp`
- `exercises/A01_compiletime_values/validation/bad/compiletime_values.hpp`
- `exercises/A01_compiletime_values/validation/diagnostics/*`
- Author evidence: `references/validation/revision-20260910/values-a01-boundary-*`
- Independent r2 evidence: `references/validation/revision-20260910/values-blind/18-*` through `29-*`

## Result

- CRITICAL: 0
- HIGH: 0
- MEDIUM: 0
- LOW: 0

## Closed Finding

Previous finding: A01 key boundary was specified but not implemented or tested.

Status: CLOSED.

Evidence:

- `chapters/18-compiletime-values.md:33` now uses `static_assert(N <= row::key_width)`.
- `chapters/18-compiletime-values.md:47` explains `key_width == 16` as 15 ASCII payload bytes plus terminating NUL, allows empty key, and rejects embedded NUL/non-ASCII payload.
- `exercises/A01_compiletime_values/README.md:3` and `README.md:8` now state the same contract.
- `exercises/A01_compiletime_values/src/reference/compiletime_values.hpp:17` and `validation/good/compiletime_values.hpp:17` enforce `N <= row::key_width`.
- `exercises/A01_compiletime_values/src/reference/compiletime_values.hpp:21`, `validation/good/compiletime_values.hpp:21`, `validation/bad/compiletime_values.hpp:21`, and `src/student/compiletime_values.hpp:21` reject payload `NUL` and bytes above `0x7F`.
- `exercises/A01_compiletime_values/checks/compiletime_values_checks.cpp:20` through `48` cover empty key, 15-character key, and `std::string_view{"", 1}` not matching empty key.
- `exercises/A01_compiletime_values/CMakeLists.txt:20` through `42` register negative compile cases for too-long key, non-ASCII key, and embedded NUL key.
- `validation/diagnostics/key_too_long.cpp:5`, `key_non_ascii.cpp:5`, and `key_embedded_nul.cpp:5` exercise the three rejected inputs.
- Static scan found no remaining `key_width + 1` in the reviewed A01 docs/source scope.

## Independent Validation

All commands below used C02 `record_process.py`.

| Record | Result | Meaning |
| --- | --- | --- |
| `18-old-boundary-probe-rejected-r2.json` | PASS, child exit 1 | The previous reproducer using a 16-character key now fails to compile with `literal key is too long`. |
| `19-a01-r2-positive-configure.json` | PASS | Independent positive boundary probe configured. |
| `20-a01-r2-positive-build.json` | PASS | Independent positive boundary probe built. |
| `21-a01-r2-positive-run.json` | PASS | Empty key, 15-character key, and embedded-NUL lookup behavior pass. |
| `22-a01-r2-full-configure.json` | PASS | Fresh A01 full configure in `build/vb-a01-r2-full`. |
| `23-a01-r2-full-build-debug.json` | PASS | Debug Reference/good/bad build passed. |
| `24-a01-r2-full-ctest-debug.json` | PASS | Debug CTest reports 7/7 passed. |
| `25-a01-r2-full-build-release.json` | PASS | Release Reference/good/bad build passed. |
| `26-a01-r2-full-ctest-release.json` | PASS | Release CTest reports 7/7 passed. |
| `27-a01-r2-student-configure.json` | PASS | Fresh Student configure in `build/vb-a01-r2-student`. |
| `28-a01-r2-student-build-debug.json` | PASS | Student Debug build passed. |
| `29-a01-r2-student-ctest-debug-expected-fail.json` | PASS, child exit 8 | Student still fails on intended checker message while key diagnostics pass. |

Author records `values-a01-boundary-ctest-debug-vs.json` and `values-a01-boundary-ctest-release-vs.json` also show 7/7 passed; `values-a01-boundary-student-ctest-debug-expected-fail.json` shows the Student expected-fail path remained intact.

## Stage Checks

Stage 1 spec compliance: PASS for the A01 boundary closure. The stated 15-character non-NUL ASCII payload contract, empty key allowance, embedded NUL rejection, and lookup distinction are now covered in docs, implementation, checks, diagnostics, and independent r2 evidence.

Root-cause fallback guard: PASS. The repair tightened the primary `make_row`/comparison contract and added diagnostics; it did not mask the previous failure with a fallback branch.

Stage 2 code quality/security: PASS for this slice. No hardcoded secret pattern, broad catch, TODO/FIXME, or Reference include leak was found in the reviewed A01 source/validation scope. `lsp_diagnostics` and `ast_grep_search` were still not callable in this environment; compiler/CTest and `rg`/`Select-String` static scans were used instead.

## Recommendation

APPROVE for the A01 boundary r2 slice.
