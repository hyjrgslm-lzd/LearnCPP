# Author A L06 sample gate r3

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

Fixes after teaching review r2:

- Corrected chapter and exercise text: `replace_prefix_strong()` uses full table copy, updates prefix on the copy, then swaps.
- `replace_prefix_strong()` checker now covers copy failure points k = 1..5: three copies for the table backup, then two copies for updating the backup prefix.
- Added `validation/bad_prefix/table.hpp`, which copies the input prefix first, then performs throwing assignment into the live table.
- Added `L06_transactions_validation_bad_prefix_rejected`, a separate expected-failure target using the same checker.

Fresh local commands and raw logs:

- `r3-configure-core.txt`
- `r3-build-core-debug.txt`
- `r3-ctest-core-debug.txt`
- `r3-configure-student.txt`
- `r3-build-student-debug.txt`
- `r3-ctest-student-debug.txt`
- `r3-source-sha256.txt`

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

Interpretation:

- Core L06 sample passes Reference and independent good implementation.
- Both bad implementations are rejected by expected-failure wrappers.
- Student is buildable and fails through normal checker diagnostics when explicitly registered.
- This evidence covers Debug on local MSVC 19.51.36256.0 / Visual Studio 18 2026 generator.

