# 03 模块 B：generator 与 task 的使用

## 模块目标

模块 A 让你建立了三个关键字的肌肉记忆。模块 B 要把 generator 和 task 的使用技能向前推一步：

- generator 不只是线性 yield，递归 generator 可以表达树形遍历，而 symmetric transfer 让递归不爆栈。
- task 不只是单步 `co_return`，多个 task 的 `co_await` 串联可以表达顺序异步组合，且异常沿 `co_await` 链自然传播。
- 真实工程中大量的回调式 API（如文件读取、网络请求、定时器）可以通过写出最小 awaiter 适配器，变成可 `co_await` 的协程友好接口。

## 模块完成标准

做完本模块，你至少要能稳定说清楚：

- 递归 generator 中每次 `co_yield` 停在哪一层协程帧上，symmetric transfer 如何保证树遍历的栈不爆炸。
- `co_await task1(); co_await task2(); co_return combine(...)` 与回调金字塔相比，好在哪（异常传播、值流显式、控制流线性）。
- 如何把"传入回调 + 异步启动"模式的 C API，仅通过 ~25 行 awaiter 代码变成协程可消费的 `co_await`。

## 使用约定

- 本模块沿用模块 A 的 `lazy_task.hpp` 头文件。
- generator 使用 C++23 `<generator>`。
- 所有日志都建议打印：阶段名、协程标识、线程 ID。
- 注意观察点中的栈行为和协程帧归属问题。

---

## 练习 B-1：递归 generator 与栈

### 目标

用递归 generator 实现二叉树的中序遍历（惰性 yield 每个节点值），亲手观察对称传输（symmetric transfer）如何在编译器层面防止递归 yield 爆栈。

### 前置理解

- 你知道 `std::generator<T>` 可以用递归函数实现：在协程内部 `co_yield` 当前节点值，然后递归遍历子树。
- 你知道朴素递归函数遍历一棵深层树时，每一层递归都会压栈，深度树可能爆栈。
- 你听过"symmetric transfer"这个词，但请特别留意：**仅 `co_yield std::ranges::elements_of(inner_gen)` 这一 P2502R2 recursive yield 语法才能触发 symmetric transfer。普通的 `for (int v : recurse(node)) co_yield v;` 不触发 symmetric transfer——它是普通迭代器层层 resume，深度退化树仍爆栈。**
- 你接受本题的重点是观察递归 yield 的栈行为，并亲手对比两种写法的栈安全差异。

### 必做任务

1. 定义一个简单的二叉树节点结构：
   ```cpp
   struct Node {
       int value;
       Node* left;
       Node* right;
   };
   ```
2. 构造一棵深度至少 5 层的满二叉树（节点值可以简单设为 1 到 2^depth-1），但至少包含 20+ 个节点。
3. 写两个版本的递归 generator 函数 `inorder(Node* root)`，返回 `std::generator<int>`：
   - **版本 A（推荐，P2502 symmetric transfer）**：左子树用 `co_yield std::ranges::elements_of(inorder(root->left));` 递归 yield。这是 C++23 P2502R2 的 recursive yield 语法，能触发 symmetric transfer——编译器直接跳转到子 generator 的帧，深度树不爆栈。
   - **版本 B（反例）**：左子树用 `for (int v : inorder(root->left)) co_yield v;` 递归 yield——这是普通迭代器层层 resume，每层 generator 的 `++it` 都进入 caller 栈帧，深度退化树仍爆栈。
   - 两个版本都通过 `co_yield root->value;` 产出当前节点值，然后用相同方式处理右子树。
4. 在 `main()` 中用 range-based for 遍历这个 generator，打印所有值，验证输出是正确的中序遍历序列。
5. 在递归 generator 函数入口和每个 `co_yield` 前后加日志。观察：当递归进入深层左子树并开始 yield 最左叶子时，日志的顺序是怎样的。
6. 画一张调用图：标注最左叶子被 yield 时，协程帧栈上每一层父 generator 分别挂起在哪个 `co_yield` / `for` 循环的哪个迭代位置。

