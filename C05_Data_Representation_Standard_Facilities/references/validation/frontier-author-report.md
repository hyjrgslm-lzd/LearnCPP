# F01 frontier author validation

时间：2026-09-09 23:40 +08:00

范围只覆盖 `exercises/F01_frontier` 和 `chapters/18-standard-frontier.md`。公共 CMake、standard index、17章未修改。

## 环境

- Compiler: MSVC 19.51.36256.0
- STL include: `D:/VisualStudio2026/Installed/VC/Tools/MSVC/14.51.36231/include`
- Fixed upstream MSVC STL commit used for source-reading anchors: `4edbc1d63a1ec156bed6dbf1727f413fa682abba`
- CMake generator: Visual Studio 18 2026, x64
- F01 targets: `LanguageStandard=stdcpplatest`; verbose build also showed `/std:c++latest`

## Commands

- `cmake -S C05_Data_Representation_Standard_Facilities/exercises/F01_frontier -B C05_Data_Representation_Standard_Facilities/exercises/build/f01-frontier-off`
- `cmake --build C05_Data_Representation_Standard_Facilities/exercises/build/f01-frontier-off --config Release`
- `ctest --test-dir C05_Data_Representation_Standard_Facilities/exercises/build/f01-frontier-off -C Release --output-on-failure`
- `cmake -S C05_Data_Representation_Standard_Facilities/exercises/F01_frontier -B C05_Data_Representation_Standard_Facilities/exercises/build/f01-frontier-on -DDATA_STUDY_ENABLE_FRONTIER=ON`
- `cmake --build C05_Data_Representation_Standard_Facilities/exercises/build/f01-frontier-on --config Release`
- `ctest --test-dir C05_Data_Representation_Standard_Facilities/exercises/build/f01-frontier-on -C Release --output-on-failure`

Raw records:

- `frontier-author-configure-off.json`
- `frontier-author-build-off.json`
- `frontier-author-show-only-off.json`
- `frontier-author-configure-on.json`
- `frontier-author-build-on.json`
- `frontier-author-ctest-on.json`
- `frontier-author-summary.json`
- `frontier-author-source-hashes.json`

## Result

OFF path: configure reports `F01 disabled; no capability tests registered`; `ctest --show-only=json-v1` contains zero F01 tests. This is DISABLED, not a capability SKIP.

ON path: 9 capability executables compiled and linked. CTest result: 0 failed, 9 skipped. SKIP is the correct capability state for this local toolchain; it is not reported as PASS. Configure-time `std::format("{}", 42)` probe succeeded before classifying the P3395 `std::error_code` formatter probe.

| Target | Status | Evidence |
| --- | --- | --- |
| `F01_text_encoding_locale` | SKIP | `<text_encoding>` is not available |
| `F01_charconv_result_bool` | SKIP | `__cpp_lib_to_chars` below `202306L` |
| `F01_to_string_semantics` | SKIP | `__cpp_lib_to_string` missing |
| `F01_runtime_format` | SKIP | `__cpp_lib_format` below `202311L` |
| `F01_path_formatter` | SKIP | `__cpp_lib_format_path` missing |
| `F01_format_float_c29` | SKIP | actual output probe observed old P3505 behavior: `1e+05` and `1234567890123456774144` |
| `F01_error_code_formatter_c29` | SKIP | configure-time semantic probe for `std::format("{}", std::error_code)` failed |
| `F01_bitops_shift_c29` | SKIP | `__cpp_lib_bitops` below `202606L` |
| `F01_bitops_permutation_c29` | SKIP | `__cpp_lib_bitops` below `202607L` |

## Notes

The C29 format checks do not rely on an old aggregate macro as proof of absence. `F01_format_float_c29` runs actual output probes, and `F01_error_code_formatter_c29` uses a configure-time compile probe before compiling the runtime formatter call path.

No SIMD probe was added; P3772R2 is documented in chapter 18, but SIMD belongs to the later SIMD course lane and this F01 unit does not add a SIMD dependency.
