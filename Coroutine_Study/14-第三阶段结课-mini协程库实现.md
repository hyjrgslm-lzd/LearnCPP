# 14 第三阶段结课：mini 协程库实现

## 这份文件怎么用

模块 D 到 G 让你手写了 promise_type、awaitable 三层、协程帧和 symmetric transfer。模块 J 让你认识了协程的八大陷阱。这份文件是对上述所有知识的综合考试：从零实现一个 mini 协程库。核心库禁第三方依赖，只用标准库；stdexec 只用于可选桥接验证。

本文件包含 1 个结课项目：

1. `mini 协程库实现`（仓库稳定 ID：`Capstone5_mini_corolib`；数字 5 是结课项目 ID，不随章节重排改变）

完成后，你将拥有一套可以从第一性原理出发理解的协程基础设施。这不仅是练习，也是你后续阅读 cppcoro / folly / stdexec 协程源码时最可靠的"对照坐标"。

仓库内有两条路径：

- `Coroutine_Study/exercises/Capstone5_mini_corolib/include` 与 `src` 是学生 starter，核心路径禁第三方依赖，必须能在 `student`/`verify-core` preset 下编译。
- `Coroutine_Study/exercises/Capstone5_mini_corolib/reference` 是可运行 reference，包含 `task`、`generator`、`shared_task`、`when_all`、`when_any`、`sync_wait`、`run_loop`、`task_scope`、`stop_token` 的核心断言测试；`as_awaitable` 是 stdexec 可用时才构建的可选桥接测试。

Reference 验收命令：

```powershell
cmake -S Coroutine_Study/exercises --preset verify-core
cmake --build Coroutine_Study/exercises/build/verify-core --config Release --target mini_reference_task mini_reference_generator mini_reference_sync_wait mini_reference_when_all mini_reference_when_any mini_reference_run_loop mini_reference_task_scope mini_reference_stop mini_reference_shared_task
ctest --test-dir Coroutine_Study/exercises/build/verify-core -C Release -R "mini_reference_"
```

注意：`as_awaitable` 的 stdexec 适配属于可选桥接层，只在 `stdexec::stdexec` target 存在时构建；mini 库核心 9 个 reference 测试不应因为缺少 stdexec 而配置失败。

Reference 教学契约：

- `task` 由单一 owner 驱动；`start()` / `co_await` 后不得二次启动，完成后的重复 `co_await` / `await_resume` 也应拒绝。
- `when_all` 先启动全部分支，等全部分支完结后返回 tuple；若有异常，传播首个异常。
- `when_any` 首个成功 value 获胜并请求 stop，但仍等待全部分支收束后返回；若没有成功 value，传播首个异常。
- 不同线程完成由每个 operation 自己的 `mutex` 或等价同步保护；runner 在 `final_suspend` 路径计入完成。
- `sync_wait` 只 start root task 一次，完成通知来自 runner 的 `final_suspend` 后续路径，而不是 blind-resume 循环。

---

## 结课项目 5：mini 协程库实现

### 项目目标

从零实现一个最小但功能完整的协程库。核心组件禁第三方依赖（只允许 C++23 标准库）；`as_awaitable(stdexec_sender)` 属于可选桥接层，只有在 pinned stdexec 可用时构建验证。这不是"照着 cppcoro 抄一遍"——你要自己决定每一层的设计选择，然后验证它们能否组合在一起。

### 必须包含的组件

你的 mini 协程库至少要包含以下 10 个组件：