### 进阶任务

- 构造一棵退化树（每个节点只有右子树或只有左子树），深度达到 200 或 500。用普通递归函数遍历这棵树——观察是否爆栈。然后用版本 A（`elements_of`，symmetric transfer）和版本 B（`for` + `co_yield`）分别遍历同一棵树——**验证版本 A 不爆栈，版本 B 爆栈**。记录两者的栈行为差异和对 P2502 准确理解的重要性。
- 不用 `for (int v : recurse(left)) co_yield v;` 这种"用户层递归"方式，而是直接在 `promise_type` 层面给 generator 加一个 `yield_from(generator&&)` 方法（类似 Python 的 `yield from`）。这要求你在 generator 的 `promise_type` 中实现 `await_transform` 来拦截对另一个 generator 的 `co_await`。这是一个接近二级难度的任务，做不出来可以先跳过，但思考其设计意图。
- 在 GCC 上用 `-fdump-tree-coro`，或在 Clang 上用 `-Xclang -ast-dump -fsyntax-only`，观察编译器为递归 generator 生成的协程帧结构。尝试从中辨认出：哪些字段是你写的局部变量，哪些是编译器生成的簿记字段（如当前状态点、resume 地址等）。

### 验收点

- 你能用中序遍历的正确输出证明递归 generator 的逻辑正确性。
- 你能在日志中清晰地看到：深层左子树的 `co_yield` 在浅层节点的 `co_yield` 之前被执行（符合中序遍历语义），并且浅层节点的 for 循环被"冻结"在等待下一个值的状态。
- 你能解释：为什么版本 B（`for` + `co_yield`，不触发 symmetric transfer）在深度退化树上会和普通递归函数一样爆栈——它的每一层 `++it` 都进入 caller 栈帧。版本 A（`elements_of`，触发 P2502 symmetric transfer）才不爆栈。
- 你能画出退化树遍历时协程帧之间的 resume 链。

### 观察点

- 递归 generator 的每一层递归调用都会创建一个新的协程帧（每一层都是一个独立的协程实例，有自己的 coroutine_handle，独立分配在堆上）。
- 版本 A 中使用 `co_yield std::ranges::elements_of(inner_gen)` 时，子 generator 的 `final_suspend` 通过 symmetric transfer 直接跳回父帧——编译器在编译期识别 `elements_of` 语法并生成 symmetric transfer 路径。版本 B 的 `for (int v : ...) co_yield v;` 是普通迭代器 resume 链：每层 `operator++` 都进入上一层帧再返回，栈深度随树深线性增长。
- 如果你在进阶任务中试了退化树，你会亲眼看到：普通递归和版本 B 都在几百层爆栈，而版本 A（`elements_of`）安然无恙——这是 P2502 symmetric transfer 最直观的价值证明。
- 注意：`elements_of` 语法的 symmetric transfer 支持取决于编译器和版本。MSVC 17.10+、Clang 17+、GCC 14+ 通常都支持，但如果你的编译器较旧，可能需要确认。

### 常见坑

- `for (int v : inorder(left)) co_yield v;` 这种写法的前提是 `inorder(left)` 返回的 generator 临时对象的生命周期要覆盖整个 for 循环。幸运的是，range-based for 的规范保证了这一点（临时对象存活到循环结束）。
- 递归 generator 中，每一层递归都会创建一个独立的 generator。如果不对这些临时 generator 的生命周期有清楚认识，很容易写出 dangling reference。
- 在退化树实验中，如果使用版本 B（`for` + `co_yield`）则爆栈是必然的——这不是编译器限制，而是 `for` + `co_yield` 本来就不触发 symmetric transfer。只有版本 A（`elements_of`）能在退化树上安全遍历。请务必区分两者。
- 把"for 循环展开递归 yield"等同于"symmetric transfer"——这是本模块最核心的纠正点。P2502R2 的 symmetric transfer 仅由 `co_yield std::ranges::elements_of(...)` 触发，`for (int v : inner_gen) co_yield v;` 不触发。

### 提示

