# C04 foundations author handoff

Date: 2026-09-09

Scope owned in this pass:

- `chapters/01-template-model.md`
- `chapters/02-deduction.md`
- `chapters/03-forwarding-ctad.md`
- `exercises/L01_templates/**`
- `exercises/L02_deduction/**`
- `exercises/L03_forwarding/**`

Implemented:

- L01 teaches and checks function/class/variable/alias templates, explicit specialization, partial specialization, lazy instantiation, same-unit constraints, and a short ODR/extern-template bridge.
- L02 teaches and checks value/reference/cv deduction, array and function decay, array-reference extent preservation, non-deduced context with `std::type_identity_t`, and `decltype(auto)` identity.
- L03 teaches and checks reference collapsing, forwarding references versus ordinary `T&&`, `std::move` versus `std::forward`, return reference preservation, `noexcept` propagation, CTAD, and initializer_list boundaries.
- Student implementations are intentionally incomplete but structurally compilable by design; Reference and validation/good are independent correct implementations; validation/bad targets the registered checker diagnostic.

Non-build verification run in this pass:

- `git diff --check -- C04_Generic_CompileTime_Reflection\chapters\01-template-model.md C04_Generic_CompileTime_Reflection\chapters\02-deduction.md C04_Generic_CompileTime_Reflection\chapters\03-forwarding-ctad.md C04_Generic_CompileTime_Reflection\exercises\L01_templates C04_Generic_CompileTime_Reflection\exercises\L02_deduction C04_Generic_CompileTime_Reflection\exercises\L03_forwarding` passed with no output.
- `rg -n "C04_|assert\(|#include <.*reference|SKIP|完成标记|TODO|FIXME" ...` only matched intended `static_assert` lines in `L03_forwarding/observations/counterexamples.cpp`.
- File inventory confirms 3 chapter files and 24 lesson files under L01-L03.

Not run:

- No `cmake`, `cmake --build`, or `ctest` command was started because the build window is not open yet.

Recommended first build-window checks:

```powershell
python C02_Objects_Lifetime_Ownership/exercises/tools/record_process.py --label c04-l01-configure --timeout 180 -- cmake -S C04_Generic_CompileTime_Reflection/exercises/L01_templates -B C04_Generic_CompileTime_Reflection/exercises/L01_templates/build/foundations-author-debug -G "Visual Studio 18 2026" -A x64
python C02_Objects_Lifetime_Ownership/exercises/tools/record_process.py --label c04-l01-build --timeout 900 -- cmake --build C04_Generic_CompileTime_Reflection/exercises/L01_templates/build/foundations-author-debug --config Debug
python C02_Objects_Lifetime_Ownership/exercises/tools/record_process.py --label c04-l01-ctest --timeout 420 -- ctest --test-dir C04_Generic_CompileTime_Reflection/exercises/L01_templates/build/foundations-author-debug -C Debug --output-on-failure
```

Repeat the same configure/build/ctest pattern for `L02_deduction` and `L03_forwarding`, then run Student-only with `-DGENERIC_STUDY_BUILD_REFERENCE=OFF -DGENERIC_STUDY_TEST_STUDENTS=ON` and confirm the intended failing diagnostics.
