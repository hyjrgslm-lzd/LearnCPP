# C04 A01/A03 values independent review

日期：2026-09-10

结论：ITERATE。A01/A03 的核心教学链路、Student 初态失败、Reference/good/bad/diagnostics 架构基本成立；但 A01 明确写了“key 最长 15 个 ASCII 字符”，Reference/good/bad 的 `make_row` 实际接受 16 字符 key，且不拒绝非 ASCII byte。该边界在原检查器中也没有覆盖。

## Files Reviewed

- `chapters/18-compiletime-values.md`
- `chapters/20-explicit-object-forwarding.md`
- `exercises/A01_compiletime_values/README.md`
- `exercises/A01_compiletime_values/CMakeLists.txt`
- `exercises/A01_compiletime_values/checks/compiletime_values_checks.cpp`
- `exercises/A01_compiletime_values/src/student/compiletime_values.hpp`
- `exercises/A01_compiletime_values/src/reference/compiletime_values.hpp`
- `exercises/A01_compiletime_values/validation/good/compiletime_values.hpp`
- `exercises/A01_compiletime_values/validation/bad/compiletime_values.hpp`
- `exercises/A01_compiletime_values/validation/diagnostics/*`
- `exercises/A03_explicit_object/README.md`
- `exercises/A03_explicit_object/CMakeLists.txt`
- `exercises/A03_explicit_object/checks/explicit_object_checks.cpp`
- `exercises/A03_explicit_object/src/student/explicit_object.hpp`
- `exercises/A03_explicit_object/src/reference/explicit_object.hpp`
- `exercises/A03_explicit_object/validation/good/explicit_object.hpp`
- `exercises/A03_explicit_object/validation/bad/explicit_object.hpp`
- `exercises/A03_explicit_object/validation/diagnostics/*`
- `references/reviews/revision-values-author.md`

## Severity

- CRITICAL: 0
- HIGH: 0
- MEDIUM: 1
- LOW: 0

## Issues

### [MEDIUM] A01 key boundary is specified but not implemented or tested

File: `chapters/18-compiletime-values.md:43`, `exercises/A01_compiletime_values/README.md:3`, `exercises/A01_compiletime_values/src/reference/compiletime_values.hpp:16`, `exercises/A01_compiletime_values/src/reference/compiletime_values.hpp:17`, `exercises/A01_compiletime_values/validation/good/compiletime_values.hpp:16`, `exercises/A01_compiletime_values/validation/good/compiletime_values.hpp:17`

Issue: The lesson and README say the table uses keys up to 15 ASCII characters, but Reference and good use `static_assert(N <= row::key_width + 1)` with `key_width == 16`, so a 16-character literal is accepted. They also copy bytes without checking ASCII. The original A01 checker covers sorting, first-wins dedup, empty input, shape, and lookup, but not this declared boundary.

Evidence: `values-blind/09-a01-boundary-run.json` runs a probe against the current Reference and prints `A01 boundary gap reproduced: implementation accepts 16-char and non-ASCII keys`. The probe compiles `make_row("1234567890123456", 16)` and `make_row("\xC3\xA9", 1)`, then finds both values.

Why it matters: This is a teaching/spec mismatch, not a runtime security bug. A learner following the README sees a stricter contract than the implementation and checks enforce. It also conflicts with the chapter sample at `chapters/18-compiletime-values.md:33`, which permits `key_width + 1`, while nearby prose says 15 ASCII characters.

Fix: Pick one contract and make code, docs, and diagnostics match. The smaller repair is to enforce the stated contract: change `make_row` to `static_assert(N <= row::key_width, "literal key is too long")` because `N` includes the trailing null; add a consteval ASCII byte check; add negative compile cases for a 16-character literal and a non-ASCII byte string; mirror the same behavior in Reference, good, bad, and Student starter guidance. If the intended contract is “up to 16 raw bytes”, change the prose instead and remove the ASCII claim.

## Passing Evidence

- Blind solvability: `values-blind/01-a01-configure.json`, `02-a01-build.json`, `03-a01-run.json` are PASS using only the chapter, README, checker, and Student initial header. This proves A01 can be solved from the visible teaching material, aside from the boundary rule above.
- Frozen blind inputs: `values-blind/05-input-sha-pwsh.json` is PASS. `values-blind/04-input-sha.json` is a retained wrapper/tooling failure: Windows PowerShell in that child process did not resolve `Get-FileHash`.
- A01 current full gate: `values-blind/11-a01-full-configure.json`, `12-a01-full-build.json`, `14-a01-full-ctest-r2.json` are PASS; Debug CTest reports 4/4 passed. `13-a01-full-ctest.json` is a retained wrapper assertion mistake, while its child `ctest` exit code is 0 and output says 100% passed.
- A03 current full gate: `values-blind/15-a03-full-configure.json`, `16-a03-full-build.json`, `17-a03-full-ctest.json` are PASS; Debug CTest reports 4/4 passed.
- A03 extra probe: `values-blind/10-a03-extra-run.json` is PASS. It confirms `const&& value()` returns `const int&&`, lvalue/const lvalue/const rvalue `take()` are rejected, and `name_holder::name()` preserves borrow references.
- Student initial states: existing author records `values-a01-student-configure.json`, `values-a01-student-build-debug.json`, `values-a01-student-ctest-debug-expected-fail.json`, `values-a03-student-configure.json`, `values-a03-student-build-debug.json`, `values-a03-student-ctest-debug-expected-fail.json` show both Student targets build and then fail with intended checker messages.
- Static isolation: `values-static-good-isolation-r2.json` reports clean for good/bad Reference includes and TODO/FIXME. `values-source-hashes-r2.json` records owned source hashes.

## Stage Checks

Stage 1 spec compliance: fails on the A01 key boundary above. Core A01 behavior otherwise matches the request: stable sort, first-wins dedup before sorting, `table<N>` shape, binary lookup, empty input, and scratch pointer negative diagnostic are covered. A03 matches the requested cvref, move-only, `const&& take`, `noexcept`, recursive lambda, and borrowing behavior.

Root-cause fallback guard: PASS. I did not find masking fallback/workaround branches that swallow failures or route around the primary contract. The retained failed JSON records preserve evidence rather than hiding it.

Stage 2 code quality/security: no hardcoded secret pattern, broad catch, `std::function` implementation use, TODO/FIXME, or Reference include leak was found in the reviewed A01/A03 source scope. `lsp_diagnostics` and `ast_grep_search` were not callable in this environment; I used compiler/CTest plus `rg` static scans instead.

## Recommendation

ITERATE. Fix the A01 key boundary or rewrite the stated contract, then rerun A01/A03 targeted gates and keep the new boundary probe evidence.
