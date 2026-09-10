# C04 revision foundation review

日期：2026-09-10

结论：REQUEST CHANGES。F01 前沿能力分层本身通过本机复验；`compile_case.py` 的 SETUP 通道在短路径下能正确驱动 A02 正/负控制；但 L10 当前 `validation/good` 的溢出负例无法在 180 秒内给出正常语义诊断，仍不满足已批准修正计划里“L10 拒绝溢出并保留诊断证据”的门槛。另外 chapter14 仍有一处旧施工叙述。

## Files reviewed

- `CONTENT_REFACTORING_GUIDE.md`
- `LEARNCPP_GLOBAL_PLAN.md`
- `C04_Generic_CompileTime_Reflection/references/revision-plan-20260910.md`
- `C04_Generic_CompileTime_Reflection/references/standards-and-implementations.md`
- `C04_Generic_CompileTime_Reflection/chapters/14-splicing-generation.md`
- `C04_Generic_CompileTime_Reflection/chapters/15-annotations-frontier.md`
- `C04_Generic_CompileTime_Reflection/exercises/F01_frontier/CMakeLists.txt`
- `C04_Generic_CompileTime_Reflection/exercises/F01_frontier/README.md`
- `C04_Generic_CompileTime_Reflection/exercises/F01_frontier/probes/*.cpp`
- `C04_Generic_CompileTime_Reflection/exercises/cmake/StudySetup.cmake`
- `C04_Generic_CompileTime_Reflection/exercises/tools/compile_case.py`
- `C04_Generic_CompileTime_Reflection/exercises/tools/audit_good.py`
- `C04_Generic_CompileTime_Reflection/exercises/L10_constexpr/src/reference/constexpr_tools.hpp`
- `C04_Generic_CompileTime_Reflection/exercises/L10_constexpr/validation/good/constexpr_tools.hpp`

## Findings

### HIGH: L10 overflow diagnostic still times out instead of producing the intended semantic rejection

File: `C04_Generic_CompileTime_Reflection/exercises/L10_constexpr/validation/good/constexpr_tools.hpp:38`

Issue: `decimal_parser<Text, Index, Accumulator, false>` checks overflow with `static_assert`, but the same specialization still declares `value = decimal_parser<Text, Index + 1, Accumulator * 10 + digit>::value` at line 40. For `"2147483648"`, the rejected branch still forms the overflowing `int` template argument. The reviewer run timed out after 180 seconds and `compile_case.py` reported `verdict=FAIL`, not the required semantic diagnostic.

Evidence:

- `cmake --build C04_Generic_CompileTime_Reflection/build/foundation-review-l10 --config Release --clean-first`: PASS after current good/reference bytes.
- `ctest --test-dir C04_Generic_CompileTime_Reflection/build/foundation-review-l10 -C Release --output-on-failure`: FAIL only on `L10_decimal_overflow_diagnostic`.
- Evidence JSON: `C04_Generic_CompileTime_Reflection/build/foundation-review-l10/diagnostics/L10_decimal_overflow_diagnostic/evidence-1.json`, subject timed out at 180 seconds with `TimeoutExpired`.

Fix: Make the independent good implementation stop forming the next parser state when overflow is known. The Reference pattern already does this with `if constexpr (!fits) { static_assert(fits, ...); } else { return parse_decimal_impl<... Result * 10 + digit>(); }`. Keep the same diagnostic string expected by `L10_constexpr/CMakeLists.txt`.

### MEDIUM: chapter14 still describes the reflection backend as future leader integration

File: `C04_Generic_CompileTime_Reflection/chapters/14-splicing-generation.md:79`

Issue: The text says “反射backend由leader集成时”, but `P1_static_record` already contains `src/reflection/record_ops.hpp` and `checks/reflection_driver.cpp`; its README says the frontier build uses real meta queries, annotations and splicing under the same checker. This is now stale施工叙述 in learner-facing material.

Fix: Reword the paragraph to the current state: C++23 主线仍用 member pointer schema；P1 已有 reflection backend, but local MSVC cannot run it without `<meta>`, so the implementation is present and capability-gated.

### MEDIUM: diagnostic helper is path-length fragile on Windows validation paths

File: `C04_Generic_CompileTime_Reflection/exercises/cmake/StudySetup.cmake:63`

Issue: `c04_add_compile_case` nests each diagnostic build under `${CMAKE_BINARY_DIR}/diagnostics/${ARG_NAME}`. In the requested evidence directory `references/validation/revision-20260910/foundation-review-a02-setup`, nested CMake try-compile paths hit MSBuild `FTK1011` while compiling the trivial compiler check. The same A02 diagnostics pass from shorter `C04_Generic_CompileTime_Reflection/build/foundation-review-a02`, so this is not a DSL semantic failure; it is validation infrastructure path sensitivity.

Fix: Shorten diagnostic build roots on Windows, for example by hashing `${ARG_NAME}` or using a shallow `_diag/<short-id>` directory under the exercise build, while keeping evidence files append-only. Also classify this as infrastructure failure, as `compile_case.py` already does for configure failure.

## Passed checks

- F01 normal run: configure and build PASS; CTest reports 14/14 tests skipped with 0 failures on MSVC 19.51. This matches the current local boundary and does not claim any unsupported subject syntax passed.
- F01 failure-control run: with `GENERIC_STUDY_FRONTIER_ENABLE_FAILURE_CONTROL=ON`, build PASS and CTest FAILs on `F01_declared_failure_control`, proving non-77 subject failure is visible.
- A02 SETUP smoke: all seven diagnostic compile cases PASS from a short build directory, proving the SETUP include bridge can make positive control and subject use `validation/good`.
- `python -B -m py_compile` on `compile_case.py` and `audit_good.py`: PASS.
- Static grep for hardcoded secrets, empty catch, and broad masking fallback in the reviewed F01/helper scope found no blocking security issue.

## Tooling gaps

No callable `lsp_diagnostics` or `ast_grep_search` tool was available in this child lane. I used CMake/MSBuild/CTest, `py_compile`, and `rg` pattern scans instead; approval is withheld due to the concrete L10 and documentation findings above.

## Recommendation

REQUEST CHANGES.
