# 10 模块 H：协程与 sender-receiver 桥接

## 模块目标

这个模块是整个 Coroutine_Study 学习包中**最关键**的模块——对应的 Execution_Study 模块 H-3 也是那个包的最深点。两条学习路线在这里交汇：

- 亲手实现 `as_awaitable` 桥：将任意的 sender 转换为协程可消费的 awaitable。bridge receiver 在 `set_value`/`set_error`/`set_stopped` 三个 completion channel 中 resume 协程。
- 使用 `std::execution::task<T>`（P3552R3）：理解 C++26 标准 task 的 environment propagation、scheduler affinity、以及 `co_await schedule(sch)` 切换调度器的完整语义。
- 实现双向互操作：让自定义协程 task 既能被 stdexec `sync_wait` 消费（实现 sender 协议），又能 `co_await` 任意的 stdexec sender（实现 await_transform）。task 同时是 awaitable 和 sender 的**双重身份**。

如果你不经过这一层，协程和 sender-receiver 就永远是"两套框架"，你只能选一个用——而 C++26 的设计目标是让它们无缝协作。

## 模块完成标准

做完本模块，你至少要能稳定说清楚：

- `as_awaitable(sender)` 内部到底发生了什么：从 bridge receiver 的构造，到 `connect(sender, bridge_receiver)`，到 `start(op)`，到 sender 完成时 `set_value/set_error/set_stopped` 中 resume 协程，到 `await_resume` 把值/异常还回协程体。
- bridge receiver 为什么是"协程 <-> sender" 的枢纽——它的三条 completion channel 如何与协程的 resume/异常传播精确对应。
- `std::execution::task<T>`（P3552R3）与普通 `lazy_task<T>` 的本质区别：environment 感知、scheduler affinity、与 when_all 等组合子的原生协作。
- 为什么 `await_transform(Sender)` 是实现 task `co_await sender` 的关键——它把 sender 翻译为 sender_awaitable，在 `await_suspend` 中完成 connect + start。
- task 如何同时成为 awaitable 和 sender——双向桥接如何确保 composer（when_all/sync_wait）和 consumer（co_await）都能使用同一个 task 类型。
- `completion_signatures` 在协程侧的合同作用——让 sender 的组合子（如 `then`、`let_value`）在编译期就能验证协程可以安全消费某个 sender。

## 使用建议

- 本模块概念密度极高，建议三道题各预留半天。先画对象关系图再写代码。
- H-1（as_awaitable）是本模块的基石——把它跑通后，H-2 和 H-3 的骨架你都能预测。
- 如果你对 sender-receiver 框架（connect、start、completion_signatures）还不熟，先回顾 Execution_Study 模块 D 和模块 F。
- 如果你对协程的 promise_type 和 co_await 变换还不熟，先回顾本工程模块 D 和模块 E。
- 强烈建议在每一步都画出调用链：协程端 co_await expr → await_transform → sender_awaitable → await_suspend(connect+start) → 挂起 → bridge_receiver 收到 completion → resume → await_resume。

---

## 练习 H-1：as_awaitable 桥

### 目标

亲手实现 `as_awaitable(sender) -> awaitable` 的完整桥接通道。bridge receiver 在 `set_value`、`set_error`、`set_stopped` 中分别 resume 协程，并在 `await_resume` 中把三种 completion 翻译回协程的 return/throw/特殊标记。这是整个协程-sender 互操作的基础。

### 前置理解

- 你已经理解协程的 `co_await` 机制（模块 E）：`await_ready() -> false` 触发 `await_suspend(handle)`，此时协程挂起；后续某个时机 `handle.resume()` 恢复协程，执行 `await_resume()` 取出结果。
- 你已经理解 sender-receiver 协议（Execution_Study 模块 D）：sender 描述任务 → `connect(sender, receiver)` 返回 operation_state → `start(op)` 实际启动 → sender 内部通过调用 receiver 的 `set_value`/`set_error`/`set_stopped` 报告完成。
- 桥接对应关系（在本练习中你可以逐行对照实现）：

  | 协程端 | sender-receiver 端 |
  |--------|-------------------|
  | `co_await awaitable` | `sender` 是待执行的异步任务 |
  | `await_ready() -> false`（挂起决定） | `connect(sender, bridge_receiver)` 准备执行 |
  | `await_suspend(handle)` 挂起协程 | `start(op)` 启动 sender 执行 |
  | 协程挂起，等待恢复 | sender 异步执行 |
  | `handle.resume()` 恢复协程 | `set_value/set_error/set_stopped` 被 sender 调用 |
  | `await_resume()` 返回结果 | bridge receiver 已将结果存入共享存储 |

- 关键设计决策：operation_state 必须存活到 sender 完成。如果你把它放在 `await_suspend` 的栈帧中，它会在 `await_suspend` 返回时析构——而此时 sender 可能还没有开始执行。

### 必做任务

