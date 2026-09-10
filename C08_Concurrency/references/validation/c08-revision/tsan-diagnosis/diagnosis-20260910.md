# C08 TSan Diagnosis 2026-09-10

Scope: diagnostic only. No tracked course source was modified.

Environment:

- WSL distro: `LearnCPP-C08-Ubuntu-24.04`
- Probe work dir: `/root/learncpp-c08-builds/tsan-diagnosis-20260910-1658`
- Source snapshot: `/root/learncpp-c08-src/baseline-20260910-162842-fc6bb723`
- TSan build: `/root/learncpp-c08-builds/baseline-20260910-162842-fc6bb723/linux-tsan`
- Compiler: `Ubuntu clang version 18.1.3 (1ubuntu1)`
- Source hashes:
  - `B3_call_once/solution.cpp`: `9d06981999a3f748b89795ae8b43966699f6ab3781fb2c0d59eda4afaba6c171`
  - `F3_seqcst_fence/solution.cpp`: `ad1abb30bb036e7f2dc8e6749a1854273758291da769816b5db778dda17da6f1`
  - `runtime_tests/scheduling_test.cpp`: `7eec6e906b51b41b021276b0116a5c17d2222da277e5bf639d95af96ab73ecd1`

## B3 call_once

Symptom: `B3_call_once_reference` times out under Clang 18 TSan.

Minimal reproduction: `agent-run-20260910-1658/call_once_probe.cpp`.

Results:

- `status-existing-B3-tsan.txt=124`
- `status-call-once-tsan-same-thread-retry.txt=124`
- `status-call-once-tsan-thread-retry.txt=124`
- `status-call-once-tsan-async-retry.txt=124`
- `status-call-once-tsan-async-8-retry.txt=124`
- `status-call-once-tsan-no-throw.txt=0`
- `status-call-once-tsan-async-8-control.txt=0`
- Plain, unsanitized retry controls all return `0`.

Root cause classification: tool limitation, not course lifetime bug. The same-thread retry case has no inter-thread lifetime hazard, but hangs only when TSan sees a `std::call_once` active execution that throws and is retried. The no-throw `std::call_once` control passes under TSan.

Standard basis: `std::call_once` explicitly permits an active execution to throw, propagates that exception, allows later active executions, and synchronizes each active completion with the next active start; the successful returning execution synchronizes with passive returns. See C++ draft `[thread.once.callonce]`.

External TSan basis: LLVM's 2026 Clang documentation patch records C++ exception paths as unsupported under TSan and warns of unreliable behavior on thrown-exception paths.

Minimal fix recommendation:

- Keep B3 source semantics unchanged.
- Exclude or `SKIP_RETURN_CODE 77` the B3 exception-retry reference under `CONCURRENCY_STUDY_SANITIZER=thread`.
- If TSan coverage for `std::call_once` is desired, add a separate no-throw `call_once` control target under TSan. Do not use suppressions or a longer timeout.

## F3 fence publication

Symptom: `F3_seqcst_fence_reference` intermittently reports a TSan data race at `solution.cpp:57` write and `solution.cpp:64` read.

Reproduction:

- Prior baseline: `linux-tsan-partial-ctest.txt` reported the race.
- Current rerun: 20 isolated executions produced TSan exit code `66` on runs `09`, `14`, and `20`; single `ctest -R ^F3_seqcst_fence_reference$` happened to pass.

Control results:

- `status-fence-tsan-atomic.txt=0`
- `status-fence-tsan-fence0.txt=0`
- `status-fence-tsan-fence1.txt=0`
- `status-fence-tsan-fence2.txt=0`
- `status-fence-plain-fence0.txt=0`
- `status-fence-plain-fence1.txt=0`
- `status-fence-plain-fence2.txt=0`

Root cause classification: TSan modeling limitation for `std::atomic_thread_fence`, not a demonstrated C++ memory-model bug.

Standard basis:

- Form 0 is release fence -> relaxed atomic store -> acquire atomic wait/load. This is covered by C++ draft `[atomics.fences]` release-fence-to-acquire-operation synchronization.
- Form 1 is release atomic store -> relaxed atomic wait/load -> acquire fence. This is covered by release-operation-to-acquire-fence synchronization.
- Form 2 is release fence -> relaxed atomic store -> relaxed atomic wait/load -> acquire fence. This is covered by release-fence-to-acquire-fence synchronization.

