# Foundations teaching review r2 approval

Verdict: APPROVE

Reviewer: original non-author critic
Scope: L07-only closure review for the prior foundations blocker. Chapters 00/01/02 and L01/L02 were not rerun because the r2 task states they were unchanged.
Git HEAD observed: `8e0f407afd155ede81f0b06a51553dadef206a23`
Workspace note: `C03_Type_Modeling_Interface_Design/` and root `references/` are untracked at review time, so this approval binds current file-content hashes and fresh command results.

## Evidence read

Author r2 evidence was found under root `references/validation/author-a-l07/`:

- `summary-r2.md`: `B49B18ADF1143C07269F66083C473182963EBA27E3958BC1BB13646F3ABF5FA7`
- `source-sha256-r2.txt`: `386FA3CBFACF8B1DA290CB62C94F6574A201731AD272B8C4906E489E7076D118`

L07 source snapshot:

- `exercises/L07_interfaces/CMakeLists.txt`: `D1BBDD205E0C77C3E89B3EF6BA1E46BD286B054E4D3BEAC0C3B2B53C98A15138`
- `exercises/L07_interfaces/checks/interface_checks.cpp`: `6A4DF76E235DE0D0AD123B5E70744AB77A35EF6DD379389C164004F060210A6D`
- `exercises/L07_interfaces/checks/rvalue_view_negative.cpp`: `BE471F5649A82B65FACE28CB8C4C941333F069888AE83CA125EEF3A175C807DC`
- `exercises/L07_interfaces/src/reference/owner.hpp`: `44A7F752FF2C2974D66B73C6752B5E1D46268F9D8F84E9EB4D50180E399B442F`
- `exercises/L07_interfaces/src/student/owner.hpp`: `AE46C7A0835220ECA12C0C6A7D5F3475C646A832BFE58E3CC302AEEC585520B9`
- `exercises/L07_interfaces/validation/good/owner.hpp`: `7DA9DAC20A86220193047948E7D03479E5D38FFD7744993B4AE119D8A9F698D9`
- `exercises/L07_interfaces/validation/bad/owner.hpp`: `6D70AA9844E1D654B3EA81C461CD6C1F62218DE9FD71D1102C32EE851F7C179B`
- `exercises/L07_interfaces/validation/bad_rvalue/owner.hpp`: `25BCF58C78C609E75F9669CD7F8A1DC2D5F33051B4DB99BB46372D7855991AAE`

## Prior blocker closure

The r1 blocker was that temporary-object view rejection was only compiled against Reference. It is closed in r2:

- `CMakeLists.txt:27-38` adds `L07_interfaces_validation_bad_rvalue_rejected`, using `validation/bad_rvalue` with the normal `checks/interface_checks.cpp` checker.
- `checks/interface_checks.cpp:12-15` defines a `requires`-based `rvalue_view_available` probe against the selected `Owner`.
- `checks/interface_checks.cpp:30` rejects selected implementations where rvalue `.view()` is available, with diagnostic `rvalue view is rejected for selected implementation`.
- `validation/bad_rvalue/owner.hpp:14` intentionally exposes `view() const noexcept` without `const&`/deleted rvalue overload, while `snapshot()` and `replace_all()` are otherwise correct. This isolates the rvalue-borrowing defect.
- `checks/rvalue_view_negative.cpp:5` and `CMakeLists.txt:12-25` remain as a Reference-only compiler diagnostic comparison, and author r2 summary no longer treats it as Student/good proof.

## Independent validation

Build caches used:

- `build/c03-critic-foundations-l07-r2-core`
- `build/c03-critic-foundations-l07-r2-student`

Fresh commands/results:

- Configure L07 core Debug with Visual Studio 18 2026: exit 0.
- Build L07 core Debug: exit 0.
- `ctest --test-dir build/c03-critic-foundations-l07-r2-core -C Debug --output-on-failure`: 5/5 PASS.
  - Includes Reference, validation_good, original bad rejection, Reference rvalue compile-negative comparison, and new bad_rvalue rejection.
- Direct run of `build/c03-critic-foundations-l07-r2-core/Debug/L07_interfaces_validation_bad_rvalue.exe`: exit 1 with `check failed: rvalue view is rejected for selected implementation`.
- Configure L07 student-on Debug with `-DTYPE_STUDY_TEST_STUDENTS=ON`: exit 0.
- Build L07 student-on Debug: exit 0.
- `ctest --test-dir build/c03-critic-foundations-l07-r2-student -C Debug --output-on-failure`: expected nonzero exit; 5/6 PASS, only `L07_interfaces_student` failed with `check failed: snapshot remains independent after owner mutation`.

## Verdict

APPROVE for the Author A foundations slice covered by the prior r1 review plus this L07 r2 closure. The approval remains scoped to 00/01/02/07 and L01/L02/L07; it does not approve 03/04/05 or the whole C03 course.
