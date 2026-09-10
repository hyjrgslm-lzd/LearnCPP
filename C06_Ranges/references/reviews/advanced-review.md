# C06 advanced slice independent review

Verdict: REQUEST CHANGES.

Scope reviewed: original chapter 11/12 body, H1/H2/H3, CAPSTONE3, CAPSTONE4, exercise CMake wiring, and validation evidence for this advanced slice. I did not edit author source. New review evidence is under `C06_Ranges/references/validation/advanced-review-*`.

Independent architect lane status: unavailable by task constraint. This review does not claim a separate architect verdict.

## Summary

Files reviewed: 67 exercise/source files plus chapter/spec/coverage docs.

Issues found:

- CRITICAL: 0
- HIGH: 3
- MEDIUM: 2
- LOW: 0

## Issues

[HIGH] H3 `my_basic_const_iterator` does not match C++23 `std::iter_const_reference_t`

File: `C06_Ranges/exercises/H3_generator_const_iter/src/reference/generator_const_iter.hpp:16`

Issue: The local alias chooses `std::iter_reference_t<I>` unchanged whenever the underlying iterator returns a prvalue/proxy. That passes the current artificial `PairProxy` probe because its `value_type` is the same proxy type, but it fails real proxy/rvalue iterator cases. With `std::vector<bool>::iterator`, `operator*` returns MSVC's writable proxy while `std::iter_const_reference_t` is `bool`; `my_basic_const_iterator` is still indirectly writable. With `std::move_iterator<std::vector<int>::iterator>`, the implementation returns `const int&` while the standard alias is `const int&&`.

Evidence:

- `C06_Ranges/references/validation/advanced-review-h3-hostile-probe.cpp:11`
- `C06_Ranges/references/validation/advanced-review-h3-probe-build-reference-expected-fail.json`: expected compile failure shows `std::_Vb_reference` vs `bool`, the `!indirectly_writable` assertion failing, and `const int&` vs `const int&&`.
- Current checker only covers lvalue `vector<int>` at `C06_Ranges/exercises/H3_generator_const_iter/checks/generator_const_iter_checks.cpp:93`, transform prvalue at `:104`, and a same-type custom proxy at `:113`.

Fix: Replace the hand-rolled alias with `std::iter_const_reference_t<I>` where available, or use the standard shape `std::common_reference_t<const std::iter_value_t<I>&&, std::iter_reference_t<I>>`. Add regression checks for `std::vector<bool>::iterator`, `!std::indirectly_writable<my_basic_const_iterator<std::vector<bool>::iterator>, bool>`, and `std::move_iterator<std::vector<int>::iterator>`.

[HIGH] H3 chapter body still teaches default-moving an owning coroutine handle

File: `C06_Ranges/11-模块H-高级实现模式.md:750`

Issue: The chapter sample uses `my_generator(my_generator&&) = default` and `operator=(my_generator&&) = default` while the destructor destroys the stored `coroutine_handle` at `:755`. `std::coroutine_handle` is a non-owning handle type; default move copies the handle and leaves the source owning wrapper non-empty. Moving a generator can therefore leave two wrappers destroying the same frame. This contradicts the exercise checker, which correctly requires move construction to clear the source and move assignment to destroy the old target frame.

Evidence:

- `C06_Ranges/11-模块H-高级实现模式.md:750`
- `C06_Ranges/11-模块H-高级实现模式.md:755`
- `C06_Ranges/exercises/H3_generator_const_iter/src/reference/generator_const_iter.hpp:67` uses `std::exchange` in the fixed move constructor.
- `C06_Ranges/exercises/H3_generator_const_iter/src/reference/generator_const_iter.hpp:69` destroys the old frame before move assignment takeover.

Fix: Update the chapter sample in place to the same ownership protocol as the exercise Reference: move construction must `std::exchange(other.handle_, {})`; move assignment must guard self-move, destroy/reset the current frame, then exchange from the source. Add a short note that `coroutine_handle` itself does not transfer ownership.

[HIGH] CAPSTONE4 `validation/good` is byte-identical to Reference, so it does not prove checker independence

File: `C06_Ranges/exercises/CAPSTONE4_mini_ranges/README.md:15`

Issue: The README states that `validation/good/my_ranges/` is an independent correct implementation. Hash evidence shows all six `validation/good` headers are byte-identical to `src/reference`. This violates the course checker contract: good must show the checker accepts a second valid implementation, not just the same answer copied into another include path.

Evidence:

- `C06_Ranges/references/validation/advanced-review-hash-audit-r3.json`
- Matching hashes for Reference and good:
  - `01_cpo.hpp`: `6ED080D4568F3AC6324CDBEB0A8D2D909C64702D6242F320BAEA2CF25F0DD68E`
  - `02_concepts.hpp`: `1B7A7B39E57B1DA133882D5388DC86B38F606C2A615588D3A1E99BB23BDEE19E`
  - `03_interface.hpp`: `853028FAFA02A767035A1A7F641C3D61E2638AD756FE430A258FADC4D73B23E5`
  - `04_factories.hpp`: `A22D2A5B6221AB5541157D9008041BAFA518E60992CC8186F2DC6F1B09C14F9F`
  - `05_adaptors.hpp`: `A02C3C26AFF90A5EF63905C9395B7C55A140449912FFF7122AE0A4682CF1E906`
  - `06_consumers.hpp`: `BFCC2F3B44616C8B4B7CE05B97FF6FB1095890B79879411F5FD08D139698C822`

Fix: Rewrite `validation/good` as a second small implementation with different internal names/structure while preserving the declared public API and behavior. Keep it self-contained under `validation/good/my_ranges/`; do not include or copy `src/reference`.

[MEDIUM] CAPSTONE3 observation contains a direct contradiction about `empty_view`

File: `C06_Ranges/exercises/CAPSTONE3_impl_source_reading/main.cpp:29`

Issue: The comment says `empty_view` is not a `sized_range`, but the next line asserts that it is. This is a teaching artifact, and the task explicitly required clearing old self-contradictions in place.

Evidence:

- `C06_Ranges/exercises/CAPSTONE3_impl_source_reading/main.cpp:29`
- `C06_Ranges/exercises/CAPSTONE3_impl_source_reading/main.cpp:30`

Fix: Replace the comment with the actual claim, for example: `empty_view 是 sized_range 和 borrowed_range；size/distance 为 0`.

[MEDIUM] H3 old `main.cpp` is not linked by the exercise target

File: `C06_Ranges/exercises/H3_generator_const_iter/main.cpp:4`

Issue: `main.cpp` calls `h3::run_generator_const_iter_checks()`, but that function is defined in `checks/generator_const_iter_checks.cpp`, and `ranges_add_exercise` builds targets from the `CHECK` source only. The old `main.cpp` is therefore a nominal entry file, not a linked executable source. It will not fail current CTest because CMake never builds it.

Evidence:

- `C06_Ranges/exercises/H3_generator_const_iter/main.cpp:4`
- `C06_Ranges/exercises/H3_generator_const_iter/CMakeLists.txt:8` passes only `checks/generator_const_iter_checks.cpp`.
- `C06_Ranges/exercises/cmake/RangesSetup.cmake:106` and `:108` call `add_executable(${target} ${ARG_CHECK})`.
- `C06_Ranges/exercises/cmake/RangesSetup.cmake:131` says the old build entry now selects the independent student driver.

Fix: Either remove/replace the obsolete `main.cpp` with explicit navigation text outside the build graph, or wire a real old-entry target that links the check implementation without duplicate `main`. Do not leave a source file that looks executable but is not part of any target.

## Validation

Recorded with `C02_Objects_Lifetime_Ownership/exercises/tools/record_process.py`:

- `advanced-review-configure.json`: PASS, MSVC 19.51 / Visual Studio 18 2026 configure.
- `advanced-review-build-targets.json`: PASS, Release build for H1/H2/H3/CAPSTONE3/CAPSTONE4 reference/good/bad scoped targets.
- `advanced-review-ctest-release.json`: PASS, 13/13 scoped Release tests; includes reference, validation_good, validation_bad_rejected, and CAPSTONE3 observation.
- `advanced-review-student-configure.json`: PASS.
- `advanced-review-student-build-targets.json`: PASS, four scoped Student executables built.
- `advanced-review-student-ctest-expected-fail.json`: PASS wrapper with child `ctest` exit 8; H1/H2/H3/CAPSTONE4 Student tests all failed with target diagnostics.
- `advanced-review-asan-configure.json`: PASS.
- `advanced-review-asan-build-targets.json`: PASS, scoped ASan safe targets built.
- `advanced-review-asan-ctest.json`: PASS, 9/9 scoped ASan safe tests.
- `advanced-review-h3-probe-build-reference-expected-fail.json`: PASS wrapper with expected child build failure proving the H3 const-iterator gap.
- `advanced-review-hash-audit-r3.json`: PASS, hash evidence for Reference/good/bad/Student independence audit.

`lsp_diagnostics` and `ast_grep_search` were not available in this agent surface. I substituted MSVC compile/CTest, ASan scoped runs, direct `rg` static scans for fallback/empty-catch/secret patterns, and the targeted H3 hostile compile probe.

## Recommendation

REQUEST CHANGES.

Minimum repair before re-review:

1. Fix H3 `iter_const_reference_t` and expand checker coverage for real proxy/rvalue iterators.
2. Fix H3 chapter move-only coroutine handle sample in place.
3. Replace CAPSTONE4 `validation/good` with an actually independent implementation.
4. Clean the CAPSTONE3 `empty_view` contradiction.
5. Resolve or explicitly remove the unused H3 `main.cpp` entry.
