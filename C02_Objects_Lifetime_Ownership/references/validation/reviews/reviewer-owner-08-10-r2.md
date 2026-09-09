# C02 Independent Review - Owner 08-10 r2

Reviewer: C02 independent teaching/code reviewer
Scope: r2 frozen owner batch only: 08/09/10 chapters and exercises, plus author r2 evidence. Prior approved 07 and foundation batches were not re-reviewed.
Verdict: REQUEST_CHANGES
Date: 2026-09-08

## Source binding

Evidence root: `Core_Study/references/validation/reviews/c02-independent-evidence/owner-08-10-r2/`

Source hashes were recorded in `source-hashes.json`. Key r2 bindings:

- `Core_Study/references/validation/author-owner-08-10-r2-report.md` SHA256 `1731AEF0CFFDE5B385B2B75F0111C8DF79701CF60FA68FB0F42F1D022C4899F4`
- `Core_Study/exercises/L08_unique/README.md` SHA256 `BB3BBB56B92C640A47AAC5D0AC46B5DF8F64894CE2D308895751EA5831B7826E`
- `Core_Study/exercises/L09_shared/README.md` SHA256 `6C32CB1D82845393376554A25A4091DF74C969514EC47D14BA8F35B0425D5767`
- `Core_Study/exercises/L09_shared/checks/owner_checks.hpp` SHA256 `669FCE0836100F4D425A2EF9E53E2EC8326BA8A02F51D68653DAF993C301ED6B`
- `Core_Study/exercises/L10_control_block/src/reference/rc.hpp` SHA256 `3505F0BB7A87E4A16A27B32872FDB0DAD1DB185EDCE671E5FCA04A30CC296E26`
- `Core_Study/exercises/L10_control_block/checks/rc_checks.hpp` SHA256 `AD5B6412DB4069F1F7211D666FED86B26CC65DDE5D6C991903DC51EB11BE11B1`

## Files reviewed

Focused review over 08/09/10 user-facing chapters, exercise READMEs, CMake wiring, trusted support/checkers, reference/student implementations, validation variants, and author r2 JSON/report evidence. Static line checks and build/run evidence are preserved under the evidence root.

## Summary by severity

- CRITICAL: 0
- HIGH: 2
- MEDIUM: 1
- LOW: 0

## Issues

### HIGH 1. L08 README Part 4 documents the wrong student API and array contents

File: `Core_Study/exercises/L08_unique/README.md:38-39`

Issue: The README tells students to implement `make_array(int n)` that writes `1..n`, and `sum_array(const int_array&, int n)`. The actual checker/reference/student interface is `make_array(int size, int first_value)` and `sum_array(const int_array&, int size)`. The checker calls `make_array(4, 10)` and expects sum `46`, i.e. `10 + 11 + 12 + 13` (`Core_Study/exercises/L08_unique/checks/owner_checks.hpp:82-85`; reference implementation at `src/reference/owner.hpp:33-41`).

Why this blocks: A student following the README will implement the wrong signature and wrong initialization rule. This violates the sample gate requirement that each Part be independently completable from the exercise text.

Fix: Change the Part 4 README contract to the real API, for example: `make_array(int size, int first_value)` creates a `std::unique_ptr<int[]>` with values `first_value ... first_value + size - 1`; `sum_array(const int_array&, int size)` sums exactly `size` elements.

### HIGH 2. L09 README Part 3 documents the removed self-report return shape instead of the actual checker API

File: `Core_Study/exercises/L09_shared/README.md:23-24`

Issue: The README still describes `lock_value(const std::weak_ptr<Node>&)` as returning `{locked, value}`. r2 code correctly removed result self-report types, and the actual API is `bool lock_value(const std::weak_ptr<node>& weak, int& out)` (`Core_Study/exercises/L09_shared/src/reference/owner.hpp:23-29`; checker calls at `checks/owner_checks.hpp:28-30` and `:36-37`).

Why this blocks: This is exactly the kind of exercise interface drift the r2 fix was meant to remove. The checker now consumes real operations, but the student-facing README still points students at a non-existent return contract.

Fix: Update Part 3 to state the exact signature and semantics: `observe(const std::shared_ptr<node>&)` returns `std::weak_ptr<node>`; `bool lock_value(const std::weak_ptr<node>& weak, int& out)` returns `true` only when `weak.lock()` succeeds and writes the node value into `out`; after expiration it returns `false`.

### MEDIUM 3. L09 README validation commands omit the new hardcoded-noop negative target

File: `Core_Study/exercises/L09_shared/README.md:55-60`

Issue: The command block lists `validation_good`, `validation_bad_cycle`, and `validation_bad_weak_resurrect`, but omits `L09_shared_validation_bad_hardcoded_noop.exe`. The prose below the block says bad hardcoded noop should fail, and CMake does define the target at `Core_Study/exercises/L09_shared/CMakeLists.txt:39-41`.

