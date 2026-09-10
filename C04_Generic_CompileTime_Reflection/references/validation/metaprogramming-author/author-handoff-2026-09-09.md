# C04 07-10 author handoff

Author scope: chapters 07-10, exercises L07_packs/L08_type_lists/L09_tuple/L10_constexpr, and this validation directory.

## Implemented

- `chapters/07-packs-nttp.md`: packs, four fold forms, empty identities, sequencing, `auto` NTTP, `fixed_string`, and template template parameters.
- `chapters/08-type-lists.md`: traits, type/value computation, lazy instantiation, `type_list` map/filter/concat/unique, completion signature transform bridge to C10.
- `chapters/09-tuple-traversal.md`: `std::get`, `std::apply`, structured bindings, `index_sequence`, cvref preservation, callback reuse, empty tuple, move-only and exception side effects.
- `chapters/10-constant-evaluation.md`: `constexpr`, `consteval`, `if consteval`, `is_constant_evaluated` boundary, optimizer folding distinction, storage and temporary allocation limits, vector/string persistence boundary, C++26 forward link.
- `exercises/L07_packs`: independent Student/Reference/good/bad, checker and observation.
- `exercises/L08_type_lists`: independent Student/Reference/good/bad, checker and observation.
- `exercises/L09_tuple`: independent Student/Reference/good/bad, checker and observation.
- `exercises/L10_constexpr`: independent Student/Reference/good/bad, checker and observation.

## Static checks run

- `git diff --check -- C04_Generic_CompileTime_Reflection`: PASS, no whitespace errors.
- File inventory for L07-L10 confirmed all expected CMake/README/checks/observations/src/reference/src/student/validation good/bad files exist.
- Targeted `rg` scan for known bad patterns (`static_assert((true`, raw empty fold snippets, bad relative reference include, target diagnostics) returned no matches.

## Not run yet

Per leader instruction, no configure/build/CTest was run in this author pass. Use the approved build window and external timeout wrapper before claiming compiled verification.

Suggested single-lesson verification when opened:

```powershell
cmake -S C04_Generic_CompileTime_Reflection/exercises/L07_packs -B C04_Generic_CompileTime_Reflection/exercises/build/L07-author -G "Visual Studio 18 2026" -A x64
cmake --build C04_Generic_CompileTime_Reflection/exercises/build/L07-author --config Debug
ctest --test-dir C04_Generic_CompileTime_Reflection/exercises/build/L07-author -C Debug --output-on-failure
```

Repeat for L08/L09/L10, then repeat with `-DGENERIC_STUDY_BUILD_REFERENCE=OFF -DGENERIC_STUDY_TEST_STUDENTS=ON` and expect Student checker failure rather than compile failure.
