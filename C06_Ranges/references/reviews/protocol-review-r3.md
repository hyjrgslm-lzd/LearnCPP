# C06 Ranges 协议 r3 非作者窄复验

Verdict: **BLOCK / REQUEST CHANGES**

本轮复验绑定当前工作区源码，聚焦 r2 唯一新 BLOCK：G2 `my_transform_view` non-common input/sentinel/category 路径，并复扫同族 G1/G3。源码层面的 r2 BLOCK 已修复：G2 reference 使用 sentinel wrapper 和惰性 `iterator_category` helper，独立 G2/G3 `istream_view` probes 都能编译并运行通过；G1/G2/G3 fresh Debug/Release 测试也通过。

仍不能 APPROVE：G2 教学正文没有同步到当前源码。`10-模块G-自行实现视图.md` 的主代码块仍返回 `sentinel{...}` 却没有定义 `sentinel` 类型；同一代码块和 G2 README 仍只展示 `operator()(R&&) const` 拷贝 `f_` 的 closure 写法，没有当前源码中支持 move-only callable 的 `const&`/`&&` 分裂。学生按正文/README 实现，会得到和当前 reference 不一致的不可编译或不满足 move-only 验收的代码。

## Scope

- `C06_Ranges/10-模块G-自行实现视图.md`
- `C06_Ranges/exercises/G1_my_take_view/{README.md,main.cpp,src/reference/my_take_view.hpp}`
- `C06_Ranges/exercises/G2_my_transform_closure/{README.md,main.cpp,src/reference/my_transform_view.hpp,validation/good/my_transform_view.hpp}`
- `C06_Ranges/exercises/G3_my_enumerate_borrowed/{README.md,main.cpp,src/reference/my_enumerate_view.hpp}`
- 作者 r3 记录：`C06_Ranges/references/validation/ranges-protocols-r3/`
- 本轮新证据：`C06_Ranges/references/validation/protocol-review-r3-*`

## 已复验通过的 r2 BLOCK 修复

- **G2 reference non-common input 修复通过。** `G2_my_transform_closure/src/reference/my_transform_view.hpp:12-25` 定义了惰性 `iterator_category_for`；`:31`、`:66-76` 定义 sentinel wrapper；`:83-86` 在 non-common 路径返回 wrapper；`:100-108` 区分 `const&` 拷贝 closure 和 `&&` move closure。
- **G2 good oracle 已独立。** `G2_my_transform_closure/validation/good/my_transform_view.hpp:8-20` 直接别名/调用标准库 `std::ranges::transform_view` / `std::views::transform`，不再复制 reference 实现。
- **G2 checker 覆盖 input/non-common。** `G2_my_transform_closure/main.cpp:58-79` 覆盖 empty、short、post++、non-common input sentinel；`:54-55` 覆盖 move-only rvalue closure；`:80-86` 覆盖 stateful callable 状态持久化。
- **G1/G3 同族路径未发现回退。** G1 `src/reference/my_take_view.hpp:24-37` 移动保存底层 iterator/sentinel 并用 `remaining <= 0 || current == last` 终止；G3 `src/reference/my_enumerate_view.hpp:54-72` 仍保留 sentinel wrapper，`:49-50` 保留 proxy `iter_move`。

## Issues

[HIGH] G2 教学正文仍是旧骨架，和当前 reference/checker 自相矛盾  
File: `C06_Ranges/10-模块G-自行实现视图.md:483-490`、`:523-528`；`C06_Ranges/exercises/G2_my_transform_closure/README.md:64-70`、`:77-81`。  
Issue: 10 章主代码块在 non-common 分支返回 `sentinel{std::ranges::end(base_)}`，但该代码块没有 `class sentinel` / `struct sentinel` 定义；`protocol-review-r3-static-scan.json` 对 10 章 G2 片段只命中 `return sentinel`，没有命中 `class sentinel` 或 `struct sentinel`。同一主代码块和 G2 README 的 closure 任务仍只展示 `constexpr auto operator()(R&& r) const { return ... f_; }`，没有源码里的 `const&` + `&&` overload，也没有 `copy_constructible<F>` 限制；这与 `main.cpp:54-55` 的 move-only callable 验收和 `src/reference/my_transform_view.hpp:100-108` 的实际实现不一致。  
Risk: 当前源码测试能通过，但教学入口仍会把学生带回旧实现：non-common input 路径缺 sentinel 类型，move-only closure 路径缺 rvalue move 调用。末尾或源码修正不能替代正文原位置修复。  
Fix: 将 10 章 G2 完整代码块同步到 `src/reference/my_transform_view.hpp` 的当前结构：补 `class sentinel` 声明/定义、`friend class sentinel`、iterator/sentinel 比较；把 closure operator 改为 `operator()(R&&) const& requires copy_constructible<F>` 和 `operator()(R&&) &&` 两个重载。G2 README 任务 5/6 同步说明 sentinel wrapper 定义位置与 move-only closure 的双重载要求。

## Validation

- `protocol-review-r3-target-build.json`：fresh configure/build，G1/G2/G3 的 reference/good/bad/student 目标在 Release+Debug 全部构建成功。
- `protocol-review-r3-summary.json`：Release 9/9 PASS，Debug 9/9 PASS；G1/G2/G3 student executable 在 Release+Debug 共 6 次均真实运行失败。
- `protocol-review-r3-noncommon-probes.json`：独立 G2/G3 `std::views::istream<int>` non-common input probes 均 build/run exit 0。
- `protocol-review-r3-static-scan.json`：确认 10 章和 G2 README 仍存在文档同步缺口；范围内未命中 hardcoded secret、空 catch、`console.log`、`apiKey =`、`password =`、`token =`、`best effort`。

`lsp_diagnostics` 工具在当前工具面不可用；本轮用 MSVC Debug/Release 编译、CTest、student executable、独立 CMake probes 和静态 grep 作为诊断替代。

## Recommendation

**REQUEST CHANGES / BLOCK**。源码层面的 r2 G2 BLOCK 已修复；请修复 G2 章节/README 的原位教学代码与任务说明后再复审。