1. `mini::task<T>` -- lazy 启动的异步协程任务，支持 co_await 和 co_return
2. `mini::generator<T>` -- 同步惰性序列生成器
3. `mini::shared_task<T>` -- 支持多个 awaiter 同时等待的协程任务
4. `mini::when_all` -- 并发等待多个 task，在最后一个完成时恢复
5. `mini::when_any` -- 竞速等待，第一个完成的 task 触发恢复
6. `mini::sync_wait` -- 同步阻塞等待 task 完成并提取结果
7. `mini::async_scope` -- 结构化并发作用域，管理 in-flight 协程
8. `mini::stop_token` -- 协作式取消（最小版，可基于 `std::stop_token` 或自写）
9. `mini::single_thread_executor` -- 单线程调度器，驱动协程执行
10. `mini::as_awaitable(stdexec_sender)` -- 将 stdexec sender 桥接为 awaitable

### 设计约束

- **operation_state 等价物 non-movable**：coroutine state / frame 一旦创建，其地址不会因为外层 `task` wrapper 的 move 而改变；move 移动的是 owning handle。真正的风险是：`start()` 之后 operation_state、receiver、等待者链和唯一所有权已经建立，再移动 wrapper 会让所有权与生命周期难以证明。你的设计必须保证已启动对象不被移动（最简单的方法：禁 copy，允许未启动 move；或 `start()` 后标记并禁止 move）。
- **completion_signatures 编译期可查询**：每个 sender-like 组件（task / generator / when_all 等）应该能通过元编程在编译期回答"我会产生什么类型的结果"。
- **HALO 作为实现观察项**：用 Clang `-Rpass=coroutine-elide` 观察简单路径是否出现 coroutine frame elision。HALO 是优化结果，不是标准保证；如果未触发，记录原因，而不是让功能验收失败。

### 必做任务

按从底层到上层的顺序实现。这个顺序是你自己决定的分层——每个选择都有理由。

#### 第一层：CPO 与 concept 基础设施（约 60-90 行）

1. 定义基础 CPO（Customization Point Object）：
   - `mini::connect_t` -- 将 sender 与 receiver 连接
   - `mini::start_t` -- 启动 operation_state
   - `mini::set_value_t` / `mini::set_error_t` / `mini::set_stopped_t` -- 三条完成通道
   - `mini::get_env_t` -- 从 receiver 获取 environment

2. 定义最小 concept：
   - `mini::sender<S>` -- 可被 connect
   - `mini::receiver<R>` -- 有 get_env 且能接收三条 completion
   - `mini::operation_state<O>` -- 可被 start

3. 用 `static_assert` 验证：`mini::sender<decltype(mini::just(42))>` 通过。

#### 第二层：just 与基础 sender（约 50-80 行）

4. 实现 `mini::just(values...)`：
   - 返回一个 sender，内部存储值
   - `completion_signatures` 声明为 `set_value_t(Ts...)` 和 `set_error_t(std::exception_ptr)`
   - `connect(receiver)` 返回 operation_state，`start()` 同步调用 `set_value(receiver, values...)`

5. 实现 `mini::just_error(ep)` 和 `mini::just_stopped()`。

6. 验证：`mini::sync_wait(mini::just(42))` 返回 42。

#### 第三层：promise_type 与 task<T>（约 120-180 行）

7. 实现 `mini::task<T>` 的 `promise_type`：
   - 8 个 hook 全部实现：`get_return_object`、`initial_suspend`（返回 `suspend_always`，lazy）、`final_suspend`（返回自定义 awaitable 支持 symmetric transfer）、`return_value`、`return_void`、`unhandled_exception`、`yield_value`（断言不调用，task 不用 co_yield）、`await_transform`
   - 在 promise 中存储结果：`std::variant<std::monostate, T, std::exception_ptr>`
   - `final_suspend` 的 awaitable 检查是否有等待者（continuation），有则对称转移

8. 实现 `mini::task<T>` 的 awaiter 接口：
   - 让 task 能被 `co_await`：实现 task 级别的 `await_ready` / `await_suspend` / `await_resume`
   - `await_suspend` 保存调用者的 coroutine_handle 到 promise 的 continuation_ 字段

