# 07 模块 E：awaitable 三层与 co_await 变换

模块 D 解释了协程对象怎样创建、启动、完成和销毁。本模块只盯住一件事：协程体中每一个 `co_await expr` 到底被编译器改写成什么。

`co_await` 的表面含义很像“暂停等待一个东西”。真实规则更精确：先把表达式转换成 awaiter，然后调用 `await_ready()` 判断是否需要挂起；需要挂起时调用 `await_suspend(handle)`，它决定当前协程挂起后控制权流向哪里；恢复后调用 `await_resume()` 取得结果或抛出错误。

这一层是后面所有高级设施的共同地基。`task` 通过它等待另一个 task，future awaiter 通过它接回调，`when_all` 通过它收束子任务，stdexec 桥接通过它把 sender 转成 awaitable。

本模块练习安排如下：

- E-1 验证 `co_await` 查找：promise 的 `await_transform`、`operator co_await`、对象本身作为 awaiter。
- E-2 验证 `await_suspend` 三种返回值：`void`、`bool`、`std::coroutine_handle<>`。
- E-3 验证 ready 短路：`await_ready()==true` 时跳过 `await_suspend`，再把这件事与 HALO 区分开。

<a id="e1"></a>

## 一、awaitable 与 awaiter 先分开

下面这个类型已经能直接作为 awaiter：

```cpp
struct ready_int {
    bool await_ready() noexcept { return true; }
    void await_suspend(std::coroutine_handle<>) noexcept {}
    int await_resume() noexcept { return 42; }
};
```

它有三方法，`co_await ready_int{}` 可以直接使用它。执行顺序是：

```text
await_ready() -> true
await_resume() -> 42
```

`await_suspend()` 不会被求值。

但 `co_await expr` 的 `expr` 不一定已经是 awaiter。它可能先经过一层转换：

```cpp
struct source {
    ready_int operator co_await() noexcept {
        return {};
    }
};
```

此时 `source` 是 awaitable，`operator co_await()` 的返回值才是 awaiter。更进一步，当前协程的 promise 还能用 `await_transform` 抢在前面改写表达式：

```cpp
struct promise_type {
    ready_int await_transform(source) noexcept {
        return ready_int{};
    }
};
```

所以读协程库时要问三个问题：原始表达式是什么、它先被 promise 改写了吗、最终三方法属于哪个对象。

## 二、普通 `co_await expr` 的三层查找

对普通 await-expression，标准规则可以按下面的决策树理解：

```text
co_await expr
  |
  v
当前协程 promise scope 中是否找到 await_transform 名字？
  |
  +-- 找到：
  |      尝试调用 promise.await_transform(expr)
  |      调用良构 -> 得到 awaitable
  |      调用不良构 -> 程序 ill-formed
  |
  +-- 没找到：
         expr 自身作为 awaitable

awaitable
  |
  v
对 operator co_await 做重载决议
  |
  +-- 有唯一最佳候选：调用它，返回 awaiter
  +-- 候选歧义：程序 ill-formed
  +-- 无可行候选：awaitable 自身就是 awaiter

awaiter
  |
  v
await_ready / await_suspend / await_resume
```

一个容易写错的点是 `await_transform`。普通 `co_await` 会先查找 promise scope 的 `await_transform` 名字；一旦找到，表达式形如 `promise.await_transform(expr)`。如果这个调用不可行，程序就是 ill-formed，编译器不会继续尝试原始表达式。

这意味着一个过宽的模板：

```cpp
template <class T>
auto await_transform(T&& x);
```

会拦截当前协程体内几乎所有 `co_await` 表达式。它可以用来统一注入调度、日志或 sender 桥接，也可能意外盖掉你本来希望走 ADL `operator co_await` 的类型。E-1 要用日志把这个优先级亲手跑出来。

## 三、`operator co_await` 是普通重载决议

`operator co_await` 候选可能来自成员函数，也可能来自 ADL 找到的自由函数：

```cpp
struct member_source {
    ready_int operator co_await() noexcept;
};

namespace ext {
    struct free_source {};
    ready_int operator co_await(free_source) noexcept;
}
```

