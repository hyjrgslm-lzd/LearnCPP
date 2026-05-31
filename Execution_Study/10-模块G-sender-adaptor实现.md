# 10 模块 G：sender adaptor 实现

## 模块目标

前面的模块让你手写过最小 sender、receiver、operation_state，也做过一个最简的 tap adaptor。这个模块要把你推到真正实现一个变换值类型的 adaptor 的层次：

- 亲手实现 `my_then`，掌握 inner receiver 拦截 + 值变换 + completion_signatures 推导的完整模式
- 理解管道语法 `operator|` 和 closure object 的设计思路，把 sender 链写出可读的线性形式
- 用基础组件组合出更高层的 `retry` 算法，体验"从原语到策略"的抽象跃升

如果你不经过这一层，就很容易把标准算法当成"库作者的黑盒魔法"，而不是"可以自己写出来的结构化模式"。

## 前置要求

- 模块 D（自定义 sender/receiver）：你已经能手写最小 sender、receiver、operation_state，理解 `connect -> start -> completion` 链路
- 模块 E（tag_invoke / CPO）：你理解自定义点对象的设计意图，能用 `tag_invoke` 或成员函数方式接入框架
- 模块 F（completion_signatures）：你理解签名声明的类型级合同作用，能手动组装和变换签名

## 模块完成标准

做完本模块，你至少要能稳定说清楚：

- 为什么 sender adaptor 的核心模式是 inner receiver 包装 + channel 拦截 + 签名变换。
- 为什么 `connect` 时要创建新的 receiver 去包装下游 receiver，而不是直接修改值。
- 为什么 completion_signatures 必须在类型级别跟随变换函数的返回类型更新。
- 为什么 pipe 语法需要 closure object 和 `operator|` 配合。
- 为什么更高层的组合器（如 retry）可以从基础原语搭建出来，而不需要侵入框架内部。

## 使用建议

- 先做 G-1，把 `my_then` 跑通，再考虑 pipe 语法。
- G-2 和 G-3 可以按你的兴趣调整顺序，但建议先做 G-2 再做 G-3。
- 本模块代码量会比前面的模块明显增大。如果你发现编译错误很多，先回去重新确认 completion_signatures 是否正确，这是最常见的卡点。
- 画对象图是最有效的调试手段。每写一个新类型，就在图上标出它持有谁、被谁调用。

---

## 练习 G-1：实现 my_then

### 目标

亲手实现一个简化版 `then` adaptor，把 inner receiver 拦截值通道、应用变换函数、推导输出签名的完整模式刻进直觉。这是所有 sender adaptor 的原型结构。

### 前置理解

- 你已经在模块 D 练习 13 中做过 `tap` adaptor，理解 inner receiver 包装的基本形状。
- 你知道 `then` 与 `tap` 的核心区别：`then` 的变换函数 `f` 会改变值的类型，因此 completion_signatures 必须跟着变。
- 你知道 error 和 stopped 通道需要原样透传给下游 receiver。
- 你接受这题先只处理单个 `set_value_t(T)` 签名的简化情况，不追求多参数包展开。

### 必做任务

1. **定义 `my_then_sender<InnerSender, F>`**：
   - 持有两个成员：inner sender 和可调用对象 f
   - 声明 `sender_concept = stdexec::sender_t`
   - 推导 `completion_signatures`：假设 inner sender 的 value 签名是 `set_value_t(T)`，那么输出签名是 `set_value_t(std::invoke_result_t<F, T>)`；error 和 stopped 签名从 inner sender 原样保留
   - 实现 `connect(my_then_sender, Receiver)` 接口

2. **定义 `my_then_receiver<DownstreamReceiver, F>`**：
   - 持有两个成员：下游 receiver 和可调用对象 f
   - 声明 `receiver_concept = stdexec::receiver_t`
   - `set_value(auto&&... values)`：调用 `f(values...)`，将返回值通过 `stdexec::set_value` 转发给下游 receiver
   - `set_error(auto&& err)`：直接调用 `stdexec::set_error(downstream, err)` 透传
   - `set_stopped()`：直接调用 `stdexec::set_stopped(downstream)` 透传
   - `get_env()`：转发到下游 receiver 的 `get_env()`

