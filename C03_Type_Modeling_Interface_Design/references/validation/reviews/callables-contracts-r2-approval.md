# C03 Author C r2 独立复验：callables/contracts

审查时间：2026-09-09  
审查角色：非作者独立 reviewer  
结论：APPROVE

## 复验范围

本轮只核对上一轮两个 MEDIUM 阻断的最小受影响范围：

- L12 `std::move_only_function` cv/ref/noexcept 签名限定实验。
- L14 C++26/C++29 contracts 分界与 P3097R3 virtual contracts 推导。
- `references/validation/author-c-review-fix-20260909.md` 与 `references/validation/author-c-source-sha256-r2-20260909.json`。

未重复 L13/L15/F01 全矩阵；未改作者源码。

## 绑定 SHA

采用 `references/validation/author-c-source-sha256-r2-20260909.json` 的 r2 绑定：

- `chapters/12-callable-objects-and-type-erasure.md`：`4061C027B505FC1296132479E5FCAE595C45263E0C143F105322B982A259F252`（见 `author-c-source-sha256-r2-20260909.json:15`-`:17`）
- `chapters/14-contracts-hardening-and-interface-boundaries.md`：`43F61A06BC967B03EAC24F8A04CD6DEC3CC6F600455BBBD1123125D0F9F36CF4`（见 `author-c-source-sha256-r2-20260909.json:25`-`:27`）
- `exercises/L12_callables/README.md`：`13B184917B9AD6492A37FF3514F4A78C83F5B7B9BCBAA7734F85DE918EADF062`（见 `author-c-source-sha256-r2-20260909.json:40`-`:42`）
- `exercises/L12_callables/callables_observation.cpp`：`F9810BA8C0A86520651F416360DA03DB9A9FD05237869C61CC7541F41F9C5E2D`（见 `author-c-source-sha256-r2-20260909.json:45`-`:47`）

## 关闭项 1：L12 callable qualifier 实验

上一轮问题：正文讲 `std::move_only_function` 的 `&` 与 `noexcept` 签名语义，但作者侧没有可复现实验。

复验结论：已关闭。

证据：

- 修复记录声明新增 `std::move_only_function<int() &>` 的 `std::is_invocable_v<Wrapper&>` / `!std::is_invocable_v<Wrapper&&>`，并新增 `std::move_only_function<int() const noexcept>` 的 `std::is_nothrow_invocable_v<const Wrapper&>` 与 `const&` 实际调用；同时不运行空 `move_only_function`：`references/validation/author-c-review-fix-20260909.md:13`-`:17`。
- 章节把 Part 5 写入教学闭环，明确真实标准类型检查 `int() &` 和 `int() const noexcept`：`chapters/12-callable-objects-and-type-erasure.md:19`。
- README 新增 Part 5，说明 `&` 限定与 `const noexcept` 语义：`exercises/L12_callables/README.md:13`，解析强调函数类型不是装饰文字：`exercises/L12_callables/README.md:23`。
- 实验源码实际构造 `std::move_only_function<int() &>`，有左值/右值可调用性正反 `static_assert`，并只在非空左值上调用：`exercises/L12_callables/callables_observation.cpp:73`-`:80`。
- 实验源码实际构造 `std::move_only_function<int() const noexcept>`，验证 `const&` 可调用和 `std::is_nothrow_invocable_v`，并从 `const&` 调用：`exercises/L12_callables/callables_observation.cpp:82`-`:87`。
- `main()` 确实调用新增检查：`exercises/L12_callables/callables_observation.cpp:111`-`:117`。

独立 MSVC 复验：

- Configure 使用 `Visual Studio 18 2026` / MSVC 19.51：`reviews/callables-contracts-evidence/r2-l12-msvc-configure.json:2`-`:16`，verdict PASS：`:20`-`:27`。
- Release build 编译 `callables_observation.cpp` 并生成 `L12_callables_observation.exe`：`reviews/callables-contracts-evidence/r2-l12-msvc-build-release.json:2`-`:12`，verdict PASS：`:16`-`:23`。
- Release ctest `L12_callables_observation` 1/1 passed，`100% tests passed`，verdict PASS：`reviews/callables-contracts-evidence/r2-l12-msvc-ctest-release.json:10`-`:24`。

## 关闭项 2：L14 P3097R3 virtual contracts 推导

上一轮问题：C++29 virtual contracts 只概述，缺 P3097R3 caller-facing/callee-facing 核心组合规则。

复验结论：已关闭。

证据：

- 修复记录声明 L14 是纯文档修复，未重跑 L14；这是合理边界：`references/validation/author-c-review-fix-20260909.md:21`-`:35`。
- 正文现在明确 C++26 contracts MVP 不含 virtual function 的 `pre/post`，P3097R3 是 C++29 增量：`chapters/14-contracts-hardening-and-interface-boundaries.md:11`。
- 正文解释 caller-facing assertions 是调用表达式静态选中的函数，callee-facing assertions 是动态派发最终调用的 final overrider，并列出顺序：caller-facing pre → callee-facing pre → body → callee-facing post → caller-facing post：`chapters/14-contracts-hardening-and-interface-boundaries.md:13`。
- 正文用 `Base::f pre(x > 0)` / `Derived::f pre(x < 100)` 推导 `Base& b = d; b.f(150)` 的替换风险，并对比 `Derived&` 直接调用的归属：`chapters/14-contracts-hardening-and-interface-boundaries.md:15`。
- 正文补充同一最终函数不应讲成无意义重复检查，并提示显式继承/复用基类断言必须按固定提案 wording 核对，避免写成自动继承所有基类契约：`chapters/14-contracts-hardening-and-interface-boundaries.md:17`。

官方规则核对：

- P3097R3 明确 C++26 最后阶段去掉了 virtual `pre/post`，该提案是为 C++29 重新推进 virtual contracts；见 https://wg21.link/p3097r3 第 0 页/第 2-3 页。
- P3097R3 明确 overriding function 的 assertions 独立于 overridden function；虚调用会评估静态选中函数和 final overrider 两组 assertions；顺序为 caller-facing pre、callee-facing pre、body、callee-facing post、caller-facing post；见 https://wg21.link/p3097r3 第 0 页/第 4-5 页。

## 工具与验证边界

- `lsp_diagnostics` 与 `ast_grep_search` 当前工具面不可用；本轮没有伪称运行 LSP。
- 替代验证为：源码逐行核对、P3097R3 官方文档核对、MSVC L12 Release configure/build/ctest。
- L14 是纯文档修复，本轮未重跑 L14 编译；未变 L13/L15/F01 未重复矩阵。

## Verdict

APPROVE。上一轮两个 MEDIUM 阻断均已按目标范围关闭；未发现新的 HIGH/MEDIUM 问题。