- 树节点的构造可以用简单的 `new`（在教学代码中），但注意不用时别忘了 `delete`，或者直接用 `std::unique_ptr<Node>` 管理。
- 日志可以用一个全局递增计数器来追踪执行顺序，让交叉的协程执行序更容易读懂。
- 如果没有条件使用 GCC/Clang 的 dump 功能，可以先关注 MSVC 的汇编输出，用 `/d1reportSingleClassLayout` 观察帧结构。
- 如果 symmetric transfer 部分理解困难，先确保你理解了模块 A 中的 awaiter 三方法，特别是 `await_suspend` 可以返回 `coroutine_handle` 这一点——symmetric transfer 就是 `std::generator` 的 final_suspend 内部做了这件事。

### 复盘问题

- 递归 generator 中，每一层子树对应一个独立的协程帧。当树有 100 层深时，内存中有多少个 generator 对象？多少个协程帧？
- P2502R2 的 `elements_of` 语法如何触发 symmetric transfer？版本 B（`for` + `co_yield`）为什么做不到？
- 为什么说 `for (int v : inner_gen) co_yield v;` 和 `co_yield std::ranges::elements_of(inner_gen);` 在栈行为上有本质差异？哪个更接近尾调用优化（tail call optimization）的协程等价物？
- 这题中的 generator 递归模式，和 Python 的 `yield from` 语法有什么异同？

### 对应官方参考

- P2502R2：`std::generator` 中 symmetric transfer 的使用
- Lewis Baker 协程系列第 5 篇：symmetric transfer 详解
- Raymond Chen 协程系列第 6 篇：generator 的嵌套与递归

---

## 练习 B-2：task<T> 顺序异步组合

### 目标

用 `lazy_task<T>` 串联多个"伪异步"操作（`fetch` -> `parse` -> `validate`），每个操作都是一个独立的协程 task。通过 `co_await` 把三个任务串起来，让最终结果自然流到 `co_return`。对比等效的回调金字塔写法，体会协程如何让异步顺序看起来像同步代码。

### 前置理解

- 你已经从 A-2 理解了 `lazy_task<T>` 的基本用法。
- 你知道 `co_await some_task` 的语义是：挂起自己，等 `some_task` 完成后取走它的返回值。
- 你理解"异常沿 co_await 链自然传播"的含义：如果子 task 抛出了异常（进入了 `unhandled_exception`），那么父协程的 `co_await` 表达式会重新抛出这个异常，父协程可以选择 `try-catch` 处理或让异常继续向上传播。
- 你接受本题的"异步操作"全部用纯计算模拟（`std::this_thread::sleep_for` 仅用于模拟耗时），不需要真正的网络或 I/O。

### 必做任务

1. 写三个独立的协程函数，每个返回 `lazy_task<SomeType>`（类型自定）：
   - `fetch_user(int user_id)`：返回 `User` 结构体（含 `id`、`name`、`email`）。模拟耗时：在协程体内加一个 ~50ms 的 sleep。
   - `parse_profile(User user)`：返回 `Profile` 结构体（含 `user_id`、`display_name`、`bio`）。模拟耗时：~30ms sleep。
   - `validate_profile(Profile profile)`：返回 `ValidatedProfile` 结构体（含原始 profile 字段 + `is_valid: bool` + `score: int`）。模拟耗时：~20ms sleep。
2. 写一个顶层协程 `process_user(int user_id)`，返回 `lazy_task<std::string>`：
   ```cpp
   auto user    = co_await fetch_user(user_id);
   auto profile = co_await parse_profile(user);
   auto result  = co_await validate_profile(profile);
   if (result.is_valid)
       co_return std::format("User {} ({}) validated with score {}", result.user_id, result.display_name, result.score);
   else
       co_return std::format("User {} ({}) failed validation", result.user_id, result.display_name);
   ```