3. **定义 `my_then_operation_state<InnerSender, DownstreamReceiver, F>`**：
   - 持有一个成员：inner sender 与 `my_then_receiver` 连接后产生的 inner operation_state
   - `start()`：调用 inner operation_state 的 `start()`
   - 注意：inner operation_state 的类型是 `connect_result_t<InnerSender, my_then_receiver<DownstreamReceiver, F>>`

4. **实现工厂函数 `my_then(Sender, F)`**：
   - 返回 `my_then_sender<Sender, F>`

5. **验证正常路径**：
   ```cpp
   auto result = sync_wait(my_then(just(42), [](int x){ return x + 1; }));
   // 期望 result 包含 43
   ```

6. **验证错误路径透传**：
   ```cpp
   auto result = sync_wait(my_then(
       just_error(std::make_exception_ptr(std::runtime_error("oops"))),
       [](int x){ return x + 1; }));
   // 期望错误原样到达 sync_wait
   ```

7. **验证值类型变换**：
   ```cpp
   auto result = sync_wait(my_then(just(42), [](int x){ return std::to_string(x); }));
   // 期望 result 包含 "42"（string 类型）
   ```

8. **画出对象拥有关系树**：
   - `my_then_operation_state` 拥有 `inner_operation_state`
   - `inner_operation_state` 内部包含对 `my_then_receiver` 的拥有
   - `my_then_receiver` 拥有 `downstream_receiver` 和 `f`
   - 标注 `start()` 调用路径和 `set_value` 回调路径

### 进阶任务

- 把 `my_then` 扩展为支持 `f` 返回 `void` 的情况：如果 `f` 返回 void，则向下游发射 `set_value()` 无参完成。相应更新 completion_signatures。
- 支持 inner sender 产出多参数值的情况，例如 `just(1, 2)` 后接 `my_then([](int a, int b){ return a + b; })`。
- 尝试让 `f` 本身可能抛异常的情况也反映到 completion_signatures 中（添加 `set_error_t(std::exception_ptr)`）。
- 对比你的实现与 `stdexec` 源码中 `then` 的实现，记录它做了哪些你没做的防御和优化。

### 验收点

- `sync_wait(my_then(just(42), f))` 能编译通过并返回正确结果，证明你的 sender 满足框架对 completion_signatures 的要求。
- error 和 stopped 通道不被你的 adaptor 吞掉，原样到达下游。
- 你能清楚画出从 `connect` 到 `start` 到 `set_value` 的完整调用链。
- 你能解释为什么 `my_then_receiver` 必须拥有（而不是引用）下游 receiver。
- 你能解释 completion_signatures 为什么必须根据 `f` 的返回类型推导，而不是照抄 inner sender。

### 观察点

- `then` 的核心只做了一件事：在 value channel 上插入一层函数调用。但为了让这件事在类型系统里合法，你需要写出 sender、receiver、operation_state 三个类型外加签名推导。
- 这种"小功能、大骨架"的模式就是所有 sender adaptor 的通用结构。一旦你把 `my_then` 写熟，`upon_error`、`upon_stopped`、`let_value` 的结构你都能预测。
- inner receiver 是这个模式的关键角色：它站在 inner sender 和 downstream receiver 之间，负责拦截、变换、转发。

### 常见坑

- completion_signatures 写错或忘记推导，导致 `sync_wait` 编译失败后以为是框架 bug。
- `my_then_receiver::set_value` 里调用了 `f` 但忘记把结果转发给下游 receiver，导致图断裂。
- `set_error` 和 `set_stopped` 没有实现或没有转发，导致非正常路径静默丢失。
- `my_then_receiver` 以引用方式持有下游 receiver，导致生命周期悬空。
- `my_then_operation_state` 在构造时没有正确连接 inner sender 和 my_then_receiver，导致 `start()` 调用到错误对象。
- `get_env()` 没有转发给下游 receiver，导致 environment query 链断裂。
- 过早追求泛型多参数支持，反而把单参数主线搞不定。

