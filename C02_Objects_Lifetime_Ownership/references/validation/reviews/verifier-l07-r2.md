## Verdict

- REQUEST_CHANGES for the L07 r2 sample gate.
- Behavioral contract checks passed for the shipped good/reference paths and the shipped public bad variants.
- Blocking issue: the trusted fixture can still be shadowed because implementation include directories precede `checks/support`. A verifier-only fake support layer passed `validation_check.cpp`.

## Success Criteria Checked

- Fresh L07 leaf configure/build/test in Debug and Release.
- Reference/observation tests pass.
- Student-only build excludes the Reference target and the placeholder fails through the real ownership checker.
- Public validation variants cover good, no-op bad, fake-completed bad, and memberwise-move-order bad.
- Checker covers first/second acquire failure, multiple owners, reset, self move, move construction, and move assignment release order.
- Fixture uses bounded event storage and validates resource kind/id/repeated release.
- Student no longer defines counters/events in the current checked-in `src/student`.
- New fake-complete/shadow bypass attempt was tested against the r2 API.

## Fresh Source Fingerprints

- `Core_Study/chapters/07-raii-and-ownership.md`
  `5B1BBCA7CFC6F7DF50E9768E8D404681A5C9669854DAD26E95D7881169340D4E`
- `Core_Study/exercises/L07_raii/README.md`
  `AA82217B2AE7ED656390C01D452AD4427B31C081589E9507C759A3341B2D1ECE`
- `Core_Study/exercises/L07_raii/CMakeLists.txt`
  `D10D13864C293D28E53B7187BCD6A85D9123784C5AF12717DF5631146D2CC8AF`
- `Core_Study/exercises/L07_raii/checks/support/resource_model.hpp`
  `E3CAC71C517F6CEDF95ECFAF2CD524AB75C57E77D418D58CCD0626ED458D8D35`
- `Core_Study/exercises/L07_raii/checks/owner_checks.hpp`
  `4D10652E27375E7671BF201FE07EEDADC7654608998A46D925EFE77D1364C9D7`
- `Core_Study/exercises/L07_raii/checks/validation_check.cpp`
  `6CD4C833D4977FD11520611A4AE33D3B03E5ED2E4BF3F8222C78C8FB054F40B5`
- `Core_Study/exercises/L07_raii/src/reference/owner.hpp`
  `F94BEF9E929274C663CB427B6DD00968B872D6DC2260CDB3E353E358E58821DF`
- `Core_Study/exercises/L07_raii/src/student/owner.hpp`
  `CE2E64DB99D26509443A5B9DBA844382E55ABEEEAE6AF309F4AD65EE4D074BD0`
- `Core_Study/exercises/L07_raii/validation/good/owner.hpp`
  `AD7364CDED04A80A0C98E5781E66F072A87E41EF5885BC3AC25EFA85AB716B89`
- `Core_Study/exercises/L07_raii/validation/bad_noop/owner.hpp`
  `222EBBE8A841892B7F5A7B56778ABDF84674B5CD08A4C9414809337252BC5C8B`
- `Core_Study/exercises/L07_raii/validation/bad_fake_completed/owner.hpp`
  `041829290FC211B5DD650A8951DC3390E200D2758BD453F26286289AB774594D`
- `Core_Study/exercises/L07_raii/validation/bad_memberwise_move_order/owner.hpp`
  `4886C4585F67A1201C86EC55C8BF743C355453093E5964A7FA8A12EB65026BD1`

## Environment

- OS: Microsoft Windows NT 10.0.26100.0
- PowerShell: 7.6.2
- Python: 3.10.11
- CMake: 4.2.3
- Clang: 22.1.3, `D:\VisualStudio2026\Installed\VC\Tools\Llvm\x64\bin\clang++.exe`

## Evidence

