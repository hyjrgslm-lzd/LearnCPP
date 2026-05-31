# 02 模块 A：线程生命周期与 jthread

## 模块目标

这个模块要把「安全地创建、等待、取消一个线程，并正确处理它抛出的异常」刻进你的肌肉记忆。它是阶段一的起点，也是你第一次直面「线程是一种需要被管理生命周期的资源」这件事。

你会先对比 `std::thread`（C++11）与 `std::jthread`（C++20）的生命周期语义，理解为什么忘记 `join` 的 `std::thread` 析构会让整个进程 `std::terminate`，而 `std::jthread` 用 RAII（资源获取即初始化，Resource Acquisition Is Initialization）替你兜底；接着学会 C++20 的协作式取消（cooperative cancellation）三件套 `std::stop_source` / `std::stop_token` / `std::stop_callback`，理解「取消是请求而非强制」；最后学会异常如何跨线程传播——因为线程函数里逃逸的异常**不会**自动跑回主线程，而是直接 `terminate`。

## 模块完成标准

做完本模块，你至少要能稳定说清楚（can-do）：

- 你能说出 `std::thread` 析构时若仍 `joinable` 为什么会调用 `std::terminate()`，并能用 `join()` 或 `detach()` 避免它。
- 你能解释 `std::jthread` 相对 `std::thread` 的两点改进：析构自动 `join`、内置 `stop_token` 支持协作式取消。
- 你能写一个轮询 `std::stop_token::stop_requested()` 的工作循环，并用 `request_stop()` 让它优雅退出。
- 你能说清「取消是协作式的」——没有任何标准 API 能强行杀死一个正在运行的线程。
- 你能用 `std::exception_ptr` + `std::current_exception` / `std::rethrow_exception`，或用 `std::promise::set_exception`，把子线程的异常安全搬运到主线程处理。

## 使用约定

- 本模块默认使用 `std::jthread`，仅在需要对比时才用 `std::thread`。
- `std::jthread` / `std::stop_token` / `std::stop_source` / `std::stop_callback` 均为 **C++20** 特性，分别位于头文件 `<thread>`（jthread）与 `<stop_token>`（取消三件套）。本模块全部可在 MSVC（VS2026，`/std:c++20`）上编译运行。
- 每道题的骨架 `main.cpp` 用 `// TODO [必做 N]:` 与 `// TODO [进阶 N]:` 标记填空点；未填的 TODO 都有最小占位实现保证编译通过，注释里写清真正该做什么并附参考代码。
- 所有日志用随附的 `concurrency_study/log.hpp`：`cs::log("...")` 打印带线程 id 与相对时间戳的一行，`cs::logf(a, b, ...)` 拼接多段；`cs::println` 原样打印（用于表头/分隔线）。

---

## 练习 A-1：jthread 生命周期与自动 join

### 目标

用 `std::thread` 与 `std::jthread` 各起一个 worker，亲手观察 `std::jthread` 离开作用域时**自动 `request_stop()` + `join()`**；并理解「未 `join`/`detach` 的 `std::thread` 析构会调用 `std::terminate()`」这条铁律——用一种不真的终止本进程的方式把它讲清楚。

### 前置理解

- 你知道 `std::thread`（C++11，头文件 `<thread>`）创建后处于「可结合（joinable）」状态。线程对象析构前，你必须对它调用 `join()`（阻塞等它跑完）或 `detach()`（与它脱钩，让它后台自生自灭），二选一。
- 你知道若 `std::thread` 析构时仍 `joinable`（既没 `join` 也没 `detach`），`~thread()` 会调用 `std::terminate()`，整个进程立即 `abort`。这是标准的刻意设计：线程泄漏是程序错误，宁可炸也不静默。
- 你知道 `std::jthread`（C++20，头文件 `<thread>`）是「会自动 join 的 thread」：它的析构函数先 `request_stop()`、再 `join()`。这是典型的 RAII——线程的生命周期被对象的生命周期接管。
- 你接受本题的重点是观察生命周期事件的先后，而非写复杂的 worker 逻辑。

### 必做任务

1. 用 `std::thread` 起一个睡 100ms 的 worker，然后**手动 `join()`** 它（骨架 `// TODO [必做 1]`）。从日志确认主线程的「已 join」一行出现在 worker「干完了」之后。
2. 用 `std::jthread` 起一个同样的 worker，放在一个内层作用域 `{ ... }` 里，**不写任何 join**（骨架 `// TODO [必做 2]`）。从日志时间戳确认：内层作用域结束的那一行，一定在 worker「干完了」之后——证明 `~jthread()` 自动 join 了。
3. 阅读骨架里 `demo_thread_forgot_join_DO_NOT_CALL()` 的注释，用自己的话复述：如果那段代码真的执行，会在哪一行触发 `std::terminate()`，为什么。

