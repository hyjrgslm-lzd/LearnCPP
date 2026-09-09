# C02 Independent Closeout - Owner 08-10 r2 doc interface

Reviewer: C02 independent teaching/code reviewer
Scope: close only the remaining README interface findings from `reviewer-owner-08-10-r2.md`; no owner code matrix rerun.
Verdict: APPROVE
Date: 2026-09-09

## Source binding

Evidence root: `Core_Study/references/validation/reviews/c02-independent-evidence/owner-08-10-r2-doc-closeout/`

Bound files and hashes are recorded in `source-hashes.json`, including:

- `Core_Study/references/validation/author-owner-08-10-r2-doc-interface-report.md`
- `Core_Study/exercises/L08_unique/README.md`
- `Core_Study/exercises/L08_unique/checks/owner_checks.hpp`
- `Core_Study/exercises/L08_unique/src/reference/owner.hpp`
- `Core_Study/exercises/L09_shared/README.md`
- `Core_Study/exercises/L09_shared/checks/owner_checks.hpp`
- `Core_Study/exercises/L09_shared/src/reference/owner.hpp`
- `Core_Study/exercises/L09_shared/CMakeLists.txt`

## Closed items

### L08 Part 4 README interface drift: closed

`Core_Study/exercises/L08_unique/README.md:38-41` now documents the real API and data contract:

- `make_array(int size, int first_value)`
- `sum_array(const int_array& values, int size)`
- checker call `make_array(4, 10)` with expected values `10, 11, 12, 13` and sum `46`

This matches the checker at `Core_Study/exercises/L08_unique/checks/owner_checks.hpp:83-85` and reference implementation at `Core_Study/exercises/L08_unique/src/reference/owner.hpp:33-41`.

### L09 Part 3 README interface drift: closed

`Core_Study/exercises/L09_shared/README.md:24-26` now documents the real API:

- `bool lock_value(const std::weak_ptr<node>& weak, int& out)`
- success writes the real node value and returns `true`
- expired weak returns `false`

This matches the checker calls at `Core_Study/exercises/L09_shared/checks/owner_checks.hpp:29` and `:37`, and the reference implementation at `Core_Study/exercises/L09_shared/src/reference/owner.hpp:23-29`.

### L09 public validation command omission: closed

`Core_Study/exercises/L09_shared/README.md:56` now lists `./build/c02-owner-l09/Debug/L09_shared_validation_bad_hardcoded_noop.exe`, matching the target defined at `Core_Study/exercises/L09_shared/CMakeLists.txt:39-41`.

## Validation performed

- `scan-summary.json`: old bad interface scan exit 1; new expected interface scan exit 0.
- `old-interface-scan.stdout.txt`: no hits for old `make_array(int n)`, `1..n`, `{locked, value}`, `has_state`, `state_value`, or removed self-report names.
- `new-interface-scan.stdout.txt`: confirms current L08/L09 README contains the corrected public APIs and command.

Prior r2 code/ASan validation remains the bound technical evidence from `reviewer-owner-08-10-r2.md`; this closeout intentionally did not rerun the owner matrix.

## Recommendation

APPROVE for the 08-10 r2 doc-interface closeout. The earlier owner 08-10 r2 README blockers are closed.
