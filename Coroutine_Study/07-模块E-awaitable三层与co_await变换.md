# 07 模块 E：awaitable 三层与 co_await 变换

## 模块目标

前面模块 D 让你实现了 promise type 的必需主干与可选定制点，看到了协程的生命周期全貌。这个模块要把协程最核心的运行时动态——`co_await` 的编译器变换——彻底拆开：

- `co_await` 表达式在编译器内部经历了怎样的变换（await_transform / operator co_await 重载决议 / awaiter 三方法）
- `await_suspend` 的三种返回值（void / bool / coroutine_handle）各自产生什么样的控制流和状态机
- Trivial Awaitable（`await_ready` 直接返回 true）如何让编译器完全消除挂起——理解 HALO 的前置条件在 awaitable 层面的对应物

如果你跳过这一层，`co_await` 就永远是一个“暂停然后继续”的魔法，你无法解释 ready 短路、awaiter 与 coroutine state 的不同生命周期，以及 P0913 symmetric transfer 和 `[exec.as.awaitable]` sender 桥接在解决什么问题。

## 模块完成标准

做完本模块，你至少要能稳定说清楚：

- `co_await expr` 的编译器变换过程：从表达式到 awaitable、到挂起/恢复决策、到结果提取的完整步骤。
- `co_await` 变换的优先顺序和执行逻辑——普通 await expression 先查 promise scope 的 `await_transform`；再对结果做 `operator co_await` 重载决议；无可行 operator 时对象本身作为 awaiter。
- `await_suspend` 三种返回值各自产生怎样不同的控制流，以及 symmetric transfer 如何表达控制权转交。
- `await_ready()` 返回 true 时，标准如何保证跳过 `await_suspend`，以及优化器可能怎样消除对应分支。
- 如何把 `await_ready` 的语义短路与 HALO 的 frame allocation elision 区分开：前者是控制流规则，后者是实现优化。

## 使用建议

- E-1 偏编译期查找规则，建议先在纸上画出三条路径的决策树，再写代码验证。
- E-2 偏运行时控制流，建议每条返回值各写一个最小示例，用打印或断点观察执行顺序。
- E-3 偏编译期优化，强烈建议配合编译器 flag（`-Rpass=coroutine-elide` 等）观察。
- 本模块的所有练习都可以单线程完成，不依赖第三方库。

---

## 练习 E-1：co_await 三步查找

### 目标

写出三个不同的 awaitable 类型 / 协程 / 自由函数，分别触发 `co_await` 编译器变换的几条路径：promise.await_transform、成员 `operator co_await`、ADL 可见的自由 `operator co_await`、对象本身作为 awaiter。亲手验证编译器在每种情况下的选择，理解"await_transform 是 promise 唯一的拦截钩子"的含义。

### 前置理解

- 当编译器在协程体内遇到普通 `co_await expr` 时，它不是直接把 `expr` 当作 awaiter。它先在 promise 类型作用域中查找 `await_transform`；如果找到可调用重载，其返回值进入下一步，否则原始 `expr` 进入下一步。
- 下一步不是手写的“先成员后 ADL”流程，而是枚举适用的 `operator co_await` 候选并做重载决议。候选可能来自成员函数，也可能来自 ADL 找到的自由函数；如果决议歧义，程序 ill-formed。
- 如果没有可行的 `operator co_await`，上一步对象本身作为 awaiter，必须提供 `await_ready/await_suspend/await_resume`。

查找优先级总结（从高到低）：

1. `promise.await_transform(expr)` —— 如果 promise scope 中找到可调用重载
2. `operator co_await` 重载决议 —— 成员候选和 ADL 自由函数候选都在这里竞争
3. 对象本身作为 awaiter —— 没有可行 `operator co_await` 时才走这一层

理解这个优先级的关键含义：**即使 expr 自身是一个合法的 awaitable（有正确的三个 await 方法），如果 promise 定义了匹配的 `await_transform`，编译器也会先调用 `await_transform`，用它的返回值作为真正的 awaitable，而不是直接使用 expr。** 这意味着 `await_transform` 可以"拦截并改写"任何 `co_await` 表达式——这是 promise 对协程体内所有 await 行为的全局控制点。

### 必做任务