1. **实现 bridge receiver**——连接 sender completion 和协程 resume 的枢纽：

   ```cpp
   template<typename Promise>
   struct bridge_receiver {
       using receiver_concept = stdexec::receiver_t;

       std::coroutine_handle<Promise> coro_;

       // 共享存储：存放 sender 产生的结果
       // monostate = 未完成, T = 值, exception_ptr = 错误, stopped_tag = 停止
       struct stopped_tag {};
       using storage_type = std::variant<
           std::monostate,
           /* 值类型 —— 由 sender_awaitable 模板参数决定 */,
           std::exception_ptr,
           stopped_tag
       >;
       storage_type* storage_;  // 指向 sender_awaitable 中的存储

       // ---- value channel ----
       // sender 正常完成，把值存入 storage，然后 resume 协程
       friend void tag_invoke(stdexec::set_value_t, bridge_receiver&& self,
                              auto&&... values) {
           // 用具名 emplace 替代位置 emplace，避免 variant 重排后静默错位
           self.storage_->template emplace<value_type>(
               std::forward<decltype(values)>(values)...
           );
           self.coro_.resume();
       }

       // ---- error channel ----
       // sender 以 error 完成，存储错误指针，然后 resume 协程
       friend void tag_invoke(stdexec::set_error_t, bridge_receiver&& self,
                              auto&& error) {
           // 将错误转为 exception_ptr
           if constexpr (std::is_same_v<std::decay_t<decltype(error)>,
                                        std::exception_ptr>) {
               self.storage_->template emplace<std::exception_ptr>(error);
           } else {
               // 非 exception_ptr 的错误类型：用 make_exception_ptr 包装
               try {
                   throw std::move(error);
               } catch (...) {
                   self.storage_->template emplace<std::exception_ptr>(
                       std::current_exception());
               }
           }
           self.coro_.resume();
       }

       // ---- stopped channel ----
       // sender 被取消/停止，标记 stopped，然后 resume 协程
       friend void tag_invoke(stdexec::set_stopped_t, bridge_receiver&& self) noexcept {
           self.storage_->template emplace<stopped_tag>(stopped_tag{});
           self.coro_.resume();
       }

       // ---- environment query ----
       // 从协程的 promise 获取 environment（最小版本：返回空 env）
       // 进阶任务可改为转发协程 promise 的 scheduler / stop_token
       friend auto tag_invoke(stdexec::get_env_t, const bridge_receiver& /*self*/)
           -> stdexec::empty_env { return {}; }
   };
   ```

   注意：上述骨架使用了 `tag_invoke` 风格。如果你的 stdexec 版本支持成员函数接入，也可以使用 `set_value(values...)`、`set_error(error)`、`set_stopped()` 的成员函数写法。关键是三条 channel 全部覆盖且最终都要 `resume` 协程。

2. **实现 `sender_awaitable`**——包装 bridge receiver 和 operation_state：

   ```cpp
   template<typename Sender, typename Promise>
   struct sender_awaitable {
       using receiver_type = bridge_receiver<Promise>;

       Sender sender_;
       std::coroutine_handle<Promise> coro_;

       // 存储结构
       // 最小可编译版本：硬编码 value_type = int（用于 co_await stdexec::just(42)）
       // 通用 completion_signatures 推导放进阶任务
       using value_type = int;
       using storage_type = std::variant<
           std::monostate,
           value_type,
           std::exception_ptr,
           typename bridge_receiver<Promise>::stopped_tag
       >;
       storage_type storage_;

       // operation_state 的存储——必须与 sender_awaitable 同生命周期
       // 使用 std::optional 延迟构造（在 await_suspend 中构造）
       using op_state_t = stdexec::connect_result_t<
           Sender,
           bridge_receiver<Promise>
       >;
       std::optional<op_state_t> op_state_;

       // ---- awaitable 接口 ----

       bool await_ready() noexcept {
           // sender 不能同步完成（在 await_suspend 之前就知道结果）
           // 总是挂起，等待 sender 完成
           return false;
       }

       auto await_suspend(std::coroutine_handle<Promise> h) noexcept {
           // 构造 bridge receiver
           bridge_receiver<Promise> rcvr{h, &storage_};

           // connect(sender, bridge_receiver) → operation_state
           // 在 optional 中就地构造
           op_state_.emplace(
               stdexec::connect(std::move(sender_), std::move(rcvr))
           );

           // start(operation_state) —— 启动 sender 执行
           stdexec::start(*op_state_);

           // 协程挂起，控制权返回给调用者
           // 当 sender 完成后，bridge receiver 会 resume 这个协程
           // （通过 symmetric transfer 可以避免直接返回）
       }

       value_type await_resume() {
           // 协程被 resume 后，检查 storage 的状态
           if (std::holds_alternative<value_type>(storage_)) {
               return std::move(std::get<value_type>(storage_));
           }
           if (std::holds_alternative<std::exception_ptr>(storage_)) {
               std::rethrow_exception(
                   std::get<std::exception_ptr>(storage_));
           }
           if (std::holds_alternative<
                   typename bridge_receiver<Promise>::stopped_tag
               >(storage_)) {
               // 处理 stopped：可以抛特殊异常或返回 std::optional
               throw std::runtime_error("sender stopped");
           }
           // 如果是 monostate：不应该到达这里
           throw std::logic_error("sender_awaitable resumed without completion");
       }
   };
   ```

   关键细节：
   - `op_state_` 使用 `std::optional` 延迟构造——在 `await_ready` 时还没 connect，在 `await_suspend` 中才 connect + start。
   - `storage_` 必须是 `sender_awaitable` 的成员（不是 bridge receiver 的成员），它必须活过 `await_suspend` 返回直到 `await_resume` 被调用。
   - `await_suspend` 返回 void，意味着协程挂起后控制权回到调用 `resume` 的那个实体。这在 single-thread 环境下是安全的——sender 完成时会 resume 同一个协程。

3. **实现 `as_awaitable` 函数**：

   ```cpp
   template<typename Sender, typename Promise>
   auto as_awaitable(Sender&& sender, std::coroutine_handle<Promise> h) {
       return sender_awaitable<
           std::remove_cvref_t<Sender>,
           std::remove_cvref_t<Promise>
       >{std::forward<Sender>(sender), h};
   }
   ```

   在更完整的实现中，`as_awaitable` 还需要：
   - 从 sender 的 `completion_signatures` 推导 `value_type`
   - 处理 sender 有多个 value channel 重载的情况（如 `set_value_t(int)` 和 `set_value_t(std::string)`）
   - 处理 `set_value_t()` 无参完成（此时 `await_resume` 返回 void）