3. 在 `main()` 中用 `sync_wait` 消费 `process_user`，打印最终结果字符串。
4. 在每个协程的入口和 `co_return` 前都加日志（含协程名、线程 ID）。构造一条清晰的日志时间线。
5. 给 `fetch_user` 加一个"30% 概率抛异常"的逻辑（随机数）。当它抛异常时，观察异常是否沿 `co_await` 链自然传播到了 `sync_wait`（`sync_wait` 中 `std::rethrow_exception` 重新抛出）。
6. 写等效的回调版本（嵌套 lambda 或 `std::function` 链）做对比：`fetch_user(id, [](User u){ parse_profile(u, [](Profile p){ validate_profile(p, [](auto r){ ... }); }); });`。比较两者在处理错误路径和中间值的差异。
7. 画一张协程调用树：`process_user` 在 `co_await fetch_user` 处挂起，`fetch_user` 被 resume 并执行；`fetch_user` 完成后 `process_user` 恢复并在 `co_await parse_profile` 处再次挂起……直到最终的 `co_return`。

### 进阶任务

- 把上面三个操作由纯顺序改成"同时启动、再分别等待"的并发模式，用 `lazy_task` 配合 `when_all` 的等价物。由于第一阶段的 `lazy_task` 不支持直接 when_all，你需要模拟：先创建三个 task，再用 `co_await` 分别等待。思考：这样做的并发效果如何？每个 task 是何时被第一次 resume 的？
- 在 `process_user` 的顶层加 `try-catch`，捕获 `fetch_user` 或 `parse_profile` 可能抛出的异常，并降级返回一个默认字符串。验证：协程体内的 `try-catch` 是否能像同步代码一样工作。
- 把其中一个操作的返回类型改为 `lazy_task<std::optional<T>>`（模拟"可能无结果"），在 `process_user` 中用 `if (!result.has_value()) co_return "failed";` 处理无结果情况。体会 `co_await` 链上类型的变化。

### 验收点

- 你的日志顺序清楚地展示了：`fetch_user` 先打印，完成；然后 `parse_profile` 打印，完成；最后 `validate_profile` 打印，完成——完全是顺序执行。
- 你证明了当 `fetch_user` 抛异常时，异常沿 `co_await` 链传播到 `sync_wait`，`parse_profile` 和 `validate_profile` 的日志完全没有出现。
- 你能说明回调版本中，"错误处理"分散在每一层回调的参数里，而协程版本中，错误处理可以集中在 `try-catch` 块中——就像同步代码一样。
- 你能指出每个 `co_await` 点是当前协程的潜在挂起点，也是异常传播的通道入口。

### 观察点

- `co_await task1(); co_await task2(); co_await task3();` 这行代码读起来像三个同步函数调用，但实际每一步都伴随着挂起-恢复循环。这种"看起来像同步，本质是异步"的体验，是协程最大的工程价值。
- 如果你在进阶任务中做了"同时启动三个 task"的版本，你会发现虽然创建 task 时不执行（lazy），但你需要在 `co_await` 之前手动第一次 resume 它们才能让它们开始并发运行。这引出了一个核心问题：lazy task 在并发场景下的局限性——这也是为什么 C++26 `std::execution::task<T>` 和 Folly 的 `Task<T>` 选择了不同的启动策略。
- 异常在 `co_await` 链上的传播是自动的：子协程的 `unhandled_exception` 保存了 `exception_ptr`，父协程的 `co_await` 在 `await_resume` 中重新抛出。这个机制由 promise_type 和 awaiter 协作完成，不是语言层面的魔法。

### 常见坑

