# Foundation review r5 validation summary

日期：2026-09-10

- `compile_case.py` now has `is_infrastructure_failure()` covering optional `fatal` prefix for `C1001` / `C1060` / `C1076` / `C1083`, plus MSBuild/linker and Clang ICE markers.
- `check_diagnostic_messages.py` now asserts C1083 with and without `fatal`, C1001 rejection, and C7510/C2039 non-rejection.
- `python -B -m py_compile C04_Generic_CompileTime_Reflection/exercises/tools/compile_case.py C04_Generic_CompileTime_Reflection/exercises/tools/check_diagnostic_messages.py`: PASS.
- `python -B C04_Generic_CompileTime_Reflection/exercises/tools/check_diagnostic_messages.py`: PASS.
- `ctest --test-dir C04_Generic_CompileTime_Reflection/exercises/build/c04-l10-v4-final -C Debug -R C04_diagnostic_message_controls --output-on-failure`: PASS in 0.07s.
- Original controlled missing-include case rerun after the fix: `compile_case.py` returned exit code `1` and wrote `verdict=FAIL` to `C04_Generic_CompileTime_Reflection/references/validation/revision-20260910/r3-infra-reject/build/evidence-3.json`.

审查结论：APPROVE for this narrow r5 slice。C1083 基础设施拒绝阻断已关闭；该 FAIL 是 helper 正确拒绝基础设施失败，不是课程失败。
