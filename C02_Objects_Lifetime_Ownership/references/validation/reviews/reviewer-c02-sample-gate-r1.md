# C02 sample gate independent review r1

Reviewer: C02 independent teaching/code reviewer
Scope: `00-model-and-route.md`, `04-construction-and-unwinding.md`, `L04_construction` original snapshot evidence, `07-raii-and-ownership.md`, `L07_raii`
Date: 2026-09-08
Verdict: REQUEST_CHANGES

## Snapshot fingerprints

L04 original snapshot inspected before author repair began:

- `Core_Study/chapters/00-model-and-route.md`: SHA256 `50241862CBA2BE899279C0B1C217E2BB841EE3C4DFAE69FF5FD3451A6D60056E`
- `Core_Study/chapters/04-construction-and-unwinding.md`: SHA256 `A6D0CAB292D673D0D6B020EE110CD6CB224E8623451C933496315CB4D478601A`
- `Core_Study/exercises/L04_construction/README.md`: SHA256 `7DE623D9F436343995D7E0D2890C4709AAB9391A085D0FB7F3BAD1A9C6D580F3`
- `Core_Study/exercises/L04_construction/checks/run_checks.cpp`: SHA256 `A395235C1CA1C5A7B7E6DD8C4CA00BAC19C37AD0201416D7D0E8A05E041D1401`
- `Core_Study/exercises/L04_construction/checks/l04_checks.hpp`: SHA256 `9B51B4953303EA406398D7FA7FB268C7D47F30E18DD434CEAD9E9E75F1F93FB3`
- `Core_Study/exercises/L04_construction/src/student/construction_lab.cpp`: SHA256 `5A43A0898DA9701DBF97E6C213846EC12C352660EA774D6455E929DE99ED855A`
- `Core_Study/exercises/L04_construction/reference/construction_lab.cpp`: SHA256 `15B1F02FEEBFD20EAFC26362D0CD68C48DCFE9FB9C5DFDE77A7CF804F64AE7D5`

L07 snapshot inspected:

- `Core_Study/chapters/07-raii-and-ownership.md`: SHA256 `EFC68820863BDBDD525B9918DC6AFB679D2413B5BB855F1692D7EE2E2B50900B`
- `Core_Study/exercises/L07_raii/README.md`: SHA256 `870E6B3C14064CF400A83A806CB13003060C38FD32201361A9104B2BFCC21106`
- `Core_Study/exercises/L07_raii/checks/owner_checks.hpp`: SHA256 `F43CE4DE6EFFC438C750D294AFFF07C39C520601E79B6B717F10515D79C21600`
- `Core_Study/exercises/L07_raii/src/reference/owner.hpp`: SHA256 `55A9B5A476FDB82A0FF1E88D61335442FC36B8406E54C8CA935F5A18C47BB460`
- `Core_Study/exercises/L07_raii/src/student/owner.hpp`: SHA256 `1BA28DA2893041C10214EFBB6A8F61FB76693B061F79DDF74968A9104E6FA1EF`

## Validation run

