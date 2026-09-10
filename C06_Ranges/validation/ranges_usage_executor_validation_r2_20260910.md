# C06 Ranges usage executor validation r2 - 2026-09-10

Scope: narrow fix for `C06_Ranges/references/reviews/usage-review.md` 5 blocking items.

Commands run from `F:\CPPTrain\LearnCPP`.

## Changed programs revalidated

Affected entries:

- `C1_1_join`
- `B1_all_ref_owning`
- `B3_take_drop_closure`
- `C2_3_ranges_to`
- `D1_zip_adjacent_chunk`
- `D2_chunk_by_join_with_asconst`
- `D3_std_generator`

Command shape:

```powershell
$units = @('C1_1_join','B1_all_ref_owning','B3_take_drop_closure','C2_3_ranges_to','D1_zip_adjacent_chunk','D2_chunk_by_join_with_asconst','D3_std_generator')
foreach ($cfg in @('Release','Debug')) {
  foreach ($u in $units) {
    $src = Join-Path 'C06_Ranges/exercises' $u
    $bld = Join-Path $src 'build-author'
    cmake -S $src -B $bld
    cmake --build $bld --config $cfg --parallel 2
    ctest --test-dir $bld -C $cfg --output-on-failure
  }
}
```

Result: 14/14 passed.

```text
PASS Release C1_1_join :: 100% tests passed, 0 tests failed out of 1
PASS Release B1_all_ref_owning :: 100% tests passed, 0 tests failed out of 1
PASS Release B3_take_drop_closure :: 100% tests passed, 0 tests failed out of 1
PASS Release C2_3_ranges_to :: 100% tests passed, 0 tests failed out of 1
PASS Release D1_zip_adjacent_chunk :: 100% tests passed, 0 tests failed out of 1
PASS Release D2_chunk_by_join_with_asconst :: 100% tests passed, 0 tests failed out of 1
PASS Release D3_std_generator :: 100% tests passed, 0 tests failed out of 1
PASS Debug C1_1_join :: 100% tests passed, 0 tests failed out of 1
PASS Debug B1_all_ref_owning :: 100% tests passed, 0 tests failed out of 1
PASS Debug B3_take_drop_closure :: 100% tests passed, 0 tests failed out of 1
PASS Debug C2_3_ranges_to :: 100% tests passed, 0 tests failed out of 1
PASS Debug D1_zip_adjacent_chunk :: 100% tests passed, 0 tests failed out of 1
PASS Debug D2_chunk_by_join_with_asconst :: 100% tests passed, 0 tests failed out of 1
PASS Debug D3_std_generator :: 100% tests passed, 0 tests failed out of 1
```

## Behavior now checked by programs

- `C1_1_join/main.cpp`: stored `vector<vector<int>> | views::join` asserts `common_range` and same begin/end type; separate `iota | filter | take` asserts non-common iter/sentinel range and checks `{0,2,4,6}`.
- `B1_all_ref_owning/main.cpp`: checks original vector mutation is observed through `ref_view`; checks `owning_view` iterates owned values with sum `15`.
- `B3_take_drop_closure/main.cpp`: adds `drop_while` check producing `{3,4,5}`.
- `C2_3_ranges_to/main.cpp`: adds split materialization into `vector<string>{"alpha","beta","gamma"}`.
- `D1_zip_adjacent_chunk/main.cpp`: adds `adjacent<3>` structured-binding window sum `15`.
- `D2_chunk_by_join_with_asconst/main.cpp`: adds nondecreasing `chunk_by` grouping sizes `{3,4}` while preserving equal-run RLE checks.
- `D3_std_generator/main.cpp`: still checks preorder `{1,2,4,3,5,6}` and filtered DFS order `{4,3,5,6}`; filtering does not sort.

## Static review probes

```powershell
rg -n '3 4 5|第一次 `\+\+it`|全文 580|join_view.*不是 `common_range`|static_assert\(!std::ranges::common_range<decltype\(flat\)>\)' C06_Ranges/04-模块C1-结构适配器.md C06_Ranges/06-模块D-C++23高阶视图与协程桥.md C06_Ranges/exercises/C1_1_join C06_Ranges/exercises/D3_std_generator
```

Result: stale block phrase scan returned clean.

