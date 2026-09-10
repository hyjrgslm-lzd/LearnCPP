# C06 CAPSTONE1 样章非作者复验

Verdict: BLOCK

Review date: 2026-09-10
Reviewer role: non-author verifier. This is not a dedicated architect or ralplan approval.

Scope:

- `C06_Ranges/chapters/00-log-pipeline-evolution.md`
- `C06_Ranges/exercises/CAPSTONE1_log_pipeline/**`, excluding generated build products as author source
- `C06_Ranges/exercises/cmake/RangesSetup.cmake`

## Evidence

Fresh isolated build directory: `C06_Ranges/exercises/CAPSTONE1_log_pipeline/build-sample-review`.

Recorded commands:

- `references/validation/sample-review-configure.json`: PASS, CMake configure exit 0.
- `references/validation/sample-review-build-release-exit0.json`: PASS, Release build exit 0.
- `references/validation/sample-review-ctest-release-exit0.json`: PASS, CTest exit 0 with `100% tests passed, 0 tests failed out of 3`.
- `references/validation/sample-review-build-student.json`: PASS, `CAPSTONE1_log_pipeline_student` builds when explicitly targeted.
- `references/validation/sample-review-student-exit1.json`: PASS, unfinished Student exits 1 with `check failed: valid row parses`.
- `references/validation/sample-review-probes-configure-fixed.json`: PASS, local probe configure exit 0.
- `references/validation/sample-review-probes-build.json`: PASS, local probes build exit 0. This compile-checks the temporary optional reference example without running undefined behavior.
- `references/validation/sample-review-ranges-to-run.json`: PASS, MSVC 19.51 accepts and runs `filter | take | std::ranges::to<std::vector<int>>()`.
- `references/validation/sample-review-negative-limit-run.json`: PASS as a diagnostic probe: current Reference `summarize(lines, -1)` returns 3 first errors, proving negative `error_limit` is not rejected.

The earlier `sample-review-build-release.json` and `sample-review-ctest-release.json` show command exit 0 but recorder verdict FAIL because the expected text was too specific for localized MSBuild/CTest output. They are superseded by the `*-exit0.json` records above.

## Blocking Findings

1. `count_reparse_demo` does not reproduce the same pipeline shape shown in the chapter.

   The chapter presents `lines | transform(parse_counted) | filter(optional.has_value) | transform(unwrap optional)` and says the 16 count comes from the checker's `count_reparse_demo`. The Reference and good implementations instead use `lines | filter(parse_counted(line).has_value) | transform(*parse_counted(line))`. Both produce `9 + 7 == 16`, but they are not the same view chain. The current checker only asserts the final count, so it does not prove the exact chain taught by the prose.

   Minimal fix: make `count_reparse_demo` use the same transform/filter/transform chain shown in the chapter, or change the prose and README to describe the actual line-based filter/transform probe. Keep the checker assertion at 16.

2. The sample bypasses `std::ranges::to` even though the course promises it as a core consumer and the local toolchain supports the needed case.

   `C06_Ranges/README.md` names `ranges::to` as a core consumer/implementation topic, while the chapter says the implementation avoids it because of possible STL differences. The review probe `sample-review-ranges-to-run.json` proves this MSVC setup can run a `filter | take | std::ranges::to<std::vector<int>>()` collection. Current Reference/good code uses vector constructors or explicit loops instead.

   Minimal fix: use `std::ranges::to<std::vector<LogRecord>>()` in at least one relevant sample path that this chapter asks students to learn, and keep any fallback note tied to a specific unsupported toolchain/version rather than a generic assumption.

3. `summarize(lines, int error_limit)` has an undeclared negative-value contract.

   The public exercise signature accepts `int error_limit`, but README and chapter do not state a precondition. Reference/good pass it directly into `std::views::take`. The negative-limit probe shows `summarize(sample_lines, -1)` returns 3 first errors instead of rejecting invalid input. That is a hidden API precondition in a sample about contracts and ranges.

   Minimal fix: either change the interface to a non-negative type, or explicitly reject negative values and add a checker case. If keeping `int`, prefer a visible rejection path before calling `views::take`.

