# C06 Ranges 协议 r4 非作者窄复验

Verdict: **APPROVE（窄范围）**

本轮复验绑定当前最终文档和相关输入，只覆盖上一轮唯一文档 BLOCK：G2 教学正文/README 是否已同步到可编译、可照抄的最终实现；同时复看 E2/E3 后续加入的 `<check.hpp>` 运行断言。不重复全课矩阵。

## Scope

- `C06_Ranges/10-模块G-自行实现视图.md`
- `C06_Ranges/exercises/G2_my_transform_closure/README.md`
- `C06_Ranges/exercises/G2_my_transform_closure/src/reference/my_transform_view.hpp`
- `C06_Ranges/exercises/G2_my_transform_closure/validation/good/my_transform_view.hpp`
- `C06_Ranges/references/validation/ranges-protocols-r5/doc-probe/{CMakeLists.txt,g2_doc_probe.cpp}`
- `C06_Ranges/references/validation/ranges-protocols-r5/probe-location.md`
- `C06_Ranges/exercises/E2_niebloid/main.cpp`
- `C06_Ranges/exercises/E3_cpo_tagdispatch_compare/main.cpp`

## 原 r3 文档 BLOCK 复验

通过。G2 最终正文块已经不是关键词补丁，而是可编译完整块：

- `10-模块G-自行实现视图.md:341-349` 定义 `iterator_category_for` 惰性 helper。
- `10-模块G-自行实现视图.md:356`、`:395`、`:398-407` 定义并接入 `sentinel` wrapper。
- `10-模块G-自行实现视图.md:416-421` 的 `end()` 对 non-common range 返回 wrapper sentinel。
- `10-模块G-自行实现视图.md:439-449` 提供 `const&` / `&&` closure overload，`&&` 分支移动 `function_` 支持 move-only callable。
- `10-模块G-自行实现视图.md:504-517` 在文档块内直接验证 move-only closure、`std::views::istream<int>` input/non-common、move-only iterator、post++ 和 sentinel stop。
- `G2_my_transform_closure/README.md:54-89` 同步 sentinel wrapper 任务说明，明确不能直接透传 raw sentinel。
- `G2_my_transform_closure/README.md:91-108` 同步 `const&` / `&&` 双重载与 move-only callable 要求。
- `G2_my_transform_closure/README.md:194` 同步 input/non-common、移动底层 iterator、惰性 `iterator_category` 的验收边界。

## E2/E3 `<check.hpp>` 运行断言复看

通过。E2/E3 不再只是打印 demo：

- `E2_niebloid/main.cpp:18` 引入 `<check.hpp>`；`:40`、`:76`、`:83`、`:114-115`、`:134`、`:145` 对 sort object、`template<auto>` wrapper、comparator forwarding、projection、legacy adapter 做运行断言。
- `E3_cpo_tagdispatch_compare/main.cpp:15` 引入 `<check.hpp>`；`:192`、`:196`、`:205`、`:210`、`:214` 对 member begin、member size、range-for、tag_invoke dispatch、ADL fallback 做运行断言。

## Validation

- `protocol-review-r4-replay.json` / `protocol-review-r4-summary.json`：
  - 使用新源路径 `C06_Ranges/references/validation/ranges-protocols-r5/doc-probe` 独立重放 doc-probe。
  - `g2_doc_probe` Release 1/1 PASS；Debug 1/1 PASS。
  - E2/E3 fresh Release 2/2 PASS；Debug 2/2 PASS。
  - E2/E3 executable 单独运行均 exit 0，并输出预期 demo 文本。
- `protocol-review-r4-source-inspection.json`：记录最终文档/source/probe 输入 SHA256，并用 grep 确认 G2 最终块含 helper、sentinel、move-only closure、`std::views::istream<int>` 和 input_range 检查；E2/E3 有 `<check.hpp>` 与运行断言；范围内未命中 hardcoded secret、空 catch、`console.log`、`apiKey =`、`password =`、`token =`、`best effort`。

`lsp_diagnostics` 工具在当前工具面不可用；本轮用 MSVC Debug/Release 编译、CTest、独立 doc-probe replay、E2/E3 executable 和静态 grep/hash 作为诊断替代。

## Issues

无阻断问题。

## Recommendation

**APPROVE（窄范围）**。上一轮 G2 文档 BLOCK 已解除；本结论不覆盖 root 正在跑的整课 Release/Debug/ASan 矩阵。