### 提示

- 先让 `my_then` 只支持 `just(单个int值)` + 返回 `int` 的 `f`，跑通后再泛化。
- completion_signatures 推导可以先硬编码一个版本，确认其他部分正确后再改为模板推导。
- 画两张图：一张是 sender 嵌套图（构图阶段），一张是 operation_state 嵌套图（connect 之后）。两张图的结构是镜像关系。
- 如果编译错误看不懂，先检查三件事：completion_signatures 是否正确、receiver_concept 是否声明、connect 返回类型是否匹配。

### 复盘问题

- 为什么 sender adaptor 不能"直接修改值然后传下去"，而是必须引入一层 inner receiver？
- 如果你要实现 `upon_error`（只拦截 error channel），结构上需要改哪些地方？
- `my_then_receiver` 的 `get_env()` 为什么要转发给下游 receiver，而不是返回空 environment？
- 你现在能否预测 `then(then(just(1), f), g)` 在 connect 时会生成几层嵌套的 operation_state？
- completion_signatures 推导出错时，编译器给你的提示是否足够定位问题？如果不够，你会怎么调试？

### 对应官方参考

- `stdexec` 源码中 `then` 的实现（`stdexec/__detail/__then.hpp` 或同类路径）
- P2300R10 中对 sender adaptor 协议的说明
- P3090R0 中对 sender 组合模式的概述

---

## 练习 G-2：pipe 语法

### 目标

为你的 sender adaptor 添加管道语法支持，让 sender 链可以写成 `just(1) | my_then(f) | my_then(g)` 的线性形式。理解 closure object、partial application 和 `operator|` 在类型层面的协作。

### 前置理解

- 你已经在 G-1 中实现了 `my_then(sender, f)`，它接受两个参数。
- 你知道管道语法的核心思路是把双参数调用拆成两步：先传 `f` 得到一个 closure，再通过 `operator|` 把 sender 喂进去。
- 你接受 `ranges::views` 的管道语法是相似的设计先例。
- 你知道链式管道需要 closure 之间也能用 `operator|` 组合。

### 必做任务

1. **实现 `my_then(f)` 单参数重载**，返回一个 closure object：
   - 这个 closure 持有 `f`
   - 它要么是一个 lambda 要么是一个具名类型，例如 `my_then_closure<F>`
   - 当它与一个 sender 通过 `operator|` 组合时，等价于调用 `my_then(sender, f)`

2. **实现 `operator|(Sender, Closure)`**：
   - 左操作数是满足 sender 概念的任意类型
   - 右操作数是你的 closure 类型
   - 返回 `closure(sender)` 的结果

3. **实现 `sender_adaptor_closure` 基类**，使链式管道成为可能：
   - 基本思路：任何继承自 `sender_adaptor_closure` 的 closure 类型都自动获得 `operator|` 支持
   - 当两个 closure 通过 `operator|` 组合时，产生一个新的 closure，它先应用左侧再应用右侧
   - 例如 `auto combined = my_then(f) | my_then(g);` 产生一个新 closure，之后 `just(1) | combined` 等价于 `just(1) | my_then(f) | my_then(g)`

4. **验证单层管道**：
   ```cpp
   auto result = sync_wait(just(1) | my_then([](int x){ return x * 2; }));
   // 期望 result 包含 2
   ```

5. **验证链式管道**：
   ```cpp
   auto result = sync_wait(
       just(1)
       | my_then([](int x){ return x + 10; })
       | my_then([](int x){ return x * 3; })
   );
   // 期望 result 包含 33
   ```

6. **用一段文字解释管道机制在类型层面的工作过程**：
   - `my_then(f)` 返回什么类型
   - `just(1) | my_then(f)` 时 `operator|` 怎么选中
   - `my_then(f) | my_then(g)` 时 `operator|` 怎么选中
   - 最终 `just(1) | combined_closure` 时发生了什么

