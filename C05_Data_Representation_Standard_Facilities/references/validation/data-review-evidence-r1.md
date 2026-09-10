# C05 data/P1/public build verification evidence r1

Date: 2026-09-10
Reviewer role: non-author verifier
Scope: data/serialization/P1/public build lane only.

## Verdict

APPROVE for this lane.

The current scoped source passed fresh targeted verification for L15 schema evolution, P1 resource manifest, student ref-off wiring, and a reviewer-owned probe covering representative serialization/P1 integration risks. Static inspection found no `__has_include` bypass in scoped data/public-build files. Historical P1 failures are preserved as author evidence and were rechecked against the current fixed source.

## Fresh commands run by reviewer

Working directory for build/test commands:

`F:\CPPTrain\LearnCPP\C05_Data_Representation_Standard_Facilities\exercises`

### L15 leaf

```text
cmake -S L15_schema_evolution -B build/data-review/leaf-L15 -G "Visual Studio 18 2026" -A x64 -DDATA_STUDY_BUILD_REFERENCE=ON -DDATA_STUDY_TEST_STUDENTS=OFF
exit=0

cmake --build build/data-review/leaf-L15 --config Release --parallel 1
exit=0

ctest --test-dir build/data-review/leaf-L15 -C Release --output-on-failure
3/3 tests passed
exit=0
```

Coverage bound: L15 reference/good/bad leaf only. This proves the current schema-evolution checker accepts reference and independent good, and rejects the representative bad implementation in Release.

### P1 leaf after cleanup-parent and empty-zone fixes

```text
cmake -S P1_resource_manifest -B build/data-review/leaf-P1-r3 -G "Visual Studio 18 2026" -A x64 -DDATA_STUDY_BUILD_REFERENCE=ON -DDATA_STUDY_TEST_STUDENTS=OFF
exit=0

cmake --build build/data-review/leaf-P1-r3 --config Release --parallel 1
exit=0

ctest --test-dir build/data-review/leaf-P1-r3 -C Release --output-on-failure
100% tests passed, 0 tests failed out of 3
P1_resource_manifest_reference ................. Passed
P1_resource_manifest_validation_good ........... Passed
P1_resource_manifest_validation_bad_rejected ... Passed
exit=0

build\data-review\leaf-P1-r3\Release\P1_resource_manifest_validation_bad.exe
check failed: creates the package file
exit=1
```

Coverage bound: P1 reference/good/bad leaf only. The direct bad run fails at the intended behavioral diagnostic after cleanup checks, not at the old temp-parent cleanup problem.

### Reviewer-owned probe

Probe source path:

`exercises/build/data-review/probe/probe.cpp`

Probe checks:

- Duplicate record ID constructed from independent golden wire bytes is rejected by `decode_manifest`.
- L15 v1 reader consumes the v2 empty-note golden and returns the v1 manifest shape.
- P1 reference pipeline writes/reads the package, does not create/open the manifest `resource_path`, and removes its scratch directory.

Commands:

```text
cmake -S build/data-review/probe -B build/data-review/probe-build -G "Visual Studio 18 2026" -A x64
exit=0

cmake --build build/data-review/probe-build --config Release --parallel 1
exit=0

build\data-review\probe-build\Release\c05_data_review_probe.exe
data probe passed
exit=0
```

### Student ref-off wiring

```text
cmake -S . -B build/data-review/student -G "Visual Studio 18 2026" -A x64 -DDATA_STUDY_BUILD_REFERENCE=OFF -DDATA_STUDY_TEST_STUDENTS=ON -DDATA_STUDY_ENABLE_ICU=OFF -DDATA_STUDY_ENABLE_FRONTIER=OFF
exit=0

cmake --build build/data-review/student --config Release --parallel 1
exit=0

ctest --test-dir build/data-review/student -C Release --output-on-failure
19 tests total, 7 expected student failures: L03, L06, L08, L13, L14, L15, P1
exit=1
```

Evidence interpretation: `DATA_STUDY_BUILD_REFERENCE=OFF` did not build reference/good targets in this build, while student targets were registered and failed in unfinished initial state. Observation/capability/negative tests present in the student configuration passed.

### Integration evidence reviewed

Author/root evidence read, not trusted alone:

```text
references/validation/integration-ctest-r2.json
command: ctest --test-dir build/integration-r1 -C Release --output-on-failure
exit_code: 0
33/33 tests passed
```

P1 historical failure/fix evidence read:

```text
references/validation/p1-temp-repro-run-r1.json
stderr: scratch parent check: expected "...Temp\" actual "...Temp"
stderr: check failed: cleanup path remains owned temp child

references/validation/p1-owned-temp-cleanup-r1.json
old owned temp dirs were removed after absolute parent check

references/validation/p1-empty-zone-repro-run-r1.json
stderr: check failed: empty manifest validates configured timezone

references/validation/p1-empty-zone-fix-ctest-r1.json
P1_resource_manifest_reference/good/bad_rejected 3/3 passed
```