编译器不会先无条件选成员，再无条件选自由函数。它会形成候选集合并进行重载决议。只有没有可行 `operator co_await` 时，awaitable 自身才需要提供三方法。

E-1 的两个 task 对照很关键：

- `transform_task` 的 promise 定义 `await_transform`，所以 A/B 会先被 promise 改写。
- `plain_task` 的 promise 没有 `await_transform`，所以 B 走成员 `operator co_await`，C 走 ADL 自由函数。

预期现象要由日志证明：哪个 wrapper 的 `await_suspend` 被调用，最终 `await_resume` 返回的值是多少。

<a id="e2"></a>

## 四、awaiter 三方法组成一次等待

得到 awaiter 后，执行顺序固定为：

```cpp
auto&& awaiter = /* 查找得到的 awaiter */;
if (!awaiter.await_ready()) {
    // 当前协程准备挂起
    awaiter.await_suspend(current_handle);
}
auto result = awaiter.await_resume();
```

真实改写还要处理异常、返回类型、生命周期和恢复路径。先抓住三个方法的职责：

- `await_ready()`：快速路径判断。返回 `true` 时，本次 `co_await` 不挂起。
- `await_suspend(current_handle)`：当前协程将要挂起时调用。它拿到当前协程 handle，可以保存到事件循环、回调、线程、另一个协程或共享状态中。
- `await_resume()`：协程恢复后取结果。成功值、异常重新抛出、停止状态转换通常都在这里发生。

`await_suspend` 被调用前，当前协程已经处于可被恢复的挂起状态。这个规则允许 awaiter 把 handle 交给其他线程。它也带来一个危险窗口：如果别的线程在 `await_suspend` 返回前恢复并跑完当前协程，awaiter 对象所在的协程帧可能已经继续变化甚至被销毁。生产级 awaiter 必须有同步完成握手。Capstone5 的 `stdexec_awaitable` reference 用 atomic phase 处理这个窗口。

## 五、`await_suspend` 返回 `void`

返回 `void` 表示当前协程保持挂起，控制权回到恢复当前协程的那一方。当前协程何时继续，取决于 awaiter 是否把 handle 保存到某处并在之后调用 `resume()`。

```cpp
struct event_awaiter {
    event& e;

    bool await_ready() noexcept { return e.ready(); }

    void await_suspend(std::coroutine_handle<> h) {
        e.set_waiter(h);
    }

    int await_resume() {
        return e.value();
    }
};
```

这就是回调、事件循环、IO 完成通知的常见形状。`await_suspend` 只登记等待者，不直接产生结果。完成事件到来时，外部代码恢复保存的 handle，协程回到 `await_resume()`。

如果返回 `void` 却没有保存 handle，协程会永久挂起。E-2 的 `awaiter_void` 通过静态变量保存 handle，再由 `main` 手动恢复，专门让你看到这条责任链。

## 六、`await_suspend` 返回 `bool`

返回 `bool` 让 awaiter 在最后一刻决定是否保持挂起：

```cpp
bool await_suspend(std::coroutine_handle<> h) {
    if (result_became_ready()) {
        return false; // 不挂起，当前协程立即继续到 await_resume
    }
    save_waiter(h);
    return true;      // 保持挂起，等待外部 resume
}
```

`true` 与 `void` 模式一样，表示保持挂起。`false` 表示取消本次挂起，直接执行 `await_resume()`。

它和 `await_ready()==true` 的差别在于时机。`await_ready()` 没有当前协程 handle；`await_suspend()` 已经拿到 handle，也可能已经把自己接入某个共享结构。许多同步完成 race 正好发生在这一步，所以 `bool false` 是“我刚准备挂起，但发现结果已经到了，于是立即继续”的表达。

## 七、`await_suspend` 返回 `coroutine_handle<>`

返回 handle 表示 symmetric transfer：当前协程挂起后，接下来恢复返回的那个协程。

```cpp
std::coroutine_handle<> await_suspend(std::coroutine_handle<> caller) noexcept {
    child.promise().continuation = caller;
    return child;
}
```

