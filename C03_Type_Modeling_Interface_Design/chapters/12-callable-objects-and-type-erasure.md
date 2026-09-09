# 12：可调用对象与函数包装器

前几章已经把接口分成值、可选值、状态空间、错误通道和多态对象。本章处理另一类接口：把“以后要调用的一段行为”作为参数、成员或返回值传递。行为本身也有类型、所有权、生命周期和失败语义；如果只把它们统称为 callback，很容易把拥有和借用混在一起。

C++ 里的可调用对象包括普通函数、函数指针、lambda、函数对象、成员函数指针、成员数据指针，以及被 `std::reference_wrapper` 包住的对象。`std::invoke` 是统一入口：它把这些形式规约成一次调用表达式。教学上应先建立这个共同模型，再比较 `std::function`、`std::move_only_function`、`std::copyable_function` 和 `std::function_ref`。包装器不是“更高级的函数指针”，而是不同所有权和约束的接口。

`std::invoke` 的价值在于接口可以接收“可调用”概念，而不是手写一组重载。成员函数指针需要对象，成员数据指针需要对象，函数对象需要 `operator()`；`std::invoke` 把这些调用形式统一成同一个表达式。`std::reference_wrapper` 的价值是显式借用：它让本来会复制的泛型算法携带引用。它不延长对象生命期，因此适合局部算法组合，不适合跨线程、异步保存或对象外长期缓存。

`std::function<R(Args...)>` 是 C++11 的拥有型类型擦除包装器。它要求目标可复制，包装器自身可复制；空调用会抛 `std::bad_function_call`。这一点和函数指针不同：空函数指针调用是未定义行为，`std::function` 为空时有定义的异常通道。代价是类型擦除、可能分配、间接调用，以及 const 语义的历史缺陷：`std::function::operator() const` 不保证被包装目标按 const 调用。也就是说，一个 `const std::function<void()>` 仍可能调用目标的非 const `operator()` 并修改目标内部状态。接口若把 const 当成“回调无副作用”就错了。

`std::move_only_function` 是 C++23 的拥有型包装器，目标只需要可移动，不要求可复制。它补齐了 move-only 捕获，例如捕获 `std::unique_ptr` 的 lambda。它还把函数类型签名的 cv/ref/noexcept 限定纳入接口：`std::move_only_function<int() &>` 表示只能在左值包装器上调用；`std::move_only_function<int() noexcept>` 表示调用表达式承诺不抛。它的空调用是强前置条件：标准不要求像 `std::function` 那样抛 `bad_function_call`。课程和实验不能通过故意空调用来“观察行为”，因为那会进入不应依赖的前置条件违反区。

C++26 又把同一设计空间拆得更清楚。`std::copyable_function` 仍是拥有型，但修正 const/ref/noexcept 语义，目标也要满足对应构造约束；它适合“我要保存一份可复制的行为，而且签名限定要可信”的场景。`std::function_ref` 是非拥有型借用视图，通常只作为函数参数使用；它避免分配和所有权转移，但不能逃出被借用 callable 的生命周期。`function_ref` 不是轻量 `std::function`，它是“临时借用某段行为”。

比较这些类型时先问三个问题：谁拥有 callable？包装器能否复制？空状态和失败语义是什么？`std::function` 适合保存可复制行为并需要空调用异常；`move_only_function` 适合保存 move-only 行为并把空状态作为调用前置条件；`copyable_function` 适合 C++26 后需要 copyable 且 const/ref/noexcept 约束准确的接口；`function_ref` 适合只在调用栈内借用行为。函数模板或 `auto` 参数仍是零成本首选，只有需要类型擦除、存储、ABI 边界或运行时组合时才使用包装器。

MSVC STL 的源码导读入口固定在本机 `D:\VisualStudio2026\Installed\VC\Tools\MSVC\14.51.36231\include\functional`，头文件 SHA 在 `references/validation/capabilities/local-msvc-stl-inputs-20260909.md`。该文件中，`std::function` 路径可从 `_Func_class`、`_Func_impl_no_alloc`、`_Copy`、`_Delete_this` 和 `_Xbad_function_call` 读起：复制经 `_Copy`，销毁经 `_Delete_this`，空调用进入 `_Xbad_function_call`。`std::move_only_function` 路径可从 `_Function_storage_mode`、`_Function_base`、`_Function_call` 和 `move_only_function` 读起：小对象和大对象走不同存储模式，移动/销毁经表驱动分派。这里的 SBO 大小是实现细节，只能说本机 MSVC STL 以 `_Small_object_num_ptrs` 派生存储空间，不能硬断所有标准库实现都有相同阈值。

练习 L12 是观察型。Part 1 用 `std::invoke` 调用成员函数、成员数据和函数对象，说明“可调用”不是单一语法。Part 2 用 `std::reference_wrapper` 修改原对象，说明它是显式借用而非复制。Part 3 比较 `std::function` 的空调用异常和 const 调用历史缺陷。Part 4 用 `std::move_only_function` 保存 move-only 捕获，并只在非空时调用。Part 5 使用真实标准类型检查 `std::move_only_function<int() &>` 只能由左值 wrapper 调用，并检查 `std::move_only_function<int() const noexcept>` 可从 `const&` 调用且满足 `std::is_nothrow_invocable_v`。C++26 的 `copyable_function/function_ref` 不在 L12 重复探测，统一链接到 F01 的真实前沿示例。

完整解析：先用 `std::invoke` 把所有调用形式压到同一抽象层，再选择包装器。若函数只立即调用一次，模板参数或 `std::invoke` 足够；若要保存可复制任务，用 `std::function` 或 C++26 `copyable_function`；若任务捕获独占资源，用 `move_only_function`；若只是借用上层传入的策略，用 `function_ref`。生命周期错误通常不是包装器本身造成，而是把借用接口保存到了 callable 之后；空调用错误则取决于类型契约，`std::function` 抛异常，`move_only_function` 是调用前置条件。
