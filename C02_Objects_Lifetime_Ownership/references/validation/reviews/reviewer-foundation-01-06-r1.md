# C02 independent review — foundation 01/02/03/05/06 r1

## Verdict

APPROVE for the frozen foundation batch covering 01/02/03/05/06 and L01/L02/L03/L05/L06.

This approval is limited to the current frozen foundation batch. It does not reopen 00/04, and it does not approve the pending L10 alias-assignment fix; `verifier-l10-alias-assignment-asan-r1.md` remains a separate REQUEST_CHANGES item for the owner batch until r2 is frozen and rechecked against the same repro.

## Scope reviewed

- `Core_Study/chapters/01-initialization.md`
- `Core_Study/chapters/02-expressions-and-references.md`
- `Core_Study/chapters/03-lifetime-and-borrowing.md`
- `Core_Study/chapters/05-special-members.md`
- `Core_Study/chapters/06-move-and-return.md`
- `Core_Study/exercises/L01_initialization/**`
- `Core_Study/exercises/L02_value_categories/**`
- `Core_Study/exercises/L03_lifetimes/**`
- `Core_Study/exercises/L05_special_members/**`
- `Core_Study/exercises/L06_move_return/**`
- `Core_Study/references/validation/author-foundation/foundation-final-r2*` and r2c scan evidence

Build directories under lesson `build/**` were excluded from source review except when reading generated logs as evidence.

## Source binding

`c02-independent-evidence/foundation-01-06-r1/source-hashes.json` records 59 reviewed source/evidence hashes.

Key hashes:

- `Core_Study/chapters/01-initialization.md`: see `source-hashes.json`
- `Core_Study/chapters/02-expressions-and-references.md`: see `source-hashes.json`
- `Core_Study/chapters/03-lifetime-and-borrowing.md`: see `source-hashes.json`
- `Core_Study/chapters/05-special-members.md`: see `source-hashes.json`
- `Core_Study/chapters/06-move-and-return.md`: see `source-hashes.json`
- `Core_Study/references/validation/author-foundation/foundation-final-r2c-scan-trusted-header-paths.json`: included in `source-hashes.json`
- `Core_Study/references/validation/author-foundation/foundation-final-r2-scan-no-student-report-marker.json`: included in `source-hashes.json`

## Evidence

Fresh reviewer evidence is saved under `Core_Study/references/validation/reviews/c02-independent-evidence/foundation-01-06-r1/` and `.../foundation-01-06-r1-shortbuild/`.

- L01 Debug and Release configure/build/CTest passed from isolated evidence build path.
- L03 Debug and Release configure/build/CTest passed from isolated evidence build path. CTest covered reference, validation good, and bad dangling-view expected failure.
- L05 Debug and Release configure/build/CTest passed from isolated evidence build path. CTest covered reference, validation good, bad shallow-copy expected failure, and bad move-leak expected failure.
- L03 Student + Reference OFF configured and built; Student CTest failed as expected with `check failed: borrow from live owner must be valid`.
- L05 Student + Reference OFF configured and built; Student CTest failed as expected with `check failed: copy must have independent storage`.
- L02 and L06 passed Debug CTest from short isolated build paths under `build/c02-reviewer-foundation-r1-*`. This covered their observation tests and negative compile tests.
- Direct manual builds of L02/L06 negative cases confirmed actual MSVC diagnostics include `C2440`, `C2664`, and `C2280`; saved in `negative-manual/manual-negative-summary.json`.
- The first very deep nested evidence build path caused L02/L06 negative wrapper failures because nested case build dirs lacked generated `*.slnx` / `ALL_BUILD.vcxproj`, producing `MSB1009` instead of the C++ diagnostic. I treat that as a non-conclusion path-length/build-tree artifact because the same tests pass from the documented shorter build shape and the underlying negative source diagnostics are present.
- Author r2/r2c JSON evidence was parsed into `author-foundation-r2-json-summary.txt`: current r2 lesson records are PASS where expected, Student expected-fail records preserve exit 8 as PASS, and earlier trusted-header scan failures are preserved before r2c PASS.

