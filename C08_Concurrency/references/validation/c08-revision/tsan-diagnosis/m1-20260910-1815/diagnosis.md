# M1 Work-Stealing Pool TSan Diagnosis 2026-09-10

Scope: diagnostic only. No tracked course source was modified.

Environment:

- WSL distro: `LearnCPP-C08-Ubuntu-24.04`
- Probe work dir: `/root/learncpp-c08-builds/m1-tsan-diagnosis-20260910-1815`
- Final source snapshot: `/root/learncpp-c08-src/final-20260910-173010-d5356576`
- Final TSan build: `/root/learncpp-c08-builds/final-20260910-173010-d5356576/clang-tsan`
- `pwd` was recorded before probe generation: `pwd.txt`

## Symptom

`M1_work_stealing_pool_reference` fails intermittently under Clang 18 + libstdc++ 13 + TSan.

Primary report:

- `linux-final/final-20260910-173010-d5356576/clang-tsan-ctest.txt`
- `M1_work_stealing_pool/solution.cpp:40`: main thread catches `std::runtime_error` from `future.get()` and reads `std::string_view(e.what())`.
- `work_stealing_pool.hpp:69`: worker executes the packaged task wrapper.
- `work_stealing_pool.hpp:79`: the worker's local `std::function<void()> work` is destroyed as `run_one` returns; that drops the captured `std::shared_ptr<std::packaged_task<...>>`.
- TSan reports concurrent writes/free in `std::__future_base::_Result<int>::_M_destroy()` / `std::__exception_ptr::exception_ptr::_M_release()` against the main thread's `e.what()` read.

## Reproduction

Existing final M1 target was rerun five times:

- `status-existing-M1-tsan-01.txt=66`
- `status-existing-M1-tsan-02.txt=0`
- `status-existing-M1-tsan-03.txt=0`
- `status-existing-M1-tsan-04.txt=66`
- `status-existing-M1-tsan-05.txt=0`

This confirms the failure is intermittent and matches the full CTest failure.

## Minimal Probes

### Pure packaged_task/thread/future

Probe: `packaged_task_exception_probe.cpp`

Shape:

- `std::shared_ptr<std::packaged_task<int()>>`
- `std::function<void()>` captures that task
- `std::thread` executes the function
- main calls `future.get()`, catches `std::runtime_error`, reads `e.what()`, then joins

Results:

- TSan exception mode: `status-tsan-exception.txt=0`
- TSan exception repeat runs 01-10: all `0`
- TSan normal value control: `status-tsan-value.txt=0`
- TSan join-before-get control: `status-tsan-join-before-get.txt=0`
- ASan exception/value/join-before-get controls: all `0`
- Plain exception/value/join-before-get controls: all `0`

Conclusion: this first standard-library probe did not reproduce the report, but it was not ownership-isomorphic to the M1 failure. The probe kept an owning `shared_ptr<packaged_task>` in the main function until after the worker joined, so the worker did not own the last task-state reference.

### Single-worker queue without course pool

Probe: `single_worker_queue_exception_probe.cpp`

Shape:

- One worker thread.
- Mutex/condition_variable queue of `std::function<void()>`.
- `submit` uses the same `shared_ptr<packaged_task>` wrapper pattern.
- main calls `future.get()`, catches `std::runtime_error`, reads `e.what()`, then joins.

Results:

- TSan exception mode: `status-queue-tsan-exception.txt=0`
- TSan exception repeat runs 01-10: all `0`
- TSan normal value control: `status-queue-tsan-value.txt=0`
- TSan join-before-what control: `status-queue-tsan-join-before-what.txt=0`
- ASan exception/value/join-before-what controls: all `0`

Conclusion: the queue probe did not reproduce the report, but it was still too weak as a final discriminator. Fast worker completion could leave the effective last cleanup timing different from M1, and the probe did not force worker-side last-owner destruction during the main thread's caught-exception `what()` window.

### Pure packaged_task with worker-side last owner

Probe: `standard_last_owner_probe.cpp`

Shape:

- `std::packaged_task<int()>` is moved directly into the worker-thread lambda; main keeps no provider/task owner.
- Worker executes the task, then spins on a relaxed atomic gate.
- Main calls `future.get()`, catches `std::runtime_error`, reads `e.what()` twice, then stores the relaxed gate and joins.
- The gate controls phase only; it does not publish the exception payload or provide a happens-before edge for the exception object.
- The task provider is destroyed on the worker thread when the thread lambda exits.

Results:

- TSan exception worker-last-owner mode: `status-standard-last-owner-tsan-exception-worker-last-owner.txt=66`
- TSan exception repeat runs 01-10: all `66`
- TSan normal value worker-last-owner control: `status-standard-last-owner-tsan-value-worker-last-owner.txt=0`
- TSan exception join-before-get control: `status-standard-last-owner-tsan-exception-join-before-get.txt=0`
- ASan exception/value/join-before-get controls: all `0`
- Plain exception/value/join-before-get controls: all `0`

