# A02 field projection blind solve notes

Captured from the pre-revision sample at git HEAD `f261bea559d6722c31135fb2d3589be52fe958ed`.

## Inputs read before solving

- `CONTENT_REFACTORING_GUIDE.md`
- `LEARNCPP_GLOBAL_PLAN.md`
- `C04_Generic_CompileTime_Reflection/chapters/19-field-projection-dsl.md`
- `C04_Generic_CompileTime_Reflection/exercises/A02_field_projection/README.md`
- `C04_Generic_CompileTime_Reflection/exercises/A02_field_projection/checks/projection_schema.hpp`
- `C04_Generic_CompileTime_Reflection/exercises/A02_field_projection/checks/projection_checks.hpp`
- `C04_Generic_CompileTime_Reflection/exercises/A02_field_projection/checks/field_projection_checks.cpp`
- `C04_Generic_CompileTime_Reflection/exercises/A02_field_projection/src/student/field_projection.hpp`

No `src/reference`, `validation/good`, `validation/bad`, or author review notes were read before this point.

## Rules directly available from the sample

- `get<"name">(object)` maps a compile-time field name to a schema descriptor and returns the real member expression.
- `get` preserves cv/ref and can consume an rvalue member expression.
- `project<"a,b">(object)` parses comma-separated ASCII identifiers, preserves the DSL order, and returns a tuple of member references.
- `project` allows an empty spec as `std::tuple<>`.
- `project` rejects temporaries, including const rvalues, because its tuple stores borrowed member references. The temporary lifetime ends at the end of the full expression, so returning these references would dangle after the call expression completes.
- Unknown fields, duplicate fields, invalid identifiers, leading/trailing commas, empty components, and embedded NUL bytes must be compile-time errors.

## Rules I had to supply from existing C++ knowledge

- The concrete implementation shape: recursive `std::tuple_element_t` lookup over `schema<T>::fields`, a `consteval` parser for component spans, and `std::forward_as_tuple` to preserve references.
- The exact way to make `project` reject every rvalue category while accepting const lvalues: use a forwarding reference constrained by `std::is_lvalue_reference_v<T&&>`.
- The detail that `std::tuple{object.member...}` would copy fields, so the implementation must avoid class template argument deduction for projected references.

## Teaching gap observed in the pre-revision text

The text states the required behavior, but it does not yet give a continuous path from input DSL to generated result type. A student can infer the destination contract from the checker, but must fill in the parser shape, descriptor lookup, pack expansion, tuple-reference construction, and rvalue rejection mechanics from prior experience.

Current gate decision for the pre-revision sample: `ITERATE`.

## Blind validation

Configure:

```powershell
cmake -S C04_Generic_CompileTime_Reflection\references\validation\revision-20260910\sample-blind -B C04_Generic_CompileTime_Reflection\references\validation\revision-20260910\sample-blind\build -G "Visual Studio 18 2026" -A x64
```

Result: exit code 0, configured with MSVC 19.51.36256.0.

Build:

```powershell
cmake --build C04_Generic_CompileTime_Reflection\references\validation\revision-20260910\sample-blind\build --config Debug
```

Result: exit code 0, produced `sample_blind_field_projection.exe`.

Run:

```powershell
C04_Generic_CompileTime_Reflection\references\validation\revision-20260910\sample-blind\build\Debug\sample_blind_field_projection.exe
```

Result: exit code 0, output `A02 field projection contract passed`.

Student initial-state check:

```powershell
C04_Generic_CompileTime_Reflection\references\validation\revision-20260910\sample-blind\build\Debug\sample_blind_student_projection.exe
```

Result: the Student target built successfully, then exited with code 1 at runtime with `check failed: project returns writable references in requested order`.
