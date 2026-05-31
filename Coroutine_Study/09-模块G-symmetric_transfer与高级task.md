# 09 模块 G：symmetric_transfer 与高级 task

## 模块目标

前面模块 D 和 E 让你写出了一个最小 `lazy_task<T>`、理解了 promise_type 全貌和 co_await 的三步变换。这个模块要把 task 从"只能一个等一个"推进到"真正的高级异步原语"：

- 实现 `shared_task<T>`：允许多个协程同时 `co_await` 同一个 task，理解引用计数、多 awaiter 链表、以及 frame 销毁与计数之间的竞态窗口。
- 实现 `when_all<T...>`：N 个子 task 并行执行，最后一个完成时自动唤醒等待者，用 tuple 汇合结果。理解原子计数与错误合并策略。
- 实现 `sync_wait`：用 condition_variable + 内置 receiver 风格，在非协程上下文中驱动 task 到完成。理解为什么 `main` 不能是协程。

如果你跳过这一层，你就只能用单协程串行——无法实现真正的并行组合和跨协程/线程同步。

## 模块完成标准

做完本模块，你至少要能稳定说清楚：

- shared_task 的引用计数为什么必须在对 frame 有任何操作之前完成增减，以及 final_suspend 中多 awaiter 的链表唤醒如何避免竞态。
- when_all 的原子计数如何保证"最后一个完成的子 task 唤醒等待者"——这本质上是一个 barrier 协议。
- when_all 中错误处理策略（谁的异常胜出、如何传播、如何不丢异常）的设计权衡。
- sync_wait 如何通过 condvar 将协程挂起/恢复转化为阻塞-唤醒——这是"把协程世界桥接回同步世界"的标准模式。
- 为什么 `main` 不能是协程——因为标准未规定谁负责驱动 main 协程的启动和最终销毁。

## 类型骨架桥接段

在开始实现之前，回顾一下 `lazy_task<T>` 的关键结构（基于模块 D 的骨架）：

```cpp
template<typename T>
struct lazy_task {
    struct promise_type {
        T result_value;
        std::exception_ptr result_exception;
        std::coroutine_handle<> continuation;  // 正在等待本 task 的协程

        lazy_task get_return_object() {
            return lazy_task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        std::suspend_always initial_suspend() noexcept { return {}; }
        auto final_suspend() noexcept {
            struct final_awaiter {
                bool await_ready() noexcept { return false; }
                auto await_suspend(std::coroutine_handle<promise_type> h) noexcept {
                    if (h.promise().continuation)
                        return h.promise().continuation;  // symmetric transfer
                    return std::noop_coroutine();
                }
                void await_resume() noexcept {}
            };
            return final_awaiter{};
        }
        void return_value(T value)      { result_value = std::move(value); }
        void unhandled_exception() noexcept { result_exception = std::current_exception(); }
    };

    std::coroutine_handle<promise_type> h_;

    // 移动语义：独占所有权
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
    ~lazy_task() { if (h_) h_.destroy(); }

    // awaitable 接口：让 task 可被 co_await
    bool await_ready() noexcept { return false; }
    auto await_suspend(std::coroutine_handle<> caller) noexcept {
        h_.promise().continuation = caller;
        return h_;  // symmetric transfer
    }
    T await_resume() {
        auto& p = h_.promise();
        if (p.result_exception) std::rethrow_exception(p.result_exception);
        return std::move(p.result_value);
    }
};
```

本模块所有实现都以这个骨架为基础，逐步改和扩展。

---

## 练习 G-1：从 task 到 shared_task

### 目标

把 `lazy_task<T>` 升级为 `shared_task<T>`：允许多个协程同时 `co_await` 同一个 shared_task，每个等待者都能拿到结果（值语义）或异常。实现引用计数管理、final_suspend 的多 awaiter 链表唤醒、并妥善处理 frame 销毁与计数之间的竞态窗口。

### 前置理解

- `lazy_task<T>` 是 move-only 的——它独占协程帧的所有权。只有一个协程可以 `co_await` 它（然后在 `final_suspend` 时被对称传回控制权）。
- `shared_task<T>` 是 copyable 的——每次拷贝增加引用计数。多个协程可以同时（或先后）`co_await` 同一个 shared_task。当最后一个持有者释放引用时，协程帧被销毁。
- 关键语义差异：
  - **lazy_task**：move 后源变空。`co_await` 后 task 被"消耗"（协程帧最终被 `await_suspend` 交出去）。
  - **shared_task**：拷贝增加计数。`co_await` 不消耗 task——多个协程各自拿到一份结果的拷贝。
- 生命周期挑战：`co_await shared_task` 时，如果 task 尚未完成，当前协程需要把自己注册到 shared_task 的等待者链表中。当 task 完成（final_suspend）时，需要遍历链表唤醒所有等待者。同时，等待者链表的节点可能在 task 完成前就被销毁（如果等待者协程被外部 destroy），需要安全处理。
- 竞态窗口：shared_task 的引用计数和协程帧的销毁之间存在竞态——最后一个外部引用释放时，协程帧必须还在；但协程帧的释放又依赖于等待者链表中没有遗留的悬空引用。

