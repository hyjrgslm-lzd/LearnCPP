# C06 Ranges 协议 r2 非作者审查

Verdict: **BLOCK / REQUEST CHANGES**

本轮复核绑定当前工作区源码，未修改作者源码。原 r1 四个阻断点已按 r2 目标完成原位修复；但新增发现 G2 `my_transform_view` 仍声明/实现了 non-common range 路径，却不能支持 `std::views::istream` 这类 input/non-common range。该问题会让 reference/good 在一类基础 ranges 输入上不是合法 `input_range`，因此不能批准冻结交付。

## 审查范围

- 正文：`C06_Ranges/08-模块E-CPO与niebloid.md`、`C06_Ranges/09-模块F-概念精化与迭代器分类.md`、`C06_Ranges/10-模块G-自行实现视图.md`
- 练习：`E1_my_begin_cpo`、`E2_niebloid`、`E3_cpo_tagdispatch_compare`、`F1_iterator_hierarchy`、`G1_my_take_view`、`G2_my_transform_closure`、`G3_my_enumerate_borrowed`
- 作者 r2 记录：`C06_Ranges/references/validation/ranges-protocols-r2/`
- 本轮新证据：`C06_Ranges/references/validation/protocol-review-r2-*`

## 原 r1 BLOCK 复验

- **E/CPO 分类：已修复。** `08-模块E-CPO与niebloid.md:24-28` 和 `:769-797` 已明确区分 `ranges::begin/end/size` 访问 CPO 与 `ranges::sort/find/transform` 算法 niebloid；不再把所有 niebloid 都写成用户可定制 CPO。
- **G1 unsized/default_sentinel 旧方案：已修复。** `10-模块G-自行实现视图.md:63-65`、`:94-111` 和 `G1_my_take_view/README.md:13`、`:53` 都已把 unsized 路径改为保存底层 sentinel 的 guarded iterator，并说明 `remaining == 0 || current == last` 才终止。
- **G3 non-common/input sentinel：已修复。** `G3_my_enumerate_borrowed/README.md:76-79`、`:142-144` 已禁止直接返回 raw sentinel；`src/reference/my_enumerate_view.hpp:54-62` 提供 sentinel wrapper，`:68-73` 在非 common+sized 路径返回 wrapper；`main.cpp:45-66` 覆盖 empty、short、non-common input、move-only input iterator、post++ 和 sentinel 比较。
- **G3 enumerate 标准版本口径：已修复到 r2 要求。** `10-模块G-自行实现视图.md:1058-1062` 和 `G3_my_enumerate_borrowed/README.md:154-162` 统一写为 C++23/P2164。
- **五个 key checker 入口：已修复。** `E1/F1/G1/G2/G3` 的 `CMakeLists.txt` 均指向 `CHECK main.cpp`，旧 `checks/` 目录在这五个练习下不存在。

## Issues

[HIGH] G2 `my_transform_view` 的 non-common input range 路径仍不合法  
File: `C06_Ranges/exercises/G2_my_transform_closure/src/reference/my_transform_view.hpp:27-28`、`:53-56`；同类缺口也存在于 `C06_Ranges/exercises/G2_my_transform_closure/validation/good/my_transform_view.hpp:24-25`、`:48-50`。  
Issue: 当前模板接受任意 `std::ranges::view V`，并为 `!common_range<V>` 显式返回底层 sentinel；但自定义 iterator 没有与 `sentinel_t<V>` 比较的 wrapper。同时 `iterator_category` 用 `std::conditional_t` 直接命名 `std::iterator_traits<base_iterator>::iterator_category`，在 `std::views::istream<int>` 这类 input iterator 上该成员不存在时仍会被实例化。结果是 `c06_g2::my_transform(std::views::istream<int>(input), f)` 不能满足 `std::ranges::input_range`，range-for 也无法编译。  
Evidence: `C06_Ranges/references/validation/protocol-review-r2-noncommon-probes.json` 中 `g3_noncommon_probe` 构建/运行通过，但 `g2_noncommon_probe` 构建失败；MSVC 报 `iterator_category` 不是 `std::iterator_traits<std::ranges::basic_istream_view<int,...>::_Iterator>` 成员，随后 `static_assert(std::ranges::input_range<...>)` 失败，并且 `iterator != std::default_sentinel_t` 无匹配比较。  
Risk: G2 正文在 `10-模块G-自行实现视图.md:480-485` 教授“非 common_range：end() 返回原始 sentinel”，这与 G3 刚修复的问题属于同类协议错误。当前 reference/good 和 checker 都能在 vector/prvalue/stateful/move-only closure 上通过，但无法证明 transform view 对基础 input/non-common ranges 的 view 契约。  
Fix: 像 G3 一样为 G2 增加 `sentinel` wrapper，保存 `sentinel_t<V>`，提供 `operator==(iterator, sentinel)` / 反向比较，并让 `end()` 的 non-common 分支返回该 wrapper；同时把 `iterator_category` 改成惰性/受约束 trait，避免在底层 input iterator 没有 `iterator_category` 时实例化该名字。将 main checker 和 validation/good 加入 `istream_view` 或等价 non-common input probe，确保 good 独立覆盖该路径。

## Validation

- Fresh configure/build: `protocol-review-r2-target-build.json`。MSVC 19.51 / Visual Studio 18 2026；Release+Debug 下 22 个目标全部构建成功，包含 17 个协议目标和 5 个 student 目标。
- 17/17 协议测试：`protocol-review-r2-ctest-17.json`。Release 17/17 PASS；Debug 17/17 PASS。
- Student 拒绝：`protocol-review-r2-summary.json`。五个 student executable 在 Release 和 Debug 均真实运行失败，分别命中 E1/F1/G1/G2/G3 的预期 check 消息。
- G3 原 non-common probe：`protocol-review-r2-noncommon-probes.json`。`g3_noncommon_probe` 编译成功并运行 exit 0。
- G2 同类探针：同一 JSON。`g2_noncommon_probe` 编译失败，形成上面的 HIGH issue。
- 静态扫描：`protocol-review-r2-static-scan.json`。范围内未命中 `console.log`、空 catch、`apiKey =`、`password =`、`token =`、`best effort`。

`lsp_diagnostics` 工具在当前工具面不可用；本轮用 MSVC Debug/Release 编译、CTest、student executable 和独立 CMake probe 作为可执行诊断替代。

## Recommendation

**REQUEST CHANGES / BLOCK**。修复 G2 non-common input range 契约后再复审；原 r1 四个 BLOCK 不再阻断。
