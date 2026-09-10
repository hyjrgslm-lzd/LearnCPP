# C04 reflection frontier r2 delivery contract alignment

日期：2026-09-10  
身份：非作者审查补充  
当前 Git HEAD：`f261bea559d6722c31135fb2d3589be52fe958ed`  
前置报告：`C04_Generic_CompileTime_Reflection/references/reviews/frontier-review-r2.md`

## 判定 A：已确认交付范围

结论：通过。

本轮已确认的交付范围是“本机验证 + 明确未测项”：完整前沿正文、真实源码、实验规格、本机 MSVC 不支持 `<meta>` 时明确 SKIP；不要求安装新工具链、远端编译或在真实反射实现上运行主体。按该契约，r2 已满足。

依据：

- r2 审查已关闭 6 个原阻断，无新增 r2 阻断。
- OFF 路径无测试，符合默认关闭前沿实验的预期。
- ON 路径 12 个 probe 全部构建并由 CTest 记录为 SKIP，且 stdout 保留 `header/macro/body`，没有冒称 PASS。
- `F01_declared_failure_control` 受控项证明：已声明能力并进入主体后，返回 1 会被 CTest 报为 Failed，不会被 SKIP77 吞掉。
- 文档已区分 N5050/N5054/N5055、P4101R1 DR、P3385R8 未采纳提案和本机实现状态。
- LSP 不可用已在 r2 报告中说明，并以 MSVC 构建/CTest原始证据、源码审读、静态文本扫描和官方 WG21 依据替代；这符合当前确认的交付契约。

所以 final 质量报告可以写：C04 F01 frontier 的“本机验证 + 明确未测项”交付范围通过；不能写成“真实 C++26/C++29 反射主体已执行通过”。

## 判定 B：真实 C++26/C++29 主体运行验证

结论：未验证。

原因不是新问题，而是已确认的环境边界：本机 MSVC 19.51 没有 `<meta>`，也没有对应 reflection/annotations/P4101/P3385 实现能力。ON 路径的 12 项结果是 SKIP 证据，不是主体 PASS 证据。

未验证内容包括：

- P2996/N5050 reflection query 主体。
- splicing 与 `template for` 相关主体。
- P3394 annotations 查询和 `extract<T>` 主体。
- P3491 `std::define_static_*` 真实实现主体。
- P4101R1 consteval-only values 正反例在真实支持编译器上的编译接受/拒绝。
- P3385R8 attributes reflection 提案 API 主体。
- C++29 template-name pack indexing 主体。

这些未验证项应在 final 质量报告中保留为“真实支持编译器待测”，不能作为失败项扣回判定 A，也不能被表述成已运行通过。

## 本补充未改变的事实

- 未新增 build/test。
- 未修改被审源码。
- 未覆盖 `frontier-review-r2.md`。
- 未改写作者或 r2 实验证据。
- 未触碰 P1、C05 或全局文件。
