# C06 Ranges 协议 slice 非作者审查

审查范围：`08-模块E-CPO与niebloid.md`、`09-模块F-概念精化与迭代器分类.md`、`10-模块G-自行实现视图.md`，以及 E1/E2/E3/F1/G1/G2/G3 七个练习的 CMake、README、checker、student/reference/good/bad 实现。

结论：REQUEST CHANGES。

## Stage 1 - 规格符合性

本轮可执行验证边界基本成立：E1/E2/E3/F1/G1/G2/G3 的 reference、good、bad-rejected、observation 目标在 Release 下 17/17 通过；E1/F1/G1/G2/G3 五个 Student 可执行文件均真实 exit 1。证据见：

- `C06_Ranges/references/validation/protocol-review-target-build.json`
- `C06_Ranges/references/validation/protocol-review-ctest-release.json`
- `C06_Ranges/references/validation/protocol-review-student-rejections.json`

但正文原位修复没有闭合，且 G3 reference/good 仍有一个未覆盖的真实接口缺口。

## Issues

### [HIGH] G3 声称接受 `viewable_range`，但非 common range 下结果不是 range

File: `C06_Ranges/exercises/G3_my_enumerate_borrowed/src/reference/my_enumerate_view.hpp:53`

Issue: `my_enumerate_view::end()` 对非 `common_range` 直接返回底层 `std::ranges::end(base_)`，但 iterator 只定义了 `operator==(iterator, iterator)`，没有定义 iterator 与底层 sentinel 的比较。`my_enumerate_fn::operator()(R&&)` 又接受任意 `std::ranges::viewable_range R`，所以 `std::views::istream<int>` 这类合法 input/non-common range 会得到一个不满足 `std::ranges::input_range` 的 `my_enumerate_view`。

Evidence: `C06_Ranges/references/validation/protocol-review-g3-noncommon-probe.json` 中，最小探针 `auto enumerated = c06_g3::my_enumerate(std::views::istream<int>(input)); static_assert(std::ranges::input_range<decltype(enumerated)>);` 编译失败，MSVC 报 `std::ranges::input_range<...>` 为 false，并指出 `iterator` 与 `std::default_sentinel_t` 没有可用 `!=`。

Fix: 给 G3 添加内部 sentinel wrapper，保存 `std::ranges::sentinel_t<V>`，实现 `operator==(iterator, sentinel)` / 反向比较，并让非 common `end()` 返回该 wrapper；或者把 `my_enumerate_view` 和工厂约束收窄到 `common_range`，同时同步 README/正文声明。更符合本课 ranges 教学目标的是前者，并补一个 non-common input range checker。

### [HIGH] G1 正文和练习 README 仍在原位教 unsafe `counted_iterator + default_sentinel` 路径

File: `C06_Ranges/10-模块G-自行实现视图.md:65`

Issue: G1 早段仍写“否则返回 `counted_iterator{begin(), count_}` 搭配 `default_sentinel`”，示例 `begin()` 又直接返回底层 `begin()`，没有把 counted iterator 放在 begin 侧，也没有在 unsized 短 range 下同时检查底层 end。`G1_my_take_view/README.md:47` 也继续要求路径 B 返回 `counted_iterator{std::ranges::begin(base_), count_}`。文末 `10-模块G...md:1146` 才补充“unsized 必须同时检查剩余计数或底层 end”，这不能替代原位置修复；学生按前文实现会越过短输入边界。

Evidence: 当前 reference/good 的实际实现已经改成 guarded iterator，说明作者知道原正文方案不成立；`G1_my_take_view/checks/main.cpp` 也有 `short_input_range` 覆盖短 input range。

Fix: 在 G1 原示例和 README 任务处直接改成三分支：random-access+sized 用 `begin + min`；sized 非 random-access 可用已截断 `counted_iterator`；unsized/non-common 使用保存底层 sentinel 的 guarded iterator。把旧 `counted_iterator + default_sentinel` 方案移到“历史错误/反例”小节，并明确为什么会越界。

### [HIGH] E 模块原位术语仍把 niebloid 说成 CPO

File: `C06_Ranges/08-模块E-CPO与niebloid.md:26`

Issue: 模块完成标准仍写“所有 niebloid 都是 CPO”，并说算法 niebloid “要求整个算法实现都封装在 `operator()` 内部”。同文件 `:769`、`:795` 也重复这个关系。文末 `:819` 才说“C++26 的算法函数对象不是把所有算法统称为可定制 CPO”，但没有修掉前面的学习目标和总结。规格要求原位置正确，不能用末尾校准抵消前文错误。

Fix: 在 `:26`、`:769`、`:795` 原位改成版本分层：ranges CPO 是可定制访问点对象，如 `ranges::begin`；ranges 算法 niebloid 是标准算法函数对象，提供 ADL 隔离、约束和 projection，不等于“可定制 CPO”。`std::ranges::sort` 可以作为函数对象传递，但不是用户可通过 ADL 定制的 CPO。

### [MEDIUM] G3 校准段把 enumerate 写成 C++23 语义

File: `C06_Ranges/10-模块G-自行实现视图.md:1150`

Issue: 同章前文和练习 README 均把标准 `views::enumerate` 对照定位到 C++26/P2164，但文末校准写“按 C++23 enumerate 语义教学实现”。这会和本轮明确要求的 C++26 enumerate 定位冲突。

Fix: 改为“按 C++26/P2164 enumerate 对照的教学子集实现”，并说明本题只实现索引/元素 proxy、`iter_move`、closure 组合和 borrowed 条件，不声称完整标准 `views::enumerate`。

## Stage 2 - 代码质量与安全

安全：未发现凭据、网络、文件删除、外部系统调用或注入面。

根因/回退：未见用 broad fallback 掩盖失败的代码。主要阻塞不是 fallback，而是 G3 对非 common range 的主接口契约缺口，以及正文用末尾校准代替原位修正。

诊断：当前环境没有可调用的 `lsp_diagnostics` / `ast_grep_search` 工具；我用 MSVC C++latest 目标构建、CTest、Student 直接运行和 `rg` 静态模式检查替代。由于存在 HIGH，不能批准。

## Recommendation

REQUEST CHANGES。

关闭上述 HIGH 后，建议复跑本报告保存的三类验证，并增加 G3 non-common input range 正/坏/Student 覆盖。
