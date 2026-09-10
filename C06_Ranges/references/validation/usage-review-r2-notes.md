# usage-review-r2 validation notes

- 日期：2026-09-10
- 绑定 HEAD：`f261bea559d6722c31135fb2d3589be52fe958ed`
- 审查报告：`C06_Ranges/references/reviews/usage-review-r2.md`
- 作者验证参考：`C06_Ranges/validation/ranges_usage_executor_validation_r2_20260910.md`

## 范围

仅复验 usage r2 对旧 `usage-review.md` 5 个 BLOCK 的修复点，以及受影响 7 个 unit：

- `C1_1_join`
- `B1_all_ref_owning`
- `B3_take_drop_closure`
- `C2_3_ranges_to`
- `D1_zip_adjacent_chunk`
- `D2_chunk_by_join_with_asconst`
- `D3_std_generator`

未修改作者源码，未删除旧失败报告，未扩展到其他作者范围。

## 证据文件

- 独立构建 summary：`C06_Ranges/references/validation/usage-review-r2-build-20260910/summary-f261bea5.json`
- 独立构建完整结果：`C06_Ranges/references/validation/usage-review-r2-build-20260910/results-f261bea5.json`
- 静态扫描记录：`C06_Ranges/references/validation/usage-review-r2-static-scan-f261bea5.txt`
- 当前审查范围文件哈希：`C06_Ranges/references/validation/usage-review-r2-filehashes-f261bea5.json`

## 独立构建命令摘要

对每个 unit 使用独立 build dir：

```powershell
cmake -S C06_Ranges/exercises/<unit> -B C06_Ranges/references/validation/usage-review-r2-build-20260910/<unit>
cmake --build <build-dir> --config Release
ctest --test-dir <build-dir> -C Release --output-on-failure
cmake --build <build-dir> --config Debug
ctest --test-dir <build-dir> -C Debug --output-on-failure
```

结果：

- 6 个 unit 的 Debug/Release ctest 12/12 通过：`C1_1_join`、`B1_all_ref_owning`、`B3_take_drop_closure`、`D1_zip_adjacent_chunk`、`D2_chunk_by_join_with_asconst`、`D3_std_generator`。
- `C2_3_ranges_to` 的 Release/Debug build 均失败，未进入 ctest，因此总执行 ctest 为 12/14。

## 失败摘录

`C06_Ranges/exercises/C2_3_ranges_to/main.cpp:43`：

```text
未满足关联约束
尝试匹配参数列表“(std::ranges::split_view<...>, std::ranges::_Range_closure<std::ranges::views::_Transform_fn,...>)”时
```

随后 `C06_Ranges/exercises/C2_3_ranges_to/main.cpp:46`：

```text
error C3536: “tokens”: 初始化之前无法使用
error C2088: 内置运算符 '==' 无法应用于类型为 'std::vector<std::string,std::allocator<std::string>>' 的操作数
```

完整失败日志见 `results-f261bea5.json`。

## 静态核对摘录

- `C06_Ranges/exercises/C1_1_join/README.md:24` 仍命中“不是 `common_range`”，且位于“预计练习方向”的直接学生要求中。
- `C06_Ranges/exercises/C1_1_join/README.md:80` 已提出不能说 `join_view` 一定不是 `common_range`，与前部练习要求矛盾。
- 未在审查范围内发现旧“580 行”声明。
- 未在审查范围内发现旧 D3 filter 输出 `3 4 5` 或“第一次 `++it` 才启动”的残留。