### 必做任务

1. **实现 `shared_task<T>` 的引用计数架构**：

   ```cpp
   template<typename T>
   struct shared_task {
       struct promise_type {
           T result_value;
           std::exception_ptr result_exception;
           bool done_ = false;

           // 等待者链表——允许多个协程同时等待
           struct awaiter_node {
               std::coroutine_handle<> coro;
               awaiter_node* next = nullptr;
           };
           awaiter_node* waiters_head_ = nullptr;

           shared_task get_return_object() {
               return shared_task{
                   std::coroutine_handle<promise_type>::from_promise(*this)
               };
           }
           std::suspend_always initial_suspend() noexcept { return {}; }
           void return_value(T value) {
               result_value = std::move(value);
               done_ = true;
           }
           void unhandled_exception() noexcept {
               result_exception = std::current_exception();
               done_ = true;
           }

           // final_suspend：唤醒所有等待者
           auto final_suspend() noexcept {
               struct final_awaiter {
                   bool await_ready() noexcept { return false; }
                   std::coroutine_handle<> await_suspend(
                       std::coroutine_handle<promise_type> h) noexcept {
                       auto& p = h.promise();
                       // 遍历等待者链表，前 N-1 个用 .resume()，最后一个用 symmetric transfer
                       auto* node = p.waiters_head_;
                       std::coroutine_handle<> last{};
                       while (node) {
                           auto next = node->next;
                           if (!next) { last = node->coro; break; }
                           node->coro.resume();
                           node = next;
                       }
                       // 最后一个等待者（或无等待者）用 symmetric transfer；如果链表为空返回 noop
                       return last ? last : std::noop_coroutine();
                   }
                   void await_resume() noexcept {}
               };
               return final_awaiter{};
           }
       };

   private:
       struct control_block {
           std::coroutine_handle<promise_type> h_;
           std::atomic<int> ref_count_{1};

           explicit control_block(std::coroutine_handle<promise_type> h)
               : h_(h) {}

           void add_ref() { ref_count_.fetch_add(1, std::memory_order_relaxed); }
           void release_ref() {
               if (ref_count_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
                   h_.destroy();  // 最后一个引用释放时销毁协程帧
                   delete this;
               }
           }
       };

       control_block* cb_;

       explicit shared_task(control_block* cb) : cb_(cb) {}

   public:
       explicit shared_task(std::coroutine_handle<promise_type> h)
           : cb_(new control_block(h)) {}

       shared_task(const shared_task& other) : cb_(other.cb_) {
           cb_->add_ref();
       }
       shared_task& operator=(const shared_task& other) {
           if (this != &other) {
               if (cb_) cb_->release_ref();
               cb_ = other.cb_;
               cb_->add_ref();
           }
           return *this;
       }
       shared_task(shared_task&& other) noexcept : cb_(std::exchange(other.cb_, nullptr)) {}
       shared_task& operator=(shared_task&& other) noexcept {
           if (this != &other) {
               if (cb_) cb_->release_ref();
               cb_ = std::exchange(other.cb_, nullptr);
           }
           return *this;
       }
       ~shared_task() { if (cb_) cb_->release_ref(); }
   };
   ```

   关键设计决策：
   - 使用独立的 `control_block` 将引用计数与协程帧分离——这样即使协程帧被销毁，引用计数逻辑仍正常工作。
   - `final_suspend` 遍历等待者链表：前 N-1 个用 `.resume()` 唤醒，最后一个（或链表中唯一的）用 symmetric transfer 返回 handle。这避免了头节点被 resume 两次的 UB——循环只 resume 前 N-1 个，最后一个由框架通过 symmetric transfer 跳转。

2. **实现 shared_task 的 awaitable 接口**（允许多个协程 co_await 同一个 shared_task）：

   ```cpp
   // shared_task 的 co_await 支持
   // awaiter 自身就是 intrusive list node，await_suspend 中把 this 指针挂到链表头
   struct shared_task_awaiter {
       std::coroutine_handle<promise_type> h_;
       std::coroutine_handle<> caller_;
       shared_task_awaiter* next_ = nullptr;  // intrusive list node —— 自身就是节点

       bool await_ready() noexcept {
           // 如果 task 已经完成，不需要挂起
           return h_.promise().done_;
       }

       bool await_suspend(std::coroutine_handle<> caller) noexcept {
           caller_ = caller;
           auto& p = h_.promise();
           if (p.done_) {
               // 在 await_ready 和 await_suspend 之间 task 完成了
               // 返回 false：不挂起，立即继续
               return false;
           }
           // 把自己（this）以头插法挂入 promise 的等待者链表
           next_ = p.waiters_head_;
           p.waiters_head_ = this;
           return true;  // 挂起（同 void 语义）
       }

       T await_resume() {
           auto& p = h_.promise();
           if (p.result_exception)
               std::rethrow_exception(p.result_exception);
           return p.result_value;  // 返回拷贝（多等待者共享）
       }
   };
   ```

   关键设计：`shared_task_awaiter` 自身就是 intrusive list node——它的 `next_` 指针和 `caller_` 成员都内嵌在 awaiter 对象中。`awaiter` 分配在协程帧上（作为 `co_await expr` 的临时对象），因此 node 的生命周期与等待者协程帧一致——在 final_suspend 遍历链表时 node 一定还活着。注意：`await_resume` 返回值的**拷贝**而非移动——因为其他等待者也需要读取这个值，这是 shared_task 比 lazy_task 慢的核心原因之一。