### 进阶任务

- 让 jthread 的可调用对象把 `std::stop_token` 作为**第一个形参**（运行期会自动注入该 jthread 内置的 token），把工作循环条件改成 `while (!st.stop_requested())`（骨架 `// TODO [进阶 1]`）。观察：当 jthread 析构调用 `request_stop()` 时，循环自然退出。这是「自动 join」之外，jthread 的另一半价值——「自动请求停止」。
- 故意把一个 `std::jthread` 用 `std::move` 转移给另一个 jthread 变量，观察移动后源对象不再 `joinable`，析构不会重复 join。
- 对照实验：把演示 1 里的 `worker.join()` 注释掉，**在你确认理解后**单独编译一个小程序跑一次，亲眼看到进程因 `std::terminate` 而 abort（务必单独跑，别污染主测试驱动）。

### 验收点

- 你能解释为什么忘记 `join` 的 `std::thread` 析构会 `terminate`，而 `std::jthread` 不会。
- 你能从日志时间戳论证：jthread worker 的「干完了」一定早于其作用域结束那一行。
- 你能说出 `~jthread()` 的两步顺序：先 `request_stop()`，后 `join()`，并解释这个顺序为什么合理（先通知它该停了，再等它停）。

### 观察点

- `std::jthread` 把「线程」变成了一个普通的栈对象——它遵守和 `std::unique_ptr`、`std::lock_guard` 一样的 RAII 直觉：对象活着资源就在，对象析构资源就还。
- 自动 join 意味着 jthread 的析构是**阻塞的**：如果 worker 不响应 `request_stop()`、又跑得很久，析构会卡在 `join()` 上。这正是进阶任务要你把循环改成检查 `stop_requested()` 的原因。
- `detach()` 之后的线程脱离了对象管理，你失去了等待它、知道它结束的能力——这通常是设计上的坏味道，阶段一里几乎用不到。

### 常见坑

- 以为 `std::thread` 析构会「自动 join」——不会，它会 `terminate`。这是 `thread` 与 `jthread` 最大的语义差异。
- 在已经 `join()` 过的 `std::thread` 上再次 `join()`：会抛 `std::system_error`。先用 `joinable()` 判断，或干脆改用 jthread。
- 把 jthread 的 worker 写成不检查 `stop_token` 的死循环，然后疑惑「为什么程序退不出去」——析构卡在 `join()` 上，因为 worker 永远不响应停止请求。
- 误以为 `request_stop()` 会「立刻打断」worker。它只是设了个标志位，worker 得自己去查。

### 提示

- 用 `cs::log` 打印的每行都带相对时间戳（`+Nms`）和线程 id，把它当成你的「时序示波器」——事件先后一目了然。
- 内层作用域用一对 `{ }` 框住 jthread 的声明，是制造「可观察的析构时刻」的最简单办法。
- 想观察自动 `request_stop`，worker 循环里每轮 `sleep_for` 一小段（如 30ms），这样它有机会在两轮之间看到停止标志。

### 复盘问题

- `std::thread` 为什么不像 `std::jthread` 那样默认自动 join？（提示：想想 C++11 时代的设计权衡，以及「隐式阻塞析构」可能带来的意外。）
- `~jthread()` 为什么要先 `request_stop()` 再 `join()`，而不是反过来或只 `join()`？
- 如果一个 worker 永远不检查 `stop_token`，jthread 还能帮到你什么？还剩哪一半价值？
- `detach()` 在什么罕见场景下才是合理的？把一个线程 detach 后，你放弃了哪些能力？

### 对应官方参考

- cppreference：`std::jthread` — https://en.cppreference.com/w/cpp/thread/jthread
- cppreference：`std::thread` — https://en.cppreference.com/w/cpp/thread/thread
- cppreference：`std::thread::~thread`（未 join/detach → `std::terminate`）— https://en.cppreference.com/w/cpp/thread/thread/~thread
- 提案 `P0660R10`：*Stop Token and Joining Thread*（`std::jthread` 与 stop_token 的标准提案）。
- Anthony Williams《C++ Concurrency in Action, 2nd ed.》第 2 章（线程管理）、第 9.2 节（中断线程 / jthread）。

---

## 练习 A-2：stop_token 协作式取消

### 目标

用 C++20 协作式取消（cooperative cancellation）三件套实现一个「可被请求停止」的工作循环：worker 轮询 `std::stop_token::stop_requested()`，主线程调用 `request_stop()` 请求它退出；再用 `std::stop_callback` 在取消发生的瞬间触发回调，记录「取消时刻」。核心是建立这个直觉——**取消是请求，不是强杀**。

