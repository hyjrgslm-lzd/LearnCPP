# 05 模块 D：future 与异步任务

## 模块目标

模块 A 到 C 让你掌握了“起线程、保护共享数据、等条件成立”这三件事。但你会发现一个反复出现的笨重模式：想从一个线程拿回另一个线程算出的结果，你得自己造一套“共享变量 + mutex + condition_variable + 一个 ready 标志”的三件套，还得小心处理“算的过程中抛了异常怎么把异常带回来”。这套样板代码每次都要手写，既繁琐又容易出错。

本模块给你标准库现成的答案——**future（未来值）模型**：把“一个还没算出来、将来某个时刻才会就绪的值”抽象成一个对象。生产方通过 `std::promise`（承诺）往一条**一次性通道**里塞结果或异常，消费方握着对应的 `std::future`（未来值）在需要时 `get()` 取出；如果还没就绪就阻塞等待，就绪后直接拿到值——异常也会被原样重新抛出。在此之上，`std::async`（异步）让你连线程都不用手动起，一行就能把一个可调用对象（callable）跑到“别处”并拿回 future；`std::packaged_task`（打包任务）则把“可调用对象 + 它的 future”打包成一个可投递的单元，是线程池的天然积木。

这套模型把 C-模块里“等条件 + 取数据 + 传异常”的手工活全部收拢进了几个类型里。但它也有两个必须讲准的著名陷阱：**`std::async` 返回的 future 在析构时会阻塞**（与 `launch::async` 关联时），以及 **launch 策略默认值的不确定性**——不理解这两点，写出来的“并行”代码可能悄悄退化成串行。本模块就是要把 future 模型的能力边界和这些坑一次性讲透。

## 模块完成标准

做完本模块，你至少要能稳定说清楚：

- `std::promise` / `std::future` 构成的**一次性通道**：`set_value` / `set_exception` 如何把结果或异常送过去，`future.get()` 如何取出（含异常重新抛出），以及为什么 `future` 只能 `get()` 一次。
- `std::shared_future`（共享未来值）与 `std::future` 的区别：为什么需要它、它如何让**多个消费者**等同一个结果并各自多次 `get()`。
- `std::async` 的三种启动策略：`launch::async`（保证新线程、立即开跑）、`launch::deferred`（惰性、`get()` 时才在调用线程同步执行）、默认 `async|deferred`（实现自选，可能两者皆可），以及如何观察它们在“执行线程”和“执行时机”上的差异。
- **`std::async` future 析构阻塞**这一著名陷阱：与 `launch::async` 关联的 future 在析构时会阻塞直到任务结束；不保存返回的 future（写成临时量）会让本想并行的多个 `async` 调用退化成串行。
- `std::packaged_task` 的定位：它如何包装一个可调用对象、暴露一个 `get_future()`，以及如何被投递到线程或任务队列里执行——它与 `promise`、`async` 三者的分工关系。

---

## 练习 D-1：promise / future 基础

> 代码目录：`exercises/D1_promise_future/`

### 目标

用 `std::promise` / `std::future` 实现一条**跨线程的一次性通道（one-shot channel）**：worker 线程把计算结果用 `set_value` 送回主线程，主线程用 `future.get()` 取出。再把“传值”扩展到“传异常”——worker 出错时用 `set_exception` 把异常送过通道，`get()` 时异常会在主线程被原样重新抛出。最后用 `std::shared_future` 让**多个消费者**等同一个结果。

### 前置理解