1. **准备三个 awaitable 类型**，各有一个明显的名字以在日志中区分：

   ```cpp
   // awaitable_A —— 会被 await_transform 拦截
   struct awaitable_A {
       bool await_ready() { return false; }
       void await_suspend(std::coroutine_handle<>) {
           std::println("  awaitable_A::await_suspend");
       }
       int await_resume() { return 1; }
   };

   // awaitable_B —— 有自身的 operator co_await（成员函数）
   struct awaitable_B {
       bool await_ready() { return false; }
       void await_suspend(std::coroutine_handle<>) {
           std::println("  awaitable_B::await_suspend (should NOT be called)");
       }
       int await_resume() { return 2; }

       // 成员 operator co_await —— 返回另一个 awaitable
       auto operator co_await() {
           struct wrapper {
               bool await_ready() { return false; }
               void await_suspend(std::coroutine_handle<>) {
                   std::println("  awaitable_B wrapper::await_suspend");
               }
               int await_resume() { return 20; }
           };
           return wrapper{};
       }
   };

   // awaitable_C —— 靠 ADL operator co_await
   namespace custom_ns {
       struct awaitable_C {
           bool await_ready() { return false; }
           void await_suspend(std::coroutine_handle<>) {
               std::println("  awaitable_C::await_suspend (should NOT be called)");
           }
           int await_resume() { return 3; }
       };

       // ADL operator co_await
       auto operator co_await(awaitable_C c) {
           struct wrapper {
               bool await_ready() { return false; }
               void await_suspend(std::coroutine_handle<>) {
                   std::println("  awaitable_C ADL wrapper::await_suspend");
               }
               int await_resume() { return 30; }
           };
           return wrapper{};
       }
   }
   ```

2. **定义带 await_transform 的 promise 协程**：

   ```cpp
   struct transform_task {
       struct promise_type {
           transform_task get_return_object() {
               return transform_task{
                   std::coroutine_handle<promise_type>::from_promise(*this)
               };
           }
           std::suspend_always initial_suspend() { return {}; }
           std::suspend_always final_suspend() noexcept { return {}; }
           void return_void() {}
           void unhandled_exception() {}

           // await_transform —— 拦截所有 co_await 表达式
           // 对 awaitable_A 做特殊处理
           auto await_transform(awaitable_A a) {
               struct intercepted {
                   bool await_ready() { return false; }
                   void await_suspend(std::coroutine_handle<>) {
                       std::println("  [await_transform] intercepted awaitable_A");
                   }
                   int await_resume() { return 100; }
               };
               return intercepted{};
           }

           // 对 awaitable_B 也做拦截
           auto await_transform(awaitable_B b) {
               struct intercepted {
                   bool await_ready() { return false; }
                   void await_suspend(std::coroutine_handle<>) {
                       std::println("  [await_transform] intercepted awaitable_B");
                   }
                   int await_resume() { return 200; }
               };
               return intercepted{};
           }

           // 不拦截 awaitable_C —— 让它走 ADL 路径
       };

       std::coroutine_handle<promise_type> h_;
       explicit transform_task(std::coroutine_handle<promise_type> h) : h_(h) {}
       ~transform_task() { if (h_) h_.destroy(); }

       void run() { h_.resume(); }  // 启动并跑到结束
   };
   ```

3. **写协程体，分别 co_await 三种类型**：

   ```cpp
   transform_task demo() {
       std::println("  === co_await awaitable_A ===");
       int a = co_await awaitable_A{};
       std::println("  result A: {} (expect 100 if await_transform took effect)", a);

       std::println("  === co_await awaitable_B ===");
       int b = co_await awaitable_B{};
       std::println("  result B: {} (expect 200 if await_transform took effect)", b);

       std::println("  === co_await awaitable_C ===");
       int c = co_await custom_ns::awaitable_C{};
       std::println("  result C: {} (expect 30 from ADL operator co_await)", c);

       co_return;
   }
   ```

4. **写一个不定义 await_transform 的对照协程**，确保 awaitable_B 的成员 operator co_await 和 awaitable_C 的 ADL operator co_await 在无 promise 拦截时正常工作。

5. **在笔记中画出 `co_await` 变换决策树**：

   ```
   co_await expr
     |
     v
   promise.await_transform(expr) 存在且可调用？
     |-- 是 --> 使用 await_transform 的返回值进入下一步
     |           （即使 expr 本身也可以是 awaiter，也会先被覆盖）
     |
     |-- 否 --> 使用 expr 自身进入下一步
                |
                v
   枚举并重载决议 operator co_await 候选
     |-- 有唯一最佳候选 --> 使用其返回值作为 awaiter
     |-- 候选歧义 -------> ill-formed
     |
     |-- 无可行候选 -----> 对象本身作为 awaiter
                          （需满足 await_ready/await_suspend/await_resume）
   ```

