# Author A L06 sample gate r4

Scope:

- `chapters/06-exception-safety-and-transactions.md`
- `exercises/L06_transactions/README.md`
- `exercises/L06_transactions/CMakeLists.txt`
- `exercises/L06_transactions/checks/transaction_checks.cpp`
- `exercises/L06_transactions/checks/tracked_value.hpp`
- `exercises/L06_transactions/src/reference/table.hpp`
- `exercises/L06_transactions/src/student/table.hpp`
- `exercises/L06_transactions/validation/good/table.hpp`
- `exercises/L06_transactions/validation/bad/table.hpp`
- `exercises/L06_transactions/validation/bad_prefix/table.hpp`

Fixes after technical review r2:

- Checker no longer assumes Reference's copy count is the only legal implementation.
- `replace_prefix_strong()` k = 1..5 rule: if injected copy failure is reached, original table must remain unchanged; if the implementation performs fewer copies and no exception occurs, the final result must be complete.
- `validation/good/table.hpp` now uses a distinct legal algorithm: build final sequence from source prefix plus old suffix, then swap.
- `validation/bad_prefix/table.hpp` remains the live assignment commit anti-pattern and is rejected by the same checker.
- `validation/bad/table.hpp` still covers clear-then-copy all-replace failure and is independently rejected.

Fresh local commands and raw logs:

- `r4-configure-core.txt`
- `r4-build-core-debug.txt`
- `r4-ctest-core-debug.txt`
- `r4-configure-student.txt`
- `r4-build-student-debug.txt`
- `r4-ctest-student-debug.txt`
- `r4-source-sha256.txt`

Core result:

```text
100% tests passed, 0 tests failed out of 4
L06_transactions_reference: Passed
L06_transactions_validation_good: Passed
L06_transactions_validation_bad_rejected: Passed
L06_transactions_validation_bad_prefix_rejected: Passed
```

Student-on result:

```text
L06_transactions_student: Failed
check failed: basic prefix failure reports copy error
L06_transactions_reference: Passed
L06_transactions_validation_good: Passed
L06_transactions_validation_bad_rejected: Passed
L06_transactions_validation_bad_prefix_rejected: Passed
80% tests passed, 1 tests failed out of 5
```

Source SHA:

- Full file hashes are in `r4-source-sha256.txt`.
- `validation/good/table.hpp` SHA differs from `src/reference/table.hpp`.

