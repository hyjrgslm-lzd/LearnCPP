# Author A L01/L02 validation r2

Scope:

- `chapters/00-route-and-boundaries.md`
- `chapters/01-class-invariants.md`
- `chapters/02-value-semantics.md`
- `exercises/L01_invariants`
- `exercises/L02_value_semantics`

Content changes:

- Expanded 00 with public interface boundaries, construction/factory failure phases, and value/identity/resource/representation terms.
- Expanded 01 with invariant definition, public-entry protection, invalid public setter counterexample, construction failure, and factory boundary.
- Expanded 02 with value versus identity, regular-like expectations, equality/order/hash consistency, semantic laws, and concrete bad comparisons.
- L01 now checks inverted bounds and has independent validation good implementation.
- L02 now checks equality, ordering, and `hash_key()` consistency; validation good is independent from Reference.

Core Debug/Release:

```text
L01 Debug: 100% tests passed, 0 tests failed out of 3
L01 Release: 100% tests passed, 0 tests failed out of 3
L02 Debug: 100% tests passed, 0 tests failed out of 3
L02 Release: 100% tests passed, 0 tests failed out of 3
```

Student-on Debug/Release:

```text
L01 Debug: student failed with "check failed: constructor rejects values outside range"; reference/good/bad_rejected passed
L01 Release: student failed with "check failed: constructor rejects values outside range"; reference/good/bad_rejected passed
L02 Debug: student failed with "check failed: different currencies cannot be added"; reference/good/bad_rejected passed
L02 Release: student failed with "check failed: different currencies cannot be added"; reference/good/bad_rejected passed
```

Raw logs:

- `l01-configure-core.txt`
- `l01-build-core-debug.txt`
- `l01-build-core-release.txt`
- `l01-ctest-core-debug.txt`
- `l01-ctest-core-release.txt`
- `l01-configure-student.txt`
- `l01-build-student-debug.txt`
- `l01-build-student-release.txt`
- `l01-ctest-student-debug.txt`
- `l01-ctest-student-release.txt`
- `l02-configure-core.txt`
- `l02-build-core-debug-r2.txt`
- `l02-build-core-release-r2.txt`
- `l02-ctest-core-debug-r2.txt`
- `l02-ctest-core-release-r2.txt`
- `l02-configure-student.txt`
- `l02-build-student-debug-r2.txt`
- `l02-build-student-release-r2.txt`
- `l02-ctest-student-debug-r2.txt`
- `l02-ctest-student-release-r2.txt`
- `source-sha256-r2.txt`

Additional checks:

```text
rg validation include Reference/Student: no matches
git diff --check scoped author A files: clean
```

Environment:

```text
Visual Studio 18 2026 generator, x64
MSVC 19.51.36256.0
```