`task` 的 `await_suspend` 常用这一模式。调用者协程挂起，把自己的 handle 作为 continuation 存入子 task，然后返回子 task 的 handle。编译器生成的 `co_await` 变换接过这个 handle，恢复子 task。子 task 在 `final_suspend` 中再返回 continuation，让调用者继续。

标准保证的是控制权转交语义。它给实现提供了避免库代码层层 `.resume()` 的表达方式；机器级尾调用、恒定栈空间、具体汇编形状都要按工具链实测。

## 八、异常从 `await_suspend` 抛出时怎样走

`await_suspend` 可以抛异常，除非它声明了 `noexcept`。如果它抛出，当前协程会被恢复，异常在当前协程中从 `co_await` 表达式处重新抛出。换句话说，异常会交回协程体自己的 `try/catch` 或 promise 的 `unhandled_exception()`。

这条规则很重要：awaiter 在登记等待失败时，不应该留下半注册的 handle。最小做法是在修改共享状态前完成可能抛出的操作，或在异常路径撤销注册。很多教学 awaiter 把 `await_suspend` 写成 `noexcept`，正是为了把这类失败从练习焦点中拿掉。

<a id="e3"></a>

## 九、ready 短路与 trivial awaitable

`await_ready()` 返回 `true` 时，标准保证不求值 `await_suspend()`，随后求值 `await_resume()`。

```cpp
struct always_ready {
    bool await_ready() noexcept { return true; }
    void await_suspend(std::coroutine_handle<>) noexcept {
        ++suspend_calls;
    }
    int await_resume() noexcept { return 7; }
};
```

`co_await always_ready{}` 的结果是 `7`，`suspend_calls` 保持 `0`。注意三方法仍要在语义分析上良构。运行时不会进入 `await_suspend`，不表示这个成员可以不存在或签名随便写。

`std::suspend_never` 就是最简单的 trivial awaitable：ready 永远为真。`std::suspend_always` 则 ready 永远为假。

优化器如果能证明 `await_ready()` 恒为真，可能把判断和挂起分支都删掉。这是 as-if 优化。HALO 处理的是 coroutine state 分配能否省略，ready 短路处理的是某个 await-expression 是否进入挂起路径。二者可以同时出现，也可以单独出现。

## 十、E-1：把查找顺序跑出来

进入 [练习 E-1](exercises/E1_co_await_lookup/README.md) 时，先按下面方式读 starter：

1. 找 `awaitable_A/B/C`，确认它们各自本来能走哪条路径。
2. 找 `transform_task::promise_type`，确认它对 A/B 定义了 `await_transform`。
3. 找 `plain_task::promise_type`，确认它没有 `await_transform`。
4. 跑程序，看 A/B/C 的日志和返回值。

当 `transform_task` 里 `co_await awaitable_B{}` 返回 `200` 时，含义是 promise 拦截先于 B 的成员 `operator co_await`。当 `plain_task` 里同一类型返回 `20` 时，含义是没有 promise 拦截后，成员 `operator co_await` 生效。

你要在笔记里画的决策树必须写上“不良构”的路径，尤其是 await_transform 找到但不可调用的情况。

## 十一、E-2：把三种 `await_suspend` 返回值跑出来

[练习 E-2](exercises/E2_await_suspend_three/README.md) 的目标是观察控制权。每个 awaiter 都要独立观察：

- `void`：保存当前 handle，`main` 稍后手动 `resume()`。
- `bool true`：保存当前 handle，保持挂起，外部恢复。
- `bool false`：已经进入 `await_suspend`，随后立即继续执行 `await_resume()`。
- `coroutine_handle<>`：返回目标 handle，控制权交给目标协程。

看到日志后，把每条输出前面的执行者标出来：是当前协程体、awaiter、外部 `main`、目标协程，还是恢复后的 `await_resume()`。只要能标清楚执行者，三种返回值的差异就不会混。

## 十二、E-3：ready 短路和 HALO 分开验证

