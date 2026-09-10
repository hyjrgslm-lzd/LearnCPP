# C05 sample verifier evidence r1

Verifier: non-author technical / experiment review.
Date: 2026-09-09.
Scope: `include/c05/{types,bytes,utf}.hpp`, L01-L06, L14, shared `StudySetup`, chapter 14 sample narrative.

## Source fingerprints checked

Fresh `Get-FileHash -Algorithm SHA256` over scoped sample files matched the important author r2 hashes for:

- `include/c05/bytes.hpp`: `6CF21A07F7BF782D798591B1D81801C4C9C42DB130F545BD471AED86AB438B07`
- `include/c05/utf.hpp`: `36F6B4F3B99D0EBA3E62669E18E7026FA8171EA913B768B8C7469835C2F1B9F6`
- `L06_transcoding/checks/transcode_checks.cpp`: `438A55DC11008D6D88243A24F6F9DD7DDF5E8CF8716065192909648517C12023`
- `L14_binary_fields/checks/field_checks.cpp`: `4D6D62D187B6292CB511E1EB7D11650A62B9EC6B16C6971498C287DF6947AB10`
- `L14_binary_fields/validation/bad/bounded_field.hpp`: `7FA8CF1C2261EE715358B4F9D1A6BE7D95E108582B31A70485B79F31CBEDDCEB`

Additional files hashed in this pass:

- `include/c05/types.hpp`: `AF8AF2FCCF9FF02B36F912162CEB7D98CF94DF4A72CD04C2A47D947ACDDC3D39`
- `exercises/cmake/StudySetup.cmake`: `2D49EABA331C9E1109F71B24B82E89CA0E6ABB545CEEA4C5BA0AA538300B0E30`

## Fresh commands

- `cmake --version` from `C05_Data_Representation_Standard_Facilities/exercises` exited 0: CMake 4.2.3.
- `cmake -S . -B build/review-sample/core -G "Visual Studio 18 2026" -A x64 -DDATA_STUDY_BUILD_REFERENCE=ON -DDATA_STUDY_TEST_STUDENTS=OFF` exited 0: MSVC 19.51.36256.0 / toolset path `MSVC/14.51.36231`.
- `cmake --build build/review-sample/core --config Release --target ALL_BUILD --parallel 1 --verbose` exited 0: `已成功生成。0 个警告 0 个错误`.
- `ctest --test-dir build/review-sample/core -C Release --output-on-failure` exited 0: 13/13 passed.
- `cmake --build build/review-sample/core --config Debug --target ALL_BUILD --parallel 1` exited 0.
- `ctest --test-dir build/review-sample/core -C Debug --output-on-failure` exited 0: 13/13 passed.
- `cmake -S . -B build/review-sample/student -G "Visual Studio 18 2026" -A x64 -DDATA_STUDY_BUILD_REFERENCE=OFF -DDATA_STUDY_TEST_STUDENTS=ON` exited 0.
- `cmake --build build/review-sample/student --config Debug --target ALL_BUILD --parallel 1` exited 0 and generated only observation + student sample targets.
- `ctest --test-dir build/review-sample/student -C Debug --output-on-failure` exited 1: expected student failures observed for L03, L06, L14; 4 observation tests passed.
- `cmake -S L14_binary_fields -B build/review-sample/leaf-L14 -G "Visual Studio 18 2026" -A x64 -DDATA_STUDY_BUILD_REFERENCE=ON -DDATA_STUDY_TEST_STUDENTS=OFF` exited 0.
- `cmake --build build/review-sample/leaf-L14 --config Release --parallel 1` exited 0.
- `ctest --test-dir build/review-sample/leaf-L14 -C Release --output-on-failure` exited 0: L14 leaf 3/3 passed.
- Temporary oracle in `build/review-sample/oracle` built and ran:
  - `cmake -S build/review-sample/oracle -B build/review-sample/oracle-build -G "Visual Studio 18 2026" -A x64` exited 0.
  - `cmake --build build/review-sample/oracle-build --config Release --parallel 1` exited 0.
  - `build\review-sample\oracle-build\Release\c05_sample_verifier_oracle.exe` exited 0 with `oracle passed`.

## Oracle coverage

The temporary oracle directly checked:

- `append_be` byte order and `read_be<std::uint64_t>` max value.
- noncharacters remain valid scalar UTF-8.
- invalid UTF-8 categories: lone continuation, overlong 2/3 byte, surrogate, above U+10FFFF, 5-byte lead, bad continuation.
- valid UTF-16 surrogate pair, lone low surrogate, lone high surrogate, high followed by high.
- UTF-8 to UTF-16 output budget and UTF-16 to UTF-8 output budget.
- `read_utf8_field` invalid UTF-8 from nonzero cursor reports global byte offset and does not commit cursor.

## Blocking finding

Chapter 14 narrative no longer matches the checked bad implementation.

Evidence:

- `chapters/14-bounded-binary-fields.md:34` says the independent L14 bad variant consumes the four-byte header, sees truncated payload for `00 00 00 03 78`, and leaves `cursor=4`.
- `chapters/14-bounded-binary-fields.md:36` says the bad variant is rejected by diagnostic `truncated payload keeps cursor`.
- Actual `exercises/L14_binary_fields/validation/bad/bounded_field.hpp:13-24` uses local `probe`; on truncated payload it returns `incomplete_input` without assigning to external `cursor`.
- Actual `exercises/L14_binary_fields/validation/bad/bounded_field.hpp:30` assigns `cursor = probe` only after successful copy.
- Actual `exercises/L14_binary_fields/CMakeLists.txt:9` expects bad rejection text `rejects invalid utf8`.
- Direct run `build\review-sample\core\L14_binary_fields\Release\L14_binary_fields_validation_bad.exe` exited 1 with `check failed: rejects invalid utf8`.

Root cause: the bad variant was repaired or replaced to preserve cursor, but chapter 14 still describes the older cursor-advance counterexample and old diagnostic. The current bad variant demonstrates missing UTF-8 validation, not the stated cursor commit bug.

## Other risk

`cmake --build build/review-sample/core --config Release --parallel 2` reproducibly exited 1 through the CMake wrapper with only MSBuild version / partial ZERO_CHECK output and no compiler or linker error. Direct MSBuild with equivalent `/m:2 /v:n` exited 0, and `--parallel 1` builds passed. Treat as environment/tool-wrapper risk, not a proven sample source defect.
