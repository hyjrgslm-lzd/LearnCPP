# C08 student validation platform review

Status: **APPROVE**

Reviewer: non-author platform increment review

Current script under review:

- `C08_Concurrency/exercises/tools/verify_students.py`
- SHA-256: `9756a46c307e5c142f0d6751217a9380b32a9ed67d46ca9c5f1b825d04471158`

## Code Review Summary

**Files / evidence reviewed:** 7 evidence groups

- Current `verify_students.py`
- `students-win-r2/summary.json`
- `students-win-r2/public-index.json`
- `students-win-r2/verify_students.py.txt`
- `linux-final/student-portability-20260910-175508-13ddcee8/student-portability-summary.json`
- `linux-final/students-r2-full-20260910-182216-452abc20/script-summary.json`
- `linux-final/students-r2-full-20260910-182216-452abc20/students-r2-full-summary.json`
- `linux-final/students-r2-full-20260910-182216-452abc20/compile-command-samples.txt`

**Total Issues:** 0

### By Severity

- CRITICAL: 0
- HIGH: 0
- MEDIUM: 0
- LOW: 0

## Scope verdict

The r2 platform changes close the two Linux/Ninja blockers reproduced in the r1 portability evidence:

- `configure_and_build()` now passes `-DCMAKE_BUILD_TYPE={config}` during CMake configure, not only `--config Release` during build.
- POSIX `-H` include output is parsed into dependency trace entries for real compiler include lines.

No new blocker found in the platform increment. The prior r1 approval for the student assets remains valid.

## Evidence

### Static script review

- `verify_students.py:544-548` now builds the configure command with `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON`, `-DCMAKE_BUILD_TYPE={config}`, and `-DCMAKE_CXX_FLAGS={include_flag}`.
- `verify_students.py:542-543` keeps the existing Windows behavior: `/showIncludes` is still the default on `os.name == "nt"`, `-H` is used otherwise.
- `verify_students.py:575-584` keeps the Windows drive-root regex and adds POSIX `-H` parsing only for compiler-output lines whose stripped text starts with `.`. The accepted token must still be an absolute path (`/` or drive-root). This does not introduce a broad fallback that silently accepts arbitrary relative include text.
- `verify_students.py:585-596` still reads depfiles/tlog as an additional source and filters through the same `add_dependency()` absolute-path gate.
- `compile()` check on the current Python script passed.

### r1 Linux failure evidence preserved

`linux-final/student-portability-20260910-175508-13ddcee8/student-portability-summary.json` records the original blockers:

- Ninja configure did not set `CMAKE_BUILD_TYPE=Release`.
- The sampled compile commands lacked `-O3` and `-DNDEBUG`.
- POSIX dependency audit found no compiler dependency trace.
- The sampled good executables themselves ran with exit `0`; the failure was the audit/tooling path, not the good overlay behavior.

### r2 Linux final evidence

`linux-final/students-r2-full-20260910-182216-452abc20/students-r2-full-summary.json`:

- command exit code: `0`
- script SHA: `9756a46c307e5c142f0d6751217a9380b32a9ed67d46ca9c5f1b825d04471158`
- completed cases: `48`
- mode counts: `16 initial`, `16 good`, `16 bad`
- good dependency traces: `16/16`
- good dependency trace range: `276..355`
- reference leak targets: `[]`
- `compileCommandSamplesHaveO3AndDNDEBUG: true`
- stderr bytes: `0`

`linux-final/students-r2-full-20260910-182216-452abc20/script-summary.json` and stdout agree on:

- `status: PASS`
- `cases: 48`
- targets: 16
- modes: `initial`, `good`, `bad`

`compile-command-samples.txt` includes Linux/Ninja sample compile commands with both `-O3` and `-DNDEBUG` for representative targets including `I2_hazard_pointer`, `Capstone3_parallel_compute`, `I3_rcu`, and `R2_qsbr`.

### r2 Windows focused evidence

`students-win-r2/public-index.json`:

- script SHA: `9756a46c307e5c142f0d6751217a9380b32a9ed67d46ca9c5f1b825d04471158`
- all referenced public files exist and match their SHA-256 entries
- six focused cases cover `I2_hazard_pointer` and `Capstone3_parallel_compute` across `initial`, `good`, `bad`
- good exits are `0`; initial/bad exits are `1`

`students-win-r2/verify_students.py.txt` hash matches the current script hash.

Focused Windows logs checked:

- every configure JSON includes `-DCMAKE_BUILD_TYPE=Release`
- every build JSON includes `--config Release`
- `I2_hazard_pointer/good/dependencies.txt`: 111 dependency entries
- `Capstone3_parallel_compute/good/dependencies.txt`: 112 dependency entries
- neither good dependency trace contains `reference.hpp`, `solution.cpp`, or `/validation/`

## Limitations

- LSP diagnostics were not available in this review surface, so I do not claim “LSP clean”. This approval relies on static inspection, Python syntax compilation, MSVC/Windows focused evidence, Linux/Ninja full compiler evidence, strict run exits, and `rg` pattern inspection.
- I did not rerun full Windows 48-case validation, per task instruction. The r1 full Windows approval and r2 focused Windows six-case evidence are the Windows basis.
- In the local final Linux export I found `script-summary.json`, `students-r2-full-summary.json`, stdout JSON, command metadata, snapshot/source manifests, and compile command samples. I did not find a separate `case-outputs/` directory in this local copy; the per-case result data used above is embedded in `script-summary.json` and stdout.

## Recommendation

**APPROVE**. No CRITICAL/HIGH/MEDIUM/LOW blocking issue remains in the platform increment.
