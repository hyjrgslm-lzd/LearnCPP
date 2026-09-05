# 06 模块 D：promise_type 全解

## 模块目标

前面三个模块在使用现成的 `lazy_task<T>` 和 `std::generator`。这个模块要把"编译器到底在帮你生成什么"彻底拆开：

- 协程的 8 个 promise_type hook 各自在什么时机被调用
- eager 和 lazy 启动策略只差一个 `initial_suspend` 的返回值
- `final_suspend` 为什么通常要挂起以保留结果消费窗口（除非你很清楚自己在做什么）
- `await_suspend` 返回 `coroutine_handle` 是怎么把控制转交给 continuation，从而避免库代码直接嵌套 `.resume()` 的

如果你跳过这一层，协程就永远是一个"黑盒语法糖"。

## 模块完成标准

做完本模块，你至少要能稳定说清楚：

- promise_type 的 8 个 hook 分别在什么时机被框架调用，以及它们之间如何协作构成一条完整协程生命周期。
- `initial_suspend` 是 lazy 和 eager 启动语义的核心开关。
- `final_suspend` 返回 `suspend_always` 是为了让协程帧的销毁晚于最终结果被取走。
- 为什么教学 task 通常让 `unhandled_exception` 存储异常并 `noexcept`，以及标准允许它抛出时会进入怎样的异常传播路径。
- `await_suspend` 返回 `coroutine_handle` 如何转交控制权，从而避免库代码层层直接 `.resume()` 导致的栈增长风险。

## 类型骨架桥接段

从"使用现成 task"到"从零实现 promise_type"是这个学习包中最大的跳跃。为了降低摩擦，这里给出一个完整的 `lazy_task<T>` 的 promise_type 8 个 hook 骨架。你不需要背它们，但每当你遇到"编译器说 promise_type 缺什么"时，回来对照这段骨架。

### promise_type 完整 8 hook 骨架