## Stage 1 — teaching/spec compliance

The batch satisfies the requested ground-up teaching shape.

- L01 starts from declaration/storage/value/lifetime split, then builds through default/value/copy/direct/list initialization, narrowing, aggregates/designated initialization, static/dynamic initialization, `constinit`, cv, and evaluation order. The chapter explicitly avoids running uninitialized reads and distinguishes C++23 from C++26 erroneous-behavior direction.
- L02 builds expression categories from lvalue/xvalue/prvalue through materialization, reference binding, overload selection, `auto`, `decltype(auto)`, reference collapsing, and forwarding reference. It does not treat `std::move` as a real move operation.
- L03 covers owner/borrower, storage duration vs lifetime, temporary full-expression lifetime, direct lifetime extension and exceptions, brace-vs-paren aggregate reference member behavior, dynamic `new` boundary, returning references, `initializer_list` direct-extension/copy boundary, C++23 range-for lifetime extension limits, lambda capture lifetime, coroutine-frame borrowing risk, `string_view`/`span`/ranges views, and interface ownership design. This closes the previously noted Holder brace ambiguity, initializer_list/range-for gaps, and lambda/coroutine direction gap in the current text.
- L05 explains the six special members, implicit declaration/definition/deletion/suppression, Rule of 0, Rule of 5, deep copy, strong copy-assignment ordering, move transfer, moved-from contract boundaries, and `noexcept` move/container relevance.
- L06 explains `std::move` as value-category conversion, const moving, guaranteed same-type prvalue elision, NRVO as optional optimization, C++23 implicit move, `return std::move(local)`, `move_if_noexcept`, and moved-from state boundaries.
- Exercise README Part lists match the compiled interfaces for this batch. L03 and L05 are real implementation tasks with Student stubs; L01/L02/L06 are correctly documented as observation/negative compile tasks with no Student implementation.

## Stage 2 — code/checker quality

No blocking code/checker quality issue found in this batch.

- L01/L02/L06 negative checks do not use `WILL_FAIL`; they build isolated negative projects and require fixed diagnostics.
- L03/L05 bad variants are rejected by wrappers that execute the bad target and require fixed `check failed` text.
- L03/L05 checkers directly create trusted owner/storage state and consume student operations; they do not accept student-provided ready flags, reports, counters, or completion markers.
- `foundation-final-r2-scan-no-student-report-marker.json` reports `no forbidden patterns` for `implemented|ResourceAttempt|student_ready|WILL_FAIL|return .*report|run_.*case` over this batch.
- `foundation-final-r2c-scan-trusted-header-paths.json` reports no bare trusted-header include matches and lists expected `support/lifetime_model.hpp` / `support/memory_model.hpp` implementation includes. Source inspection confirms the check translation units include the real support header before the implementation header.
- L03/L05 fixtures use fixed-size arrays/static counters and do not allocate in destruction/release paths. L05 invalid double-release is counted instead of dereferencing released storage.
- No credential exposure, destructive behavior, silent fallback, or broad workaround branch was found.

No `lsp_diagnostics` tool is available in this lane; MSVC configure/build/CTest, direct negative manual builds, source inspection, and author JSON parsing were used instead.

## Findings

No blocking findings.

Non-blocking observation: L02/L06 negative nested build wrappers are sensitive to very deep binary directories on this Windows setup. From `Core_Study/references/validation/reviews/c02-independent-evidence/foundation-01-06-r1/build-debug-*`, nested negative case builds failed with `MSB1009` before emitting the intended diagnostic. The same tests passed from shorter isolated `build/c02-reviewer-foundation-r1-*` paths, and manual negative builds confirmed the intended compiler diagnostics. No source change is required for this batch unless the course wants to guarantee arbitrary deep build paths.

## Recommendation

APPROVE for 01/02/03/05/06 and L01/L02/L03/L05/L06.

Recheck scope after future changes should be limited to files touched by the fix unless the support/checker contract changes. L10 alias-assignment remains outside this approval and still requires owner r2 repro closure.
