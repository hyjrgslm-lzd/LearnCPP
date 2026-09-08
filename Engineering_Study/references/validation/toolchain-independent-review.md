# C01 F-I toolchain independent review

Verdict: APPROVE for code/chapters/check pipeline at the current frozen snapshot.

Reviewer: non-author independent review lane (`/root/coroutine_independent_review`)
Snapshot: 2026-09-08 F-I snapshot after G1 safe Student starter repair
Git HEAD: `c65edb3c2b5c76f386e53fa0afce87c54eb0c363`
Fingerprint manifest: `Engineering_Study/references/validation/toolchain/sources-sha256.txt`; independently checked current entries against workspace files after the G1 safety repair, all matched.

Important boundary: the six F-I exercise README files are under a separate non-author teaching/documentation pass. This report does not grant final approval for those changing README drafts. It approves the G1/G2/H1 code, chapters, public helpers, and validation mechanics reviewed here, while retaining previous unchanged F1/F2/I1 technical approvals.

## Required scope read

- `LEARNCPP_GLOBAL_PLAN.md`
- `CONTENT_REFACTORING_GUIDE.md`
- `Engineering_Study/references/implementation-spec.md`
- `Engineering_Study/references/validation/odr-independent-review.md`

Current targeted rereview:

- `Engineering_Study/exercises/G1_diagnostics/src/student/parser.cpp`
- `Engineering_Study/exercises/G1_diagnostics/checks/student_check.cpp`
- `Engineering_Study/exercises/G1_diagnostics/scripts/verify_fuzz.py`
- `Engineering_Study/exercises/G1_diagnostics/fuzz_parse.cpp`
- `Engineering_Study/exercises/include/check.hpp`
- current toolchain/shared-tools validation summaries and raw evidence

Previous same-snapshot review retained for unchanged areas:

- G2 `measure_build.py`, public `process_runner.py`, fresh 3-variant CTest, and subset driver smoke
- H1 module visibility/fragment/package consumer positive and negative boundaries
- F1/F2/I1 technical closure from earlier independent pass

## Closed findings

### HIGH - closed: default G1 Student starter previously had UB on short input

Current state: closed.

Evidence from current source:

- `Engineering_Study/exercises/G1_diagnostics/src/student/parser.cpp:3-7` now ignores the `std::string_view` input and returns `-1` with a TODO comment. It does not read `text[0]`, `text[1]`, or `.at()`.
- Static grep over `src/student/parser.cpp` and `checks/student_check.cpp` found no direct `text[0]`, `text[1]`, or `.at()` access in the starter path.
- `student_check.cpp:7` fails on the first valid-input check, before the short-input call at line 10 is reached. This gives a safe, clear failure for unfinished student code without exercising invalid indexing.

Independent Release run:

```text
cmake -S Engineering_Study/exercises/G1_diagnostics -B Engineering_Study/exercises/build/toolchain-independent-review/g1-safety-release -G Ninja ... -DCMAKE_BUILD_TYPE=Release -DENGINEERING_STUDY_TEST_STUDENTS=ON
cmake --build .../g1-safety-release --target G1_diagnostics_reference G1_diagnostics_student
ctest --test-dir .../g1-safety-release -C Release -R G1_diagnostics_reference --output-on-failure
ctest --test-dir .../g1-safety-release -C Release -R G1_diagnostics_student --output-on-failure
```

Result:

- build exit 0
- Reference CTest exit 0, passed in 0.28s
- Student CTest exit 8, failed in 0.25s with `check failed: student parser must parse valid input`
- no timeout, no access violation, no uncaught-exception dialog pattern

Independent Debug run:

```text
cmake -S Engineering_Study/exercises/G1_diagnostics -B Engineering_Study/exercises/build/toolchain-independent-review/g1-safety-debug -G Ninja ... -DCMAKE_BUILD_TYPE=Debug -DENGINEERING_STUDY_TEST_STUDENTS=ON
cmake --build .../g1-safety-debug --target G1_diagnostics_reference G1_diagnostics_student
ctest --test-dir .../g1-safety-debug -C Debug -R G1_diagnostics_reference --output-on-failure
ctest --test-dir .../g1-safety-debug -C Debug -R G1_diagnostics_student --output-on-failure
```

