# C02 L07 r3 incremental independent re-review

Reviewer: C02 independent teaching/code reviewer
Scope: incremental re-review of L07 r3 include wiring and affected checks
Date: 2026-09-08
Verdict: APPROVE

This report supersedes the r2 approval only for the include-shadow issue raised by `verifier-l07-r2.md`. The prior teaching review remains valid; this pass did not re-expand the teaching scope.

## Code Review Summary

Files reviewed: 13
Total issues: 0

By severity:

- CRITICAL: 0
- HIGH: 0
- MEDIUM: 0
- LOW: 0

Recommendation: APPROVE the C02 sample gate for `00`, `04/L04`, and `07/L07`.

## Reviewed source fingerprints

- `Core_Study/exercises/L07_raii/CMakeLists.txt`: SHA256 `6838FB984FB6A03095D487592A88840874512C8D607C68C5DCD6A14BEA4714F4`
- `Core_Study/exercises/L07_raii/README.md`: SHA256 `4CB57317FE241A606C390B7F33927D13F3389D72AC9B22F3E77AB5D65FF476D3`
- `Core_Study/exercises/L07_raii/checks/owner_checks.hpp`: SHA256 `23250A6C7D7C2CCD448A2B90C5A9A53619A547D47F413316168FC5E6716A3568`
- `Core_Study/exercises/L07_raii/checks/support/resource_model.hpp`: SHA256 `E3CAC71C517F6CEDF95ECFAF2CD524AB75C57E77D418D58CCD0626ED458D8D35`
- `Core_Study/exercises/L07_raii/src/student/owner.hpp`: SHA256 `34D3063559032BDE19FDB4C2D12FDCA455D2068B7DDF01421671A4F3CEAE46BA`
- `Core_Study/exercises/L07_raii/src/reference/owner.hpp`: SHA256 `2AB13F03A11033DEBF081AC80DFAF09278D42DBE351286F6F5A37EB498401A2D`
- `Core_Study/exercises/L07_raii/validation/good/owner.hpp`: SHA256 `AD7364CDED04A80A0C98E5781E66F072A87E41EF5885BC3AC25EFA85AB716B89`
- `Core_Study/exercises/L07_raii/validation/bad_noop/owner.hpp`: SHA256 `34D3063559032BDE19FDB4C2D12FDCA455D2068B7DDF01421671A4F3CEAE46BA`
- `Core_Study/exercises/L07_raii/validation/bad_fake_completed/owner.hpp`: SHA256 `2855798260C7D57857F5AD88B013FE0310387C0E8F7B1A7BF077683F93D4457A`
- `Core_Study/exercises/L07_raii/validation/bad_memberwise_move_order/owner.hpp`: SHA256 `4751A327E05C7E05C98DDEB5D66F5D424A0F6F4E6C8A8C8A91C93583E3ADF610`
- `Core_Study/exercises/L07_raii/validation/bad_support_shadow/owner.hpp`: SHA256 `A26BC54497E248B9779E15BB64D537C1CED7AA5154ABDB1AC67A3DAAA4CC1EA8`
- `Core_Study/exercises/L07_raii/validation/bad_support_shadow/resource_model.hpp`: SHA256 `52077757C98525DA001115334DBA2BCF24A851EAC79A46CECE1D3FCE25A70192`
- `Core_Study/exercises/cmake/StudySetup.cmake`: SHA256 `31184FF231653E03FDA46D257E482A147CD01DFEDF05D28CBC0FD3CD1D37F641`

## Stage 1: spec compliance

PASS.

The r2 include-shadow blocker is closed. `checks/owner_checks.hpp` includes `"support/resource_model.hpp"` before `<owner.hpp>`, so the checker fixes the trusted fixture before it consumes an implementation. Normal `src/student`, `src/reference`, and validation owner headers include the real fixture by relative path (`../../checks/support/resource_model.hpp`). The only local `"resource_model.hpp"` include left by static search is the intentional `validation/bad_support_shadow` negative case.

The L07 CMake include order is now support before implementation for Student, Reference, and validation targets. The generated `L07_raii_validation_bad_support_shadow.vcxproj` confirms include order as `Engineering_Study/exercises/include`, then `checks/support`, then `validation/bad_support_shadow`, then `checks`; the root public `check.hpp` directory is also inserted with `BEFORE` by `StudySetup.cmake`.

The new `bad_support_shadow` case attempts the old bypass and is rejected at compile time with `error C2011` for `l07_support::ResourceCounters`, `AcquisitionPlan`, and `ResourceHandle`, proving the fake fixture no longer silently replaces the trusted fixture.

## Stage 2: code quality and safety

PASS.

The functional L07 r2 checks are preserved: Reference/observation pass, Student placeholder fails through the real ownership checker, good validation passes, and the three runtime bad variants are rejected with their exact contract diagnostics. Static scan found no remaining `ResourceAttempt`, `implemented`, `student_ready`, `student_placeholder`, `WILL_FAIL`, `SKIP`, or hardcoded secret pattern in L07. The one local fake fixture include is the intentional shadow negative target.

