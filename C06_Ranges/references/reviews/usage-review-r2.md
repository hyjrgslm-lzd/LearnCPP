# C06 Ranges usage r2 非作者审查

- 审查日期：2026-09-10
- 绑定 HEAD：`f261bea559d6722c31135fb2d3589be52fe958ed`
- 审查范围：仅复验 usage r2 对旧 `usage-review.md` 5 个 BLOCK 的修复，以及受影响 7 个 unit：`C1_1_join`、`B1_all_ref_owning`、`B3_take_drop_closure`、`C2_3_ranges_to`、`D1_zip_adjacent_chunk`、`D2_chunk_by_join_with_asconst`、`D3_std_generator`。
- 复验限制：未改作者源码；未递归委派；旧失败报告和旧验证产物保留。

## 结论

BLOCK。

旧 `D3` DFS/filter/begin 描述已修，`B1/B3/D1/D2` 的 README 摘要与实际 checks 已对齐，模块 D 末尾旧“580 行”声明已删除。但当前 r2 仍有 2 个会影响交付的实质问题：一个是 `C1_1_join` README 原位残留错误要求，另一个是 `C2_3_ranges_to` 新增真实 check 在独立 Debug/Release 构建中均无法编译。

## 问题

### [HIGH] C1 README 仍要求学生对 stored `vector<vector<int>> | views::join` 断言 `not common_range`

File: `C06_Ranges/exercises/C1_1_join/README.md:23`

Issue: “预计练习方向”第 3 点仍写成“用 `static_assert` 验证 `join_view` 迭代器是 `bidirectional_iterator`，不是 `random_access_iterator`，且不是 `common_range`”。同一文件后文 `C06_Ranges/exercises/C1_1_join/README.md:80` 已提出“为什么不能说 `join_view` 一定不是 `common_range`”，`C06_Ranges/exercises/C1_1_join/main.cpp:11` 和 `:12` 也已经断言 stored `vector<vector<int>> | views::join` 是 `common_range` 且 begin/end 同型。也就是说，正文前部仍在原练习位置要求学生写一个会与真实代码相反的断言。

Fix: 在 `C1_1_join/README.md:23-24` 原位改成：stored `vector<vector<int>> | views::join` 应验证 `bidirectional_iterator`、非 `random_access_iterator`、并且是 `common_range`；另单列一个 iter/sentinel 异型的非 common 示例，例如当前 `main.cpp:16-21` 的 `iota | filter | take`。同时把 `README.md:52-53` 的“通常不是 common_range”改成条件化表述，避免和 stored vector 示例冲突。

### [HIGH] `C2_3_ranges_to` 的新增 split materialization check 当前无法编译

File: `C06_Ranges/exercises/C2_3_ranges_to/main.cpp:42`

Issue: r2 新增的 split materialization 写法：

```cpp
auto tokens = std::string_view{"alpha,beta,gamma"}
    | std::views::split(',')
    | std::views::transform([](auto part) { return std::string(part); })
    | std::ranges::to<std::vector>();
```

在独立 out-of-tree build 下 Release 和 Debug 均失败。失败点是 `main.cpp:43` 的 pipe 约束未满足，随后 `main.cpp:46` 报 `tokens` 初始化前无法使用。原始日志保存在 `C06_Ranges/references/validation/usage-review-r2-build-20260910/results-f261bea5.json`。

Fix: 用当前标准库可编译的 range materialization 写法修复 lambda，例如把子 range 显式转成 string：`[](auto part) { return part | std::ranges::to<std::string>(); }`；若该实现仍不支持子 range 到 string 的 `ranges::to`，退回到 `std::string(std::ranges::begin(part), std::ranges::end(part))`。修复后必须重新跑 `C2_3_ranges_to` 的独立 Debug/Release build + ctest，不能只沿用旧 build 目录结果。

## 已通过复验的旧点

- `C06_Ranges/04-模块C1-结构适配器.md:112-115` 已把 stored `vector<vector<int>> | views::join` 修为 begin/end 同型、`common_range`。
- `C06_Ranges/04-模块C1-结构适配器.md:119-131` 已单列真正 non-common 的 iter/sentinel 异型示例。
- `C06_Ranges/06-模块D-C++23高阶视图与协程桥.md:771-785` 已统一 D3 树 DFS 输出为 `1 2 4 3 5 6`，filter 输出为 `4 3 5 6`。
- `C06_Ranges/exercises/D3_std_generator/main.cpp:47-51` 已用 `begin()` 首次启动到第一个 yield、`++it` 推进到下一个 yield 的方式验证。
- `B1/B3/D1/D2` 的 README “当前程序”摘要与各自 `main.cpp` 中新增 checks 对齐。
- `C06_Ranges/06-模块D-C++23高阶视图与协程桥.md` 末尾未再发现旧“580 行”声明。

## 验证

- 独立构建验证：`C06_Ranges/references/validation/usage-review-r2-build-20260910/summary-f261bea5.json`
  - `C1_1_join`、`B1_all_ref_owning`、`B3_take_drop_closure`、`D1_zip_adjacent_chunk`、`D2_chunk_by_join_with_asconst`、`D3_std_generator`：Debug/Release ctest 12/12 通过。
  - `C2_3_ranges_to`：Release/Debug build 均失败，因此总计只执行 12/14 个 ctest。
- 原始构建日志：`C06_Ranges/references/validation/usage-review-r2-build-20260910/results-f261bea5.json`
- 静态扫描记录：`C06_Ranges/references/validation/usage-review-r2-static-scan-f261bea5.txt`
- 文件哈希绑定：`C06_Ranges/references/validation/usage-review-r2-filehashes-f261bea5.json`

