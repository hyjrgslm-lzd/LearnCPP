# C04/C05 revision dependencies review r2

日期：2026-09-10

结论：APPROVE for this narrow r2 dependency slice。上一轮唯一阻断 `spdlog::spdlog` 外部 target 污染已关闭；原 mock 污染工程现在配置失败，错误明确来自 pinned SOURCE_DIR 校验。fmt backend 与 std backend 的 U03_spdlog 正常消费仍可配置并构建。

## Rechecked files

- `C05_Data_Representation_Standard_Facilities/exercises/cmake/FormatLibraries.cmake`
- `C05_Data_Representation_Standard_Facilities/references/validation/revision-20260910/dependency-review-spdlog-pollution`

## Closed issue

[CLOSED] External `spdlog::spdlog` target pollution is now rejected

File: `C05_Data_Representation_Standard_Facilities/exercises/cmake/FormatLibraries.cmake:90`

`c05_ensure_spdlog()` no longer returns before validation. Current flow validates `DATA_STUDY_SPDLOG_BACKEND`, checks selected backend consistency, loads/verifies pinned spdlog dependency metadata, and then calls `c05_assert_pinned_static_target(spdlog::spdlog spdlog "${C05_SPDLOG_SOURCE_DIR}")` before accepting an existing target.

Validation command:

```powershell
python C02_Objects_Lifetime_Ownership/exercises/tools/record_process.py `
  --output C05_Data_Representation_Standard_Facilities/references/validation/revision-20260910/dependency-review-spdlog-pollution/configure-r2.json `
  --timeout 90 -- cmake `
  -S C05_Data_Representation_Standard_Facilities/references/validation/revision-20260910/dependency-review-spdlog-pollution `
  -B C05_Data_Representation_Standard_Facilities/references/validation/revision-20260910/dependency-review-spdlog-pollution/build-r2 `
  -G "Visual Studio 18 2026" -A x64
```

Observed: exit code `1`, expected rejection. Evidence:

- `C05_Data_Representation_Standard_Facilities/references/validation/revision-20260910/dependency-review-spdlog-pollution/configure-r2.json`
- stderr contains `spdlog::spdlog does not use pinned spdlog source`

This is the correct result for the pollution negative control.

## Normal consumption smoke checks

fmt backend:

```powershell
cmake -S C05_Data_Representation_Standard_Facilities/exercises/U03_spdlog `
  -B C05_Data_Representation_Standard_Facilities/references/validation/revision-20260910/dependency-review-normal-fmt-build `
  -G "Visual Studio 18 2026" -A x64 `
  -DDATA_STUDY_ENABLE_FORMAT_LIBS=ON -DDATA_STUDY_SPDLOG_BACKEND=fmt
cmake --build C05_Data_Representation_Standard_Facilities/references/validation/revision-20260910/dependency-review-normal-fmt-build `
  --config Release --target U03_spdlog_DEBUG
```

Evidence:

- `C04_Generic_CompileTime_Reflection/references/validation/revision-20260910/dependencies-review-r2-c05-format-fmt-configure.json`: PASS
- `C04_Generic_CompileTime_Reflection/references/validation/revision-20260910/dependencies-review-r2-c05-format-fmt-build-u03-debug.json`: PASS

std backend:

```powershell
cmake -S C05_Data_Representation_Standard_Facilities/exercises/U03_spdlog `
  -B C05_Data_Representation_Standard_Facilities/references/validation/revision-20260910/dependency-review-normal-std-build `
  -G "Visual Studio 18 2026" -A x64 `
  -DDATA_STUDY_ENABLE_FORMAT_LIBS=ON -DDATA_STUDY_SPDLOG_BACKEND=std
cmake --build C05_Data_Representation_Standard_Facilities/references/validation/revision-20260910/dependency-review-normal-std-build `
  --config Release --target U03_spdlog_DEBUG
```

Evidence:

- `C04_Generic_CompileTime_Reflection/references/validation/revision-20260910/dependencies-review-r2-c05-format-std-configure.json`: PASS
- `C04_Generic_CompileTime_Reflection/references/validation/revision-20260910/dependencies-review-r2-c05-format-std-build-u03-debug.json`: PASS

Note: two discarded evidence files exist from an initial wrong target-name attempt:

- `dependencies-review-r2-c05-format-fmt-build.json`
- `dependencies-review-r2-c05-format-std-build.json`

They fail with MSBuild `MSB1009` because `U03_spdlog_reference` is not a real target. They are not dependency failures and are not used as passing evidence.

## Validation limits

No `lsp_diagnostics` or `ast_grep_search` tool is available in this child review environment. This r2 pass only reran the pollution negative control and the narrow fmt/std normal-consumption checks requested by parent.

## Recommendation

APPROVE for this narrow dependency r2 slice. The previous spdlog pollution blocker is closed.