4. **在 task 的 promise_type 中接入 `await_transform`**：

   ```cpp
   // 在 my_task<T> 的 promise_type 中
   template<typename Sender>
   auto await_transform(Sender&& sender) {
       // 将 sender 转换为 awaitable
       return as_awaitable(
           std::forward<Sender>(sender),
           std::coroutine_handle<promise_type>::from_promise(*this)
       );
   }
   ```

   这样，在 `my_task<T>` 的协程体中，任何 `co_await some_sender` 都会自动走 `await_transform`，把 sender 翻译为 `sender_awaitable`，然后在 `await_suspend` 中 connect + start。

5. **编写完整验证测试**：

   ```cpp
   my_task<int> bridge_demo() {
       // co_await 一个 sender——走 await_transform -> as_awaitable -> connect + start
       int x = co_await stdexec::just(42);
       int y = co_await stdexec::just(10);
       co_return x + y;  // 期望 52
   }

   int main() {
       auto result = sync_wait(bridge_demo());
       std::println("bridge_demo result: {}", result);  // 52
       return 0;
   }
   ```

6. **验证错误路径**：

   ```cpp
   my_task<int> error_demo() {
       try {
           co_await stdexec::just_error(
               std::make_exception_ptr(std::runtime_error("whoops"))
           );
           co_return 0;  // 不会执行到这里
       } catch (const std::runtime_error& e) {
           std::println("caught: {}", e.what());
           co_return -1;
       }
   }
   ```

7. **验证 stopped 路径**：

   ```cpp
   my_task<int> stopped_demo() {
       co_await stdexec::just_stopped();
       co_return 0;  // 不会执行到这里
       // 注意：just_stopped 挂起后，bridge receiver 收到 set_stopped，
       // await_resume 发现 stopped_tag 并抛异常
   }
   ```

8. **在笔记中画出完整的桥接调用链**：

   ```
   [协程体]  co_await some_sender
     |
     v
   [编译器]   调用 promise.await_transform(some_sender)
     |                   |
     |                   v
     |         as_awaitable(sender, coro_handle)
     |                   |
     |                   v
     |         sender_awaitable{sender, coro_handle}
     |
     v
   [运行时]   sender_awaitable::await_ready() -> false
     |
     v
              sender_awaitable::await_suspend(coro_handle)
                |
                |-- 构造 bridge_receiver{coro_handle, &storage_}
                |-- connect(sender, bridge_receiver) -> op_state
                |-- start(op_state)
                |-- 返回 void，协程挂起
     |
     v
   [Sender 异步执行...]
     |
     |-- set_value(values...) --> storage_ = values; coro_.resume()
     |   或
     |-- set_error(error)     --> storage_ = error_ptr; coro_.resume()
     |   或
     |-- set_stopped()        --> storage_ = stopped_tag; coro_.resume()
     |
     v
   [协程恢复] sender_awaitable::await_resume()
                |
                |-- 如果 storage_ 是值: 返回它
                |-- 如果 storage_ 是异常: rethrow
                |-- 如果 storage_ 是 stopped: 抛异常或特殊处理
     |
     v
   [协程体]   赋给 co_await 左侧的变量，继续执行
   ```

### 进阶任务

- 让 bridge receiver 支持多参数 `set_value(values...)`——当 sender 产生多个值时（如 `just(1, "hello")`），如何在 `await_resume` 中取出？选项：用 `std::tuple` 包装多值；或要求 sender 只能产生单值（标准 task 的选择）。
- 实现 `await_suspend` 的 symmetric transfer 版本：当 sender 同步完成时（`start` 中直接调用 `set_value`），`await_suspend` 不应返回 void——它应该返回一个 `coroutine_handle`（symmetric transfer 回当前协程）或 `bool(true)`（不挂起）。
- 为 bridge receiver 的 `get_env()` 实现环境转发：从协程的 promise（通过 `h.promise()`）获取 environment，转发 sender 的环境查询请求。这确保了 sender 可以访问协程的调度器、stop_token 等上下文。
- 支持 sender 的 `connect` 结果类型推导——用 `stdexec::connect_result_t<Sender, bridge_receiver<Promise>>` 替代硬编码的 `op_state_` 类型。
- 处理 `co_await` 一个 void 返回的 sender（`set_value_t()` 签名）：`await_resume` 应该返回 `void` 而不是值。

### 验收点

- 你的 `my_task<int>` 协程能 `co_await stdexec::just(42)` 并正确拿到结果。
- 错误路径正确：`co_await just_error(...)` 能在 `await_resume` 中 rethrow 异常。
- 你能画出从 `co_await sender` 到 `await_resume` 返回结果的完整调用链（至少 8 个步骤）。
- 你的 bridge receiver 覆盖了全部三条 completion channel（value/error/stopped）。
- `op_state_` 的生命周期安全——它在 `await_suspend` 中构造，在 `await_resume` 返回后析构。

### 观察点

- `await_transform` 是协程与 sender-receiver 之间的**翻译层**：它把 sender 的语言翻译成协程的语言。这个翻译不需要修改 sender 的源码——任何满足 sender 概念的类型都能被自动桥接。
- bridge receiver 是整个桥接的**枢纽零件**——它是 sender 的"回调"，同时是协程的"唤醒者"。它的三条 completion channel 就像是三根电线，把 sender 的信号传到协程的 resume 电路上。
- `as_awaitable` 本质上是一个**适配器模式**的实现——它把 sender 协议适配为 awaitable 协议。适配的核心是"将 completion channel 转换为 resume + 结果存储"。
- `operation_state` 的生命周期必须在 `sender_awaitable` 中管理——你不能在 `await_suspend` 的栈上创建它，因为 `await_suspend` 返回后协程已挂起，op_state 需要在 sender 完成时仍然存活。
- 这个桥接模式和 Execution_Study 模块 H-3 的协程桥接在结构上精确对应——只是那边的 sender 端是主角，这边的协程端是主角。两者组合就构成了 H-3 的双向桥接。

