# C04 A04/A05 类型管线与表达式模板独立审查

日期：2026-09-10

审查结论：APPROVE

原因：A04 与 A05 的主体实现、good/bad、diagnostic、Student 初态和独立盲审迁移都能对上课程目标。A05 现在明确公开包装默认是 potentially-throwing，不承诺 `noexcept`，不捕获或吞异常；条件 `noexcept` 归 11/20 章。本轮没有发现需要阻塞的代码、规格或安全问题。

## 阶段一：盲审迁移

只读输入：

- `chapters/21-type-pipelines.md`
- `chapters/22-expression-templates.md`
- `chapters/08-type-lists.md`
- `chapters/03-forwarding-ctad.md`
- `chapters/07-packs-nttp.md`
- `exercises/A04_type_pipelines/README.md`
- `exercises/A04_type_pipelines/checks/type_pipeline_checks.cpp`
- `exercises/A04_type_pipelines/src/student/type_pipelines.hpp`
- `exercises/A05_expression_templates/README.md`
- `exercises/A05_expression_templates/checks/expression_template_checks.cpp`
- `exercises/A05_expression_templates/src/student/expression_templates.hpp`

未读内容：阶段一未读 `src/reference`、`validation/good`、`validation/bad`、`references/reviews/revision-pipelines-author.md`。

冻结点：

- Git HEAD：`f261bea`
- `21-type-pipelines.md` SHA256：`0D57DC77880EB77CFB7EF56F6B1084F940B9247CB3016E2B99F38702AF3A3D72`
- `22-expression-templates.md` SHA256：阶段一输入为 `EE9CCFE496798F62BFA3F1B3CD4019DB8E2451486C0CA7626F441ADA7FEBED3D`；澄清后复验版本为 `B703520AA00A7D4A0656B009EDA90AB27F8A0C73AA91DF6F6DCAC8A7D7E4A3A5`
- `08-type-lists.md` SHA256：`54AD32E84C5435A158B3DCF810FB9C18480FFCEAB0E1BB55C4EE9E0D6B3FF479`
- `03-forwarding-ctad.md` SHA256：`286F00B0C7EF9E9A1329A9A0B93A10EA8F9F4F727AA69A997555AA9EC9D55CF6`
- `07-packs-nttp.md` SHA256：`CCBA5E27AFCFA0E786F548B4E8362CB18392928DF2994DACAA2ADD9C54D2BBC6`
- A04 README/check/student SHA256：`6AAEFF1C0CB7FC51F57968CDC66187299C6023B0C1D9292737FBD0278F012DC0` / `CEF71AF4763A83D187C1EF22708DB65F64FECDE45017A40E61389FF796B344C0` / `33A2772708FBA8E27BA6505484BF2784D3235D9162C6621C367E2A053D2417BD`
- A05 README/check/student SHA256：阶段一输入为 `C1137C222A95C1EB471F0A7819454C6C0168961B68A70A56434C11550D825CE7` / `D6309B66193245DC0A573F5EABF4E9273FF614AC694E4A12256C9FDD1EEE2D68` / `AF4B9D208C263AD67B297D8925C6BEA7521C2496384941D08A7C454E0B3A5F82`；澄清后复验 README/check 为 `C7860D2F1C1AE8C433F444C2F98803C2F319A1125862429AD95366B511D3B236` / `4F08B8864921EC9A1087D79B7AAC26BB60EA6149AB2C0B5F061DC74438D53128`
- 独立实现：`references/validation/revision-20260910/pipelines-blind/a05_blind_expression_core.cpp`
- 独立实现 SHA256：`28C6BE3DC85D5C2E1CA3A2DAC6DFDEC5C70C21BAA23D520ACBD8367EC28E5B52`

盲审结果：

- A05 正文足以独立完成核心迁移：固定 `double vec<N>`、表达式节点按 `operator[]` 延迟求值、左值借用、右值拥有、`eval` 物化、`assign` 先物化再写回。
- 独立程序使用陌生输入 `vec<5>`，eager 参考值为 `{55.0, 28.5, 37.75, 75.0, 40.0}`，并覆盖左值借用、右值拥有、`reverse` alias assign、异尺寸加法 false boundary。
- 盲审实现 Release 构建通过并运行退出码 0。
- 盲审 ASan 构建通过；首次直接运行退出 `-1073741515`，归因为 ASan DLL 未在 PATH。临时加入 `D:\VisualStudio2026\Installed\VC\Tools\MSVC\14.51.36231\bin\Hostx64\x64` 后运行退出码 0。

阶段一发现的未讲先用/建议补强，已在澄清后处理：

- `chapters/22-expression-templates.md` 使用 `template<expr L, expr R>`，但没有完整给出 `expr` concept 如何把 `vec`、`add_expr`、`scale_expr`、`reverse_expr` 纳入表达式集合。Reference/good 通过 `is_expr_v` 特化完成；建议正文补一小段，避免学生只能从 checker 反推。