6. **讨论**：`await_transform` 为什么是"promise 唯一的拦截钩子"？——它让 promise 有能力在协程体内所有 `co_await` 点之前插入逻辑，比如注入调度上下文、包装错误处理、或者将 sender 翻译为 awaitable（这正是模块 H 要做的事）。

### 进阶任务

- 实现一个 **logging promise**：`await_transform` 对所有类型都记录一条日志然后返回原始值（让它继续走后续查找路径）。这需要将 `await_transform` 设计为"泛型兜底 + 特定类型拦截"的组合。
- 写一个类型同时有成员 `operator co_await()` 和所在命名空间的 ADL `operator co_await()`。在没有 `await_transform` 的情况下，观察重载决议选哪一个；如果两个候选同样好，程序应当歧义报错。
- 在 `await_transform` 中返回一个比原始 awaitable 多一层计时的 wrapper，实现对协程体内所有 await 点的无侵入计时。

### 验收点

- 你能通过日志输出确认 `await_transform` 在三条路径中优先被选择。
- 你能在不定义 `await_transform` 的对照协程中，确认成员 `operator co_await()`、ADL `operator co_await()`、对象本身作为 awaiter 三种路径各自生效。
- 你能画出完整的 `co_await` 变换决策树，并标注每一步的编译器行为。
- 你能解释：如果 promise 定义了模板化的泛型 `await_transform(T&&)`，为什么它会拦截所有 co_await 表达式——因为任何类型都能匹配。

### 观察点

- `await_transform` 的存在与否，决定了一个 promise 类型是"透明传输 co_await"还是"集中控制所有异步操作"。
- 这正是 `stdexec::task` 的 promise 能够把 `co_await sender` 翻译为连接 sender 与 bridge receiver 的机制基础——`await_transform(sender)` 返回一个 `sender_awaitable`，而后者在 `await_suspend` 中执行 `connect + start`。
- 成员 `operator co_await()` 和 ADL `operator co_await()` 的设计让第三方类型（你不拥有其源码）也能通过非侵入方式变为 awaitable。
- 这套变换设计确保了 promise 的控制权最高，同时允许类型成员和非侵入 ADL 扩展通过同一套重载决议进入 awaiter。

### 常见坑

- 在 promise 中定义了过于宽泛的 `await_transform`（如 `template<typename T> auto await_transform(T&& x)`），导致所有东西都被拦截，包括你本来希望走 ADL 路径的类型。
- 混淆 `operator co_await()` 和 awaitable 的三个接口方法——`operator co_await()` 的返回值才是真正的 awaitable，不是调用它的那个对象。
- 对基本类型（如 `int`）写 `co_await`，期望它走 ADL——但实际上需要 `await_transform` 或 `operator co_await()` 的支持，因为基本类型没有关联命名空间。
- ADL `operator co_await()` 没有放在正确类型的命名空间中，导致 ADL 找不到。

### 提示

- 先在纸上画出完整的决策树，再每写一个类型就验证一个分支。
- 用 `std::println` 在每个 `await_suspend` / `await_ready` / `await_resume` 中打印明确的来源信息，观察调用顺序。
- 对照实验（有无 `await_transform`）是确认查找规则最有效的手段。
- 如果你对 ADL 不熟，回到 Execution_Study 模块 E-1 复习 ADL 的基本规则。

### 复盘问题

- 如果 promise 定义了 `await_transform(int)`，但没有定义 `await_transform(double)`，`co_await 3.14` 会走哪条路径？
- 为什么 `await_transform` 不能定义为 non-member 函数？（因为它的调用方式 `promise.await_transform(expr)` 决定了它必须是 promise 的成员。）
- C++ 标准委员会为什么不让 `co_await` 直接使用 expr 自身作为 awaitable，而要引入这三步查找？动机是什么？
- 在 sender-receiver 框架中，`await_transform` 承担了什么角色？如果你删掉 `stdexec::task` 的 `await_transform`，会发生什么？

### 对应官方参考

- Lewis Baker "C++ coroutines: Understanding operator co_await"（系列第 3 篇）
- Lewis Baker "C++ coroutines: The co_await operator"（系列第 4 篇）
- Raymond Chen "The many meanings of co_await"（系列 7-9 篇）
- C++20 `[expr.await]` 与 `[coroutine.trivial.awaitables]`
- N4775：C++20 coroutines 标准文本中的 co_await 规范 (7.6.2.3)

---

## 练习 E-2：await_suspend 三种返回值

### 目标

完整实现并对比 `await_suspend` 的三种合法返回类型——`void`、`bool`、`std::coroutine_handle<>`——各自跑一遍完整的挂起-恢复流程。通过观察每种返回值的控制流差异，建立"symmetric transfer 如何表达嵌套协程控制转交"的直觉。