No dedicated `lsp_diagnostics` tool was available in this worker surface. MSVC Debug/Release builds were used as compile/type diagnostics.

## Independent validation

Default configure:

```text
cmake -S Core_Study/exercises/L07_raii -B Core_Study/references/validation/reviews/c02-independent-evidence/l07-r3-default -G "Visual Studio 18 2026" -A x64
```

Result: passed with MSVC 19.51.36256.0.

Default Debug/Release build:

```text
cmake --build Core_Study/references/validation/reviews/c02-independent-evidence/l07-r3-default --config Debug
cmake --build Core_Study/references/validation/reviews/c02-independent-evidence/l07-r3-default --config Release
```

Result: both passed for `L07_raii_observation` and `L07_raii_reference`.

Default Debug/Release CTest:

```text
ctest --test-dir Core_Study/references/validation/reviews/c02-independent-evidence/l07-r3-default -C Debug --output-on-failure
ctest --test-dir Core_Study/references/validation/reviews/c02-independent-evidence/l07-r3-default -C Release --output-on-failure
```

Result: both passed 2/2.

Validation configure:

```text
cmake -S Core_Study/exercises/L07_raii -B Core_Study/references/validation/reviews/c02-independent-evidence/l07-r3-validation -G "Visual Studio 18 2026" -A x64 -DL07_RAII_BUILD_VALIDATION_VARIANTS=ON
```

Result: passed.

Validation build, excluding the intentional compile-fail shadow target:

```text
cmake --build Core_Study/references/validation/reviews/c02-independent-evidence/l07-r3-validation --config Debug --target L07_raii_validation_good L07_raii_validation_bad_noop L07_raii_validation_bad_fake_completed L07_raii_validation_bad_memberwise_move_order
cmake --build Core_Study/references/validation/reviews/c02-independent-evidence/l07-r3-validation --config Release --target L07_raii_validation_good L07_raii_validation_bad_noop L07_raii_validation_bad_fake_completed L07_raii_validation_bad_memberwise_move_order
```

Result: both passed. Release still emits unreachable-code warnings only in the deliberately failing no-op target after the first runtime check fails.

Validation direct runs, Debug and Release:

- `L07_raii_validation_good.exe`: exit 0, `L07_raii_validation_contract OK`
- `L07_raii_validation_bad_noop.exe`: exit 1, `check failed: normal owner must own first`
- `L07_raii_validation_bad_fake_completed.exe`: exit 1, `check failed: normal lifetime: second resource still alive`
- `L07_raii_validation_bad_memberwise_move_order.exe`: exit 1, `check failed: move assignment releases target old second first`

Shadow negative build:

```text
cmake --build Core_Study/references/validation/reviews/c02-independent-evidence/l07-r3-validation --config Debug --target L07_raii_validation_bad_support_shadow
```

Result: exit 1 with `error C2011` and `l07_support::ResourceCounters` redefinition, plus related `AcquisitionPlan` and `ResourceHandle` redefinitions.

Student/ref-off configure:

```text
cmake -S Core_Study/exercises/L07_raii -B Core_Study/references/validation/reviews/c02-independent-evidence/l07-r3-student-ref-off -G "Visual Studio 18 2026" -A x64 -DCORE_STUDY_TEST_STUDENTS=ON -DCORE_STUDY_BUILD_REFERENCE=OFF
```

Result: passed.

Student/ref-off build:

```text
cmake --build Core_Study/references/validation/reviews/c02-independent-evidence/l07-r3-student-ref-off --config Debug
cmake --build Core_Study/references/validation/reviews/c02-independent-evidence/l07-r3-student-ref-off --config Release
```

Result: both passed for observation and student. `ctest -N` listed only `L07_raii_observation` and `L07_raii_student`.

Reference target absent check:

```text
cmake --build Core_Study/references/validation/reviews/c02-independent-evidence/l07-r3-student-ref-off --config Debug --target L07_raii_reference
```

Result: exit 1, `MSB1009: 项目文件不存在`.

Student/ref-off CTest:

```text
ctest --test-dir Core_Study/references/validation/reviews/c02-independent-evidence/l07-r3-student-ref-off -C Debug --output-on-failure
ctest --test-dir Core_Study/references/validation/reviews/c02-independent-evidence/l07-r3-student-ref-off -C Release --output-on-failure
```

Result: both fail as expected at Student, with `check failed: normal owner must own first`; observation passes.

## Final recommendation

APPROVE the overall C02 sample gate for the reviewed sample scope: `00`, `04/L04`, and `07/L07`.

This approval does not cover bulk 08-10 expansion, whole-course integration, top-level navigation, or final Core_Study completion.
