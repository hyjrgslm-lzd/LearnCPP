# C05 text/time technical review 2026-09-10

Scope reviewed:

- `include/c05/text.hpp`, `time.hpp`, `paths.hpp`, `config.hpp`, and supporting `utf.hpp`.
- Exercises `L07_text_semantics` through `L13_configuration`.
- `U01_icu_unicode` full Unicode data driver.
- `F01_frontier` probes and chapter `18-standard-frontier.md`.
- Chapters `07` through `13` for technical fact alignment.

## Verdict

COMMENT.

No CRITICAL/HIGH/MEDIUM/LOW implementation issues found in this slice. Formal APPROVE is withheld only because the required `lsp_diagnostics`/`ast_grep_search` tools were not available in this agent surface; fresh compile/test/direct-run evidence passed.

## Spec compliance

- `text.hpp` uses `std::from_chars`, rejects empty input, classifies invalid/range/trailing data, checks full consumption, and rejects non-finite double results.
- `config.hpp` enforces UTF-8 first, rejects NUL, supports LF/CRLF and initial BOM, rejects bare/inner CR with byte offset and line, keeps fixed keys/defaults, rejects duplicates, and validates `package_file` as a Win32-safe basename including ASCII-folded device aliases.
- `paths.hpp` validates only generic relative resource metadata and does not call `canonical` or `equivalent`.
- `time.hpp` keeps UTC path independent of tzdb, catches only `std::runtime_error` from zone acquisition, uses classic locale formatting, prints offset seconds, and rejects local display dates outside 0001-9999.
- L11 `tzdb_observation.cpp` broad-catches the whole capability probe, but `check()` exits instead of throwing, so business assertion failures are not converted into CTest SKIP.
- U01 checks runtime ICU 77.1 and Unicode 16.0, validates UTF-8 before ICU conversion, uses explicit UTF-8 byte length, drives all `NormalizationTest.txt` and `GraphemeBreakTest.txt` records, and maps UTF-16 boundaries back to UTF-8 offsets.
- F01 OFF registers zero tests. F01 ON registers 9 capability probes. Macro/header absence returns 77; semantic mismatch after capability claim returns FAIL. P3395 error_code formatter is gated by configure-time real compilation, and P3505 float format distinguishes known old output from unexpected failure.

## Verification

Fresh validation record: `references/validation/text-time-review-20260910.md`.

Key results:

- L07-L13 Release leaf builds/tests: all passed.
- U01 Release and Debug direct runs: `ICU 77.1 Unicode 16.0`, `NormalizationTest records 19965`, `GraphemeBreakTest records 1093`.
- L11 tzdb direct run: `tzdb version: 2022g.27`.
- F01 OFF: `Total Tests: 0`.
- F01 ON: 9/9 capability tests SKIP with precise unsupported reasons, no hidden FAIL.
- Extra contract probe over text/config/path/time edge cases: passed.

## Issues

None.

## Residual risk

`lsp_diagnostics` and `ast_grep_search` unavailable; used MSVC compile, CTest, direct exe runs, and `rg` scans instead. Under the active code-review rule, that prevents a formal APPROVE despite no defects found.