Result:

- build exit 0
- Reference CTest exit 0, passed in 0.26s
- Student CTest exit 8, failed in 0.24s with `check failed: student parser must parse valid input`
- no timeout, no access violation, no uncaught-exception dialog pattern

### HIGH - closed: G1 fuzz must reject bad parser by oracle marker, not arbitrary process failure

Current state: still closed after G1 Student safety repair.

Independent rerun after the Student change:

```text
verify_fuzz.py --output Engineering_Study/exercises/build/toolchain-independent-review/g1-fuzz-safety-r3 --clang D:\VisualStudio2026\Installed\VC\Tools\Llvm\x64\bin\clang++.exe --timeout 60
```

Result: PASS.

Evidence:

- real parser compile/run passed; real run stderr contained `Done 16 runs`.
- bad parser compile passed.
- bad parser run exited nonzero with `exit_code=3221225477`, `timeout=false`, `cleanup_error=""`, `error=""`.
- bad parser stderr contained `G1_ORACLE_MISMATCH:invalid-accepted`.

This verifies the bad parser is rejected by the harness oracle contract, not by missing DLL, timeout, cleanup failure, or unrelated launch error.

### HIGH - closed: G2 fresh behavior and driver closure

Retained from previous independent pass; not rerun in this G1-only pass because G2 did not change.

Evidence previously collected in `Engineering_Study/exercises/build/toolchain-independent-review/`:

- `measure_build.py --self-test`: PASS; existing output rejection, nonzero command classification, timeout classification.
- fresh G2 leaf configure/build/CTest: baseline/PCH/LTO all passed, `100% tests passed, 0 tests failed out of 3`.
- subset driver `baseline/noop`, 1 warmup + 1 sample: PASS, report summary length exactly 1, runner hash `77f7aa8a86ff5fa4fdb933c8bb9929d637748a1aa00fb7afb3dac941e47d936b`.

Formal 1+5 performance sampling remains outside this report and should be run by root after all builds are quiet.

### MEDIUM - closed: H1 visibility/fragment/package boundaries

Retained from previous independent pass; not rerun in this G1-only pass because H1 code/chapters did not change.

Evidence previously collected:

- H1 module reference, visibility boundary, global/private fragment, and package consumer passed 4/4.
- `H1_hidden_type_negative` failed at compile stage with MSVC `C2065` for direct `hidden_state` naming.
- Chapter 08 correctly distinguishes hidden type name visibility from definition reachability through `auto`, member access, and exported functions.

## Public helper status

- `Engineering_Study/exercises/include/check.hpp:8-12` prints `check failed: ...` to stderr and exits with `EXIT_FAILURE`.
- The Debug/Release G1 Student reruns above directly exercise this behavior in the current starter path.
- Shared-tools evidence still records Debug/Release public check self-tests and public `process_runner.py` normal/nonzero/timeout behavior.

## Documentation boundary

`Engineering_Study/exercises/G1_diagnostics/README.md` no longer points users to directly rerun the old fixed-output `568` script. It now shows the reusable `verify_fuzz.py --output $out` shape and points to validation summary for historical records. However, root stated the F-I README teaching pass is still changing and will receive separate final documentation review, so this report does not approve final README wording or Part/解析 completeness.

## Not verified in this G1-only pass

- No full G2/H1/I1 rerun; previous approvals retained because code/chapters in those areas were not part of the final G1 safety patch.
- No ASan/Linux/macOS rerun.
- No full root aggregate C01 matrix.
- No formal G2 1 warmup + 5 sample performance collection.
- No final exercise README teaching review.

## Recommendation

APPROVE for the current F-I technical/code/check closure within the stated boundary. The root-discovered G1 default Student UB issue is closed by the safe `-1` starter and fresh Debug/Release evidence.
