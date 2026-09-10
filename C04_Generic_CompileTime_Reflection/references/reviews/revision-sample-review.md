# C04 A02 field projection sample blind review

Verdict: `ITERATE`

Review scope: pre-revision sample frozen at git HEAD `f261bea559d6722c31135fb2d3589be52fe958ed`. Blind input hashes are recorded in `references/validation/revision-20260910/sample-blind/source-inputs.json`.

I did not read `src/reference`, `validation/good`, `validation/bad`, or author notes before freezing the blind solve. After the freeze, the chapter file changed in the working tree; this report therefore only gates the frozen pre-revision sample and must not be mixed with later author edits.

## Result

The exercise contract is technically implementable from the stated API and checks, and my independent implementation passes the existing positive checker:

- Configure: exit code 0, MSVC 19.51.36256.0.
- Build: exit code 0, produced `sample_blind_field_projection.exe`.
- Run: exit code 0, output `A02 field projection contract passed`.
- Student initial target: build succeeded, then runtime check exited code 1 with `check failed: project returns writable references in requested order`.

Evidence: `references/validation/revision-20260910/sample-blind/blind-solve-notes.md:39`.

## Blocking Issue

[MEDIUM] `C04_Generic_CompileTime_Reflection/references/validation/revision-20260910/sample-blind/blind-solve-notes.md:27`

Issue: The pre-revision chapter/README state the endpoint behavior, but a blind student still has to supply the implementation bridge from existing C++ knowledge: parser representation, descriptor lookup over `schema<T>::fields`, pack expansion from parsed names, tuple reference construction, and exact rvalue rejection. Passing behavior is not enough for the sample gate because the task is to validate teaching transfer, not only whether an experienced reviewer can complete the code.

Fix: Add a continuous worked path in the chapter or exercise notes: `fixed_string` input -> component validation -> type-level/name-list representation or equivalent constexpr span model -> schema descriptor lookup -> `get` expansion -> `std::forward_as_tuple` reference result -> lvalue-only `project` constraint. Keep Reference details separate, but teach enough mechanics that the Student can reproduce the solution without guessing.

## Student Baseline

The Student file is intentionally incomplete and should fail real contract checks:

- `src/student/field_projection.hpp:10` uses only `Name.value[0]` for `id`, so names like `"index"` would select `id` instead of being rejected.
- `src/student/field_projection.hpp:12` returns `name` for any remaining `Person` name, so unknown Person fields are silently accepted.
- `src/student/field_projection.hpp:22` and `src/student/field_projection.hpp:23` use `std::tuple{...}`, which copies fields instead of returning reference tuples.
- `src/student/field_projection.hpp:23` falls back to `id,name` for most Person specs, so DSL order and unknown/duplicate/invalid syntax are not enforced.

The independent harness proves this Student initial state compiles and then fails the checker at runtime. I did not run Reference/good/bad second-stage validation because the current gate already requires iteration and a new author revision is pending.

## Blind Implementation Evidence

The independent implementation in `references/validation/revision-20260910/sample-blind/field_projection.hpp` uses:

- `valid_spec` for empty spec, identifier rules, leading/trailing comma, empty component, and embedded NUL rejection.
- `no_duplicate_components` for duplicate field rejection.
- `field_index_for_component` over `schema<T>::fields` for unknown-field rejection.
- `std::forward_as_tuple` for tuple references.
- `requires std::is_lvalue_reference_v<T&&>` plus deleted rvalue overload for all rvalue `project` calls, including const rvalues.

No fallback/workaround branch was added to mask schema or DSL failures.

## Recommendation

`ITERATE`. Do not use this sample as the bulk-chapter quality gate yet. Re-run blind review on the revised chapter before reading Reference/good/bad or author notes.

---

# R2 Review Addendum

Verdict: `APPROVE`

R2 scope was re-frozen in `references/validation/revision-20260910/sample-blind/source-inputs-r2.json`. Validation results are recorded in `references/validation/revision-20260910/sample-blind/validation-r2.json`.

## Stage 1: Spec and Teaching Compliance

