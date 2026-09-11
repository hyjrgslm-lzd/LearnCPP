# 08 模块 F：协程帧与 allocator

D 模块让你知道 promise 什么时候被调用，E 模块让你知道 `co_await` 怎样挂起和恢复。现在要回答一个更底层的问题：协程暂停以后，那些参数、局部变量、异常状态、当前执行位置放在哪里。

答案是 coroutine state，常被叫作协程帧。普通函数返回时，调用栈帧消失；协程可以在调用函数已经返回后继续存在，所以跨挂起点仍要存活的状态必须放在一个独立对象里。这个对象通常动态分配，也可能被实现通过 HALO 省略分配。

本模块的三道练习分别观察这三个层次：

- F-1 用编译器 dump 看协程帧里通常有哪些区域。
- F-2 用 promise `operator new/delete` 接管帧分配，观察 size 与配对释放。
- F-3 用 HALO 诊断区分语义保证和实现优化。

<a id="f1"></a>

## 一、为什么普通栈帧不够

看下面的协程：

```cpp
task<int> compute(std::string name) {
    int prefix = static_cast<int>(name.size());
    co_await event{};
    co_return prefix + 1;
}
```

调用 `compute("abc")` 时，调用表达式可能很快返回一个 `task<int>`。协程体稍后才从 `event{}` 后继续执行。继续执行时仍要读 `name` 和 `prefix`。因此它们不能只存在于调用 `compute` 时的普通栈帧里。

实现通常会创建一个 coroutine state，里面保存：

```text
coroutine state
  promise
  参数副本或参数引用
  当前 resume point
  跨挂起点存活的局部对象
  awaiter 临时对象
  异常和销毁路径簿记
```

不同编译器的字段名字、顺序、padding 和拆分方式不同。课程要求你观察自己的工具链输出，不要求背某个固定布局。

标准层面的顺序是：replacement body 一开始先创建参数副本，随后构造 promise。非引用参数名在协程体内指向这份副本；引用参数副本继续绑定到原对象。销毁时 promise 生命周期先结束，参数副本随后结束。协程体里已经构造的局部对象按作用域和当前销毁路径清理，所以一个停在中间挂起点的 frame 被 `destroy()` 时，只会析构已构造且仍活跃的局部对象。

## 二、哪些变量会进入 frame

一个变量是否需要放进 frame，核心看它是否跨挂起点仍需存活。

```cpp
task<void> example(int a) {
    int before = a + 1;
    use(before);

    std::string across = make_string();
    co_await event{};

    use(across);
}
```

`before` 如果只在挂起前使用，优化后可能不出现在 frame 中。`across` 在挂起后还要使用，必须能从恢复点继续访问。对非平凡对象，编译器还要记录它是否已经构造，以便 destroy 时只析构活跃对象。

参数按值传递时，参数副本通常属于 coroutine state。这样调用方离开后，协程仍拥有自己的值：

```cpp
task<std::size_t> size_after_wait(std::string text) {
    co_await event{};
    co_return text.size();
}
```

参数按引用传递时，frame 保存的是引用关系：

```cpp
task<std::size_t> bad(std::string_view text) {
    co_await event{};
    co_return text.size();
}
```

如果 `text` 指向的字符串在恢复前已经销毁，协程恢复后访问就是悬空。F-1 让你用布局观察建立这个直觉；J 模块会专门诊断这类生命周期错误。

## 三、resume point 是协程能继续的地址标记

协程恢复时，会从上次挂起点之后继续。实现需要知道上次停在哪个挂起点。

```cpp
task<void> steps() {
    before();
    co_await a{};
    middle();
    co_await b{};
    after();
}
```

第一次恢复从 `initial_suspend` 后进入 `before()`。遇到 `a` 挂起时，frame 记录下一个恢复点。第二次恢复从 `a.await_resume()` 后继续，执行 `middle()`。遇到 `b` 再记录新的恢复点。