3. **验证基本 shared 语义**：

   ```cpp
   shared_task<int> shared_compute() {
       co_return 42;
   }

   void test_shared() {
       auto st = shared_compute();
       st.resume();  // 启动

       auto st2 = st;  // 拷贝——引用计数 2
       auto st3 = st;  // 拷贝——引用计数 3

       // st、st2、st3 指向同一个协程帧
       // 验证方式：各自 co_await 取结果（不在此处直接访问 result_value）
       // shared_task 不暴露裸 result()——使用 co_await 获取值拷贝
   }
   ```

4. **验证多 awaiter 同时 co_await**：

   ```cpp
   shared_task<int> slow_compute() {
       co_await std::suspend_always{};  // 模拟异步操作
       co_return 100;
   }

   lazy_task<void> waiter_a(shared_task<int>& st) {
       int v = co_await st;  // 挂起等待
       std::println("waiter_a got: {}", v);
       co_return;
   }

   lazy_task<void> waiter_b(shared_task<int>& st) {
       int v = co_await st;  // 挂起等待（同一个 shared_task）
       std::println("waiter_b got: {}", v);
       co_return;
   }
   ```

5. **在笔记中标注竞态窗口**：

   - 竞态 1：两个线程同时拷贝 shared_task → `add_ref` 必须用 atomic（已满足）。
   - 竞态 2：一个线程在 `co_await` 后读取结果，另一个线程在 `final_suspend` 中销毁了帧 → 通过 `done_` 标志 + `await_ready` 检查协调。
   - 竞态 3：等待者链表的插入与遍历 → 需要保证 `final_suspend` 时链表已完全构建。由于 final_suspend 在协程体结束后才执行，而 `await_suspend` 在协程体结束前执行（因为 await_ready 检查 `done_`），在单线程模式下天然安全；多线程模式需要更复杂的同步。

### 进阶任务

- 实现线程安全的等待者链表：在 `await_suspend` 插入 node 时使用 `std::atomic` compare-exchange 操作。在 `final_suspend` 遍历时安全处理并发插入。
- 为 shared_task 实现 `result()` 阻塞接口（在 task 完成前自旋或 condvar 等待）。
- 处理等待者协程在 shared_task 完成前被 `destroy()` 的情况：需要在 awaiter_node 中存储反向指针（指向协程帧），并在协程帧销毁前从链表中移除 node。
- 对比 cppcoro 的 `shared_task` 实现与你的实现，记录差异（类型擦除、allocator 支持、异常安全性等方面）。

### 验收点

- 你的 shared_task 支持拷贝语义，且拷贝后引用计数正确增加。
- 多个协程能同时 `co_await` 同一个 shared_task，且每个都能正确拿到结果。
- 最后一个 shared_task 实例销毁时，协程帧被正确释放。
- `await_resume` 返回的是值的拷贝而非移动——确保多等待者正确性。
- 你能画出引用计数从 1 到 N 再到 0 的生命周期图，并标注每一步谁持有引用。

### 观察点

- shared_task 的"共享"是有代价的：结果必须拷贝而非移动，引用计数有原子开销，等待者链表有分配和遍历开销。
- control_block 的分离设计（引用计数独立于协程帧）是共享所有权的标准模式——`std::shared_ptr` 用了同样的设计。
- 等待者链表的"头插法"在单线程环境下简洁高效；多线程则需要 lock-free 或加锁。
- shared_task 的核心应用场景是"多个消费者等待同一个一次性结果"——如多协程等待同一个缓存计算完成。

### 常见坑

- `await_resume` 对结果做了 move 而非 copy——第一个 await 之后 result_value 为空，后续 await 拿到空值。
- 引用计数为 0 时 destroy 了协程帧，但忘了 delete control_block 自身——内存泄漏。
- final_suspend 中遍历等待者链表时若对每个节点都用 `.resume()` 再对头节点额外 symmetric transfer——头节点会被 resume 两次，这是 UB。正确做法：前 N-1 个用 `.resume()`，最后一个用 symmetric transfer。
- 等待者链表的 node 是栈上的局部变量，但 `await_suspend` 返回后栈帧已销毁——node 成了悬空指针。
- 在移动赋值运算符中忘了释放旧的 control_block——引用计数泄漏。

