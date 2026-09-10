# C08 student validation asset review r2

Status: COMMENT - no blocking findings in this r2 scope.

Relation to r1: this review verifies closure of the two findings in `student-assets-review.md` without rewriting that report.

Scope reviewed:

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
- `C08_Concurrency/build/sv-final3/summary.json`

## r1 Closure

### Closed: repeated runs overwrite prior validation evidence

Evidence: `verify_students.py:681-688` now checks whether the requested `build_root` already exists and has content. If it does, the run writes under a fresh `run-YYYYMMDD-HHMMSS-ffffff` child. The per-case destructive cleanup at `verify_students.py:490-506` and `verify_students.py:593-601` is now scoped inside that selected run root.

Validation: after a 6-case run wrote `C08_Concurrency/build/student-review-r2/summary.json`, I reused the same `--build-root` for one I2 initial case. The new run wrote to `C08_Concurrency/build/student-review-r2/run-20260910-174407-184658/...`; the original root `summary.json` still reported the prior 6-case run.

### Closed: Capstone3 good overlay claims optional SIMD without implementing it

Evidence: `verify_students.py:326` now makes the good overlay throw `optional SIMD not implemented by validation good overlay` for `version == "sse2"`. The mandatory validation loop remains limited to `"naive"`, `"tiled"`, `"threaded"`, and `"par"` at `verify_students.py:363-367`.

Validation: running the Cap3 good executable with `--with-simd` returned exit code 1 and printed:

```text
Cap3 mandatory student small checks passed
optional SIMD not implemented by validation good overlay
```

## Full Summary Review

`C08_Concurrency/build/sv-final3/summary.json` was reviewed as the author full-run evidence.

- Summary status: `PASS`.
- Cases: 48.
- Targets: 16.
- Modes: `initial`, `good`, `bad`.
- Strict run-log check: all `good` cases exit 0; all `initial` and `bad` cases exit 1; no timeout, cleanup error, or sanitizer marker in `run.json`.
- Diagnostic check: every `initial` case contains its fixed `INITIAL_EXPECTED` diagnostic; every `bad` case contains its fixed `BAD_MUTATIONS` diagnostic.
- Reproducibility check: every `source_sha256` and `overlay_sha256` in `sv-final3` recomputes from the current source tree plus the current `verify_students.py` overlay builder.
- Dependency check: every `good` case has a nonzero compiler dependency trace; banned dependency leak count is 0 for `reference.hpp`, `solution.cpp`, and `/validation/`.

## Representative Rerun

Command:

```powershell
python tools\verify_students.py --build-root ..\build\student-review-r2 --target I2_hazard_pointer --target Capstone3_parallel_compute --config Release --timeout 60
```

Result: PASS, 6 cases.

- `I2_hazard_pointer`: initial exit 1, good exit 0, bad exit 1.
- `Capstone3_parallel_compute`: initial exit 1, good exit 0, bad exit 1.
- Good dependency trace counts: I2 = 114, Cap3 = 115.
- Good dependency traces included only the student/checker/shared headers expected for those targets; no Reference or validation leak found.

## L3 / Capstone3 Checker Split

The original `main.cpp` files now include `checks.hpp` directly and retain their starter TODO bodies:

- `L3_par_vs_seq_bench/main.cpp`: `student_light`, `student_heavy`, and `student_map` remain incomplete starter functions.
- `Capstone3_parallel_compute/main.cpp`: `student_gemm`, `student_sum`, and `student_sort` remain incomplete starter functions.

The Reference headers now include `checks.hpp` and keep benchmark/reference dispatch separate. The extracted checkers have an independent role because the student entry can use input generation and oracles without including the answer-bearing `reference.hpp`.

`C08_Concurrency/exercises/validation/numeric_student_checks.hpp` no longer exists in the current tree. Its former responsibility appears to have been folded into the two local `checks.hpp` files. No duplicate helper remains to remove.

## Static Scan

Reviewed-scope scan found no hardcoded secrets, CTest `WILL_FAIL`, CTest regex pass/skip shortcuts, or Reference/validation dependency in the Student good traces. The remaining `return 77` in `Capstone3_parallel_compute/main.cpp` is the documented optional-SSE2 unavailable path in the starter.

LSP diagnostics: unavailable in this environment; `tool_search` did not expose an LSP diagnostics tool. Because of that tooling gap, this report uses `COMMENT` rather than a formal `APPROVE`, but I found no r2 blocker requiring author返修.