- 在 `process_user` 中忘了 `co_await`，写成了 `auto user = fetch_user(id);`——这样 `user` 会是一个 `lazy_task<User>` 对象，而不是 `User`。编译器可能不会报错（如果 `User` 可以从 `lazy_task<User>` 构造），但运行时会错乱。
- `fetch_user` 中抛异常后，忘记在 `process_user` 中 `try-catch` 或让异常继续传播，导致程序 `std::terminate`（因为异常从一个 `noexcept` 的 context 中逃逸）。注意：A-2 的 `lazy_task` 头文件中的 `sync_wait` 会 `std::rethrow_exception`，这是有意为之——让调用方决定如何处理异常。
- 在 `validate_profile` 的必做任务中，用 `sleep_for` 模拟耗时时，忘了 sleep 不释放 CPU 线程——这会让整个过程变成完全的串行堵塞。这用于本题的练习目的完全可以接受，但要意识到真实工程中的 `co_await` 是有真正的异步等待语义的。
- 认为"co_await 链上的协程一定在同一个线程执行"。实际上线程取决于每个子 task 的 awaiter 如何实现 `await_suspend`。在 `lazy_task` 的最小实现中，`final_suspend` 的 awaiter 是 `suspend_always`，没有跨线程 resume——所以本题中所有协程都跑在同一个线程（调用 `sync_wait` 的主线程）。这在模块 C 和模块 I 中会改变。

### 提示

- 上面的代码示例用了 `fmt::format`（C++20 的 `std::format` 或 `{fmt}` 库），如果你还在用 C++17 风格，可以用 `std::ostringstream` 替代。
- 异常测试中，用 `rand() % 100 < 30` 即可，不需要真随机分发库。
- 回调版本不需要写完整的异步框架，用 `std::function` 嵌套即可看清控制流差异。
- 如果你觉得三个操作太简单，可以加第四个操作（比如 `enrich_profile` 或 `audit_log`），但不要增加太多让复盘走偏。

### 复盘问题

- 为什么"三个 co_await 串行"读起来像同步代码，但执行时每步之间有挂起-恢复循环？这种"同写异步"的能力是由协程帧和 awaiter 的哪些机制支撑的？
- 回调版本中，错误处理分布在每一层回调里。协程版本中，错误处理可以集中在顶层的 `try-catch`。造成这种差异的根本原因是什么？
- 如果你需要 `fetch_user` 和 `parse_profile` 并发执行（两者没有数据依赖），当前的 `co_await` 串行写法能满足吗？如果不能，缺少什么？
- `lazy_task` 的哪种设计导致了"创建 task 时不执行，只有被 co_await 才第一次 resume"？如果把这个设计改成创建时就执行（eager），对并发组合的写法有什么影响？

### 对应官方参考

- Lewis Baker 协程系列第 4 篇：task 的异步组合
- Raymond Chen 协程系列第 4-5 篇：co_await 链与异常传播
- cppcoro：`task.hpp` 中 `await_suspend` 的实现

---

## 练习 B-3：把回调 API 包成 awaiter

### 目标

给定一个典型的老式回调异步 API（`void async_read(Args..., Callback)`），写出一个最小 awaiter，让它可以被 `co_await` 消费。这是协程工程化的"第一道坎"——把任何回调世界里的异步操作，拉进协程世界的 `co_await`。

### 前置理解

- 你已经从 A-3 亲手写过一个 awaiter（`future_awaiter`），知道三方法的签名和调用时机。
- 你知道回调 API 的通用模式：你传入一个 callback，库在操作完成时调用这个 callback。
- 你理解本题的核心技巧：`await_suspend` 中保存当前协程的 `coroutine_handle`，在 callback 中调用 `handle.resume()`。
- 你接受本题的"回调 API"是模拟的（纯计算+回调），不需要真实的网络或文件 I/O。

### 必做任务

1. 写一个模拟的回调式异步 API：
   ```cpp
   template <typename Callback>
   void async_add(int a, int b, Callback cb) {
       // 模拟异步：在新线程中计算 a+b，然后调用 cb(result)
       std::thread([=] {
           std::this_thread::sleep_for(std::chrono::milliseconds(100));
           cb(a + b);
       }).detach();
   }
   ```
2. 写一个 awaiter 类型 `AsyncAddAwaiter`：
   - 构造函数接收 `int a, int b`。
   - `await_ready()`：返回 `false`（因为结果一定还在计算中）。
   - `await_suspend(std::coroutine_handle<> h)`：保存 `h`，然后调用 `async_add(a, b, [this, h](int result) { this->result_ = result; h.resume(); })`。回调中将结果存储到 awaiter 的成员 `result_` 中，然后 resume 协程。注意：这里需要线程安全——回调在另一个线程运行，对 awaiter 成员 `result_` 的写入和 resume 之后协程体内对 `result_` 的读取形成了跨线程 happens-before 关系。最简单的保证方式是确保 `h.resume()` 在写入 `result_` 之后调用。
   - `await_resume()`：返回 `result_`。