7. **讨论管道语法的价值**：
   - 为什么 `just(1) | my_then(f) | my_then(g)` 比 `my_then(my_then(just(1), f), g)` 可读性更好
   - 管道语法是否改变了底层的对象关系
   - 在什么情况下管道语法反而会降低可读性（例如复杂的分支合流）

### 进阶任务

- 让你的 `sender_adaptor_closure` 也支持其他 adaptor（例如前面写的 `tap`），验证基类的通用性。
- 研究标准库 ranges 中 `std::ranges::range_adaptor_closure` 的设计，比较它与你的 `sender_adaptor_closure` 的异同。
- 尝试让 `sync_wait` 也能出现在管道末尾：`just(1) | my_then(f) | sync_wait`。分析这需要什么特殊处理。
- 如果你的编译器支持 C++23 deducing this，尝试用它简化 closure 的 `operator()` 实现。

### 验收点

- `just(1) | my_then(f) | my_then(g) | sync_wait` 能编译通过并返回正确结果。
- 你能画出 `my_then(f)` 返回的 closure 对象的类型结构。
- 你能解释 `operator|` 的重载决议为什么能区分"sender | closure"和"closure | closure"两种情况。
- 你能说明管道语法没有引入额外的运行时开销，只是改变了表达形式。

### 观察点

- 管道语法的本质是 partial application + operator overloading。`my_then(f)` 是 `my_then(?, f)` 的 partial application，`|` 把 sender 填入 `?` 的位置。
- 链式管道之所以能工作，是因为每个 `|` 的结果仍然是 sender，可以继续被下一个 `|` 消费。
- `sender_adaptor_closure` 基类让你不需要为每个 adaptor 单独写 `operator|`，这是框架级的复用手段。
- 管道语法让数据从左到右流动，与人类阅读习惯一致。嵌套函数调用则需要从内向外阅读。

### 常见坑

- `operator|` 的两个重载（sender|closure 和 closure|closure）搞混，导致编译器选错重载。
- closure 以引用方式捕获 `f`，导致临时对象销毁后悬空。closure 必须值持有 `f`。
- 忘记让 closure 组合后的结果仍然是 closure 类型，导致链式管道断裂。
- 把 `operator|` 定义为成员函数而不是 friend / 非成员函数，导致左操作数为 sender 时找不到。
- 过度泛化 `operator|`，让它匹配到不该匹配的类型，产生意外的隐式转换。

### 提示

- 先不做 closure 组合（closure | closure），只做 sender | closure。跑通单层管道后再加链式支持。
- `sender_adaptor_closure` 可以用 CRTP 模式实现：`struct my_then_closure : sender_adaptor_closure<my_then_closure>`。
- 用 `requires` 或 SFINAE 约束 `operator|` 的两个重载，避免匹配到错误类型。
- 如果链式管道编译错误很难读，先把链拆成单步变量赋值来定位问题。

### 复盘问题

- 管道语法改变了 sender 图的对象关系吗，还是只改变了表达形式？
- 如果你要为一个新的 adaptor（例如 `filter`、`timeout`）添加管道支持，需要做哪些事情？
- `sender_adaptor_closure` 与 `std::ranges::range_adaptor_closure` 在设计意图上有什么共通之处？
- 管道链 `s | a | b | c` 的 `operator|` 是左结合的，这意味着什么？如果是右结合会有什么不同？
- 在哪些场景下你会选择不用管道语法，而是用传统的嵌套调用？

### 对应官方参考

- P2300R10 中对管道语法的说明
- `stdexec` 源码中 `__sender_adaptor_closure` 相关实现
- C++23 `std::ranges::range_adaptor_closure` 的设计文档

---

## 练习 G-3：retry 组合器

### 目标

用已有的基础原语（如 `let_error`）组合出一个更高层的 `retry` 算法，体验"从原语到策略"的抽象跃升。理解为什么 sender-receiver 框架的组合性使得用户可以在框架之外构建复杂控制流。

