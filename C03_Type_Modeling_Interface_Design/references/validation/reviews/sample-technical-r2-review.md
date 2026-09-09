# L06 sample technical review r2

Date: 2026-09-09.
Role: non-author technical and experiment review.
Verdict: ITERATE. This is a formative record for the r2 snapshot, not final approval. A later r3 freeze must be reviewed with fresh hashes and fresh runs.

## Snapshot

Source files read:

- `chapters/06-exception-safety-and-transactions.md`
  - SHA256: `FCB975ACB1FEC036A3BD09082886DA036C04B6F8CE353BADF4BB114265902627`
- `exercises/L06_transactions/README.md`
  - SHA256: `C331DF1F30392D330D1A6FE1602BFF4D34834713EDB0559F9C7FB08B8669F191`
- `exercises/L06_transactions/CMakeLists.txt`
  - SHA256: `5CC053A623FDFE267959F9E46FEAA42F4BF27FA9E51F8F8FCB5447687E841E9C`
- `exercises/L06_transactions/checks/tracked_value.hpp`
  - SHA256: `2CB36E52B8FD98FBCC543EBD85289341CA9FC9754712458A62B860BF2860D036`
- `exercises/L06_transactions/checks/transaction_checks.cpp`
  - SHA256: `4511A0EA48F8F365D501AFBB6608FDD2147E0D9FEC5AA5701DC3E7D7836998BC`
- `exercises/L06_transactions/src/student/table.hpp`
  - SHA256: `ED5C362569F7234FBADAE7096B71DF7FC188C8D0ED9AA59803F6681D7D946084`
- `exercises/L06_transactions/src/reference/table.hpp`
  - SHA256: `4912003BDBD36AFB2EB661779926EE44F4FFF3E399AAD225647EEF805055DB6C`
- `exercises/L06_transactions/validation/good/table.hpp`
  - SHA256: `4912003BDBD36AFB2EB661779926EE44F4FFF3E399AAD225647EEF805055DB6C`
- `exercises/L06_transactions/validation/bad/table.hpp`
  - SHA256: `1629EF4F1ED250463C753FA55792F7C562DC2A7D38828E6C709CA8080FD64D11`
- `exercises/cmake/StudySetup.cmake`
  - SHA256: `9BFC7790C46B939DFD6B3C266F81E9D1E37D662528D3588A06275740494EF47D`
- `exercises/cmake/expect_failure.cmake`
  - SHA256: `39E4769E9C801F19643ABF99DE0063FC0B3D0EECB91919CF9F10A10071E969C4`

## Findings

### HIGH: `validation/good` is not independently evidenced

Files:

- `exercises/L06_transactions/src/reference/table.hpp`
- `exercises/L06_transactions/validation/good/table.hpp`

Issue: `validation/good/table.hpp` and `src/reference/table.hpp` have identical SHA256 (`4912003BDBD36AFB2EB661779926EE44F4FFF3E399AAD225647EEF805055DB6C`). This does not satisfy the r2 claim that good is an independent implementation, even though it no longer includes Reference directly.

Fix: make `validation/good/table.hpp` an independently written passing implementation, then bind a fresh hash/diff and rerun the checker.

### HIGH: checker does not prove full strong-prefix failure surface

Files:

- `exercises/L06_transactions/checks/transaction_checks.cpp:93`
- `exercises/L06_transactions/checks/transaction_checks.cpp:101`
- `exercises/L06_transactions/CMakeLists.txt:9`
- `exercises/L06_transactions/validation/bad/table.hpp:35`

Issue: `check_strong_prefix_failure()` covers only `throw_on_copy(2)`. For the current reference implementation, prefix strong first copies the old vector and then mutates the prepared copy. The existing checker does not itself cover source-copy failure after backup (`k=4,5`) or prove no throwing copy occurs at commit (`k=6` success probe). The configured bad diagnostic also targets only `strong prefix failure keeps original prefix`, and the current bad variant delegates strong prefix to basic rather than providing an independent bad-prefix commit/coverage case.

Fix: add prefix probes for the full relevant copy surface and an independent `bad_prefix` variant that would fail without a no-throw commit boundary. The r3 request for k1..5 plus bad_prefix is the right direction.

## Evidence Run

Fresh commands were executed through `C02_Objects_Lifetime_Ownership/exercises/tools/record_process.py`, with bounded timeouts.

Passing evidence:

- `sample-technical-r2-configure-core.json`
- `sample-technical-r2-build-core-debug.json`
- `sample-technical-r2-ctest-core-debug.json`
  - Debug ctest: 3/3 passed: reference, validation_good, validation_bad_rejected.
- `sample-technical-r2-configure-student-ref-off.json`
- `sample-technical-r2-build-student-ref-off-debug.json`
- `sample-technical-r2-run-student-placeholder-debug.json`
  - Student placeholder exited 1 with `check failed: basic prefix failure reports copy error`.
- `sample-technical-r2-configure-audit-student-ref-off.json`
- `sample-technical-r2-build-audit-student-trace-debug.json`
- `sample-technical-r2-audit-student-ref-off.json`
  - Student audit passed; one student target, no reference dependency found, actual include count 150.
- `sample-technical-r2-build-core-release.json`
- `sample-technical-r2-ctest-core-release.json`
  - Release ctest passed, proving `check()` still runs under Release for this sample.
- `sample-technical-r2-configure-asan.json`
- `sample-technical-r2-build-asan-relwithdebinfo.json`
  - ASan configure/build passed. ASan ctest was intentionally not run after the review scope moved to r3.

Reviewer adversarial evidence:

- `sample-technical-r2-adversarial.cpp`
- `sample-technical-r2-adversarial/CMakeLists.txt`
- `sample-technical-r2-configure-adversarial.json`
- `sample-technical-r2-build-adversarial-debug.json`
- `sample-technical-r2-run-reference-adversarial-debug.json`
- `sample-technical-r2-run-good-adversarial-debug.json`

The adversarial program checked:

- prefix source-copy failure after backup at copy index 4 and 5,
- prefix commit has no extra throwing copy by succeeding with `throw_on_copy(6)`,
- `Table::swap` is `noexcept`,
- self `replace_all_strong(view())` reports prepare failure and preserves the original table.

Both current reference and current good passed this adversarial program. This proves the current implementation behavior on those inputs, but it does not close the independent-good evidence issue or the missing checker/bad-prefix coverage issue.

## Stop Reason

Review stopped as formative r2 evidence after receiving the instruction that teaching r2 is already ITERATE and files are moving toward r3. No approval should inherit from this r2 record.
