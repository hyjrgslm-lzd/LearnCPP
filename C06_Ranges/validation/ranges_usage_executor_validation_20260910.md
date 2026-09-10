# C06 Ranges usage executor validation - 2026-09-10

Scope: chapters 01-07 body review plus 16 usage observation exercise entries owned by /root/ranges_usage.

Commands run from `F:\CPPTrain\LearnCPP`.

## Structure check

```powershell
python - <<'PY'
# Checked the 16 owned exercise directories:
# - CMakeLists.txt includes ../cmake/RangesSetup.cmake
# - CMakeLists.txt calls ranges_add_observation(... main.cpp)
# - main.cpp includes <check.hpp>
# - main.cpp does not include <print>
# - README.md contains ## 参考解析
PY
```

Result: `structure check 16/16`.

## Release build and CTest

```powershell
$units = @('01_mental_model_warmup','A1_iota_view','A2_istream_view','A3_repeat_cartesian','B1_all_ref_owning','B2_filter_transform_degrade','B3_take_drop_closure','C1_1_join','C1_2_split_evolution','C1_3_common_reverse_elements','C2_1_projection','C2_2_dangling_borrowed','C2_3_ranges_to','D1_zip_adjacent_chunk','D2_chunk_by_join_with_asconst','D3_std_generator')
foreach ($u in $units) {
  $src = Join-Path 'C06_Ranges/exercises' $u
  $bld = Join-Path $src 'build-author'
  cmake -S $src -B $bld
  cmake --build $bld --config Release --parallel 2
  ctest --test-dir $bld -C Release --output-on-failure
}
```

Result: 16/16 passed.

```text
PASS Release 01_mental_model_warmup :: 100% tests passed, 0 tests failed out of 1
PASS Release A1_iota_view :: 100% tests passed, 0 tests failed out of 1
PASS Release A2_istream_view :: 100% tests passed, 0 tests failed out of 1
PASS Release A3_repeat_cartesian :: 100% tests passed, 0 tests failed out of 1
PASS Release B1_all_ref_owning :: 100% tests passed, 0 tests failed out of 1
PASS Release B2_filter_transform_degrade :: 100% tests passed, 0 tests failed out of 1
PASS Release B3_take_drop_closure :: 100% tests passed, 0 tests failed out of 1
PASS Release C1_1_join :: 100% tests passed, 0 tests failed out of 1
PASS Release C1_2_split_evolution :: 100% tests passed, 0 tests failed out of 1
PASS Release C1_3_common_reverse_elements :: 100% tests passed, 0 tests failed out of 1
PASS Release C2_1_projection :: 100% tests passed, 0 tests failed out of 1
PASS Release C2_2_dangling_borrowed :: 100% tests passed, 0 tests failed out of 1
PASS Release C2_3_ranges_to :: 100% tests passed, 0 tests failed out of 1
PASS Release D1_zip_adjacent_chunk :: 100% tests passed, 0 tests failed out of 1
PASS Release D2_chunk_by_join_with_asconst :: 100% tests passed, 0 tests failed out of 1
PASS Release D3_std_generator :: 100% tests passed, 0 tests failed out of 1
```

## Debug build and CTest

```powershell
foreach ($u in $units) {
  $src = Join-Path 'C06_Ranges/exercises' $u
  $bld = Join-Path $src 'build-author'
  cmake --build $bld --config Debug --parallel 2
  ctest --test-dir $bld -C Debug --output-on-failure
}
```

Result: 16/16 passed.

```text
PASS Debug 01_mental_model_warmup :: 100% tests passed, 0 tests failed out of 1
PASS Debug A1_iota_view :: 100% tests passed, 0 tests failed out of 1
PASS Debug A2_istream_view :: 100% tests passed, 0 tests failed out of 1
PASS Debug A3_repeat_cartesian :: 100% tests passed, 0 tests failed out of 1
PASS Debug B1_all_ref_owning :: 100% tests passed, 0 tests failed out of 1
PASS Debug B2_filter_transform_degrade :: 100% tests passed, 0 tests failed out of 1
PASS Debug B3_take_drop_closure :: 100% tests passed, 0 tests failed out of 1
PASS Debug C1_1_join :: 100% tests passed, 0 tests failed out of 1
PASS Debug C1_2_split_evolution :: 100% tests passed, 0 tests failed out of 1
PASS Debug C1_3_common_reverse_elements :: 100% tests passed, 0 tests failed out of 1
PASS Debug C2_1_projection :: 100% tests passed, 0 tests failed out of 1
PASS Debug C2_2_dangling_borrowed :: 100% tests passed, 0 tests failed out of 1
PASS Debug C2_3_ranges_to :: 100% tests passed, 0 tests failed out of 1
PASS Debug D1_zip_adjacent_chunk :: 100% tests passed, 0 tests failed out of 1
PASS Debug D2_chunk_by_join_with_asconst :: 100% tests passed, 0 tests failed out of 1
PASS Debug D3_std_generator :: 100% tests passed, 0 tests failed out of 1
```

## Notes

Top-level aggregate C06 exercises build was not used for final evidence because the shared tree currently contains unrelated/unowned H3/CAPSTONE work. Final verification used isolated per-entry `build-author` directories as requested.