### 常见坑

- `operation_state` 在 `await_suspend` 的栈帧中创建为局部变量——`await_suspend` 返回后它被析构，但 sender 可能还在异步执行。
- bridge receiver 以引用方式持有 `storage_` 和 `coro_`，但引用在 receiver 被 move 后悬空——需要确保 bridge receiver 和 sender_awaitable 的生命周期一致。
- sender 在 `start()` 中同步调用 `set_value`，导致协程在 `await_suspend` 返回前就被 resume——产生 use-after-return。解决方案：`await_suspend` 返回 `true`（不挂起）或使用 symmetric transfer。
- `await_transform` 只拦截了特定 sender 类型，对不认识的类型没有重载——导致编译器走成员 `operator co_await` 或 ADL 路径，绕过了桥接机制。
- `set_value` 的 `std::variant` index 写死为 1——如果 `value_type` 在 variant 中的实际位置不是第 2 个，编译错误或运行时选错 alternative。

### 提示

- 从"支持单个已知 sender 类型 + 单值返回"的硬编码版本开始跑通，再泛化。
- 先让 value channel 跑通（co_await just(42) 拿到 42），再加 error 和 stopped channel。
- `std::optional` 的 `emplace` 是管理 op_state 延迟构造的便捷方式。
- 如果 stdexec 不在本地，你可以用一个自定义的"最小 sender"（只包含 `connect` + `completion_signatures` + 一个直接调用 set_value 的 op state）来验证逻辑。
- `set_value(values...)` 的多值转发可以用 `std::forward_as_tuple` + `std::apply` 来打包/解包。

### 复盘问题

- 为什么 `as_awaitable` 不能简单地让 sender 直接实现 awaitable 的三个方法？
- bridge receiver 的存在是"不得已"还是"精确对应"？——sender-receiver 和 coroutine 在结构上是否存在根本差异？
- 如果 sender 在 `start()` 中同步完成，`await_suspend` 应该返回什么？为什么 void 返回会导致问题？
- 这个 `sender_awaitable` 和标准提案 P3175R0 中的 `as_awaitable` 在哪些方面一致，哪些方面有差异？

### 对应官方参考

- P2300R10：`std::execution` 中的 `connect`、`start`、completion channel 规范
- P3175R0：`as_awaitable` 的标准化提案
- `stdexec` 源码中 `__connect_awaitable.hpp` 和 `__sender_for_each_awaitable.hpp`
- LWG #4339：`as_awaitable` 的返回值修正
- LWG #4356：`as_awaitable` 的错误处理修正

---

## 练习 H-2：用 std::execution::task<T>

### 目标

使用 P3552R3 定义的 `std::execution::task<T>`（实际通过 stdexec 的 `stdexec::task<T>` 体验），理解 C++26 标准 task 的核心特性：scheduler affinity、environment propagation、`co_await schedule(sch)` 切换调度器、以及 `co_await when_all` 的组合语义。

### 前置理解

- 你已经完成 H-1，理解 `await_transform(sender)` 如何把 sender 转换为 awaitable。
- 你知道标准 task 的核心设计决定（P3552R3）：
  1. **Lazy**：`initial_suspend` 返回 `suspend_always`——task 被提交到调度器后才开始执行。
  2. **Scheduler-aware**：task 通过 promise 的 `get_env()` 暴露它所运行的 scheduler。`co_await schedule(sch)` 可以切换调度器。
  3. **env_promise**：promise 的 `get_env()` 返回一个包含 scheduler 的 environment——这就是 scheduler affinity 的实现。
  4. **Completion_signatures**：task 作为 sender，声明自己的 completion_signatures——这让它能够被 `then`、`when_all` 等组合子安全消费。
- 标准 task 和模块 D 的 `lazy_task<T>` 本质差别：

  | 特性 | lazy_task<T> (模块 D) | std::execution::task<T> (P3552) |
  |------|----------------------|-------------------------------|
  | 启动 | 手动 resume() | 通过 schedule(sch) 提交到调度器 |
  | scheduler | 无（单线程） | 有（通过 environment 传播） |
  | 可被 sync_wait 消费 | 否（需自己实现 awaitable 接口） | 是（既是 awaitable 又是 sender） |
  | completion_signatures | 无 | 有（类型级合同） |
  | stop_token | 不支持 | 通过 environment 传播 |

### 必做任务

1. **用 stdexec 的标准 task 写调度器切换实验**：

   ```cpp
   #include <stdexec/exec/task.hpp>
   // stdexec 的 single_thread_context 在 exec:: 命名空间下
   #include <stdexec/exec/single_thread_context.hpp>

   stdexec::task<int> scheduler_aware_compute() {
       // task 启动在默认环境下（通过 sync_wait 或 when_all 传入的 sch）

       std::println("[start] on thread {}", std::this_thread::get_id());

       // 获取当前调度器，并创建一个"在另一个线程上运行"的子任务
       // 使用 schedule() 切换调度器
       auto sch = co_await stdexec::get_scheduler();

       // 在一些实现中，schedule(sch) 返回一个 sender
       // co_await 它会挂起当前协程，由 sch 在合适的时候 resume
       co_await stdexec::schedule(sch);

       std::println("[after schedule] on thread {}", std::this_thread::get_id());

       // 在调度器的执行上下文上做一些计算
       int result = 42;

       // 切换到另一个调度器
       // 注意：stdexec 中 single_thread_context 的命名空间是 exec::
       // exec::single_thread_context ctx;
       // auto sch2 = ctx.get_scheduler();
       // co_await stdexec::schedule(sch2);

       co_return result;
   }
   ```

   关键观察：
   - `co_await stdexec::schedule(sch)` 的语义：挂起当前协程，将恢复任务提交给调度器 `sch`。当 `sch` 决定恢复时（可能在不同线程），协程从挂起点之后继续执行。
   - scheduler affinity：task "记住"它最后一个 `co_await schedule(sch)` 设置的调度器——后续的异步操作（如 timer、网络 I/O）都在这个调度器上恢复。