### 提示

- 先实现单线程版本（无原子计数），把整体流程跑通，再加 `std::atomic` 和多线程安全。
- 等待者链表的生命周期管理是最棘手的部分——建议先用 `std::list<std::coroutine_handle<>>` 简化（不最优但能跑）。
- 对照 cppcoro 的 `shared_task.hpp` 检查你的实现，它的代码量约 150 行——如果远远超出这个量，可能过度设计了。
- 这道题和 D-1（lazy_task）的直接对比例子能让你最清楚地看到 shared 和 unique 所有权的设计差异。

### 复盘问题

- 为什么 shared_task 必须返回值的拷贝而不是移动？这对其性能特征有什么影响？
- 如果 shared_task 的引用计数递减到 0 但协程尚未执行完，会发生什么？（提示：这和 `std::shared_ptr` 的生命周期语义一致。）
- 等待者链表在 final_suspend 中应该如何以 symmetric transfer 方式唤醒所有等待者？如果只有一个等待者呢？
- 在什么场景下你应该用 shared_task 而不是 lazy_task？反过来呢？

### 对应官方参考

- Lewis Baker "C++ coroutines: Sharing coroutines"（系列第 7 篇）
- cppcoro `shared_task.hpp` 完整实现
- P3175R0 中 shared task 的语义讨论

---

## 练习 G-2：实现 when_all<T...>

### 目标

亲手实现 `when_all<T...>`，让 N 个子 task 并行执行，最后一个完成的子 task 自动唤醒等待者，并用 `std::tuple` 汇合所有结果。同时实现错误合并策略——多个子 task 同时失败时，决定"谁的异常胜出"或合并所有异常。

### 前置理解

- `when_all` 是一个 barrier：所有子 task 都完成后，等待者才被唤醒。
- 实现 `when_all` 的核心是两个问题：
  1. **计数问题**：如何原子地追踪"还剩几个未完成"。
  2. **汇合问题**：如何将 N 个不同类型的结果放入一个 tuple 中。
- 错误处理有三种典型策略：
  - **Fail-fast**：第一个子 task 失败时立即取消其他子 task，传播该错误。
  - **Fail-delay**：所有子 task 都完成后，如果有任何失败，传播第一个错误。
  - **Exception aggregation**：收集所有异常，一起传播（如 `std::nested_exception` 或 `std::exception_list`）。
  - 本练习默认采用**Fail-delay**策略（最简单、C++标准最倾向的方向），进阶任务可以尝试其他策略。
- 本习题假定所有子 task 的类型为 `lazy_task<T_i>`（move-only），`when_all` 接管它们的所有权。

### 必做任务

1. **实现 when_all 的返回类型**：

   ```cpp
   template<typename... Tasks>
   struct when_all_task {
       // 从 Tasks... 中推导各子 task 的结果类型
       using result_tuple = std::tuple<
           typename std::remove_reference_t<Tasks>::value_type...
       >;

       // 每个子 task 独立启动，共享同一个 barrier
       struct shared_state {
           std::atomic<int> remaining_{sizeof...(Tasks)};
           result_tuple results_{};
           std::exception_ptr first_error_{};
           std::coroutine_handle<> waiter_{nullptr};

           // 子 task 完成时调用的回调
           // 最后一次调用时唤醒 waiter
       };

       std::shared_ptr<shared_state> state_;
       std::tuple<Tasks...> tasks_;
   };
   ```

2. **实现每个子 task 的完成回调**：

   对于每个子 task `task_i`，创建一个包装协程，在 `co_await task_i` 完成后更新共享状态：

   ```cpp
   // 为第 I 个子 task 创建的等待协程（I 由 index_sequence 推导，非硬编码 0）
   template<size_t I, typename Task>
   lazy_task<void> when_all_child(shared_state* state, Task task) {
       try {
           auto result = co_await std::move(task);
           // 存入 tuple 的对应位置
           std::get<I>(state->results_) = std::move(result);
       } catch (...) {
           // 只存储第一个异常
           if (!state->first_error_) {
               state->first_error_ = std::current_exception();
           }
       }

       // 原子递减计数
       if (state->remaining_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
           // 我是最后一个完成的！
           if (state->waiter_) {
               state->waiter_.resume();
           }
       }
       co_return;
   }
   ```

