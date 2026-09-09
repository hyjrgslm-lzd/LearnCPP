# 06 模块 D：promise_type 全解

模块 A 到 C 先让你用现成的 `lazy_task`、future awaiter 和组合器写出可观察的协程程序。从本模块开始，重点切到协程类型本身：一个返回 `task<T>` 的函数，为什么只写了 `co_return`，编译器却知道要分配哪块状态、构造哪个返回对象、在哪里挂起、怎样保存异常、完成后恢复谁。

答案集中在 `promise_type`。这里的 promise 表示协程返回类型交给编译器的协议对象，和业务层 promise 只是名字相近。编译器在识别到函数体中有 `co_await`、`co_yield` 或 `co_return` 后，会根据函数签名求出 promise 类型，再把原函数改写成一段围绕 promise、coroutine state 和 awaiter 的状态机代码。

本模块的三道练习都围绕同一条主线展开：

- D-1 从零写 `lazy_task<T>`，把创建、返回对象、初始挂起、结果保存、异常保存、最终挂起、销毁全部跑通。
- D-2 只改 `initial_suspend` 的返回值，观察 lazy 与 eager 的执行时机。
- D-3 在 `final_suspend` 中返回 continuation handle，观察 symmetric transfer 表达的控制权转交。

读完本模块后，你应该能从任意一个 `task` 实现里直接指出：谁拥有协程帧，谁启动它，谁消费结果，谁在完成时恢复等待者，哪一步如果写错会变成 use-after-free 或重复 resume。

<a id="d1"></a>

## 一、协程函数调用时，返回值先于函数体产生

普通函数调用中，调用方等函数体跑完再拿到返回值：

```cpp
int f() {
    return 42;
}

int x = f();
```

协程函数的调用顺序不同：

```cpp
lazy_task<int> f() {
    co_return 42;
}

auto task = f();
```

`f()` 这个调用表达式返回时，函数体通常还没有执行到 `co_return 42`。在 lazy task 中，函数体甚至一行都没开始跑。调用方拿到的是一个拥有或引用协程帧的返回对象。它像一个启动入口和结果入口的组合，真正的计算要等之后 `start()`、`get()` 或 `co_await` 才推进。

编译器大致生成这样的控制流程：

```text
调用协程函数
  -> 确定 promise_type
  -> replacement body 开始处创建参数副本
  -> 构造 promise
  -> 调用 promise.get_return_object() 生成返回给调用方的 task
  -> co_await promise.initial_suspend()
     -> 挂起：协程函数调用返回，调用方拿到未启动 task
     -> 不挂起：继续执行协程体，直到下一次挂起或完成
```

参数副本的顺序很关键。协程被调用时，每个参数都会在 replacement body 开始处创建副本。原声明为引用类型的参数副本仍绑定到同一个外部对象；非引用参数副本是一个新变量，并由原参数直接初始化。协程体里使用这个参数名时，命名的是这份副本。参数副本的初始化先于 promise 构造；副本之间的相对初始化顺序没有固定先后。销毁时，promise 生命周期先结束，参数副本紧随其后结束。协程体内已经构造的局部对象，则按它们各自作用域和当前销毁路径清理。

这里最重要的因果关系是：`get_return_object()` 在协程体执行前调用，所以它不能返回最终结果。它只能返回一个能找到协程帧的对象。常见做法是在返回对象中保存 `std::coroutine_handle<promise_type>`。

```cpp
lazy_task get_return_object() {
    auto h = std::coroutine_handle<promise_type>::from_promise(*this);
    return lazy_task{h};
}
```

`from_promise(*this)` 安全成立，因为此时 promise 已经构造完成。协程体局部变量还没有按源码顺序开始构造；不要在返回对象构造时假设函数体已经运行。

## 二、promise_type 从哪里来

对一个协程函数，编译器通过 `std::coroutine_traits<R, Args...>::promise_type` 找 promise。`R` 是函数声明的返回类型，`Args...` 是函数形参类型，成员函数还会把隐式对象参数纳入匹配。

对下面的函数：

```cpp
lazy_task<int> compute(int x);
```

默认情况下，编译器会找：

```cpp
std::coroutine_traits<lazy_task<int>, int>::promise_type
```

如果 `lazy_task<int>` 里有嵌套的 `promise_type`，标准库默认 traits 会使用它：