### 前置理解

- 你知道协作式取消的三个角色：`std::stop_source` 是「开关」（可调用 `request_stop()`），`std::stop_token` 是开关的「只读视图」（可廉价拷贝、查询 `stop_requested()`），`std::stop_callback` 是「开关按下瞬间触发的铃」。三者都在 `<stop_token>`，均为 C++20。
- 你知道 `std::jthread` 内部自带一个 `stop_source`：可用 `jthread::get_stop_token()` 取到对应 token，或让 worker 把 `std::stop_token` 作为第一个形参，运行期会自动注入。
- 你理解 `request_stop()` 只是把共享的停止状态翻成「已请求」，真正的退出由 worker 在自己的检查点（每轮循环开头）主动响应。没有任何标准 API 能强行终止一个正在运行的线程。
- 你接受本题的 worker 逻辑可以很简单（每轮打印一行 + 短睡），重点在取消的语义而非业务。

### 必做任务

1. 写一个轮询型工作循环：`while (!st.stop_requested()) { ...干一点活...; sleep_for(50ms); }`（骨架 `// TODO [必做 1]`）。
2. 让 worker 先跑一会儿，主线程再调用 `jthread::request_stop()` 请求取消（骨架 `// TODO [必做 2]`）。从日志确认 worker 是「收到取消后退出」，而不是「跑满固定次数退出」。
3. 阅读并理解骨架里「演示 2」——用一个独立的 `std::stop_source` 控制一个普通 `std::thread`，体会取消机制并不绑定 jthread，`stop_token` 可以独立创建、按值拷贝、共享给多个线程。

### 进阶任务

- 在 worker 内注册一个 `std::stop_callback`，绑定到它的 `stop_token` 上，回调体里打印「取消被请求」（骨架 `// TODO [进阶 1]`）。观察回调在 `request_stop()` 后几乎立刻触发。注意触发线程的语义见下方「观察点」。
- 把同一个 `stop_token` 拷贝给两个并发 worker，调用一次 `request_stop()`，确认两个 worker 都退出——理解「一处请求、处处可见」。
- 在 worker 阻塞等待（如 `condition_variable`）时如何配合取消？先记下这个问题，模块 C 的「可中断等待」会专门解决（提示：`std::condition_variable_any` + `stop_token` 重载的 `wait`）。

### 验收点

- 你能说清取消为何是协作式的，以及一个从不检查 `stop_requested()` 的 worker 为什么取消不掉。
- 你能从日志论证 worker 是「被取消退出」而非「跑满次数退出」（必做 1/2 填好后，worker 不再有固定次数上限）。
- 你能说出 `std::stop_callback` 的触发线程语义：注册时若 token 已被请求取消，回调在**注册它的线程**上同步立即执行；否则回调在**调用 `request_stop()` 的那个线程**上执行。

### 观察点

- `stop_source` 与它派生的所有 `stop_token`、`stop_callback` 共享同一份「停止状态」，引用计数管理生命周期——这和 `shared_ptr` 的直觉一致。
- `request_stop()` 是幂等的：多次调用只有第一次真正改变状态并触发回调。
- 协作式取消把「何时安全退出」的决定权交还给 worker——它可以在退出前清理资源、刷新缓冲、记录进度。强杀做不到这些，这正是协作式优于强制的根本原因。

### 常见坑

- worker 循环里干活耗时很长却不在中途检查 `stop_requested()`，导致 `request_stop()` 后还要等很久才退出——检查点的粒度决定取消的响应速度。
- 把 `stop_token` 按引用捕获进一个生命周期更长的 lambda，token 所属对象先析构，造成悬垂——`stop_token` 设计成可廉价拷贝，**按值捕获**最稳。
- 误以为 `stop_callback` 的回调会在 worker 线程里跑——它跑在调用 `request_stop()` 的线程（或注册线程），所以回调里别碰只有 worker 线程能安全访问的状态。
- 在 jthread 上既让它自动 `request_stop`（析构时），又手动 `request_stop`——这没错（幂等），但要清楚两次请求只有一次生效。

### 提示

- 让 worker 每轮 `sleep_for(50ms)`，主线程 `sleep_for(180ms)` 后再 `request_stop()`，这样能稳定看到「跑了三四轮后被取消」的时间线。
- `std::stop_callback cb{st, []{ ... }};` 是一个 RAII 对象——它在构造时注册、析构时注销。把它声明在 worker 函数体内即可。
- 想验证「一处请求处处可见」，把同一个 `token` 按值传给两个线程，一次 `source.request_stop()`，看两边日志同时停。