9. 实现 `mini::task<T>` 的 sender 接口（可选但推荐）：
   - 定义 `completion_signatures`
   - 实现 `connect(receiver)`，返回 operation_state
   - operation_state 的 `start()` 调用 `coroutine_handle::resume()`

10. 验证：
    ```cpp
    mini::task<int> compute() {
        co_return 42;
    }
    auto result = mini::sync_wait(compute());
    // result == 42
    ```

#### 第四层：generator<T>（约 60-100 行）

11. 实现 `mini::generator<T>`：
    - `promise_type` 支持 `yield_value(T)` 存入当前值
    - `initial_suspend` 返回 `suspend_always`（惰性）
    - `final_suspend` 返回 `suspend_always`（保留 frame 供调试）
    - 提供 iterator 接口：`begin()` / `end()`，每次 `operator++` 调用 `resume()`

12. 验证：
    ```cpp
    mini::generator<int> seq() {
        for (int i = 0; i < 5; ++i) co_yield i;
    }
    int sum = 0;
    for (int x : seq()) sum += x;
    // sum == 10
    ```

#### 第五层：when_all 与 when_any（约 100-150 行）

13. 实现二元 `mini::when_all(task_a, task_b)`：
    - 接受两个 `task`，返回一个新的 task/awaitable
    - 每个 operation 内部用 `std::mutex` 或等价同步保护 remaining、error 和结果槽
    - 结果存储在 `std::tuple` 中：`std::tuple<ResultA, ResultB>`
    - 如果一个子 task 出错，记录首个异常；整体仍等所有分支完结后再传播
    - `completion_signatures` 在编译期推导 tuple 类型

14. 实现二元 `mini::when_any(task_a, task_b)`：
    - 首个成功 value 获胜并请求 stop；整体仍等所有分支收束后返回
    - 如果没有成功 value，传播首个异常
    - 每个 operation 内部用 `std::mutex` 或等价同步保护 winner/error 状态

15. 验证：
    ```cpp
    auto opt = mini_ref::sync_wait(mini_ref::when_all(task_a(), task_b()));
    // opt 的类型是 std::optional<std::tuple<std::tuple<A, B>>>
    auto [a, b] = std::get<0>(*opt);
    ```

#### 第六层：sync_wait（约 50-80 行）

16. 实现 `mini::sync_wait(sender)`：
    - 固定返回类型：`std::optional<std::tuple<Ts...>>`，与 P2300 `stdexec::sync_wait` 一致；
      如果 sender 走 set_stopped，返回空 optional；如果 sender 走 set_error，rethrow 异常。
      在验证代码中需要写 `auto opt = mini::sync_wait(...); auto [v] = *opt;`，
      而非直接 `assert(result == 42)`。
    - 内部用一个 `sync_wait_receiver`，在 `set_value` 时存储结果并通知 condvar
    - 用 `std::condition_variable` + `std::mutex` 实现等待
    - 包含错误处理：如果 sender 走 error completion，sync_wait 抛出异常

17. **HALO 观察**：
    - 用 Clang `-Rpass=coroutine-elide` 编译一个简单路径：`mini::sync_wait(mini::just(42))`
    - 如果编译器报告"coroutine frame elided"，记录触发条件
    - 如果 HALO 未触发，分析原因（是否 sync_wait 在 `connect` 之后、`start` 之前有分支？是否 frame 逃逸到了堆？）

#### 第七层：single_thread_executor（约 50-80 行）

18. 实现 `mini::single_thread_executor`：
    - 内部维护一个 `std::queue<std::function<void()>>` 或轻量 intrusive queue
    - `schedule()` 返回一个 sender，connect 后 register receiver 到队列
    - `run()` 循环从队列取出并执行
    - `stop()` 设置停止标志

19. 验证：
    ```cpp
    mini::single_thread_executor ex;
    auto work = mini::then(mini::schedule(ex), [] { return 100; });
    // 在另一个线程或之后调用 ex.run()
    ```

#### 第八层：async_scope 与 stop_token（约 80-120 行）

