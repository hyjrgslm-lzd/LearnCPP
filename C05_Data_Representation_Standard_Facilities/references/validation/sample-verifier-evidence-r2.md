# C05 sample verifier evidence r2

Verifier: non-author targeted recheck after L14 bad-state repair.
Date: 2026-09-09.
Scope: four changed L14 files plus affected L14 bad/leaf behavior. Prior r1 Unicode/bytes oracle evidence is inherited because `include/c05/{types,bytes,utf}.hpp` and other scoped sample sources were not changed in this repair.

## Changed file fingerprints

- `exercises/L14_binary_fields/validation/bad/bounded_field.hpp`: `20DA195F708113F967D2BB0013316038A96945928CB11164D3951DE940DEFE4F`
- `exercises/L14_binary_fields/CMakeLists.txt`: `070E8B15A52AAB0828649F6E7C98B2D7856687263E8687900579426B4A94E893`
- `exercises/L14_binary_fields/README.md`: `F004F1F48C50799995624E9E8A165B1B3E83EDC28FB742D098A870FA087FF219`
- `chapters/14-bounded-binary-fields.md`: `F39727CEF8B672F96365983E1700EFB65AE3F9BCE0E40252317D2D972FD3B6D1`

## Static checks

- `validation/bad/bounded_field.hpp` now includes `c05/utf.hpp`.
- The bad variant keeps length-header availability check before any cursor commit.
- It computes length and checks `len > c05::max_text_bytes` before the deliberate cursor commit.
- It deliberately commits `cursor = probe` after `probe += 4` and before payload availability check.
- It still keeps payload bounds check, copies only available payload bytes, and calls `c05::validate_utf8(text)` with global byte offset mapping.
- `CMakeLists.txt` now expects `BAD_DIAGNOSTIC "truncated payload keeps cursor"`.
- README Part 4 and chapter 14 now both describe the same single bad mechanism: early state commit after length header, before payload success.

## Fresh targeted commands

- `cmake -S L14_binary_fields -B build/review-sample-r2/leaf-L14 -G "Visual Studio 18 2026" -A x64 -DDATA_STUDY_BUILD_REFERENCE=ON -DDATA_STUDY_TEST_STUDENTS=OFF` exited 0; compiler MSVC 19.51.36256.0.
- `cmake --build build/review-sample-r2/leaf-L14 --config Release --parallel 1` exited 0 and built `L14_binary_fields_reference`, `L14_binary_fields_validation_bad`, `L14_binary_fields_validation_good`.
- `ctest --test-dir build/review-sample-r2/leaf-L14 -C Release --output-on-failure` exited 0: 3/3 passed.
- Direct run `build\review-sample-r2\leaf-L14\Release\L14_binary_fields_validation_bad.exe` exited 1 with `check failed: truncated payload keeps cursor`.

## Author evidence reviewed

- `references/validation/sample-state-fix-verify-core-ctest-r1.json`: `ctest --preset verify-core` exited 0, 13/13 passed.
- `references/validation/sample-state-fix-verify-debug-ctest-r1.json`: `ctest --preset verify-debug` exited 0, 13/13 passed.
- `references/validation/sample-state-fix-leaf-ctest-r1.json`: L14 leaf CTest exited 0, 3/3 passed.

## Result

APPROVE for the r2 sample gate. The r1 blocker is fixed: the bad implementation, CMake diagnostic, README, and chapter 14 now describe and test the same state-commit defect.
