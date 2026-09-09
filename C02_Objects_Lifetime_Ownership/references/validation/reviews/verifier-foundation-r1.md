# C02 verifier report — foundation batch L01/L02/L03/L05/L06 r1

## Verdict

APPROVE.

Checked frozen foundation scope: chapters `01-initialization.md`, `02-expressions-and-references.md`, `03-lifetime-and-borrowing.md`, `05-special-members.md`, `06-move-and-return.md`; exercises `L01_initialization`, `L02_value_categories`, `L03_lifetimes`, `L05_special_members`, `L06_move_return`; author evidence under `Core_Study/references/validation/author-foundation/`. I did not edit implementation/chapter files.

Fresh evidence directory: `Core_Study/references/validation/reviews/c02-foundation-verifier-evidence-r1/`.
Fresh build directory: `build/c02-foundation-verifier-r1/`.

Scope note: L01/L02/L06 are observation + compile-negative lessons and have no Student implementation target by design. L03/L05 are implementation exercises and were checked with Student/ref-off isolation.

## Source binding

`c02-foundation-verifier-evidence-r1/source-hashes-foundation.json` records the source snapshot. Key hashes:

- `Core_Study/chapters/01-initialization.md`: `B916EF9A444ED8560D681EF91789B1CD964AB775AC3B10B8E781A24DA8196AF1`
- `Core_Study/chapters/02-expressions-and-references.md`: `ECC1147F1224D21A0EE0569E5CEE80F1A5EEDD70240343C6014BAF280207CDA4`
- `Core_Study/chapters/03-lifetime-and-borrowing.md`: `777FF771D6157D47F152DE5C4B3531F739A3CC796B78F7D2D6016B9B00BFDAB8`
- `Core_Study/chapters/05-special-members.md`: `3F046116324986B308771F0F19162B903CDF3F3FEC64902A0D6B54E38F60922E`
- `Core_Study/chapters/06-move-and-return.md`: `B8351DB60E742201D8DC48A4CAC3F7CEF85FD65B733D5C1FFD1E2F8B55A95BEF`
- `L03_lifetimes/checks/lifetime_checks.cpp`: `FDD4968966B20248D948847F5623BFE108D2498EEC4118A72F54EBA9F19DA244`
- `L03_lifetimes/checks/support/lifetime_model.hpp`: `16EDA52FC484C57906FB37F2DCB8D4535762B75EACAA31F95E4F5EDBA61BCF61`
- `L05_special_members/checks/buffer_checks.cpp`: `7721B07A4D9E0C6CECE3437B0646C94A689737D865C98DAB784DCD31981282DC`
- `L05_special_members/checks/support/memory_model.hpp`: `28E0DDE087B762D85ED8AA75AE2530DFA9F59A99AFABD732B235F056A7CA00D4`
- `Core_Study/exercises/cmake/StudySetup.cmake`: `31184FF231653E03FDA46D257E482A147CD01DFEDF05D28CBC0FD3CD1D37F641`
- `Engineering_Study/exercises/include/check.hpp`: `716138E42081AEE359E17929C31EE156ECFFD1D3B596E35107CFB08FF16C5F38`

## Evidence

Fresh MSVC matrix:

- `l01-configure-default.json`, `l01-build-default-debug.json`, `l01-build-default-release.json`, `l01-ctest-default-debug.json`, `l01-ctest-default-release.json` — all PASS. CTest ran `L01_initialization_observation`, `L01_initialization_negative_narrowing`, and `L01_initialization_negative_constinit_dynamic`; Debug and Release both report `100% tests passed`.
- `l02-configure-default.json`, `l02-build-default-debug.json`, `l02-build-default-release.json`, `l02-ctest-default-debug.json`, `l02-ctest-default-release.json` — all PASS. CTest ran observation plus `bind_rvalue_to_nonconst_lvalue` and `forward_lvalue_to_rvalue`; Debug and Release both report `100% tests passed`.
- `l06-configure-default.json`, `l06-build-default-debug.json`, `l06-build-default-release.json`, `l06-ctest-default-debug.json`, `l06-ctest-default-release.json` — all PASS. CTest ran observation plus `return_named_deleted` and `const_move_only`; Debug and Release both report `100% tests passed`.
- `l03-configure-default.json`, `l03-build-default-debug.json`, `l03-build-default-release.json`, `l03-ctest-default-debug.json`, `l03-ctest-default-release.json` — all PASS. CTest ran reference, validation_good, and bad_dangling_view expected-failure wrapper in both configs.
- `l05-configure-default.json`, `l05-build-default-debug.json`, `l05-build-default-release.json`, `l05-ctest-default-debug.json`, `l05-ctest-default-release.json` — all PASS. CTest ran reference, validation_good, bad_shallow_copy expected-failure wrapper, and bad_move_leak expected-failure wrapper in both configs.

