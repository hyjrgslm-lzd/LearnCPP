# L06 sample technical review r4

Date: 2026-09-09.
Role: non-author technical and experiment review.
Verdict: ITERATE.

## Scope

Reviewed:

- `references/validation/author-a-l06/r4-sample-gate.md`
- `chapters/06-exception-safety-and-transactions.md`
- `exercises/L06_transactions/README.md`
- `exercises/L06_transactions/CMakeLists.txt`
- `exercises/L06_transactions/checks/tracked_value.hpp`
- `exercises/L06_transactions/checks/transaction_checks.cpp`
- `exercises/L06_transactions/src/student/table.hpp`
- `exercises/L06_transactions/src/reference/table.hpp`
- `exercises/L06_transactions/validation/good/table.hpp`
- `exercises/L06_transactions/validation/bad/table.hpp`
- `exercises/L06_transactions/validation/bad_prefix/table.hpp`
- `exercises/cmake/StudySetup.cmake`
- `exercises/cmake/expect_failure.cmake`

## Source Hashes

- `r4-sample-gate.md`: `CDB95D0A7D1BB91B2F87A5A8A48C3A10F793EA4B198C95F37FF171386175E17D`
- `06-exception-safety-and-transactions.md`: `A5CA98EED7277368C5AC1F5A5921A174DE3DBCCD187A808D4FAEBE26787AC780`
- `README.md`: `7B25BC2DA591E39F1A76E4C8BC9A0E3318B283D649689176001C063F7FB580AF`
- `CMakeLists.txt`: `DD06F506A2432DDB678CD4856544A0679F88B1531B78313A1D31A1D7072EADB3`
- `tracked_value.hpp`: `2CB36E52B8FD98FBCC543EBD85289341CA9FC9754712458A62B860BF2860D036`
- `transaction_checks.cpp`: `1BC25904EDC67078520075DE74F7AD1EB4A2C9267B8B90697520C94543BA1EA6`
- `src/student/table.hpp`: `ED5C362569F7234FBADAE7096B71DF7FC188C8D0ED9AA59803F6681D7D946084`
- `src/reference/table.hpp`: `4912003BDBD36AFB2EB661779926EE44F4FFF3E399AAD225647EEF805055DB6C`
- `validation/good/table.hpp`: `A6CEB0B828C05FC569E73D8B74815BE5C7987438042A7C189A7577CE4595D1E9`
- `validation/bad/table.hpp`: `1629EF4F1ED250463C753FA55792F7C562DC2A7D38828E6C709CA8080FD64D11`
- `validation/bad_prefix/table.hpp`: `57BE34AD12825564BE206047D7B6B47626E8FD0F1A006B1369C1A1F4F41AD42F`
- `StudySetup.cmake`: `9BFC7790C46B939DFD6B3C266F81E9D1E37D662528D3588A06275740494EF47D`
- `expect_failure.cmake`: `39E4769E9C801F19643ABF99DE0063FC0B3D0EECB91919CF9F10A10071E969C4`

`validation/good/table.hpp` now differs from `src/reference/table.hpp`. Manual read confirmed the intended algorithm split: Reference copies the full table and updates the copy; good constructs the final sequence from source prefix plus old suffix.

## Blocking Finding

### HIGH: course-level `validation_bad_rejected` does not isolate the all-replace bad variant

Files:

- `exercises/L06_transactions/CMakeLists.txt:5`
- `exercises/L06_transactions/CMakeLists.txt:9`
- `exercises/L06_transactions/CMakeLists.txt:17`
- `exercises/L06_transactions/validation/bad/table.hpp:35`
- `exercises/L06_transactions/validation/bad/table.hpp:40`

Issue: `validation/bad/table.hpp` contains both a bad `replace_prefix_strong()` and a bad `replace_all_strong()`. The `c03_add_exercise()` target `L06_transactions_validation_bad_rejected` uses the full checker and `BAD_DIAGNOSTIC "strong prefix failure keeps original prefix"`. Direct reviewer run `sample-technical-r4-run-bad-all-debug.json` expected the all-replace diagnostic from `L06_transactions_validation_bad.exe`, but the program exited first with:

```text
check failed: strong prefix failure keeps original prefix
```

So the shipped course test proves that `validation_bad` is rejected, but not that the clear-then-copy all-replace anti-pattern is independently rejected. The separate new `validation_bad_prefix` target is also rejected for the same prefix diagnostic, leaving the all-replace bad path hidden behind an earlier failure in the same bad implementation.

