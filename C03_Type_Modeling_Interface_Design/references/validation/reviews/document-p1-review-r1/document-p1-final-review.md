# P1 Document final technical review r1

Verdict: APPROVE
Reviewer: /root/c03_design_review code-reviewer lane
Date: 2026-09-09
Scope: P1_document source/checker/CMake, chapter 16, P1 README, Debug allocation diagnosis and boundary fix.

## Findings

No blocking issues found.

- Document contract matches approved spec: strong ElementId/PositiveLength factories, Shape as variant<Rectangle,Circle>, ordered value snapshots, optional find, expected business errors, copy independence, ordered batch semantics, strong rollback, and explicit moved-from empty/reusable behavior.
- Reference and good are independent enough for checker controls: reference uses Document candidate plus visit; good uses candidate vector plus get_if branches.
- Bad is a focused early-commit batch defect and is rejected by the intended diagnostic.
- Student is real and ref-off execution fails at the first missing public behavior, not by build/link isolation.
- Debug allocation repair is correctly bounded: normal targets do not define C03_ALLOCATION_INJECTION or _ITERATOR_DEBUG_LEVEL=0; allocation reference/good targets define both in their single TU only.
- Root-cause evidence is preserved: diagnosis keeps the original MSVC Debug string move/noexcept proxy failure and explains why IDL0 is scoped to allocation experiments.

## Validation run by this reviewer

Evidence directory: references/validation/reviews/document-p1-review-r1

- configure-debug.json, build-debug.json, ctest-debug.json: Debug configure/build/CTest passed 5/5.
- build-release.json, ctest-release.json: Release build/CTest passed 5/5.
- configure-asan.json, build-asan.json, ctest-asan.json: ASan RelWithDebInfo build/CTest passed 5/5.
- configure-student.json, build-student.json, run-student-expected-fail.json: student ref-off target builds and exits 1 with `check failed: Add inserts an actual element`.
- ctest-debug-bad-verbose.json: bad target rejected with `failed batch preserves the entire original document`.
- run-allocation-reference-debug.json, run-allocation-good-debug.json: allocation experiments directly run and report `ordinary allocation failure points checked: 3`.
- macro-audit.txt: generated vcxproj macro audit confirms normal Debug targets lack allocation/IDL0 macros; allocation targets contain `C03_ALLOCATION_INJECTION=1;_ITERATOR_DEBUG_LEVEL=0`.
- pattern-audit.txt: no secret/broad-catch/masking fallback issue found in reviewed scope; Student TODO/nullopt is intentional unfinished template and dynamically rejected.
- source-sha256.txt: reviewed source hashes.

## Tool gap

No lsp_diagnostics tool is available in this environment. I used MSVC W4 Debug/Release/ASan builds as the type/compile diagnostic gate for these C++ files.
