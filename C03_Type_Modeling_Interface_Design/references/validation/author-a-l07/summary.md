# Author A L07 validation

Scope:

- `chapters/07-interfaces-and-polymorphism.md`
- `exercises/L07_interfaces`
- `references/validation/author-a-l07`

Content:

- Chapter 07 now covers parameter choice, return-value choice, value snapshot versus borrowed view, `const` and ref-qualified members, `noexcept` versus exception guarantees, and minimal public interfaces.
- The chapter uses L06 `view()` invalidation as the bridge into lvalue borrowing and value snapshot design.
- L07 avoids 08+ polymorphism scope and does not run real dangling-reference programs.

Exercise:

- `l07::Owner` exposes `view() const& noexcept`, deleted `view() &&`, `snapshot() const`, and `replace_all(std::vector<int>) noexcept`.
- Reference and validation good are independent implementations.
- Student is a buildable operation placeholder and fails through checker diagnostics.
- Validation bad is rejected for returning a non-independent snapshot.
- `checks/rvalue_view_negative.cpp` is a compile negative case proving temporary owners cannot lend a view.

Core Debug/Release:

```text
Debug: 100% tests passed, 0 tests failed out of 4
Release: 100% tests passed, 0 tests failed out of 4
```

Student-on Debug/Release:

```text
Debug: L07_interfaces_student failed with "check failed: snapshot remains independent after owner mutation"; other 4 tests passed
Release: L07_interfaces_student failed with "check failed: snapshot remains independent after owner mutation"; other 4 tests passed
```

Raw tracked evidence:

- `configure-core.txt`
- `build-core-debug.txt`
- `build-core-release.txt`
- `ctest-core-debug.txt`
- `ctest-core-release.txt`
- `configure-student.txt`
- `build-student-debug.txt`
- `build-student-release.txt`
- `ctest-student-debug.txt`
- `ctest-student-release.txt`
- `source-sha256.txt`
- `evidence-sha256.txt`

Additional checks:

```text
rg validation include Reference/Student: no matches
git diff --check scoped L07 files: clean
```

Environment:

```text
Visual Studio 18 2026 generator, x64
MSVC 19.51.36256.0
```

