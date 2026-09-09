# C03 Author C 独立审查：L12-L15 / F01 callables-contracts

审查时间：2026-09-09  
审查角色：非作者独立审查  
结论：ITERATE

## 绑定源码 SHA

本报告绑定作者冻结清单：

- `references/validation/author-c-source-sha256-20260909.json`
- `references/validation/capabilities/source-sha256-r2-20260909.json`

关键条目：

| 范围 | SHA256 |
| --- | --- |
| `chapters/12-callable-objects-and-type-erasure.md` | `15B1E02FC49ECBAA50F0AB2036E54897A74B94C4226B5E7907879EDD490FC230` |
| `chapters/13-indirect-values-and-polymorphic-ownership.md` | `092F8585C68289CFE0759CA011934C1535C553BB4DBA86DFF4860B01DC2D914A` |
| `chapters/14-contracts-hardening-and-interface-boundaries.md` | `FC6AC1C0E50BACEEE8EDAB4CC3608156FD13D44A37ED5C324966D48ED314C392` |
| `chapters/15-interface-evolution-and-compatibility.md` | `C8FD711B1EECA018C9E85EF6AFDD350BEAC245CEB2885BAACE5064CA35B52D9E` |
| `exercises/F01_frontier/capabilities/c23_move_only_function.cpp` | `EB3F7416AEC1458B2460A9B3700DBC2F5C300C9A18FE6B3AF07A76B6615AC100` |
| `exercises/F01_frontier/capabilities/c26_contracts.cpp` | `4D6776917D939819CA018DD213DEEE180D6AD9CBA4617F1B767E2E0D30FCF928` |
| `exercises/F01_frontier/capabilities/c29_virtual_contracts.cpp` | `5AFEF2CEE0D0274CE242982B9C710CD4E0C548EF1D224929EB18074F5176D590` |
| `references/standards-and-implementations.md` | `08D24EAB63B53D2069B197BB0A7B17C19C654700791515DB54D35DE7A05034D0` |

## 独立验证

新增只读审查证据目录：`references/validation/reviews/callables-contracts-evidence/`。该目录不修改作者源码。

运行结果：

| 检查 | 结果 | 证据 |
| --- | --- | --- |
| 独立 `move_only_function` qualifier 探针 | PASS，1/1 | `callables-contracts-evidence/probe-release-check.log` |
| L12 Release | PASS，1/1 | `callables-contracts-evidence/L12-release-check.log` |
| L13 Release | PASS，1/1 | `callables-contracts-evidence/L13-release-check.log` |
| L14 Release | PASS，1/1 | `callables-contracts-evidence/L14-release-check.log` |
| L15 Release | PASS，3/3，含 bad rejected | `callables-contracts-evidence/L15-release-check.log` |
| F01 selected Release ctest | PASS/skip 符合预期，7 项 0 fail；C26/C29 缺宏项 SKIP 77 | `callables-contracts-evidence/F01-selected-ctest.log` |

`lsp_diagnostics`/`ast_grep_search` 当前工具面未暴露；本轮以源码审读、`rg` 定位、独立 CMake/CTest 编译运行和既有 MSVC 能力日志作为替代验证。

## 问题

### [MEDIUM] F01/L12 对 `std::move_only_function` “cv/ref/noexcept 签名限定”的验证覆盖不足

证据：

- 正文明确要求 `std::move_only_function` 把 cv/ref/noexcept 纳入接口，并举 `std::move_only_function<int() &>` 与 `std::move_only_function<int() noexcept>`：`chapters/12-callable-objects-and-type-erasure.md:11`。
- 标准索引声称主线可运行并讲 `move-only、cv/ref/noexcept 签名、空调用前置条件`：`references/standards-and-implementations.md:25`。
- L12 实验只构造 `std::move_only_function<int()>` 并验证 move-only 捕获：`exercises/L12_callables/callables_observation.cpp:69`、`:70`、`:74`。
- F01 的 C++23 真实标准探针只覆盖 copy 禁止和 `int() const` 调用：`exercises/F01_frontier/capabilities/c23_move_only_function.cpp:18`、`:20`、`:22`，未覆盖 ref-qualified 与 noexcept-qualified 调用语义。

影响：

读者能读到规则，但课程当前没有作者侧可复现实验证明 `&` 限定会限制右值调用、`noexcept` 签名会进入 nothrow 调用契约。该点属于本章 callable wrapper 选择的核心差异，不是风格问题。

修正建议：

在 F01 `c23_move_only_function.cpp` 或 L12 observation 中加入最小真实标准检查：

- `std::move_only_function<int() &>`：`static_assert(std::is_invocable_v<Wrapper&>)` 和 `static_assert(!std::is_invocable_v<Wrapper&&>)`。
- `std::move_only_function<int() const noexcept>`：`static_assert(std::is_nothrow_invocable_v<const Wrapper&>)`，并从 `const&` 调用一次。
- 保持空 `move_only_function` 不运行；只检查调用前先非空或用非空构造。