20. 实现 `mini::async_scope`：
    - `spawn(sender)`：在 scope 中启动一个任务，记录 operation_state
    - `on_empty()`：返回一个 sender，在 scope 变为空时完成
    - 析构函数：等待所有 in-flight 任务完成（阻塞）
    - 所有 operation_state 以 owning 方式管理（通过类型擦除或 shared_ptr）

21. 实现 `mini::stop_token` / `mini::in_place_stop_source`：
    - 最小版：基于 `std::stop_token` / `std::stop_source` 做包装
    - 进阶版：自写 `mini::in_place_stop_source`，对照 stdexec 的 inplace stop 设计
    - 或自写：一个 `std::atomic<bool>` + `std::stop_callback` 注册列表

22. 验证：
    ```cpp
    mini::async_scope scope;
    for (int i = 0; i < 5; ++i) {
        scope.spawn(some_async_work(i));
    }
    // scope 析构前确保所有工作完成
    ```

#### 第九层：as_awaitable 桥接（约 60-100 行）

23. 实现 `mini::as_awaitable(sender)`：
    - 接受一个 stdexec sender（或任何满足 sender concept 的类型）
    - 返回一个 awaitable 对象
    - `await_ready()`：返回 false（总是挂起）
    - `await_suspend(handle)`：
      - 构造 bridge receiver，持有 handle 和结果存储
      - `connect(sender, bridge_receiver)`，得到 operation_state
      - `start(operation_state)`
      - operation_state 必须存活到 sender 完成
    - `await_resume()`：从结果存储中取出值或 rethrow 异常
    - bridge receiver 的 `set_value` 存储结果并 resume handle
    - bridge receiver 的 `set_error` 存储异常并 resume handle
    - bridge receiver 的 `set_stopped` 存储特殊标记

24. 验证：
    ```cpp
    mini::task<int> bridge_test() {
        int x = co_await mini::as_awaitable(stdexec::just(42));
        int y = co_await mini::as_awaitable(stdexec::just(10));
        co_return x + y;  // 52
    }
    ```

### 关键设计决策参考

以下是在实现过程中你会遇到的几个关键设计分支。这里不给你"正确答案"，而是给出每种选择的利弊——你必须自己做决定并承担后果。

**决策 1：task 的移动语义**

- 方案 A：task 允许 move，但 `start()` 后标记为"已启动"，移动构造/赋值时的运行时检查抛异常。
  - 优点：符合 C++ 值语义习惯，可以在容器中存储 task。
    - 缺点：运行时检查开销，错误暴露较晚。
- 方案 B：task 禁 copy，允许未启动 move；一旦 start/co_await/connect 后禁止再移动。
  - 优点：保留返回值移动和容器构造的实用性，同时避免已启动对象的所有权漂移。
  - 缺点：需要清楚地区分"未启动 wrapper 可移动"与"已连接/已启动 operation_state 不可移动"。

**决策 2：final_suspend 的 symmetric transfer 方向**

- 方案 A：`final_suspend` 返回 `std::suspend_always`，在 resume 时由外部手动控制。
  - 优点：简单，容易理解。
  - 缺点：没有 symmetric transfer，深层嵌套时可能栈溢出。
- 方案 B：`final_suspend` 的 `await_suspend` 返回 continuation 的 handle（对称转移）。
  - 优点：把下一个恢复目标交给 `co_await` 变换，避免库代码直接递归 `.resume()`。
  - 注意：机器级 tail call 或恒定机器栈不是标准保证，需按目标编译器实测。
  - 缺点：实现复杂，需要正确处理"没有 continuation"的情况。

**决策 3：when_all 的错误策略**

- 方案 A：等待所有子 task 完成，整体返回成功 tuple 或首个异常。
  - 优点：结果/错误生命周期清楚，和 reference 一致。
  - 缺点：不能提前释放 loser 资源。
