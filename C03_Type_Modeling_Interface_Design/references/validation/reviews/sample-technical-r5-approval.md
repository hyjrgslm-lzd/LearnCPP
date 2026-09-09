# L06 sample technical review r5

Date: 2026-09-09.
Role: non-author technical and experiment review.
Verdict: APPROVE.

## Scope

This r5 review closes the only r4 blocker: the built-in course negative tests now isolate the all-replace bad implementation and the bad-prefix implementation separately. No author source was modified by this review.

Files re-read:

- `exercises/L06_transactions/CMakeLists.txt`
- `exercises/L06_transactions/validation/bad/table.hpp`
- `references/validation/sample-bad-isolation-build-r5.json`
- `references/validation/sample-bad-isolation-ctest-r5.json`

## Hash Check

Expected unchanged from r4 and verified unchanged:

- `chapters/06-exception-safety-and-transactions.md`: `A5CA98EED7277368C5AC1F5A5921A174DE3DBCCD187A808D4FAEBE26787AC780`
- `exercises/L06_transactions/README.md`: `7B25BC2DA591E39F1A76E4C8BC9A0E3318B283D649689176001C063F7FB580AF`
- `exercises/L06_transactions/checks/tracked_value.hpp`: `2CB36E52B8FD98FBCC543EBD85289341CA9FC9754712458A62B860BF2860D036`
- `exercises/L06_transactions/checks/transaction_checks.cpp`: `1BC25904EDC67078520075DE74F7AD1EB4A2C9267B8B90697520C94543BA1EA6`
- `exercises/L06_transactions/src/student/table.hpp`: `ED5C362569F7234FBADAE7096B71DF7FC188C8D0ED9AA59803F6681D7D946084`
- `exercises/L06_transactions/src/reference/table.hpp`: `4912003BDBD36AFB2EB661779926EE44F4FFF3E399AAD225647EEF805055DB6C`
- `exercises/L06_transactions/validation/good/table.hpp`: `A6CEB0B828C05FC569E73D8B74815BE5C7987438042A7C189A7577CE4595D1E9`
- `exercises/L06_transactions/validation/bad_prefix/table.hpp`: `57BE34AD12825564BE206047D7B6B47626E8FD0F1A006B1369C1A1F4F41AD42F`
- `exercises/cmake/StudySetup.cmake`: `9BFC7790C46B939DFD6B3C266F81E9D1E37D662528D3588A06275740494EF47D`
- `exercises/cmake/expect_failure.cmake`: `39E4769E9C801F19643ABF99DE0063FC0B3D0EECB91919CF9F10A10071E969C4`

Changed in r5 and verified:

- `exercises/L06_transactions/CMakeLists.txt`: `CA1023590841BD9A054D5C4B0D521A28DCCE42FD5EBABFE29E45B7E2BD9239EF`
- `exercises/L06_transactions/validation/bad/table.hpp`: `7A8CC3A4B338FBA2277F2F46AA5C11144C9AF1DFD4064EBF190D32DA5A09A015`

The changed files match the stated fix: `validation/bad/table.hpp` now has a correct strong-prefix implementation and keeps only the clear-then-copy all-replace bug; `CMakeLists.txt` now expects `strong failure keeps original table` for `validation_bad_rejected`.

Leader evidence hashes:

- `references/validation/sample-bad-isolation-build-r5.json`: `A6892561C8F4ADCCCD18867E1A51DD33A2D0210F9540E1D630B5208BCC16E0DD`
- `references/validation/sample-bad-isolation-ctest-r5.json`: `4A4E01C03EC7C069B1181B72611558F9E94E191ED8C8E19EA6DA0611AA936CA8`

## Fresh Review Evidence

Review evidence:

- `sample-technical-r5-build-bad-targets-debug.json`
  - SHA256: `CB0110DC1589183F874400F846359D6192C92A7F3D35615DC59CC3E40B67F0C4`
  - Result: PASS, rebuilt `L06_transactions_validation_bad` and `L06_transactions_validation_bad_prefix`.
- `sample-technical-r5-ctest-negative-debug.json`
  - SHA256: `8E5D74922E8296717B95965E76641A56A481607EA2E77B9CE030AF7F0934E56B`
  - Result: PASS, 2/2 negative tests passed.
  - `L06_transactions_validation_bad_rejected`: rejected with `strong failure keeps original table`.
  - `L06_transactions_validation_bad_prefix_rejected`: rejected with `strong prefix failure keeps original prefix`.
- `sample-technical-r5-run-bad-all-debug.json`
  - SHA256: `E3163D91D2C5A0E3AC63486424134FC40E831BB160BCC04E6591D856BC9BBDC0`
  - Result: PASS as expected failure, exit 1 with `check failed: strong failure keeps original table`.
- `sample-technical-r5-run-bad-prefix-debug.json`
  - SHA256: `8332E839674089CD6EC03DE2E2B2385513CC80914CABA0ADAC239C732DA84F6E`
  - Result: PASS as expected failure, exit 1 with `check failed: strong prefix failure keeps original prefix`.

The leader r5 Release verbose evidence was also read. It shows the same separation in Release: all-replace bad is rejected with `strong failure keeps original table`, and bad-prefix is rejected with `strong prefix failure keeps original prefix`.

## Verdict

APPROVE. The r4 technical blocker is closed. Prior r4 evidence remains applicable to unchanged files: Debug/Release core, Student ref-off direct run, Student wiring audit, ASan safe path, Reference/good adversarial behavior, and reviewer all-only harness. No remaining technical blocker was found for the L06 sample gate.