我在独立证据中已用 `callables-contracts-evidence/callable-qualifier-probe/callable_qualifier_probe.cpp:25`-`:34` 验证该修正形态可在本机 C++23 编译运行。

### [MEDIUM] C++29 virtual contracts 只给了概述，缺少 P3097 的核心 caller-facing/callee-facing 组合规则

证据：

- L14 正文说明 P3097R3 解决虚函数、override 与契约组合，并提醒“派生类更严格”不安全：`chapters/14-contracts-hardening-and-interface-boundaries.md:11`。但这里没有写出 P3097 的实际组合规则。
- F01 C29 示例只是宏满足时编译一个合法动态派发路径：`exercises/F01_frontier/capabilities/c29_virtual_contracts.cpp:5`-`:18`、`:34`-`:36`；README 也限定为“增量语法”：`exercises/F01_frontier/README.md:22`、`:40`。
- P3097R3 的核心规则是：虚调用时同时评估 caller-facing assertions（调用表达式静态选中的函数）和 callee-facing assertions（动态派发最终调用的 overrider），顺序为 caller-facing pre、callee-facing pre、函数体、callee-facing post、caller-facing post。该机制解释为什么“基类更宽、派生更窄”会在基类引用调用时暴露替换问题。

影响：

用户要求 C26/C29 contracts “具体机制与虚契约组合别只概述”。当前 C++26 contracts 部分已经给出 `pre/post/contract_assert`、checking mode、violation handling 和 input validation/hardening 边界；C++29 virtual contracts 缺少同等级的核心推导。读者会知道“有规则”，但不知道具体哪两组断言参与、何时参与、为什么会影响替换原则。

修正建议：

扩展 `chapters/14-contracts-hardening-and-interface-boundaries.md` 的 virtual contracts 段落，至少加入：

- C++26 MVP 不允许 virtual `pre/post`，P3097R3 是 C++29 增量。
- direct non-virtual call、function pointer / member pointer、virtual call 三种场景中 caller-facing 与 callee-facing assertions 的差异。
- 虚调用的五步顺序：caller-facing pre → callee-facing pre → body → callee-facing post → caller-facing post。
- 一个小型 `Base::f pre(x > 0)` / `Derived::f pre(x < 100)` 例子，解释 `Base&` 调用 `f(150)` 与 `Derived&` 直接调用的差异；不需要触发 violation，也不需要本机编译支持。

## 已核过且未发现阻断的问题

- `std::function` 空调用与 const 历史缺陷讲法正确：正文 `chapters/12-callable-objects-and-type-erasure.md:9`，实验 `exercises/L12_callables/callables_observation.cpp:50`-`:62`。
- `std::reference_wrapper` 借用但不延长生命周期讲法正确：正文 `chapters/12-callable-objects-and-type-erasure.md:7`，实验 `exercises/L12_callables/callables_observation.cpp:40`-`:45`。
- L13 明确把教学 `clone_value` 与 C++26 `std::indirect/std::polymorphic` 分开：正文 `chapters/13-indirect-values-and-polymorphic-ownership.md:11`、`:13`，README `exercises/L13_indirect_values/README.md:3`。
- L13 覆盖深复制、const 传播、clone 失败强保证和移动后空状态：正文 `chapters/13-indirect-values-and-polymorphic-ownership.md:15`，实验 `exercises/L13_indirect_values/README.md:5`-`:11`。
- L14 没把 hardening 冒充 input validation：正文 `chapters/14-contracts-hardening-and-interface-boundaries.md:3`、`:5`、`:13`，F01 `hardening_info.cpp` 只记录宏并避免违反前置条件：`exercises/F01_frontier/capabilities/hardening_info.cpp:9`-`:25`。
- L15 确实有 v2 行为模型，不是固定字符串冒充：v2 默认值在 `exercises/L15_evolution/checks/v2_engine.hpp:12`-`:16`；reference 旧入口显式保持 `1000/legacy`，新入口显式 `250/compact`：`exercises/L15_evolution/src/reference/compat_adapter.hpp:12`-`:24`；bad 反例直接使用 v2 默认实参：`exercises/L15_evolution/validation/bad/compat_adapter.hpp:12`-`:24`；checker 精确检查旧默认、新入口和 ABI 风险模型：`exercises/L15_evolution/checks/evolution_checks.cpp:20`-`:21`、`:34`-`:35`、`:41`-`:42`。
- L15 明确 source/behavior/ABI 证据边界，不声称真实跨编译器 ABI 或旧二进制加载：`chapters/15-interface-evolution-and-compatibility.md:3`、`:15`、`:17`，README `exercises/L15_evolution/README.md:27`、`:45`。
- F01 与标准索引把规范归属、本机宏、SKIP 和能力证明分开：`references/standards-and-implementations.md:5`、`:48`，capability 汇总 `references/validation/capabilities/capability-results-20260909.md:28`-`:31`、`:67`。

## Verdict

ITERATE。没有发现 C12-C15 作者代码运行失败、安全绕路、L15 伪实现或 ABI 过度声称；但上述两处是用户点名要求的教学/验证覆盖缺口，应返修后再批准。
