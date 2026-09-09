# C02 verifier report — public student-wiring audit + record_process cwd

## Verdict

APPROVE for this slice.

Scope checked: `Core_Study/exercises/tools/audit_student.py`, `Core_Study/references/validation/student-wiring/**`, `record_process.py` `cwd` field, and current public check include-order helper binding. This is limited to declared Reference-path / Student-target wiring checks; it is not a sandbox against arbitrary malicious C++ or plagiarism.

## Source binding

Fresh source hashes are recorded in `c02-student-wiring-verifier-evidence-r1/source-hashes.json`.

Key hashes:

- `Core_Study/exercises/tools/audit_student.py`: `0FA0EAAD51F0F47C277DB6EC8B2A5823950F512EAFAABE36841AB1BA56EA98A0`
- `Core_Study/exercises/tools/record_process.py`: `A405AF5089779E762A804E5135AA0FAA583773A3892696CB46E9D3A9DC927277`
- `Core_Study/exercises/cmake/StudySetup.cmake`: `31184FF231653E03FDA46D257E482A147CD01DFEDF05D28CBC0FD3CD1D37F641`
- `Engineering_Study/exercises/include/check.hpp`: `716138E42081AEE359E17929C31EE156ECFFD1D3B596E35107CFB08FF16C5F38`
- `Core_Study/references/validation/student-wiring/check_wiring.ps1`: `051E2094322817376C1C593FE5815816C1A55CAA5822841F673DB0CAE9B13F0D`

## Evidence

- `./Core_Study/references/validation/student-wiring/check_wiring.ps1 -RunName verifier-r1` — PASS. Good case built with `cmake --build ... --config Release --clean-first --target wiring_student`, audit verdict PASS, `students=[wiring_student]`, `actual_include_count=10`. Bad case also built successfully, then audit verdict FAIL with three relevant diagnostics: actual include trace reached `reference/answer.hpp`, source spelling included `../../reference/answer.hpp`, and codemodel source/include resolved the reference header.
- `Core_Study/references/validation/student-wiring/verifier-r1/good-build.json` — `cwd` present as `F:\CPPTrain\LearnCPP`, exit 0, timeout false, command includes `--clean-first --target wiring_student`, MSVC Chinese include prefix present in stdout.
- `Core_Study/references/validation/student-wiring/verifier-r1/bad-audit.json` — FAIL as expected with `Reference in actual include trace`, `Reference include spelling`, and `Reference source/include`; this proves exit-0 bad code is rejected by the audit, not accepted as a build pass.
- `c02-student-wiring-verifier-evidence-r1/codemodel-summary.json` — Release codemodel contains `wiring_student` executable; good binds `src/student/main.cpp`, bad binds `src/student/bad.cpp`; link fragments are normal system link flags and do not include Reference target fragments. Generated `.vcxproj` has `ShowIncludes>true</ShowIncludes>` and no added reference include directory.
- `c02-student-wiring-verifier-evidence-r1/wrong-config-audit-result.json` — direct negative proof: Release trace audited as Debug returns FAIL with `include-trace configuration does not match the codemodel configuration`.
- `c02-student-wiring-verifier-evidence-r1/no-include-trace-audit-result.json` — direct negative proof: fabricated successful build trace with empty stdout/stderr returns FAIL with `no actual preprocessor include trace; a no-op build is insufficient`.
- `c02-student-wiring-verifier-evidence-r1/no-student-audit-result.json` — direct negative proof: verifier-only CMake project with only `plain_target` returns FAIL including `no Student targets present` and `required Student target absent: wiring_student`.
- `c02-student-wiring-verifier-evidence-r1/missing-trace-audit-corrected.json` — direct negative proof: nonexistent trace file returns exit 1 and prints `audit input error` / `No such file`; missing trace is not accepted.
- `Core_Study/references/validation/tools/check_record_process.py --output c02-student-wiring-verifier-evidence-r1/record-process-self-check-cwd-r1.json` — PASS: 6 recorder contracts. Extracted cases show `cwd=F:\CPPTrain\LearnCPP` on success, expected-failure, wrong-marker, unicode-failure, and timeout records. Timeout record has `timeout=true`, `exit_code=1`, `verdict=FAIL`, so timeout is still not a pass.
- `c02-student-wiring-verifier-evidence-r1/source-rule-lines.txt` — source inspection confirms audit covers codemodel student target discovery, expected target absence, dependencies, link `commandFragments`, trace `cwd` build matching, `--clean-first --target`, config matching, MSVC English/Chinese include prefixes, and Clang/GCC `-H` parsing. Student-wiring CMake enables `/showIncludes /utf-8` for MSVC and `-H` otherwise.

## Gaps

- Clang/GCC `-H` support was source-inspected and CMake-inspected, not live-run in this verifier pass. Live environment evidence here is MSVC with Chinese `/showIncludes` output.
- The intentionally failed `missing-trace-audit.json` wrapper record is preserved: my first wrapper expectation used exit 2, but audit correctly returned exit 1. The corrected evidence is `missing-trace-audit-corrected.json`.

## Risks

- This checker proves declared wiring constraints for this course slice: no Reference path/target/source/include leakage through codemodel, literal includes, or actual include trace. It does not attempt to detect arbitrary malicious C++ behavior, macro attacks against checker internals, or copied solution algorithms.