[练习 E-3](exercises/E3_trivial_awaitable/README.md) 要你用计数器证明：

```text
await_ready -> true
await_suspend 调用次数 == 0
await_resume 返回值被使用
```

这条是标准语义。

然后再看编译器输出：Clang remark、GCC dump、MSVC 反汇编、Godbolt 汇编都属于实现观察。若 `call operator new` 消失，说明这个工具链在这个代码形状上可能消除了 frame allocation。若挂起分支消失，说明优化器证明了 ready 快速路径。把这两条证据分开记录。

## 本模块完成后应能说清楚

完成 E-1 到 E-3 后，你应该能回答：

1. `co_await expr` 如何从原始表达式变成 awaiter。

   **答案解析：** 普通 `co_await expr` 先在当前 promise scope 查 `await_transform`；找到并可调用时，先用它把原始表达式改写成 awaitable。随后对 awaitable 做 `operator co_await` 重载决议，若无可行 operator，awaitable 自身必须提供 `await_ready/await_suspend/await_resume`。E-1 的 A/B/C 日志把这三层路径拆开观察。
2. `await_transform` 找到但不可调用时为什么是 ill-formed。

   **答案解析：** 标准规则是一旦在 promise scope 找到 `await_transform` 名字，就形成 `promise.await_transform(expr)` 这条路径；调用不可行时程序不良构，不再回退到原始表达式或 `operator co_await`。这会让过宽或过窄的 `await_transform` 影响整个协程体内的普通 await-expression。E-1 要求在决策树里写出这条失败路径。
3. 成员和 ADL `operator co_await` 怎样进入同一次重载决议。

   **答案解析：** awaitable 转 awaiter 时，成员 `operator co_await` 和 ADL 找到的自由 `operator co_await` 会组成候选集合，再按普通重载决议选择唯一最佳候选。没有可行候选时，awaitable 自身才作为 awaiter 使用；候选歧义时程序不良构。E-1 的 `plain_task` 中，B 走成员路径，C 走 ADL 自由函数路径。
4. `await_ready`、`await_suspend`、`await_resume` 分别承担什么职责。

   **答案解析：** `await_ready()` 判断是否需要挂起，返回 true 时跳过挂起流程。`await_suspend(current_handle)` 在当前协程已经可被恢复的挂起状态下登记 handle、转交控制或决定立即继续。`await_resume()` 在协程恢复后交付值、重抛异常或转换 stopped 状态；E-2/E-3 的 awaiter 日志分别观察这些调用点。
5. `await_suspend` 返回 `void`、`bool`、`coroutine_handle<>` 的控制流差异。

   **答案解析：** 返回 `void` 表示当前协程保持挂起，后续恢复责任在 awaiter 保存的外部 handle。返回 `bool` 时，true 表示保持挂起，false 表示取消本次挂起并立即进入 `await_resume()`。返回 `coroutine_handle<>` 表示挂起当前协程后恢复返回的目标 handle，D-3/G 模块的 task continuation 就使用这个形状。
6. 同步完成与跨线程恢复为什么需要额外握手。

   **答案解析：** `await_suspend` 被调用时当前协程已经可被恢复，另一个线程或 sender 的 `start()` 可能在 `await_suspend` 返回前就恢复它。没有 phase/锁/引用寿命握手时，awaiter 可能返回 true 把已经完成的协程重新标为挂起，或稍后恢复已经销毁的 caller。Capstone5 的 `stdexec_awaitable` reference 用 atomic phase 区分 starting、suspended、completed、abandoned。
7. ready 短路、死代码消除、HALO 分别验证什么。

   **答案解析：** ready 短路验证的是标准控制流：`await_ready()==true` 时不求值 `await_suspend()`，计数器即可证明。死代码消除验证的是优化器是否删除了可证明不可达的挂起分支，要看 IR、dump 或汇编。HALO 验证的是 coroutine state 的 heap allocation 是否被省略，要看 elide remark、分配调用或反汇编，不能只靠运行时间推断。

继续阅读：[08 模块 F：协程帧与 allocator](08-模块F-协程帧与allocator.md)。