2. **实现 `get_scheduler` 的自定义查询**：

   ```cpp
   // 自定义 task 中，通过 promise.get_env() 暴露 scheduler
   struct my_env_task {
       struct promise_type {
           // ...

           // 环境查询：告诉 sender 我运行在哪个 scheduler 上
           auto get_env() const noexcept {
               // 返回一个包含 scheduler 的 environment
               return stdexec::prop{stdexec::get_scheduler, current_scheduler_};
           }

           // 默认 scheduler（在 sync_wait 或其他入口传入）
           stdexec::any_scheduler current_scheduler_;
       };
   };
   ```

   `get_env()` 是协程向 sender 传递执行上下文的**唯一通道**。sender 通过 `get_env(receiver)` 查询到 scheduler 后，就知道应该在哪个执行资源上完成任务。

3. **验证 when_all + task 的并行组合**：

   ```cpp
   stdexec::task<int> fetch_a() {
       co_await /* 模拟异步操作 */;
       co_return 10;
   }

   stdexec::task<int> fetch_b() {
       co_await /* 模拟异步操作 */;
       co_return 20;
   }

   stdexec::task<int> parallel_fetch() {
       // 用 when_all 并行执行两个子 task
       auto [a, b] = co_await stdexec::when_all(
           fetch_a(),
           fetch_b()
       );
       co_return a + b;  // 30
   }

   int main() {
       auto result = stdexec::sync_wait(parallel_fetch());
       // result: 30
   }
   ```

   关键观察：
   - `when_all` 能够接受 task 是因为 task 是一个 sender（它实现了 sender 协议）。
   - `co_await when_all(...)` 能够工作是因为 `when_all` 返回一个 sender，而 task 的 promise 有 `await_transform(sender)`。
   - 整个组合在类型层面是安全的：`when_all` 检查各子 sender 的 `completion_signatures`，编译期推导出结果的 tuple 类型。

4. **在笔记中画出 task 的完整环境传播链**：

   ```
   sync_wait(parallel_fetch())
     |
     | sync_wait 的 receiver 提供 initial environment（包含 scheduler）
     v
   connect(parallel_fetch_task, sync_wait_receiver)
     |
     | 通过 receiver.get_env() 获取 scheduler
     | 传递给 task 的 promise（env_promise 机制）
     v
   [task 启动] initial_suspend → await_suspend 拿到 scheduler
     |
     v
   [协程体] co_await when_all(fetch_a, fetch_b)
     |
     | when_all 返回的 sender 内部 connect 子 task
     | 每个子 task 的 promise 通过 get_env() 向 when_all sender 暴露 scheduler
     | sender 将子 task 的恢复调度到对应的 scheduler 上
     v
   [结果汇合] when_all 的 await_resume 返回 tuple
   ```

5. **讨论 scheduler affinity 语义**：

   scheduler affinity 的核心约定：**task 承诺总是在它最近一次 `co_await schedule(sch)` 指定的 scheduler 上恢复执行**。这意味着：
   - 在 task 中访问的局部变量不需要加锁——因为只有一个线程在运行该 task（scheduler 保证串行化）。
   - 但嵌套协程（通过 `co_await task` 调用子 task）可能在不同的 scheduler 上恢复——需要显式 `co_await schedule(sch)` 来"拉回"当前 scheduler。

### 进阶任务

- 实现自定义的 `my_scheduler`（类似 Execution_Study H-1 的 run_loop），让 task 的 scheduler affinity 指向你的调度器。观察在不同调度器之间切换时的线程 ID 变化。
- 为你的 `my_env_task` 添加 stop_token 环境传播：promise 的 `get_env()` 同时暴露 scheduler 和 stop_token。验证 `co_await just_stopped()` 或 cancellation 后的路径。
- 使用 `stdexec::then`、`stdexec::let_value` 等组合子与 task 混合使用——task 作为 sender 参与组合链，然后链的结果被另一个协程 `co_await`。
- 研究 `stdexec/exec/task.hpp` 源码，标注出 `env_promise`、`await_transform`、`completion_signatures` 的实际实现位置。

### 验收点

- 你能在 task 中使用 `co_await schedule(sch)` 切换调度器，并观察到线程 ID 的变化。
- 你能说明 `get_env()` 为什么是协程到 sender 环境传播的唯一通道。
- 你能用 `when_all` 并行组合多个 task，并用 `sync_wait` 消费。
- 你理解 scheduler affinity 的约定——它保证了什么，有什么限制。

### 观察点

- 标准 task 的 scheduler affinity 让协程代码"感觉像单线程"，即使实际运行可能跨越多个线程——这就是结构化并发的协程表达。
- `co_await schedule(sch)` 就像是 `std::this_thread::sleep_for` 的异步版本——它把"我在哪里运行"从一个隐式假设变成了一个显式操作。
- `env_promise` 机制是 P3552 的核心创新——它让协程的 promise 参与到 sender-receiver 的环境传播链中，从而打通了"调度上下文从 sender 流向协程"和"协程上下文流向 sender"两个方向。
- task 同时是 sender 和 awaitable——这双重身份让它成为 sender-receiver 生态和协程生态之间的**通用货币**。

### 常见坑