- 方案 B：等待所有子 task 完成，收集所有错误后用 `std::vector<std::exception_ptr>` 汇总。
  - 优点：信息完整，可以区分"哪些失败哪些成功"。
  - 缺点：需要错误汇总类型，取消传播更复杂。

**决策 4：operation_state 存储在何处**

- 方案 A：`sender_awaitable` 内部用对齐存储 + placement new 存储 operation_state。
  - 优点：无动态分配，最快的路径。
  - 缺点：`sender_awaitable` 也被锁定为 non-movable。
- 方案 B：`sender_awaitable` 用 `std::unique_ptr<op_state>` 堆分配。
  - 优点：move 友好，实现简单。
  - 缺点：额外的堆分配，抵消 HALO 的收益。

### 实现过程中的关键对象关系

在开始编码前，务必先在纸上画出以下关系。这些是项目中最核心的结构依赖：

1. **task 的创建到销毁链路**：
   ```
   调用者创建 task 协程
   -> 编译器分配 frame (可能 HALO)
   -> 构造 promise
   -> initial_suspend (suspend_always) // 挂起，返回 task 给调用者
   -> 调用者 co_await task 或 sync_wait(task)
   -> task.await_suspend 记录 continuation
   -> coroutine_handle::resume()
   -> 协程体执行，遇到 co_await / co_return
   -> final_suspend 检查 continuation，对称转移 resume 它
   -> frame 销毁 (coroutine_handle::destroy())
   ```

2. **when_all 的并发协调**：
   ```
   when_all_sender::connect(receiver)
   -> 为每个子 sender 分别 connect 一个 when_all_receiver
   -> 每个子 receiver 的 set_value: 在 operation mutex 保护下递减 remaining，存入 tuple slot
   -> 最后一个子 receiver 完成: resume 等待者
   -> 等待者的 await_resume 从 tuple 中取出所有结果
   ```

3. **as_awaitable 的桥接通道**：
   ```
   co_await as_awaitable(sender)
   -> await_transform 调用 as_awaitable
   -> await_ready -> false (挂起)
   -> await_suspend: connect(sender, bridge_receiver) -> start(op)
   -> sender 完成 -> bridge_receiver::set_value -> 存结果 -> resume 协程
   -> await_resume: 取出结果，或 rethrow 异常
   ```

### 观察点

- 分层实现顺序不是任意的。如果你先写 sync_wait 再写 task，sync_wait 没法测试。如果你先写 when_all 再写 as_awaitable，when_all 内部的 bridge 逻辑缺少基础。
- 每一层验证一次，不是"全写完再一起验证"。协程帧的 bug 往往不在你写的代码行，而在你"以为编译器会自动处理"的地方。
- `completion_signatures` 的编译期推导（尤其是 `when_all` 的 tuple 类型）是整个项目中最吃模板元编程能力的部分。先保证二元 task 版清楚可测，再把三元/variadic 留到进阶。
- HALO 的可观察结果由编译器、优化级别、代码形状和是否能证明生命周期严格嵌套共同决定。在 `sync_wait(just(42))` 这类同步完成路径上有机会触发，但不能作为可移植语义假设。
- 禁第三方依赖意味着你不能用 `nlohmann/json` 做序列化、不能用 `absl::Mutex` 替代 `std::mutex`、不能用 `folly::Future` 做参考。这迫使你把每一块都搞清楚。

### 常见坑