这个状态常表现为 resume index、state number 或一组编译器私有标记。它必须随 coroutine state 一起保存，因为协程挂起后没有普通调用栈告诉你下一条源码语句在哪里。

## 四、F-1：观察 frame 布局

进入 [练习 F-1](exercises/F1_frame_layout/README.md) 时，先读 `main.cpp` 中三个协程：

- `observed`：参数、字符串、多个局部变量和两个挂起点。
- `observed_complex`：加入 `vector`、`unique_ptr` 等非平凡对象。
- `observed_minimal`：极简对照。

你要用一个编译器 dump 找到 frame 结构，并标注：

```text
+-----------------------------+
| promise                     |
+-----------------------------+
| resume point / state        |
+-----------------------------+
| 参数副本或引用              |
+-----------------------------+
| 跨挂起点局部对象            |
+-----------------------------+
| awaiter 临时对象与销毁簿记  |
+-----------------------------+
```

这张图只表达你观察到的实现。标准没有规定这些字段的布局顺序。写报告时用类似“MSVC 当前输出显示”“GCC dump 中可见”这样的句子，避免把 QoI 当语言规则。

Reference 用 RAII marker 断言三个事实：`initial_suspend` 前协程体局部未构造；跨挂起点局部在暂停期间保持存活；最终按作用域销毁。它验证的是生命周期语义，dump 验证的是当前实现怎样保存这些状态。

<a id="f2"></a>

## 五、协程帧默认怎样分配

标准允许实现为 coroutine state 调用 allocation function。若需要分配且不能省略，编译器会按 promise 相关规则查找 `operator new`。

查找从 promise type 的作用域开始。只要在 promise scope 中找到了任意 `operator new` 声明，就先做一次带协程参数的重载决议：第一个实参是 `std::size_t` 的请求大小，后续实参是协程的原始参数 lvalue，保留它们的原始类型和 cv 限定。如果没有可行函数，再只用 `std::size_t` 做一次重载决议。promise scope 中没有找到声明时，才到全局作用域查找；全局查找只传请求大小这个实参。

最简单的 promise 不写分配函数时，通常走全局 `::operator new(size_t)`。

```cpp
struct promise_type {
    static void* operator new(std::size_t n) {
        return ::operator new(n);
    }

    static void operator delete(void* p, std::size_t n) noexcept {
        ::operator delete(p);
    }
};
```

传给 `operator new` 的 `n` 是实现计算出的 coroutine state 大小。不同协程体、优化级别和编译器会产生不同大小。不要硬编码“某个 task 帧一定 128 字节”。allocator 应该按传入 size 工作。

`operator delete` 要和分配路径配对。编译器销毁 coroutine state 时会先在 promise type 作用域查找 deallocation function，找不到再查全局作用域。如果同时找到只接收指针的 usual deallocation function 和接收指针加 `std::size_t` 的 usual deallocation function，会选择带 size 的版本；否则选择只接收指针的版本。实现教学 allocator 时应把 size 打印出来，确认分配和释放次数配对。

## 六、allocation failure 的特殊路径

`get_return_object_on_allocation_failure` 是 allocation failure 的条件路径。它只在 promise scope 中找到这个名字，并且分配函数采用不抛异常、失败返回 null 的形式时才有意义。

简化形状如下：

```cpp
struct promise_type {
    static void* operator new(std::size_t n) noexcept {
        return pool_try_allocate(n); // 失败返回 nullptr
    }

    static task get_return_object_on_allocation_failure() noexcept {
        return task{}; // 空 task 或失败态 task
    }
};
```

当 promise scope 中找到 `get_return_object_on_allocation_failure` 时，用于取得 coroutine state 的 allocation function 被假定可用 null 表示失败；如果最终选中的是全局 allocation function，会使用 `::operator new(size_t, std::nothrow_t)` 形式。此时选中的 allocation function 必须是 non-throwing。若它返回 null，控制权回到协程调用者，返回值来自 `promise_type::get_return_object_on_allocation_failure()`。