- L04 original default: `cmake -S Core_Study/exercises/L04_construction -B Core_Study/references/validation/reviews/c02-independent-evidence/l04-default -G "Visual Studio 18 2026" -A x64`; Debug and Release build passed with MSVC 19.51.36256.0.
- L04 original default CTest: Debug and Release `ctest --test-dir .../l04-default -C <config> --output-on-failure`; both passed 4/4 including `reference`, `validation_good`, `validation_bad_noop`, `validation_bad_leak`.
- L04 original bad direct runs: `L04_construction_validation_bad_noop.exe` failed with `check failed: outer object should not be complete after second acquire throws`; `L04_construction_validation_bad_leak.exe` failed with `check failed: completed first member should release during unwinding`.
- L04 original student isolation: `-DCORE_STUDY_TEST_STUDENTS=ON -DCORE_STUDY_BUILD_REFERENCE=OFF`; Debug and Release builds passed; CTest failed as expected with `check failed: implementation is still the safe starter`.
- L04 r2 is in author repair while this report is written. Current files were not re-approved.
- L07 default: `cmake -S Core_Study/exercises/L07_raii -B Core_Study/references/validation/reviews/c02-independent-evidence/l07-default -G "Visual Studio 18 2026" -A x64 -DL07_RAII_BUILD_VALIDATION_VARIANTS=ON`; Debug and Release build passed. Release produced C4702 unreachable-code warnings for the student/bad-noop path after `student_ready()`.
- L07 CTest: Debug and Release passed 2/2 for registered `observation` and `reference`.
- L07 validation executables: `L07_raii_validation_good.exe` passed with `L07_raii_reference OK`; `L07_raii_validation_bad_noop.exe` failed with `check failed: student implementation still placeholder`.
- L07 student isolation: `-DCORE_STUDY_TEST_STUDENTS=ON -DCORE_STUDY_BUILD_REFERENCE=OFF`; Debug and Release builds passed; CTest failed as expected with `check failed: student implementation still placeholder`.
- L07 move-assignment probe: `Core_Study/references/validation/reviews/c02-independent-evidence/l07-move-assign-order`; output shows old target resources release in this order: `release first 3`, then `release second 4`.
- Delegating constructor probe: `Core_Study/references/validation/reviews/c02-independent-evidence/delegating-ctor-throw`; output shows target success followed by delegating-body throw runs `~Delegating()` then `~Member()`.

No dedicated `lsp_diagnostics` tool was available in this worker surface; MSVC Debug/Release builds were used as the type/compile diagnostic source. I did not run broad root `Core_Study/exercises` configure because only the sample-gate chapters are in review and later exercise directories are not delivered yet.

## Findings

### HIGH: L07 reference releases move-assignment target resources in dependency-unsafe order

File: `Core_Study/exercises/L07_raii/src/reference/owner.hpp:169`

`two_resource_owner& operator=(two_resource_owner&&) noexcept = default;` delegates member move assignment in declaration order: `first_`, then `second_`. Because `first_owner::operator=` calls `reset()` before taking the new id (`owner.hpp:77-81`) and `second_owner::operator=` does the same later (`owner.hpp:111-115`), assigning into a non-empty target releases old first before old second.

Evidence: the independent probe prints:

```text
acquire first 1
acquire second 2
acquire first 3
acquire second 4
release first 3
release second 4
```

This contradicts the chapter's own dependency rule that the later resource should release first (`07-raii-and-ownership.md:21`, `:142-145`) and the destructor/reset path that intentionally releases second before first (`owner.hpp:177-180`). The checker only uses `contains_event()` for these releases (`owner_checks.hpp:114-116`), so it misses the order bug.

Fix: write a custom `two_resource_owner::operator=(two_resource_owner&&) noexcept` that handles self-move, calls `reset()` once to release old target resources in reverse order, then moves `first_` and `second_` from the source. Add an exact-order assertion for move assignment release events.

### HIGH: L07 checker trusts the implementation under test for the resource model

File: `Core_Study/exercises/L07_raii/checks/owner_checks.hpp:20`

The checker reads `l07::resource_counters()` and `l07::resource_events()` from `<owner.hpp>` (`owner_checks.hpp:20-23`, `45-50`, `65-68`, `94-95`, `111-116`). In both Student and Reference, these counters/events live inside the implementation header (`src/reference/owner.hpp:25-28`, `136-148`; `src/student/owner.hpp:27-32`). That means the subject under test supplies both behavior and evidence. A fake implementation can satisfy counters/events/accessors without proving it called a trusted acquire/release fixture or represented resources with completed RAII subobjects.

The current public bad variant does not close this gap: `validation/bad_noop/owner.hpp:3` includes the placeholder Student, and `student_check.cpp:6` exits at `student_ready()` before the ownership contract runs.

Fix: move the resource model, counters, event log, and acquire/release functions into a trusted checks/fixture header owned by the exercise. Student code should only consume that fixture. Remove the completion-marker gate from correctness validation. Add bad variants for fake-complete counters/events, missing second-failure cleanup, and wrong move-assignment release order.

### MEDIUM: L07 constructor with `AcquisitionPlan` resets global model from inside an owner constructor

File: `Core_Study/exercises/L07_raii/src/reference/owner.hpp:159`