Direct negative compile proof for L01/L02/L06:

- `l01-negative-narrowing-direct-debug.json` — build exits 1 and emits MSVC `error C2397` with narrowing conversion text.
- `l01-negative-constinit_dynamic-direct-debug.json` — build exits 1 and emits `error C2127` mentioning `constinit`.
- `l02-negative-bind_rvalue_to_nonconst_lvalue-direct-debug.json` — build exits 1 and emits `error C2440`.
- `l02-negative-forward_lvalue_to_rvalue-direct-debug.json` — build exits 1 and emits `error C2664`.
- `l06-negative-return_named_deleted-direct-debug.json` — build exits 1 and emits `error C2280` / deleted function.
- `l06-negative-const_move_only-direct-debug.json` — build exits 1 and emits `error C2280`.

This confirms the negative tests are real configure/build attempts, not marker-only tests. `expect_build_failure.cmake` also fails if the negative build exits 0 or if the expected diagnostic text is absent.

L03/L05 public good/bad and Student isolation:

- `l03-good-debug.json`, `l03-good-release.json` — validation good executables exit 0. `l03-bad-L03_lifetimes_validation_bad_dangling_view-{debug,release}.json` exit 1 with `borrow must become invalid when owner lifetime ends`.
- `l05-good-debug.json`, `l05-good-release.json` — validation good executables exit 0. `l05-bad-L05_special_members_validation_bad_shallow_copy-{debug,release}.json` exit 1 with `copy must have independent storage`; `l05-bad-L05_special_members_validation_bad_move_leak-{debug,release}.json` exit 1 with `move assignment should release old target storage`.
- `l03/l05-configure-student-ref-off.json` — PASS with `-DCORE_STUDY_BUILD_REFERENCE=OFF -DCORE_STUDY_TEST_STUDENTS=ON`.
- `l03/l05-build-student-ref-off-{debug,release}.json` — Student targets build in Debug and Release.
- `l03/l05-student-placeholder-ref-off-{debug,release}.json` — Student placeholders run and fail with `check failed`; L03 fails at `borrow from live owner must be valid`, L05 fails at `copy must have independent storage`.
- `l03/l05-reference-target-absent-ref-off.json` and `ref-off-target-files.json` — Reference `.vcxproj` absent under ref-off builds; trying to build Reference exits 1 with localized MSBuild `项目文件不存在`.

Fixture and anti-fake-completion checks:

- `marker-search.txt` — no `student_ready`, `student_placeholder`, `placeholder_state`, `return report`, `ResourceAttempt`, `implemented`, or `completed` matches under L01/L02/L03/L05/L06 exercise trees.
- `trusted-header-scan.txt` — no bare `#include "lifetime_model.hpp"` / `#include "memory_model.hpp"` in implementation/reference/validation directories; all implementation-family headers use `#include "support/lifetime_model.hpp"` or `#include "support/memory_model.hpp"`.
- `checker-fixture-lines.txt` — L03 checker exercises live owner borrow, owner mutation, owner expiry, expired read exception, and closure borrow without ownership extension. L05 checker exercises deep copy, copy assignment target release, move construction transfer, move assignment target release, self-move, live_count zero, and invalid_releases zero.
- `shadow-support-build.json` — verifier-only implementation dirs containing fake `support/lifetime_model.hpp` and `support/memory_model.hpp` are rejected at compile time with both fake-header `#error` diagnostics. This proves a local fake support header does not silently pass as trusted fixture behavior.

Environment / mode:

- `compiler-extract.json` — CMake reports `MSVC 19.51.36256.0`, `cl.exe` from `D:/VisualStudio2026/Installed/VC/Tools/MSVC/14.51.36231/bin/Hostx64/x64/cl.exe`.
- `vcxproj-mode-include-lines.txt` — generated targets use `<LanguageStandard>stdcpplatest</LanguageStandard>` and `/Zc:__cplusplus`; public `Engineering_Study/exercises/include` is injected by `StudySetup.cmake`.
- `environment.json` — CMake version `4.2.3`; `windows-ver.txt` — Windows `10.0.26100.8655`.

## Gaps

- I did not run Clang/ASan for this foundation batch. The requested fresh behavior matrix here was MSVC Debug/Release; P1 ASan work is explicitly out of this slice.
- L03/L05 validation good executables produce no success marker text; their direct evidence is exit 0 plus CTest registration/pass. Bad and Student placeholder paths do have concrete `check failed` diagnostics.

## Risks

- L03/L05 trusted fixture APIs are visible to implementation code as part of the teaching exercises. This verification proves the current checks do not rely on student-filled reports or completion markers and that fake local support headers do not silently replace the trusted fixture. It does not claim adversarial sandboxing against arbitrary deliberate mutation of public support internals.
