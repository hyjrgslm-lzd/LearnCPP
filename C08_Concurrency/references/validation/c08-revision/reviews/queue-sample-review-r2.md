# C08 queue sample review r2 2026-09-10

Review role: non-author r2 sample-chapter verification.

Relation to r1: this review verifies closure of `queue-sample-review.md` without overwriting that report.

Reviewed files and artifacts:

- `C08_Concurrency/exercises/include/concurrency_study/queue_baseline.hpp`
  - SHA256: `64CC8B2BC53892A9B1191DD4C92A68C9ACC261D001B5F5163BAA5CF3A3CF8FC5`
- `C08_Concurrency/exercises/include/concurrency_study/queue_versions.hpp`
  - SHA256: `70154D0A4DC60924A12CB7CDD66EED10F0D6665F02DC88553417702F56BC8D21`
- `C08_Concurrency/exercises/include/concurrency_study/queue_linked.hpp`
  - SHA256: `10DEE07B7CB99F4E388B26C8C6A5B4CBD83D74EFE7F8186B2C9E59FC808B8F6F`
- `C08_Concurrency/exercises/benchmarks/queue_diagnostics.cpp`
  - SHA256: `5FDF75AAA02CE2AB30848CE84ACF2662200BE17FD621C6E4B29F5A059DB83B60`
- `C08_Concurrency/exercises/benchmarks/CMakeLists.txt`
  - SHA256: `5170F77A5F048708A5904EF82113459075456C3B043A3E7A5FA94EB1795D2DB3`
- `C08_Concurrency/topics/performance/c08-revision-queue-evidence.md`
  - SHA256: `238AFD05350D2B7E2074FB0FFEBB1466211F784DDAB489E7D14910CF91077E63`
- `C08_Concurrency/references/measurements/c08-revision-queue-evidence/README.md`
  - SHA256: `69548E604D8A7E3311C290941B5F3BD6AC4881CA47D8192B79541D4A9E78EA91`
- `C08_Concurrency/references/measurements/c08-revision-queue-evidence/queue_diagnostics-r2.csv`
  - SHA256: `E59966640D46CCAA4C091356DE376420BC1BC7B9026716B969F1725519692E84`
- r1 baseline report: `C08_Concurrency/references/validation/c08-revision/reviews/queue-sample-review.md`
  - SHA256: `C566D405F8F7DE71DCC6C7143BC48B439A2107F20CE4E34E6272830517B21A5E`

## Verdict

APPROVE for sample-chapter diagnostic and reasoning quality.

Do not approve this as current-head formal performance sampling. The reviewed text still correctly says no formal samples were collected in this slice, and the formal queue sampling window remains pending.

## Closure of r1 findings

1. Closed - preallocation and SPSC cached now have minimal named-operation counters.

   r1 required a positive evidence hook for preallocation and cached SPSC beyond public API call counts. r2 adds diagnostic-only counters under `CS_QUEUE_DIAGNOSTICS`. `queue_baseline.hpp:14` and `queue_versions.hpp:19` define the diagnostic support only when the macro is present, and the fallback functions at `queue_baseline.hpp:49`/`queue_versions.hpp:54` are no-op by default. `queue_versions.hpp:116`, `queue_versions.hpp:121`, `queue_versions.hpp:132`, and `queue_versions.hpp:137` count SPSC remote-index load sites. `queue_diagnostics.cpp:108` through `queue_diagnostics.cpp:118` isolates single-thread hot-region `operator new` counts after queue construction.

   Author r2 CSV records `mutex` hot `operator new` = `2`, `ring` = `0`, `spsc` remote loads = `21735`, and `spsc-cached` remote loads = `3405`. This closes the r1 evidence gap for teaching the named operations. The count is bounded to the current diagnostic binary and current implementation; it is not a claim about all allocators, all `T`, aligned allocation, or cache misses.

2. Closed - batch public calls now map to instrumented mutex entries for the current code.

   r1 rejected treating public call count as lock count without a direct mapping. r2 records mutex acquisition after `std::lock_guard` construction at `queue_baseline.hpp:70`, `queue_baseline.hpp:78`, `queue_versions.hpp:71`, and `queue_versions.hpp:81`. `queue_diagnostics-r2.csv:4` shows `batch8` mutex acquisitions = `3785`, matching public push/pop batch calls `1667 + 2118`. `topics/performance/c08-revision-queue-evidence.md:60` explicitly states this supports the current source mapping, not wait time.