External TSan basis: LLVM PR 166542 adds a Clang warning because ThreadSanitizer does not support `std::atomic_thread_fence`, leading to false positives; linked LLVM issue 52942 contains the same pattern of a valid fence protocol reported as a race.

Minimal fix recommendation:

- Keep the fence examples unchanged for native/plain, ASan, UBSan, and normal Release/Debug validation.
- Under TSan, mark the fence-dependent F3 body as unsupported/SKIP, or split the target so only a pure release/acquire atomic publication control runs under TSan.
- Do not add extra release/acquire operations to the fence forms just to silence TSan; that changes the experiment and hides the fence lesson.

## scheduling global new/delete

Symptom: `runtime_scheduling_test` cannot link under TSan due multiple definitions of global allocation operators.

Minimal reproduction: `agent-run-20260910-1658/global_new_probe.cpp`.

Results:

- `status-build-global-new-tsan.txt=1`
- `build-global-new-tsan.txt` reports `operator new(unsigned long)` and `operator delete(void*)` first defined in `libclang_rt.tsan_cxx-x86_64.a(tsan_new_delete.cpp.o)`.
- `status-global-new-plain.txt=0`
- `status-local-alloc-tsan.txt=0`

Root cause classification: code/test design conflicts with TSan runtime interceptors. The test TU defines replacement global `operator new/delete`; LLVM TSan C++ runtime also defines/intercepts those operators.

External TSan basis: LLVM `compiler-rt/lib/tsan/rtl/tsan_new_delete.cpp` is the TSan runtime source for C++ `operator new/delete` interceptors.

Minimal fix recommendation:

- Split scheduling coverage:
  - Keep the concurrent scheduling/shutdown/cross-pool behavior in a TSan-safe runtime target without replacement global allocation operators.
  - Keep allocation-failure injection in a non-TSan target, or refactor only the injection path to use a local injectable allocation hook/resource in course code.
- Do not use linker `allow-multiple-definition`; it would choose one allocator definition and can hide TSan reports.
- Do not use suppressions; this is a link-time ownership conflict, not a runtime report to suppress.

## Evidence Notes

- Primary ext4 run evidence: `agent-run-20260910-1658/*.txt`, `*.json`, `*.cpp`.
- Fix validation evidence: `fix-run-20260910-1715/*.txt`, `*.json`.
- Misplaced recovery evidence: `agent-recovery-20260910-1646/*`. This first batch accidentally ran from `/mnt/f/CPPTrain/LearnCPP` because inline Bash variables were expanded before reaching WSL. It was preserved for audit only and should not be cited as ext4 evidence.
- ELF artifacts from the recovery run were moved to ignored `C08_Concurrency/exercises/build/c08-tsan-diagnosis-artifacts/agent-recovery-20260910-1646`.
- No `.log` files were produced.
- No global system settings, ASLR, sysctl, or suppressions were changed.

## Implemented Minimal Fix

Changed files:

- `C08_Concurrency/exercises/B3_call_once/solution.cpp`
  - Keeps the original exception-retry `std::call_once` path for non-TSan runs.
  - Under verified Clang 18 + libstdc++ 13 + TSan only, runs the successful concurrent publication and local-static checks, prints `SKIP` for exception retry, and exits `77`.
  - Other toolchain combinations run the original full check so future implementations expose their own result instead of inheriting this skip.
- `C08_Concurrency/exercises/F3_seqcst_fence/solution.cpp`
  - Keeps store-buffering checks, including the SC fence litmus.
  - Under verified Clang 18 + libstdc++ 13 + TSan only, skips only `fence_publication`, prints `SKIP`, and exits `77`.
  - Other toolchain combinations run the original full check so future implementations expose their own result instead of inheriting this skip.
- `C08_Concurrency/exercises/runtime_tests/scheduling_test.cpp`
  - Keeps concurrent scheduling, cross-pool submission, and submit/shutdown race checks.
  - Removes global `operator new/delete` replacement from the TSan-running scheduling target.