## Static inspection evidence

### Manifest encoder/decoder

`exercises/include/c05/manifest.hpp`

- `max_package_bytes` append guard: line 67.
- `validate_manifest`: line 137.
- duplicate id validation: line 151.
- `encode_manifest` calls validation before writing: lines 174 and 180.
- `decode_manifest` entry and 1 MiB cap: lines 234 and 236.
- record body bound check: line 273.
- duplicate known field checks: line 306.
- note duplicate check: line 378.
- duplicate unknown tag check: line 389.
- required id check: line 401.

### L15 independent schema evidence

`exercises/L15_schema_evolution/checks/schema_checks.cpp`

- includes independent golden fixture: line 3.
- v1 writer matches hand-written golden bytes: line 105.
- v2 empty-note writer matches hand-written golden bytes: line 108.
- v2 reader consumes v1 golden: line 111.
- trailing package data rejected: lines 124-126.
- duplicate known field rejected: lines 136-139.
- wrong scalar width rejected: line 148.
- oversized wire string rejected: line 152.
- oversized package rejected: line 154.
- duplicate unknown field rejected: line 160.
- future minor bounded unknown field skipped: line 166.
- v1 reader skips v2 note field: line 171.
- duplicate ids rejected: line 184.
- oversized string rejected: line 193.

### P1 file/cleanup behavior

`exercises/P1_resource_manifest/checks/pipeline_checks.cpp`

- temp directory is canonicalized and trailing empty path element removed: lines 19-23.
- package file creation verified: line 38.
- invalid config no-output check: line 57.
- empty manifest invalid-zone no-output check: lines 58-60.
- invalid manifest no-output check: lines 61-63.
- existing output rejected and preserved: lines 64-69.
- missing output dir reports IO error: lines 70-71.
- missing bounded input reports IO error: lines 72-73.
- cleanup target constrained to an absolute direct child of temp parent: lines 77-83.
- cleanup must pass before normal accumulated failures are emitted: lines 82-85.

`exercises/P1_resource_manifest/provided/operations.hpp`

- `write_new` uses no-replace output and bounded write path: lines 16-30.
- `read_bounded` uses bounded real file input: lines 32-48.
- `render_report` validates empty-manifest zone through `format_timestamp(0, zone)`: lines 51-55.

`exercises/P1_resource_manifest/src/reference/pipeline.hpp`

- reference path renders source, encodes, `write_new`, `read_bounded`, decodes, renders decoded result: lines 8-24.

`exercises/P1_resource_manifest/validation/good/pipeline.hpp`

- good implementation separately validates source, renders preview, encodes, writes, reads, decodes, renders decoded result: lines 8-24.

`exercises/P1_resource_manifest/validation/bad/pipeline.hpp`

- representative bad avoids the real write/read round trip and is rejected by checker.

### Public build configuration

`exercises/cmake/StudySetup.cmake`

- `DATA_STUDY_BUILD_REFERENCE`, `DATA_STUDY_TEST_STUDENTS`, `DATA_STUDY_ENABLE_FRONTIER` options: lines 4-7.
- reference/good/bad variants only added under reference build option: lines 74-75.
- student variant gated by `DATA_STUDY_TEST_STUDENTS`: lines 84 and 97.
- bad variants are wrapped by `expect_failure.cmake`: lines 100-103.
- ICU default option: line 112.

`exercises/cmake/ICU.cmake`

- requesting ICU target while ICU option is off is fatal: lines 4-5.
- ICU package is pinned to 77.1 exact components: line 12.
- matching Windows DLLs are required and copied per target: lines 46 and 49.

`exercises/CMakePresets.json`

- default verify-core builds reference and keeps students/ICU/frontier off: lines 17-21.
- student preset has reference off and students on: lines 36-37.
- frontier and ICU presets opt into those lanes: lines 51, 58, 65.

### `__has_include` bypass search

```text
rg -n "__has_include" exercises/include/c05 exercises/L03_integer_boundaries exercises/L06_transcoding exercises/L14_binary_fields exercises/L15_schema_evolution exercises/P1_resource_manifest exercises/cmake exercises/CMakeLists.txt exercises/CMakePresets.json
NO_MATCH
```

## Final bound hashes

