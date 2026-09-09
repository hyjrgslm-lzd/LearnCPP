# C02 Independent Closeout - Storage 11-15 final

Reviewer: C02 independent teaching/code reviewer
Scope: incremental closeout for storage batch 11-15 after root fixes. Full teaching/code review base is `reviewer-storage-11-15-r1.md`; this report only rechecks the prior L12 blocker, L14 frontier runner changes, and the visible ASan helper interface impact. Technical ClangCL/DLL matrix remains owned by the verifier lane per root instruction.
Verdict: APPROVE
Date: 2026-09-09

## Files reviewed

Focused files:

- `Core_Study/exercises/L12_storage/CMakeLists.txt`
- `Core_Study/exercises/L12_storage/checks/expect_failure.cmake`
- `Core_Study/exercises/L14_ub/CMakeLists.txt`
- `Core_Study/exercises/L14_ub/checks/frontier_compile.cmake`
- `Core_Study/exercises/L14_ub/checks/frontier/p2287_base_member_designator.cpp`
- `Core_Study/exercises/cmake/StudySetup.cmake`
- `Core_Study/exercises/tools/record_process.py`
- Previously reviewed teaching/code files for chapters 11-15 and `P1_object_buffer`, bound again by hash.

Evidence root: `Core_Study/references/validation/reviews/c02-independent-evidence/storage-11-15-r2-closeout/`

`source-hashes.json` binds the current closeout inputs. Key hashes:

- `L12_storage/CMakeLists.txt`: `9E87001A142415F70376BB067C3F0870A69F36BC139BDCB3267F5EFD7D744AA2`
- `L14_ub/CMakeLists.txt`: `9F1532AFBBC6F395F8AA38264C961346B2CB52CC481486FD76AA5D75FA2A6674`
- `L14_ub/checks/frontier_compile.cmake`: `47E07F26B41D2A76A4F562B6EE15F9F3315CB098A259C3AFFC398513CD796058`
- `exercises/cmake/StudySetup.cmake`: `3D5F0EDB294012C2465AF14634BB3179703AF085B2C6B166674211E0F41FC26C`
- `exercises/tools/record_process.py`: `A6CBE3E05A8A9845843254E17656E24C6F90E259F0AD93151A8AA09571246002`
- `P1_object_buffer/src/reference/object_buffer.hpp`: `11031B05BAF698837601183CD021AB24ED11014639E2F49053F0ACDBA9EE0861`

## Prior blocker closeout

### L12 full `EXPECTED_TEXT` fake-slot weakness: closed

Original blocker from `reviewer-storage-11-15-r1.md`: `Core_Study/exercises/L12_storage/CMakeLists.txt` split the intended full diagnostic into multiple CTest argv entries, so the wrapper only matched `slot`.

Current source at `Core_Study/exercises/L12_storage/CMakeLists.txt:40` passes the full text as one quoted argument:

```cmake
"-DEXPECTED_TEXT=slot must report engaged after successful construct"
```

Independent rerun evidence:

- `incremental-summary.json`
  - `l12-configure-debug exit=0`
  - `l12-build-debug exit=0`
  - `l12-negative-verbose exit=0`
  - `l12-fake-slot-full-text-probe exit=1`
- `l12-negative-verbose.stdout.txt` shows CTest invoking:
  - `"-DEXPECTED_TEXT=slot must report engaged after successful construct"`
- `l12-fake-slot-full-text-probe.stderr.txt` shows the old fake `slot unrelated failure from fake` is now rejected:
  - `expected diagnostic not found: slot must report engaged after successful construct`

This proves the public bad wrapper now checks the intended diagnostic instead of a broad substring.

## L14 incremental review

### Frontier diagnostic matching: acceptable

`Core_Study/exercises/L14_ub/checks/frontier_compile.cmake:47-50` now extracts diagnostic lines before matching expected failure patterns. This avoids matching words that only occur in target/source filenames such as `assignment` or `designator`.

Independent evidence:

