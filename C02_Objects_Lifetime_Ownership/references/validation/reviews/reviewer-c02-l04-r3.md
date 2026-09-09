# C02 L04 r3 independent re-review

Reviewer: C02 independent teaching/code reviewer
Scope: `00-model-and-route.md`, `04-construction-and-unwinding.md`, `L04_construction`
Date: 2026-09-08
Verdict for this scope: APPROVE

This report supersedes only the L04 findings in `reviewer-c02-sample-gate-r1.md`. The r1 L07 findings remain open until the L07 owner author freezes the next revision and it is re-reviewed.

## Reviewed source fingerprints

- `Core_Study/chapters/00-model-and-route.md`: SHA256 `50241862CBA2BE899279C0B1C217E2BB841EE3C4DFAE69FF5FD3451A6D60056E`
- `Core_Study/chapters/04-construction-and-unwinding.md`: SHA256 `ACCF92456B594161F1C27D38FAB7AE6AA6CF8A01AA0AF365E07CE9C0099CCDEA`
- `Core_Study/exercises/L04_construction/README.md`: SHA256 `35F81A1B5D044FD46B620E1815B16296EB5B4382CC6F58EC5A18E2357DA6D8DD`
- `Core_Study/exercises/L04_construction/CMakeLists.txt`: SHA256 `E671D4564F792008A8E3150540FB7830B5C4AAA32B61575AE06BD3BE07D3988E`
- `Core_Study/exercises/L04_construction/checks/l04_checks.hpp`: SHA256 `BA9E7380A40A53B90E5CC28FF7FAE725C846C64C5A20F8CE84719E0E1BBFDA30`
- `Core_Study/exercises/L04_construction/checks/run_checks.cpp`: SHA256 `396FDC38FC8B2DE3652521A0CA01AFFAABF7E3AD71EB46EB66E5FF625B8F95DA`
- `Core_Study/exercises/L04_construction/checks/expect_failure.cmake`: SHA256 `4EA27F55BF903F0DC7F06E46A0563D95A97042DC5A08D838D5920E8A7F157162`
- `Core_Study/exercises/L04_construction/src/student/construction_lab.hpp`: SHA256 `B2BFBD5DD9A4F68EEB146067E17D3F0A17F2571BA3F17E6CC5A2425EB1A7439B`
- `Core_Study/exercises/L04_construction/src/student/construction_lab.cpp`: SHA256 `6928378AD29B7AB1B4E89CA7D10C73C9F7FCCF2054BCD72EDE327422C5635467`
- `Core_Study/exercises/L04_construction/reference/construction_lab.hpp`: SHA256 `3D6D1AEA5FA92131A4585EB4AB3EC3BD1D735CF6CAC15681A5BE2252D2CCD9D3`
- `Core_Study/exercises/L04_construction/reference/construction_lab.cpp`: SHA256 `6D856D4C2047F6320FC4809AF1EF2F1B57A526F940E8C3F3AF1E0FD3F9ED3499`
- `Core_Study/exercises/L04_construction/validation/good/construction_lab.cpp`: SHA256 `6D856D4C2047F6320FC4809AF1EF2F1B57A526F940E8C3F3AF1E0FD3F9ED3499`
- `Core_Study/exercises/L04_construction/validation/bad_noop/construction_lab.cpp`: SHA256 `0E1E403B866544414EF0F43116BD83F631F805E7397F024029370F2999E76E74`
- `Core_Study/exercises/L04_construction/validation/bad_leak/construction_lab.cpp`: SHA256 `3E6E0D2BB558FF6213945E9CF7A64E8C4B1077ECCE44447C475A66D766563FA0`

## Code Review Summary

Files reviewed: 14
Total issues: 0

By severity:

- CRITICAL: 0
- HIGH: 0
- MEDIUM: 0
- LOW: 0

## Stage 1: spec compliance

PASS for this scope.

The r1 blocker around `ResourceAttempt` / `implemented` is closed. The checker now directly constructs `l04::TwoResourceOwner` with a checker-owned `l04_checks::Recorder` (`run_checks.cpp:47-88`) and reads the recorder state directly. Student code no longer returns a report object that can be filled by hand.

The resource model is now bounded and no-allocation on the release/log path: events are enum values in `std::array`, live slots are fixed arrays, `release()` is `noexcept`, does not `delete`, and does not read a freed allocation (`l04_checks.hpp:39-109`). This closes the r1 UAF and destructor/logging-allocation risks for L04.