```text
2f826cb338f73e12f950df91a8170d5532004305365baed41cb4bc7e38bdec10  exercises/include/c05/model.hpp
af8af2fccf9ff02b36f912162ceb7d98cf94df4a72cd04c2a47d947acddc3d39  exercises/include/c05/types.hpp
6cf21a07f7bf782d798591b1d81801c4c9c42db130f545bd471aed86ab438b07  exercises/include/c05/bytes.hpp
36f6b4f3b99d0eba3e62669e18e7026fa8171ea913b768b8c7469835c2f1b9f6  exercises/include/c05/utf.hpp
3203c99b399eba1afcb013f53c837f3bf937ef07ca5f53b69d1a0e086cf9ef46  exercises/include/c05/manifest.hpp
6bfca0c4a15f896ecc5a8497e5e3ee4a15d9760cab96f67c4a6446e52517c1c0  exercises/include/c05/paths.hpp
773adf8948fc43e8b8079507f8e42d33e226de3680ab9c41a41f5dff17a3627b  exercises/include/c05/config.hpp
aed042d3b6399ce3879019dec85ab6ea8187aa04994b439a35c8ccb92fd9f750  exercises/include/c05/time.hpp
575bd0dc47cae78ae05cd805f8d2e5b84801efc5560c20c34c9d2f574a903e78  exercises/fixtures/golden.hpp
8d21e8f6eccc5713b8b738dbf5ae3d046814b186074e4efc934af006973e542c  exercises/L03_integer_boundaries/checks/checked_int_checks.cpp
438a55dc11008d6d88243a24f6f9dd7ddf5e8cf8716065192909648517c12023  exercises/L06_transcoding/checks/transcode_checks.cpp
4d6d62d187b6292cb511e1eb7d11650a62b9ec6b16c6971498c287df6947ab10  exercises/L14_binary_fields/checks/field_checks.cpp
96c06a1dcf80ae2b0a883b3687ef3a9d2a70b98974decbd9eda006f228539972  exercises/L15_schema_evolution/checks/schema_checks.cpp
b0025b5040e55fcc4d6324d0e6df9065a5b0015b63d3be783174c162fe622ad1  exercises/L15_schema_evolution/provided/v1_reader.hpp
0757a8e76841501aef655b9bfefc09ddc46d25882485f760d0a62fc90f2ec63e  exercises/L15_schema_evolution/src/reference/schema_evolution.hpp
76f0225efaf373657d1521f5ca5a832f61e79d97939c8e3284f5f37a72c9692b  exercises/L15_schema_evolution/validation/good/schema_evolution.hpp
082651faf0755b9fbeff9672eb30c0d5178c24a81ad04b124007d7e7e7abfe0d  exercises/L15_schema_evolution/validation/bad/schema_evolution.hpp
2713317b1247a93a096ebf0bfa12901bbb8aaaccaff8508bb4c1467e7d2945db  exercises/P1_resource_manifest/checks/pipeline_checks.cpp
75ff8866a94c61b260c3df58d368e6799abe1a7bf39e3077b88bae8318da8c23  exercises/P1_resource_manifest/provided/contract.hpp
fe1535ec01c833380d719aab66363661f22536b00d36ade2e9502f92d135a280  exercises/P1_resource_manifest/provided/operations.hpp
077eed14e72c1cba1aa224d539e368b6b3a0edda312410bf6af792e793066c53  exercises/P1_resource_manifest/src/reference/pipeline.hpp
ab7fb3853d2d43cc5bb9110f71397229d44146b2bd1e2667aa7fb13d961a9865  exercises/P1_resource_manifest/validation/good/pipeline.hpp
1c5540a12815d4e379a63a58d6d9fe3496bb6f929fc3a55e1a10878b31d7ca43  exercises/P1_resource_manifest/validation/bad/pipeline.hpp
11b0c779028bbefe80e744b23a3a4ede10f8895248310ed7b528603012e0abcd  exercises/cmake/StudySetup.cmake
c19f4500a000bd0e68cc6288a335fb7a19a5961007fcf6486bda79fce5bd2ea3  exercises/cmake/ICU.cmake
e38fe129e3a82ddb294f036367e27271fa804f874fceb525e6432556a3aafebe  exercises/CMakePresets.json
688af42d99804320062011df0936322a7e12cdc2da3234acf6207df5b7cc8a91  exercises/CMakeLists.txt
```

## Gaps and boundaries

- This lane did not repeat the earlier full Unicode/bytes oracle because core `bytes.hpp` and `utf.hpp` were unchanged from the approved sample review boundary; it did inspect and bind their final hashes.
- Time/config/path runtime behavior was mostly assigned to another reviewer. This lane checked public build wiring and included final hashes for `paths.hpp`, `config.hpp`, and `time.hpp` because P1/manifest depend on them.
- ICU runtime behavior was out of this lane. This lane statically checked `ICU.cmake` option isolation, exact version requirement, and DLL copy/error wiring.
- No reviewed source, lesson prose, or other course files were modified by this verifier.