```cpp
template <typename T>
struct lazy_task {
    struct promise_type {
        T result_value;                           // 存储 co_return 的值
        std::exception_ptr result_exception;       // 存储未捕获异常
        std::coroutine_handle<> continuation;      // 等待本协程的调用者

        // ---- 1. get_return_object ----
        // 协程帧创建后第一个被调用的 hook。
        // 返回给调用者的 task 对象，它持有 coroutine_handle。
        lazy_task get_return_object() {
            return lazy_task{
                std::coroutine_handle<promise_type>::from_promise(*this)
            };
        }

        // ---- 2. initial_suspend ----
        // 决定协程体是否立即开始执行。
        // suspend_always -> lazy（调用者手动 resume）
        // suspend_never  -> eager（协程创建即开始执行）
        std::suspend_always initial_suspend() noexcept { return {}; }

        // ---- 3. final_suspend ----
        // 协程体执行完后被调用。
        // 必须挂起，否则协程帧在 continuation 被设置前就销毁了。
        // 如果 continuation 存在，可利用 symmetric transfer 跳过去。
        auto final_suspend() noexcept {
            struct final_awaiter {
                bool await_ready() noexcept { return false; }
                auto await_suspend(std::coroutine_handle<promise_type> h) noexcept {
                    if (h.promise().continuation)
                        return h.promise().continuation; // symmetric transfer
                    return std::noop_coroutine();
                }
                void await_resume() noexcept {}
            };
            return final_awaiter{};
        }

        // ---- 4. return_value ----
        // co_return expr; 时被调用。
        // 保存值，等待调用者通过 task.get() 取出。
        void return_value(T value) {
            result_value = std::move(value);
        }

        // ---- 5. unhandled_exception ----
        // 协程体抛出未捕获异常时被调用。
        // 教学 task 选择存储异常；标准允许该函数抛出，但会把完成路径复杂化。
        void unhandled_exception() noexcept {
            result_exception = std::current_exception();
        }

        // ---- 6. yield_value (仅 generator 用, lazy_task 可省略) ----
        // 本 task 不涉及 co_yield，如需 generator 则实现此 hook。

        // ---- 7. operator new（可选，[dcl.fct.def.coroutine] allocation 定制） ----
        // 自定义协程帧的分配。配合 operator delete 使用。
        // 若不定义，编译器使用全局 operator new。
        static void* operator new(std::size_t size) {
            return ::operator new(size);
        }
        static void operator delete(void* ptr, std::size_t size) {
            ::operator delete(ptr);
        }

        // ---- 8. get_return_object_on_allocation_failure (可选) ----
        // 当 promise scope 中找到该 hook 时，协程分配走 nothrow 失败路径；
        // allocation function 返回 nullptr 后调用本 hook 返回失败态 task。
        static lazy_task get_return_object_on_allocation_failure() noexcept {
            return lazy_task{};
        }

        // ---- 可选: await_transform ----
        // 当协程体内出现 co_await expr 时，编译器会先尝试
        // promise.await_transform(expr)。如果定义了，用其返回值作为
        // 真正的 awaitable；否则直接用 expr 自身。
        // 一般 lazy_task 不定义，留空即可。
        // 模块 E 会详细介绍这个 hook。
    };

    // ---- task 自身 ----
    std::coroutine_handle<promise_type> h_;

    explicit lazy_task(std::coroutine_handle<promise_type> h) : h_(h) {}
    lazy_task(lazy_task&& other) noexcept : h_(std::exchange(other.h_, {})) {}
    lazy_task& operator=(lazy_task&& other) noexcept {
        if (this != &other) {
            if (h_) h_.destroy();
            h_ = std::exchange(other.h_, {});
        }
        return *this;
    }
    lazy_task(const lazy_task&) = delete;
    lazy_task& operator=(const lazy_task&) = delete;

    ~lazy_task() {
        if (h_) h_.destroy();
    }

    // ---- 对外接口 ----
    // get()：驱动协程直到完成，然后返回结果或重新抛异常。
    T get() {
        if (!h_) throw std::logic_error("empty task");
        h_.resume();                          // 启动/继续协程
        auto& p = h_.promise();
        if (p.result_exception)
            std::rethrow_exception(p.result_exception);
        return std::move(p.result_value);
    }

    // 让 task 可以被 co_await（支持嵌套协程）
    // 优化：若 task 已完成（或为空），直接走 ready 路径，免一次挂起
    bool await_ready() noexcept { return !h_ || h_.done(); }
    auto await_suspend(std::coroutine_handle<> caller) noexcept {
        h_.promise().continuation = caller;
        return h_;                           // symmetric transfer
    }
    T await_resume() {
        auto& p = h_.promise();
        if (p.result_exception)
            std::rethrow_exception(p.result_exception);
        return std::move(p.result_value);
    }
};
```

### 8 个可定制点调用时序一览

以下为协程框架的 8 个可定制点，按调用时序排列。注意："8 个 hook"的说法有歧义——`yield_value` 和 `return_value` 互斥（一个协程不会同时使用两者），`await_transform` 虽然属于可定制点但不占用固定时序位置。

**必有 hook（按调用时序）**：

| 序号 | Hook | 调用时机 | 关键作用 |
|------|------|----------|----------|
| 1 | `get_return_object` | 协程帧分配后、协程体执行前 | 构造返回给调用者的 task 对象 |
| 2 | `initial_suspend` | `get_return_object` 之后 | 决定是否立即执行协程体 |
| 3 | `return_void` 或 `return_value` | `co_return` 处 | 保存返回值（互斥二选一） |
| 4 | `final_suspend` | 协程体结束/`co_return` 执行后 | 挂起并通知等待者 |
| 5 | `unhandled_exception` | 协程体抛出未捕获异常时 | 存储异常 |

**可选 hook（按职责）**：

| Hook | 职责 | 说明 |
|------|------|------|
| `yield_value` | `co_yield expr` 时产生中间值 | 与 `return_value` 互斥，仅 generator 使用 |
| `await_transform` | `co_await expr` 前转换 awaitable | 可拦截、包装或拒绝特定类型的 awaitable |
| `operator new` | coroutine state 需要额外存储且分配未被消除时 | `[dcl.fct.def.coroutine]` 规定的 promise allocation 定制入口 |
| `operator delete` | 协程帧释放时 | 与 operator new 配对 |
| `get_return_object_on_allocation_failure` | 分配函数按 nothrow 语义失败并返回 null 时 | 分配失败回退路径 |