如果分配函数按普通 throwing `operator new` 风格抛 `std::bad_alloc`，这条失败态返回对象路径不会负责处理那个异常。F-2 的 reference 覆盖 aligned allocation、delete 配对、nothrow allocation failure 到空 task，正是为了把这两个路径分开。

## 七、F-2：promise allocator 实验

[练习 F-2](exercises/F2_promise_allocator/README.md) 的 pool 很朴素：它只统计和分配，不追求生产级回收。这样做的目的很明确：把“协程帧真的通过 promise `operator new` 分配”跑出来。

实验观察顺序：

1. 创建一个只含一个挂起点的 `tiny_coro`，记录分配 size。
2. 创建一个含更多局部变量和多个挂起点的 `wider_coro`，记录 size 变化。
3. 正常销毁 task，确认 `operator delete` 计数增长。
4. 触发 nothrow 失败路径，确认返回空 task，并避免访问不存在的 frame。

性能对比只在确认统计正确后做。不要在分配函数里频繁 flush 输出；I/O 会盖掉 allocator 的真实开销。

<a id="f3"></a>

## 八、HALO：实现可以省略动态分配

HALO 指 Heap Allocation eLision Optimization。它处理的是 coroutine state 的动态分配能否省略。标准允许实现省略一次动态分配，前提是实现能证明协程状态生命周期严格嵌套在调用方生命周期内，并且状态大小在调用点可知。

常见有利条件：

- 协程创建与消费路径对优化器可见。
- 返回对象或内部 handle 没有逃逸到更长生命周期。
- 恢复和销毁路径都能被证明在调用方生命周期内结束。

常见破坏条件：

- 把返回对象存进全局或长生命周期容器。
- 暴露并保存 `std::coroutine_handle<>`。
- 将等待者、operation state 或 receiver 分配到不透明堆对象中。
- 跨编译单元或虚调用让优化器看不清生命周期。

HALO 不改变语言语义。协程代码仍按“可能有 coroutine state”来写。触发后，当前实现可以把状态放在调用方栈帧、寄存器或其他等价位置；标准不规定具体存放位置。

## 九、ready 短路与 HALO 的边界

E-3 已经证明：`await_ready()==true` 时跳过 `await_suspend()`。这是单个 await-expression 的控制流语义。

HALO 关注另一件事：整个 coroutine state 的动态分配是否消失。一个协程内部所有 awaiter 都 ready，也不自动保证 frame allocation 被省略。反过来，一个有真实挂起点的局部 generator，在特定形状下也可能让分配被优化掉。

记录结果时分三栏：

| 观察项 | 证明方式 | 说明 |
| --- | --- | --- |
| ready 短路 | 计数器证明 `await_suspend` 未调用 | 标准语义 |
| 挂起分支消除 | 汇编或 IR 中分支消失 | 实现优化 |
| frame allocation elision | remark、dump、汇编中无 `operator new` | 实现优化 |

这三栏分清楚，后面读性能报告就不会把耗时差异误写成标准承诺。

## 十、F-3：诊断 HALO 触发与失败

[练习 F-3](exercises/F3_halo_diagnose/README.md) 有两个版本：

- 版本 A：局部创建并消费 generator，返回对象和 handle 不逃逸。
- 版本 B：把对象地址写到全局，制造候选逃逸路径；它不保证阻止分配消除。

先跑程序确认两版功能结果一致，再用编译器证据看动态分配是否被省略。Clang 用 `-Rpass=coroutine-elide`，GCC 看 coro dump 或分配调用，MSVC 在 Release 下查反汇编中的分配点。

