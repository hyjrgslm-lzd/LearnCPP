# C04 revision foundation review r3

日期：2026-09-10

结论：REQUEST CHANGES。r2 的 L10 overflow 阻断已关闭，短路径 SETUP 与 include SHA 绑定也已在当前 evidence 中复验；但 `compile_case.py` 的基础设施错误拒绝边界仍有漏判：本机 MSVC 实际缺头输出为 `error C1083`，当前正则只覆盖 `fatal error C1083`，导致缺头失败可被判为 PASS。

## Rechecked files

- `C04_Generic_CompileTime_Reflection/exercises/L10_constexpr/validation/good/constexpr_tools.hpp`
- `C04_Generic_CompileTime_Reflection/exercises/tools/compile_case.py`
- `C04_Generic_CompileTime_Reflection/exercises/cmake/StudySetup.cmake`
- `C04_Generic_CompileTime_Reflection/references/validation/revision-20260910/blind-core/post-review/repair-v4.json`

## Closed r2 blocker: L10 overflow diagnostic no longer times out

File: `C04_Generic_CompileTime_Reflection/exercises/L10_constexpr/validation/good/constexpr_tools.hpp:24`

Current good implementation uses a `decimal_result` value object and a bounded `consteval` loop, then checks `parsed.digits` and `parsed.fits` with `static_assert`. It no longer recurses through an overflow-prone NTTP accumulator.

Evidence:

- Current good SHA: `7D5A1E8CDB4FCD61B2231DA6BB18B050B844C6F18D6A6110A8569888D5016333`.
- `C04_Generic_CompileTime_Reflection/references/validation/revision-20260910/blind-core/post-review/repair-v4.json` records the same SHA and marks the repair as post-blind v4.
- Existing root evidence `root-l10-v4-overflow.json`: `L10_decimal_overflow_diagnostic` PASS in 6.38s.
- Independent r3 rerun:

```powershell
ctest --test-dir build/c04-l10-v4 -C Debug -R L10_decimal_overflow_diagnostic --output-on-failure
```

Result: PASS in 3.23s. Latest diagnostic evidence `C04_Generic_CompileTime_Reflection/exercises/build/_diag/f1464f52547c/Debug/evidence-3.json` has `verdict=PASS`, `changed_inputs=[]`, `subject.timeout=false`, `subject.exit_code=1`, and `local_include_sha256` includes `validation/good/constexpr_tools.hpp`.

## Closed: SETUP short path and include evidence binding

File: `C04_Generic_CompileTime_Reflection/exercises/cmake/StudySetup.cmake:61`

`c04_add_compile_case` now hashes `${CMAKE_BINARY_DIR}|${ARG_NAME}` and places diagnostic builds under `C04_Generic_CompileTime_Reflection/exercises/build/_diag/<hash>/$<CONFIG>`. It passes the platform as one argv item: `"--platform=${CMAKE_GENERATOR_PLATFORM}"`, so empty platform no longer shifts parser arguments.

File: `C04_Generic_CompileTime_Reflection/exercises/tools/compile_case.py:39`

`compile_case.py` now records initial direct input SHA values, rejects if those direct inputs change before evidence write, forces the positive control through `--clean-first`, and records repo-local include hashes from `/showIncludes` output.

Evidence:

- `python -B -m py_compile C04_Generic_CompileTime_Reflection/exercises/tools/compile_case.py`: PASS.
- `ctest --test-dir C04_Generic_CompileTime_Reflection/references/validation/revision-20260910/foundation-review-a02-setup -C Release -R A02_diag_unknown --output-on-failure`: PASS in 4.01s.
- `C04_Generic_CompileTime_Reflection/exercises/build/_diag/6e120c26e973/Release/evidence-2.json` has `verdict=PASS`, `changed_inputs=[]`, control command contains `--clean-first`, and `local_include_sha256` includes:
  - `A02_field_projection/validation/good/field_projection.hpp`
  - `A02_field_projection/checks/projection_schema.hpp`
  - `C01_Build_Compile_Link/exercises/include/check.hpp`

## Issue

[MEDIUM] `compile_case.py` accepts actual MSVC `error C1083` as a semantic diagnostic

File: `C04_Generic_CompileTime_Reflection/exercises/tools/compile_case.py:65`

Issue: the infrastructure filter contains `fatal error C(?:1001|1060|1076|1083)`, but the current MSVC 19.51 output for a missing include in this environment is:

```text
error C1083: 无法打开包括文件: “definitely_missing_r3_review_header.hpp”: No such file or directory
```

Because `error C1083` does not match the infrastructure regex, `compile_case.py` accepted the failed subject build as PASS when the expected pattern matched the C1083 text. This violates the helper contract: infrastructure failures must not be counted as semantic negative-example success.

Reproduction artifact:

```powershell
python -B C04_Generic_CompileTime_Reflection/exercises/tools/compile_case.py `
  --source C04_Generic_CompileTime_Reflection/references/validation/revision-20260910/r3-infra-reject/subject_missing_header.cpp `
  --control C04_Generic_CompileTime_Reflection/references/validation/revision-20260910/r3-infra-reject/control.cpp `
  --pattern "cannot open include file|C1083|无法打开包括文件" `
  --build C04_Generic_CompileTime_Reflection/references/validation/revision-20260910/r3-infra-reject/build `
  --generator "Visual Studio 18 2026" --platform x64 --config Debug `
  --compiler D:/VisualStudio2026/Installed/VC/Tools/MSVC/14.51.36231/bin/Hostx64/x64/cl.exe
```

Observed: command exits `0`, prints `PASS: expected semantic diagnostic with a normal nonzero compiler exit`, and writes `C04_Generic_CompileTime_Reflection/references/validation/revision-20260910/r3-infra-reject/build/evidence-1.json` with `subject_has_error_C1083=true`.

Fix: update the infrastructure classifier to reject actual MSVC formats, for example by matching `(?:fatal error|error) C(?:1001|1060|1076|1083)` or a narrower explicit `error C1083` branch. Add a small controlled regression for the helper boundary so a missing include cannot be accepted as a semantic diagnostic when the regex pattern happens to match the compiler infrastructure text.

## Validation limits

No `lsp_diagnostics` or `ast_grep_search` tool is available in this child review environment. I used source inspection, `git diff`, `py_compile`, targeted CTest runs, regex probes, and controlled helper execution instead. The `audit_good.py` actual branch remains intentionally outside this r3 slice per parent instruction.

## Recommendation

REQUEST CHANGES for `compile_case.py:65`. L10 overflow, SETUP short path, control clean-first, direct input `changed_inputs`, and repo-local include SHA evidence are verified for this r3 slice.