---

## 练习 D-1：从零写 lazy_task<T>

### 目标

完整实现 `lazy_task<T>` 的 promise_type 可定制点（get_return_object / initial_suspend / final_suspend / return_value / unhandled_exception 及其余可选 hook），验证 `co_return` 值被外部 `get()` 正确取出、协程体内抛异常被外部 `get()` 重新抛出。

### 前置理解

- 你已经会使用附带的 `lazy_task<T>` 写协程函数。
- 你知道协程体在遇到 `co_return` / `co_await` / `co_yield` 时编译器会为你生成状态机，状态机的"控制器"就是 `promise_type`。
- 你接受本题的重点是实现 8 个 hook 的最小正确版本，不追求高性能或无锁。
- 关键概念：`coroutine_handle<promise_type>` 是协程帧的句柄。`from_promise(p)` 可以从 promise 对象获取句柄。`h.resume()` 启动或恢复协程执行。`h.destroy()` 销毁协程帧。`h.promise()` 获取帧内的 promise 对象。

### 必做任务

1. 参照上面的骨架代码，从头写出完整的 `lazy_task<T>`。必须实现以下可定制点：
   - `get_return_object`
   - `initial_suspend`（返回 `suspend_always`）
   - `final_suspend`（返回自定义 final_awaiter）
   - `return_value`
   - `unhandled_exception`
   - `yield_value`（即使不用，也要声明或明确省略）
   - `operator new` / `operator delete`
   - `get_return_object_on_allocation_failure`

2. 写测试协程函数验证正常流程：
   ```cpp
   lazy_task<int> compute() {
       int x = 10;
       int y = 20;
       co_return x + y;
   }
   auto task = compute();
   int result = task.get();  // 期望 30
   ```

3. 写测试协程函数验证异常流程：
   ```cpp
   lazy_task<int> faulty() {
       throw std::runtime_error("boom");
       co_return 0;  // 永远不会执行到这里
   }
   auto task = faulty();
   try {
       task.get();
   } catch (const std::runtime_error& e) {
       // 应该捕获到 "boom"
   }
   ```

4. 验证多重 `co_await` 嵌套调用：
   ```cpp
   lazy_task<int> inner() { co_return 42; }
   lazy_task<int> outer() {
       int v = co_await inner();
       co_return v * 2;
   }
   auto task = outer();
   int result = task.get();  // 期望 84
   ```

5. 在笔记中画出：协程帧何时分配、`get_return_object` 何时被调用、`initial_suspend` 挂起后控制权回到哪、`get()` 调用 `resume()` 后在哪个 hook 继续执行、`final_suspend` 时 continuation 是谁。

### 进阶任务

- 用 `-fdump-tree-coro`(GCC) 或 `/d1reportSingleClassLayout`(MSVC) 打印你的 `lazy_task<int>` 协程帧布局，数一数帧里包含多少字段（promise + 参数拷贝 + 局部变量）。
- 在多线程环境下验证：一个线程创建 task，另一个线程调用 `get()`。观察 `coroutine_handle` 的线程安全性——它的 `resume()` 可以在任意线程调用。
- 给 `lazy_task` 添加 `operator co_await`，让它可以被 `co_await task` 使用。对比直接调用 `get()` 和 `co_await` 的差异。

### 验收点

- 你的 `lazy_task<T>` 能正确返回 `co_return` 的值。
- 你的 `lazy_task<T>` 能重新抛出协程体内的异常到 `get()` 调用者。
- 你的 `lazy_task<T>` 支持嵌套 `co_await`。
- 你能说明每个 hook 在协程生命周期中的位置和职责。
- 你没有在教学 task 的 `unhandled_exception` 中再次 throw；异常通过 `exception_ptr` 延迟交给消费者。

### 观察点