The TSan report is isomorphic to M1:

- Worker thread writes/frees through `std::runtime_error::~runtime_error()`, `std::__future_base::_Result<int>::_M_destroy()`, and `std::packaged_task<int()>::~packaged_task()`.
- Main thread reads the exception message through `std::string_view(e.what())`.
- The program still prints `ok exception-worker-last-owner ...`, then TSan exits `66`.

Conclusion: a course-free standard-library example reproduces the same class of report when the provider/task last owner is destroyed on the worker thread while the main thread has just read the caught exception's message.

### M1 type-only temporary variant

Temporary diff: `m1-type-only-variant.diff`

Only changed `M1_work_stealing_pool/solution.cpp:40` from:

```cpp
try { error.get(); } catch (const std::runtime_error& e) { caught = std::string_view(e.what()) == "task-error"; }
```

to:

```cpp
try { error.get(); } catch (const std::runtime_error&) { caught = true; }
```

The pool implementation and all scheduling/recursive/drain/stealing timing stayed unchanged.

Results:

- TSan build: `status-m1-type-only-build.txt=0`
- TSan runs 01-20: all `0`

Conclusion: removing only the `e.what()` text read from the caught exception makes the M1 TSan failure disappear across 20 independent process runs. This is consistent with the corrected standard last-owner probe: type propagation is observable under TSan, but caught-exception message inspection is not reliable for this toolchain/timing.

## Standard and Tooling Basis

C++ draft `[futures.unique.future]` says `future::get()` waits until the shared state is ready, retrieves the result, releases the shared state, and throws the stored exception if one exists.

C++ draft `[propagation]` says:

- `exception_ptr` can refer to an exception object.
- Implementations can use reference counting for `exception_ptr`.
- operations on `exception_ptr` objects must not introduce data races by accessing the referred exception object.
- `rethrow_exception` may throw the referred exception object or a copy.
- changes in the number of `exception_ptr` objects referring to an exception do not introduce a data race.

LLVM's 2026 Clang documentation patch records that C++ exception paths are unsupported under TSan and can produce unreliable results on thrown-exception paths.

## Classification

This is best classified as a verified Clang 18 + libstdc++ 13 + TSan exception-path limitation, not a pool use-after-free:

- Plain and ASan probes pass.
- The original pure standard packaged_task/future and queue probes did not reproduce because they did not force worker-side last-owner cleanup.
- The corrected pure standard packaged_task worker-last-owner probe reproduces the same report 11/11 times under TSan and passes under ASan/plain.
- Value and join-before-get controls pass under TSan.
- The original pool failure disappears when only `e.what()` text inspection is removed.
- The TSan stack enters uninstrumented/libstdc++ exception internals: `std::__exception_ptr::exception_ptr::_M_release()` and `std::runtime_error::~runtime_error()`.

Most supported classification: Clang 18 + libstdc++ 13 + TSan reports a race for a standard packaged-task exception path when worker-side provider cleanup overlaps, without a TSan-visible happens-before edge, with main-side caught-exception message access. The pool protocol still needs ordinary non-TSan/ASan coverage for the exact exception message.

## Minimal Fix Recommendation

Do not add `join-before-get` or other synchronization to silence TSan; that changes the pool lifecycle being taught and broke the follow-up "pool survives user exception" check in the temporary variant.

Recommended source treatment:

- Keep full `e.what() == "task-error"` check in non-TSan and ASan runs.
- For verified `Clang 18 + libstdc++ 13 + TSan` only, catch `std::runtime_error` by type, set `caught = true`, continue the remaining M1 checks, print a precise message that exception message text is unverified under this TSan combination, and return `77` at the end so CTest/JUnit records M1 as SKIP rather than PASS.
- Other compiler/library/TSan combinations should run the original full check and expose their own result.

If root wants TSan PASS credit for the supported portions, split M1 into a TSan-supported target and a non-TSan exact-message target. Without split, returning `77` is the honest single-target status.

## Evidence Files

- `packaged_task_exception_probe.cpp`
- `single_worker_queue_exception_probe.cpp`
- `status-summary.json`
- `queue-status-summary.json`
- `m1-type-only-status-summary.txt`
- `m1-type-only-variant.diff`
- `run_m1_probe_20260910_1815.sh`
- `run_m1_queue_probe_20260910_1828.sh`
- `run_m1_type_only_variant_20260910_1848.sh`
- `standard_last_owner_probe.cpp`
- `standard-last-owner-status-summary.json`
- `run_standard_last_owner_probe_20260910_1905.sh`

No `.log` files were produced. No global system settings, ASLR, sysctl, suppressions, or tracked course source files were changed.
