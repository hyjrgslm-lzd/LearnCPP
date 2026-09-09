# C02 independent review — owner 08-10 r1

## Verdict

REQUEST_CHANGES for the 08-10 frozen owner batch.

Per lesson:

- L08_unique: REQUEST_CHANGES. Runtime validation is healthy, but the exercise README gives an API that does not match the checker/reference API for Part 5.
- L09_shared: REQUEST_CHANGES. Runtime good/bad variants behave as authored, but the checker can be bypassed by hardcoded reports for Part 4 and Part 5.
- L10_control_block: APPROVE for this batch. I found no blocking issue in the reviewed teaching/code scope.

07/L07 remains on the earlier approved frozen state and was not reopened here.

## Scope reviewed

- `Core_Study/chapters/08-unique-ownership.md`
- `Core_Study/chapters/09-shared-ownership.md`
- `Core_Study/chapters/10-control-block.md`
- `Core_Study/exercises/L08_unique/**`
- `Core_Study/exercises/L09_shared/**`
- `Core_Study/exercises/L10_control_block/**`
- `Core_Study/references/validation/author-owner-08-10-r1-report.md`
- `Core_Study/references/validation/author-owner-l08-r1-*.json`
- `Core_Study/references/validation/author-owner-l09-r1-*.json`
- `Core_Study/references/validation/author-owner-l10-r1*.json`

This review does not approve later still-changing foundations/storage chapters. It also does not repeat the full public helper verification already approved in `verifier-student-wiring-r1.md`.

## Source binding

Hashes are saved in `c02-independent-evidence/owner-08-10-r1b/source-hashes.json`.

Key hashes:

- `Core_Study/chapters/08-unique-ownership.md`: `108F067583D9A5B0CE5A5634DFBFE824018C503613CC35BE723EE3E1D52656EF`
- `Core_Study/chapters/09-shared-ownership.md`: `5D89380EDEC7A8EB36EC390D4ADEAFDD7831682A6120C8FAD20D33A0389F98A9`
- `Core_Study/chapters/10-control-block.md`: `AD9D0B637EEB512B03C5B68198B321592CCEBE34F4868D5C31450643E65CA6DE`
- `Core_Study/exercises/L08_unique/README.md`: `24860108908F7AC3CC893878FBD62FE70B5B63A25053CDCE84407850B65F3C62`
- `Core_Study/exercises/L09_shared/README.md`: `534DC7D46C7AB3FD328356BEFE0756FF31A67E8AB48C4A33A342C7E2CCB72FAE`
- `Core_Study/exercises/L10_control_block/README.md`: `674DD543E1DCF1B6D3FEBF202CF4931529A84E2BACD3BEA7EDB2117ACCABA396`
- `Core_Study/exercises/L08_unique/checks/owner_checks.hpp`: `882F517BADEEA42A3D7668476A10A54D8135FCCA08BC296369474FF791D72691`
- `Core_Study/exercises/L09_shared/checks/owner_checks.hpp`: `BC2B200F08D74B63E7AF42477B6BCB12C1207369984A7B014F3257B65E5DFED9`
- `Core_Study/exercises/L10_control_block/checks/rc_checks.hpp`: `D79869745DAEA612D94EA9B0339AA54AA91A730D8DCE81472F83905FB3FF3FA4`
- `Core_Study/references/validation/author-owner-08-10-r1-report.md`: `54F698F38A71B15BA827B75577AB244E052BED2ADE570A8986C8A32717D4E0FB`

MSVC STL `<memory>` source guide path exists locally and matched the stated SHA256 `8955101828FE9D46C77E214E50415BAC5B1E59697E0F59BC46FA145AA1E8AE4C`; saved in `c02-independent-evidence/owner-08-10-r1b/msvc-memory-hash.json`.

## Evidence

Actual independent runs, saved under `c02-independent-evidence/owner-08-10-r1b/`:

- Debug validation builds for L08/L09/L10 all configured and built with exit 0.
- `ctest -C Debug` for all three validation builds returned exit 0 for observation/reference tests.
- Public good executables all returned exit 0.
- Public bad executables all returned exit 1 with concrete diagnostics:
  - L08 bad noop: `check failed: make_tracked returns owner`
  - L08 bad release-keeps-owner: `check failed: release leaves unique_ptr empty`
  - L09 bad cycle: `check failed: weak backedge does not keep cycle alive`
  - L09 bad weak resurrect: `check failed: weak lock fails after destruction`
  - L10 bad leak-control-block: `check failed: last strong: control block still alive`
  - L10 bad weak resurrect: `check failed: object destroyed after last strong`
- Release default configure/build/ctest for L08/L09/L10 all returned exit 0.
- Student + Reference OFF configure/build for L08/L09/L10 all returned exit 0; Student CTest returned exit 8 because the placeholder failed the real checker, with diagnostics `make_tracked returns owner`, `make_node returns shared owner`, and `make_rc returns owner` respectively.
- Author JSON summary is saved in `c02-independent-evidence/owner-08-10-r1b/author-json-summary.txt`; original failed intermediate records are still present alongside later fixed records.
- L09 fake-probe is saved in `c02-independent-evidence/owner-08-10-r1/probe-l09-fake-results-r1b/`. It compiled and ran a fake implementation that implemented Part 1-3 normally but hardcoded Part 4/5 result structs; output was `L09_shared_validation_contract OK` with exit 0.