### 前置理解

- `await_suspend(coroutine_handle<P>)` 在 `await_ready()` 返回 `false` 之后被调用。标准上，进入 `await_suspend` 前当前协程已经被视为挂起；这也是为什么 awaiter 可以把 handle 交给其他线程。但如果在 `await_suspend` 尚未返回时同步重入恢复同一个协程，库状态很容易被破坏，必须显式设计同步完成路径。
- `await_suspend` 的返回值决定了挂起之后控制权流向哪里。

三种返回值的语义：

| 返回值 | 语义 | 控制流 |
|--------|------|--------|
| `void` | 无条件挂起当前协程。控制权返回给"resume 当前协程的那个调用者"（通常是上一级协程或 `sync_wait` 的驱动循环）。当前协程的恢复需要由外部代码通过 `handle.resume()` 触发。 | `await_suspend` 返回 → 框架执行挂起 → 控制权回到 caller。之后某处 `h.resume()` → 恢复执行 `await_resume()` |
| `bool` | `true` = 挂起当前协程（同 `void` 行为）；`false` = 不挂起，当前协程立即继续执行 `await_resume()`。这是唯一允许 awaitable 在 `await_ready` 返回 `false` 之后"反悔"不挂起的机制。 | `true` → 挂起，控制权返回 caller。`false` → 即刻恢复当前协程 |
| `coroutine_handle<>` | **symmetric transfer**：当前协程挂起，标准 `co_await` 变换恢复返回的 handle 指向的协程。具体是否跳过中间 caller 的机器栈帧由实现决定。 | `await_suspend` 返回 handle → 恢复目标协程 |

关键理解：返回 `void` 意味着"有一个人会来 resume 我"。返回 `coroutine_handle` 意味着"我不回去了，直接去另一个人那里"。返回 `bool` 意味着"我来决定是否挂起"——`true` 挂起（同 void），`false` 不挂起，立即继续。

### 必做任务

1. **定义三个不同的 awaiter 类型**，每个对应一种返回值：

   ```cpp
   // === awaiter_void：返回 void ===
   struct awaiter_void {
       bool await_ready() noexcept {
           std::println("  [awaiter_void] await_ready -> false");
           return false;
       }
       void await_suspend(std::coroutine_handle<> h) noexcept {
           std::println("  [awaiter_void] await_suspend -> void");
           // 当前协程挂起。我们把 handle 存下来，稍后手动 resume
           saved_handle = h;
       }
       void await_resume() noexcept {
           std::println("  [awaiter_void] await_resume");
       }
       static inline std::coroutine_handle<> saved_handle;
   };

   // === awaiter_bool：返回 bool ===
   struct awaiter_bool_true {
       bool await_ready() noexcept {
           std::println("  [awaiter_bool_true] await_ready -> false");
           return false;
       }
       bool await_suspend(std::coroutine_handle<> h) noexcept {
           std::println("  [awaiter_bool_true] await_suspend -> true (suspend, same as void)");
           // 返回 true：挂起！控制权返回 caller。需要外部 resume 来恢复
           saved_handle = h;
           return true;
       }
       void await_resume() noexcept {
           std::println("  [awaiter_bool_true] await_resume");
       }
       static inline std::coroutine_handle<> saved_handle;
   };

   struct awaiter_bool_false {
       bool await_ready() noexcept {
           std::println("  [awaiter_bool_false] await_ready -> false");
           return false;
       }
       bool await_suspend(std::coroutine_handle<> h) noexcept {
           std::println("  [awaiter_bool_false] await_suspend -> false (do NOT suspend)");
           // 返回 false：不挂起！await_resume 会立即被调用
           return false;
       }
       void await_resume() noexcept {
           std::println("  [awaiter_bool_false] await_resume");
       }
   };

   // === awaiter_symmetric：返回 coroutine_handle ===
   struct awaiter_symmetric {
       std::coroutine_handle<> target;  // 要跳转到的目标协程

       bool await_ready() noexcept {
           std::println("  [awaiter_symmetric] await_ready -> false");
           return false;
       }
       std::coroutine_handle<> await_suspend(std::coroutine_handle<> h) noexcept {
           std::println("  [awaiter_symmetric] await_suspend -> coroutine_handle (symmetric transfer)");
           // 不保存 h 也不通过 h 做任何事——直接返回目标 handle
           // 框架将直接 resume target，不经过当前函数的栈帧
           return target;
       }
       void await_resume() noexcept {
           std::println("  [awaiter_symmetric] await_resume");
       }
   };
   ```

