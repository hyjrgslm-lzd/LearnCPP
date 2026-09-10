# 练习 L03：转发包装与 CTAD

先读 [03. 转发与 CTAD](../../chapters/03-forwarding-ctad.md)。本题只编辑 `src/student/forwarding_tools.hpp`。

Part 1：实现 `invoke_preserving(F&&, Args&&...)`。它要调用传入 callable，完美转发 callable 和参数，返回类型用 `decltype(auto)`，异常规格跟随真实调用表达式。

Part 2：实现 `holder<T>`。它拥有一个 `T value`，支持 `holder h{42}` 的 CTAD。

Part 3：实现 `make_list_holder(std::initializer_list<T>)`，返回 `holder<std::vector<T>>`。裸花括号不是普通转发参数，本题给它独立入口。

checker 会检查左值/右值参数落到不同重载、返回引用身份、`noexcept` 传播、CTAD 推导值类型，以及 initializer_list 入口拥有元素。`validation/bad` 把参数统一 `std::move`，必须被拒绝。
