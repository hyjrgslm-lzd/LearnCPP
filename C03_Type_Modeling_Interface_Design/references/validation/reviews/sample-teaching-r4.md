# L06 sample teaching review r4

Reviewer: non-author teaching review.
Status: APPROVE for the L06 sample gate only.

This review approves the L06 sample as a teaching gate for later C03 batches. It does not approve the full C03 course. Chapters 03-05 are outside this L06 hard-prerequisite review and still require their own depth review.

## Reviewed snapshot

- `chapters/00-route-and-boundaries.md`: `9215D6FC5536C0BDE7119C639745F4C7BC709FDF3E4A3538B6D60DC5FD2C529B`
- `chapters/01-class-invariants.md`: `0278051206907970FAB1D4A15CFBAF21065FF64E7394ACED85293E3ACD95711C`
- `chapters/02-value-semantics.md`: `60477869A8E45C35516714E38D11429B9B202A97282DAD65ED868FC1DCCD4174`
- `chapters/06-exception-safety-and-transactions.md`: `A5CA98EED7277368C5AC1F5A5921A174DE3DBCCD187A808D4FAEBE26787AC780`
- `exercises/L06_transactions/README.md`: `7B25BC2DA591E39F1A76E4C8BC9A0E3318B283D649689176001C063F7FB580AF`
- `exercises/L06_transactions/checks/tracked_value.hpp`: `2CB36E52B8FD98FBCC543EBD85289341CA9FC9754712458A62B860BF2860D036`
- `exercises/L06_transactions/checks/transaction_checks.cpp`: `1BC25904EDC67078520075DE74F7AD1EB4A2C9267B8B90697520C94543BA1EA6`
- `exercises/L06_transactions/src/student/table.hpp`: `ED5C362569F7234FBADAE7096B71DF7FC188C8D0ED9AA59803F6681D7D946084`
- `exercises/L06_transactions/src/reference/table.hpp`: `4912003BDBD36AFB2EB661779926EE44F4FFF3E399AAD225647EEF805055DB6C`
- `exercises/L06_transactions/validation/good/table.hpp`: `A6CEB0B828C05FC569E73D8B74815BE5C7987438042A7C189A7577CE4595D1E9`
- `exercises/L06_transactions/validation/bad/table.hpp`: `1629EF4F1ED250463C753FA55792F7C562DC2A7D38828E6C709CA8080FD64D11`
- `references/validation/author-a-l06/r4-sample-gate.md`: `CDB95D0A7D1BB91B2F87A5A8A48C3A10F793EA4B198C95F37FF171386175E17D`

## Independent validation

Core leaf:

```powershell
cmake -S C03_Type_Modeling_Interface_Design/exercises/L06_transactions -B build/c03-critic-l06-r4-core -G "Visual Studio 18 2026" -A x64
cmake --build build/c03-critic-l06-r4-core --config Debug
ctest --test-dir build/c03-critic-l06-r4-core -C Debug --output-on-failure
```

Result: configure and build succeeded with MSVC 19.51.36256.0 / Visual Studio 18 2026 generator. CTest passed 4/4: reference, validation good, bad rejected, and bad prefix rejected.

Student leaf:

```powershell
cmake -S C03_Type_Modeling_Interface_Design/exercises/L06_transactions -B build/c03-critic-l06-r4-student -G "Visual Studio 18 2026" -A x64 -DTYPE_STUDY_TEST_STUDENTS=ON
cmake --build build/c03-critic-l06-r4-student --config Debug
ctest --test-dir build/c03-critic-l06-r4-student -C Debug --output-on-failure
```

Result: configure and build succeeded. CTest produced the intended student failure: `L06_transactions_student` failed with `check failed: basic prefix failure reports copy error`; reference, validation good, bad rejected, and bad prefix rejected passed.

## Review result

- The prior blocking issue is closed. The chapter and README now teach strong prefix as either whole-table-copy/mutate-copy/swap or final-sequence/swap, and explicitly reject copying only the prefix then assigning back into the live table.
- The checker no longer assumes one legal copy count. For prefix strong, copy injection k = 1..5 accepts either old-table preservation when the injected failure is reached or complete success when the implementation uses fewer copies.
- The new `validation/bad_prefix` covers the live-commit anti-pattern and is rejected by the same checker.
- A learner can complete L06 while editing only `src/student/table.hpp`; `TrackedValue` is a shared fixture under `checks/`.
- The L06 hard-prerequisite path is sufficient for this sample: C02 copy/RAII/vector/span plus C03 00-02 invariants, value semantics, and no-throw swap as commit point.

No teaching blockers remain for the L06 sample gate. Stop condition for this review is met: specific prior issue repaired, affected controls rerun, and new SHAs bound.
