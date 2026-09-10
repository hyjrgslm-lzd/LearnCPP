# C04 07-10 r2 author handoff

Input review: `references/reviews/metaprogramming-static-review.md` returned ITERATE for L08 depth/lazy evidence, L10 constant-evaluation depth/diagnostics, and L07 count mismatch. Root lesson registration was already handled outside this author scope.

## Fixes

- L07: added `count_types` and `count_values` to Student/Reference/bad and checker; README now has a dedicated pack-count part.
- L08: expanded chapter and README with `map`, `filter`, `unique` hand expansion, base cases, recursive steps, and `Seen/Rest` invariant. Added `lazy_type_t`, runtime positive checker coverage, observation coverage, and an isolated eager-instantiation diagnostic case. Kept normal bad variant buildable and failing only at the intended completion-signature diagnostic.
- L09: no static review blocker; preserved coverage.
- L10: expanded chapter and README with manifestly constant-evaluated context, immediate invocation/context, `if consteval` versus `std::is_constant_evaluated` trial behavior, object/storage boundaries, `vector`/`string` transient allocation and persistence failures, C++23/C++26 boundary, and constexpr parse diagnostics. Reworked `decimal_value` parser to give a named compile-time invalid-digit diagnostic and added an isolated compile case.

## Verification

Reference/good/bad/observation single-lesson Release runs:

- L07: configure/build/CTest PASS (`r2-L07_packs-01-configure.json`, `r2-L07_packs-02-build-release.json`, `r2-L07_packs-03-ctest-release.json`).
- L08: configure/build PASS; first CTest exposed bad diagnostic mismatch, fixed; rerun build/CTest PASS (`r2-L08_type_lists-02-build-release-rerun.json`, `r2-L08_type_lists-03-ctest-release-rerun.json`). Diagnostic `L08_lazy_type_eager_bad` passed inside CTest.
- L09: configure/build/CTest PASS (`r2-L09_tuple-01-configure.json`, `r2-L09_tuple-02-build-release.json`, `r2-L09_tuple-03-ctest-release.json`).
- L10: configure/build/CTest PASS (`r2-L10_constexpr-01-configure.json`, `r2-L10_constexpr-02-build-release.json`, `r2-L10_constexpr-03-ctest-release.json`). Diagnostic `L10_invalid_digit_diagnostic` passed inside CTest.

Student-only Release runs:

- L07: configure/build PASS; CTest expected failure, `check failed: empty all fold identity must be true`.
- L08: configure/build PASS; CTest expected failure, `check failed: map_t applies a unary template to each element`; observation and diagnostic still PASS.
- L09: configure/build PASS; CTest expected failure, `check failed: tuple traversal must preserve left-to-right order`.
- L10: configure/build PASS; CTest expected failure, `check failed: decimal_value parses digits at compile time`; observation and diagnostic still PASS.

Static checks:

- `git diff --check -- C04_Generic_CompileTime_Reflection`: PASS.
- Targeted status shows only this author scope as untracked/modified under C04 07-10 and `references/validation/metaprogramming-author`.

Superseded evidence retained:

- `r2-L08_type_lists-03-ctest-release.json`: expected intermediate FAIL before bad diagnostic fix.
- `r2-L09_tuple-02-build-release-rerun.json`: infrastructure FAIL because the previous loop stopped before configuring L09; replaced by normal L09 configure/build/CTest PASS files.

## Frozen file hashes

| File | SHA256 |
|---|---|
| `chapters/07-packs-nttp.md` | `CCBA5E27AFCFA0E786F548B4E8362CB18392928DF2994DACAA2ADD9C54D2BBC6` |
| `chapters/08-type-lists.md` | `46BAFA4FAAA5D42FB95C03D9CD31AB4BDDDC07D8CEF6BDCF52B8CB264F3D94E5` |
| `chapters/09-tuple-traversal.md` | `F0244F23832E6D42F674B3F76172269548DA8ABFEA979B75DCEAB122890A61A7` |
| `chapters/10-constant-evaluation.md` | `AA3B8C275EB8157993DC4B029B61FED6C4340E7982D8B466D8843822124C6A13` |
| `exercises/L07_packs/checks/pack_tools_checks.cpp` | `1BFD9ABF29DE70B50C1C762AE6C699777F00C1C5D2323624EDD631207C850E42` |
| `exercises/L08_type_lists/checks/type_list_checks.cpp` | `5625047EA40FC789CD9BB40D26EAF29642449D6D3DA1D64AD7E3D3034E0B086B` |
| `exercises/L09_tuple/checks/tuple_for_each_checks.cpp` | `86D6FD123B1470B5FF9906B7478BC7D3394E0B39C4F3CF5788CCDBD85302163D` |
| `exercises/L10_constexpr/checks/constexpr_tools_checks.cpp` | `CFFCFF4CDA40E7127F3F77EA789DCE5F3711E6AAC39945D071FE3690B8FCE382` |

## Limits

Only single-lesson Release and Student-only Release were run in this author pass. Root whole-course, Debug matrix, ASan/frontier, and independent non-author review remain outside this author handoff.