### 前置理解

- 你已经理解 `let_error` 的语义：当 sender 以 error 完成时，调用一个函数产生新的 sender 继续执行。
- 你理解 sender 是惰性蓝图，每次 `connect + start` 都是一次新的执行实例。
- 你接受 retry 的核心难点不在于逻辑本身，而在于"如何在 sender 框架内表达重复执行"。
- 你知道 stop_token 是取消协作的标准机制。

### 必做任务

1. **实现 `retry(sender, max_attempts)` 函数**，返回一个新的 sender：
   - 基本思路：用 `let_error` 拦截错误，在错误处理函数中决定是重试还是放弃
   - 需要一个共享的计数器来追踪剩余尝试次数
   - 伪代码结构：
     ```
     retry(sndr, N) =
       let_error(sndr, [counter = N](auto err) {
           if (counter > 0) {
               --counter;
               return retry(sndr, counter);  // 递归重试
           } else {
               return just_error(err);        // 放弃，转发错误
           }
       })
     ```

2. **处理计数器的生命周期**：
   - 计数器不能是栈上局部变量（sender 是惰性的，lambda 执行时栈帧可能已经不在）
   - 使用 `std::shared_ptr<int>` 或将计数器嵌入 sender 自身
   - 确保每次 `connect` 都使用独立的计数器副本（sender 可能被多次连接）

3. **在每次重试时打印日志**：
   - 记录当前是第几次尝试
   - 记录捕获到的错误信息
   - 格式示例：`[retry] attempt 2/5 failed: connection refused`

4. **构造一个可控的测试 sender**：
   - 这个 sender 前 N 次执行时以 error 完成，第 N+1 次以 value 完成
   - 例如用一个共享计数器控制行为：
     ```cpp
     auto make_flaky_sender(std::shared_ptr<int> call_count, int fail_times, int success_value) {
         return just() | then([=]() -> int {
             if ((*call_count)++ < fail_times) {
                 throw std::runtime_error("transient failure");
             }
             return success_value;
         });
     }
     ```

5. **验证 retry 正常工作**：
   ```cpp
   // sender 失败 3 次后成功，最多重试 5 次
   auto result = sync_wait(retry(make_flaky_sender(counter, 3, 42), 5));
   // 期望 result 包含 42，日志显示 3 次失败 + 1 次成功
   ```

6. **验证 retry 耗尽后转发错误**：
   ```cpp
   // sender 始终失败，最多重试 2 次
   auto result = sync_wait(retry(make_flaky_sender(counter, 100, 42), 2));
   // 期望收到 error
   ```

7. **进阶：添加 stop_token 取消支持**：
   - 在每次重试前检查 stop_token 是否已被请求停止
   - 如果已停止，不再重试，直接以 stopped 完成
   - 这需要从 receiver 的 environment 中查询 stop_token

8. **进阶：添加指数退避**：
   - 第 1 次重试等待 100ms，第 2 次等待 200ms，第 3 次等待 400ms...
   - 退避可以用一个 `schedule_after` 类的定时 sender 实现（如果你的版本不支持，可以用 `then` + `std::this_thread::sleep_for` 简化替代）
   - 退避上限设为 5 秒

9. **讨论**：retry 为什么能体现"higher-level algorithms composed from primitives"：
   - retry 本身不需要修改框架内部
   - 它完全通过组合 `let_error`、`just_error`、`just`、`then` 等已有原语实现
   - 任何满足 sender 概念的输入都能被 retry 包装
   - 这种组合性是 sender-receiver 框架的核心设计优势

### 进阶任务

- 把 retry 改成 retry policy 模式：接受一个策略对象来决定是否重试、等待多久、如何变换错误。
- 实现一个 `retry_when(sender, predicate)` 变体：只在 predicate 对错误返回 true 时重试。
- 尝试让 retry 支持不同的错误类型（不只是 `exception_ptr`），观察 completion_signatures 如何变化。
- 对比你的实现与生产级重试库（如 folly::futures 的 retrying）的设计差异。