`explicit two_resource_owner(AcquisitionPlan next_plan)` calls `reset_resource_model(next_plan)` inside the owner constructor before acquiring resources (`owner.hpp:159-162`). This can erase counters/events for another live owner and is not a normal owner responsibility. It also teaches that constructing an owner may reset external global state, which conflicts with the ownership model the chapter is trying to isolate.

Fix: keep model reset as an explicit fixture operation outside `two_resource_owner`. The owner constructor should only acquire resources according to the already configured plan.

### MEDIUM: L07 release functions are marked `noexcept` but allocate while logging

File: `Core_Study/exercises/L07_raii/src/reference/owner.hpp:54`

`release_first()` and `release_second()` are `noexcept`, but each does `events.push_back("release ... " + std::to_string(id))` (`owner.hpp:54-63`). Allocation failure in this logging path would call `std::terminate`, including during destructor/unwind paths. The observation model has the same pattern in `observation_check.cpp:42-49`.

Fix: use a bounded no-allocation event log for the fixture, such as enum events plus ids in fixed storage, or pre-reserve and keep release logging out of `noexcept` release paths. The release operation used by destructors should be non-throwing in the model.

### MEDIUM: L07 validation bad path proves only the placeholder gate, not representative wrong implementations

File: `Core_Study/exercises/L07_raii/checks/student_check.cpp:6`

`student_check.cpp` calls `l07::student_ready()` before `run_reference_contract()`. The bad-noop validation includes the Student placeholder and fails at `student implementation still placeholder`; Release builds then warn that later contract checks are unreachable. This verifies the marker, not that the checker rejects no-op, fake-complete, leak-on-second-failure, or wrong move-order implementations.

Fix: remove the marker from validation, or keep it only as a starter safety check outside checker validation. Register and run actual bad implementations that bypass the marker and fail for the intended contract reason.

### MEDIUM: 04 text overstates unwinding when no caller catches the exception

File: `Core_Study/chapters/04-construction-and-unwinding.md:154`

The sentence says whether the caller catches the exception does not affect `out` cleanup. That is true for propagation through normal stack unwinding to a handler. If no matching handler exists and the program terminates, whether the stack is unwound before termination is not a portable guarantee. The current wording is too absolute for a ground-up lesson about destruction guarantees.

Fix: qualify the sentence: when an exception propagates out through stack unwinding toward a handler, completed local objects on that path are destroyed. Separately mention the uncaught-exception/termination boundary.

### MEDIUM: 04 delegated-constructor section omits the target-success/body-throw case

File: `Core_Study/chapters/04-construction-and-unwinding.md:68`

The text covers the case where the target constructor throws. It does not cover the important exception case where the target constructor succeeds but the delegating constructor body throws. The independent probe observed MSVC running the complete object's destructor in that case:

```text
Member()
target body
delegating body
~Delegating()
~Member()
caught
```

This is a useful boundary because it is the exception to the simpler "constructor throws, outer destructor does not run" mental model.

Fix: add a short example and rule immediately after the current delegated-constructor paragraph. Keep it scoped to delegation; do not expand the chapter.

## Teaching review

`00-model-and-route.md` is acceptable as a sample-gate prerequisite. It introduces type/name/object/storage/lifetime/resource in continuous prose, gives small self-checks, and closes the five answers without relying on Reference code.

`07-raii-and-ownership.md` is directionally acceptable as teaching prose: it starts from a manual success baseline, introduces the second-acquire failure, maps that failure to partially constructed object rules, then derives per-resource RAII members, move transfer, and the `unique_ptr` comparison. The current blockers are not missing narrative depth in the chapter body; they are correctness and checker-trust gaps in the exercise/reference plus two 04 prerequisite wording gaps.

## Recommendation

REQUEST_CHANGES for sample-gate r1.

Do not approve 07/L07 or use it as the bulk authoring template until the r2 repair is frozen and independently re-run. The minimum repair is: trusted shared resource fixture, no completion-marker-dependent bad validation, bounded no-throw event logging for destructor paths, no owner constructor resetting global model, custom reverse-release move assignment, and public bad variants that prove the checker rejects fake-complete, leak-on-second-failure, and wrong release order.
