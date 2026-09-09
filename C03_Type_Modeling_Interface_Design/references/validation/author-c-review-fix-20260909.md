# 作者 C：callables/contracts 审查返修记录，2026-09-09

来源：`references/validation/reviews/callables-contracts-independent-review.md` 的两个 MEDIUM 阻断。

## 修复 1：L12 `std::move_only_function` 签名限定实验不足

修改范围：

- `chapters/12-callable-objects-and-type-erasure.md`
- `exercises/L12_callables/README.md`
- `exercises/L12_callables/callables_observation.cpp`

新增作者侧真实标准实验：

- `std::move_only_function<int() &>`：`static_assert(std::is_invocable_v<Wrapper&>)`，`static_assert(!std::is_invocable_v<Wrapper&&>)`，并从左值 wrapper 实际调用。
- `std::move_only_function<int() const noexcept>`：`static_assert(std::is_nothrow_invocable_v<const Wrapper&>)`，并从 `const&` 实际调用。
- 不运行空 `move_only_function`。

验证：只重跑 L12 Debug/Release，保存为 `author-c-l12-r2-*-20260909-*.json`，全部 PASS。

## 修复 2：C++29 virtual contracts 推导不足

修改范围：

- `chapters/14-contracts-hardening-and-interface-boundaries.md`

新增正文推导：

- 明确 C++26 contracts MVP 不含 virtual `pre/post`，P3097R3 是 C++29 增量。
- 解释 caller-facing assertions 与 callee-facing assertions。
- 写出顺序：caller-facing pre → callee-facing pre → body → callee-facing post → caller-facing post。
- 用 `Base::f pre(x > 0)` / `Derived::f pre(x < 100)` 分析 `Base&` 调用 `f(150)` 的替换风险。
- 明确同一函数是否重复检查必须按 P3097R3 固定 wording 核对，不能写成自动继承所有基类契约。

验证：纯文档修复，按要求未重跑 L14。来源静态复核自 P3097R3：caller/callee 五步顺序见 https://wg21.link/p3097r3。