- future 模型来自 `<future>`，是 C++11 引入的。它把“一个将来才就绪的值”抽象成对象，替代你在模块 C 里手写的“mutex + condition_variable + ready 标志 + 数据 + 异常字段”这一整套样板。
- 一对 `std::promise<T>` 与 `std::future<T>` 通过一个共享状态（shared state）相连：promise 是**写端（生产方）**，future 是**读端（消费方）**。你用 `promise.get_future()` 取出与之配对的那个 future。
- 关键概念：
  - **一次性（one-shot）**：这条通道只能被写一次。对同一个 promise 调用两次 `set_value`（或 `set_value` 后再 `set_exception`）会抛 `std::future_error`（错误码 `promise_already_satisfied`）。
  - **`get()` 只能调用一次**：`std::future::get()` 会把值**移出**共享状态，调用后 future 变为无效（`valid() == false`），再次 `get()` 是未定义行为/抛异常。这是 `future` 与 `shared_future` 最关键的区别。
  - **传异常**：`promise.set_exception(std::make_exception_ptr(e))` 把异常存入通道；消费方 `future.get()` 时该异常被重新抛出。这让“跨线程传异常”变得和返回值一样自然——你不用再手动塞一个 `std::exception_ptr` 字段。
  - **promise 析构而未设值**：若 promise 在没 `set_value`/`set_exception` 的情况下被销毁，共享状态会被置成一个“坏掉”的状态——future.get() 会抛 `std::future_error`（`broken_promise`）。这是“生产方意外退出”的安全网。
  - **`std::shared_future`**：由 `future.share()` 得到（或 `promise.get_future().share()`）。它**可被拷贝**、可被**多个线程各自 `get()` 多次**，每次 `get()` 都返回同一个结果的引用/拷贝。普通 `future` 不可拷贝、`get()` 一次即失效。

### 必做任务

1. **promise 传值**（对应 `// TODO [必做 1]`）：主线程创建 `std::promise<int>`，取出 `future`；起一个 worker 线程，把 promise **move** 进去，worker 算出结果后 `set_value`。主线程 `future.get()` 阻塞到结果就绪并取出。观察：`get()` 之后这个 future 就失效了。

2. **promise 传异常**（对应 `// TODO [必做 2]`）：再做一遍，但这次 worker 在计算中“出错”——用 `set_exception(std::make_exception_ptr(std::runtime_error{...}))` 把异常送过通道。主线程在 `try { fut.get(); } catch (...)` 里捕获到这个异常，证明异常被跨线程原样传回。

3. **shared_future 多消费者**（对应 `// TODO [必做 3]`）：把一个 `future` 用 `.share()` 转成 `std::shared_future`，拷贝给**多个**等待线程；主线程（生产方）置值后，每个等待线程各自 `get()` 都拿到同一个结果。观察：`shared_future` 可拷贝、可多次 `get()`，而普通 `future` 做不到。

4. 记录并回答：
   - 为什么 `std::future` 的 `get()` 只能调用一次？它和 `shared_future` 在“值的所有权”上有什么本质差异？
   - `set_exception` 传过去的异常，是在 `set_exception` 时抛、还是在 `get()` 时抛？
   - 如果 worker 线程在 `set_value` 之前就异常退出、promise 被析构，主线程 `get()` 会发生什么？

### 进阶任务

- **broken_promise 安全网**（对应 `// TODO [进阶 1]`）：故意让一个 promise 不设值就离开作用域（销毁），观察 future.get() 抛出 `std::future_error`，错误码为 `broken_promise`。理解这是“生产方没履约”的兜底信号。
- 用 `future.wait_for(0ms)` 检查 `std::future_status`（`ready` / `timeout` / `deferred`），实现“非阻塞地查询是否就绪”，对比直接 `get()` 的阻塞语义。
- 思考：`promise<void>` 有什么用？（答案：当你只需要传递“事件发生了/异常发生了”这一信号、不需要携带数据时——它就是一个可跨线程的一次性事件 + 异常通道。）

### 验收点

- 你能用 promise/future 在两个线程间传一个值，并解释 `get()` 的阻塞与“取一次即失效”语义。
- 你能用 `set_exception` 把异常跨线程传回，并在消费方 `get()` 处捕获到它。
- 你能用 `shared_future` 让多个消费者拿到同一结果，并说清它与 `future` 的所有权差异。
- 你能说出 `broken_promise` 在什么情况下发生、它保护了什么。

### 观察点

- `future.get()` 在结果未就绪时阻塞——这背后正是标准库替你实现的“等条件成立”，省掉了你手写的 condition_variable。
- 异常不是在 `set_exception` 那一刻抛的，而是被存进通道、推迟到消费方 `get()` 时才抛出——这与返回值“在 get 时才取出”完全对称。
- `promise` 与 `future` 都是**只移动（move-only）**类型：promise 要 move 进 worker，future 留在主线程。`shared_future` 则是可拷贝的。
- `share()` 之后，原 `future` 失效（valid() 变 false）；值的所有权转交给了那些 `shared_future` 副本共享。

### 常见坑

