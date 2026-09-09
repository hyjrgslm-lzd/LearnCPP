# C02 sample gate independent review r2

Reviewer: C02 independent teaching/code reviewer
Scope: `00-model-and-route.md`, `04-construction-and-unwinding.md`, `L04_construction`, `07-raii-and-ownership.md`, `L07_raii`
Date: 2026-09-08
Verdict: APPROVE

This report keeps the L04 r3 approval from `reviewer-c02-l04-r3.md` and adds the L07 r2 frozen re-review. The older r1 findings remain useful history, but the L04/L07 blockers found there are closed in the source versions fingerprinted below.

## Code Review Summary

Files reviewed in this L07 pass: 16
Total issues: 0

By severity:

- CRITICAL: 0
- HIGH: 0
- MEDIUM: 0
- LOW: 0

Recommendation: APPROVE sample gate.

## L07 r2 source fingerprints

- `Core_Study/chapters/07-raii-and-ownership.md`: SHA256 `5B1BBCA7CFC6F7DF50E9768E8D404681A5C9669854DAD26E95D7881169340D4E`
- `Core_Study/exercises/L07_raii/README.md`: SHA256 `AA82217B2AE7ED656390C01D452AD4427B31C081589E9507C759A3341B2D1ECE`
- `Core_Study/exercises/L07_raii/CMakeLists.txt`: SHA256 `D10D13864C293D28E53B7187BCD6A85D9123784C5AF12717DF5631146D2CC8AF`
- `Core_Study/exercises/L07_raii/checks/support/resource_model.hpp`: SHA256 `E3CAC71C517F6CEDF95ECFAF2CD524AB75C57E77D418D58CCD0626ED458D8D35`
- `Core_Study/exercises/L07_raii/checks/owner_checks.hpp`: SHA256 `4D10652E27375E7671BF201FE07EEDADC7654608998A46D925EFE77D1364C9D7`
- `Core_Study/exercises/L07_raii/checks/observation_check.cpp`: SHA256 `701CC805A97BE63BC205D4FFA42526898F8AAD394A587AC00E240F8C7B4AFEB6`
- `Core_Study/exercises/L07_raii/checks/reference_check.cpp`: SHA256 `A21B49D08A6B6F7848E59367901327D3D2EAA9A9D0543E1BEC2AAC7E2B57B37A`
- `Core_Study/exercises/L07_raii/checks/student_check.cpp`: SHA256 `78F860CA014BCF7101D7F517F2A64EEC0FDDA929C481D9C7B4CBAF9B27E9DC9F`
- `Core_Study/exercises/L07_raii/checks/validation_check.cpp`: SHA256 `6CD4C833D4977FD11520611A4AE33D3B03E5ED2E4BF3F8222C78C8FB054F40B5`
- `Core_Study/exercises/L07_raii/src/student/owner.hpp`: SHA256 `CE2E64DB99D26509443A5B9DBA844382E55ABEEEAE6AF309F4AD65EE4D074BD0`
- `Core_Study/exercises/L07_raii/src/reference/owner.hpp`: SHA256 `F94BEF9E929274C663CB427B6DD00968B872D6DC2260CDB3E353E358E58821DF`
- `Core_Study/exercises/L07_raii/validation/good/owner.hpp`: SHA256 `AD7364CDED04A80A0C98E5781E66F072A87E41EF5885BC3AC25EFA85AB716B89`
- `Core_Study/exercises/L07_raii/validation/bad_noop/owner.hpp`: SHA256 `222EBBE8A841892B7F5A7B56778ABDF84674B5CD08A4C9414809337252BC5C8B`
- `Core_Study/exercises/L07_raii/validation/bad_fake_completed/owner.hpp`: SHA256 `041829290FC211B5DD650A8951DC3390E200D2758BD453F26286289AB774594D`
- `Core_Study/exercises/L07_raii/validation/bad_memberwise_move_order/owner.hpp`: SHA256 `4886C4585F67A1201C86EC55C8BF743C355453093E5964A7FA8A12EB65026BD1`

## Stage 1: spec compliance

PASS.

The r2 implementation matches the sample-gate repair requirements:

- The trusted resource fixture is now `checks/support/resource_model.hpp`. It owns counters, failure injection, event ids, live-id slots, and invalid-release detection. Student code includes the fixture but cannot provide its own counters or events.
- `src/student/owner.hpp` contains only the placeholder `two_resource_owner` implementation surface. There is no `student_ready`, completion marker, `ResourceAttempt`, or Student-side resource report.
- `src/reference/owner.hpp` removed the old `two_resource_owner(AcquisitionPlan)` constructor. Failure plans are set only through the checker fixture.
- Reference move assignment is custom: `two_resource_owner::operator=` calls `reset()` first, so old target resources release second before first, then it moves from the source.
- `owner_checks.hpp` covers normal lifetime, first acquire failure, second acquire failure with first unwind, reset idempotence, move construction, move assignment exact event order, self move, interleaved owners, and fixture reset rejection while resources are live.
- Public validation variants cover no-op, fake-complete/leak-second, and default memberwise move assignment order. These are representative of the old r1 checker gaps.