- 在 task 的协程体中不 `co_await schedule(sch)` 就假设自己在某个特定线程上——scheduler affinity 只知道"上一次 schedule 在哪个 sch"，如果没有显式 schedule，就是不确定。
- 用 `co_await task` 调用子 task 后，子 task 的 scheduler 和当前 task 的 scheduler 不一致——子 task 的结果在"别人的线程"上回调，当前 task 的恢复可能还在等待。
- 忘了在 `get_env()` 中转发 `get_stop_token` 等查询——scheduler 之外的 query 被吞掉了。
- `completion_signatures` 写错导致组合子（`then`、`when_all`）编译报错——sender 协议要求类型级合同的精确匹配。

### 提示

- 先用 `stdexec::task<T>` 跑通三个基本场景（单 task、调度器切换、when_all 并行），再尝试自定义。
- 画环境传播图时，标注三个关键节点：sync_wait 的 env → task promise 的 env → 子 sender 的 env。
- Scheduler affinity 的约定理解比语法细节更重要——花时间理解"为什么"而不是"怎么用"。

### 复盘问题

- 为什么 P3552 把 task 设计为 lazy 启动，而不是 eager？eager 启动会破坏哪些组合语义？
- scheduler affinity 的约定在实际工程中是如何减少同步开销的？
- 如果 task 的 `get_env()` 返回了一个错误的 scheduler，会对谁（task 自身？嵌套子 task？组合子？）产生影响？
- P3552 的 task 和模块 D 的 lazy_task 在什么场景下不能互相替代？

### 对应官方参考

- P3552R3：`std::execution::task<T>` 提案
- P2300R10：`std::execution` 中 task 和 scheduler 的语义
- `stdexec/exec/task.hpp`：实际实现参考
- `stdexec/examples/` 中的 hello_coro 和 schedule 示例

---

## 练习 H-3：与 stdexec sender 互操作

### 目标

让自定义协程 task（基于模块 D 的 `lazy_task<T>` 扩展）同时具备两种身份：
1. **作为 sender**：能被 `stdexec::sync_wait` 消费，能被 `then`、`when_all` 等组合子使用。
2. **作为协程**：能 `co_await` 任意的 stdexec sender（通过 `await_transform` 桥接）。

实现完整的双向桥接，让 `my_task<T>` 成为 sender-receiver 生态和协程生态之间的**通用货币**——task 同时是 awaitable 和 sender 的**双重身份**。

### 前置理解

- 你已经完成 H-1（as_awaitable 桥）和 H-2（标准 task 的语义）。
- 你理解 sender 概念的要求（Execution_Study 模块 D）：
  - `sender_concept = stdexec::sender_t`
  - `completion_signatures` 类型成员
  - `connect(sender, receiver) -> operation_state`
- 你理解 awaitable 协议（本工程模块 E）：
  - `await_ready()` / `await_suspend()` / `await_resume()`
- 双向桥接意味着：**同一个类型 `my_task<T>` 既实现了 sender 协议，也实现了 awaitable 协议**。这是一对二的映射。
- 核心挑战：
  1. 实现 `connect(my_task, receiver)` —— task 作为 sender 被连接。
  2. 实现 `co_await my_task` —— task 作为 awaitable 被消费。
  3. 实现 `co_await some_sender` —— 在 my_task 协程体内消费 sender（通过 H-1 的 `await_transform`/`as_awaitable`）。

### 必做任务

1. **扩展 `my_task<T>` 实现 sender 协议**：

   ```cpp
   template<typename T>
   struct my_task {
       // === sender 协议 ===
       using sender_concept = stdexec::sender_t;

       // completion_signatures：告诉框架这个 task 会产生什么
       using completion_signatures = stdexec::completion_signatures<
           stdexec::set_value_t(T),
           stdexec::set_error_t(std::exception_ptr),
           stdexec::set_stopped_t()
       >;

       // connect：允许 sync_wait / then / when_all 等消费此 task
       // 实现：把 my_task 和外部 receiver 连接
       template<typename Receiver>
       friend auto tag_invoke(stdexec::connect_t, my_task self, Receiver rcvr) {
           return task_op_state<T, Receiver>{
               std::move(self.h_),
               std::move(rcvr)
           };
       }
   };
   ```

2. **实现 `task_op_state<S, Receiver>`**——从 sender 侧驱动协程：

   ```cpp
   template<typename T, typename Receiver>
   struct task_op_state {
       using operation_state_concept = stdexec::operation_state_t;

       std::coroutine_handle<typename my_task<T>::promise_type> coro_;
       Receiver rcvr_;
       bool started_ = false;

       // start：实现 operation_state 协议
       friend void tag_invoke(stdexec::start_t, task_op_state& self) noexcept {
           // 设置 continuation 和 external_receiver_
           auto& p = self.coro_.promise();
           // continuation 仅在 task 被 co_await 时使用；
           // sender 路径下不需要协程链 continuation，设为 noop
           p.continuation = std::noop_coroutine();
           p.external_receiver_ = &self.rcvr_;

           // 启动协程
           self.coro_.resume();
       }
   };
   ```

   在 `final_suspend` 中，将协程的结果转发给 external receiver：

   ```cpp
   auto final_suspend() noexcept {
       struct final_awaiter {
           bool await_ready() noexcept { return false; }

           std::coroutine_handle<> await_suspend(
               std::coroutine_handle<promise_type> h) noexcept
           {
               auto& p = h.promise();

               if (p.external_receiver_) {
                   // 协程被 sync_wait / then / when_all 等消费
                   if (p.result_exception) {
                       stdexec::set_error(
                           std::move(*p.external_receiver_),
                           std::move(p.result_exception)
                       );
                   } else {
                       stdexec::set_value(
                           std::move(*p.external_receiver_),
                           std::move(p.result_value)
                       );
                   }
               }

               // 同时检查内部 continuation（如果有协程在 co_await 这个 task）
               if (p.continuation)
                   return p.continuation;
               return std::noop_coroutine();
           }
           void await_resume() noexcept {}
       };
       return final_awaiter{};
   }
   ```

   注意这里的关键设计：`external_receiver_` 是 sender 链上的下游 receiver（如 sync_wait 的 receiver）；`continuation` 是协程链上的等待者（如 co_await 这个 task 的协程）。当 task 被 sender 路径消费时（`start()` 调用），`continuation` 设为 `std::noop_coroutine()`——final_suspend 中先通知 `external_receiver_` 完成 sender 协议，再 `return continuation`（noop 等价于不转移控制权）。当 task 被协程 `co_await` 时，`continuation` 指向等待者协程——final_suspend 通过 symmetric transfer 跳转到等待者。**同一个 final_suspend 需要处理两套完成通知机制**——这正是 task 双重身份的体现。