微基准只能辅助判断。时间接近可能是两边都 elide、都未 elide，或成本被其他工作掩盖；时间差大也可能来自循环化简等其他优化。应沿实际 consumer 调用路径检查 remark、IR、dump 或汇编，不能搜索全模块分配符号就下结论。

本轮 Clang 18.1.3、`-O2` 无插桩输出中，两个 consumer 均没有动态分配并已化简为求和循环，独立导出的 `range_values` 却仍分配 48 字节。没有 elide remark；这证明实际路径消除了动态分配，但不指定优化 pass，也不能将两版耗时差归因于 HALO 有无。完整证据与干扰边界见[F3 最终实验判读](references/validation/c09-refresh/performance/final/analysis.md)。

## 本模块完成后应能说清楚

完成 F-1 到 F-3 后，你应该能回答：

1. 为什么协程需要独立 coroutine state。

   **答案解析：** 协程调用表达式可能在函数体完成前返回，之后还会从挂起点继续执行。普通调用栈帧已经离开，跨挂起点仍要使用的参数副本、局部对象、awaiter 和恢复位置必须存到独立 coroutine state 中。F-1 的 `observed` 用两个挂起点让这些状态可被 dump 或生命周期日志观察。
2. 哪些参数和局部变量会进入 frame。

   **答案解析：** 需要跨挂起点继续存活的状态通常进入 frame：按值参数副本、挂起后还要读的局部对象、当前 awaiter 临时和销毁簿记。只在挂起前使用的临时或局部可能被优化掉；引用参数保存的是引用关系，指向对象的寿命仍由调用方负责。F-1 的 `std::string param_c` 是按值副本，挂起后仍可安全使用。
3. resume point 为什么必须随 frame 保存。

   **答案解析：** 协程恢复时要从上次挂起点之后继续，而此时普通函数调用栈已经不能告诉实现下一条源码语句在哪里。实现通常用 resume index、state number 或私有 label 记录恢复位置，并随 coroutine state 保存。多挂起点的 `steps()` 或 F-1 的 `observed` 都需要这类状态。
4. `coroutine_handle::resume()` 与 `destroy()` 的使用前提是什么。

   **答案解析：** `resume()` 只能用于处于挂起状态且尚未完成到不可恢复阶段的协程；对已经停在 final suspend 的完成态继续 resume 是未定义行为。`destroy()` 也要求 handle 指向挂起的协程，它会按当前状态析构已构造且仍活跃的对象并释放 coroutine state。教学 task 靠 initial/final suspend 与 owner 协议维持这些前提。
5. promise `operator new/delete` 如何接管帧分配与释放。

   **答案解析：** 需要动态分配 coroutine state 时，编译器先在 promise type 作用域查找 allocation function；可行时调用 promise 的 `operator new`，销毁 frame 时再按 promise scope 查找对应 `operator delete`。传入 size 是当前实现计算出的 frame 大小，不能硬编码。F-2 的 `pool_task::promise_type` 记录 alloc/free 次数和地址对齐来证明这条路径被调用。
6. `get_return_object_on_allocation_failure` 的触发条件是什么。

   **答案解析：** 只有 promise scope 找到 `get_return_object_on_allocation_failure`，且选中的 allocation function 是 non-throwing、失败用 null 表示时，才会走这个 hook。它返回一个失败态或空返回对象，调用方不能继续访问不存在的 frame。F-2 reference 用 `noexcept operator new` 返回 null，再断言得到空 task。
7. HALO 需要哪些可观察证据，哪些只是性能推断。

   **答案解析：** HALO 的可靠证据来自编译器 remark、IR/dump、反汇编中分配调用消失，或自定义分配 hook 没有被调用且代码形状可解释。耗时差异只能提示可能发生了优化，因为优化级别、I/O、内联和缓存都会影响微基准。F-3 要同时记录工具链版本、优化级别和诊断来源。

继续阅读：[09 模块 G：symmetric_transfer 与高级 task](09-模块G-symmetric_transfer与高级task.md)。