- `l07-default-configure.json`: PASS, configured `Core_Study/exercises/L07_raii` to `build/c02-verifier-l07-r2-default`.
- `l07-default-build-debug.json`: PASS, built `L07_raii_observation.exe` and `L07_raii_reference.exe`.
- `l07-default-build-release.json`: PASS, built `L07_raii_observation.exe` and `L07_raii_reference.exe`.
- `l07-default-ctest-debug.json`: PASS, Debug CTest contains observation/reference and `100% tests passed`.
- `l07-default-ctest-release.json`: PASS, Release CTest contains observation/reference and `100% tests passed`.
- `l07-student-configure.json`: PASS with `CORE_STUDY_BUILD_REFERENCE=OFF` and `CORE_STUDY_TEST_STUDENTS=ON`.
- `l07-student-build-debug.json`: PASS, built observation/student.
- `l07-student-ctest-placeholder-fails.json`: recorder PASS for expected CTest exit 8; diagnostic `check failed: normal owner must own first`.
- `l07-student-reference-target-absent.json`: recorder PASS for expected exit 1; diagnostic `MSB1009`.
- `l07-validation-build.json`: PASS, Debug built good/noop/fake-completed/memberwise-order variants.
- `l07-validation-good-run.json`: PASS, output `L07_raii_validation_contract OK`.
- `l07-validation-bad-noop-run.json`: recorder PASS for expected exit 1; diagnostic `check failed: normal owner must own first`.
- `l07-validation-bad-fake-completed-run.json`: recorder PASS for expected exit 1; diagnostic `check failed: normal lifetime: second resource still alive`.
- `l07-validation-bad-memberwise-move-order-run.json`: recorder PASS for expected exit 1; diagnostic `check failed: move assignment releases target old second first`.
- `l07-validation-build-release.json`: PASS, Release built all four validation variants.
- `l07-validation-good-run-release.json`: PASS, output `L07_raii_validation_contract OK`.
- `l07-validation-bad-noop-run-release.json`: recorder PASS for expected exit 1; diagnostic `check failed: normal owner must own first`.
- `l07-validation-bad-fake-completed-run-release.json`: recorder PASS for expected exit 1; diagnostic `check failed: normal lifetime: second resource still alive`.
- `l07-validation-bad-memberwise-move-order-run-release.json`: recorder PASS for expected exit 1; diagnostic `check failed: move assignment releases target old second first`.

## Blocking Evidence

- `build/c02-verifier-l07-r2-student/L07_raii_student.vcxproj` has include order:
  `src/student;checks/support;checks;...`.
- `build/c02-verifier-l07-r2-validation/L07_raii_validation_bad_fake_completed.vcxproj` has the same pattern:
  `validation/bad_fake_completed;checks/support;checks;...`.
- `owner_checks.hpp` includes `<owner.hpp>` and `<resource_model.hpp>`. With the implementation directory first, an implementation directory can provide its own `resource_model.hpp` and shadow the intended trusted fixture.
- Verifier-only bypass:
  - Files: `Core_Study/references/validation/reviews/c02-l07-verifier-evidence-r2/fake-support-shadow/owner.hpp` and `resource_model.hpp`.
  - Build: `l07-fake-support-shadow-build-r2.json`, PASS.
  - Run: `l07-fake-support-shadow-run-r2.json`, PASS, child exit 0, stdout `L07_raii_validation_contract OK`.
- The first shadow attempt without faked unwind evidence failed at `second acquire failure: first resource still alive`, then the shadow fixture fabricated the expected `release_first 1` event and passed. This is a direct proof that the report source, not the owner behavior alone, can still be controlled when `resource_model.hpp` is shadowable.

## Gaps

- I did not run ASan for L07 because the trust-boundary blocker is sufficient to stop the sample gate.
- I did not verify the new `audit_student.py` / `student-wiring` tool slice in this report. If that audit is intended to be mandatory before accepting any Student/validation result, it must be verified and wired into the acceptance path before this sample gate can pass.

## Minimal Fix Required

- Make `checks/support/resource_model.hpp` non-shadowable in the compile path. Minimal options:
  - put `checks/support` before implementation include directories for all L07 targets, or
  - include the fixture by a non-shadowable relative path from checker code, and keep implementation include dirs from supplying support headers.
- Keep or enforce an audit that rejects student/validation directories containing `resource_model.hpp` or other checker/fixture replacements.
- Re-run the same good/bad matrix plus a shadow attempt after the include/audit fix.