- `get_return_object` 在协程体执行**之前**就被调用。这意味着此时返回值还不存在——它只是返回一个"遥控器"。
- `initial_suspend` 返回 `suspend_always` 意味着协程体一行代码都没执行时就已经挂起了。控制权回到 `compute()` 的调用者，调用者拿到的是"未开始的 task"。
- `final_suspend` 挂起后，协程帧还在。`get()` 此时才能安全地读取 `result_value`。如果 `final_suspend` 返回 `suspend_never`，协程帧在 `get()` 读取前就销毁了——UB。
- 教学 task 的 `unhandled_exception` 中再次 throw 会绕开你设计的 `exception_ptr` 消费通道，并让 final-suspend/调用者传播路径变难维护。课程要求这里存储，不重抛。

### 常见坑

- `final_suspend` 写了 `suspend_never`——结果 `get()` 读到了已销毁帧上的值。
- `unhandled_exception` 里写 `throw;`——标准允许异常传播给 caller/resumer，但这会破坏本题通过 `exception_ptr` 统一消费异常的契约。
- `get_return_object` 中用 `coroutine_handle<promise_type>::from_promise(*this)`，但忘了此时 promise 还没有被构造完成（实际上 `get_return_object` 被调用时 promise 已构造，所以这个写法是对的。但要注意不要在构造函数中依赖协程体执行——它还没有开始）。
- `lazy_task` 的移动构造忘记清空源的 `h_`，导致两个 task 都认为自己拥有协程帧——双重 destroy。
- 析构函数中直接 `h_.destroy()` 但对象可能正处于运行中或被别处 resume——`destroy()` 的前提是 handle 指向的协程处于挂起状态；对未挂起协程 destroy 是 UB。

### 提示

- 先让正常 return 流程跑通（只需 `get_return_object` + `initial_suspend` + `final_suspend` + `return_value`），再补异常处理和其他 hook。
- `final_suspend` 的 `await_suspend` 中，如果 `continuation` 为空（没人等待这个 task），返回 `std::noop_coroutine()` 即可。
- 抄骨架代码时注意类型名——你的 `lazy_task`、`promise_type`、`final_awaiter` 之间的嵌套关系必须严格一致。

### 复盘问题

- 为什么 `get_return_object` 能在协程体执行之前被调用？框架是怎么知道该返回什么类型的？
- 如果 `initial_suspend` 返回 `suspend_never`，协程体在 `get_return_object` 已经构造出返回对象之后、协程函数调用返回给调用者之前开始执行吗？日志如何证明？
- 为什么 `final_suspend` 几乎总是需要挂起（返回非 `suspend_never`）？
- `unhandled_exception` 中存储的 `std::exception_ptr` 为什么能在 `get()` 中被安全地 `rethrow_exception`？

### 对应官方参考

- Lewis Baker "Understanding the promise type"（系列第 5 篇）
- Lewis Baker "C++ coroutines: Managing the coroutine frame"（系列第 6 篇）
- cppcoro `task.hpp` 的 promise_type 实现
- P0913R1：Add symmetric coroutine control transfer (Gor Nishanov)

---

## 练习 D-2：eager vs lazy 切换

### 目标

只改 `initial_suspend` 一行代码，把 lazy task 变成 eager task，观察两种语义的差异。再实现混合策略：在惰性启动的基础上提供"立即启动"的变体。

### 前置理解

- 你已经完整实现了 D-1 的 `lazy_task<T>`。
- 你知道 `initial_suspend` 返回 `suspend_always`（内置 awaiter，`await_ready` 返回 false）意味着协程创建后立即挂起，等待第一次 `resume()`。
- 你知道 `suspend_never`（`await_ready` 返回 true）意味着协程创建后立即开始执行协程体。
- 关键差异：**lazy**（惰性）——协程创建 = 获得描述对象，需要显式 `resume()` 才开始执行。**eager**（立即）——协程创建 = 立即开始执行到第一个挂起点。
- P3552R3 中 `std::execution::task<T>` 选择 lazy 语义，因为这个 task 本身可能被传给 `when_all` 等组合子——组合子需要先拿到 task 对象，再决定何时启动它。

### 必做任务