4. The chapter says all four `LogRecord` fields are `std::string`, but `level` is an enum.

   This is small, but it sits in the ownership explanation. The intended point is that textual fields own storage while `level` is parsed into `Level`.

   Minimal fix: rewrite that sentence to say the textual fields are `std::string`, and `level` is an enum value.

## Passing Checks

- Teaching flow exists: correct loop baseline first, then repeated lazy evaluation, then materialized optional storage, then summary collection.
- The checker validates valid parsing, invalid filtering, loop count 9, corrected ranges count 9, report counts, first three errors, and reparse count 16.
- Reference and validation/good are independent implementations by source inspection; no include path points good at Reference.
- validation/bad is a real behavioral bad case, and CTest confirms the expected diagnostic path rejects it.
- Student initial state is safe and finite: it compiles only when explicitly targeted and fails behavior checks with exit 1.
- The temporary optional dangling example compiles as a review probe and is not run as a stable bad target, which matches the chapter's stated UB boundary.
- `RangesSetup.cmake` correctly wires default Reference/good/bad tests, keeps Student out of default CTest unless opted in, and routes implementations by include path.

## Fingerprints

Reviewed source SHA256:

```text
6115A256B4BAA6BD34E5A5A8FCEE5766093BAFD0F076011822E7375EE5C3370C  F:\CPPTrain\LearnCPP\C06_Ranges\chapters\00-log-pipeline-evolution.md
2E1130FD3E961D5A199AA0C3686E8F9945CB05014E84F88B1648CA32263EDC78  F:\CPPTrain\LearnCPP\C06_Ranges\exercises\CAPSTONE1_log_pipeline\README.md
441611281532FE8E4EE26F4E297980ACBB91F77A62B337B4B0F87C88878F1EB7  F:\CPPTrain\LearnCPP\C06_Ranges\exercises\CAPSTONE1_log_pipeline\CMakeLists.txt
C15188FB8568A307FE80BCA382AB26CD8C5605E9CFF402B6B4E1E69BC23C6494  F:\CPPTrain\LearnCPP\C06_Ranges\exercises\cmake\RangesSetup.cmake
3FF50D9DB7C58C9EFE713B75DD71BEE28B7EEB2E515F9815A3A2CC59B0E907FD  F:\CPPTrain\LearnCPP\C06_Ranges\exercises\CAPSTONE1_log_pipeline\main.cpp
8FE5CB0FF5C60CB4B0F28A4DBED9C235F3E8F104D258931112ED09DE37B93745  F:\CPPTrain\LearnCPP\C06_Ranges\exercises\CAPSTONE1_log_pipeline\include\log_pipeline_data.hpp
28D5951C8FB65F25FB79FFBF810F98D7F4D6BD1AE75A18FBFA01F5B1FA5C2E03  F:\CPPTrain\LearnCPP\C06_Ranges\exercises\CAPSTONE1_log_pipeline\src\reference\log_pipeline.hpp
6C595ED0E1F37715E1CFAC440A9A26FC4F00626EEA3C47AF8973EEA3B88E2E8B  F:\CPPTrain\LearnCPP\C06_Ranges\exercises\CAPSTONE1_log_pipeline\validation\good\log_pipeline.hpp
6E7E20721A32BE54B429EC686852B4788891CDAA26BC3338A2B23049364997C6  F:\CPPTrain\LearnCPP\C06_Ranges\exercises\CAPSTONE1_log_pipeline\validation\bad\log_pipeline.hpp
6BBCE871C689D647EAA73A8F67E04A070AAAD2BC23CFBD4FF7A71AB1BC745440  F:\CPPTrain\LearnCPP\C06_Ranges\exercises\CAPSTONE1_log_pipeline\src\student\log_pipeline.hpp
```

## Stop Condition

Do not expand this sample as the other authors' quality baseline yet. Close the four blocking findings, rerun the isolated Release build/CTest/Student/negative-limit evidence, update this review or add a non-author re-review, then it can be considered for APPROVE.