### 复盘问题

- 为什么 C++ 标准选择「协作式取消」而不是提供一个 `thread::kill()`？强杀一个线程会带来哪些无法收拾的后果？
- `stop_source`、`stop_token`、`stop_callback` 三者的生命周期关系是怎样的？谁持有共享状态？
- 如果 worker 正阻塞在 `mutex::lock` 或一个不支持 stop_token 的 `wait` 上，`request_stop()` 能把它叫醒吗？为什么？（这正是模块 C 要解决的）
- `stop_callback` 的回调在哪个线程执行？如果回调里访问了共享数据，你需要额外的同步吗？

### 对应官方参考

- cppreference：`std::stop_token` — https://en.cppreference.com/w/cpp/thread/stop_token
- cppreference：`std::stop_source` — https://en.cppreference.com/w/cpp/thread/stop_source
- cppreference：`std::stop_callback` — https://en.cppreference.com/w/cpp/thread/stop_callback
- cppreference：`std::jthread::request_stop` — https://en.cppreference.com/w/cpp/thread/jthread/request_stop
- 提案 `P0660R10`：*Stop Token and Joining Thread*。
- Anthony Williams《C++ Concurrency in Action, 2nd ed.》第 9.2 节（中断 / 协作式取消线程）。

---

## 练习 A-3：线程中的异常传播

### 目标

理解「线程函数让异常逃逸顶层 → `std::terminate()`」这条铁律，并掌握两条安全把子线程异常搬运到主线程处理的通道：手动通道 `std::exception_ptr` + `std::current_exception` / `std::rethrow_exception`，以及正式通道 `std::promise::set_exception` + `std::future::get`。

### 前置理解

- 你知道异常**不会自动跨线程传播**：每个线程有自己的调用栈，主线程的 `try/catch` 接不住子线程里抛出的异常。
- 你知道若一个线程的入口函数让异常逃逸到顶层（无人 catch），运行期会调用 `std::terminate()`，整个进程 `abort`。所以子线程里「该 try 的地方一定要 try」。
- 你知道 `std::current_exception()`（`<exception>`）只能在 catch 块内调用，返回一个指向「当前正在处理的异常」的 `std::exception_ptr`；`exception_ptr` 引用计数管理、可安全跨线程传递；`std::rethrow_exception(ptr)` 在任意线程把它重新抛出，于是你能在主线程的 `try/catch` 里按类型接住。
- 你知道 `std::promise<T>` / `std::future<T>`（`<future>`）是一对「一次性的值/异常通道」：worker 持 promise，主线程持 future；`promise::set_exception(eptr)` 之后，主线程 `future.get()` 会把那个异常重新抛出。

### 必做任务

1. worker 在自己的 `try { ... } catch (...) { ... }` 里捕获异常，用 `std::current_exception()` 把它打包进一个共享的 `std::exception_ptr`（骨架 `// TODO [必做 1]`）。
2. 主线程 `join()` worker 之后，检查这个 `exception_ptr`：若非空，用 `std::rethrow_exception` 重抛，并用 `try/catch` 按 `const std::exception&` 接住、打印 `what()`（骨架 `// TODO [必做 2]`）。
3. 阅读骨架里 `demo_uncaught_in_thread_DO_NOT_CALL()` 的注释，复述：为什么主线程的 try/catch 接不住子线程的异常，以及这段代码若真执行会在何处 `terminate`。

### 进阶任务

- 改走 promise/future 通道：worker 在 catch 里 `p.set_exception(std::current_exception())`，主线程 `fut.get()` 用 `try/catch` 接住被重抛的异常（骨架 `// TODO [进阶 1]` 与 `// TODO [进阶 2]`）。体会这条通道把「取结果」和「取错误」统一到了 `get()` 一个接口上。
- 思考并验证：如果 worker 既不 `set_value` 也不 `set_exception` 就让 promise 析构，主线程 `fut.get()` 会发生什么？（提示：会抛 `std::future_error`，错误码 `broken_promise`。）
- 对比 `exception_ptr` 手动通道与 promise/future 通道，写下各自的适用场景：前者轻量、无需 `<future>` 机制，适合你已经在手动管理线程同步时；后者语义统一、适合「异步任务返回一个结果」的场景（模块 D 的主场）。

### 验收点

- 你能解释为什么子线程未捕获的异常会导致 `std::terminate()`，且主线程的 try/catch 对此无效。
- 你能用 `exception_ptr` 把异常从 worker 安全搬运到主线程并按类型重抛处理（必做填好后，主线程日志能打印出原始异常的 `what()`）。
- 你能说出 promise/future 通道相对手动 `exception_ptr` 的优势：取值与取错误走同一个 `get()` 接口，`get()` 自动重抛，调用方代码更统一。