3. **实现 when_all 的主协程（先做固定 2-task 版）**：

   以下先给出可编译的 2-task 固定类型版作为主任务起点：

   ```cpp
   template<typename T0, typename T1>
   lazy_task<std::tuple<T0, T1>> when_all_2(lazy_task<T0> t0, lazy_task<T1> t1) {
       // 启动两个 task
       t0.resume(); t1.resume();
       // 逐个 resume 直到各自完成（单线程简单驱动）
       while (!t0.h_.done()) t0.h_.resume();
       while (!t1.h_.done()) t1.h_.resume();
       co_return std::tuple{t0.await_resume(), t1.await_resume()};
   }
   ```

   变参版（进阶任务）：使用 `std::index_sequence_for<Tasks...>` 把索引带进 fold expression，实现最小可工作的 fold-with-index 模式。下面的伪骨架标注"以下是设计示意，并非可编译代码"：

   ```cpp
   // 设计示意——不可直接编译，需要配合 index_sequence 完成索引递增
   // 可编译的变参实现见进阶任务。
   template<typename... Tasks>
   auto when_all(Tasks... tasks) {
       // 借助 index_sequence_for 为每个 task 分配对应索引 I
       // 在 fold 中展开时用独立的模板参数 I 而非硬编码 0
       // [&]<size_t... Is>(std::index_sequence<Is...>) {
       //     (when_all_child<Is>(state.get(), std::move(tasks)), ...);
       // }(std::index_sequence_for<Tasks...>{});
   }
   ```

4. **单线程验证 + 错误处理**：

   `when_all_2` 的简易版已经在必做任务 3 中给出。进阶读者可以扩展为：
   - 并行启动多个子 task（如提交到线程池）
   - 在 `await_resume` 中检查 `first_error_` 并传播异常

   **错误合并策略实现**：

   ```cpp
   // Fail-fast 策略示例：
   // 在子 task 完成回调中，一旦检测到错误，立即通过类似 stop_source 的机制
   // 通知其他子 task 停止。简化版：直接抛异常中断等待者。

   // Fail-delay 策略示例（推荐默认）：
   // 所有子 task 完成后检查：如果有 any error，抛出第一个
   if (state->first_error_) {
       std::rethrow_exception(state->first_error_);
   }

   // Exception aggregation 策略示例：
   // 收集所有异常到 std::vector<std::exception_ptr>
   // 完成后抛出一个包含所有异常的聚合异常类型
   std::vector<std::exception_ptr> errors;
   // ... 收集 ...
   if (!errors.empty()) {
       throw aggregated_exception{std::move(errors)};
   }
   ```

6. **在笔记中画出 when_all 的状态转换图**：

   ```
   初始状态: remaining=N, waiter=null, results=empty
     |
     v
   子 task-1 完成: remaining=N-1, results[1]=...
   子 task-2 完成: remaining=N-2, results[2]=...
   ...
   子 task-k 完成: remaining=1 → 0, waiter.resume()!
     |
     v
   await_resume: 检查 first_error, 返回 tuple
   ```

### 进阶任务

- 实现真正的并行 `when_all`：将所有子 task 的 `coroutine_handle` 提交到线程池或 run_loop，让它们在不同线程上并行执行。
- 支持异构类型的子 sender/awaitable（不仅仅是 `lazy_task<T>`）——任何可以 `co_await` 的东西都应该能成为 `when_all` 的输入。
- 实现 `when_any`：第一个完成的子 task 触发等待者，其余子 task 被取消/丢弃。
- 实现 `when_all` 的 cancel 传播：如果 waiter 被取消（stop_token 触发），所有未完成的子 task 收到取消信号。
- 让 `when_all` 支持编译期推导 `result_tuple`：通过 `decltype(co_await task)` 而非显式指定 `Task::value_type`。

### 验收点

- 你的 `when_all` 能正确汇合两个不同类型的结果（如 `lazy_task<int>` 和 `lazy_task<std::string>`）。
- 当所有子 task 都成功时，`await_resume` 返回的 tuple 包含正确的结果。
- 当任一子 task 失败时，错误被正确传播给等待者。
- 你能解释为什么 `remaining_` 必须是 `std::atomic<int>` 而不是普通 `int`。
- 你能画出计数从 N 递减到 0 的每一步含义。

### 观察点

- `when_all` 的核心是一个 barrier + 结果槽位数组。每一个子 task 完成时原子减一，最后一个触发唤醒。
- 结果的汇合需要编译期"知道每个槽位的类型"——`std::tuple` 天然支持这一点，因为每个位置类型在编译期确定。
- 错误处理策略是 when_all 设计中最有争议的部分——标准库 (`std::execution::when_all`) 倾向于 fail-fast + 取消传播，但具体实现仍在演进中。
- 在单线程模式中，`when_all` 等价于"顺序 co_await"——没有真正的并行。要并行，必须有多线程调度基础设施。

### 常见坑