3. 写一个协程 `compute_with_callback(int x, int y)`，返回 `lazy_task<int>`：
   ```cpp
   AsyncAddAwaiter awaiter{x, y};
   int sum = co_await awaiter;
   co_return sum * 3;
   ```
4. 在 main 中用 `sync_wait` 取走结果并打印。
5. 在 `await_suspend`、回调 lambda、`await_resume`、以及协程体内都打印线程 ID，验证执行流的跨线程转移。
6. 画一张交互图：协程挂起 -> `await_suspend` 调用 `async_add` -> 新线程中 `sleep` -> 回调中 `h.resume()` -> 协程恢复 -> `await_resume` 返回 `result`。

### 进阶任务

- 给 `AsyncAddAwaiter` 加上错误处理：模拟回调 API 有时会失败（比如 20% 概率回调一个错误码而不是结果）。在 awaiter 内部把错误码转换为 `std::exception_ptr`，在 `await_resume` 中检查并 `std::rethrow_exception`。验证异常能自然传播到协程体的 `co_await` 点。
- 不是写独立 awaiter 类，而是实现一个通用模板 `callback_awaiter<AsyncFunc, Args...>`，它接受一个异步函数对象和参数，自动完成 awaiter 三方法的包装。这是"一次适配、处处 co_await"的工程实践。
- 替换模拟 API 为真实的异步回调 API——例如 Boost.Asio 的 `async_read` 或 Windows 的 `ReadFileEx`，将它们的完成回调接到 awaiter 中。尽量让外部依赖最小化，仅做概念验证。
- 给 awaiter 加上 `std::stop_token` 支持：在 `await_suspend` 中注册一个 stop_callback，当取消被请求时，尝试取消底层异步操作（如果它支持取消的话）。

### 验收点

- 你能在日志中看到明确的跨线程执行：`await_suspend` 在线程 A，回调 lambda 在线程 B，协程恢复后的代码也在线程 B（因为 `h.resume()` 是在线程 B 中调用的）。
- 你能解释为什么 `await_ready` 在本场景返回 `false`——因为异步操作还没完成，协程必须挂起等待。
- 你能画出一张完整的 awaiter 三方法 + 异步操作 + 回调 + resume 的时序图。
- 你能说清楚回调中调用 `h.resume()` 和 awaiter 中 `result_` 写入之间的顺序保证。

### 观察点

- 回调式 API 适配为 awaiter 的模式非常固定："在 `await_suspend` 中保存 handle，启动异步操作，在回调中填充结果并 resume"。一旦你理解了这个模式，几乎所有回调 API 都可以用同样的方式拉进协程。
- 这个模式下，awaiter 对象本身在挂起点一直存活（它通常分配在协程帧中），直到 `await_resume` 返回后才析构。所以回调中可以安全地用 `this` 指针引用 awaiter 的成员。
- 跨线程 resume 意味着协程的后续代码可能跑在与调用 `co_await` 时不同的线程上——这就是为什么协程不是线程亲和（thread-affine）的，除非显式地通过 scheduler/executor 约束执行上下文。
- 如果你在进阶任务中实现了 `callback_awaiter` 模板，你会注意到：不同类型的回调 API（不同的参数个数、不同的回调签名）需要不同的模板特化或重载——C++ 的模板系统此时展现了强大的适配能力。

### 常见坑