### 观察点

- `join()` 在主线程与 worker 之间建立了 happens-before 关系：worker 里对 `exception_ptr` 的写，在 `join()` 返回后对主线程的读一定可见——所以这个共享的「信箱」不需要额外加锁。
- `std::exception_ptr` 是类型擦除的：它不关心异常的静态类型，`rethrow_exception` 时原始类型被完整还原，你照样能 `catch (const std::runtime_error&)`。
- promise 的「一次性」很关键：`set_value` 与 `set_exception` 二选一、且只能设一次；future 的 `get()` 也只能取一次。

### 常见坑

- 在子线程里不写 try/catch，指望主线程 try 整个 `jthread` 构造——接不住，照样 `terminate`。异常必须在它产生的那个线程里被捕获。
- 在 catch 块**之外**调用 `std::current_exception()`：此时没有「当前异常」，返回空 `exception_ptr`，悄无声息地丢了异常。
- 在 `join()` **之前**读共享的 `exception_ptr`：没有 happens-before 保证，构成数据竞争（UB）。一定要先 join 再读。
- promise 路径里 worker 走了异常分支却忘了 `set_exception`，promise 析构触发 `broken_promise`，主线程 `get()` 抛出的是 `future_error` 而非你的原始异常——错误被掩盖了。

### 提示

- worker 的 catch 用 `catch (...)`（捕获一切），再 `current_exception()` 打包，是最通用的写法——你不必在 worker 端枚举所有异常类型。
- 主线程重抛后用 `catch (const std::exception& e)` 接，调 `e.what()` 拿消息；需要区分类型时再加更具体的 catch 分支。
- 验证 promise 通道时，先让 worker 必然走异常分支（直接 `throw`），确认 `get()` 重抛；再改成正常 `set_value` 路径对照。

### 复盘问题

- 为什么 C++ 选择「线程顶层未捕获异常 → terminate」，而不是把异常默默吞掉或自动传给创建者线程？
- `std::current_exception()` 为什么必须在 catch 块内调用？它在 catch 块外返回什么？
- `join()` 提供的 happens-before 关系，具体保证了哪两个操作之间的可见性？为什么因此读 `exception_ptr` 不用加锁？
- 什么时候你会偏好手动 `exception_ptr` 通道，什么时候偏好 promise/future？各自的「重量」和「语义清晰度」如何权衡？

### 对应官方参考

- cppreference：`std::exception_ptr` — https://en.cppreference.com/w/cpp/error/exception_ptr
- cppreference：`std::current_exception` — https://en.cppreference.com/w/cpp/error/current_exception
- cppreference：`std::rethrow_exception` — https://en.cppreference.com/w/cpp/error/rethrow_exception
- cppreference：`std::promise::set_exception` — https://en.cppreference.com/w/cpp/thread/promise/set_exception
- Anthony Williams《C++ Concurrency in Action, 2nd ed.》第 8.4.1 节（异常与并发）、第 4.2 节（future 传播异常）。

---

## 做完模块 A 之后，你现在应该能说清楚什么

合上文档，用自己的话回答（答不顺的回到对应练习）：

1. **线程是资源**：`std::thread` 析构时若仍 `joinable` 会 `std::terminate`；你必须 `join` 或 `detach`。`std::jthread` 用 RAII 替你兜底——析构先 `request_stop()` 再 `join()`。
2. **jthread 相对 thread 的两点改进**：自动 join（RAII）、内置 `stop_token`（协作式取消）。你能说出 `~jthread()` 的两步顺序及其合理性。
3. **协作式取消**：`stop_source`（开关）/ `stop_token`（只读视图）/ `stop_callback`（按下瞬间的铃）。取消是请求不是强杀；worker 必须主动在检查点响应 `stop_requested()`。你能说清回调的触发线程语义。
4. **异常不跨线程自动传播**：子线程顶层逃逸的异常会 `terminate`。安全搬运的两条路——`exception_ptr` 手动通道（配合 `join` 建立的 happens-before，无需加锁）与 `promise::set_exception` + `future.get()` 正式通道。
5. **happens-before 初体验**：`join()` 在主线程与子线程间建立了顺序与可见性，这是你后续模块反复使用的同步基石——这一模块里它表现为「join 之后读子线程写的 exception_ptr 是安全的」。

这五点是阶段一后续模块（B 互斥与锁、C 条件变量、D future 与异步任务）的地基。下一模块，我们给共享状态加锁。