The 07 teaching prose is adequate for the sample gate. It starts from a manual success baseline, adds the second-acquire failure, explains why plain integers do not carry release semantics, derives the partially constructed object rule from 04, then builds a single-resource owner and two-resource RAII composition. It also states the model boundary: the experiment proves this bounded resource model, not real UB or all system handles.

The L07 README is independently usable. It names the Student edit file, explains the support fixture, gives Parts 1-5, explains expected observations and checks, and lists the public good/bad validation variants. A student can attempt the owner without opening Reference after reading 00/04/07.

## Stage 2: code quality and safety

PASS.

Static pattern search found no `ResourceAttempt`, `implemented`, `student_ready`, placeholder gate, `WILL_FAIL`, `SKIP`, hardcoded secret pattern, or Student-owned fixture in L07. `std::vector`, `push_back`, and `std::to_string` remain only in `observation_check.cpp`, which is a standalone safe observation program and not the trusted checker support release path.

The trusted fixture release functions are `noexcept`, use fixed arrays, do not allocate strings, and do not delete or read freed objects. Invalid or repeated release is recorded as fixture state and checked by `check_empty_model()`.

No dedicated `lsp_diagnostics` tool was available in this worker surface. MSVC Debug/Release builds were used as compile/type diagnostics.

## Independent validation

Fresh default + validation configure:

```text
cmake -S Core_Study/exercises/L07_raii -B Core_Study/references/validation/reviews/c02-independent-evidence/l07-r2-fresh -G "Visual Studio 18 2026" -A x64 -DL07_RAII_BUILD_VALIDATION_VARIANTS=ON
```

Result: passed with MSVC 19.51.36256.0.

Fresh Debug/Release build:

```text
cmake --build Core_Study/references/validation/reviews/c02-independent-evidence/l07-r2-fresh --config Debug
cmake --build Core_Study/references/validation/reviews/c02-independent-evidence/l07-r2-fresh --config Release
```

Result: both passed. Built observation, reference, validation_good, validation_bad_noop, validation_bad_fake_completed, and validation_bad_memberwise_move_order. Release emitted unreachable-code warnings only in deliberately failing no-op validation paths after the checker's first guaranteed failing assertion.

Fresh default CTest:

```text
ctest --test-dir Core_Study/references/validation/reviews/c02-independent-evidence/l07-r2-fresh -C Debug --output-on-failure
ctest --test-dir Core_Study/references/validation/reviews/c02-independent-evidence/l07-r2-fresh -C Release --output-on-failure
```

Result: Debug and Release both passed 2/2: observation and reference.

Fresh validation executables, Debug and Release:

```text
L07_raii_validation_good.exe
L07_raii_validation_bad_noop.exe
L07_raii_validation_bad_fake_completed.exe
L07_raii_validation_bad_memberwise_move_order.exe
```

Result:

- good: exit 0, `L07_raii_validation_contract OK`
- bad_noop: exit 1, `check failed: normal owner must own first`
- bad_fake_completed: exit 1, `check failed: normal lifetime: second resource still alive`
- bad_memberwise_move_order: exit 1, `check failed: move assignment releases target old second first`

Student/ref-off configure:

```text
cmake -S Core_Study/exercises/L07_raii -B Core_Study/references/validation/reviews/c02-independent-evidence/l07-r2-student-ref-off -G "Visual Studio 18 2026" -A x64 -DCORE_STUDY_TEST_STUDENTS=ON -DCORE_STUDY_BUILD_REFERENCE=OFF
```

Result: passed.

Student/ref-off build:

```text
cmake --build Core_Study/references/validation/reviews/c02-independent-evidence/l07-r2-student-ref-off --config Debug
cmake --build Core_Study/references/validation/reviews/c02-independent-evidence/l07-r2-student-ref-off --config Release
```

Result: both passed for observation and student. `ctest -N` listed only `L07_raii_observation` and `L07_raii_student`.

Reference target absent check:

```text
cmake --build Core_Study/references/validation/reviews/c02-independent-evidence/l07-r2-student-ref-off --config Debug --target L07_raii_reference
```

Result: failed with `MSB1009: 项目文件不存在`, confirming the Reference target is absent when reference build is disabled.

Student/ref-off CTest:

```text
ctest --test-dir Core_Study/references/validation/reviews/c02-independent-evidence/l07-r2-student-ref-off -C Debug --output-on-failure
ctest --test-dir Core_Study/references/validation/reviews/c02-independent-evidence/l07-r2-student-ref-off -C Release --output-on-failure
```

Result: observation passed; Student failed as expected with `check failed: normal owner must own first` in both configs.

## Process note

I did not overwrite the r1 report or earlier independent probes. New evidence uses `l07-r2-*` prefixes. The author r2 report and 15 `author-owner-l07-r2-final-*.json` files are present. Any historical note about cleaned intermediate r2 JSON is a root/process follow-up, not a code or sample-gate blocker in the frozen r2 source reviewed here.

## Final recommendation

APPROVE the C02 sample gate for the reviewed scope: `00`, `04/L04`, and `07/L07`.

This approval does not cover bulk 08-10 expansion, whole-course integration, top-level navigation, or final Core_Study completion.
