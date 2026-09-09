# Author A L06 sample gate r2

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

Changes from r1:

- Moved `TrackedValue` failure fixture into `checks/tracked_value.hpp`; every implementation consumes the same trusted fixture.
- Kept Student work surface to `Table`; Student placeholder is buildable and fails through `check()` instead of unhandled exceptions or timeout.
- Made `validation/good/table.hpp` an independent implementation instead of including Reference.
- Added `replace_prefix_strong()` so the same prefix input can compare basic and strong failure guarantees.
- Fixed `replace_prefix_strong()` commit path: copy full backup, mutate backup, then `vector::swap`.
- Added checks for oversized prefix rejection and reuse, same-success prefix contract, strong prefix failure, and all prepare copy failures at k = 1, 2, 3.

Fresh local commands:

```powershell
cmake -S C03_Type_Modeling_Interface_Design/exercises/L06_transactions -B build/c03-author-a-l06-core -G "Visual Studio 18 2026" -A x64
cmake --build build/c03-author-a-l06-core --config Debug
ctest --test-dir build/c03-author-a-l06-core -C Debug --output-on-failure
```

Result:

```text
100% tests passed, 0 tests failed out of 3
L06_transactions_reference: Passed
L06_transactions_validation_good: Passed
L06_transactions_validation_bad_rejected: Passed
```

Student wiring command:

```powershell
cmake -S C03_Type_Modeling_Interface_Design/exercises/L06_transactions -B build/c03-author-a-l06 -G "Visual Studio 18 2026" -A x64 -DTYPE_STUDY_TEST_STUDENTS=ON
cmake --build build/c03-author-a-l06 --config Debug
ctest --test-dir build/c03-author-a-l06 -C Debug --output-on-failure
```

Result:

```text
L06_transactions_student: Failed
check failed: basic prefix failure reports copy error
L06_transactions_reference: Passed
L06_transactions_validation_good: Passed
L06_transactions_validation_bad_rejected: Passed
75% tests passed, 1 tests failed out of 4
```

Interpretation:

- Core sample passes Reference and independent good implementation.
- Bad implementation is rejected by expected-failure wrapper.
- Student is registered only when requested and fails through normal checker diagnostics.
- This evidence covers Debug on local MSVC 19.51.36256.0 / Visual Studio 18 2026 generator. Release, ASan, and non-Windows are not claimed here.

