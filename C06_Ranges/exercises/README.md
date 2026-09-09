# C06_Ranges 练习包

本目录是 `P:\C++Code\C06_Ranges\` 的代码练习部分，与根目录下 12 份章节 markdown 一一对应。

## 定位

- **理论在章节 markdown**：`../01-心智模型.md` 到 `../12-结课项目2-实现级源码阅读.md`
- **代码在本目录**：每章对应若干 `.cpp` 习题，每题独立可执行，TODO 填空形态
- **统一基线**：C++26，Windows 首选 Visual Studio 18 2026（见 `BUILD_GUIDE.md`）

## 索引

### 阶段一：使用层（概念理解与管道组合）

| 目录 | 对应章节 | 核心主题 |
|------|----------|----------|
| `01_mental_model_warmup/` | `../01-心智模型.md` | 六核心对象 + view 语义公理的 static_assert 预热 |
| `A1_iota_view/` | `../02-模块A-视图工厂与惰性.md` A-1 | `iota_view` 有界/无界、`unreachable_sentinel_t` |
| `A2_istream_view/` | `../02...` A-2 | `istream_view` 单遍、move-only iterator |
| `A3_repeat_cartesian/` | `../02...` A-3 | `repeat_view` + `cartesian_product_view` (C++23) |
| `B1_all_ref_owning/` | `../03-模块B-基础适配器与管道.md` B-1 | `views::all` → `ref_view` / `owning_view` (P2415R2) |
| `B2_filter_transform_degrade/` | `../03...` B-2 | filter/transform 迭代器概念降级、const 约束 |
| `B3_take_drop_closure/` | `../03...` B-3 | take/drop* 家族 + `range_adaptor_closure` (P2387R3) |
| `C1_1_join/` | `../04-模块C1-结构适配器.md` C1-1 | `views::join` 扁平化、迭代器概念 min |
| `C1_2_split_evolution/` | `../04...` C1-2 | C++20 `split` vs C++23 `split`/`lazy_split` (P2210R2) |
| `C1_3_common_reverse_elements/` | `../04...` C1-3 | `common` / `reverse` / `elements` / `keys` / `values` |
| `C2_1_projection/` | `../05-模块C2-算法·投影·范围边界.md` C2-1 | 投影（成员指针/lambda）、`min_max_result` |
| `C2_2_dangling_borrowed/` | `../05...` C2-2 | `ranges::dangling`、`enable_borrowed_range` 自定义 |
| `C2_3_ranges_to/` | `../05...` C2-3 | `ranges::to` CTAD + `from_range_t` (P1206R7 / P2781R5) |
| `D1_zip_adjacent_chunk/` | `../06-模块D-C++23高阶视图与协程桥.md` D-1 | `zip`/`zip_transform`/`adjacent`/`slide`/`chunk`/`stride` |
| `D2_chunk_by_join_with_asconst/` | `../06...` D-2 | `chunk_by`/`join_with`/`as_const`/`as_rvalue` |
| `D3_std_generator/` | `../06...` D-3 | `std::generator` 作为 `input_range` (P2502R2) |
| `CAPSTONE1_log_pipeline/` | `../07-结课项目1-数据管道与源码阅读.md` | CSV 日志多层管道 + projection + `ranges::to` |

### 阶段二：实现层（自定义 view 与适配器开发）

| 目录 | 对应章节 | 核心主题 |
|------|----------|----------|
| `E1_my_begin_cpo/` | `../08-模块E-CPO与niebloid.md` E-1 | 手写 `my_begin` CPO：成员/ADL/fallback 三路径 |
| `E2_niebloid/` | `../08...` E-2 | niebloid 可传递性 vs 函数模板 |
| `E3_cpo_tagdispatch_compare/` | `../08...` E-3 | ranges CPO vs stdexec `tag_invoke` 对比 |
| `F1_iterator_hierarchy/` | `../09-模块F-概念精化与迭代器分类.md` F-1 | forward → bidirectional → random_access → contiguous 递进 |
| `G1_my_take_view/` | `../10-模块G-自行实现视图.md` G-1 | `view_interface` CRTP + 自定义 take_view |
| `G2_my_transform_closure/` | `../10...` G-2 | `range_adaptor_closure` 注入 `operator|` |
| `G3_my_enumerate_borrowed/` | `../10...` G-3 | `enable_borrowed_range` 条件特化 |
| `H1_non_propagating_cache/` | `../11-模块H-高级实现模式.md` H-1 | `__non_propagating_cache` + `my_filter_view` |
| `H2_common_iter_proxy/` | `../11...` H-2 | `common_iterator` + `iter_move` / `iter_swap` 定制 |
| `H3_generator_const_iter/` | `../11...` H-3 | generator `promise_type` + `basic_const_iterator` |
| `CAPSTONE3_impl_source_reading/` | `../12-结课项目2-实现级源码阅读.md` 项目3 | stdlib ranges 源码对照 |
| `CAPSTONE4_mini_ranges/` | `../12...` 项目4 | mini-ranges 子集实现（6 层架构） |

合计 31 个子目录：27 道习题 + 3 个结课项目 + 1 个预热题。

## 开始做题

1. 阅读 `BUILD_GUIDE.md`，确认本地工具链满足 C++26 前置条件
2. 选一道题，先读章节 markdown 再打开 `main.cpp`
3. 搜索 `// TODO [必做]`，按提示补码
4. `cmake --preset vs2026 && cmake --build build-vs2026 --target <题目名>`
5. 回到该题 README 回答"复盘问题"

## 重要约束

- **view 的 O(1) 公理**：所有练习都假定你遵守"view 必须 O(1) move/copy/destroy"。违反这条公理的类型不能满足 `std::ranges::view` concept。
- **`filter_view` 的 const 迭代问题**：C++20/23 的 `filter_view` 不支持 const 迭代（`begin()` 是非 const 成员）。练习中会反复验证这一点。
- **C++23 vs C++20 `split` 语义差异**：C1_2 专门演示两者取舍；不要混淆。
- **proxy reference 的双轨**：C++23 `zip_view` 等的 `iterator_concept` 与 `iterator_category` 不同。

详见根 `../README.md` 的"两个阶段定位差异"和"术语速查表"。
