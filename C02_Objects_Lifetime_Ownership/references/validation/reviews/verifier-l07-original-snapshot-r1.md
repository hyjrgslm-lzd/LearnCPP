## Verdict

- REQUEST_CHANGES for the original L07 snapshot.
- Scope: `Core_Study/chapters/07-raii-and-ownership.md`, `Core_Study/exercises/L07_raii/**`, and `references/validation/author-owner-l07-final-*.json`.
- Stop condition: author started r2 repair while this verification was running, so this report binds only to the original snapshot fingerprints below and waits for r2 fresh verification.

## Snapshot Fingerprints

- `Core_Study/chapters/07-raii-and-ownership.md`
  `EFC68820863BDBDD525B9918DC6AFB679D2413B5BB855F1692D7EE2E2B50900B`
- `Core_Study/exercises/L07_raii/README.md`
  `870E6B3C14064CF400A83A806CB13003060C38FD32201361A9104B2BFCC21106`
- `Core_Study/exercises/L07_raii/CMakeLists.txt`
  `43EDFD7EDFA73EB629F1A14B8685743A503939B7C0F6731DD22E9AF332247131`
- `Core_Study/exercises/L07_raii/checks/owner_checks.hpp`
  `F43CE4DE6EFFC438C750D294AFFF07C39C520601E79B6B717F10515D79C21600`
- `Core_Study/exercises/L07_raii/src/reference/owner.hpp`
  `55A9B5A476FDB82A0FF1E88D61335442FC36B8406E54C8CA935F5A18C47BB460`
- `Core_Study/exercises/L07_raii/src/student/owner.hpp`
  `1BA28DA2893041C10214EFBB6A8F61FB76693B061F79DDF74968A9104E6FA1EF`
- `Core_Study/exercises/L07_raii/validation/good/owner.hpp`
  `AD7364CDED04A80A0C98E5781E66F072A87E41EF5885BC3AC25EFA85AB716B89`
- `Core_Study/exercises/L07_raii/validation/bad_noop/owner.hpp`
  `85C57212A6924ADFACAB54C505D5CF33BD9CB3A3D6A9E21B4FEC6420E66E1235`

## Passing Evidence

- Fresh default L07 configure/build/test:
  - `l07-default-configure.json`: PASS, Visual Studio 18 2026 x64 configure.
  - `l07-default-build-debug.json`: PASS, built `L07_raii_observation.exe` and `L07_raii_reference.exe`.
  - `l07-default-build-release.json`: PASS, built `L07_raii_observation.exe` and `L07_raii_reference.exe`.
  - `l07-default-ctest-debug.json`: PASS, `100% tests passed`, observation/reference.
  - `l07-default-ctest-release.json`: PASS, `100% tests passed`, observation/reference.
- Fresh student isolation:
  - `l07-student-configure.json`: PASS with `CORE_STUDY_BUILD_REFERENCE=OFF` and `CORE_STUDY_TEST_STUDENTS=ON`.
  - `l07-student-build-debug.json`: PASS, built observation/student.
  - `l07-student-ctest-placeholder-fails.json`: recorder PASS for expected CTest exit 8, stderr contains `check failed: student implementation still placeholder`.
  - `l07-student-reference-target-absent.json`: recorder PASS for expected exit 1, stdout contains `MSB1009`.
- Original public validation variants:
  - `l07-validation-good-run.json`: PASS, output `L07_raii_reference OK`.
  - `l07-validation-bad-noop-run.json`: recorder PASS for expected exit 1, stderr `check failed: student implementation still placeholder`.

## Blocking Evidence

- Checker can be bypassed by a fake-complete implementation:
  - Verifier-only variant: `Core_Study/references/validation/reviews/c02-l07-verifier-evidence-r1/fake-complete-bad/owner.hpp`.
  - It removes the placeholder gate, owns the reporting API, and fabricates the observed counters/events needed by `owner_checks.hpp`.
  - `l07-fake-complete-bad-build.json`: PASS, built the isolated target against the original `checks/reference_check.cpp`.
  - `l07-fake-complete-bad-run.json`: PASS, child exit 0, stdout `L07_raii_reference OK`.
  - This proves the original checker accepts a self-reported fake-complete implementation; the current public `bad_noop` only proves `student_ready()` fails, not that resource behavior is independently verified.
- Original `src/student/owner.hpp` owns `ResourceCounters`, `resource_counters()`, `resource_events()`, and `student_ready()` behavior. A student can make `student_ready()` pass and hand-fill resource reports without exercising a shared fixture.
- Original `src/reference/owner.hpp` recorded release events inside `noexcept` release/reset paths using `std::vector<std::string>::push_back` and string construction. Allocation failure there would call `std::terminate`, so the release path is not actually no-throw.
- Original acquire functions incremented alive/acquired counters before appending log strings. If event logging throws, model state can remain mutated without a complete event record.
- Original `two_resource_owner(AcquisitionPlan)` called `reset_resource_model(next_plan)` inside construction. That public constructor can reset global model state while another owner is still live, so it breaks multi-owner/resource accounting.

## Gaps

- I did not approve L07 sample chapter quality beyond the technical/experimental slice above.
- I did not run ASan for L07 Reference owner after the blocking checker/fixture issues were proven.
- A verifier-only `reference-global-reset-probe` was started after author r2 edits began; it no longer cleanly binds to the original snapshot and is not used as primary verdict evidence.

## Minimal Fix Required

- Move resource model and observation state into a shared checker-owned fixture that Student cannot redefine.
- Delete `student_ready()` and the ready gate as a validation boundary.
- Replace allocating event strings in no-throw cleanup paths with bounded/non-allocating event storage or make the failure boundary explicit outside `noexcept`.
- Ensure acquire logging cannot leave counters mutated without a consistent event/state record.
- Remove the `two_resource_owner(AcquisitionPlan)` constructor that resets global model state, or make plan setup external and invalid while owners are live.
- Add public fake-complete-bad plus first/second failure variants that must fail for the right resource-behavior reason, not only because a placeholder gate trips.