The previous teaching blocker is fixed. The chapter now gives the missing bridge from structured string input to projected member expressions:

- `chapters/19-field-projection-dsl.md:14` introduces structured `fixed_string` NTTP input.
- `chapters/19-field-projection-dsl.md:29` explains why length-aware parsing matters for embedded NUL.
- `chapters/19-field-projection-dsl.md:50` separates compile-time field/member descriptors from the runtime object.
- `chapters/19-field-projection-dsl.md:74` manually traces `project<"name,id">(person)` from slices to `std::tuple<std::string&, int&>`.
- `chapters/19-field-projection-dsl.md:93` states both `name_list` and constexpr index-array approaches and clarifies that `name_list` is not the only valid implementation.
- `chapters/19-field-projection-dsl.md:107` explains the temporary lifetime boundary as full-expression lifetime, not function-return lifetime.

The checker also covers the r2 contract additions:

- `checks/projection_checks.hpp:34` checks mutable and const rvalue `get` categories.
- `checks/projection_checks.hpp:45` checks const projection returns tuple references, not copies.
- `checks/projection_checks.hpp:61` rejects temporary `project` calls, including const rvalue expressions.
- `CMakeLists.txt:18` through `CMakeLists.txt:63` register independent diagnostics for unknown, duplicate, empty component, invalid identifier, rvalue project, trailing comma, and embedded NUL.

Student initial state remains a real failing baseline: it builds, then fails with `check failed: project returns writable references in requested order`.

## Stage 2: Code, Security, and Root-Cause Review

Files reviewed in r2: chapter, README, exercise CMake, `StudySetup.cmake`, `compile_case.py`, schema/checker files, Student, Reference, good, bad, diagnostics, author note, and blind evidence harness.

Root-cause fallback guard passed. Reference and good do not add silent defaults or alternate success paths; unknown and invalid DSL cases are rejected by compile-time checks. The `validation/bad` copy/schema-order implementation is correctly rejected by the runtime negative test.

No security issue found. This is local C++ teaching code with no credentials, network, SQL, shell interpolation from untrusted input, or external production effects.

### Issue

[LOW] Unknown-schema diagnostics contain follow-on template errors

File: `C04_Generic_CompileTime_Reflection/exercises/A02_field_projection/src/reference/field_projection.hpp:112` and `C04_Generic_CompileTime_Reflection/exercises/A02_field_projection/validation/good/field_projection.hpp:130`

Issue: `project<"id">(ExternalRecord&)` is rejected and emits the intended `unknown projection record` static assertion, but compilation then continues far enough to also emit incomplete `schema<ExternalRecord>` errors at `src/reference/field_projection.hpp:51` / `validation/good/field_projection.hpp:58`. This is diagnostic noise, not a contract escape.

Fix: If cleaner diagnostics matter, wrap the lookup/member body behind `if constexpr (has_schema<record>)` or constrain the lookup helper so `schema<Record>::fields` is not named after the unknown-record assertion fails.

## Validation

- Blind implementation r2 configure/build/run: pass, output `A02 field projection contract passed`.
- Formal A02 Debug build: pass under MSVC 19.51.36256.0.
- Formal CTest: 12/12 pass, including Reference, good, bad rejection, two observations, and seven diagnostic cases.
- Formal Student target: build pass, runtime fail at the intended checker message.
- Unknown-schema probes: Reference and good both reject with `unknown projection record`; extra follow-on diagnostics noted above.
- `compile_case.py` empty platform behavior was source-checked at `exercises/tools/compile_case.py:51`: empty `--platform=` does not pass `-A`. A live Ninja no-platform configure was attempted, but the local GNU ABI detection hung for over 60 seconds and was interrupted before reaching `compile_case.py`; this is not used as A02 pass/fail evidence.
- No `lsp_diagnostics` tool is available in this lane. Compiler diagnostics came from the project build path using MSVC `/W4 /permissive-`.

## Recommendation

`APPROVE` for this sample gate. It is acceptable to let later authors use this r2 sample as the quality baseline, while not treating the 12-test runtime total alone as teaching approval.
