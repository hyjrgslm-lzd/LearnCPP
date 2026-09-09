# L06 sample teaching review r2

Reviewer: non-author teaching review.
Status: ITERATE.

This is a formative review of the frozen L06 sample, not final approval for C03. L06 does not depend on chapters 03-05; optional, variant, and expected depth remain separate course blockers for their own review lane.

## Reviewed snapshot

- `chapters/00-route-and-boundaries.md`: `23AB39D5CF9A514C65066EBF47E28DC619CAE4F1B3A565B1C652517C25E6027A`
- `chapters/01-class-invariants.md`: `8E9032214879BF4A6710D0882F06DD9FB1F85D69211B6E2D62CDE06F2725D745`
- `chapters/02-value-semantics.md`: `05448CC0448BD618592BAF072859871069E951BF955B921FB22E4817C6393434`
- `chapters/06-exception-safety-and-transactions.md`: `FCB975ACB1FEC036A3BD09082886DA036C04B6F8CE353BADF4BB114265902627`
- `exercises/L06_transactions/README.md`: `C331DF1F30392D330D1A6FE1602BFF4D34834713EDB0559F9C7FB08B8669F191`
- `exercises/L06_transactions/checks/tracked_value.hpp`: reviewed through current file content.
- `exercises/L06_transactions/checks/transaction_checks.cpp`: `4511A0EA48F8F365D501AFBB6608FDD2147E0D9FEC5AA5701DC3E7D7836998BC`
- `exercises/L06_transactions/src/student/table.hpp`: `ED5C362569F7234FBADAE7096B71DF7FC188C8D0ED9AA59803F6681D7D946084`
- `exercises/L06_transactions/src/reference/table.hpp`: `4912003BDBD36AFB2EB661779926EE44F4FFF3E399AAD225647EEF805055DB6C`
- `exercises/L06_transactions/validation/good/table.hpp`: `4912003BDBD36AFB2EB661779926EE44F4FFF3E399AAD225647EEF805055DB6C`
- `exercises/L06_transactions/validation/bad/table.hpp`: `1629EF4F1ED250463C753FA55792F7C562DC2A7D38828E6C709CA8080FD64D11`
- `references/validation/author-a-l06/r2-sample-gate.md`: `1FB54776091D478011F57666412761983A2B912B1879853B938F3E06CB67FA73`

## Independent validation

Core leaf:

```powershell
cmake -S C03_Type_Modeling_Interface_Design/exercises/L06_transactions -B build/c03-critic-l06-core -G "Visual Studio 18 2026" -A x64
cmake --build build/c03-critic-l06-core --config Debug
ctest --test-dir build/c03-critic-l06-core -C Debug --output-on-failure
```

Result: configure and build succeeded with MSVC 19.51.36256.0 / Visual Studio 18 2026 generator. CTest passed 3/3: `L06_transactions_reference`, `L06_transactions_validation_good`, and `L06_transactions_validation_bad_rejected`.

Student leaf:

```powershell
cmake -S C03_Type_Modeling_Interface_Design/exercises/L06_transactions -B build/c03-critic-l06-student -G "Visual Studio 18 2026" -A x64 -DTYPE_STUDY_TEST_STUDENTS=ON
cmake --build build/c03-critic-l06-student --config Debug
ctest --test-dir build/c03-critic-l06-student -C Debug --output-on-failure
```

Result: configure and build succeeded. CTest produced the intended student failure: `L06_transactions_student` failed with `check failed: basic prefix failure reports copy error`; reference, validation good, and validation bad rejected passed.

## Passing points

- The L06 hard prerequisites are limited to C02 copy/RAII/vector/span plus C03 00-02 invariants and value semantics. I found no required optional, variant, or expected mechanism in L06.
- `TrackedValue` is now a shared checker fixture rather than a student-owned no-op fixture, so a learner can leave the fixture alone and edit only `Table`.
- The core leaf has a real reference pass, a good pass, and an expected bad rejection; the student leaf fails through `check()` rather than timeout or unhandled process behavior.
- `replace_prefix_basic` and `replace_prefix_strong` now use the same successful input shape, so the intended distinction is the failure guarantee, not a different task.

## Blocking issue

`chapters/06-exception-safety-and-transactions.md` still teaches an unsafe strong-prefix commit shape. Lines describing `replace_prefix_strong` say to copy a temporary prefix and then overwrite the target prefix; the final解析 repeats this as "strong prefix 先准备前缀副本，复制全部成功后再覆盖". Because `TrackedValue::operator=` can throw, overwriting the target prefix after preparing only the prefix is itself a throwing commit phase. That does not establish the strong guarantee for later assignment failures.

The reference implementation avoids this by copying the whole table, mutating the backup, and swapping (`src/reference/table.hpp`), but the teaching text and the checker do not force that lesson. The checker covers `replace_prefix_strong` only with `throw_on_copy(2)`, which fails before the target-overwrite phase for a "copy prefix then assign target" implementation. A learner following the current prose could plausibly pass while still using a throwing commit path.

Minimum repair:

- Change the L06 prose and README解析 so strong prefix is taught as "copy the whole current table, mutate the copy, then no-throw swap" or another design with a genuinely non-throwing commit.
- Add a prefix-strong failure check that reaches after the temporary preparation stage, for example copy indices covering the backup copy plus source assignment for `table{1,2,3}` and `source{7,8}`.
- Add or adjust a bad variant that copies only the prefix and then assigns into the live target, and prove the checker rejects it.

Stop condition for sample approval: after the above repair, rerun the same core and student leaf commands, bind new SHAs, and have this review lane re-check the specific issue.