```cpp
template <class T>
struct lazy_task {
    struct promise_type {
        // 协程协议写在这里。
    };
};
```

这解释了为什么返回类型能决定协程行为。同样写 `co_return 42`，返回 `lazy_task<int>` 与返回 `generator<int>` 会触发不同 promise 协议。前者需要保存最终值，后者需要在每次 `co_yield` 保存当前元素。

## 三、最小 task 的生命周期

一个教学 `lazy_task<T>` 至少需要保存三类状态：

```cpp
template <class T>
class lazy_task {
public:
    struct promise_type {
        std::optional<T> value;
        std::exception_ptr error;
        std::coroutine_handle<> continuation;

        lazy_task get_return_object();
        std::suspend_always initial_suspend() noexcept;
        auto final_suspend() noexcept;
        void return_value(T);
        void unhandled_exception() noexcept;
    };

private:
    std::coroutine_handle<promise_type> h_;
};
```

`value` 保存 `co_return expr` 的结果。`error` 保存协程体未捕获异常。`continuation` 保存正在 `co_await` 本 task 的调用者协程。

协程帧由返回对象最终负责销毁：

```cpp
~lazy_task() {
    if (h_) h_.destroy();
}
```

`destroy()` 的前提是 handle 指向的协程处于挂起状态。这个条件在教学 `lazy_task` 中靠协议保证：创建后停在 `initial_suspend`，完成后停在 `final_suspend`，中间挂起点也必须由 awaiter 明确管理。对正在执行的协程调用 `destroy()` 是未定义行为。

停在 final suspend 的协程帧可以由 owner 调用 `destroy()` 清理。对已经停在 final suspend 的协程调用 `resume()` 会进入未定义行为；完成态只保留给读取 promise 状态和销毁 frame，不再允许继续执行协程体。

移动构造要把源对象清空：

```cpp
lazy_task(lazy_task&& other) noexcept
    : h_(std::exchange(other.h_, {})) {}
```

这表达唯一所有权：同一个协程帧只能有一个 owner 负责销毁。忘记清空源对象会导致两个 task 析构时都调用 `destroy()`。

<a id="d2"></a>

## 四、`initial_suspend` 决定启动时机

`initial_suspend()` 返回一个 awaiter。编译器会对它执行一次 `co_await` 变换。教学中最常见的两个返回值是：

```cpp
std::suspend_always initial_suspend() noexcept { return {}; } // lazy
std::suspend_never  initial_suspend() noexcept { return {}; } // eager
```

`std::suspend_always` 的 `await_ready()` 返回 `false`。因此协程创建后立刻挂起，调用方拿到返回对象。协程体第一行还没执行。

`std::suspend_never` 的 `await_ready()` 返回 `true`。因此初始挂起被短路，协程体立即开始执行。它会跑到第一个真正挂起点，或者一路跑到 `co_return`。

下面的日志能直接观察差异：

```cpp
lazy_task<int> lazy_compute() {
    std::println("body");
    co_return 1;
}

std::println("before call");
auto t = lazy_compute();
std::println("after call");
auto v = t.get();
```

如果 `initial_suspend` 是 `suspend_always`，输出顺序是：

```text
before call
after call
body
```

如果改成 `suspend_never`，输出顺序变成：

```text
before call
body
after call
```

这个实验说明 lazy 与 eager 的差别是启动语义。lazy task 适合组合器：`when_all` 可以先收集多个 task，接好 continuation、stop token 和结果槽，再统一启动。eager task 适合创建即应开始的应用级操作，但它让启动发生在调用表达式内部，组合器没有机会先注入上下文。

初始挂起还有一条异常规则。标准的 replacement body 中有一个 `initial-await-resume-called` 标志，初始值为 false，并在 initial await expression 的 `await_resume()` 求值前立刻设为 true。若 `initial_suspend()` 返回的 awaiter 在到达这一步前失败，例如 awaiter 构造、`await_ready()` 或 `await_suspend()` 抛出，异常会直接传播给协程调用者。若 `initial_suspend` 的 `await_resume()` 自身抛出，此时标志已经为 true，异常会进入协程体外层 catch，再调用 `promise.unhandled_exception()`，随后进入完成路径。

## 五、`co_return` 怎样进入 promise

协程体执行：

```cpp
co_return expr;
```

对非 `void` task，会调用：

```cpp
promise.return_value(expr);
```

教学 task 通常写成：