- `fetch_sub` 的 memory order 写得过弱（如 `memory_order_relaxed`）——最后一个完成的子 task 可能看不到其他子 task 写入的结果。
- 在 waiter 的 `await_suspend` 中先设置 waiter 再检查计数，但中间可能已被最后一个子 task 唤醒——导致 `resume` 被调用两次。
- `result_tuple` 的构造方式不正确——对 move-only 类型调用了拷贝构造。
- 多个子 task 同时设置 `first_error_` 产生 data race——需要用 `std::atomic` swap 或 `compare_exchange` 保护。
- `when_all` 返回的 task 自身也需要被 await/消费——如果丢弃它，子 task 继续跑但结果无人接收（fire-and-forget）。

### 提示

- 先用两个固定类型的 task（如 `lazy_task<int>` 和 `lazy_task<double>`）实现 `when_all_2`，验证逻辑正确后再泛化为变参模板。
- `std::apply` + fold expression 是实现变参并行启动的简洁方式。
- 错误合并可以先用最简单的"第一个异常胜出"策略，其他策略作为进阶。
- 不要一开始就追求 lock-free——用 `std::atomic` 的正确 memory order（至少 `acq_rel`）是更稳妥的起点。
- 这道题的核心是计数和结果收集的协作——画好状态转换图再写代码。

### 复盘问题

- 为什么 `when_all` 不能简单地用 N 个 `co_await` 串行实现——如果串行的话，第二个 task 必须在第一个完成后才开始，这就不是"并行"了？
- 如果 `when_all` 中的一个子 task 永不完成，整个系统会怎样？如何用 timeout 机制保护？
- 你选择的错误合并策略在什么场景下最优？在什么场景下会丢重要信息？
- `when_all` 和 `when_any` 在底层实现上有什么本质区别？谁更复杂？

### 对应官方参考

- Lewis Baker "C++ coroutines: Composing coroutines"（系列第 8 篇）
- Lewis Baker "C++ coroutines: Concurrency with coroutines"（系列第 9 篇）
- cppcoro `when_all.hpp` 完整实现
- P3175R0：sender-receiver 中的 when_all 语义
- P2300R10：`std::execution::when_all` 规范

---

## 练习 G-3：实现 sync_wait

### 目标

实现一个 `sync_wait` 函数，用 condition_variable + 内置 receiver 风格，在非协程上下文中驱动 task 到完成。将协程的挂起-恢复转化为阻塞-唤醒模式，理解"协程世界"和"同步世界"之间的桥接原理。

### 前置理解

- `sync_wait` 的本质：在普通函数中创建一个协程 task，然后**阻塞当前线程**等待协程完成。这是"把异步带回同步"的标准桥接模式。
- 为什么需要 `sync_wait`：因为 `main` 函数不能是协程（标准没有规定谁驱动 main 协程），所以必须有一个"从同步世界进入协程世界"的入口。
- 实现策略：创建一个 condition_variable 和一个 shared state。当协程完成时（final_suspend），设置标志并通知 condvar。在 `sync_wait` 中，启动协程后 wait condvar，醒来后检查错误并返回结果。
- 核心挑战：协程可能在 `sync_wait` 进入 wait 之前就完成了——需要使用互斥锁做双重检查。

### 必做任务

1. **实现 `sync_wait` 的基础版本**（针对 `lazy_task<T>`）：

   ```cpp
   template<typename T>
   T sync_wait(lazy_task<T> task) {
       // 1. 创建共享状态
       struct shared_state {
           std::mutex mtx;
           std::condition_variable cv;
           bool done = false;
           std::exception_ptr error;
           std::optional<T> result;  // optional 以支持 void 返回
       };
       auto state = std::make_shared<shared_state>();

       // 2. 将原始 task 与"完成通知"包装在一起
       //    我们使用一个包装协程，完成后设置 state->done 并 notify
       struct notifier {
           std::shared_ptr<shared_state> state_;

           // 在独立的协程中运行 task，然后通知
           // 注意：这里需要一个 fire-and-forget 协程，或直接用 task.resume()
           void operator()(lazy_task<T> t) {
               try {
                   // 手动驱动 task 的非协程方式
                   // （需要在 sync_wait 中直接驱动，不经过包装协程）
               } catch (...) {
                   state_->error = std::current_exception();
               }
               {
                   std::lock_guard lk(state_->mtx);
                   state_->done = true;
               }
               state_->cv.notify_one();
           }
       };

       // 3. 在独立线程或手动驱动协程，然后阻塞等待
       // 简化方案：在我们自己的线程中驱动协程到完成
       std::thread driver([&task, state]() {
           try {
               // 驱动协程：resume 直到完成
               task.resume();  // 从 initial_suspend 挂起后执行协程体
               // 协程体在执行过程中可能 co_await 其他 awaitable
               // 在 single-thread 环境下，所有 await_suspend 都会
               // 把 handle 存下来，由外部循环 resume
               while (!task.h_.done()) {
                   task.h_.resume();
               }
               auto& p = task.h_.promise();
               if (p.result_exception) {
                   state->error = p.result_exception;
               } else {
                   state->result = std::move(p.result_value);
               }
           } catch (...) {
               state->error = std::current_exception();
           }
           {
               std::lock_guard lk(state->mtx);
               state->done = true;
           }
           state->cv.notify_one();
       });

       // 4. 阻塞等待
       {
           std::unique_lock lk(state->mtx);
           state->cv.wait(lk, [&] { return state->done; });
       }
       driver.join();

       // 5. 返回结果或重抛异常
       if (state->error) {
           std::rethrow_exception(state->error);
       }
       return std::move(*state->result);
   }
   ```