2. **准备两个独立的协程**，观察 symmetric transfer 的跳转行为：

   ```cpp
   // 协程 B —— 被 symmetric transfer 的目标
   simple_task coro_B() {
       std::println("  [coro_B] executing...");
       co_return;
   }

   // 协程 A —— 通过 symmetric transfer 跳转到 B
   simple_task coro_A() {
       std::println("  [coro_A] before symmetric transfer");
       co_await awaiter_symmetric{coro_B_task_handle};
       // 如果 symmetric transfer 生效，下面的代码在 coro_B 完成后才执行
       std::println("  [coro_A] after symmetric transfer");
       co_return;
   }
   ```

   注意：需要先创建 `coro_B` 并获取其 `coroutine_handle`，再传递给 `awaiter_symmetric`。由于 `coro_A` 的 `await_suspend` 返回了 B 的 handle，框架直接 resume B——A 的恢复被"推迟"到 B 完成之后（通过 B 的 `final_suspend` symmetric transfer 回来）。

3. **用 void 版本的 awaiter 实现嵌套 resume 实验**：

   ```cpp
   // 手动驱动：coro_A 的 await_suspend(void) 把 handle 存到 saved_handle
   // 调用者拿到这个 handle 后手动 .resume()
   auto task_a = coro_A();
   task_a.resume();  // 启动 A，在 co_await 处挂起
   // 此时 saved_handle 是 A 的 handle
   awaiter_void::saved_handle.resume();  // 手动 resume A
   // A 继续执行到结束
   ```

   对比：void 版本需要"中间人"手动 resume。symmetric transfer 版本不经过中间人。

4. **写三层嵌套协程链，分别用 void 和 symmetric transfer 实现**，对比栈深度。参考实验：

   ```cpp
   // N 层嵌套，每层 co_await 下一层的结果
   // 用直接 .resume() 链：栈深度可能随 N 线性增长
   // 用返回 coroutine_handle 的控制转交：通常可避免库层栈增长，需实测确认
   ```

5. **在笔记中画三张控制流图**，分别对应三种返回值。标注每个步骤中"谁的栈帧在执行"、"handle 被谁持有"、"谁负责 resume 谁"。

6. **控制流讨论**：symmetric transfer 在嵌套协程中解决了什么问题？
   - 无 symmetric transfer：A 的库代码直接调用 B 的 `.resume()`，深层嵌套可能导致栈溢出。
   - 有 symmetric transfer：A 不直接 resume B——A 从 `await_suspend` 返回 B 的 handle，让 `co_await` 变换恢复它。这是协程世界表达控制转交的标准方式；机器级 tail call 或恒定机器栈不是标准保证。

### 进阶任务

- 在 `awaiter_symmetric::await_suspend` 返回的目标 handle 的协程中，再次使用 symmetric transfer 跳回原始协程。这样形成一个"A <-> B"的对称跳转对，观察控制流在两个协程之间的无栈增长切换。
- 用 `-fsanitize=address` 和显式栈深度计数器测量 void 版本在 1000 层嵌套时的实际栈深度 vs symmetric transfer 版本的栈深度。
- 实现一个 `await_suspend` 返回 `bool` 的"条件挂起"awaiter：例如只有在某个条件满足时才挂起。观察这种模式在"轮询"场景中的应用。
- 研究 `std::suspend_always` 和 `std::suspend_never` 的源码——它们的 `await_suspend` 返回什么类型？为什么？

### 验收点

- 你能让每个 awaiter 类型在协程中实际工作，并通过日志观察到不同的控制流。
- 你能解释：为什么 `await_suspend` 返回 `void` 时，必须有"外部代码"负责 resume 当前协程——这个"外部代码"通常是调用者或事件循环。
- 你能解释：`await_suspend` 返回 `false`（不挂起）时，`await_ready` 返回 `false` 有什么意义？为什么不让 `await_ready` 直接返回 `true`？
- 你能画出三张控制流图并指出哪一段在哪个协程的栈帧中执行。
- 你能向同事解释：symmetric transfer 为什么不是"A 的库代码直接调用 B"，而是"`await_suspend` 返回 B 的 handle，由 `co_await` 变换恢复 B"。

### 观察点