- **对同一 promise 设值两次**：抛 `promise_already_satisfied`。一次性通道只能写一次。
- **对已 `get()` 过的 future 再次 `get()`**：普通 `future` 取一次即失效，再取是错误。需要多次取/多消费者，请用 `shared_future`。
- **忘记把 promise move 进线程**：promise 不可拷贝；按值捕获到 lambda 里会编译失败，必须 `std::move`。
- **以为 set_exception 会立刻抛**：它只是把异常**存起来**，真正抛出发生在消费方 `get()`。
- **promise 设值前线程崩了/提前 return 没设值**：future 端会收到 `broken_promise`，别把它和业务异常混为一谈。

### 提示

- worker 的签名用 `void worker(std::promise<int> p)` 按值接收（move 进去）；启动用 `std::thread t(worker, std::move(p));`。
- 传异常：`p.set_exception(std::make_exception_ptr(std::runtime_error{"boom"}));`。
- 多消费者：`std::shared_future<int> sf = fut.share();` 后把 `sf` **按值拷贝**进每个等待线程的 lambda。
- 用本仓库的 `cs::logf(...)`（`concurrency_study/log.hpp`）打印带时间戳和线程 id 的日志，观察“谁在等、谁先就绪”。

### 复盘问题

- `future` 与 `shared_future` 在“能否拷贝、能 get 几次、值如何被取走”这三点上分别是怎样的？
- `set_value` 与 `set_exception` 写的是同一条通道吗？对同一个 promise 两者能否各调一次？
- `broken_promise` 是什么时候、由谁触发的？它和 worker 主动 `set_exception` 传回的业务异常有何不同？
- 如果你要在多个线程间广播“同一个计算结果”，你会选 `future` 还是 `shared_future`？为什么？

### 对应官方参考

