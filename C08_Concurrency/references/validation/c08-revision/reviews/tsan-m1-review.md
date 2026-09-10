# C08 M1 TSan Boundary Independent Review

Date: 2026-09-10

Role: non-author code/spec/security review. Previous TSan review remains unchanged; this report covers only the M1 exception-message boundary.

## Verdict

APPROVE

No blocking issue found. The M1 change is a narrow, evidence-backed TSan boundary, not a workaround that masks a pool defect.

## Files Reviewed

- `C08_Concurrency/exercises/M1_work_stealing_pool/solution.cpp`
- `C08_Concurrency/exercises/M1_work_stealing_pool/README.md`
- `C08_Concurrency/references/validation/c08-revision/tsan-diagnosis/m1-20260910-1815/diagnosis.md`
- `C08_Concurrency/references/validation/c08-revision/tsan-diagnosis/m1-20260910-1815/standard_last_owner_probe.cpp`
- `C08_Concurrency/references/validation/c08-revision/tsan-diagnosis/m1-20260910-1815/standard-last-owner-status-summary.json`
- Supporting status summaries and temporary-variant evidence under `m1-20260910-1815/`

## Spec Compliance

PASS.

- `solution.cpp` changes only the exact-message assertion under verified Clang 18 + libstdc++ 13 + TSan.
- The TSan path still catches `std::runtime_error` by type and continues all M1 pool checks: invalid worker count, scheduling variants, recursive wait, move-only callable, exception survival, shutdown drain, submit rejection, and stealing.
- The TSan path returns `77` after those checks and prints `PARTIAL_SKIP`, so it is not reported as a fake PASS.
- Non-TSan and other compiler/library combinations keep the full `std::string_view(e.what()) == "task-error"` check.
- No `join-before-get`, no suppression, no sanitizer disable, and no extra synchronization was added to make TSan quiet.
- README explains the boundary: standard exception lifetime remains valid; this evidence does not classify the pool as UAF; normal exact-message coverage remains required outside this verified TSan combination.

## Proof Review

The corrected pure standard probe is ownership-isomorphic enough to support the classification.

- `standard_last_owner_probe.cpp` moves `std::packaged_task<int()>` directly into the worker lambda. Main holds only the `future`; it does not retain the provider/task owner.
- Worker executes the task, waits on a relaxed atomic phase gate, then destroys the provider on the worker thread. The gate controls timing only and does not publish exception payload state through a stronger happens-before edge.
- Main calls `future.get()`, catches `std::runtime_error`, reads `e.what()` twice, then opens the relaxed gate and joins.
- `standard-last-owner-status-summary.json` reports TSan exception-worker-last-owner first run plus 10 repeats all `66`; TSan value worker-last-owner and exception join-before-get controls are `0`; plain and ASan controls are `0`.
- The earlier packaged-task and queue probes are correctly retained as negative evidence because they did not force worker-side last-owner cleanup.
- The M1 type-only temporary variant changed only the `e.what()` read to type-only catch and ran 20/20 TSan passes, while pool logic and timing stayed intact.

## Current Rerun

I built current M1 from a fresh WSL `/tmp` ext4 source snapshot:

- Environment: `Ubuntu clang version 18.1.3 (1ubuntu1)`, `g++ 13.3.0`, `cmake 3.28.3`, `ninja 1.11.1`.
- Source hash for current `M1_work_stealing_pool/solution.cpp`: `3ac94d97f2bff274ff311e16243566d72887bf59eb116bc0925b521961156535`.
- Source hash for current `include/concurrency_study/work_stealing_pool.hpp`: `2ec9a625dfeb839c452d381f13cf1600ae9887314247e208820f4560ebc15f33`.
- TSan build target `M1_work_stealing_pool_reference`: build exited `0`.
- CTest for `M1_work_stealing_pool_reference`: skipped via CTest `SKIP_RETURN_CODE 77`, zero failures.
- Five direct runs: all printed `M1 reference OK: variants, recursion, errors, drain, stealing`, all printed the `PARTIAL_SKIP` message, and all had WSL-internal `RUN_RC=77`.
- Those five direct outputs contained no `WARNING: ThreadSanitizer`, `data race`, or `SUMMARY: ThreadSanitizer` text.

Windows plain rerun was assigned to the main thread, so I did not duplicate it here.

## Source Basis

Sufficient.

- C++ draft `[futures.unique.future]` says `future::get()` waits until ready, retrieves the result, releases the shared state, and throws the stored exception if present.
- C++ draft `[propagation]` says `exception_ptr` can refer to an exception object, implementations may reference-count it, and `exception_ptr` bookkeeping must not introduce races on the referred exception object.
- The Google ThreadSanitizer C++ manual states that C++ exceptions are unsupported by TSan.

## Root-Cause Guard

PASS.

This is not a masking fallback. The patch removes only the unsupported TSan text-inspection assertion on an exact proven toolchain boundary and keeps all behavior checks running. Passing behavior does not come from a new alternate pool path, suppression file, sanitizer opt-out, extra join, or changed scheduler protocol.

## Static Checks

- Pattern scan found no hardcoded secrets, suppressions, `TSAN_OPTIONS`, `no_sanitize`, `allow-multiple-definition`, empty catches, or broad fallback branches in the reviewed scope.
- `git diff --check` on M1 source and README found no whitespace errors; only the existing CRLF conversion warning appeared.
- `lsp_diagnostics` and `ast_grep_search` were unavailable in this session. I substituted compile/CTest, direct process reruns, `rg` pattern scans, and `git diff --check`.
- `clangd` is unavailable on Windows PATH and inside the WSL distro used for this review.

## Issues

CRITICAL: 0

HIGH: 0

MEDIUM: 0

LOW: 0

Recommendation: APPROVE.