- `await_suspend` 返回 `void` 是最常见、最灵活的模式——你可以把 coroutine_handle 存到任何地方（队列、condvar、回调），让任意线程在任意时间 resume。
- `await_suspend` 返回 `bool` 提供了"在最后一刻反悔"的能力。返回 `false`（不挂起）允许 awaitable 在已经进入 `await_suspend` 后根据运行时状态（如检查一个已完成的 future）决定不挂起而直接继续执行。返回 `true`（挂起）则与 void 行为一致——协程正式挂起，等待外部 resume。
- `await_suspend` 返回 `coroutine_handle` 是 symmetric transfer 的唯一入口。没有这个返回类型，symmetric transfer 就不可能存在——你无法在不经过自己栈帧的情况下"跳转"到另一个协程。
- 这三种返回值不是"三种风格"而是"三种语义契约"——它们决定了挂起后控制权的去向，进而决定了整个异步架构的组织方式。

### 常见坑

- 在 `await_suspend` 返回 `void` 的实现中忘记保存 `coroutine_handle`——协程挂起后没有任何引用指向它，协程永久挂起（泄漏）。
- 在返回 `bool` 的实现中返回 `true`（挂起）但忘记保存 handle——挂起后没有任何引用指向协程，协程永久挂起（泄漏）。
- 在返回 `coroutine_handle` 的实现中返回空 handle——编译器不会阻止你，但运行时 resume 空 handle 是 UB。
- 将 `await_suspend` 中的对称传输与 `final_suspend` 中的对称传输混淆——两者使用相同的机制，但触发时机不同（前者在协程体中间，后者在协程结束时）。
- 以为 symmetric transfer 是一种"库提供的功能"——它是编译器在 `co_await` 变换代码中实现的低级控制流。

### 提示

- 先用日志打印让控制流可见，再画图。
- 每种返回值写一个最小独立示例，不要混在一个协程里测——控制流会更清晰。特别提醒：`await_suspend` 返回 `bool true` = 挂起（同 void），`bool false` = 不挂起（立即继续）。请务必对照 cppreference 验证，不要在笔记中写反。
- symmetric transfer 实验需要两个协程相互配合，建议先画好"谁 resume 谁"的图再写代码。
- 返回值实验的 simple_task 不需要复杂的 promise_type——只需要 `initial_suspend = suspend_always`，`final_suspend = suspend_always`，不做 continuation 追踪即可。

### 复盘问题

- 如果 `await_suspend` 只能返回 `void`，整套协程生态会缺失哪些模式？
- symmetric transfer 是否可以替代所有返回 `void` 的模式？如果不能，哪些场景必须用 `void` 返回？
- 为什么 `await_suspend` 返回 `bool` 时不直接让 `await_ready` 来决策？——因为 `await_suspend` 拿到的是"协程 handle"，它可以根据只有此时才能获取的信息（比如检查一个锁是否已释放）做最终决定。
- 在 sender-receiver 框架中，`as_awaitable` 的 `await_suspend` 应该返回哪种类型？为什么？

### 对应官方参考

- Lewis Baker "C++ coroutines: Symmetric transfer"（系列第 5-6 篇）
- Raymond Chen "The await_suspend return type"（系列第 8-9 篇）
- P0913R1：symmetric transfer 的设计动机
- cppcoro `task.hpp` 中的 `await_suspend` 实现

---

## 练习 E-3：Trivial Awaitables 与短路优化

### 目标

实现 `await_ready()` 直接返回 `true` 的 awaitable，先证明标准保证的短路行为，再用编译器诊断/汇编观察优化器是否消除分支。建立“并非每次 `co_await` 都会挂起或进入调度器”的直觉，并把这件事与 HALO 的 frame allocation elision 分开验证。

### 前置理解

- 正常情况下，`co_await expr` 的变换代码包含：调用 `await_ready()`；如果 false，调用 `await_suspend()` 挂起；恢复后调用 `await_resume()`。这是标准的三段式。
- 如果 `await_ready()` 求值为 `true`，`[expr.await]` 保证不求值 `await_suspend`，随后求值 `await_resume()`。若优化器还能证明它恒为 true，可能进一步消除判断和不可达分支；是否生成相同机器码不是标准保证，也不能用固定优化级别概括。
- HALO（Heap Allocation eLision Optimization）处理 coroutine state 的 allocation elision；ready 短路处理单个 await-expression 的控制流。它们可能同时出现，但不是同一项语义或同一项优化，必须分别取证。
- `std::suspend_never` 就是标准库中最典型的 Trivial Awaitable——它的 `await_ready()` 始终返回 `true`。

### 必做任务