- cppreference [`std::promise`](https://en.cppreference.com/w/cpp/thread/promise)
- cppreference [`std::future`](https://en.cppreference.com/w/cpp/thread/future)
- cppreference [`std::shared_future`](https://en.cppreference.com/w/cpp/thread/shared_future)
- cppreference [`std::future_error`](https://en.cppreference.com/w/cpp/thread/future_error) / [`std::future_errc`](https://en.cppreference.com/w/cpp/thread/future_errc)
- 《C++ Concurrency in Action, 2nd ed.》(Anthony Williams) 第 4 章 4.2

---

## 练习 D-2：std::async 启动策略

> 代码目录：`exercises/D2_async_policies/`

### 目标

用 `std::async` 把一个可调用对象跑到“别处”并拿回 `future`，亲手对比它的三种启动策略——`std::launch::async`、`std::launch::deferred`、以及默认（二者皆可）——在**执行线程**和**执行时机**上的差异。然后正面演示并解释 future 模型最著名的两个陷阱：**`std::async` 返回的 future 析构会阻塞**，以及由此导致的“临时 future 未保存 → 看似并行实则串行”。

### 前置理解

- `std::async`（`<future>`，C++11）接受一个启动策略 + 一个可调用对象（及其参数），返回一个 `std::future<返回类型>`。你不用手动 `std::thread` + `promise`，它把这些都封装好了。
- 三种启动策略：
  - **`std::launch::async`**：**保证**在一个新线程上执行该任务，且**立即**开始（不等你 `get()`）。
  - **`std::launch::deferred`**：**惰性求值（lazy）**。任务不立即跑；直到你对返回的 future 调用 `get()`（或 `wait()`）时，才在**调用 `get()` 的那个线程**上**同步**执行——本质是“延迟到的函数调用”，根本没有新线程。
  - **默认（不传策略，等价于 `async | deferred`）**：由实现自行选择上面两者之一。这意味着任务**可能**在新线程异步跑，也**可能**被推迟到 `get()` 时同步跑——你不能假设它一定是异步的。
- 关键概念：
  - **`std::async` future 析构阻塞陷阱**：当 future 与一个 `launch::async` 任务关联时，**该 future 的析构函数会阻塞，直到任务执行完毕**（仿佛隐式调用了 `wait()`）。这是标准明文规定的特例（只对 `async` 返回的、且与 async 策略关联的 future 成立）。
  - **临时 future → 串行化**：如果你写 `std::async(launch::async, task);` 而**不保存**返回值，那个 future 是个**临时量**，会在**当前完整表达式结束（分号处）立即析构**——于是析构阻塞当场生效，等到任务跑完才继续下一行。多个这样的调用并排写，就会一个接一个地串行执行，完全失去并行意义。**正确做法是把每个 future 存进变量（如 `std::vector<std::future<T>>`），让它们的析构推迟到一起。**
  - **deferred 的可观测特征**：对 deferred 任务，`future.wait_for(0s)` 返回 `std::future_status::deferred`；任务直到 `get()` 才在调用线程上跑（可通过打印线程 id 验证：它和主线程 id 相同）。

### 必做任务

1. **三种策略对比**（对应 `// TODO [必做 1]`）：写一个会打印自己所在线程 id 的任务函数。分别用 `launch::async`、`launch::deferred`、默认策略各调一次 `std::async`，观察并记录：
   - `async`：任务在**别的线程**上、且在你 `get()` 之前就已经开跑（可在主线程 sleep 一会儿后看到它已打印）。
   - `deferred`：在你 `get()` 之前任务**完全没动**；`get()` 时才在**主线程**上同步执行（线程 id 与主线程相同）；`wait_for(0s)` 返回 `deferred`。
   - 默认：打印实际落到了哪种行为（不同实现/负载下可能不同）。

2. **future 析构阻塞 → 串行化陷阱**（对应 `// TODO [必做 2]`）：
   - **反面**：连续写若干条 `std::async(launch::async, slow_task, i);`（**不保存**返回的 future）。每条语句的临时 future 在分号处析构、阻塞到该任务跑完，于是总耗时 ≈ N × 单任务耗时（**串行**）。用计时打印证明它没并行。
   - **正面**：把 N 个 `std::async(launch::async, slow_task, i)` 的返回值**存进 `std::vector<std::future<...>>`**，先全部发起（此时它们真并行跑着），再统一 `get()`。总耗时 ≈ 单任务耗时（**并行**）。对比两者的墙钟时间，亲眼看到“保存 future”是并行的前提。

3. 记录并回答：
   - `launch::deferred` 的任务在哪个线程、什么时刻执行？它和直接调用这个函数有什么区别？
   - 为什么“不保存 `async` 返回的 future”会导致串行？是哪个对象的哪个行为造成的？
   - 默认策略为什么“不可假设它是异步的”？这在实践中可能埋下什么 bug？

### 进阶任务

- **deferred 的惰性可观测**（对应 `// TODO [进阶 1]`）：对一个 `launch::deferred` 的 future，先 `wait_for(0s)` 看到 `std::future_status::deferred`，sleep 一段时间后再查仍是 `deferred`（证明它真没跑），最后 `get()` 触发执行，打印执行线程 id 验证 == 主线程。
- 给 `slow_task` 抛异常，观察 `std::async` 返回的 future 在 `get()` 时把异常重新抛出（与 D-1 的 promise 传异常同源——`async` 内部就是用 promise 实现的）。
- 思考：既然默认策略不可靠，实践中你应该**显式写出 `std::launch::async`**还是 `deferred`？什么场景下 deferred 反而更合适（提示：纯惰性计算、可能根本用不到的结果）？

### 验收点

- 你能用三种策略各跑一次并解释清楚“线程”和“时机”两个维度的差异。
- 你能用计时数据证明“不保存 future → 串行”“保存 future → 并行”，并指出根因是 future 析构阻塞。
- 你能说清 `launch::deferred` 是在 `get()` 调用线程上同步执行的惰性求值。
- 你能解释默认策略的不确定性以及它带来的实践风险。

### 观察点

- `launch::async` 的任务在 `get()` 之前就已经在别的线程上跑了——`async` 返回那一刻线程就启动了。
- `launch::deferred` 的任务在 `get()` 之前一动不动；`get()` 时在**当前线程**同步跑完才返回——它根本不是并发，只是“延后的同步调用”。
- 反面写法里，每条 `std::async(...)` 语句末尾的分号就是临时 future 的析构点；析构阻塞让“下一条”必须等“上一条”跑完——这就是串行的来源。
- 正面写法里，所有 future 存活到 vector 一起销毁（或被 `get()`）之前，N 个任务因此能真正同时在跑。

### 常见坑

- **不保存 `async(launch::async, ...)` 的返回值**：临时 future 当场析构阻塞，并行退化成串行。这是本题的头号陷阱。
- **依赖默认策略获得并行**：默认是 `async|deferred`，实现可能选 deferred，于是你的“后台任务”其实在 `get()` 时才在前台线程跑——后台不后台全看运气。要并行就**显式写 `std::launch::async`**。
- **把 deferred 当并发**：deferred 没有新线程；在它上面“等它在后台完成”是空等，它在你 `get()` 前永远不动。
- **忽略 async 的 future 的特殊析构语义**：只有 `std::async` 返回的、且与 async 策略关联的 future 才有“析构即 wait”这一特例；普通 promise/future、packaged_task 的 future 析构**不**阻塞。别张冠李戴。

### 提示

- 任务里用 `cs::logf("task ", i, " on thread ", std::this_thread::get_id())` 打印执行线程，最直观地区分 async / deferred。
- 计时用 `std::chrono::steady_clock`：记录发起前、`get()` 完成后的时间差。`slow_task` 里 `std::this_thread::sleep_for(200ms)` 模拟耗时。
- 保存 future：`std::vector<std::future<int>> futs; futs.push_back(std::async(std::launch::async, slow_task, i));`。
- 查 deferred：`if (fut.wait_for(std::chrono::seconds(0)) == std::future_status::deferred) ...`。

### 复盘问题

- 三种策略在“是否新线程”“何时开始执行”两个维度上各是什么？默认策略为什么两者都可能？
- “不保存 future 导致串行”的根因是哪个对象的哪个成员函数行为？把返回值存进 vector 为什么就并行了？
- `launch::deferred` 与“直接调用该函数”有何区别？它存在的意义是什么？
- 如果一段代码需要确定性的后台并行，你会怎么写 `std::async` 调用？为什么不能省略策略参数？

### 对应官方参考

- cppreference [`std::async`](https://en.cppreference.com/w/cpp/thread/async)
- cppreference [`std::launch`](https://en.cppreference.com/w/cpp/thread/launch)
- cppreference [`std::future::wait_for`](https://en.cppreference.com/w/cpp/thread/future/wait_for) / [`std::future_status`](https://en.cppreference.com/w/cpp/thread/future_status)
- 《C++ Concurrency in Action, 2nd ed.》(Anthony Williams) 第 4 章 4.2.1；另见 Scott Meyers《Effective Modern C++》Item 35–36（async 策略与陷阱）

---

## 练习 D-3：packaged_task 打包任务

> 代码目录：`exercises/D3_packaged_task/`

### 目标

用 `std::packaged_task` 把一个可调用对象**打包**成“一个可被投递、执行后自动把结果写进其 future”的任务单元；用 `get_future()` 取出它的 future；把若干 packaged_task 放进一个**任务队列**，由 **worker 线程**取出并执行，主线程统一收集各自的 future 结果。借此理清 `packaged_task`、`promise`、`async` 三者的分工，并为后续 Capstone1（线程池）建立“任务 = packaged_task”的接口直觉。

### 前置理解

- `std::packaged_task<R(Args...)>`（`<future>`，C++11）包装一个可调用对象。它本身**也是可调用的**：当你像调用函数一样 `task(args...)` 调用它时，内部的可调用对象被执行，返回值（或抛出的异常）被**自动写入**它持有的共享状态——你事先用 `task.get_future()` 取出的那个 future 随即就绪。
- 三者的分工，一句话区分：
  - **`std::promise`**：你**手动**往通道里 `set_value` / `set_exception`——最底层、最灵活，结果从哪来由你定。
  - **`std::packaged_task`**：把“调用一个函数 → 自动把其返回值/异常写进 future”这件事打包好——你只管在合适的线程上**调用**它。
  - **`std::async`**：连“在哪调用、要不要起线程”都帮你决定了——最高层、最省事，但策略受限（见 D-2）。
  - 可以这样理解递进：`async` ≈ `packaged_task` + 自动起线程/调度；`packaged_task` ≈ `promise` + 自动 set。
- 关键概念：
  - **`packaged_task` 是 move-only**：不可拷贝（它持有共享状态与可调用对象）。放进 `std::queue`、跨线程传递都要 `std::move`。
  - **类型擦除（type erasure）需求**：不同签名的 `packaged_task<R(Args...)>` 是不同类型。要把“任意无参、返回 int 的任务”放进同一个队列，最简单的办法是统一成 `std::packaged_task<int()>`（或更通用地用 `std::function<void()>` 包一层——这正是线程池常见做法）。
  - **任务与执行解耦**：`get_future()` 在**投递前**就能拿到（future 与 task 同时诞生）；任务在哪个线程、何时被 `task()` 调用，与“谁持有 future”完全解耦。这就是线程池的核心抽象——生产者造任务拿 future，消费者（worker）取任务执行。

### 必做任务

1. **打包并取 future**（对应 `// TODO [必做 1]`）：创建一个 `std::packaged_task<int()>`（包装一个返回 int 的 lambda），用 `get_future()` 取出 future；在**另一个线程**上 `std::move` 这个 task 过去并调用它（`task()`）；主线程 `future.get()` 拿到结果。观察：你从没手动 `set_value`，结果是 task 被调用时自动写进去的。

2. **任务队列 + worker 线程**（对应 `// TODO [必做 2]`）：
   - 用一个线程安全队列（可复用模块 C 的有界/无界阻塞队列思路，或这里用 `std::queue<std::packaged_task<int()>>` + mutex + condition_variable 现搭一个最小版）装 packaged_task。
   - 主线程造 N 个 `packaged_task<int()>`，对每个**先 `get_future()` 存进 `std::vector<std::future<int>>`**，再 `std::move` 进队列。
   - 起一个（或多个）worker 线程：从队列取出 task、调用它（结果自动入各自 future）、循环直到收到“关闭”信号。
   - 主线程对收集到的 N 个 future 逐个 `get()`，验证拿到全部结果（顺序与投递一致）。

3. **传异常也走同一条路**（对应 `// TODO [必做 3]`）：让其中一个任务抛异常，验证它被 worker 调用 `task()` 时捕获进 future，主线程 `get()` 该 future 时异常被重新抛出——与 D-1 的 promise 传异常、D-2 的 async 传异常完全一致（因为底层都是同一套共享状态机制）。

4. 记录并回答：
   - `packaged_task` 的结果是“谁、在什么时刻”写进 future 的？你需要手动 `set_value` 吗？
   - 为什么把 N 个 future 在**投递前**就先存起来？（提示：task 一旦 move 进队列你就够不着它了，但 future 早已在手。）
   - `packaged_task` 和 `std::async(launch::async, ...)` 相比，多做了/少做了什么？

### 进阶任务

- **线程池接口直觉**（对应 `// TODO [进阶 1]`，为 Capstone1 埋点）：把上面的“队列 + worker”收口成一个最小 `submit` 接口的雏形：
  ```cpp
  template <class F>
  std::future<std::invoke_result_t<F>> submit(F f) {
      using R = std::invoke_result_t<F>;
      std::packaged_task<R()> task(std::move(f));
      std::future<R> fut = task.get_future();
      enqueue(std::packaged_task<void()>([t = std::move(task)]() mutable { t(); }));
      return fut;
  }
  ```
  体会：调用方拿到一个 `future<R>`，完全不知道任务在哪个 worker 上跑——这正是 Capstone1 线程池 `submit` 的核心形状。用 `std::function<void()>` / `std::packaged_task<void()>` 做类型擦除，把“任意返回类型的任务”统一塞进一个队列。
- 起多个 worker，验证任务被并行分摊（打印各 task 落在哪个线程 id 上）。
- 思考：为什么线程池里几乎总用 `packaged_task` 而不是裸 `promise`？（提示：你只想“把函数扔过去执行”，不想手写 set_value/set_exception 与异常捕获——packaged_task 把这套全包了。）

### 验收点

- 你能打包一个可调用对象、在另一个线程上调用它，并通过 future 拿到结果（无需手动 set_value）。
- 你能把多个 packaged_task 投进队列、由 worker 执行、主线程统一收集 futures。
- 你能让任务抛异常并在 future.get() 处捕获，验证异常走的是同一条共享状态通道。
- 你能用一两句话讲清 promise / packaged_task / async 的递进分工。

### 观察点

- `get_future()` 必须在 task 被 move 走之前调用并保存——future 与 task 同生，但 task 会离开你的手，future 留下。
- worker 调用 `task()` 的那一刻，对应 future 就绪；主线程若已在 `get()` 上阻塞，会被立即放行。
- 异常路径与正常返回路径用的是同一套机制：`task()` 内部捕获异常并 `set_exception`，你在 `get()` 处接住。
- 多 worker 下，任务落在哪个线程是调度决定的；future 这一侧完全不关心这件事——这就是解耦。

### 常见坑

- **忘了 `get_future()` 就 move 走 task**：task 进队列后你再也拿不到它的 future，结果取不回来。永远**先 get_future 再投递**。
- **试图拷贝 packaged_task**：它 move-only。放进容器、传进 lambda 都要 `std::move`，且持有它的 lambda 要标 `mutable`（因为调用 `task()` 会改其状态）。
- **同一个 task 调用两次**：共享状态只能就绪一次，第二次 `task()` 抛 `promise_already_satisfied`。
- **队列里混放不同签名的 packaged_task**：类型不同没法放同一容器。统一成 `packaged_task<void()>` 或 `std::function<void()>` 做类型擦除。
- **worker 退出条件没设计好**：用模块 C 学的 `close()` / `closed_` 标志或哨兵任务（poison pill）让 worker 在队列排空后优雅退出，别让它空转或卡死。

### 提示

- 打包：`std::packaged_task<int()> task([]{ return 42; }); auto fut = task.get_future();`。
- 跨线程调用：`std::thread t(std::move(task)); t.join();`（`packaged_task` 本身可作为线程入口被调用），或把它 move 进队列由 worker `task()`。
- 队列最小版：`std::queue<std::packaged_task<void()>>` + `std::mutex` + `std::condition_variable` + `bool closed_`，push/pop 套用模块 C 的谓词 wait。
- 类型擦除统一签名：把 `packaged_task<int()>` 包进 `packaged_task<void()>` 或 `std::function<void()>`，队列只存这一种类型。

### 复盘问题

- `packaged_task` 相比 `promise` 自动化了什么？相比 `async` 又把什么决定权交还给了你？
- 为什么必须“先 `get_future()` 再把 task 投递出去”？反过来会怎样？
- 在一个线程池里，“任务”为什么天然适合用 `packaged_task` 表达？它和你即将在 Capstone1 写的 `submit()` 返回 future 是同一回事吗？
- 异常在 packaged_task 路径上是怎么从 worker 线程传回主线程的？这和 D-1、D-2 是同一套机制吗？

### 对应官方参考

- cppreference [`std::packaged_task`](https://en.cppreference.com/w/cpp/thread/packaged_task)
- cppreference [`std::packaged_task::get_future`](https://en.cppreference.com/w/cpp/thread/packaged_task/get_future)
- cppreference [`std::packaged_task::operator()`](https://en.cppreference.com/w/cpp/thread/packaged_task/operator())
- 《C++ Concurrency in Action, 2nd ed.》(Anthony Williams) 第 4 章 4.2.2；第 9 章（线程池里 packaged_task 的应用）

---

## 做完模块 D 之后，你现在应该能说清楚什么

至少把下面几句话说顺：

- future 模型把“一个将来才就绪的值（含异常）”抽象成对象，替代你在模块 C 手写的“mutex + condition_variable + ready 标志 + 数据/异常字段”样板。
- `std::promise`（写端）与 `std::future`（读端）构成一次性通道：`set_value`/`set_exception` 写入，`get()` 取出且只能取一次，异常被推迟到 `get()` 重新抛出；生产方不设值就析构会触发 `broken_promise`。
- `std::shared_future` 可拷贝、可被多个消费者各自多次 `get()`，用于把同一结果广播给多个等待者；普通 `future` 取一次即失效。
- `std::async` 的三种策略：`launch::async` 保证新线程立即跑；`launch::deferred` 惰性、`get()` 时才在调用线程同步跑；默认是二者皆可、不可假设异步。**`async` 返回的（async 策略）future 析构会阻塞**，不保存返回值会让多个 async 调用串行——要并行必须存住每个 future 并显式写 `launch::async`。
- `std::packaged_task` 把“调用可调用对象 → 自动写结果/异常进 future”打包成可投递单元，是线程池的天然积木：promise 最底层手动、packaged_task 自动 set、async 连调度都包办；Capstone1 的 `submit()` 返回 future 正是建立在 packaged_task 之上。