1. 在 D-1 的 `lazy_task<T>` 上做最小修改：将 `initial_suspend` 的返回类型从 `suspend_always` 改为 `suspend_never`。
2. 写对比实验：
   ```cpp
   // 版本 A：lazy（initial_suspend = suspend_always）
   auto t = lazy_compute();  // 此时协程还未执行
   // ... 这里可以做其他事 ...
   int r = t.get();           // 此时才真正开始执行协程体

   // 版本 B：eager（initial_suspend = suspend_never）
   auto t = eager_compute();  // 协程体已开始执行，可能已经跑完了
   int r = t.get();           // 只是取走结果
   ```
   在两个版本中分别在协程体开始处和 `get()` 调用处插入时间戳打印，观察执行时机的差异。

3. 写一个协程体包含多个 `co_await` 点的 eager task。观察：eager task 在创建后会执行到第一个 `co_await` 挂起，还是跑到 `co_return`？
   ```cpp
   lazy_task<int> multi_step() {  // 注意：即使改了 initial_suspend，函数仍返回 lazy_task
       std::print("step 1\n");
       co_await async_sleep(50ms);
       std::print("step 2\n");
       co_return 42;
   }
   ```

4. 实现混合策略：保留基础版 `lazy_task<T>`（`initial_suspend = suspend_always`），同时提供一个工厂函数 `eager_start(task)`：
   ```cpp
   template <typename T>
   lazy_task<T> eager_start(lazy_task<T> t) {
       t.h_.resume();  // 立即启动
       return t;        // 返回已启动的 task
   }
   ```
   验证：调用 `eager_start(multi_step())` 后，在 `get()` 之前协程已经执行到第一个挂起点。

5. 在笔记中写清楚：eager 语义的真实代价是什么？为什么制定中的 P3552R3 选择 lazy 作为 `std::execution::task<T>` 的默认？

### 进阶任务

- 测量 eager vs lazy 在"创建 1000 个 task 但不全部消费"场景下的帧分配开销差异。
- 研究 cppcoro 的 `task<T>` 是如何处理 `initial_suspend` 的——它用了 lazy 还是 eager？
- 思考：如果 task 的协程体中有 `co_await` 一个外部资源（如 socket），eager 启动意味着什么？safety 方面有什么隐患？

### 验收点

- 你能只改一行代码切换 lazy / eager 行为。
- 你能观察到 lazy 时协程体在 `get()` 调用后才执行。
- 你能解释为什么框架级 task（P3552）倾向于 lazy，应用级 task 有时需要 eager。
- 你能说明 eager 启动"跑到第一个挂起点再停"的行为对资源使用的影响。

### 观察点

- lazy 是"先拿到遥控器，再决定什么时候按开关"。eager 是"创建即开始"。
- lazy 组合性更强：`when_all` 可以先收集所有 task，再统一启动。
- eager 让"启动"变得隐式，可能导致在组合子还没来得及设置上下文（如 stop_token）时协程已经开始跑了。
- `suspend_always` 和 `suspend_never` 是内置的 trivial awaitable，它们的存在说明"挂起与否"只取决于 `await_ready` 的返回值。

### 常见坑

- 把 eager 当成"性能更好"的 lazy——两者性能差异在绝大多数场景下可忽略。lazy 多了一次 `resume()` 调用，但 eager 在"task 创建后可能被丢弃"的场景下浪费了一次帧分配和执行。
- eager task 创建后立即执行，但调用者还没有机会设置 continuation——如果协程体快速 `co_return`，`final_suspend` 中 continuation 为空，协程帧就销毁了。之后 `get()` 访问已销毁帧 -> UB。
- 混合使用 lazy 和 eager 时，不注意"task 是否已经启动"这个状态，导致重复 resume 或遗漏 resume。

### 提示

- 测量 lazy 和 eager 时差时，用 `std::chrono::high_resolution_clock` 并取多次平均值。
- 混合策略的 `eager_start` 不改变 task 类型——它只是立即 resume 一次。这保持了接口一致性。
- 思考题：如果你的 `lazy_task` 不支持 `operator co_await`，`eager_start` 和 `co_await` 会不会有语义冲突？

### 复盘问题