### 验收点

- retry 在 sender 失败次数 < max_attempts 时能成功恢复。
- retry 在 sender 始终失败且超过 max_attempts 时能正确转发最后一次错误。
- 每次重试都有可观察的日志输出，包含尝试序号和错误信息。
- 你能解释 retry 内部的 sender 图结构：每次 `let_error` 触发时生成了什么样的新 sender。
- 你能说明为什么 retry 不需要修改框架内部就能实现——它是纯粹的用户层组合。

### 观察点

- retry 的核心困难是"在惰性框架中表达重复"。sender 是蓝图，你不能用普通的 while 循环来重试，因为循环是急切的。你需要用 `let_error` 把"下一次尝试"表达为一个新的 sender 蓝图。
- 计数器的生命周期是 retry 实现中最容易出错的地方。它必须活过整个重试序列，而 sender 图的惰性特征意味着构图阶段和执行阶段的时间跨度可能很大。
- retry 是"面向组合"设计哲学的典型产物：框架提供原语，用户在框架外部组合出策略。
- 指数退避的引入让你体验到：即使是时间维度的控制，也可以通过 sender 组合来表达。

### 常见坑

- 用普通 `for` 循环实现 retry，完全绕过了 sender 图的惰性模型。这样写能跑，但你没有练到框架的组合能力。
- 计数器是栈上变量，sender 执行时已经销毁。
- 忘记在递归构造 sender 时传递新的剩余次数，导致无限重试。
- `let_error` 的回调函数返回类型不正确：它必须返回一个 sender，不是直接返回值。
- 测试 sender 的状态在多次 connect 之间共享，但没有考虑多次 connect 的独立性要求。
- 指数退避使用 `std::this_thread::sleep_for` 阻塞当前线程，在真实异步场景中是不可接受的（但作为学习简化版本可以接受）。

### 提示

- 先实现一个没有退避、没有取消的最简版本。验证逻辑正确后再加功能。
- `let_error` 是这道题的核心工具。如果你对它的语义不确定，先单独写一个最小的 `let_error` 例子。
- 测试 sender 可以用一个 `shared_ptr<int>` 计数器来控制"第几次调用失败"，这比用全局变量更安全。
- 如果递归构造 sender 导致类型无限递归，考虑用类型擦除（`any_sender`）来打断递归。
- 画出 retry 在"失败 2 次后成功"场景下的完整 sender 图展开过程。

### 复盘问题

- 为什么 retry 不能用普通 while 循环实现，而必须用 sender 组合？
- retry 内部用到了 `let_error` 的哪个关键特性？如果没有 `let_error`，你能实现 retry 吗？
- 计数器为什么必须有共享生命周期？如果你用值捕获会发生什么？
- retry 的递归性质会不会导致类型无限递归？如果会，怎么解决？
- 如果你要把 retry 发布为库组件，你会如何设计它的接口（参数、约束、文档）？
- 从 retry 的实现过程中，你对"面向组合的框架设计"有什么新的理解？

### 对应官方参考

- `stdexec` 中 `retry` 相关示例或测试（如有）
- P2300R10 中 `let_error` 的语义说明
- P2175R0 / P2519R0 中对 sender algorithm 组合性的讨论

---

## 做完模块 G 之后，你现在应该能说清楚什么

至少把下面几句话说顺：

- sender adaptor 的核心模式是：创建 inner receiver 包装下游 receiver，拦截目标 completion channel，变换或处理后转发，其余 channel 原样透传。
- completion_signatures 必须跟随变换函数的返回类型在类型级别更新，否则框架拒绝组合。
- pipe 语法通过 closure object + `operator|` 实现，是 partial application 在 C++ 中的惯用表达，不改变底层对象关系。
- 更高层的算法（如 retry）可以完全在框架外部，通过组合 `let_error`、`just`、`then` 等原语实现。
- sender-receiver 框架的最大价值之一就是这种组合性：框架提供原语和协议，用户在上层搭建策略。