2. **实现更完善的版本：内联驱动**。

   上面的实现用了一个额外的 `std::thread`，在真正的工程实现中，更好的做法是在调用 sync_wait 的线程上直接驱动协程——协程的执行和 sync_wait 的等待交替进行：

   ```cpp
   template<typename T>
   T sync_wait(lazy_task<T> task) {
       struct state_t {
           std::mutex mtx;
           std::condition_variable cv;
           bool done = false;
           bool started = false;
           std::exception_ptr error;
           std::optional<T> result;
       };
       auto state = std::make_shared<state_t>();

       // 方案 B：用 condvar 在内联模式中交替
       // 每次协程挂起时 notify，sync_wait 醒来后决定是否继续 resume
       // 这是更"协程原生"的模式

       // 自定义 awaiter：挂起时通知 condvar
       struct sync_awaiter {
           std::shared_ptr<state_t> state_;

           bool await_ready() noexcept { return false; }
           void await_suspend(std::coroutine_handle<> h) noexcept {
               {
                   std::lock_guard lk(state_->mtx);
                   state_->started = true;
               }
               state_->cv.notify_one();
               // 不在 await_suspend 中 resume——让 sync_wait 的循环来 resume
               // 注意：handle 需要保存下来供后续 resume
               // 简化版：存在 state 中
           }
           void await_resume() noexcept {}
       };

       // 包装协程：在 co_await 点之前先 co_await sync_awaiter
       // ... 完整实现较复杂，骨架省略，核心思想如上
   }
   ```

   实际上，最简单的内联版本是**完全不依赖 condvar，直接用 while 循环驱动协程**：

   ```cpp
   template<typename T>
   T sync_wait(lazy_task<T> task) {
       task.resume();  // 启动

       // 驱动循环：只要协程未完成，就 resume
       // 协程在各种 co_await 点挂起后，await_suspend 不自己 resume
       // 本循环不断 resume 直到 final_suspend 被执行
       while (!task.h_.done()) {
           task.h_.resume();
       }

       auto& p = task.h_.promise();
       if (p.result_exception) {
           std::rethrow_exception(p.result_exception);
       }
       return std::move(p.result_value);
   }
   ```

   这个简化版在**所有 await_suspend 都返回 void 且没有旁路 resume** 的场景下是正确的。它每个 iteration 对应一次"从挂起到恢复"的驱动。

3. **实现更健壮的内联驱动**：

   上面的简化版有一个致命问题：如果某个 awaiter 的 `await_suspend` 中又 resume 了当前协程（或 symmetric transfer 到了其他协程），那么简单的 while 循环会乱。完整实现需要跟踪"当前活跃的协程 handle"。

   ```cpp
   template<typename T>
   T sync_wait(lazy_task<T> task) {
       auto handle = task.h_;
       handle.resume();  // 从 initial_suspend 开始

       while (!handle.done()) {
           handle.resume();  // 从上次挂起点恢复
       }

       auto& p = handle.promise();
       if (p.result_exception)
           std::rethrow_exception(p.result_exception);
       return std::move(p.result_value);
   }
   ```

   这个版本在 simples 的嵌套协程链上工作良好：`coro_A co_await coro_B`——coro_A 的 `await_suspend`（返回 `coro_B`的 handle）将控制权以 symmetric transfer 方式交给 coro_B，但因为我们没有运行在协程框架内，我们需要主动检测并 resume。

4. **写测试验证 sync_wait**：

   ```cpp
   lazy_task<int> async_compute() {
       int a = co_await async_value(10);   // async_value 是一个简单的模拟异步 awaitable
       int b = co_await async_value(20);
       co_return a + b;
   }

   // 在 main 中
   int main() {
       int result = sync_wait(async_compute());
       std::println("result = {}", result);  // 期望 30
       return 0;
   }
   ```

5. **在笔记中回答**：
   - 为什么 `main` 不能是协程？——C++ 标准没有定义"main 协程"的启动机制：谁负责创建 main 协程的 frame？谁负责在 main 协程的 initial_suspend 之后调用 resume？谁负责在 main 协程结束后销毁 frame？这些在标准中都没有规定。
   - `sync_wait` 承担了什么角色？——它是"协程世界"和"同步世界"之间的翻译器。它把协程的挂起/恢复转换为 block/wake，把 `co_return` 的值传递给同步调用者的返回语句。
   - condvar + receiver 模式和手动循环驱动模式各自的适用场景是什么？——condvar 适合协程可能在另一个线程上完成的场景（如 io_uring completion）。手动循环适合单线程全协程环境（如 asio io_context 中的协程）。

