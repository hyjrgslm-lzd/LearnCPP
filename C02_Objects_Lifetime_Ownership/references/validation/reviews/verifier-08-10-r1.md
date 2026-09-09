# C02 verifier report — Core Study 08-10 technical/experiment slice r1

## Verdict

APPROVE.

Checked scope: `Core_Study/chapters/08-unique-ownership.md`, `09-shared-ownership.md`, `10-control-block.md`, `Core_Study/exercises/L08_unique/**`, `L09_shared/**`, `L10_control_block/**`, and author report `author-owner-08-10-r1-report.md`. I did not edit implementation or chapter files. New evidence is under `Core_Study/references/validation/reviews/c02-0810-verifier-evidence-r1/` and build output is under `build/c02-0810-verifier-r1/`.

This verdict covers the declared RAII/shared/control-block contracts and wiring checks. It does not claim to detect arbitrary malicious C++ or copied algorithms.

## Source binding

`c02-0810-verifier-evidence-r1/source-hashes-0810.json` records the source snapshot. Key hashes:

- `Core_Study/chapters/08-unique-ownership.md`: `108F067583D9A5B0CE5A5634DFBFE824018C503613CC35BE723EE3E1D52656EF`
- `Core_Study/chapters/09-shared-ownership.md`: `5D89380EDEC7A8EB36EC390D4ADEAFDD7831682A6120C8FAD20D33A0389F98A9`
- `Core_Study/chapters/10-control-block.md`: `AD9D0B637EEB512B03C5B68198B321592CCEBE34F4868D5C31450643E65CA6DE`
- `L08_unique/checks/owner_checks.hpp`: `882F517BADEEA42A3D7668476A10A54D8135FCCA08BC296369474FF791D72691`
- `L09_shared/checks/owner_checks.hpp`: `BC2B200F08D74B63E7AF42477B6BCB12C1207369984A7B014F3257B65E5DFED9`
- `L10_control_block/checks/rc_checks.hpp`: `D79869745DAEA612D94EA9B0339AA54AA91A730D8DCE81472F83905FB3FF3FA4`
- `L08_unique/src/student/owner.hpp`: `E8EB8687135FE681BB96EDDE568A5012803937DDD5D7270DDA71DE920D5A14B0`
- `L09_shared/src/student/owner.hpp`: `6039DB92115200467179A94765CECB7BE5ABAB401E1A3D3BEB63092BDD8B4611`
- `L10_control_block/src/student/rc.hpp`: `196586EAB54B5988EADC3ED219571F7379F450B1126B5D093D811E4D5C5D9D8C`
- `Core_Study/exercises/cmake/StudySetup.cmake`: `31184FF231653E03FDA46D257E482A147CD01DFEDF05D28CBC0FD3CD1D37F641`
- `Engineering_Study/exercises/include/check.hpp`: `716138E42081AEE359E17929C31EE156ECFFD1D3B596E35107CFB08FF16C5F38`

## Commands / evidence

Fresh default builds with validation variants:

- `l08-configure-default.json`, `l08-build-default-debug.json`, `l08-build-default-release.json`, `l08-ctest-default-debug.json`, `l08-ctest-default-release.json` — all PASS. Debug and Release CTest both contain `100% tests passed`.
- `l09-configure-default.json`, `l09-build-default-debug.json`, `l09-build-default-release.json`, `l09-ctest-default-debug.json`, `l09-ctest-default-release.json` — all PASS. Debug and Release CTest both contain `100% tests passed`.
- `l10-configure-default.json`, `l10-build-default-debug.json`, `l10-build-default-release.json`, `l10-ctest-default-debug.json`, `l10-ctest-default-release.json` — all PASS. Debug and Release CTest both contain `100% tests passed`.

Public good/bad validation:

- L08 good Debug/Release: `L08_unique_validation_contract OK`. Bad noop Debug/Release exits 1 with `check failed`; bad release-keeps-owner Debug/Release exits 1 with `release leaves unique_ptr empty`.
- L09 good Debug/Release: `L09_shared_validation_contract OK`. Bad cycle Debug/Release exits 1 with `weak backedge does not keep cycle alive`; bad weak-resurrect Debug/Release exits 1 with `weak lock fails after destruction`.
- L10 good Debug/Release: `L10_control_block_validation_contract OK`. Bad leak-control-block Debug/Release exits 1 with `control block still alive`; bad weak-resurrect Debug/Release exits 1 with `object destroyed after last strong`.

Student / Reference OFF isolation:

- `l08/l09/l10-configure-student-ref-off.json` — PASS with `-DCORE_STUDY_BUILD_REFERENCE=OFF -DCORE_STUDY_TEST_STUDENTS=ON`.
- `l08/l09/l10-build-student-ref-off.json` and `*-release.json` — PASS for Student targets in Debug and Release.
- `l08/l09/l10-student-placeholder-ref-off.json` and `*-release.json` — Student placeholders run and fail with `check failed`, so build success / placeholder state is not accepted as completion.
- `ref-off-target-files.json` — each ref-off build has the Student `.vcxproj` and lacks the Reference `.vcxproj`.
- `l08/l09/l10-reference-target-absent-ref-off-corrected.json` — trying to build the Reference target exits 1 with localized MSBuild `项目文件不存在`.

Fixture loading / Student wiring:

- `marker-search.txt` — no `student_ready`, `student_placeholder`, `placeholder_state`, `report`, `completed`, or `TODO` markers found under L08/L09/L10 exercises.
- `checker-fixture-lines.txt` — checkers include real fixture first via `#include "support/..."`, then consume the implementation via `<owner.hpp>` or `<rc.hpp>`. Student/reference/validation implementation headers include fixture with `../../checks/support/...`.
- `shadow-fixture-project` — verifier-only project placed fake `support/*.hpp` headers with `#error` ahead of real include directories. `shadow-fixture-build-debug.json` and `shadow-fixture-l08/l09/l10-run.json` all PASS, proving the real fixture was loaded through fixed checker/source-relative paths rather than a shadowable include search path.
- `vcxproj-include-lines.txt` — generated Student/validation projects put public `Engineering_Study/exercises/include` first, then lesson `checks/support`, current implementation dir, and `checks`; this binds the current public `check.hpp` helper behavior.

Contract coverage inspected in checker source:

- L08 `owner_checks.hpp` covers basic unique lifetime, release transfer, reset deletes old object then owns replacement, construction failure cleanup, array owner, incomplete owner move/destruction.
- L09 `owner_checks.hpp` covers shared copy/use_count/destruction, aliasing pointer keeping control block alive, weak backedge cycle break, expired weak lock failure, and `enable_shared_from_this` same-object/use_count behavior.
- L10 `rc_checks.hpp` covers last strong destroying object/control block, strong copy/move/reset/self assignment, weak keeping control block after object destruction, weak copy/move/reset, lock no resurrection, and construction failure cleanup.
- `l10-reference-release-lines.txt` records reference release logic: strong release destroys object at strong zero, drops implicit weak, deletes block when weak reaches zero; weak release deletes block only when both strong and weak are zero.

## Gaps

- The first Reference OFF absence records `l08/l09/l10-reference-target-absent-ref-off.json` are preserved but have wrapper verdict FAIL because I required English `does not exist`; MSBuild output here is Chinese. Corrected records use `项目文件不存在` and pass.
- This pass uses MSVC / Visual Studio 18 2026. I did not rerun the three lessons under Clang/ASan; root explicitly assigned P1 ASan debugging elsewhere.

## Risks

- The trusted fixture counters are visible C++ support APIs, especially for the teaching `rc_ptr` exercise where implementation must call control-block hooks. The verification proves declared contract behavior, public bad rejection, and fixture loading. It does not attempt to make the exercise an adversarial sandbox against a student deliberately mutating support internals.