- `C08_Concurrency/exercises/runtime_tests/scheduling_allocation_test.cpp`
  - Moves allocation-failure injection into its own runtime target.
  - Under verified Clang 18 + libstdc++ 13 + TSan only, avoids defining replacement global allocation operators, prints precise `SKIP`, and exits `77`.
  - Other toolchain combinations define the original global allocation operators and expose their own link/runtime result.

No shared `StudySetup.cmake` or `runtime_tests/CMakeLists.txt` changes were made. The existing `runtime_tests/*.cpp` glob discovers `scheduling_allocation_test.cpp` after configure or glob recheck.

Validation:

- `fix-run-20260910-1715/plain-ctest-targets.txt`: `B3_call_once_reference`, `F3_seqcst_fence_reference`, `runtime_scheduling_allocation_test`, and `runtime_scheduling_test` all passed in Release non-TSan.
- `fix-run-20260910-1715/tsan-ctest-targets.txt`: same four tests passed as a CTest set under Clang TSan; `runtime_scheduling_allocation_test` is reported as skipped through exit `77`.
- `fix-run-20260910-1715/tsan-direct-B3.txt`: confirms B3 TSan runs successful publication/local static and prints the exception-retry `PARTIAL_SKIP`.
- `fix-run-20260910-1715/tsan-direct-F3.txt`: confirms F3 TSan runs store-buffering and prints the fence-publication `PARTIAL_SKIP`.
- `fix-run-20260910-1715/tsan-direct-scheduling-allocation.txt`: confirms the allocation-injection target prints the precise TSan `SKIP`.
- `fix-run-20260910-1725/linux-asan-ctest-targets.txt`: the same four targets passed under Linux Clang ASan/UBSan.
- `fix-run-20260910-1730-win/windows-ctest-targets.txt`: the same four targets passed under Windows MSVC Release.
- `git diff --check` on the changed source/report files found no whitespace errors; it only reported existing CRLF conversion warnings.
- `r3-run-20260910-1745/plain-ctest-targets.txt`: the same four targets passed in Linux non-TSan after narrowing the skip guard.
- `r3-run-20260910-1745/tsan-ctest-targets.txt`: verified Clang 18 + libstdc++ 13 + TSan produced one pass (`runtime_scheduling_test`), three skips (`B3_call_once_reference`, `F3_seqcst_fence_reference`, `runtime_scheduling_allocation_test`), and zero failures.
- `r3-run-20260910-1745/tsan-direct-B3.txt`: confirms B3 executed successful publication/local static before returning skip `77`.
- `r3-run-20260910-1745/tsan-direct-F3.txt`: confirms F3 executed store-buffering checks before returning skip `77`.
- `r3-run-20260910-1745/tsan-direct-scheduling-allocation.txt`: confirms the allocation-injection target returned skip `77` without defining replacement global allocation operators.

Post-fix source hashes:

- `B3_call_once/solution.cpp`: `2365f8cbeee927d961e21c22c2de26878793699c372554fde8798c23bdb53f32`
- `F3_seqcst_fence/solution.cpp`: `ba8a8a08f702794209f65ccc3e4d65b29787011dd8d50e9260c7c4d2766b5de6`
- `runtime_tests/scheduling_test.cpp`: `e47448fde11bfe668cb5f34f0bc8aafe8f0bf60c19c52bdfee5e7dc85ffcdf87`
- `runtime_tests/scheduling_allocation_test.cpp`: `0616da974dab18fb33dbce9df2a4574856eda51f2812cd036290e1ad6049f5e5`

R3 narrowed source hashes:

- `B3_call_once/solution.cpp`: `531a4ba8055ab90e26490f7a7db3565a80cccb8f1f2f0dcbbc0ff51ce6545c5c`
- `F3_seqcst_fence/solution.cpp`: `6ad71d1b4d36cc1e787001aa3b40156f1b3614c01b39075bd7102f000e350a52`
- `runtime_tests/scheduling_test.cpp`: `e47448fde11bfe668cb5f34f0bc8aafe8f0bf60c19c52bdfee5e7dc85ffcdf87`
- `runtime_tests/scheduling_allocation_test.cpp`: `da499b5d6d5bdf7e0ced3701ba22999058280552e00e16825306cf6d1df19059`
