# C08 student validation asset review

Status: REVISE

Scope reviewed:

- `C08_Concurrency/exercises/tools/verify_students.py`
- `C08_Concurrency/exercises/validation/README.md`
- `C08_Concurrency/exercises/validation/numeric_student_checks.hpp`
- `C08_Concurrency/exercises/L3_par_vs_seq_bench/main.cpp`
- `C08_Concurrency/exercises/L3_par_vs_seq_bench/checks.hpp`
- `C08_Concurrency/exercises/L3_par_vs_seq_bench/reference.hpp`
- `C08_Concurrency/exercises/Capstone3_parallel_compute/main.cpp`
- `C08_Concurrency/exercises/Capstone3_parallel_compute/checks.hpp`
- `C08_Concurrency/exercises/Capstone3_parallel_compute/reference.hpp`
- `C08_Concurrency/exercises/I2_hazard_pointer/main.cpp`
- `C08_Concurrency/exercises/I2_hazard_pointer/checks.hpp`
- `C08_Concurrency/exercises/I2_hazard_pointer/student.hpp`

## Findings

### HIGH: repeated runs overwrite prior validation evidence

File: `C08_Concurrency/exercises/tools/verify_students.py:493`

`copy_tree()` deletes the previous copied course when `dst.exists()`, `verify_one()` deletes the previous build tree at line 600, and `checked_process()` writes fixed names such as `configure.json`, `build.json`, and `run.json` at lines 527-530. `main()` also overwrites `summary.json` at line 704. That means reusing a `--build-root` silently replaces earlier logs and summaries.

Risk: this violates the review contract that existing outputs are not overwritten. It also weakens evidence integrity: an author or reviewer can accidentally replace the raw stdout/stderr/JSON from a previous run without a distinct run id.

Fix: make each invocation write to a unique run directory, or fail fast if the selected `build-root` / case log directory already exists. Keep destructive cleanup limited to directories created inside the current run id.

### MEDIUM: Capstone3 good overlay claims optional SIMD without implementing it

File: `C08_Concurrency/exercises/tools/verify_students.py:325`

The public Capstone3 good overlay routes `version == "sse2"` to `student_gemm_tiled(...)`. The starter explicitly says the optional SIMD part should be a real explicit vector loop, and the good overlay includes `concurrency_study/simd_kernels.hpp`, but the implementation never uses a SIMD backend for that path.

Risk: running the good overlay with `--with-simd` would pass matrix output while proving only tiled scalar GEMM. The mandatory six parts are covered separately, so this is not a blocker for the current mandatory Capstone3 pass, but it is a false positive if optional SIMD is advertised as validated.

Fix: either remove the optional SIMD claim from the public good overlay, or implement a real independent SSE2/vector path for `version == "sse2"` and make the validation runner exercise it when the capability is available.

## Evidence

Representative command run in a fresh build root:

```powershell
python tools\verify_students.py --build-root ..\build\student-review --target I2_hazard_pointer --target Capstone3_parallel_compute --config Release --timeout 60
```

Result: PASS, 6 cases.

- `I2_hazard_pointer`: initial exit 1, good exit 0, bad exit 1.
- `Capstone3_parallel_compute`: initial exit 1, good exit 0, bad exit 1.
- Good dependency traces were collected from compiler/build output: I2 had 114 entries; Cap3 had 115 entries.
- Good traces did not include `reference.hpp`, `solution.cpp`, or validation paths.

Static pattern scan:

```powershell
rg -n "catch\s*\([^)]*\)\s*\{\s*\}|catch \(\.\.\.\) \{\s*\}|apiKey|api_key|password|secret|token\s*=\s*\"|return\s+77|WILL_FAIL|PASS_REGULAR_EXPRESSION|SKIP_REGULAR_EXPRESSION|include_trace|dependency_trace|shutil\.rmtree|write_text|TODO" C08_Concurrency\exercises\tools\verify_students.py C08_Concurrency\exercises\validation C08_Concurrency\exercises\Capstone3_parallel_compute C08_Concurrency\exercises\L3_par_vs_seq_bench C08_Concurrency\exercises\I2_hazard_pointer
```

Result: no hardcoded secrets, empty catches, `WILL_FAIL`, or CTest regex shortcuts found in the reviewed scope.

LSP diagnostics: unavailable in this environment; `tool_search` did not expose an LSP diagnostics tool. I did not approve the asset set.

## Recommendation

REQUEST CHANGES for the evidence-overwrite issue before this validation runner becomes authoritative. After that, rerun at least I2 and Cap3 in a fresh root, then approve only after the author has stable full-16 initial/good/bad evidence.
