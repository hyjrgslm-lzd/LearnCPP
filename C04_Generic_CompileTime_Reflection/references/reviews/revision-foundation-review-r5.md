# C04 revision foundation review r5

日期：2026-09-10

结论：APPROVE for this narrow r5 slice。上一轮 `error C1083` 基础设施错误漏判已关闭：`compile_case.py` 现在通过 `is_infrastructure_failure()` 显式覆盖带或不带 `fatal` 前缀的 `C1001`、`C1060`、`C1076`、`C1083`，并保留 Clang ICE 类标识。原受控缺头案例现在返回非 0，evidence 为 `FAIL`；这是 helper 正确拒绝基础设施失败，不是课程失败。

## Rechecked files

- `C04_Generic_CompileTime_Reflection/exercises/tools/compile_case.py`
- `C04_Generic_CompileTime_Reflection/exercises/tools/check_diagnostic_messages.py`

## Verified fix

File: `C04_Generic_CompileTime_Reflection/exercises/tools/compile_case.py:28`

`is_infrastructure_failure()` now matches:

- `error C1083`
- `fatal error C1083`
- `error C1001`
- `C1060` / `C1076`
- `MSB80xx` / `LNKxxxx`
- Clang-style ICE markers such as `PLEASE submit a bug report` and `frontend command failed due to signal`

File: `C04_Generic_CompileTime_Reflection/exercises/tools/check_diagnostic_messages.py:9`

The control script now asserts both C1083 variants and C1001 are infrastructure failures, while `error C7510` and `error C2039` remain semantic diagnostics and are not rejected as infrastructure.

## Validation

Syntax and helper controls:

```powershell
python -B -m py_compile C04_Generic_CompileTime_Reflection/exercises/tools/compile_case.py C04_Generic_CompileTime_Reflection/exercises/tools/check_diagnostic_messages.py
python -B C04_Generic_CompileTime_Reflection/exercises/tools/check_diagnostic_messages.py
ctest --test-dir C04_Generic_CompileTime_Reflection/exercises/build/c04-l10-v4-final -C Debug -R C04_diagnostic_message_controls --output-on-failure
```

Results:

- `py_compile`: PASS
- `check_diagnostic_messages.py`: PASS, output `diagnostic message controls passed`
- `C04_diagnostic_message_controls`: PASS in 0.07s

Original controlled missing-include case:

```powershell
python -B C04_Generic_CompileTime_Reflection/exercises/tools/compile_case.py `
  --source C04_Generic_CompileTime_Reflection/references/validation/revision-20260910/r3-infra-reject/subject_missing_header.cpp `
  --control C04_Generic_CompileTime_Reflection/references/validation/revision-20260910/r3-infra-reject/control.cpp `
  --pattern "cannot open include file|C1083|无法打开包括文件" `
  --build C04_Generic_CompileTime_Reflection/references/validation/revision-20260910/r3-infra-reject/build `
  --generator "Visual Studio 18 2026" --platform x64 --config Debug `
  --compiler D:/VisualStudio2026/Installed/VC/Tools/MSVC/14.51.36231/bin/Hostx64/x64/cl.exe
```

Result: exit code `1`, helper output `FAIL: expected semantic diagnostic with a normal nonzero compiler exit`.

Evidence: `C04_Generic_CompileTime_Reflection/references/validation/revision-20260910/r3-infra-reject/build/evidence-3.json`

Key fields:

- `verdict=FAIL`
- `semantic_diagnostics="error C1083: 无法打开包括文件: “definitely_missing_r3_review_header.hpp”: No such file or directory"`
- `changed_inputs=[]`
- control command contains `--clean-first`
- `subject.exit_code=1`
- `subject.timeout=false`

This is the desired result: C1083 is present in semantic diagnostics for transparency, but infrastructure classification blocks acceptance.

## Validation limits

No `lsp_diagnostics` or `ast_grep_search` tool is available in this child review environment. This r5 pass intentionally did not rescan unrelated C04 implementation files or rerun the full suite, per parent instruction.

## Recommendation

APPROVE for the final `compile_case.py` C1083 infrastructure-boundary fix and its narrow controls. Previous L10 overflow and SETUP short-path findings remain closed from r3/r4 evidence.