```cpp
template <class U>
void return_value(U&& v) {
    value.emplace(std::forward<U>(v));
}
```

这一步只保存结果，不负责恢复调用者。恢复动作属于完成路径，也就是 `final_suspend`。把保存结果和通知完成分开，能让异常、取消、同步等待和嵌套 `co_await` 都走同一条完成协议。

`void` task 则提供：

```cpp
void return_void() noexcept {}
```

一个协程体使用 `co_return expr` 时需要 `return_value`；使用无表达式的 `co_return` 或自然流到末尾时需要 `return_void`。课程里把它们都列在 hook 清单中，是为了教学完整性；真实类型按返回语义选择。

## 六、未捕获异常怎样保存

协程体如果抛出未捕获异常，编译器会进入 promise 的异常通道：

```cpp
void unhandled_exception() noexcept {
    error = std::current_exception();
}
```

随后协程仍会走完成路径，进入 `final_suspend`。消费者在 `await_resume()` 或 `get()` 中检查 `error` 并重新抛出：

```cpp
T await_resume() {
    auto& p = h_.promise();
    if (p.error) std::rethrow_exception(p.error);
    return std::move(*p.value);
}
```

这种设计让成功和失败都在消费结果的位置被观察。同步函数在调用栈上抛异常；协程可能稍后、甚至在另一个线程完成，所以需要先把异常保存进协程帧。

标准允许 `unhandled_exception()` 本身抛出异常。若它抛出，协程被视为已经停在 final suspend point，这个异常传播给调用者或 resumer。教学 task 选择 `noexcept` 保存异常，让协程体失败成为 task 的一类完成结果。

<a id="d3"></a>

## 七、为什么 `final_suspend` 通常要挂起

协程体正常 `co_return` 或异常通道完成后，编译器会执行：

```cpp
co_await promise.final_suspend();
```

这个阶段有两个任务：

1. 让协程帧保持存在，直到 owner 或消费者读取结果。
2. 通知等待者继续执行。

如果 `final_suspend` 返回 `std::suspend_never`，协程完成后会自动销毁 coroutine state。对一个把结果保存在 promise 中的 task，这会让外部随后读取已销毁帧中的 `value` 或 `error`。

因此教学 task 的 `final_suspend` 返回自定义 awaiter：

```cpp
auto final_suspend() noexcept {
    struct final_awaiter {
        bool await_ready() noexcept { return false; }

        std::coroutine_handle<> await_suspend(
            std::coroutine_handle<promise_type> h) noexcept {
            auto continuation = h.promise().continuation;
            return continuation ? continuation : std::noop_coroutine();
        }

        void await_resume() noexcept {}
    };
    return final_awaiter{};
}
```

`await_ready()` 返回 `false`，保证最终挂起。`await_suspend()` 返回等待者 handle，表达 symmetric transfer。没有等待者时返回 `std::noop_coroutine()`，这是一个恢复和销毁都无副作用的协程句柄，避免返回空 handle 后被恢复造成未定义行为。

`co_await promise.final_suspend()` 还有一个硬性限制：这个表达式不得是 potentially-throwing。最终挂起用于发布完成状态和保留销毁入口；把可抛异常放在这里会让完成协议没有稳定落点。

## 八、`co_await task` 的 continuation 从哪里来

让 task 可以被另一个协程等待，需要 task 自己也是 awaitable：

```cpp
bool await_ready() const noexcept {
    return !h_ || h_.done();
}

std::coroutine_handle<> await_suspend(std::coroutine_handle<> caller) {
    h_.promise().continuation = caller;
    return h_;
}

T await_resume() {
    auto& p = h_.promise();
    if (p.error) std::rethrow_exception(p.error);
    return std::move(*p.value);
}
```

假设 `outer()` 等待 `inner()`：

```cpp
lazy_task<int> inner() { co_return 42; }

lazy_task<int> outer() {
    int v = co_await inner();
    co_return v * 2;
}
```

时序是：

```text
outer 被启动
  -> outer 遇到 co_await inner()
  -> inner task.await_suspend(outer_handle)
     -> inner.promise.continuation = outer_handle
     -> 返回 inner_handle
  -> 编译器恢复 inner
  -> inner co_return 42，进入 final_suspend
  -> final_awaiter 返回 outer_handle
  -> outer 从 await_resume 取得 42，继续执行
```

