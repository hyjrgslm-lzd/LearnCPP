## Verdict

- APPROVE for the L07 r3 sample gate technical/experimental slice.
- Scope: `Core_Study/chapters/07-raii-and-ownership.md`, `Core_Study/exercises/L07_raii/**`, current `Core_Study/exercises/cmake/StudySetup.cmake`, and L07 r3 evidence.
- Not covered here: `audit_student.py`, record-process r3/cwd, whole-course integration, and ASan refresh after the public helper change.

## Success Criteria Checked

- Fresh L07 leaf configure/build/test in Debug and Release.
- Student-only mode keeps Reference target absent and rejects the safe placeholder through ownership behavior.
- Public good/noop/fake-completed/memberwise-order validation variants still pass/fail with expected diagnostics.
- Public `bad_support_shadow` is rejected at compile time by expected fixture redefinition, not by a missing file or unrelated syntax error.
- Verifier-only r2-style support-shadow attack is rejected at compile time by the same expected redefinition path.
- Current `StudySetup.cmake` binds C01 `check.hpp` with `BEFORE PRIVATE`.
- Generated project include order places C01 check path and `checks/support` before implementation dirs.

## Fresh Source Fingerprints

- `Core_Study/chapters/07-raii-and-ownership.md`
  `5B1BBCA7CFC6F7DF50E9768E8D404681A5C9669854DAD26E95D7881169340D4E`
- `Core_Study/exercises/cmake/StudySetup.cmake`
  `31184FF231653E03FDA46D257E482A147CD01DFEDF05D28CBC0FD3CD1D37F641`
- `Core_Study/exercises/tools/record_process.py`
  `A405AF5089779E762A804E5135AA0FAA583773A3892696CB46E9D3A9DC927277`
- `Engineering_Study/exercises/include/check.hpp`
  `716138E42081AEE359E17929C31EE156ECFFD1D3B596E35107CFB08FF16C5F38`
- `Core_Study/exercises/L07_raii/CMakeLists.txt`
  `6838FB984FB6A03095D487592A88840874512C8D607C68C5DCD6A14BEA4714F4`
- `Core_Study/exercises/L07_raii/README.md`
  `4CB57317FE241A606C390B7F33927D13F3389D72AC9B22F3E77AB5D65FF476D3`
- `Core_Study/exercises/L07_raii/checks/owner_checks.hpp`
  `23250A6C7D7C2CCD448A2B90C5A9A53619A547D47F413316168FC5E6716A3568`
- `Core_Study/exercises/L07_raii/checks/support/resource_model.hpp`
  `E3CAC71C517F6CEDF95ECFAF2CD524AB75C57E77D418D58CCD0626ED458D8D35`
- `Core_Study/exercises/L07_raii/src/reference/owner.hpp`
  `2AB13F03A11033DEBF081AC80DFAF09278D42DBE351286F6F5A37EB498401A2D`
- `Core_Study/exercises/L07_raii/src/student/owner.hpp`
  `34D3063559032BDE19FDB4C2D12FDCA455D2068B7DDF01421671A4F3CEAE46BA`
- `Core_Study/exercises/L07_raii/validation/bad_support_shadow/resource_model.hpp`
  `52077757C98525DA001115334DBA2BCF24A851EAC79A46CECE1D3FCE25A70192`
- `Core_Study/exercises/L07_raii/validation/bad_support_shadow/owner.hpp`
  `A26BC54497E248B9779E15BB64D537C1CED7AA5154ABDB1AC67A3DAAA4CC1EA8`

## Environment

- OS: Microsoft Windows NT 10.0.26100.0
- PowerShell: 7.6.2
- Python: 3.10.11
- CMake: 4.2.3

## Evidence

- Fresh default build/test:
  - `l07-default-configure.json`: PASS, configured to `build/c02-verifier-l07-r3-default`.
  - `l07-default-build-debug.json`: PASS, built `L07_raii_observation.exe` and `L07_raii_reference.exe`.
  - `l07-default-build-release.json`: PASS, built `L07_raii_observation.exe` and `L07_raii_reference.exe`.
  - `l07-default-ctest-debug.json`: PASS, `100% tests passed`, observation/reference.
  - `l07-default-ctest-release.json`: PASS, `100% tests passed`, observation/reference.
- Fresh student isolation:
  - `l07-student-configure.json`: PASS with `CORE_STUDY_BUILD_REFERENCE=OFF`, `CORE_STUDY_TEST_STUDENTS=ON`.
  - `l07-student-build-debug.json`: PASS, built observation/student.
  - `l07-student-ctest-placeholder-fails.json`: recorder PASS for expected CTest exit 8; diagnostic `check failed: normal owner must own first`.
  - `l07-student-reference-target-absent.json`: recorder PASS for expected exit 1; diagnostic `MSB1009`.
- Fresh validation variants:
  - `l07-validation-configure.json`: PASS.
  - `l07-validation-build.json`: PASS, built good/noop/fake-completed/memberwise-order variants.
  - `l07-validation-good-run.json`: PASS, output `L07_raii_validation_contract OK`.
  - `l07-validation-bad-noop-run.json`: recorder PASS for expected exit 1; diagnostic `check failed: normal owner must own first`.
  - `l07-validation-bad-fake-completed-run.json`: recorder PASS for expected exit 1; diagnostic `check failed: normal lifetime: second resource still alive`.
  - `l07-validation-bad-memberwise-move-order-run.json`: recorder PASS for expected exit 1; diagnostic `check failed: move assignment releases target old second first`.
- Fixture shadow closure:
  - `l07-validation-bad-support-shadow-build-rejected.json`: recorder PASS for expected build exit 1; diagnostics include `error C2011`, `ResourceCounters`, `AcquisitionPlan`, and `ResourceHandle`, with declarations coming from `checks/support/resource_model.hpp`.
  - `l07-verifier-shadow-build-rejected.json`: verifier-only r2-style local `resource_model.hpp` shadow attempt; recorder PASS for expected build exit 1 with the same `error C2011` and l07_support type redefinition diagnostics.
- Include-order evidence:
  - `build/c02-verifier-l07-r3-student/L07_raii_student.vcxproj`: C01 check path first, then `checks/support`, then `src/student`, then `checks`.
  - `build/c02-verifier-l07-r3-validation/L07_raii_validation_bad_support_shadow.vcxproj`: C01 check path first, then `checks/support`, then `validation/bad_support_shadow`, then `checks`.
  - `build/c02-verifier-l07-r3-default/L07_raii_reference.vcxproj`: C01 check path first, then `checks/support`, then `src/reference`, then `checks`.

## Gaps

- `audit_student.py` and `student-wiring` remain a separate tool slice, as requested.
- ASan evidence under `references/validation/asan` is still bound to the older helper input; final whole-course ASan should be refreshed after the public helper change.
- This verdict does not claim protection against arbitrary malicious C++ such as deliberate process exit or macro rewriting of checker code; that is outside the approved course-contract scope.

## Risks

- No remaining L07 r3 technical/experimental blocker found in this bounded sample-gate scope.
