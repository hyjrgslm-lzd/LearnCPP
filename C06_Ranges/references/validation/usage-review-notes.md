# C06 Ranges usage review validation notes - 2026-09-10

Reviewer: `/root/usage_review`

## Commands and evidence

Diff scope:

```powershell
git diff --stat
git diff --name-only
```

Targeted structure and source inspection:

```powershell
rg -n "ranges_add_observation|ranges_add_exercise|add_subdirectory" C06_Ranges\exercises -g "CMakeLists.txt"
rg -n "check\(|static_assert|requires|__cpp_lib_|HAS_|RANGES|to<std::|generator|elements_of|as_const|chunk_by|join_with|zip|split|lazy_split|borrowed_range|dangling|owning_view|ref_view|filter|transform|cartesian|repeat|iota|istream_view" C06_Ranges\exercises\...\main.cpp
rg -n "当前程序|预测：|验证产出|日志验证|drop_while|分数|split|窗口和|递增分组" C06_Ranges\exercises\...\README.md
```

Result: all 16 owned usage entry `CMakeLists.txt` files call `ranges_add_observation(... main.cpp)`. The 16 `main.cpp` files contain live `check()` and/or `static_assert` coverage. Several README "当前程序" descriptions do not match those checks; details are in `references/reviews/usage-review.md`.

## Independent build matrix

Script used:

```powershell
$units=@('01_mental_model_warmup','A1_iota_view','A2_istream_view','A3_repeat_cartesian','B1_all_ref_owning','B2_filter_transform_degrade','B3_take_drop_closure','C1_1_join','C1_2_split_evolution','C1_3_common_reverse_elements','C2_1_projection','C2_2_dangling_borrowed','C2_3_ranges_to','D1_zip_adjacent_chunk','D2_chunk_by_join_with_asconst','D3_std_generator')
$root='C06_Ranges\references\validation\usage-review-build-20260910'
foreach($u in $units) {
  $src=Join-Path 'C06_Ranges\exercises' $u
  $bld=Join-Path $root $u
  cmake -S $src -B $bld
  cmake --build $bld --config Release --parallel 2
  ctest --test-dir $bld -C Release --output-on-failure
  cmake --build $bld --config Debug --parallel 2
  ctest --test-dir $bld -C Debug --output-on-failure
}
```

Result: 32/32 configure/build/ctest result rows had exit code 0.

Raw result file: `C06_Ranges/references/validation/usage-review-build-20260910/results.json`

Compiler/toolchain observed in the matrix: Visual Studio 18 2026, MSVC 19.51.36256.0, CMake 4.2.3.

## Join common-range probe

Probe source: `C06_Ranges/references/validation/usage-review-probes/join_common_probe.cpp`

Relevant assertion:

```cpp
std::vector<std::vector<int>> nested = {{1, 2}, {3, 4}};
auto flat = nested | std::views::join;
static_assert(std::ranges::common_range<decltype(flat)>);
static_assert(std::same_as<decltype(flat.begin()), decltype(flat.end())>);
```

Build command:

```powershell
cmake -S C06_Ranges\references\validation\usage-review-probes -B C06_Ranges\references\validation\usage-review-probes\build
cmake --build C06_Ranges\references\validation\usage-review-probes\build --config Release --parallel 2
```

Result: build exit code 0. This contradicts `C06_Ranges/04-模块C1-结构适配器.md:114`, which asserts `!std::ranges::common_range<decltype(flat)>` for the same stored `vector<vector<int>>` shape.

## Tool limitation

No dedicated `lsp_diagnostics` tool was exposed in this subagent lane. The review used MSVC compile diagnostics from the Debug/Release matrix as the available type/compile-safety check.