The bad variants are no longer raw `WILL_FAIL`. CMake wraps them with `expect_failure.cmake`, requires non-zero exit, rejects timeout, and matches fixed diagnostic text (`CMakeLists.txt:31-42`, `70-102`; `expect_failure.cmake:9-29`).

Student builds by default as a target and is only registered as a test when `CORE_STUDY_TEST_STUDENTS=ON` (`CMakeLists.txt:45-55`). The starter compiles safely and fails explicitly when run.

The 04 prerequisite wording is repaired:

- delegated constructor target success followed by delegating-body throw is covered (`04-construction-and-unwinding.md:68-70`);
- uncaught-exception / `std::terminate` boundary is qualified (`04-construction-and-unwinding.md:164`).

`00-model-and-route.md` remains acceptable for this sample-gate prerequisite: it gives a ground-up model of type/name/object/storage/lifetime/resource and closes its self-check answers without relying on reference code.

## Stage 2: code quality and safety

PASS for this scope.

Static pattern search found no remaining `ResourceAttempt`, `implemented`, `WILL_FAIL`, `SKIP`, completion marker, `student_ready`, release-after-`delete`, or logging allocation in L04. The only `return {}` is the safe Student starter's empty `observe_order()`, which fails under the checker.

No dedicated `lsp_diagnostics` tool was available in this worker surface. MSVC Debug/Release builds were used as compile/type diagnostics.

## Independent validation

Fresh default configure:

```text
cmake -S Core_Study/exercises/L04_construction -B Core_Study/references/validation/reviews/c02-independent-evidence/l04-r3-fresh -G "Visual Studio 18 2026" -A x64
```

Result: passed with MSVC 19.51.36256.0.

Fresh default builds:

```text
cmake --build Core_Study/references/validation/reviews/c02-independent-evidence/l04-r3-fresh --config Debug
cmake --build Core_Study/references/validation/reviews/c02-independent-evidence/l04-r3-fresh --config Release
```

Result: both passed. Default build compiled `L04_construction_student`, `L04_construction_reference`, `L04_construction_validation_good`, `L04_construction_validation_bad_noop`, and `L04_construction_validation_bad_leak`.

Fresh default CTest:

```text
ctest --test-dir Core_Study/references/validation/reviews/c02-independent-evidence/l04-r3-fresh -C Debug --output-on-failure
ctest --test-dir Core_Study/references/validation/reviews/c02-independent-evidence/l04-r3-fresh -C Release --output-on-failure
```

Result: Debug and Release both passed 4/4: reference, validation good, bad_noop wrapper, bad_leak wrapper.

Direct bad executable checks:

```text
L04_construction_validation_bad_noop.exe
```

Result: exit 1, `check failed: both resources should be live inside the complete object scope`.

```text
L04_construction_validation_bad_leak.exe
```

Result: exit 1, `check failed: completed first member should release during unwinding`.

Direct Student starter:

```text
L04_construction_student.exe
```

Result: exit 1, `check failed: both resources should be live inside the complete object scope`.

Student/ref-off configure and build:

```text
cmake -S Core_Study/exercises/L04_construction -B Core_Study/references/validation/reviews/c02-independent-evidence/l04-r3-student-ref-off -G "Visual Studio 18 2026" -A x64 -DCORE_STUDY_TEST_STUDENTS=ON -DCORE_STUDY_BUILD_REFERENCE=OFF
cmake --build Core_Study/references/validation/reviews/c02-independent-evidence/l04-r3-student-ref-off --config Debug
cmake --build Core_Study/references/validation/reviews/c02-independent-evidence/l04-r3-student-ref-off --config Release
```

Result: configure and builds passed. `ctest -N` listed only `L04_construction_student`; no Reference test was registered.

Student/ref-off CTest:

```text
ctest --test-dir Core_Study/references/validation/reviews/c02-independent-evidence/l04-r3-student-ref-off -C Debug --output-on-failure
ctest --test-dir Core_Study/references/validation/reviews/c02-independent-evidence/l04-r3-student-ref-off -C Release --output-on-failure
```

Result: both failed 1/1 with the expected starter diagnostic `check failed: both resources should be live inside the complete object scope`.

One earlier parallel run of build and CTest produced a transient Not Run because CTest raced before the executable existed. The sequential rerun above is the valid result.

## Recommendation

APPROVE 00/04/L04 r3 for the sample-gate prerequisite scope.

Overall sample-gate approval remains pending L07 r2 freeze and independent re-review.
