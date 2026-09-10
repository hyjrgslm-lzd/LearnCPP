# Foundation review validation summary

日期：2026-09-10

- `foundation-review-f01`: `cmake -S .../F01_frontier -B .../foundation-review-f01 -G "Visual Studio 18 2026" -A x64 -DGENERIC_STUDY_ENABLE_FRONTIER=ON` PASS; build PASS; `ctest -C Release` PASS with 14 SKIP / 0 FAIL.
- `foundation-review-f01-control`: same configure with `-DGENERIC_STUDY_FRONTIER_ENABLE_FAILURE_CONTROL=ON` PASS; build PASS; `ctest -C Release` FAIL on `F01_declared_failure_control`, proving body failure is not reported as SKIP.
- `foundation-review-a02-setup`: configure/build PASS, but CTest FAILs 5 diagnostics through MSBuild `FTK1011` in nested long try-compile paths. Shorter rebuild proves this is path sensitivity.
- `C04_Generic_CompileTime_Reflection/build/foundation-review-a02`: configure/build PASS; full CTest PASS 12/12; diagnostic subset PASS 7/7.
- `C04_Generic_CompileTime_Reflection/build/foundation-review-l10`: configure PASS; `cmake --build --clean-first` PASS; CTest FAIL on `L10_decimal_overflow_diagnostic` after subject timeout. Evidence JSON: `build/foundation-review-l10/diagnostics/L10_decimal_overflow_diagnostic/evidence-1.json`.
- `python -B -m py_compile C04_Generic_CompileTime_Reflection/exercises/tools/compile_case.py C04_Generic_CompileTime_Reflection/exercises/tools/audit_good.py`: PASS.

审查结论：REQUEST CHANGES。当前阻断是 L10 溢出诊断不能正常失败；另有 chapter14 旧施工叙述和 Windows 长路径验证风险。