### 进阶任务

- 为 `sync_wait` 添加 timeout 支持：如果 task 在指定时间内未完成，抛 timeout 异常并销毁协程帧。
- 实现一个可以同时作为 `sync_wait` 和 `co_await` 使用的通用驱动函数——它检测当前上下文是否在协程内，如果在则 `co_await`，如果不在则 `sync_wait`。
- 让 `sync_wait` 支持 void 返回类型的 task（`lazy_task<void>`）。
- 用 `std::atomic_flag` 替代 condvar 实现一个自旋版本 `sync_wait`（spin-wait），对比两种方案的 CPU 使用率。

### 验收点

- 你的 `sync_wait` 能在 `main` 中正确驱动一个协程 task 到完成。
- 协程体中的异常能被 `sync_wait` 正确捕获并重新抛出。
- 你能解释为什么 `main` 不能是协程——从协程帧的分配、启动、销毁三个角度说明。
- 你能说明 condvar 版本和手动循环版本各自的适用场景和局限性。

### 观察点

- `sync_wait` 在概念上非常简单——驱动协程直到完成并取出结果。但它的实现暴露了协程和同步世界之间的根本张力：协程期望有人"异步地"resume 它，而 sync_wait 要求"同步地"等待。
- condvar 版本的 `sync_wait` 是生产代码中最常见的模式——它支持多线程调度，不浪费 CPU 自旋。
- 手动循环驱动版本在单线程事件循环（如 asio io_context）中更自然——因为 resumer 和 awaiter 在同一个线程上交替执行。
- `sync_wait` 让协程"看起来像一个同步函数"——这是异步代码测试的基础设施。

### 常见坑

- 协程在 `sync_wait` 之前就已经完成（eager 启动），condvar 在 wait 之后永远不会被 notify——需要用 `done` 标志 + 锁做双重检查。
- 手动循环版在 `await_suspend` 返回 `bool` 时行为正确吗？——如果返回 `true`（不挂起），协程继续执行，下一次 while 的 `done()` 检查将返回 false（尚未到 final_suspend），会错误地再 resume 一次。
- `handle.done()` 只在 final_suspend 的 `await_suspend` 返回后才返回 `true`——在此之前，即使协程体执行完了，`done()` 也返回 `false`。
- 在 manual 循环中 resume 协程后，没有检查协程是否在 resume 内部就完成了——导致多余的 resume 调用（不过多余的 resume 通常无副作用）。

### 提示

- 从手动循环版本开始实现——它最简单，适合理解基本流程。
- 然后升级到 condvar 版本——理解多线程同步的挑战。
- `sync_wait` 的代码量很小（约 30-60 行），不要在辅助设施上过度设计。
- 这道题的关键不是代码量，而是对"协程执行模型 vs 同步阻塞模型"之间转换的理解。

### 复盘问题

- 如果 `sync_wait` 在一个协程内部被调用（即从协程 A 调用 sync_wait 驱动协程 B），会发生什么？会阻塞 A 吗？有没有更好的方式？
- 为什么标准没有规定 main 可以是一个协程？如果需要 main 协程，哪个实体来驱动它？
- `sync_wait` 的 condvar 版本和 `std::latch` / `std::barrier` 有什么本质区别？
- 如果你要在生产代码中实现 `sync_wait`，你希望它支持哪些额外特性？

### 对应官方参考

- cppcoro `sync_wait.hpp` 实现
- Lewis Baker "C++ coroutines: Building a sync_wait"（系列第 9 篇）
- P3175R0 中 `this_thread::sync_wait` 的语义
- `stdexec` 中 `sync_wait` 的实现

---

## 做完模块 G 之后，你现在应该能说清楚什么

至少把下面几句话说顺：

- `shared_task<T>` 通过独立的 control_block + 引用计数实现多线程共享。final_suspend 遍历等待者链表唤醒所有 `co_await` 它的协程。结果必须返回拷贝而非移动——这是共享语义的代价。
- `when_all<T...>` 的核心是一个原子 barrier：每个子 task 完成时减一，最后一个触发 waiter 恢复。结果的编译期类型汇合依赖 `std::tuple`。错误处理的策略选择（fail-fast / fail-delay / aggregation）直接决定了组合子的容错语义。
- `sync_wait` 是协程世界向同步世界的桥接：condvar + 互斥锁替代了挂起/恢复，让 `main` 这类不能是协程的函数也能消费协程结果。它的实现揭示了协程"挂起-恢复"与线程"阻塞-唤醒"之间的精确对应关系。
- 这三道题共同展示了"协程不只是写线性异步代码"——shared_task 涉及共享所有权与并发安全，when_all 涉及并行组合与错误策略，sync_wait 涉及跨模型桥接。这些是工程级协程基础设施的核心构件。