3. **实现 `await_transform(Sender)`**——让 my_task 能 co_await sender：

   这一部分完全复用 H-1 的实现：

   ```cpp
   // 在 promise_type 中
   template<typename Sender>
   auto await_transform(Sender&& sender) {
       return as_awaitable(
           std::forward<Sender>(sender),
           std::coroutine_handle<promise_type>::from_promise(*this)
       );
   }
   ```

4. **实现 task 作为 awaitable 的接口**——让其他协程能 `co_await my_task`：

   ```cpp
   // my_task<T> 的 awaitable 接口（复用模块 D/E 的骨架）
   bool await_ready() noexcept { return h_.done(); }

   template<typename Promise>
   auto await_suspend(std::coroutine_handle<Promise> caller) noexcept {
       h_.promise().continuation = caller;
       return h_;  // symmetric transfer
   }

   T await_resume() {
       auto& p = h_.promise();
       if (p.result_exception)
           std::rethrow_exception(p.result_exception);
       return std::move(p.result_value);
   }
   ```

5. **端到端验证三个场景**：

   场景 A：my_task 被 sync_wait 消费（sender 侧）

   ```cpp
   my_task<int> sender_side() {
       co_return 42;
   }
   auto v = stdexec::sync_wait(sender_side());  // 走 sender 协议
   ```

   场景 B：my_task 被协程 co_await 消费（awaitable 侧）

   ```cpp
   my_task<int> inner() { co_return 10; }
   my_task<int> outer() {
       int v = co_await inner();  // 走 awaitable 协议
       co_return v * 2;
   }
   ```

   场景 C：my_task 协程体内部 co_await stdexec sender（桥接侧）

   ```cpp
   my_task<int> bridge_side() {
       int v = co_await stdexec::just(42);  // 走 await_transform -> as_awaitable
       co_return v + 1;
   }
   auto r = stdexec::sync_wait(bridge_side());  // 43
   ```

6. **验证组合子链**：

   ```cpp
   // my_task 参与 stdexec 的 sender 组合链
   auto r = stdexec::sync_wait(
       my_task<int>([]() -> my_task<int> { co_return 10; }())
       | stdexec::then([](int x) { return x * 3; })
   );
   // r = 30
   ```

7. **在笔记中画出双向桥接的完整对象图**：

   ```
   [sync_wait 消费 my_task]                     [coroutine 内 co_await sender]
   
   sync_wait(my_task)                          my_task 协程体
     |                                             |
     v                                             v
   connect(my_task, sync_wait_receiver)         co_await stdexec::just(42)
     |                                             |
     v                                             v
   task_op_state                              await_transform(just(42))
     |                                             |
     v                                             v
   start(op_state)                            sender_awaitable{just(42), coro_handle}
     |                                             |
     v                                             v
   coro_.resume()                             await_suspend(handle)
     |                                             |-- connect(sender, bridge_receiver)
     v                                             |-- start(op)
   [协程体执行]                                   |
     |                                             v
     v                                           [协程挂起, sender 执行]
   final_suspend                                   |
     |                                             v
     v                                           set_value(42) -> bridge_receiver
   set_value(external_receiver_, result)            |
                                                   v
                                                 coro_.resume()
                                                   |
                                                   v
                                                 await_resume() -> 42
   ```

8. **讨论 `completion_signatures` 的协程侧合同**：

   当 `my_task<T>` 声明 `completion_signatures = completion_signatures<set_value_t(T), set_error_t(exception_ptr), set_stopped_t()>` 时：

   - 它在告诉 sender 框架："我承诺产出 T 类型的值，或一个 exception_ptr，或 stopped。"
   - `then` 组合子可以根据这个签名推导出 `lambda(T) -> U` 的返回类型。
   - `when_all` 组合子可以根据这个签名推导出结果 tuple 中对应位置的类型。
   - `sync_wait` 可以根据这个签名限制它期待的返回类型。
   - 如果这个签名写错了（例如声明 `set_value_t(int)` 但实际 `co_return std::string{}`），编译器可能在很后面才报错——错误信息难以阅读。

### 进阶任务

- 支持 `my_task<void>`：`set_value_t()` 的 completion_signature。在 `final_suspend` 和 `return_void` 中做相应处理。
- 让 `my_task` 支持 `co_await` 产生多值的 sender——`await_transform` 的 `sender_awaitable` 使用 `std::tuple` 作为值类型。
- 让 `my_task` 的 `get_env()`（通过 `tag_invoke(get_env_t, const my_task&)`）返回包含 scheduler 和 stop_token 的 environment。验证 `then`、`when_all` 等组合子能正确查询这些属性。
- 实现 `my_task` 的 stop_token 传播：在 `final_suspend` 或 `await_transform` 中检查 stop_token，如果已停止则转发 stopped completion。
- 对比你的实现与 stdexec 中的 `task.hpp`——记录差异并分析原因。