- 什么场景下 eager 是合理的默认行为？什么场景下 lazy 更安全？
- 如果协程体抛出异常且 unhandled_exception 已经存储了异常，eager task 的 `get()` 是在什么时候发现这个异常的？
- P3552R3 选择 lazy 的理由中，你认为哪一条最有说服力？
- 你的 `lazy_task<T>` 是 movable 的。如果 `eager_start` 后 task 被移动到另一个线程，会发生什么？

### 对应官方参考

- Lewis Baker "C++ coroutines: Understanding the promise type"（initial_suspend 部分）
- P3552R3：`std::execution::task<T>` 的 lazy 语义设计
- cppcoro `task.hpp` 中 `initial_suspend` 的实现

---

## 练习 D-3：final_suspend + symmetric transfer

### 目标

在 `final_suspend` 的 `await_suspend` 中返回 `coroutine_handle` 实现控制权转交，让协程完成时把等待者交给 `co_await` 变换恢复，避免库代码直接写成"resume 调用 resume"导致栈增长。

### 前置理解

- 你已经完成 D-1，实现了 `final_suspend` 中 continuation 的基本逻辑。
- 你知道 `await_suspend` 有三种合法返回类型：`void`、`bool`、`std::coroutine_handle<>`。本题聚焦第三种。
- 关键概念：**symmetric transfer**——当协程 A 完成时（`final_suspend`），它不直接 `resume` 等待者 B，而是从 `await_suspend` 返回 B 的 `coroutine_handle`。标准 `co_await` 变换会恢复这个 handle；优秀实现通常会把这条路径优化成不累积调用栈的控制转交。在深层嵌套或循环嵌套协程场景下，它避免的是库代码直接递归 `.resume()` 的栈增长风险。
- 相比之下，如果在 `final_suspend` 中直接写 `continuation.resume()`，你就是在 A 的栈帧中调用 B 的恢复。每嵌套一层，栈就深一层。

### 必做任务

1. 在 D-1 的 `final_awaiter::await_suspend` 中，确认你的实现已经使用了 symmetric transfer：
   ```cpp
   auto await_suspend(std::coroutine_handle<promise_type> h) noexcept {
       if (h.promise().continuation)
           return h.promise().continuation;  // symmetric transfer
       return std::noop_coroutine();
   }
   ```
   注意这里的返回值类型是 `std::coroutine_handle<>`，不是 void。

2. 写一个嵌套调用链实验：创建 N 层嵌套的协程，最内层 `co_return` 一个值，观察控制权从最内层一路传递到最外层：
   ```cpp
   lazy_task<int> chain(int n) {
       if (n == 0) co_return 0;
       int v = co_await chain(n - 1);
       co_return v + 1;
   }
   auto task = chain(1000);  // 1000 层嵌套
   int result = task.get();   // 期望 1000。如果栈没爆，symmetric transfer 生效
   ```

3. 写对比实验：故意在 `final_suspend` 中直接调用 `continuation.resume()`（不用 symmetric transfer），同样跑 1000 层嵌套，观察是否能完成。如果你的编译器和平台有小栈限制，可能在 100 层以内就栈溢出。

4. 画一张图描绘 symmetric transfer 的控制流：
   - 协程 A 完成 -> `final_suspend` -> `await_suspend` 返回 B 的 handle
   - 标准 `co_await` 变换收到这个返回的 handle -> 恢复 B；具体是否生成机器级 tail call 由实现决定
   - B 的 `await_resume` 被调用，拿到 A 的结果

5. 在笔记中明确记录：
   - symmetric transfer 消除了哪种类型的栈增长？
   - 为什么 `await_suspend` 返回 `coroutine_handle` 是标准层面表达这种控制转交的方式？
   - 这和 `await_suspend` 返回 `void`（框架在 suspend 后就返回调用者，调用者手动 resume 等待者）的差异是什么？

### 进阶任务

- 用 `-fsanitize=address` 和显式的栈深度计数器，测量 symmetric transfer 版和非 symmetric transfer 版在 1000 层嵌套下的**实际栈使用量**差异。
- 研究 `std::noop_coroutine()`：它是什么？为什么在 continuation 为空时返回它而不是返回空 handle？
- 在 `lazy_task::await_suspend`（让 task 可被 co_await 的那个）中也使用 symmetric transfer。验证嵌套 `co_await` 链的控制流完全通过 symmetric transfer 完成。

