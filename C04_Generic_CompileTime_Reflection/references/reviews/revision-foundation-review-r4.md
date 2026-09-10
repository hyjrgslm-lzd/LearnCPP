# C04 revision foundation review r4

日期：2026-09-10

结论：REQUEST CHANGES。作者在 r3 后新增的 `diagnostic_messages()` 过滤能避免 include trace 和源文件路径误触发语义 pattern，新增控制脚本也通过；但上一轮发现的基础设施错误漏判仍未修复。实际 MSVC `error C1083` 缺头输出仍被 `compile_case.py` 接受为 PASS。

## Rechecked files

- `C04_Generic_CompileTime_Reflection/exercises/tools/compile_case.py`
- `C04_Generic_CompileTime_Reflection/exercises/tools/check_diagnostic_messages.py`
- `C04_Generic_CompileTime_Reflection/exercises/CMakeLists.txt`
- `C04_Generic_CompileTime_Reflection/exercises/L10_constexpr/validation/good/constexpr_tools.hpp`
- `C04_Generic_CompileTime_Reflection/exercises/cmake/StudySetup.cmake`

## Verified good changes

File: `C04_Generic_CompileTime_Reflection/exercises/tools/compile_case.py:18`

`diagnostic_messages()` now extracts only real compiler error messages for semantic pattern matching. This closes the “pattern accidentally matches include trace or source path” class.

Validation:

```powershell
python -B -m py_compile C04_Generic_CompileTime_Reflection/exercises/tools/compile_case.py C04_Generic_CompileTime_Reflection/exercises/tools/check_diagnostic_messages.py
python -B C04_Generic_CompileTime_Reflection/exercises/tools/check_diagnostic_messages.py
ctest --test-dir C04_Generic_CompileTime_Reflection/exercises/build/c04-l10-v4-final -C Debug -R C04_diagnostic_message_controls --output-on-failure
```

Results: all PASS. CTest `C04_diagnostic_message_controls` passed in 0.09s.

No regression in the previously closed paths:

```powershell
ctest --test-dir build/c04-l10-v4 -C Debug -R L10_decimal_overflow_diagnostic --output-on-failure
ctest --test-dir C04_Generic_CompileTime_Reflection/references/validation/revision-20260910/foundation-review-a02-setup -C Release -R A02_diag_unknown --output-on-failure
```

Results: L10 overflow PASS in 3.45s; A02 SETUP diagnostic PASS in 4.18s.

## Blocking issue

[MEDIUM] `compile_case.py` still accepts actual MSVC `error C1083` infrastructure failure

File: `C04_Generic_CompileTime_Reflection/exercises/tools/compile_case.py:76`

Issue: infrastructure classification still uses:

```python
re.search(r'(?:fatal error C(?:1001|1060|1076|1083)|error (?:MSB80\d+|LNK\d+)|internal compiler error|内部编译器错误)', text, re.I)
```

This catches `fatal error C1083`, but this environment emits missing include as `error C1083`. The new `diagnostic_messages()` then extracts that same line as a semantic diagnostic, so a pattern matching the C1083 text still passes.

Reproduction:

```powershell
python -B C04_Generic_CompileTime_Reflection/exercises/tools/compile_case.py `
  --source C04_Generic_CompileTime_Reflection/references/validation/revision-20260910/r3-infra-reject/subject_missing_header.cpp `
  --control C04_Generic_CompileTime_Reflection/references/validation/revision-20260910/r3-infra-reject/control.cpp `
  --pattern "cannot open include file|C1083|无法打开包括文件" `
  --build C04_Generic_CompileTime_Reflection/references/validation/revision-20260910/r3-infra-reject/build `
  --generator "Visual Studio 18 2026" --platform x64 --config Debug `
  --compiler D:/VisualStudio2026/Installed/VC/Tools/MSVC/14.51.36231/bin/Hostx64/x64/cl.exe
```

Observed: exit code `0`, helper output `PASS: expected semantic diagnostic with a normal nonzero compiler exit`, evidence `C04_Generic_CompileTime_Reflection/references/validation/revision-20260910/r3-infra-reject/build/evidence-2.json`.

The evidence has:

- `verdict=PASS`
- `semantic_diagnostics="error C1083: 无法打开包括文件: “definitely_missing_r3_review_header.hpp”: No such file or directory"`
- `subject.exit_code=1`
- `subject.timeout=false`
- `changed_inputs=[]`
- control command contains `--clean-first`

Fix: extend only the infrastructure classifier, not the semantic message extractor. Add an explicit `error C1083` infrastructure branch, for example `(?:fatal error|error) C1083`, while keeping normal semantic MSVC diagnostics such as `error C7510` and `error C2039` matchable through `diagnostic_messages()`. Add a controlled helper test where subject has a missing include and the pattern matches `C1083`; expected result must be FAIL/nonzero.

## Validation limits

No `lsp_diagnostics` or `ast_grep_search` tool is available in this child review environment. I used source inspection, `git diff`, `py_compile`, targeted CTest, and controlled helper execution. `audit_good.py` include-trace branch remains outside this r4 slice per parent instruction.

## Recommendation

REQUEST CHANGES until `compile_case.py:76` rejects actual `error C1083` infrastructure failures. The latest `diagnostic_messages()` work is useful but does not close the infrastructure failure gate.
