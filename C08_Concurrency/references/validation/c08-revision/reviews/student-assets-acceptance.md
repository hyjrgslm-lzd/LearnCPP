# C08 student validation assets acceptance

Status: APPROVE.

This is the final non-author acceptance for the C08 student validation asset slice. It supersedes the r2 wording limit in `student-assets-review-r2.md`: LSP diagnostics were not available in this environment, so this report does not claim LSP clean. For this repository slice, acceptance is based on actual MSVC build execution, strict process results, controlled good/bad overlays, compiler dependency traces, raw log SHA verification, and static `rg` scans.

## Accepted Scope

- `C08_Concurrency/exercises/tools/verify_students.py`
- `C08_Concurrency/exercises/validation/README.md`
- `C08_Concurrency/exercises/L3_par_vs_seq_bench/main.cpp`
- `C08_Concurrency/exercises/L3_par_vs_seq_bench/checks.hpp`
- `C08_Concurrency/exercises/L3_par_vs_seq_bench/reference.hpp`
- `C08_Concurrency/exercises/Capstone3_parallel_compute/main.cpp`
- `C08_Concurrency/exercises/Capstone3_parallel_compute/checks.hpp`
- `C08_Concurrency/exercises/Capstone3_parallel_compute/reference.hpp`
- `C08_Concurrency/exercises/I2_hazard_pointer/main.cpp`
- `C08_Concurrency/exercises/I2_hazard_pointer/checks.hpp`
- `C08_Concurrency/exercises/I2_hazard_pointer/student.hpp`
- Public evidence under `C08_Concurrency/references/validation/c08-revision/students-win-r1/`

## Evidence

Public export checked:

- `students-win-r1/summary.json`: `status=PASS`, `cases=48`, 16 targets, modes `initial/good/bad`.
- `students-win-r1/public-index.json`: 448 file entries with SHA-256; every exported file exists and matches its recorded hash.
- `public-index.json` binds the evidence to current `verify_students.py` byte SHA-256:
  `91565efd46583e8733385a3432c4eaa43e4374bcb0d86871f9dae7927622dcb2`.

Strict result checks:

- All 16 `good` cases exit `0`.
- All 16 `initial` cases exit `1` and contain the fixed initial diagnostic.
- All 16 `bad` cases exit `1` and contain the fixed bad diagnostic.
- No checked `run.json` contains timeout, cleanup error, or sanitizer marker.
- All `good` cases have nonzero compiler dependency traces.
- Reference/answer leakage count is 0 for `reference.hpp`, `solution.cpp`, and `/validation/` in `good` dependency traces.

Reproducibility checks:

- Every `source_sha256` in the 48-case summary recomputes from the current source tree using the current validation script's text-hash rule.
- Every `overlay_sha256` recomputes from the current source tree plus the current `verify_students.py` overlay builder.
- Representative rerun passed for I2 and Capstone3:

```powershell
python tools\verify_students.py --build-root ..\build\student-review-r2 --target I2_hazard_pointer --target Capstone3_parallel_compute --config Release --timeout 60
```

- Reusing that same build root created `run-20260910-174407-184658/...`, leaving the previous 6-case root summary intact.
- Running the Capstone3 good executable with `--with-simd` returned exit `1` and printed `optional SIMD not implemented by validation good overlay`, so optional SIMD is not falsely accepted as a good implementation.

Static/code-shape checks:

- The two original student `main.cpp` files only switched from answer-bearing `reference.hpp` to independent `checks.hpp` plus needed standard includes; their student TODO bodies remain starter code.
- Capstone3 good independently implements the mandatory `naive/tiled/threaded/par` GEMM, `plain/par/threaded` reduction, and sort paths; it does not call Reference `gemm_*`, `sum_*`, or `sort_values`.
- L3 good only reuses the allowed shared traversal `cs::numeric::map_with`.
- `C08_Concurrency/exercises/validation/numeric_student_checks.hpp` no longer exists; its useful responsibility is now local to the two `checks.hpp` files, so no duplicate validation helper remains.
- Static `rg` scan found no hardcoded secrets, CTest `WILL_FAIL`, pass/skip regex shortcuts, or Student good dependency on Reference/validation files.

## Limitation

LSP diagnostics were not run because no LSP diagnostics tool was available in this environment. This is recorded as a validation limitation, not as a clean LSP result.

## Verdict

APPROVE for the student validation asset slice. The previously reported overwrite and Capstone3 optional-SIMD false-positive issues are closed, and no substantive blocker remains in this reviewed scope.