3. Closed - the diagnostic target is reproducible through CMake.

   r1 noted that `queue_diagnostics.cpp` was not in the build graph. `benchmarks/CMakeLists.txt:15` adds the `queue_diagnostics` executable, `benchmarks/CMakeLists.txt:17` defines `CS_QUEUE_DIAGNOSTICS=1` only for that target, and `benchmarks/CMakeLists.txt:19` through `benchmarks/CMakeLists.txt:20` add `diagnostic_queue_counts` with `CS_TEST_TIMEOUT`.

## Fresh reviewer evidence

Commands run from `F:\CPPTrain\LearnCPP`:

```text
cmake --build build/c08-evolution-author --config Release --target queue_diagnostics
```

Result: PASS. MSBuild generated `build/c08-evolution-author/benchmarks/Release/queue_diagnostics.exe`.

After the later GCC-compatibility overload patch, this command re-ran CMake generation and still passed. The regenerate output also showed `CS_HAS_STD_THREAD_ATTRIBUTES` and `CS_HAS_STD_HAZARD_POINTER_BATCH` probes as `DISABLED`, and `U01_async_logging disabled; CONCURRENCY_STUDY_ENABLE_SPDLOG=OFF`, preserving the default-off boundary.

```text
ctest --test-dir build/c08-evolution-author -C Release -R '^diagnostic_queue_counts$' --output-on-failure
```

Result: PASS before and after the GCC-compatibility overload patch. Final CTest output: `1/1 Test #67: diagnostic_queue_counts ..........   Passed    0.49 sec`; total real time `0.54 sec`.

Direct reviewer run:

```text
build/c08-evolution-author/benchmarks/Release/queue_diagnostics.exe
```

Result: PASS. Output completed all five variants with `completed=10003`, `push_items=10003`, and `pop_items=10003`.

Observed reviewer counts:

| variant | mutex acquisitions | SPSC remote loads | hot `operator new` |
| --- | ---: | ---: | ---: |
| mutex | 23838 | 0 | 2 |
| ring | 23930 | 0 | 0 |
| batch8 | 4072 | 0 | 0 |
| spsc | 0 | 20330 | 0 |
| spsc-cached | 0 | 2287 | 0 |

These values differ from the author r2 CSV because concurrent retry counts depend on scheduling. The supported claim is the controlled observation under this diagnostic: batch uses fewer instrumented mutex entries than single-item ring for the same completed work, cached SPSC performs fewer remote-index load-site visits than ordinary SPSC, and preallocated ring has zero counted hot-region ordinary `operator new` calls for this `size_t` loop. The result must not be written as a stable ratio, a universal speedup, a cache-miss claim, or a formal benchmark result.

## Default-off and side-effect boundary

- Ordinary timed benchmark targets remain the `*_bench.cpp` glob in `benchmarks/CMakeLists.txt:5`; `CS_QUEUE_DIAGNOSTICS=1` is attached only to `queue_diagnostics` at `benchmarks/CMakeLists.txt:17`.
- Header changes do compile into ordinary targets, but when `CS_QUEUE_DIAGNOSTICS` is absent they define only no-op functions and no diagnostic atomic object. This preserves ordinary build behavior except for harmless inline empty calls at the instrumented source locations.
- `queue_diagnostics.cpp:30` through `queue_diagnostics.cpp:35` define global ordinary `operator new`/`new[]` and delete overloads in the diagnostic executable only. Counting is enabled only between `queue_diagnostics.cpp:111` and `queue_diagnostics.cpp:117`, after queue construction and before the single-thread hot loop ends. This is adequate for the current `size_t` queue-storage teaching observation. It does not count aligned allocation forms and should not be generalized beyond this diagnostic.

## GCC compatibility patch review

The later compatibility change removes default lambda template parameters from hook overloads while preserving explicit Hook forms. `queue_versions.hpp:192` now provides the single-argument `try_push` overload that forwards to `try_push(value, []() noexcept {})`; the templated `try_push(value, Hook)` remains available for tests that need the reservation hook. `queue_linked.hpp:99` does the same for `try_pop(value)`, forwarding to `try_pop(value, []() noexcept {})`, while `queue_linked.hpp:101` keeps the explicit removed-hook overload. This is an overload-resolution and GCC-compatibility repair; it does not change the queue algorithm or diagnostic counters. Linux build verification is delegated to the Linux lane and is not claimed by this review.

## Remaining boundary

Formal queue sampling remains unapproved. `references/measurements/c08-revision-queue-evidence/README.md:5` still says no formal samples were collected in this slice, and `topics/performance/c08-revision-queue-evidence.md:64` keeps formal sampling behind the main-thread exclusive window. This review approves only the r2 diagnostic evidence and the teaching logic around it.

Stop condition: r1 REVISE items are closed for diagnostic/sample-chapter approval; no author files were edited.