Fix: split the bad implementations. Keep `validation/bad_prefix/table.hpp` for the live prefix assignment commit anti-pattern. Make `validation/bad/table.hpp` or a new `validation/bad_all/table.hpp` use a legal `replace_prefix_strong()` and only break `replace_all_strong()`, then bind that target to an all-replace diagnostic such as `strong failure keeps original table`. Rerun Debug, Release, ASan, and the direct bad-all negative check.

## Passing Evidence

All commands were run with bounded timeouts through `C02_Objects_Lifetime_Ownership/exercises/tools/record_process.py` unless noted.

Core:

- `sample-technical-r4-configure-core.json`: PASS
- `sample-technical-r4-build-core-debug.json`: PASS
- `sample-technical-r4-build-core-release.json`: PASS
- `sample-technical-r4-ctest-core-debug.json`: PASS, 4/4 tests passed
- `sample-technical-r4-ctest-core-release.json`: PASS, 4/4 tests passed

Student/ref-off:

- `sample-technical-r4-configure-student-ref-off.json`: PASS
- `sample-technical-r4-build-student-ref-off-debug.json`: PASS
- `sample-technical-r4-run-student-placeholder-debug.json`: PASS as expected failure, exit 1 with `check failed: basic prefix failure reports copy error`
- `sample-technical-r4-configure-audit-student-ref-off.json`: PASS
- `sample-technical-r4-build-audit-student-trace-debug.json`: PASS
- `sample-technical-r4-audit-student-ref-off.json`: PASS; one student target, no reference dependency found, actual include count 150

ASan:

- `sample-technical-r4-configure-asan.json`: PASS
- `sample-technical-r4-build-asan-relwithdebinfo.json`: PASS
- `sample-technical-r4-ctest-asan-relwithdebinfo.json`: PASS, 4/4 tests passed
- copied ASan DLL in target dir: `7F901A239979C49B36A3B18ECD1843F551F94B5879585BDED41A650C5B71F35F  build/c03-review-l06-r4-asan/RelWithDebInfo/clang_rt.asan_dynamic-x86_64.dll`

Negative variants:

- `sample-technical-r4-run-bad-prefix-debug.json`: PASS as expected failure, exit 1 with `check failed: strong prefix failure keeps original prefix`
- `sample-technical-r4-run-bad-all-only-debug.json`: PASS as expected failure in reviewer all-only harness, exit 1 with `check failed: all-only strong failure keeps original table`

Reviewer adversarial behavior contract:

- `sample-technical-r4-adversarial.cpp`
  - SHA256: `E2A64289E11B762CADABEC50D628848DBF0BD5A4FE536715589645CECCFB8F23`
- `sample-technical-r4-adversarial/CMakeLists.txt`
  - SHA256: `4AC3D49E82DAE3DE264E3B7DDAF35D4ED881B79E8770D4BCCD29E3B82194A050`
- `sample-technical-r4-configure-adversarial.json`: PASS
- `sample-technical-r4-build-adversarial-debug.json`: PASS
- `sample-technical-r4-run-reference-adversarial-debug.json`: PASS
- `sample-technical-r4-run-good-adversarial-debug.json`: PASS

This harness checks behavior rather than implementation copy counts: across copy indices 1..8, strong prefix either throws and keeps the original table or succeeds with the complete prefix result. It also checks self all-replace across copy indices 1..6, oversize rejection and reuse, and `Table::swap` `noexcept`.

Reviewer all-only harness:

- `sample-technical-r4-bad-all-only.cpp`
  - SHA256: `B43787BC0B770D60C3A1743C83FDFFCB8C01C10B9895ECB9B74C11053E2C4905`
- `sample-technical-r4-bad-all-only/CMakeLists.txt`
  - SHA256: `C007E90CF4B0BD42ECA30C9DA189B22BC4AAE33C0DAFEA28DA9ADBF388035BF2`
- `sample-technical-r4-configure-bad-all-only.json`: PASS
- `sample-technical-r4-build-bad-all-only-debug.json`: PASS
- `sample-technical-r4-run-reference-all-only-debug.json`: PASS
- `sample-technical-r4-run-good-all-only-debug.json`: PASS
- `sample-technical-r4-run-bad-all-only-debug.json`: PASS as expected failure

## Residual Risk

No source fixes were made. The only remaining blocker is evidence wiring for the built-in all-replace bad variant. Current runtime behavior and reviewer-only evidence support the Table contracts, but the course-owned negative target should prove bad-all directly before this sample gate is marked final approved.
