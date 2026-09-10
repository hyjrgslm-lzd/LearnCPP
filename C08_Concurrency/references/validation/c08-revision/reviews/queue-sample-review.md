# C08 queue sample review 2026-09-10

Review role: non-author sample-chapter review.

Reviewed scope:

- `C08_Concurrency/topics/queues/01-mutex-baseline.md`
  - SHA256: `12B550B06A6FBF554AC27A97C72AC57A914580A4E3E3C7BB180337C35052275A`
- `C08_Concurrency/topics/queues/02-bounded-and-batch.md`
  - SHA256: `67F9A134E4C37201EDB8F956BD4AEED9377676B6D946D66FE08917B326DAF691`
- `C08_Concurrency/topics/queues/03-spsc.md`
  - SHA256: `BACF6FFAF96FB4B3D4FFFC574BD65F11674F76E937122319BAEEDE3CE893535F`
- `C08_Concurrency/topics/queues/08-validation-and-benchmark.md`
  - SHA256: `00AAC280AF282D08E23663D5D7496E2873B76AAD3DC708006081CBEDF219C3CA`
- `C08_Concurrency/topics/queues/VALIDATION.md`
  - SHA256: `CD8CC7CEE531AD153653C6465825845881F9828C874C7E68989ED42A9D6146B0`
- `C08_Concurrency/topics/performance/c08-revision-queue-evidence.md`
  - SHA256: `DDA87658D024AD1EFDBCD58815BFFF0CB7EF83BB049974D95024015B091142ED`
- `C08_Concurrency/exercises/benchmarks/queue_diagnostics.cpp`
  - SHA256: `787E6DD22AF404F09B555740AA37ED108701D2E0FE08C7D584033ABFD0156E2B`
- `C08_Concurrency/references/measurements/c08-revision-queue-evidence/README.md`
  - SHA256: `789FBA7112579123BA68011EA4D983A0EE85E41151735F8E6F17AD1636F18C86`
- `C08_Concurrency/references/measurements/c08-revision-queue-evidence/queue_diagnostics.csv`
  - SHA256: `6FAAD31251ED09FE391341165686A032E6D5C9F3DDB4E1BEFAB31B68D6FE4186`

## Verdict

REVISE.

The current slice is honest about its measurement boundary and is usable as a local batch-call diagnostic, but it is not yet strong enough to approve as the quality standard for other authors under the new "locate first, then evolve" rule. The evidence proves completed transfers and public queue API call counts. It does not prove allocation pressure, actual mutex acquisition/wait behavior, internal atomic refresh counts, cache misses, or a current-head formal benchmark. The reviewed text mostly states those limits correctly; the remaining gap is that the sample still presents preallocation and SPSC cached evolution without a minimal positive evidence hook equivalent to the batch-call counter.

## Findings

1. P1 - Preallocation and SPSC cached evolution are still protocol/code-inspection claims, not measured cost-location evidence.

   Evidence boundary: `queue_diagnostics.cpp:29`, `queue_diagnostics.cpp:39`, `queue_diagnostics.cpp:61`, and `queue_diagnostics.cpp:69` count wrapper-level calls around `try_push`, `try_pop`, `push_batch`, and `pop_batch`. `topics/performance/c08-revision-queue-evidence.md:31` correctly says the counters are outside public API calls and cannot prove mutex kernel calls, CAS failures, cache misses, or scheduling. `03-spsc.md:70` correctly says reduced remote-index reads are inferred from the protocol, not measured cache-miss evidence. `02-bounded-and-batch.md:63` only covers public call granularity. This is enough for batch interface granularity, but not enough to make preallocation or cached SPSC the exemplar for evidence-first evolution.

   Minimal fix: either narrow this sample's approved standard to "batch public API call-count evidence only" and mark preallocation/SPSC cached as code-protocol teaching until separately instrumented, or add a small diagnostic-only evidence hook for those two claims. For preallocation, record a controlled allocation/storage event count or an explicit no-hot-path-allocation proof tied to the implementation under the same driver. For SPSC cached, record diagnostic-only remote-index load/refresh counts or a profiler/hardware-counter note with perturbation separated from timing.

2. P1 - The sample should not let "reduced public calls" stand in for "reduced lock count" without a direct mapping.

   Evidence boundary: `02-bounded-and-batch.md:3` frames batch as a hypothesis about reducing lock count, while `02-bounded-and-batch.md:63` and `topics/performance/c08-revision-queue-evidence.md:41` only prove public call count reduction. In the current `mutex_ring` implementation a batch public call may correspond to one critical-section entry, but that relationship is not measured by the wrapper and is not recorded here as a source-level invariant. The CSV line `queue_diagnostics.csv:4` supports `batch8` public push/pop calls of `1570/2188`, not lock wait time or lock acquisition telemetry.

   Minimal fix: keep the student-facing claim as "public batch-call count / per-call critical-section opportunity is reduced" unless a source citation or internal counter explicitly binds one batch call to one mutex acquisition. If the wording says "lock count" in the quality-standard checklist, require either implementation citation plus limitation text, or rename the measured quantity to public API call count.

3. P2 - The run artifact is acceptable as an author self-check, but it is not independently reproducible from the current CMake graph.

   Evidence boundary: `references/measurements/c08-revision-queue-evidence/README.md:5` says no formal samples were collected in this slice, and `README.md:7` says `queue_diagnostics.csv` is only an author self-check. `exercises/benchmarks/CMakeLists.txt` only glob-builds `*_bench.cpp`, while this driver is named `queue_diagnostics.cpp`; it is not an ordinary benchmark target. My review shell also had no `cl` in PATH, so I could not independently rerun the documented `cl` command from `topics/performance/c08-revision-queue-evidence.md:37`.

   Minimal fix: keep the current CSV as self-check evidence, but do not make this driver a required author standard until the command is either made reproducible through the normal benchmark build graph or the review template explicitly accepts the manual Developer Command Prompt prerequisite. Formal sampling should still wait for the approved measurement window.

## Approved parts

- The corrected final-sample backlink is acceptable. `08-validation-and-benchmark.md:71` and `VALIDATION.md:143` no longer imply missing historical queue samples; they distinguish `final-20260908` historical samples from current-head revision evidence.
- The new evidence document correctly avoids claiming current formal benchmark results. `references/measurements/c08-revision-queue-evidence/README.md:5` and `topics/performance/c08-revision-queue-evidence.md:47` reserve formal sampling for the later exclusive window.
- The batch diagnostic supports the narrow claim that public batch interface calls are fewer while completed work remains equal. `queue_diagnostics.csv:2` through `queue_diagnostics.csv:6` show all variants completed `10003` pushed and popped items, and `batch8` used fewer public push/pop calls than the single-item MPMC-like runs.

## Review validation

- Read-only diff/content inspection of the scoped topic files, `VALIDATION.md`, evidence note, driver, and measurement directory.
- SHA256 bound above with `Get-FileHash`.
- Independent driver run was attempted but not completed: `cl` was not available in the active PowerShell PATH, and `queue_diagnostics.cpp` is not included by the current `*_bench.cpp` CMake glob.

Stop condition: report written only to this review file; no author content edited.