- `final_suspend` 返回 `suspend_never` 导致协程 frame 在最后的 co_await 点自动销毁，而 continuation 还在等待——经典的 use-after-free。
- `promise_type::unhandled_exception` 中重新抛出异常（`throw;`）——标准允许异常传播给 caller/resumer，但这不等同于教学 task 常用的 `exception_ptr` 存储路径。你必须明确消费端如何观察这个异常。
- `when_all` 的 remaining、winner/error 和结果槽必须由每个 operation 自己的 `mutex` 或等价同步保护；不要在未同步路径上读写它们。
- `sync_wait` 内部 receiver 的 `set_value` 在 condvar 通知之前存储结果。如果顺序反了（先 notify 再存），等待线程可能拿到空结果。
- `async_scope` 的 `spawn` 后，如果 spawn 的对象是临时 sender，op_state 在 start 后立即析构——这是 operation_state 生命周期管理的经典陷阱。
- 在 `as_awaitable` 的 `await_suspend` 中，如果 sender 同步完成（start 内部直接调用了 set_value），必须处理"完成发生在 await_suspend 返回前"的路径。可用 atomic 阶段握手并返回 `false`，也可用其他设计延迟 resume；不要无条件保存 handle 后假设完成一定异步到来。
- `generator<T>` 的 `yield_value` 接受了引用参数，但没有存副本——generator 消费者拿到的是快要被下一次 `resume()` 覆盖的地址。

### 提示

- 先写死 T = int 的版本跑通所有组件，再用模板泛化。协程帧的模板展开会导致编译时间爆炸，在 Debug 阶段没必要承受。
- 给每个组件写一个独立的 `_test.cpp` 文件，不要所有验证塞在一个 main 里。这样改一个组件只需重编译一个测试文件。
- `completion_signatures` 的编译期推导可以先用 `static_assert` + 手写期望类型验证，不用上来就追求完美的 `make_completion_signatures` 元函数。
- 如果你的 `when_all` 实现中，取消传播过于复杂，先实现 reference 的"全分支收束后返回结果/首异常"契约。
- HALO 验证时注意：`-Rpass=coroutine-elide` 只在 Clang 17+ 有效。GCC 用 `-fdump-ipa-coro`，MSVC 无等价的公开诊断选项。如果只有 MSVC 环境，通过分析汇编（查找 `operator new` 调用）间接验证。
- `shared_task` 是最晚实现的组件——它的引用计数 + 多等待者链表 + 析构竞态是协程库中最复杂的对象生命周期题。除非你特别想挑战，建议把 shared_task 留到进阶任务。

### 进阶任务

- 实现 `mini::on(scheduler, sender)`：将 sender 切换到指定调度器上执行。
- 实现 `mini::let_value(sender, factory)`：在当前 sender 的 value completion 上继续一个新的 sender 图。
- 把二元 `when_all` / `when_any` 泛化为三元或 variadic 版本。
- 为 `mini::task<T>` 添加 allocator 感知：promise 的 `operator new` 接收自定义 allocator（P0912 风格）。
- 实现 `mini::generator<T>` 的 symmetric transfer 优化：co_yield + 递归生成时利用 symmetric transfer 避免栈溢出。
- 做一次完整的性能对比：用 `mini::task` vs cppcoro `task` 跑 `when_all` 1000 次，记录帧分配次数和运行时间。
- 实现 `mini::split(sender)`：让一个 sender 的结果可以被多个下游 consumer 共享。

### 最终验证

完成实现后，用下面的片段或仓库 reference tests 验证。涉及 stdexec 的片段只在 pinned stdexec 可用时适用。

**验证 1：task + sync_wait**
```cpp
mini::task<int> compute() {
    int x = co_await mini::as_awaitable(stdexec::just(21));
    co_return x * 2;
}
auto opt = mini::sync_wait(compute());
auto [v] = *opt;

// assert(v == 42)
```

**验证 2：generator + ranges**
```cpp
mini::generator<int> fib() {
    int a = 0, b = 1;
    co_yield a;
    co_yield b;
    for (int i = 0; i < 8; ++i) {
        int c = a + b;
        co_yield c;
        a = b; b = c;
    }
}
std::vector<int> values;
for (int v : fib() | std::views::take(10)) values.push_back(v);
// values == {0,1,1,2,3,5,8,13,21,34}
```