1. **实现几个不同粒度的 Trivial Awaitable**：

   ```cpp
   // 始终不挂起（等同 suspend_never）
   struct always_ready {
       bool await_ready() const noexcept { return true; }
       void await_suspend(std::coroutine_handle<>) const noexcept {}
       int await_resume() const noexcept { return 42; }
   };

   // 根据运行时条件决定是否挂起
   struct conditional_ready {
       bool should_suspend;  // 运行时参数

       bool await_ready() const noexcept {
           return !should_suspend;  // should_suspend=false 时返回 true
       }
       bool await_suspend(std::coroutine_handle<>) const noexcept {
           std::println("  [conditional_ready] await_suspend reached");
           return false; // 本练习只观察短路差异；返回 false 表示不保持挂起
       }
       int await_resume() const noexcept {
           return should_suspend ? 42 : 0;
       }
   };

   // 标准 C++20 awaiter：即使 await_ready 恒为 true，类型检查仍要求三方法都有效。
   struct trivial_awaitable {
       bool await_ready() const noexcept { return true; }
       void await_suspend(std::coroutine_handle<>) const noexcept {
           // 按 [expr.await]，运行时不会到达这里。
       }
       std::string await_resume() const { return "trivial-result"; }
   };
   ```

2. **写一个包含多个 `co_await always_ready{}` 的协程**：

   ```cpp
   simple_task many_noop_co_awaits() {
       std::println("  [task] before co_await 1");
       int v1 = co_await always_ready{};
       std::println("  [task] after co_await 1, v1={}", v1);
       int v2 = co_await always_ready{};
       std::println("  [task] after co_await 2, v2={}", v2);
       int v3 = co_await always_ready{};
       std::println("  [task] after co_await 3, v3={}", v3);
       co_return;
   }
   ```

   观察：
   - 程序执行是否是连续无间断的（像普通函数一样）？
   - 在 debug 模式下，协程帧是否仍然分配？（debug 模式通常不做优化）
   - 在 release 模式下，生成的汇编是否和"不加 co_await、直接调用函数"一样？

3. **用编译器 flag 检查优化效果**：

   Clang 用户：
   ```bash
   clang++ -std=c++23 -O2 -Rpass=coroutine-elide test.cpp -o test
   ```
   观察是否有 "coroutine frame elided" 的 remark。

   在 Godbolt 上写一个包含 `co_await always_ready{}` 的协程，观察生成的汇编：
   - 是否有 `call operator new`？
   - `await_suspend` 的代码是否还在二进制中？

4. **对比实验：Trivial vs Non-Trivial**：

   写两个版本，一个用 `co_await always_ready{}`，一个用 `co_await std::suspend_always{}`。分别测量 1000 万次调用的耗时：

   ```cpp
   // 版本 A：Trivial awaitable（每次 co_await 都 ready）
   simple_task version_A() {
       co_await always_ready{};
       co_await always_ready{};
       co_return;
   }
   // 版本 B：non-trivial（每次 co_await 都挂起）
   simple_task version_B() {
       co_await std::suspend_always{};
       co_await std::suspend_always{};
       co_return;
   }
   ```

   记录两个版本的每次迭代耗时。版本 A 应该显著快于版本 B（数量级差异）。

5. **区分语义保证与优化证据**：

   当前 C++ 标准要求 awaiter 的 `await_ready`、`await_suspend`、`await_resume` 表达式都良构；运行时 `await_ready()==true` 只保证跳过 `await_suspend`。在笔记中分别回答：
   - 哪一条是 `[expr.await]` 的可移植语义，哪一条只是编译器 as-if 优化？
   - 为什么 `await_suspend` 虽不被求值，表达式仍需在模板实例化/语义分析阶段良构？
   - HALO 消除的对象是什么？ready 短路消除的控制流是什么？怎样用不同证据验证二者？

### 进阶任务

- 写一个"缓存命中时不挂起，缓存未命中时挂起"的 awaiter，模拟异步数据加载。观察：在命中路径下，co_await 退化为了直接取值——性能与普通函数调用几乎无差别。
- 在协程中使用一个"大部分时间 ready，偶尔 suspend"的 awaiter，用 perf 或计时统计两种路径的比例。思考：为什么工程中通常设计为"优先走 ready 路径"？
- 研究 Clang/GCC 在什么优化级别下能对 Trivial Awaitable 做完全的挂起消除。尝试写一个无法优化的反例（例如 `await_ready` 的返回取决于一个 `volatile` 变量）。
- 用 `std::suspend_never` 替换你的 Trivial Awaitable，用编译器 flag 确认 HALO 是否在 `initial_suspend = suspend_never` 的情况下触发。

### 验收点

- 你能用计数器证明 ready 路径不调用 `await_suspend`，再用汇编说明当前编译器是否消除了分支。
- 你能解释 `std::suspend_never` 为什么是 Trivial Awaitable 的最典型代表——它的 `await_ready()` 永远返回 `true`。
- 你能区分 ready 短路、编译器死代码消除、HALO 三件事，不把“可能生成相同机器码”写成标准承诺。

