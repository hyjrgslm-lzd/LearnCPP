# 结课项目 3：实现级源码对照

> 对应章节：`12-结课项目2-实现级源码阅读.md` / CAPSTONE3。

## 项目目标

把模块 E-H 的实现模式带回标准库源码：CPO、`view_interface`、`range_adaptor_closure`、inner iterator、`__non_propagating_cache`、iterator/sentinel、`iterator_concept`、`borrowed_range`。

本项目不是“搜到符号名就算读懂”。`main.cpp` 是 observation target，用可编译的 `static_assert` 和运行时观察固定几个实现事实；`notes/` 负责把这些事实解释成源码阅读笔记。

## 固定源码入口

以 `references/source-reading.md` 中固定版本为准。本轮对照使用 MSVC STL 145 / `_MSVC_STL_UPDATE=202604L` 的 `<ranges>` 实现；不同标准库命名不同，但模式相同。

常见映射：

- libstdc++ `_M_current` / `_M_parent` 对应 MSVC STL `_Current` / `_Parent` 一类命名。
- libstdc++ `__non_propagating_cache` 对应 MSVC STL 的非传播缓存工具。
- `range_adaptor_closure`、`view_interface`、inner iterator、sentinel 的结构角色比具体私有名字更重要。

## 交付文件

- `main.cpp`：observation target。验证选中 view 的 iterator/concept/cache/borrowed 行为。
- `notes/01_object_diagram.md`：对象关系图，边标注 owns 或 points-to。
- `notes/02_pattern_table.md`：源码模式表，记录每个 view 的存储、iterator 能力、缓存和误解修正。
- `notes/03_cpo_checklist.md`：CPO 清单，并与 stdexec tag_invoke 风格做设计对比。
- `notes/04_reading_note.md`：完整读书笔记，不只罗列符号。

## observation 覆盖

- `single_view<int>` 的 iterator 是指针形态。
- `empty_view<int>` 是 sized 和 borrowed。
- `iota_view<int, int>` 是 borrowed，iterator concept 为 random access。
- `transform_view<vector<int>>` 从 contiguous 降到 random access。
- `filter_view<vector<int>>` 不是 sized；forward 底层仍有 begin 缓存，普通 const begin 不成立。
- `join_view<vector<vector<int>>>` 不自动 borrowed，并按底层能力给出 forward/bidirectional iterator。
- `std::ranges::begin` 是 CPO 对象，不是普通函数模板。

## C++26 标准更新口径

N5047 LWG Poll 13 以 DR 采纳 P3725R3，给 `filter_view` 增加受限 `const begin/end` 分支：`input_range<const V> && !forward_range<const V>` 且 predicate 可 const 调用。普通 vector 底层仍属于 forward 缓存分支，不能据此笼统说“所有 filter 都可 const 迭代”，也不能继续说“任何 filter 都不可 const 迭代”。

Poll 14 / P3828R1 把前沿工具名从 `to_input` 改为 `as_input`。本项目的 observation 不依赖该前沿实现。

## 验收点

- observation target 能独立编译运行。
- 四份 notes 都有实质解析，并能对应到固定源码入口。
- 解释缓存、iterator 能力、borrowed、CPO 的原因，而不是只列符号。
- 标准更新口径与 `references/standards.md` 一致。