**验证 3：when_all + 类型安全**
```cpp
auto opt = mini_ref::sync_wait(
    mini_ref::when_all(
        []() -> mini_ref::task<int> { co_return 2+3; }(),
        []() -> mini_ref::task<int> { co_return 2*3; }()
    )
);
auto [sum, prod] = std::get<0>(*opt);
// sum == 5, prod == 6
```

**验证 4：async_scope + stop_token**
```cpp
mini::single_thread_executor ex;
mini::async_scope scope;
mini::in_place_stop_source stop_src;

for (int i = 0; i < 10; ++i) {
    scope.spawn(mini::on(ex.get_scheduler(),
        []() -> mini::task<void> {
            // 模拟异步工作
            co_return;
        }()
    ));
}
// scope 析构时等待所有任务完成
```

**验证 5：HALO 观察**
使用 Clang 编译以下代码：
```cpp
auto result = mini::sync_wait(mini::just(42));
```
编译命令加上 `-Rpass=coroutine-elide`，记录是否出现 "coroutine frame elided"。如果没有出现，保留编译器版本、优化级别和代码形状作为观察记录。

**验证 6：valgrind / ASan 无 leak**
```bash
g++ -std=c++23 -O2 -fsanitize=address mini_task_test.cpp -o test
./test
# 通过时记录 ASan 0 errors；失败时先修生命周期问题
```

### 验收点

- 10 个组件全部实现，通过的验证代码片段不低于 4 个。
- operation_state 等价物 non-movable 的设计约束被满足。
- `completion_signatures` 在编译期可查询（用 static_assert 验证 at least task 和 when_all）。
- HALO 观察有记录：触发或未触发都要说明编译器、优化级别和原因分析。
- valgrind / ASan 验证有记录；如果本机没有这些工具，必须明确写下未验证原因。
- 你能画出 task / when_all / sync_wait / as_awaitable 的完整对象关系图。
- 你能解释每一层的设计选择（为什么 lazy 启动、为什么 final_suspend 用 symmetric transfer、为什么 operation_state 必须 non-movable）。

### 项目复盘问题

- 实现过程中，哪一层最出乎你意料地复杂？为什么？
- `when_all` 的 `std::tuple` 结果推导在实现中造成了什么困难？
- operation_state non-movable 约束对 `async_scope` 的实现方式有什么影响？
- 你的 `final_suspend` 中 symmetric transfer 在目标编译器上呈现怎样的机器栈表现？标准只保证了哪些控制转交语义？
- 如果要把 `mini::task<T>` 扩展为 `mini::eager_task<T>`（initial_suspend 返回 suspend_never），需要改哪些地方？
- 完成这个项目后，你对 P3552（std::execution::task）的设计选择有了什么新的理解？
- 你的 mini 库和 cppcoro 相比，最大的简化在哪里？这些简化会在什么场景下出问题？
- 如果以后要给这个库加 "coroutine frame pool allocator"，最自然的插入点在哪里？

### 完成后你应该能说清楚什么

至少把下面几句话说顺：

- 这 10 个组件分别解决什么问题：task、generator、shared_task、when_all、when_any、sync_wait、async_scope、stop_token、single_thread_executor、as_awaitable 桥接。
- 为什么 coroutine frame 不会因为外层 task wrapper move 而移动，但已启动 task / operation_state 仍然需要严格的 ownership 和 non-movable 约束。
- 为什么 `completion_signatures` 要能在编译期查询，以及它如何影响 `when_all` / `sync_wait` 的返回类型。
- HALO 是优化观察项，不是语义保证；你能用具体编译器输出说明它在你的实现上是否触发。
- ASan / valgrind 能证明哪些生命周期问题，哪些还需要代码审查、TSan 或结构化所有权设计。
- 这个项目如何帮助你阅读 cppcoro、folly coro、stdexec task：哪些是协程基础设施必须做的事，哪些是生产库补出来的工程边界。