### 验收点

- `stdexec::sync_wait(my_task<int>(...))` 能编译通过并返回正确结果。
- 协程能 `co_await my_task<T>`（awaitable 协议正确）。
- `my_task<T>` 协程体能 `co_await stdexec::just(...)`（sender-to-awaitable 桥接正确）。
- `my_task` 能参与 `then`、`when_all` 等 sender 组合链。
- 你能说明 `external_receiver_` 和 `continuation` 分别在什么场景下使用，以及为什么需要两者。
- 你能画出双向桥接的完整对象关系图。

### 观察点

- task 的双重身份是 C++26 异步模型的核心设计决策：同一个类型既是 sender（可以参与组合链）又是 awaitable（可以用协程语法消费）。这单一类型消除了"需要两种不同写法"的认知负担。
- `final_suspend` 同时处理 `external_receiver_`（sender 侧）和 `continuation`（协程侧）——这是 task 实现中最微妙的点。如果顺序写错（先处理 external_receiver 再处理 continuation  vs. 反过来），可能导致竞态或丢失 completion。
- `completion_signatures` 在协程侧不是可选的——它是 task 作为 sender 参与生态的类型级护照。没有它，任何标准组合子都无法验证 task 的安全性。
- 双向桥接的实施要点不是"两边都要写"，而是"同一个类型如何满足两套协议的语义契约"——很多契约在 sender 侧和协程侧的含义不同，需要精准翻译。

### 常见坑

- `final_suspend` 中先调用了 `set_value(external_receiver_, ...)`，然后才检查 `continuation`——但如果 continuation 是 external_receiver_ 关联的协程，这个顺序可能导致 double completion。
- `task_op_state::start()` 中直接 resume 了协程，但协程在 `final_suspend` 完成时 external_receiver 已经析构——start 的调用者没有等待 completion。
- `my_task` 的移动构造导致 `h_` 为空——后续 sender 或 awaitable 协议访问了空 handle。
- 同时实现了成员 `connect` 和 `friend tag_invoke(connect_t, ...)`——重载决议选择了错误的版本。
- `completion_signatures` 声明与实际的 completion 行为不一致——sender 框架在编译期检测到不匹配时给出难以阅读的错误信息。

### 提示

- 先实现 sender 协议（`connect` + `completion_signatures`），再实现 awaitable 协议，最后加 `await_transform`。分三步走，每步验证。
- `external_receiver_` 用类型擦除存储（`stdexec::any_receiver_ref<completion_signatures>`）可以避免复杂的模板实例化。
- 如果 `completion_signatures` 推导或 `connect` 报错，回到 Execution_Study 模块 D 确认最小 sender 骨架。
- 这道题的代码量在 150-250 行左右——如果超过 300 行，检查是否在基础设施上过度设计了。
- 画出"一个 task 被 sync_wait 消费时，哪些对象在何时创建、何时通信、何时销毁"的完整时间线。

### 复盘问题

- 为什么 `my_task<T>` 的 `final_suspend` 需要同时处理 `external_receiver_` 和 `continuation`？什么情况下两者同时非空？
- 如果 task 在 `final_suspend` 中先通过 symmetric transfer 恢复了 continuation（协程侧），external_receiver_（sender 侧）会怎样？它会丢失 completion 吗？
- `completion_signatures` 的协程侧合同与 sender 侧合同在语义上是否完全一致？如果协程可以用 `co_yield` 产生中间值，签名能否表达？
- 你认为 task 的双重身份（既是 sender 又是 awaitable）是一种优雅的设计，还是概念的过度重叠？

### 对应官方参考

- P2300R10：`std::execution` 中的 sender 协议和 `completion_signatures` 规范
- P3175R0：`as_awaitable` 和 task-sender 桥接
- P3552R3：`std::execution::task<T>` 的完整规范
- `stdexec/exec/task.hpp`：实际实现参考
- `stdexec/__connect_awaitable.hpp`：sender-to-awaitable 桥接实现
- Execution_Study 模块 H-3 的协程桥接练习（本练习的 sender 侧对等物）

---

## 做完模块 H 之后，你现在应该能说清楚什么

至少把下面几句话说顺：

- `as_awaitable(sender)` 通过 bridge receiver 将 sender 的三条 completion channel 映射为协程的 resume：`set_value` → 存储值 + resume → `await_resume` 返回值；`set_error` → 存储异常指针 + resume → `await_resume` rethrow；`set_stopped` → 标记 + resume → `await_resume` 特殊处理。`await_transform` 是这一切的入口——它把 `co_await sender` 翻译为 connect + start + 挂起 + resume 的完整流程。
- `std::execution::task<T>`（P3552R3）的核心增量是 scheduler affinity 和 environment propagation：promise 的 `get_env()` 让 sender 感知协程的运行上下文，`co_await schedule(sch)` 让协程主动切换运行资源。这些机制使 task 能参与 `when_all`、`let_value` 等组合子的类型安全并行组合。
- `my_task<T>` 的双重身份（同时是 sender 和 awaitable）是 C++26 异步模型统一的基石：作为 sender 时，它通过 `connect` + `completion_signatures` 参与组合链；作为 awaitable 时，它通过 `await_ready`/`await_suspend`/`await_resume` 被协程消费；作为协程时，它通过 `await_transform(sender)` 消费任意 sender。`final_suspend` 同时处理 `external_receiver_` 和 `continuation` 两套完成通知——这是双向桥接中最微妙的设计点。
- 至此，你已经亲手搭建了协程与 sender-receiver 之间的全部三座桥梁：`as_awaitable`（sender 到 awaitable）、`connect`（协程 task 到 sender）、`await_transform`（sender 到协程内 awaitable）。这三座桥构成了 C++26 异步基础设施的交通枢纽。