## 阶段二：作者材料与源码对照

A04 结论：通过。

证据：

- `chapters/21-type-pipelines.md:7-36` 明确 `zip` 等长约束和 false boundary；`validation/good/type_pipelines.hpp:38-47` 与 `src/reference/type_pipelines.hpp:33-41` 实现一致。
- `chapters/21-type-pipelines.md:49-100` 明确根必须是 `type_list`、内部仅 `type_list` 递归展开；`validation/good/type_pipelines.hpp:49-59` 实现一致。
- `chapters/21-type-pipelines.md:102-158` 明确 `cartesian_product_t<>` 单位元、任一输入空列表为空结果、left-major 顺序；`validation/good/type_pipelines.hpp:61-75` 实现一致。
- `chapters/21-type-pipelines.md:196-217` 明确 `variant` 先转 `type_list` 再复用 product；`validation/good/type_pipelines.hpp:77-80` 实现一致。
- `chapters/21-type-pipelines.md:229-301` 明确 value/error/stopped 规则、`exception_ptr` 通道、`void -> value_sig<>`、不可调用 false boundary；`validation/good/type_pipelines.hpp:87-109` 实现一致。
- 陌生输入 probe `a04_probe_good` 构建与运行通过，覆盖 `LvalueOnly&` 的实际调用类型、throwing long 分支追加 `error_sig<std::exception_ptr>`、原 `error_sig<std::runtime_error>`/`stopped_sig` 保留、`RvalueOnly&` false boundary。

A05 结论：通过。

证据：

- `chapters/22-expression-templates.md:38-76` 明确表达式节点保存策略；`validation/good/expression_templates.hpp:22-24`、`:28-31`、`:39-42`、`:50-52` 实现左值借用/右值拥有和递归求值。
- `chapters/22-expression-templates.md:77-101` 明确大小约束在运算符入口拒绝；`validation/good/expression_templates.hpp:58-60` 和 `validation/diagnostics/size_mismatch.cpp:1-5` 覆盖 `vec<2> + vec<3>`。
- `chapters/22-expression-templates.md:148-196` 明确 `eval` 返回拥有结果、`assign` 先物化再写回；`validation/good/expression_templates.hpp:65-74` 实现一致。
- `validation/bad/expression_templates.hpp:73-74` 只把 `assign` 改成直接写回；负例焦点干净，`A05_expression_templates_validation_bad_rejected` 通过。
- `chapters/22-expression-templates.md:200` 与 `README.md:9` 明确公开包装默认 potentially-throwing，不承诺 `noexcept`，不捕获或吞异常；Reference/good 未声明 `noexcept` 与文档一致。
- `checks/expression_template_checks.cpp:14-27` 覆盖 `eval(reverse(vec<0>{}))` 零访问边界、嵌套临时、左值借用、eval 拥有结果、reverse alias assign。
- `a05_noexcept_probe_good` 构建失败：`static_assert(noexcept(...))` 全部失败。该 probe 保留为被否定的测试假设，证明“公开包装全为 nothrow”不是当前验收条件。

## 验证记录

- A04 Debug：`ctest --test-dir .../pb-a04 -C Debug --output-on-failure`，7/7 passed。
- A04 Release：`ctest --test-dir .../pb-a04 -C Release --output-on-failure`，7/7 passed。
- A04 Student Debug：构建通过；CTest 1/5 failed，失败信息 `check failed: flatten recursively expands nested type_list elements`，符合初态失败目标。
- A05 Debug：澄清后复跑 `ctest --test-dir .../pb-a05 -C Debug --output-on-failure`，6/6 passed。
- A05 Release：`ctest --test-dir .../pb-a05 -C Release --output-on-failure`，6/6 passed。
- A05 Student Debug：澄清后复跑构建通过；CTest 1/4 failed，失败信息 `check failed: empty expression evaluation performs no element access`，符合初态失败目标。
- A05 ASan Debug：澄清后复跑 `ctest --test-dir .../pb-a05-asan -C Debug --output-on-failure`，6/6 passed。
- 静态模式扫查：`rg` 检查空 catch、硬编码 key/password/token、宽 fallback、best effort、静默 `return type_list<>`，A04/A05 作者目录未命中。`sg`/ast-grep 不在当前环境，未运行。

## Issue

无。

已关闭的审查假设：我原先把“检查 `noexcept`”解释成“公开包装应全部 `noexcept`”，并用 `a05_noexcept_probe_good` 做了全 true 断言。澄清后确认这是错误验收口径；当前课程只需避免错误承诺，文档已明确默认 potentially-throwing，源码未声明 `noexcept`，两者一致。

## Recommendation

APPROVE。A04/A05 合同、负例、诊断、Student 初态、A05 空向量边界和 ASan 证据可接受。
