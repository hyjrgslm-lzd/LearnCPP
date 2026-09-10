# C08 TSan Fix Independent Review

Date: 2026-09-10

Role: non-author code/spec/security review. Scope is limited to the TSan fix and two adjacent infra checks requested by the lead thread.

## Verdict

APPROVE

No blocking correctness, safety, or spec-compliance issue found.

## Files Reviewed

- `C08_Concurrency/exercises/B3_call_once/solution.cpp`
- `C08_Concurrency/exercises/F3_seqcst_fence/solution.cpp`
- `C08_Concurrency/exercises/runtime_tests/scheduling_test.cpp`
- `C08_Concurrency/exercises/runtime_tests/scheduling_allocation_test.cpp`
- `C08_Concurrency/exercises/runtime_tests/CMakeLists.txt`
- `C08_Concurrency/exercises/cmake/StudySetup.cmake`
- `C08_Concurrency/exercises/cmake/Sanitizers.cmake`
- `C08_Concurrency/exercises/CMakeLists.txt`
- `C08_Concurrency/exercises/CMakePresets.json`
- `C08_Concurrency/references/validation/c08-revision/tsan-diagnosis/diagnosis-20260910.md`
- `C08_Concurrency/references/validation/c08-revision/tsan-diagnosis/r3-run-20260910-1745/*`

## Spec Compliance

PASS.

- B3 keeps the original exception-retry `std::call_once` exercise on non-TSan paths. Under only the verified Clang 18 + libstdc++ 13 + TSan combination, it still runs successful publication and function-local static publication checks before returning `77`.
- F3 keeps store-buffering checks, including the SC fence litmus, on the verified TSan path. It skips only `fence_publication()` under the verified unsupported tool combination and returns `77`.
- `runtime_scheduling_test.cpp` keeps the scheduling, cross-pool, and submit/shutdown checks in the TSan-running target.
- `scheduling_allocation_test.cpp` isolates the global `operator new/delete` allocation-injection test. It avoids defining replacement global allocation operators only for verified Clang 18 + libstdc++ 13 + TSan, then returns `77`.
- Future toolchain combinations are not silently skipped: the skip guard is exact on `__clang_major__ == 18`, `_GLIBCXX_RELEASE == 13`, and `__has_feature(thread_sanitizer)`.
- No `allow-multiple-definition`, TSan suppressions, `no_sanitize`, or broad alternate execution path was added.
- Runtime Python tool checks no longer disappear quietly: `runtime_tests/CMakeLists.txt` uses `find_package(Python3 3.10 REQUIRED COMPONENTS Interpreter)`.
- Root `C08_Concurrency/exercises/CMakeLists.txt` explicitly adds `F03_native_facilities`; that directory registers only independent native subject targets.

## Evidence Checked

Source hashes in current checkout match the recorded r3 evidence:

- `B3_call_once/solution.cpp`: `531a4ba8055ab90e26490f7a7db3565a80cccb8f1f2f0dcbbc0ff51ce6545c5c`
- `F3_seqcst_fence/solution.cpp`: `6ad71d1b4d36cc1e787001aa3b40156f1b3614c01b39075bd7102f000e350a52`
- `runtime_tests/scheduling_test.cpp`: `e47448fde11bfe668cb5f34f0bc8aafe8f0bf60c19c52bdfee5e7dc85ffcdf87`
- `runtime_tests/scheduling_allocation_test.cpp`: `da499b5d6d5bdf7e0ced3701ba22999058280552e00e16825306cf6d1df19059`

Independent Linux rerun from a fresh WSL `/tmp` ext4 source snapshot:

- Environment: `Ubuntu clang version 18.1.3 (1ubuntu1)`, `g++ 13.3.0`, `cmake 3.28.3`, `ninja 1.11.1`.
- Plain build: 4/4 requested targets built.
- Plain CTest: 4/4 passed.
- TSan build: 4/4 requested targets built.
- TSan CTest: 1 passed, 3 skipped, 0 failed.
- Direct TSan returns: B3 `77`, F3 `77`, scheduling `0`, scheduling allocation `77`.