### 观察点

- 并不是所有 `co_await` 都意味着昂贵的上下文切换。Trivial Awaitable 的 `await_ready` 返回 `true` 时，整个 `co_await` 表达式退化为对 `await_resume` 的直接调用——就像作用域守卫一样轻量。
- 编译器优化视图：`if (await_ready()) { /* skip suspend */ } else { /* suspend */ }` ——如果编译器能证明 await_ready 恒为 true，则整个 else 分支是死代码，可以被消除。
- 即使运行时语义固定，是否保留检查代码仍由优化器和可见性决定；以实际汇编或优化 remark 为证。
- 工程启示：设计 awaiter 时，如果多数情况下不需要挂起，把 `await_ready` 优先设为 `true`——这能让编译器在 fast path 上做更多优化。

### 常见坑

- 在 debug 或 -O0 模式下期望 Trivial Awaitable 产生优化——不会，编译器在无优化时保留完整的挂起路径。
- 把 `await_ready` 实现为 `return some_global_flag;`——编译器做常量折叠时，如果 global_flag 的初始化在另一个编译单元，编译器无法证明它恒为 true/false，挂起路径会保留。
- 以为 `await_ready` 返回 `true` 意味着 `await_resume` 不会被调用——恰恰相反，返回 `true` 意味着跳过 `await_suspend`，直接调用 `await_resume`。
- 因为 `await_ready` 恒为 true 就删掉 `await_suspend`——当前标准仍要求该表达式良构。

### 提示

- 使用 Godbolt 是确认优化效果的最高效方式——你可以同时看到源码、汇编和编译器 remark。
- 如果你本地无法触发 HALO，先确认你用了 -O2 且协程帧没有逃逸到外部。
- Trivial Awaitable 的观测重点不是"有没有语法糖"，而是"优化器如何利用 `await_ready` 的返回值做分支预测和死代码消除"。
- 把这道题和模块 F-3（HALO）联系起来看——Trivial Awaitable 和 HALO 的触发前提互为补充。

### 复盘问题

- 如果 C++ 标准从最初就把 `await_ready` 设为 `constexpr` 而不是运行时函数，对协程优化有什么影响？
- ready fast path 与 P3552 `task` 的 lazy 启动语义如何组合？一个 lazy task 开始后，内部 awaiter 仍可能同步 ready 吗？
- 在 HIGH 频繁 co_await 的热路径中，Trivial Awaitable 的优化对你的设计决策有什么影响？
- 为什么 `std::suspend_never` 是标准库中唯一一个名字明确表达"不做任何事"的内置 awaiter？

### 对应官方参考

- C++ working draft：`[expr.await]`、`[coroutine.trivial.awaitables]`
- Lewis Baker "C++ coroutines: Understanding the co_await operator"（系列第 4 篇）
- Raymond Chen "await_ready and the trivial awaitable"（系列第 7 篇）
- Gor Nishanov "HALO: Heap Allocation eLision Optimization" CppCon 2018
- `std::suspend_never` / `std::suspend_always` 的标准库实现

---

## 做完模块 E 之后，你现在应该能说清楚什么

至少把下面几句话说顺：

- `co_await expr` 在编译器中经历分层变换：普通 await expression 先查 promise scope 的 `await_transform`，再对结果做 `operator co_await` 重载决议；如果没有可行 operator，对象本身必须是 awaiter。`await_transform` 是 promise 唯一的全局拦截点——整个模块 H 的 sender 到协程的桥接都建立在这个机制之上。
- `await_suspend` 的三种返回值不是"三种风格"而是"三种契约"：`void` 把恢复责任交给外部代码；`bool` 允许在最后一刻反悔；`coroutine_handle<>` 实现 symmetric transfer，让控制权交给返回的目标协程。它能避免库代码直接嵌套 `.resume()` 的栈增长风险，但不承诺机器级 tail call 或恒定机器栈。
- `await_ready()` 返回 `true` 时，标准保证跳过 `await_suspend` 并调用 `await_resume`；优化器是否把分支完全消掉要看生成代码。HALO 则另行决定 coroutine state 的分配是否可消除。
- 模块 H 的 sender-receiver 桥接、模块 G 的 shared_task/when_all/sync_wait、模块 I 的 io_uring/IOCP awaiter——这一切都建立在本模块的三层 co_await 机制之上。不理解这三个层次，就看不懂任何高级协程基础设施的源码。
