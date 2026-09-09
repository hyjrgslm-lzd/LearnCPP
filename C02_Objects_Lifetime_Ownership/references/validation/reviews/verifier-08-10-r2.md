# C02 independent verifier report — Core 08-10 owner r2

## Verdict

APPROVE for the bounded 08-10 r2 technical/experiment slice.

The r2 evidence closes the prior L10 alias-assignment UAF for both copy and move assignment. L09 r2 now exercises real shared/weak object wiring instead of accepting student-filled result structs, and the public bad variants fail on the intended checker predicates. L08 README/interface repair was smoke-tested successfully. This approval is limited to the declared 08-10 r2 scope; it does not claim protection against arbitrary hostile C++ or replace root's later full-course integration/ASan run.

## Scope checked

- `Core_Study/references/validation/author-owner-08-10-r2-report.md`
- `Core_Study/exercises/L10_control_block/**`
- `Core_Study/exercises/L09_shared/**`
- `Core_Study/exercises/L08_unique/README.md` and normal smoke path
- Shared helper binding through the current `Core_Study/exercises/cmake/StudySetup.cmake` and `Core_Study/exercises/C01_hello_toolchain/checks/check.hpp`

Evidence directory: `Core_Study/references/validation/reviews/c02-0810-r2-verifier-evidence-r1/`

## Source binding

Fresh environment: `environment.json`

- `cwd`: `F:\CPPTrain\LearnCPP`
- `git_head`: `d81c136c342b62d66ceb8767ffc581e12ca2f5d2`
- CMake: `cmake version 4.2.3`
- MSVC generator: `Visual Studio 18 2026 -A x64`
- Clang ASan probe compiler: `clang version 22.1.3`
- Python: `Python 3.10.11`
- ASan child-process settings used by the repro cmd files: `PATH` includes `D:\VisualStudio2026\Installed\VC\Tools\Llvm\x64\bin` and `...\lib\clang\22\lib\windows`; `ASAN_OPTIONS=halt_on_error=1:exitcode=1`.

Fresh source hashes: `source-hashes.json`

- `author-owner-08-10-r2-report.md`: `1731AEF0CFFDE5B385B2B75F0111C8DF79701CF60FA68FB0F42F1D022C4899F4`
- `L10_control_block/src/reference/rc.hpp`: `3505F0BB7A87E4A16A27B32872FDB0DAD1DB185EDCE671E5FCA04A30CC296E26`
- `L10_control_block/checks/rc_checks.hpp`: `AD5B6412DB4069F1F7211D666FED86B26CC65DDE5D6C991903DC51EB11BE11B1`
- `L09_shared/src/reference/owner.hpp`: `08297578A2DF9FB1FDEBE9B086D2305CAD304659B2000A794827EFEAADA14FCE`
- `L09_shared/src/student/owner.hpp`: `5CFAEC4B53F18E84589602861F567F607C20F0A736164FF43497BBED056513E2`
- `L09_shared/checks/owner_checks.hpp`: `669FCE0836100F4D425A2EF9E53E2EC8326BA8A02F51D68653DAF993C301ED6B`
- `L09_shared/checks/support/shared_support.hpp`: `8F15BC32CFEC598B8F04A60085B3F24AB45B2C0FF3848316724E570BD202884F`
- `L08_unique/README.md`: `BB3BBB56B92C640A47AAC5D0AC46B5DF8F64894CE2D308895751EA5831B7826E`
- `StudySetup.cmake`: `31184FF231653E03FDA46D257E482A147CD01DFEDF05D28CBC0FD3CD1D37F641`
- `check.hpp`: `716138E42081AEE359E17929C31EE156ECFFD1D3B596E35107CFB08FF16C5F38`

## Evidence

### L10 alias assignment UAF closure

The original r1 repro shape was rebuilt unchanged in spirit with a nested RHS object:

```cpp
struct Node { int value; l10::rc_ptr<Node> child; };
auto p = l10::make_rc<Node>(1);
p->child = l10::make_rc<Node>(2);
p = p->child;
p = std::move(p->child);
```

Fresh ASan results:

- `build-alias-asan-r2.json` — PASS, clang-cl build exited 0 and recorded `clang version 22.1.3`.
- `run-copy-asan-r2.json` — PASS, process exit 0, stdout contains `copy alias assignment OK`, no ASan failure text.
- `run-move-asan-r2.json` — PASS, process exit 0, stdout contains `move alias assignment OK`, no ASan failure text.

Source inspection confirms the repair shape:

- `l10-r2-assignment-lines.txt` — copy assignment stores `auto* incoming = other.block_` before `release()`; move assignment uses `std::exchange(other.block_, nullptr)` before `release()`.
- `rc-private-lines.txt` — `rc_ptr(control_block_base*, bool)` is under `private:` at `rc.hpp:114-118`.
- `build-private-adopt-ctor-r2.json` — PASS as a negative compile proof: external `l10::rc_ptr<int> p(nullptr, false);` exits 1 with `calling a private constructor` and points to `rc.hpp(118)`.

L10 normal matrix:

- `l10-configure-default-r2.json` — PASS, configured `L10_CONTROL_BLOCK_BUILD_VALIDATION_VARIANTS=ON`.
- `l10-build-default-debug-r2.json` — PASS, Debug clean build.
- `l10-ctest-default-debug-r2.json` — PASS, `100% tests passed`.
- `l10-build-default-release-r2.json` — PASS, Release clean build.
- `l10-ctest-default-release-r2.json` — PASS, `100% tests passed`.

### L09 real-object interface and wiring

Source inspection:

- `l09-r2-interface-lines.txt` — author report and checker show `connect_parent_child(parent, child)` and `shared_from_existing(node&)`; old student-returned `CycleResult`/`EnableResult` path is gone from exercise sources.
- `l09-legacy-marker-search.json` — `rg` over `Core_Study/exercises/L09_shared` exits 1 with no matches for `CycleResult`, `EnableResult`, `build_parent_child_without_cycle`, `shared_from_this_result`, `student_ready`, `student_placeholder_state`, or `placeholder_state`.
- `owner_checks.hpp` creates real nodes and checks observable state: `parent owns child through shared next`, weak parent expiration after graph scope, and `shared_from_existing(*owner)` ownership identity.

Fresh public good/bad results:

- `l09-configure-default-r2.json` — PASS, configured `L09_SHARED_BUILD_VALIDATION_VARIANTS=ON`.
- `l09-build-default-debug-r2.json` — PASS, Debug clean build.
- `l09-ctest-default-debug-r2.json` — PASS, `100% tests passed`.
- `l09-run-L09_shared_validation_good-debug-r2.json` — PASS, exit 0, `L09_shared_validation_contract OK`.
- `l09-run-L09_shared_validation_bad_hardcoded_noop-debug-r2.json` — PASS, exit 1, required diagnostic `parent owns child through shared next`.
- `l09-run-L09_shared_validation_bad_cycle-debug-r2-corrected.json` — PASS, exit 1, required diagnostic `parent weak expires after graph scope`.
- `l09-run-L09_shared_validation_bad_weak_resurrect-debug-r2.json` — PASS, exit 1, required diagnostic `lock fails after destruction`.
- `l09-build-default-release-r2.json` — PASS, Release clean build.
- `l09-ctest-default-release-r2.json` — PASS, `100% tests passed`.
- `l09-run-L09_shared_validation_good-release-r2.json` — PASS, exit 0, `L09_shared_validation_contract OK`.
- `l09-run-L09_shared_validation_bad_hardcoded_noop-release-r2.json` — PASS, exit 1, required diagnostic `parent owns child through shared next`.
- `l09-run-L09_shared_validation_bad_cycle-release-r2-corrected.json` — PASS, exit 1, required diagnostic `parent weak expires after graph scope`.
- `l09-run-L09_shared_validation_bad_weak_resurrect-release-r2.json` — PASS, exit 1, required diagnostic `lock fails after destruction`.

Student/ref-off wiring:

- `l09-configure-student-ref-off-r2.json` — PASS, configured `CORE_STUDY_BUILD_REFERENCE=OFF` and `CORE_STUDY_TEST_STUDENTS=ON`.
- `l09-build-student-ref-off-debug-r2.json` — PASS, Debug builds `L09_shared_student` without reference target.
- `l09-student-placeholder-ref-off-debug-r2.json` — PASS, placeholder student executable exits 1 on real checker predicate `make_node returns shared owner`.
- `l09-build-student-ref-off-release-r2.json` — PASS, Release builds `L09_shared_student` without reference target.
- `l09-student-placeholder-ref-off-release-r2.json` — PASS, placeholder student executable exits 1 on `make_node returns shared owner`.
- `l09-reference-target-absent-ref-off-r2.json` — PASS, building `L09_shared_reference` in ref-off tree exits 1 with CMake/MSBuild missing-project diagnostic.

Two earlier `bad_cycle` wrapper attempts remain in the evidence directory and failed only because I used the old expected string. Corrected records above match the r2 author report and current checker source.

### L08 README/interface smoke

- `l08-configure-smoke-r2.json` — PASS, configured `L08_UNIQUE_BUILD_VALIDATION_VARIANTS=ON`.
- `l08-build-smoke-debug-r2.json` — PASS, Debug clean build.
- `l08-ctest-smoke-debug-r2.json` — PASS, `100% tests passed`.
- `l08-good-smoke-debug-r2.json` — PASS, good validation executable exits 0 with `L08_unique_validation_contract OK`.

## Gaps

- L08 was intentionally limited to README/interface smoke for r2 because the implementation matrix was already covered in the prior approved 08-10 technical pass and the r2 task called out only README/interface repair for L08.
- I did not run the final full-course integration/ASan matrix. Root explicitly owns that later pass and this slice only rechecked the targeted L10 ASan regression plus L09/L08 impacted paths.

## Risks

- This verifies declared lesson contracts, fixture loading, student/ref-off wiring, and targeted ownership regressions. It is not a C++ sandbox against arbitrary malicious code such as deliberate process exit, macro sabotage, or compiler-specific undefined-behavior tricks outside the stated contract.