Independent Windows rerun on existing `c08-tsan-fix-win-20260910-1730` build dir:

- Build of the same four targets exited `0`.
- CMake regenerate found Python `3.10.11`, satisfying the new required Python path for tool tests.
- CTest: 4/4 passed in `Release`.

Recorded r3 evidence checked:

- `r3-run-20260910-1745/status-summary.json` has configure/build/test status `0`, direct B3/F3/allocation status `77`, direct scheduling status `0`.
- `r3-run-20260910-1745/tsan-ctest-targets.txt` reports `B3_call_once_reference`, `F3_seqcst_fence_reference`, and `runtime_scheduling_allocation_test` skipped; `runtime_scheduling_test` passed.
- `r3-run-20260910-1745/plain-ctest-targets.txt` reports all four targets passed.
- `diagnosis-20260910.md` explicitly marks `agent-recovery-20260910-1646/*` as misplaced `/mnt/f` evidence and says it should not be cited as ext4 evidence.

## First-Party Source Check

Sufficient for the tool-limitation judgment.

- `std::call_once`: C++ draft `[thread.once.callonce]` permits exceptional active executions, retry, and passive publication synchronization. Reference used: `https://eel.is/c++draft/thread.once.callonce`.
- TSan exception limitation: Google/TSan C++ manual FAQ says C++ exceptions are unsupported. Reference used: `https://github.com/google/sanitizers/wiki/threadsanitizercppmanual`.
- `std::atomic_thread_fence`: C++ draft `[atomics.fences]` covers the three fence synchronization forms. Reference used: `https://eel.is/c++draft/atomics.fences`.
- TSan fence limitation: LLVM PR 166542 records that ThreadSanitizer does not support `std::atomic_thread_fence` and can produce false positives. Reference used: `https://lists.llvm.org/pipermail/cfe-commits/Week-of-Mon-20251103/771602.html`.
- Global new/delete conflict: LLVM compiler-rt TSan source defines C++ `operator new/delete` interceptors in `compiler-rt/lib/tsan/rtl/tsan_new_delete.cpp`. Reference used: `https://codebrowser.dev/llvm/compiler-rt/lib/tsan/rtl/tsan_new_delete.cpp.html`.

## Code Quality and Safety

PASS.

- Guard/macro behavior is exact enough for the proven issue and intentionally exposes future Clang/libstdc++/GCC/libc++ combinations instead of hiding them.
- `SKIP_RETURN_CODE 77` is configured where these executables are registered as CTest tests.
- Resource cleanup in `scheduling_allocation_test.cpp` preserves the original exception, releases latches on failure, and keeps observed data alive until after pool/producers unwind.
- Moving global allocation replacement out of `runtime_scheduling_test.cpp` fixes the TSan link ownership conflict without changing scheduler semantics.
- Pattern scan found no hardcoded secrets, `TSAN_OPTIONS`, suppressions, `allow-multiple-definition`, `no_sanitize`, empty catches, or best-effort fallback branches in the reviewed source/CMake scope.
- `git diff --check` on the reviewed scope found no whitespace errors; only existing CRLF conversion warnings were printed.

## Tool Gaps

- `lsp_diagnostics` and `ast_grep_search` were not available in this session. I substituted actual Windows/Linux compile+CTest coverage, `cmake --list-presets`, JSON parsing for `CMakePresets.json`, `git diff --check`, and `rg` pattern scans.
- `clangd` was not available on Windows PATH or inside the WSL distro.

## Non-Blocking Notes

- `diagnosis-20260910.md` line 3 says the scope is diagnostic-only, while the same file later documents the implemented fix. This is mildly confusing but not a correctness blocker because the evidence and changed-file list are explicit.
- Consider adding the exact source URLs above directly into `diagnosis-20260910.md` if this document is meant to stand alone without the review report.

## Issues

CRITICAL: 0

HIGH: 0

MEDIUM: 0

LOW: 0

Recommendation: APPROVE.