这条链解释了 continuation 的来源。全局调度器不会自动知道等待者；`await_suspend(caller)` 被调用时，当前被挂起的协程 handle 会显式交给被等待的 task。

## 九、symmetric transfer 解决哪种栈增长

`await_suspend` 可以返回 `void`、`bool` 或 `std::coroutine_handle<>`。返回 handle 时，标准层面表达的是：当前协程已经挂起，接下来恢复这个返回的 handle。

在 `final_suspend` 中直接写：

```cpp
continuation.resume();
```

意味着库代码在当前恢复调用栈里再调用下一层恢复。深层 `task` 链会形成嵌套 `.resume()`。

返回 handle：

```cpp
return continuation;
```

则把下一个恢复目标交给 `co_await` 变换。优秀实现通常能把这条路径降成不累积库层调用栈的控制转交。标准保证的是控制流语义，不保证某个编译器一定生成机器级 tail call，也不保证所有平台上机器栈恒定。D-3 的实验只声明 handle-return control transfer 能沿 continuation 链恢复，实际栈表现要用目标编译器观察。

## 十、D-1：从零写 `lazy_task<T>`

进入 [练习 D-1](exercises/D1_promise_8_hooks/README.md) 前，先把本节的生命周期图画出来：

```text
compute() 调用
  -> 分配 coroutine state
  -> 创建参数副本
  -> 构造 promise
  -> promise.get_return_object() 返回 lazy_task(handle)
  -> initial_suspend 挂起
  -> 调用方拿到 task
  -> task.get() 或 co_await task 启动
  -> 协程体执行
  -> return_value 或 unhandled_exception 保存结果
  -> final_suspend 挂起并通知 continuation
  -> task owner 析构时 destroy frame
```

D-1 的 starter 在 `main.cpp` 中让你补齐 `lazy_task<T>`。不要只把 TODO 填成能编译的代码；每写一个 hook 都在旁边标出它服务哪一步：

- `get_return_object`：把 promise 地址变成 handle，再把 handle 放进返回对象。
- `initial_suspend`：选择 lazy。
- `return_value`：保存最终值。
- `unhandled_exception`：保存异常。
- `final_suspend`：保留帧并恢复等待者。
- `operator new/delete`：参与 coroutine state 分配与释放。
- `get_return_object_on_allocation_failure`：只有 non-throwing 分配路径返回 null 时才使用。

Reference 会验证三件事：值能取出，异常能从 `get()` 重新抛出，嵌套 `co_await` 能通过 continuation 恢复。通过这些断言后，再回头解释每个断言为什么成立。

## 十一、D-2：lazy 与 eager 的一行差别

[练习 D-2](exercises/D2_eager_vs_lazy/README.md) 使用同一类 task，只切换 `initial_suspend`。实验需要看三组输出：

1. lazy：调用协程函数后，body 日志尚未出现。
2. eager：调用表达式返回前，body 日志已经出现。
3. eager + 内部 `co_await`：创建后执行到第一个挂起点，随后等待外部恢复。

`eager_start(lazy_task<T>)` 是一个中间策略：类型仍是 lazy task，但工厂函数在返回前主动 `resume()` 一次。

```cpp
template <class T>
lazy_task<T> eager_start(lazy_task<T> t) {
    t.start();
    return t;
}
```

这个函数看起来很小，语义变化很大：调用者收到的是已经启动过的 task。后续 `get()` 或 `co_await` 必须知道它不能二次启动同一帧。Capstone5 reference 因此把 `start()` 后的重复启动视为逻辑错误。

## 十二、D-3：完成时把控制权交回等待者

[练习 D-3](exercises/D3_final_suspend_symmetric/README.md) 聚焦 `final_suspend` 的最后一跳。用 1000 层嵌套链：

```cpp
task chain(int n) {
    if (n == 0) co_return 0;
    co_return 1 + co_await chain(n - 1);
}
```

当最内层完成时，它的 continuation 是上一层协程；上一层恢复后也会很快完成，再把 continuation 交给更上一层。这个过程重复 1000 次。

你的笔记要把两种写法分开：

```text
直接 resume:
  A.final_suspend.await_suspend
    -> B.resume()
       -> B.final_suspend.await_suspend
          -> C.resume()

返回 handle:
  A.final_suspend.await_suspend -> return B
  co_await 变换恢复 B
  B.final_suspend.await_suspend -> return C
```