- 回调中调用了 `h.resume()` 但没有确保 `result_` 已经写好了（比如先 resume 再写 result），导致协程恢复后在 `await_resume` 中读到旧值或未初始化的内存。正确处理：**先写 result_，后 resume**。
- 使用 `detach()` 的线程在 awaiter 被提前销毁时仍然持有 `this` 指针，造成 use-after-free。这个问题在本练习的简单场景下不会出现（因为 awaiter 在协程帧中，协程帧在 resume 之前不会被销毁），但在后续自己写通用 awaiter 模板时要特别警惕。
- 在 `await_suspend` 中捕获了局部变量的引用而不是拷贝——比如 `[&a, &b]` 而不是 `[=]`，导致异步操作启动后引用悬挂。
- 在 `await_ready` 中错误地返回了 `true`，导致 `await_suspend` 不会被调用，异步操作从未启动，`await_resume` 中读取到未初始化的值。
- `async_add` 的回调在 `await_suspend` 返回前就执行了（如果底层 API 支持同步完成），导致 `h.resume()` 在一个还没完全挂起的协程上被调用——UB。这在 C++20 协程规范中是受保护的（`resume()` 在协程尚未挂起时的行为是实现定义的），但在实践中最安全的做法是在 `await_suspend` 返回前不要调用 `h.resume()`。如果 API 可能同步完成，在 `await_ready` 中检查并直接返回 `true`。

### 提示

- A-3 中的 `future_awaiter` 是本练习的前置——确保你已经理解了 awaiter 三方法，再进入本题。
- 回调可以是 lambda，也可以是函数指针、`std::function`、或者任何可调用对象。先用 lambda 做，跑通后再尝试泛化。
- 如果你对线程安全不确定，记住一条简单规则：在 `h.resume()` 之前，所有要传给协程的数据（通过 `result_` 等字段）都必须已经写完。`h.resume()` 本身充当了内存屏障的角色。
- 如果你使用 `std::jthread` 代替 `std::thread` + `detach()`，代码会更简洁安全：`std::jthread t([=]{ h.resume(); });` 不需要手动 detach。但要注意 jthread 的析构会在主线程中 join，注意不要形成死锁。

### 复盘问题

- 为什么说"awaiter 是协程和任何异步世界之间的适配器"？从本题看，awaiter 做了哪几件适配工作？
- 如果回调 API 支持错误和取消两种 completion 路径，你的 awaiter 应该如何在 `await_resume` 中区分这三种情况？
- 协程帧在这个过程中充当了"保存现场"的角色——协程挂起时，所有局部变量和执行位置都被保存。回调 API 适配的关键就是把"恢复现场"的动作（`h.resume()`）接到异步操作完成的通知上。这个描述准确吗？
- 为什么真实工程中，这种适配模式会被封装成库（如 Asio 的 `awaitable<T>`、Folly 的 `Future<T>::to_task()`）而不是每个回调都手写一次？

### 对应官方参考

- Lewis Baker 协程系列第 1 篇：awaiter 协议与回调适配
- Raymond Chen 协程系列第 7 篇：把 Windows 回调 API 适配为 awaiter
- Asio 文档：`awaitable<T>` 与回调包装
- Folly：`folly/experimental/coro/` 中 `to_task()` 和 `to_future()` 的实现

---

## 做完模块 B 之后，你现在应该能说清楚什么

至少把下面几句话说顺：

- 递归 generator 的每一层子树对应一个独立的协程帧。P2502R2 的 `co_yield std::ranges::elements_of(inner_gen)` 触发 symmetric transfer——子帧完成后编译器直接跳回父帧的恢复点，不通过普通函数调用链返回，因此栈不会线性增长。普通的 `for (int v : inner_gen) co_yield v;` 不触发 symmetric transfer，深度退化树仍爆栈。
- task 的 `co_await` 链让顺序异步组合读起来像同步代码。值沿 `co_await` 的返回值流动（不是全局共享状态），异常沿 `co_await` 的异常路径自动传播。
- 回调 API 适配为 awaiter 的模式是固定的：`await_suspend` 中保存 `coroutine_handle`，启动异步操作，在回调中填充结果并 resume。这个模式让你可以把任意老式回调 API 拉进协程的世界。
- generator、task、awaiter 这三个概念构成了协程日常使用的完整工具箱：generator 负责惰性数据生产，task 负责异步计算表达，awaiter 负责桥接外部异步世界。
