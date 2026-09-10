# C04 values blind review notes

Scope: A01 blind solvability from `chapters/18-compiletime-values.md`,
`exercises/A01_compiletime_values/README.md`, original A01 checks, and
student initial header only. A03 files were read only to establish the later
review surface. Reference, validation/good, validation/bad, and author review
were not read before `03-a01-run.json` passed.

## Input SHA256

| File | SHA256 |
| --- | --- |
| `chapters/18-compiletime-values.md` | `6E497B597B5DBA5946A6F400C007D5A5EE71C5EEB04C5FD30495A755E8BDEC93` |
| `chapters/20-explicit-object-forwarding.md` | `7E11E2535555E106091E0BB0909BF2DAA5B7ABB12612904434A22863BC6B0563` |
| `exercises/A01_compiletime_values/README.md` | `AE634EDC373717D105D90BA4F0A3E8C9969777C20FAFC04AF8E86DB65786838F` |
| `exercises/A01_compiletime_values/checks/compiletime_values_checks.cpp` | `CF7AADC38ED6EC7078F16796721A2966DB4D2C4E0C60CFA572BB5B60BB95A1DB` |
| `exercises/A01_compiletime_values/src/student/compiletime_values.hpp` | `F1C572DF703CA170B2462D7902A7A510E5959255A9795E5A6A1FFB0A3FC1A15D` |
| `exercises/A03_explicit_object/README.md` | `2801489D2DEDF500BD758F8D1CA4354BCD817BCD613AA8A53E0BF7CF90CDF4A3` |
| `exercises/A03_explicit_object/checks/explicit_object_checks.cpp` | `9CB74D1861EB3F0FE150ABFD858B524B9E3F106AF0F01F9B869BE66999F2176C` |
| `exercises/A03_explicit_object/src/student/explicit_object.hpp` | `221FC3115F552A4E1AFAFD59F9189C5A80F714188EFEF9E8D7090AD8A4A7062D` |

## Blind A01 steps

1. Read A01 chapter and exercise statement.
2. Read original checker and student initial header.
3. Implement independent `compiletime_values.hpp` in this directory:
   fixed-width literal row, stable insertion sort, first-wins dedup count,
   `table<N>` construction from `template<auto Raw>`, same-size normalization,
   and binary `find`.
4. Build with the original A01 checker through a local wrapper CMake project.
5. Preserve all command evidence with C02 `record_process.py`.

## Evidence

| Record | Result | Meaning |
| --- | --- | --- |
| `01-a01-configure.json` | PASS | CMake generated `values_blind_a01`. |
| `02-a01-build.json` | PASS | Debug build produced `values_blind_a01.exe`. |
| `03-a01-run.json` | PASS | Original checker printed `A01 compile-time values contract passed`. |
| `04-input-sha.json` | FAIL | Windows PowerShell in the recorder could not resolve `Get-FileHash`; raw failure intentionally kept. |
| `05-input-sha-pwsh.json` | PASS | PowerShell 7 recorded input hashes. |

## Rules I had to supply as reviewer

- I used unsigned-char comparison for key ordering. The chapter says ASCII key
  boundary, but it does not explicitly say signed `char` ordering must be
  avoided; this is minor because ASCII bytes below 0x80 compare the same.
- I enforced the README's "max 15 ASCII characters" as `N <= key_width` because
  `N` includes the trailing null. The chapter sample says `N <= key_width + 1`,
  which permits a 16-character key in 16 storage bytes and conflicts with the
  surrounding prose.
- I added an ASCII rejection path in `make_row`; the README states ASCII but the
  visible checker does not prove non-ASCII rejection.