第一种在库代码中嵌套恢复。第二种把恢复目标作为返回值交出去。Reference 验证第二种能跑通深链；它不把机器栈一定恒定写成可移植承诺。

## 本模块完成后应能说清楚

完成 D-1 到 D-3 后，你应该能回答：

1. 协程返回对象为什么能在协程体执行前返回。

   **答案解析：** 编译器识别协程后会先分配 coroutine state、创建参数副本、构造 promise，再调用 `promise.get_return_object()` 把返回对象交给调用方。若 `initial_suspend()` 返回 `std::suspend_always`，函数体会停在初始挂起点，所以返回对象先出现，`co_return` 的结果还没有产生。本模块 D-1 的 `lazy_task` 正是通过 `std::coroutine_handle<promise_type>::from_promise(*this)` 让返回对象先拿到 frame 入口。
2. `promise_type` 如何由函数返回类型和参数推导得到。

   **答案解析：** promise 类型来自 `std::coroutine_traits<R, Args...>::promise_type`，其中 `R` 是协程函数声明返回类型，`Args...` 是形参类型，成员函数还包含隐式对象参数。返回 `lazy_task<int>` 与返回 `generator<int>` 会选到不同 promise，因此同样的 `co_return` 或 `co_yield` 会进入不同协议。D-1 的源码把 `promise_type` 嵌在 `lazy_task<T>` 内，默认 traits 会直接使用这个嵌套类型。
3. `initial_suspend` 怎样决定 lazy/eager 启动时机。

   **答案解析：** `initial_suspend()` 的返回 awaiter 会在协程体前被等待。`std::suspend_always` 的 `await_ready()` 为 false，调用表达式返回时协程体尚未执行；`std::suspend_never` 的 `await_ready()` 为 true，初始挂起被短路，协程体会在调用表达式返回前开始执行。初始 await 的 `await_resume()` 求值前会设置 `initial-await-resume-called` 标志；因此它自己抛出的异常进入 promise 的 `unhandled_exception()`，而更早的 awaiter 构造、`await_ready()`、`await_suspend()` 异常直接传给调用者。D-2 的日志用 `before/after create` 与 `body` 顺序直接证明启动时机。
4. `co_return` 与未捕获异常怎样进入 promise。

   **答案解析：** 非 void `co_return expr` 会调用 `promise.return_value(expr)`，无表达式 `co_return` 或自然结束会调用 `return_void()`。协程体未捕获异常会进入 `promise.unhandled_exception()`，教学 task 把 `std::current_exception()` 存进 promise，之后由 `get()` 或 `await_resume()` 重新抛出。D-1 的 `value_task()` 和 `fail_task()` 分别覆盖这两条通道。
5. `final_suspend` 为什么保留结果消费窗口。

   **答案解析：** 结果和异常通常存放在 promise，也就是 coroutine state 内。`final_suspend()` 返回会挂起的 awaiter 后，frame 仍存在，外部 owner 才能读取 promise 状态并调用 `destroy()`；若最终挂起选择 `std::suspend_never`，完成时 frame 可能被自动销毁，随后读取结果会变成悬空访问。D-1/D-3 的 task 都让 final awaiter 返回 false 的 `await_ready()` 来保留这个窗口。
6. `co_await task` 的 continuation 在哪一步被保存。

   **答案解析：** continuation 在被等待 task 的 `await_suspend(caller)` 中保存，`caller` 就是当前挂起的等待者协程 handle。随后 `await_suspend` 返回子 task 的 handle 启动它；子 task 完成时，`final_suspend` 再返回保存的 continuation。D-1 的 `outer()` 等待 `inner()`，`inner.promise().continuation` 保存的就是 `outer` 的恢复入口。
7. `await_suspend` 返回 handle 表达怎样的控制权转交，以及哪些机器级优化只是实现观察。

   **答案解析：** 返回 `std::coroutine_handle<>` 表达当前协程挂起后，下一个要恢复的是这个 handle；这就是标准层面的 symmetric transfer 控制流语义。编译器是否把它降成机器级 tail call、机器栈是否恒定、汇编长什么样，都属于目标工具链的实现观察。D-3 的 `chain(1000)` 只证明 handle-return 链路能恢复到最外层，不承诺所有平台的栈表现相同。

继续阅读：[07 模块 E：awaitable 三层与 co_await 变换](07-模块E-awaitable三层与co_await变换.md)。
