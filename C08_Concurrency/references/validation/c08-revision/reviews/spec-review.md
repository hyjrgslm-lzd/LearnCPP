# C08 revision spec review 2026-09-10

Review role: non-author specification review.

Reviewed files:

- `C08_Concurrency/references/revision-plan-20260910.md`
  - SHA256: `D2860324001BED24F7001D0578E0B8C477BE886F587F8B3C82713F90147F1E2B`
- `C08_Concurrency/references/revision-audit-20260910.md`
  - SHA256: `D28297CDA547EB5E2CFA7DF5D4E9455FCDF9A10A9E9DD6E1CE420C7EFC8CFE45`

## Verdict

APPROVE.

The plan accurately carries the approved scope into three new teaching chains: `U01_async_logging`, `F01_thread_attributes`, and `F02_hazard_pointer_batches`. It also preserves the required boundaries for C++26 native probes, C++29 draft status, default-off spdlog/fmt dependency wiring, WSL/Linux validation, Student/Reference separation, and non-author review before completion.

The audit accurately records the known gaps and the corrected queue evidence boundary. In particular, A07 says the issue is "专题阶段记录与最终记录衔接不清" while recognizing that `final-20260908` contains five queue measurement groups. This matches the current evidence: the problem is a stage-record/final-report backlink and version-binding gap, not absence of final queue samples.

## Checked Evidence

- `revision-plan-20260910.md` declares user-approved implementation scope, no Git/external-production/credential action, and no overwrite of historical measurements.
- `revision-plan-20260910.md` requires C05 async logging to be completed in C08 with local pool/logger/sink, overflow policies, flush/shutdown semantics, Student/Reference/good/bad, and default-off fixed dependencies.
- `revision-plan-20260910.md` separates C++29 Thread attributes and Hazard Pointer Batches from existing affinity/HP teaching code and requires independent probe/body evidence.
- `revision-plan-20260910.md` requires queue evolution to add current cost-location and controlled comparison evidence without treating old or unrelated total timings as proof.
- `revision-audit-20260910.md` lists A01-A10 and maps each to a concrete handling path. A07 reflects the corrected queue-sample fact.
- `references/measurements/README.md` independently confirms final queue groups: `queue-storage`, `queue-batch`, `queue-spsc`, `queue-mpsc`, and `queue-ms`.

## Notes

- Current build evidence reported by the integrator is `60 total / 57 PASS / 2 SKIP / 1 FAIL`; the only failure is the materials check for unfinished `U01` files. I treat this as an execution-state blocker for final delivery, not a blocker in these two specification files.
- The plan should not later convert queue preallocation, batch, or SPSC cached results into an optimization claim unless the new diagnostic/cost-location evidence it asks for is actually recorded and linked.
- This review did not inspect generated build trees or rerun CTest. It is limited to the two reviewed files and their consistency with the supplied current evidence.
