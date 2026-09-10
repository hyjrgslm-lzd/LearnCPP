# C06 CAPSTONE1 样章 r2 非作者复验

## Verdict

APPROVE

本次结论只批准 `C06_Ranges/chapters/00-log-pipeline-evolution.md` 和 `C06_Ranges/exercises/CAPSTONE1_log_pipeline/**` 作为样章基线，可放行其他作者按该质量线扩写。它不是整门 C06 完成验收，也不是专用 architect/ralplan 批准。

## Scope

- `C06_Ranges/chapters/00-log-pipeline-evolution.md`
- `C06_Ranges/exercises/CAPSTONE1_log_pipeline/**`
- `C06_Ranges/exercises/cmake/RangesSetup.cmake`

Build products under `build-sample-review` are validation artifacts, not reviewed author source.

## Evidence

Fresh r2 evidence, recorded with `C02_Objects_Lifetime_Ownership/exercises/tools/record_process.py`:

- `references/validation/sample-review-r2-configure.json`: PASS, CMake configure exit 0.
- `references/validation/sample-review-r2-build-release.json`: PASS, Release build exit 0.
- `references/validation/sample-review-r2-ctest-release.json`: PASS, Release CTest exit 0 with `100% tests passed, 0 tests failed out of 3`.
- `references/validation/sample-review-r2-build-student.json`: PASS, explicit Student target builds in Release.
- `references/validation/sample-review-r2-student-exit1.json`: PASS, unfinished Student exits 1 with `check failed: valid row parses`.
- `references/validation/sample-review-r2-build-debug.json`: PASS, Debug build exit 0.
- `references/validation/sample-review-r2-ctest-debug.json`: PASS, Debug CTest exit 0 with `100% tests passed, 0 tests failed out of 3`.

Release and Debug CTest both register and pass:

- `CAPSTONE1_log_pipeline_reference`
- `CAPSTONE1_log_pipeline_validation_good`
- `CAPSTONE1_log_pipeline_validation_bad_rejected`

## Previous Blocker Recheck

1. `count_reparse_demo` chain shape: closed.

   Chapter, README, Reference, good, and bad now use the same `transform(parse_counted) -> filter(optional.has_value) -> transform(unwrap)` shape for the 16 vs 9 repeated-parse lesson. The checker still asserts `count_reparse_demo(lines).parse_attempts == 16`, and both Release/Debug CTest pass.

2. `std::ranges::to`: closed.

   Reference and good now use `std::ranges::to` for parsed optional materialization and final `LogRecord` collection. Summary error collection also uses `std::ranges::to<std::vector<LogRecord>>()`. This matches the C06 consumer requirement and the prior local probe that MSVC 19.51 supports this path.

3. `summarize(lines, int error_limit)` negative contract: closed.

   Reference, good, bad, and Student stub all reject `error_limit < 0` with `std::invalid_argument`. The checker covers `summarize(lines, 0)`, `summarize(lines, 99)`, and negative rejection before `views::take`.

4. `LogRecord` text ownership wording: closed.

   Chapter now states that `timestamp`, `user_id`, and `message` are owning `std::string` fields, while `level` is parsed into `Level`.

## Other Checks

- Teaching shape is now coherent: correct loop baseline first, repeated lazy parse counted as observation instrumentation, materialized optional ranges version, then summary with `filter + take + ranges::to`.
- The parse-count diagnostic is presented as this sample's observation/checker instrumentation, not as a claim that all range contracts are violated.
- Temporary optional dangling remains a compiled/safe teaching example and is not used as a stable runtime bad target.
- Reference/good/bad are independent by source inspection; no validation implementation includes the Reference implementation.
- `RangesSetup.cmake` still wires Reference/good/bad into default CTest and keeps unfinished Student out of default CTest unless explicitly targeted.

## Fingerprints

Reviewed source SHA256:

```text
EC9AF906279E5E64E70F61569A0F425A5D85B028CEBC1A4D485D6F458AD838C0  F:\CPPTrain\LearnCPP\C06_Ranges\chapters\00-log-pipeline-evolution.md
B00391C369C996CBB960CFE2E0325C04834FD201A65CD280EAA3C55DEE7EC7EB  F:\CPPTrain\LearnCPP\C06_Ranges\exercises\CAPSTONE1_log_pipeline\README.md
441611281532FE8E4EE26F4E297980ACBB91F77A62B337B4B0F87C88878F1EB7  F:\CPPTrain\LearnCPP\C06_Ranges\exercises\CAPSTONE1_log_pipeline\CMakeLists.txt
C15188FB8568A307FE80BCA382AB26CD8C5605E9CFF402B6B4E1E69BC23C6494  F:\CPPTrain\LearnCPP\C06_Ranges\exercises\cmake\RangesSetup.cmake
84E7B5114D4703B24381D21491BBF23F00C3154F349D79A0EE846385000C1F33  F:\CPPTrain\LearnCPP\C06_Ranges\exercises\CAPSTONE1_log_pipeline\main.cpp
8FE5CB0FF5C60CB4B0F28A4DBED9C235F3E8F104D258931112ED09DE37B93745  F:\CPPTrain\LearnCPP\C06_Ranges\exercises\CAPSTONE1_log_pipeline\include\log_pipeline_data.hpp
4894D231B70C87DC70A3B8CC4BE01D479C6731262FC22AE8E140977F070B1EDA  F:\CPPTrain\LearnCPP\C06_Ranges\exercises\CAPSTONE1_log_pipeline\src\reference\log_pipeline.hpp
A47AF00DB2EF0DC499412F158C2DE54C65A2C573EE95A95EFE8DF4708456DF2C  F:\CPPTrain\LearnCPP\C06_Ranges\exercises\CAPSTONE1_log_pipeline\validation\good\log_pipeline.hpp
57DA0A7AA3F764B2B1595F54D37C6C0392C2883E624E21C9BD9D3B70965CA462  F:\CPPTrain\LearnCPP\C06_Ranges\exercises\CAPSTONE1_log_pipeline\validation\bad\log_pipeline.hpp
F8D9962E2BFC7A6417A2794C00562620A7ECFAB2B58AFF3D404338D9D42F0DF1  F:\CPPTrain\LearnCPP\C06_Ranges\exercises\CAPSTONE1_log_pipeline\src\student\log_pipeline.hpp
```

## Gaps

- No ASan run was performed for this r2 sample review.
- This review did not run wider C06 tests or inspect unrelated chapters.

## Risks

- Future expansion must keep the same evidence standard: Reference/good/bad independence, Student true rejection, source-hash-bound review, and PASS/FAIL/SKIP separation.
