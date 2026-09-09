# L12：可调用对象与函数包装器

先读 `../../chapters/12-callable-objects-and-type-erasure.md`。本练习是观察型，不提供 Student。运行程序会检查 `std::invoke`、`std::reference_wrapper`、`std::function` 和 `std::move_only_function` 的核心差异。

Part 1：观察 `std::invoke` 调用成员函数、成员数据和函数对象。重点是“可调用”是语义集合，不是单一语法。

Part 2：观察 `std::reference_wrapper`。它修改原对象，证明这是借用；它不能延长对象生命期。

Part 3：观察 `std::function`。空调用抛 `std::bad_function_call`；`const std::function` 仍可能调用目标的非 const `operator()`，不能把 wrapper 的 const 当成目标无副作用。

Part 4：观察 `std::move_only_function`。程序保存捕获 `std::unique_ptr` 的 move-only lambda，只在非空状态调用；不故意触发空调用强前置条件。

Part 5：观察 `std::move_only_function` 的签名限定。`std::move_only_function<int() &>` 只能从左值 wrapper 调用；`std::move_only_function<int() const noexcept>` 可从 `const&` 调用，且 `std::is_nothrow_invocable_v` 为真。

运行：

```powershell
cmake -S C03_Type_Modeling_Interface_Design/exercises/L12_callables -B build/author-c-l12 -G "Visual Studio 18 2026" -A x64
cmake --build build/author-c-l12 --config Release
ctest --test-dir build/author-c-l12 -C Release --output-on-failure
```

解析：如果只是立即调用，模板参数加 `std::invoke` 通常比类型擦除更简单。需要保存可复制行为时用 `std::function`，需要保存 move-only 行为时用 `std::move_only_function`。`move_only_function` 的函数类型不是装饰文字：`&` 会进入可调用性约束，`const noexcept` 会进入 const 调用和不抛调用契约。C++26 `std::copyable_function` 和 `std::function_ref` 的真实标准接口由 `F01_frontier` 单独探测。