Why this matters: The new target is the key public regression for the old r1 fake-complete hole. Omitting it weakens the reproducible validation recipe even though the code wiring is now correct.

Fix: Add `./build/c02-owner-l09/Debug/L09_shared_validation_bad_hardcoded_noop.exe` to the README validation command block, with the same expected-failure framing as the other bad variants.

## Closed r1/r2 technical risks

### L08 original Part 5 mismatch: closed

`Core_Study/exercises/L08_unique/README.md:45` now matches the checker/reference shape: `explicit incomplete_owner(int value)` owns an `IncompleteState`, `value()` returns the value, moved-from empty state returns `-1`. I did not find remaining `has_state` / `state_value` / default-owning wording in the scoped scan.

### L09 fake-complete/self-report hole: closed in code

The r2 checker now owns the real graph facts:

- Part 4 creates real parent/child owners, calls `connect_parent_child(parent, child)`, then directly checks `parent->next == child`, `child->parent.lock() == parent`, external weak expiry, and fixture alive count (`Core_Study/exercises/L09_shared/checks/owner_checks.hpp:57-72`).
- Part 5 calls `shared_from_existing(*owner)` and checks same address plus shared owner count (`Core_Study/exercises/L09_shared/checks/owner_checks.hpp:75-83`).
- The old r1 fake implementation no longer compiles against r2 checks; preserved evidence: `l09-old-fake-repro/summary.json`.
- The new `bad_hardcoded_noop` target fails at the real edge check: `check failed: parent owns child through shared next`; preserved evidence: `validation-debug-summary.json`.

### L10 alias assignment UAF: closed

`rc_ptr` copy assignment now retains `other.block_` before releasing the old target (`Core_Study/exercises/L10_control_block/src/reference/rc.hpp:77-84`). Move assignment exchanges the RHS block out before releasing old target (`:89-95`). The adopt constructor is private and reachable only by `weak_rc<T>` / `detail::rc_access` (`:114-122`, `:148-154`). The checker now includes copy and move alias cases (`Core_Study/exercises/L10_control_block/checks/rc_checks.hpp:86-116`).

Independent ASan repros based on the previous verifier cases now pass with current r2 reference source:

- `l10-alias-asan/asan-summary.json`: `BuildExit=0`, `CopyExit=0`, `MoveExit=0`
- outputs: `copy alias assignment OK`, `move alias assignment OK`

## Validation performed

Debug validation matrix, saved in `validation-debug-summary.json`:

- L08 configure/build/ctest/good: PASS
- L08 `bad_noop`: rejected with `check failed: make_tracked returns owner`
- L08 `bad_release_keeps_owner`: rejected with `check failed: release leaves unique_ptr empty`
- L09 configure/build/ctest/good: PASS
- L09 `bad_hardcoded_noop`: rejected with `check failed: parent owns child through shared next`
- L09 `bad_cycle`: rejected with `check failed: parent weak expires after graph scope`
- L09 `bad_weak_resurrect`: rejected with `check failed: weak lock fails after destruction`
- L10 configure/build/ctest/good: PASS
- L10 bad leak: rejected with `check failed: last strong: control block still alive`
- L10 bad weak resurrect: rejected with `check failed: object destroyed after last strong`

Release + Student/Reference-off wiring, saved in `release-student-summary.json`:

- L08/L09/L10 Release configure/build/ctest: PASS
- Student-only builds with reference disabled: PASS build, expected checker failures at first unimplemented operation:
  - L08: `check failed: make_tracked returns owner`
  - L09: `check failed: make_node returns shared owner`
  - L10: `check failed: make_rc returns owner`

Static scans:

- `forbidden-pattern-scan.txt`: no remaining `CycleResult`, `EnableResult`, `has_state`, `state_value`, `student_ready`, `placeholder_state`, `implemented`, or report-return pattern in scoped 08/09/10 sources.
- `l10-adopt-assignment-lines.txt`: confirms private adopt path and alias-safe assignment lines.
- `l09-real-check-lines.txt`: confirms direct graph/control-block checks and `bad_hardcoded_noop` target wiring.

Tooling limitation: no `lsp_diagnostics` tool is exposed in this worker session. I used MSVC Debug/Release builds, CTest, direct bad executables, ASan repros, and static `rg` scans as the available diagnostics.

## Recommendation

REQUEST_CHANGES for the r2 owner 08-10 batch as a whole.

分项结论：

- L08: REQUEST_CHANGES. Code/checker build and Part 5 r1 mismatch are fixed, but Part 4 README still gives the wrong API and data contract.
- L09: REQUEST_CHANGES. The fake-complete/self-report checker hole is fixed in code, but Part 3 README still documents the removed return shape, and the validation recipe omits the new negative target.
- L10: APPROVE for r2. The copy/move alias UAF fix, checker coverage, ASan repro, and private adopt boundary all passed the scoped review.