- `l14-frontier-ctest-r.stdout.txt`: `ctest -R L14_frontier -V` exits 0 and runs all frontier cases.
- Baseline is included first in the `-R` run, then P2287/P2748/P2953/provenance cases run after it.
- `L14_frontier_p2287_base_member_designator_compile-*.txt` records real MSVC `error C2440: ... initializer list ... Derived` from `p2287_base_member_designator.cpp`, matching the current P2287 expected diagnostic.
- `L14_frontier_p2748_return_temp_ref_compile-*.txt` records real `warning C4172`, so SKIP after diagnostic is based on compiler output.
- `L14_frontier_p2953_defaulted_assignment_compile-*.txt` has `result=0` and no expected diagnostic, so SKIP is correctly classified as accepted without rejection diagnostic.
- `l14-diagnostic-false-positive-probe.json`: a missing target named `DefinitelyMissingAssignmentTarget` with `EXPECTED_FAILURE=assignment` exits 1, proving the runner does not pass merely because the target name contains the word.

### Baseline fixture: acceptable

`Core_Study/exercises/L14_ub/CMakeLists.txt:84-91` sets `L14_frontier_baseline_compile` as `FIXTURES_SETUP` and all other frontier cases as `FIXTURES_REQUIRED`. My `ctest -R L14_frontier -V` run shows baseline executes before the other cases, so filtered frontier runs no longer skip the prerequisite baseline check.

### P2287 source shape: acceptable

`Core_Study/exercises/L14_ub/checks/frontier/p2287_base_member_designator.cpp:9-10` still uses `Derived{.base = 1, .member = 2}`. It does not use a base-class-name designator, matching the teaching text and root instruction.

## ASan helper impact review

`Core_Study/exercises/cmake/StudySetup.cmake:20-82` now deploys a matching Windows ASan runtime after build for executable/shared/module targets. For MSVC it derives the runtime from the compiler directory; for Clang/clang-cl it queries the compiler resource directory and refuses to substitute a same-named DLL from another compiler. clang-cl also links the matching import/thunk libraries and SEH interceptor symbol.

`Core_Study/exercises/tools/record_process.py:13-19` sets the inherited Windows error mode to include process-level suppression of loader/error dialogs. This matches the required boundary: infrastructure failures should be captured as process evidence, not visible desktop popups.

I did not rerun the ClangCL/DLL matrix in this teaching/code closeout; root explicitly assigned that runtime verification to the technical verifier lane. I reviewed the helper shape for masking risk and did not find a broad fallback that would hide a missing runtime: missing matching runtime/import/thunk files are fatal at configure/link setup, not silently replaced.

## Previously reviewed teaching/code status retained

The r1 review already found no blocking issue in the teaching depth and main implementations for:

- 11 layout/alignment/representation
- 12 implicit creation / allocator array-vs-elements / union / `start_lifetime_as` teaching and `storage_slot` implementation
- 13 alias/launder/provenance and DR boundaries
- 14 behavior categories / C++26 erroneous behavior / safe-vs-frontier-vs-unsafe evidence boundaries
- 15 / P1 frozen `object_buffer<T>` strong guarantee, borrow invalidation, Student placeholder, good/bad variants, and throwing-move-only rejection

Those findings remain bound to the current hash set above for the files rechecked here. No new teaching/API mismatch was found in the incrementally changed files.

## Validation summary

New closeout evidence:

- `incremental-summary.json`
- `l12-negative-verbose.stdout.txt`
- `l12-fake-slot-full-text-probe.stderr.txt`
- `l14-frontier-ctest-r.stdout.txt`
- `l14-diagnostic-false-positive-probe.json`
- `build/c02-reviewer-storage-r2-l14-frontier/frontier/*.txt` raw frontier outputs
- `source-hashes.json`

Prior full storage evidence retained:

- `storage-11-15-r1b/msvc-debug-summary.json`
- `storage-11-15-r1b/msvc-release-summary.json`
- direct/negative/student evidence listed in `reviewer-storage-11-15-r1.md`

Tooling limitation: no `lsp_diagnostics` tool is exposed in this worker session. I used source inspection, MSVC/CMake/CTest, direct fake-negative probes, raw frontier outputs, and source hashes.

## Recommendation

APPROVE for the storage 11-15 teaching/code gate.

The only blocker from `reviewer-storage-11-15-r1.md` is closed. L14 diagnostic matching and baseline fixture changes are consistent with the reviewed teaching model. Public ASan helper changes do not introduce a masking fallback in the code I inspected; verifier owns the deeper ClangCL/DLL runtime proof.
