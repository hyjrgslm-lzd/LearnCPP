# C04 revision foundation review r2

日期：2026-09-10

结论：REQUEST CHANGES。三条原发现中，chapter14 文案已关闭，StudySetup 深路径 FTK1011 已关闭；L10 `validation/good` 的源码和 post-review 记录已按要求修，但本机复验中 overflow 诊断仍 180 秒超时，不能算关闭。

## Rechecked files

- `C04_Generic_CompileTime_Reflection/exercises/L10_constexpr/validation/good/constexpr_tools.hpp`
- `C04_Generic_CompileTime_Reflection/references/validation/revision-20260910/blind-core/post-review/repair.json`
- `C04_Generic_CompileTime_Reflection/chapters/14-splicing-generation.md`
- `C04_Generic_CompileTime_Reflection/exercises/cmake/StudySetup.cmake`
- `C04_Generic_CompileTime_Reflection/exercises/tools/compile_case.py`

## Status by previous finding

### OPEN: L10 overflow diagnostic still fails runtime validation

File: `C04_Generic_CompileTime_Reflection/exercises/L10_constexpr/validation/good/constexpr_tools.hpp:41`

Current source status: the post-review good file now gates the recursive `Accumulator * 10 + digit` transition inside `if constexpr (fits)`, and `repair.json` records this as a post-blind repair. Current good SHA is `65ECB96A7BF05EB3C4F1ECE4A06C2FF313299C9FE5A7C62D010C7FDA3CF53077`, matching `blind-core/post-review/repair.json`.

Validation status: still FAIL. Reconfigured `C04_Generic_CompileTime_Reflection/build/foundation-review-l10`, then ran:

```powershell
ctest --test-dir C04_Generic_CompileTime_Reflection/build/foundation-review-l10 -C Release -R "L10_decimal_overflow_diagnostic" --output-on-failure
```

Result: `L10_decimal_overflow_diagnostic` failed after 190.22s. The new short diagnostic evidence is `C04_Generic_CompileTime_Reflection/exercises/build/_diag/206590d3295a/Release/evidence-1.json`; `configure.status=PASS`, `control.status=PASS`, `subject.timeout=true`, `verdict=FAIL`.

Fix: keep the syntactic isolation, but reduce the overflow diagnostic to a form MSVC can reject normally under the helper timeout. If this is an MSVC frontend hang, add a separate minimal control for the compiler bug and use a bounded diagnostic source that still proves the course contract.

### CLOSED: chapter14 stale leader-integration wording

File: `C04_Generic_CompileTime_Reflection/chapters/14-splicing-generation.md:79`

Current text now says P1 already has `src/reflection/record_ops.hpp`, but this machine did not run it because the corresponding capability is missing. This matches the implemented P1 state and avoids stale施工叙述.

### CLOSED: StudySetup path-length fragility for diagnostic cases

File: `C04_Generic_CompileTime_Reflection/exercises/cmake/StudySetup.cmake:62`

Current implementation hashes `${CMAKE_BINARY_DIR}|${ARG_NAME}` to a 12-character key and writes nested diagnostic builds under `C04_Generic_CompileTime_Reflection/exercises/build/_diag/<key>/$<CONFIG>`. It also passes `"--platform=${CMAKE_GENERATOR_PLATFORM}"`, so empty platform is represented as one argv item and `compile_case.py` can omit `-A` when empty.

Deep-path validation:

```powershell
cmake -S C04_Generic_CompileTime_Reflection/exercises/A02_field_projection -B C04_Generic_CompileTime_Reflection/references/validation/revision-20260910/foundation-review-a02-setup -G "Visual Studio 18 2026" -A x64
ctest --test-dir C04_Generic_CompileTime_Reflection/references/validation/revision-20260910/foundation-review-a02-setup -C Release -R "A02_diag_(unknown|dup|empty|bad_id|rvalue|trail|nul)" --output-on-failure
```

Result: reconfigure PASS; A02 diagnostic subset PASS 7/7 from the previously failing deep parent path. The old FTK1011 failure did not recur.

## Additional evidence concern

File: `C04_Generic_CompileTime_Reflection/exercises/tools/compile_case.py:68`

`compile_case.py` hashes the diagnostic source, control, itself, and `StudySetup.cmake`, but not the headers included by the diagnostic source, including `validation/good/constexpr_tools.hpp`. The r2 overflow evidence therefore does not by itself bind the tested good header SHA; I separately recorded the current file hash above. The later whole-good include-trace gate can cover this, but the helper evidence remains incomplete when used alone.

## Validation

- `python -B -m py_compile C04_Generic_CompileTime_Reflection/exercises/tools/compile_case.py C04_Generic_CompileTime_Reflection/exercises/tools/audit_good.py`: PASS. This only checks syntax; it does not prove `audit_good` branch behavior.
- `Get-FileHash` confirms current L10 good SHA equals `blind-core/post-review/repair.json`.
- `rg` confirmed chapter14 no longer contains the old “leader 集成时” wording in the reviewed paragraph.

## Recommendation

REQUEST CHANGES for this slice until `L10_decimal_overflow_diagnostic` returns a normal semantic rejection instead of timeout. The chapter14 and StudySetup findings can be marked closed.