### 验收点

- 你的 `final_suspend` 通过返回 continuation handle 将控制权转交给等待者。
- 你能在目标编译器上跑通足够深的嵌套协程链，并记录是否出现栈增长/栈溢出。
- 你能画出 symmetric transfer 的控制流图。
- 你能对比三种 `await_suspend` 返回值（void / bool / coroutine_handle）的语义差异。

### 观察点

- symmetric transfer 的重点是：A 不在自己的库代码里直接调用 B 的 `.resume()`，而是把下一个要恢复的 handle 作为 awaiter 结果交给 `co_await` 变换。
- 如果没有 symmetric transfer，`await_suspend` 返回 void 时，框架在挂起 A 后控制权返回给"启动 resume 的那个调用者"。这个调用者再手动 resume B。这一来一去，栈就不会增长——但需要额外一层调度。
- symmetric transfer 提供了"把下一个恢复目标作为返回值交出去"的直接控制转交模式；机器栈表现仍要用目标编译器实测确认。
- 在 `final_suspend` 中使用 symmetric transfer 是最关键的场合，因为这里是一个协程生命周期的终结 + 下一个协程的开始。

### 常见坑

- 在 `await_suspend` 中直接 `h.resume()` 而不是 `return continuation;`——失去了 symmetric transfer 的全部意义。
- `return continuation;` 时忘记检查 continuation 是否为空——空 handle 的 resume 是 UB（好在 `std::noop_coroutine()` 可以兜底）。
- 混淆三种返回值的语义：
  - `void` = 框架挂起我，控制权回到调用 resume 的人手里。之后由别人 resume 等待者。
  - `bool` = 返回 true 表示当前协程保持挂起，返回 false 表示不挂起并立即继续当前协程。
  - `coroutine_handle` = 不回到调用 resume 的人，直接去跑这个 handle 指向的协程。
- 在 `final_suspend` 返回 `suspend_never` — 协程帧立即销毁，continuation 没机会被用上。

### 提示

- 先用 D-1 的骨架确认 symmetric transfer 已正确实现，再去做 1000 层嵌套实验。
- 嵌套实验如果不确定，可以先用 10 层验证流程，再逐步增加到 100、1000。
- 比较三种 `await_suspend` 返回值时，不要只记区别——各写一个最小示例跑一遍，看控制流差异。
- `std::noop_coroutine()` 返回一个 `resume()` 和 `destroy()` 都是 no-op 的协程句柄，用于避免空 handle 操作。

### 复盘问题

- symmetric transfer 解决的核心问题是库代码层层直接 `.resume()` 造成的栈增长吗？请画出调用栈证明。
- 为什么 symmetric transfer 不等于标准强制的"tail-call optimization for coroutines"？
- 在 `await_suspend` 中返回 `std::noop_coroutine()` 和返回空 handle 有什么区别？
- 如果你要在 `lazy_task` 的 `operator co_await` 中也使用 symmetric transfer，应该怎么写？

### 对应官方参考

- Lewis Baker "C++ coroutines: Symmetric transfer"（系列第 5-6 篇）
- P0913R1 中对称转移的设计动机
- cppcoro `task.hpp` 中 `final_suspend` 的 symmetric transfer 用法

---

## 做完模块 D 之后，你现在应该能说清楚什么

至少把下面几句话说顺：

- promise_type 的 8 个 hook 各有固定调用时机，构成完整的协程生命周期。
- `initial_suspend` 是协程体初始是否执行的核心开关；完整 lazy/eager API 语义还取决于返回对象何时取得所有权、谁能 start/await，以及重复启动如何拒绝。
- `final_suspend` 通常必须挂起，否则协程帧可能在结果被消费前就已销毁。
- 教学 task 的 `unhandled_exception` 存储异常并等待外部捕获；标准层面允许抛出但要理解其传播路径。
- `await_suspend` 返回 `coroutine_handle` 实现控制权转交，但不要把机器级 tail call 当成标准保证。
- 理解了这些之后，你看 cppcoro / folly coro / stdexec task 的源码时，不会再被 promise_type 的实现细节吓住。