A previous failed probe attempt is preserved in `c02-independent-evidence/owner-08-10-r1/probe-l09-fake-results/` and not reused as proof.

## Stage 1 — teaching/spec compliance

Teaching depth is generally acceptable for the batch:

- L08 explains unique ownership from raw-pointer failure, move-only ownership, `get`/`release`/`reset`, deleter state/size, arrays, incomplete types, failure construction, and source-reading boundaries.
- L09 explains stored pointer versus control block, aliasing constructor, strong/weak two-stage destruction, cycles, `enable_shared_from_this`, thread-safety boundary, and `make_shared` cost/weak retention boundary.
- L10 explains a deliberately limited single-thread `rc_ptr`/`weak_rc`, implicit weak, object/control-block two-stage lifetime, copy/move/reset, construction-failure cleanup, and gap from `std::shared_ptr`.

The batch fails Stage 1 because L08 Part 5 is not independently implementable from its exercise instructions, and L09 Part 4/5 checker accepts hardcoded reports instead of proving the requested ownership operations happened.

## Stage 2 — code/checker quality

No credential/security issue or broad fallback masking branch was found. Include wiring for all three checkers uses the expected pattern: checker includes `checks/support/...` first and implementation headers include support by explicit relative path. Leaf CMake include order puts `checks/support` before implementation directories.

No `lsp_diagnostics` tool is available in this lane; I used MSVC configure/build/CTest, direct executable runs, static source inspection, and an adversarial compile/run probe.

## Issues

[HIGH] L08 Part 5 README API does not match the checker/reference API  
File: `Core_Study/exercises/L08_unique/README.md:45`  
Issue: The exercise tells students to implement a default-constructed `incomplete_owner` with `has_state()` and `state_value()`. The actual checker constructs `l08::incomplete_owner owner(9)` and calls `owner.value()` / `moved.value()` in `Core_Study/exercises/L08_unique/checks/owner_checks.hpp:91-95`. The reference implementation exposes `explicit incomplete_owner(int)` and `value()` in `Core_Study/exercises/L08_unique/src/reference/owner.hpp:51-67`. A student following the README literally will implement the wrong interface and fail to compile or fail checks.  
Fix: Make the README and chapter Part 5 contract match the compiled interface, or change checker/reference/student stubs to the documented `has_state()` / `state_value()` / default-construction API. Then rerun Student placeholder, Reference, public good, and both bad variants.

[HIGH] L09 checker accepts hardcoded Part 4/5 reports instead of verifying real weak backedge and `shared_from_this` operations  
File: `Core_Study/exercises/L09_shared/checks/owner_checks.hpp:57-68`  
Issue: `check_weak_backedge_breaks_cycle()` trusts `CycleResult` fields returned by `build_parent_child_without_cycle()`, and `check_enable_shared_from_this()` trusts `EnableResult` fields returned by `shared_from_this_result()`. I compiled a fake implementation that genuinely implements `make_node`, `alias_value`, `observe`, and `lock_value`, but returns `{.parent_seen_from_child = true, .alive_after_scope = 0}` and `{.shared_from_this_ok = true, .use_count_during_call = 2}` without constructing the parent/child graph or calling `shared_from_this()`. The existing `validation_check.cpp` accepted it with `L09_shared_validation_contract OK`. The existing validation bad `bad_weak_resurrect` already contains the same hardcoded Part 4/5 pattern at `Core_Study/exercises/L09_shared/validation/bad_weak_resurrect/owner.hpp:22-23`, but that flaw is masked because the same bad fails earlier weak-lock checks.  
Fix: Redesign these two parts so the checker owns the observable facts. For example, expose operations that let the checker create parent/child nodes, inspect actual `weak_ptr`/`shared_ptr` state and fixture counters itself, and validate `enable_shared_from_this` by receiving/creating a real managed object and comparing the returned `shared_ptr` control behavior. Add a public bad that keeps Part 1-3 correct but hardcodes Part 4/5 reports; it must fail.

## Non-blocking findings / approved slice

- L08 runtime wiring is otherwise healthy: actual good passes, the two public bads fail with targeted diagnostics, Release default passes, and Student placeholder builds but fails the checker.
- L09 runtime good/bad variants behave as currently authored, but they are insufficient after the fake-probe finding.
- L10 is approved for this batch: the teaching scope is clearly single-threaded, code models object/control-block two-stage lifetime, construction failure cleans the control block, `lock()` cannot resurrect after strong reaches zero, public good/bad and Student placeholder behavior matched expectations.

## Recommendation

REQUEST_CHANGES for the 08-10 batch.

Recheck can be narrow after fixes: L08 Part 5 doc/API alignment plus L09 checker hardcoded-report rejection. L10 does not need a full rereview unless touched by the fix.
