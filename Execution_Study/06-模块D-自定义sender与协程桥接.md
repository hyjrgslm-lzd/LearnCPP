# 06 模块 D：自定义 sender 与协程桥接

## 模块目标

前面三个模块都在使用现成的 sender 组合器。这个模块要把你带到更底层：

- 自己看见 receiver 到底长什么样
- 自己写出最小的 `connect -> operation_state -> start`
- 理解 `completion_signatures` 为什么是 sender 可组合性的基础
- 亲手写一个最小 sender adaptor
- 再把 sender 图与 coroutine 表达方式做一次对照

如果你不经过这一层，就很容易把 `stdexec` 误解成"花哨的回调拼装器"。

## 模块完成标准

做完本模块，你至少要能稳定说清楚：

- 为什么 sender 本身不等于一次执行实例。
- 为什么 `connect` 必须返回 `operation_state`。
- 为什么 receiver 需要面对 value / error / stopped 三种完成方式。
- 为什么 `completion_signatures` 对于与标准组合器互操作是必需的。
- 为什么 sender adaptor 的核心模式是"inner receiver 包装"。
- 为什么协程语法可以改善局部表达，但不会抹掉底层调度与完成语义。

## 使用建议

- 本模块不要一上来追求"完全符合所有概念约束的泛型工业级实现"。
- 先做一个最小、单一、容易观察的实现，再逐步补 completion channel 和签名信息。
- 如果你本地固定版本与本文术语拼写略有不同，以你固定版本能编译通过为准，但要保持对象关系不变。

## 类型骨架桥接段

从"使用现有组合器"到"手写 receiver/sender"是这套练习里最大的跳跃。为了降低入门摩擦，这里给出 stdexec 中 receiver 和 sender 需要满足的最小类型要求骨架。你不需要背它们，只需要在编译不过时回来查对。

### receiver 最小骨架

```cpp
struct my_receiver {
    using receiver_concept = stdexec::receiver_t;

    // 三条 completion channel
    friend void tag_invoke(stdexec::set_value_t, my_receiver&& self, auto&&... values) {
        // 处理 value completion
    }
    friend void tag_invoke(stdexec::set_error_t, my_receiver&& self, auto&& error) {
        // 处理 error completion
    }
    friend void tag_invoke(stdexec::set_stopped_t, my_receiver&& self) noexcept {
        // 处理 stopped completion
    }

    // environment 入口
    friend auto tag_invoke(stdexec::get_env_t, const my_receiver& self) {
        return stdexec::empty_env{}; // 或你自己的 environment
    }
};
```

### sender 最小骨架

```cpp
struct my_sender {
    using sender_concept = stdexec::sender_t;

    // 声明 completion signatures
    using completion_signatures = stdexec::completion_signatures<
        stdexec::set_value_t(int),           // 可能产出 int
        stdexec::set_error_t(std::exception_ptr), // 可能产出异常
        stdexec::set_stopped_t()             // 可能被停止
    >;

    // connect：与 receiver 连接，返回 operation_state
    template <typename Receiver>
    friend auto tag_invoke(stdexec::connect_t, my_sender self, Receiver rcvr) {
        return my_operation_state<Receiver>{std::move(self), std::move(rcvr)};
    }
};
```

### operation_state 最小骨架

```cpp
template <typename Receiver>
struct my_operation_state {
    using operation_state_concept = stdexec::operation_state_t;

    my_sender sndr_;
    Receiver rcvr_;

    // start：真正开始执行
    friend void tag_invoke(stdexec::start_t, my_operation_state& self) noexcept {
        stdexec::set_value(std::move(self.rcvr_), 42); // 发射值
    }
};
```

**注意**：以上骨架基于 `tag_invoke` 风格。如果你的固定版本支持 member-function 风格（如直接定义 `connect` 成员函数），也可以使用。关键是对象关系不变：sender 描述工作 → connect 生成 operation_state → start 启动执行 → completion channel 交付结果。

在第二阶段的模块 E 中，你会专门学习 `tag_invoke` 和 CPO 的设计意图与演进历史。在这里，先把它当作"框架要求的接入方式"即可。

---

## 练习 11：最小 receiver

### 目标

亲手写一个能接住三条 completion channel 的最小 receiver，把"receiver 不是抽象概念，而是真正参与执行的对象"刻进直觉。

### 前置理解

- 你已经知道 receiver 要面对 `set_value`、`set_error`、`set_stopped`。
- 你知道 receiver 还需要提供 environment 入口。
- 你能接受这题先从很小的 receiver 壳开始，不追求复杂模板技巧。
- 你已经看过上面的类型骨架桥接段。

### 必做任务

1. 定义一个 logging receiver，给它一个明显的名字，例如 `logging_receiver`。
2. 为它实现三类 completion 入口：
   - `set_value(...)`
   - `set_error(...)`
   - `set_stopped()`
3. 给它实现最小 `get_env()`，先返回一个足够简单的 environment 即可。
4. 准备三条最小 sender：
   - 一条正常 value 路径，例如 `just(42)`
   - 一条错误路径，例如 `just_error(...)`
   - 一条停止路径，例如 `just_stopped()`
5. 对每条 sender 都手动执行一次 `connect` 与 `start`，让你的 receiver 真正接住结果。
6. 记录三种 completion 分别打到 receiver 的哪一个入口。

### 进阶任务

- 让 receiver 把收到的事件记录到一个小型日志结构里，而不只是打印文本。
- 在 `set_error` 中区分异常类型或异常来源。
- 尝试给 `get_env()` 返回一个更接近官方示例的最小可查询环境，例如包含 `never_stop_token` 的属性。

### 验收点

- 你能手动完成 `connect` 与 `start`，而不是全程依赖 `sync_wait`。
- 你能观察到三条 completion channel 分别落在哪个成员函数。
- 你能说明 receiver 为什么必须出现在执行模型里，而不是只是"库内部回调对象"。
- 你能说出 `get_env()` 与 environment 查询之间的联系。

### 观察点

- `sync_wait` 之类的高级接口平时把 receiver 藏起来了，但它并没有消失。
- receiver 是这次执行的消费端，它让 completion channel 真正有落点。
- 一旦你开始自己写 receiver，就会更容易理解 environment 为什么从 receiver 一侧传播。

### 常见坑

- 以为 receiver 只需要 `set_value`，忽略错误和停止。
- `set_error` 只打印一行文本，完全看不出异常来源。
- `get_env()` 返回内容过重，反而把注意力从 receiver 本身带偏。
- 写出一个 receiver 类型，但没有真的手动 `connect` / `start` 它。
- 在编译错误面前放弃——回到桥接段检查骨架是否匹配。

### 提示

- 可以先参考上面的骨架，再根据你的需要精简。
- 最小目标是：能看见三条 completion 真的打到你的代码里。
- 不要一开始就追求完美泛型，把这题做成模板元编程体操。
- 日志内容至少包含：sender 名字、completion 种类、收到的值或异常概述。

### 复盘问题

- 为什么 receiver 不是"可有可无的消费细节"？
- 为什么 environment 查询要从 receiver 一侧向上看？
- 你手写 receiver 之后，对 `sync_wait` 的黑盒感有没有下降？
- 如果未来你要做 tracing、metrics 或调试钩子，receiver 为什么会成为很自然的切入点？

### 对应官方参考

- `stdexec/examples/scope.cpp` 中的最小 receiver 风格
- `stdexec` 中 `just_error`、`just_stopped` 的基础用法

---

## 练习 12：最小 sender 与 operation_state

### 目标

手写一个最小 sender，让你亲眼看到：sender 是蓝图，`operation_state` 才是一次具体执行实例。**这题的 `completion_signatures` 是必做内容。**

### 前置理解

- 你已经完成练习 11，能够手动 `connect` 和 `start`。
- 你知道 sender 至少要能与 receiver 连接。
- 你知道 sender 需要声明 `completion_signatures` 才能与标准组合器互操作。

### 必做任务

1. 设计一个最小 sender，例如 `single_value_sender`，内部只保存一个整数或一个小结构体。
2. 为它定义 `connect(receiver)`，返回一个你自己写的 `operation_state` 类型。
3. 这个 `operation_state` 至少要保存两样东西：
   - 要发送的值
   - 被连接的 receiver
4. 为 `operation_state` 实现 `start()`，在其中调用 `set_value(receiver, value)`。
5. 用上一题的 logging receiver 手动连接并启动它。
6. **为 sender 定义 `completion_signatures`**，至少包含 `set_value_t(int)` 和 `set_error_t(std::exception_ptr)`。
7. **验证你的 sender 能被 `sync_wait` 消费**。如果 `sync_wait` 编译不过，检查 `completion_signatures` 是否正确——这就是签名声明的实际作用。
8. 在笔记里明确写出：sender、receiver、operation_state、completion_signatures 各自承担什么责任。

### 进阶任务

- 把 sender 从 value-only 版本升级为可配置模式：根据一个标志决定发 value、error 或 stopped。相应更新 `completion_signatures` 声明。
- 尝试故意写错 completion_signatures（声明 `set_value_t(int)` 但实际发 `set_value_t(string)`），观察编译器给你什么提示。
- 如果你愿意挑战，再试着让它和 `then` 组合，观察"进入通用生态"需要补哪些元信息。

### 验收点

- 你能清楚说出 sender 和 operation_state 的职责分界。
- 你能手动触发一次 value completion，并看到 receiver 被调用。
- 你没有把 receiver 以悬空引用等危险方式塞进 operation_state。
- 你能解释为什么 `connect` 不能直接返回"立刻运行完的结果"。
- **你的 sender 能被 `sync_wait` 消费，证明 `completion_signatures` 声明正确。**

### 观察点

- sender 更像可重复使用的描述对象。
- operation_state 更像某一次实际执行的载体。
- 一旦你手写这一层，会立刻看清"为什么生命周期问题最终会落到 operation_state 和其拥有者身上"。
- `completion_signatures` 是 sender 的"类型级合同"——没有它，编译器和组合器都无法做类型安全检查。

### 常见坑

- 在 sender 内部直接调用 receiver，完全跳过 operation_state。
- `operation_state` 没有稳定拥有 receiver，导致生命周期不安全。
- `start()` 里重复发射 completion，破坏一次执行的基本语义。
- 一开始就追求支持所有概念，结果主线对象关系反而没看清。
- 忘记定义 `completion_signatures`，导致 `sync_wait` 编译失败后以为是"库 bug"。

### 提示

- 先做一个最小可运行版本，再考虑概念完整性。
- `connect` 的关键不是"怎么写得炫"，而是"能产出一个稳定的执行实例"。
- `completion_signatures` 从一开始就要定义，不要留到最后。
- 做完后一定要画对象图：sender 可被复用，但每次 `connect` 都应该生成新的 operation_state。

### 复盘问题

- 为什么 sender 不能直接等价为任务实例？
- 为什么 operation_state 在框架里必须单独存在，而不是省略掉这一层？
- 如果同一个 sender 连续连接两个 receiver，会产生几个 operation_state？
- 你现在再看 `sync_wait`，能否想象它内部大致在做什么？
- 如果你没有定义 `completion_signatures`，为什么标准组合器拒绝与你的 sender 配合？

### 对应官方参考

- P3090R0 的基础对象关系说明
- P3143R0 对示例的分层拆解

---

## 练习 13：最小 sender adaptor

### 目标

手写一个最小 sender adaptor（包装另一个 sender 并变换其行为），理解 `then`、`upon_error` 等库算法的内部结构模式。

### 前置理解

- 你已经完成练习 12，能手写 sender、receiver、operation_state。
- 你理解 completion_signatures 的作用。
- 你接受这题先做一个只关注 value channel 的最小 adaptor。

### 必做任务

1. 设计一个 `tap` adaptor：它包装一个 inner sender，在 value completion 时先执行一个 side-effect 函数（如打印日志），然后把值原样转发给下游 receiver。
2. 实现 `tap_sender<InnerSender, F>` 类型：
   - 持有 inner sender 和 side-effect 函数 `f`
   - 定义 `completion_signatures`：与 inner sender 相同
   - 定义 `connect(receiver)`：创建一个 `tap_receiver` 包装下游 receiver
3. 实现 `tap_receiver<DownstreamReceiver, F>` 类型：
   - `set_value`：调用 `f(values...)`，然后调用下游 receiver 的 `set_value`
   - `set_error`：直接转发给下游 receiver
   - `set_stopped`：直接转发给下游 receiver
   - `get_env`：转发给下游 receiver
4. 实现 `tap_operation_state`：连接 inner sender 和 tap_receiver。
5. 验证：`sync_wait(tap(just(42), [](int x){ std::cout << "tap: " << x; }))` 应打印 "tap: 42" 并返回 42。
6. 在笔记中画出 sender 嵌套图：`tap_sender` 持有 `inner_sender`，`connect` 时生成 `tap_receiver` 包装 `downstream_receiver`，再用 `tap_receiver` 与 `inner_sender` connect。

### 进阶任务

- 再做一个 `map` adaptor（类似简化版 `then`）：与 `tap` 不同，`map` 的函数 `f` 改变值的类型，因此 `completion_signatures` 需要从 inner sender 的签名变换而来。
- 为 `tap` 或 `map` 添加管道语法 `operator|` 支持：`just(42) | tap(f) | sync_wait`。
- 尝试让 adaptor 也正确转发 environment query。

### 验收点

- 你能清楚说出 sender adaptor 的核心模式：inner receiver 包装 + channel 拦截 + 其余透传。
- 你的 adaptor 能与 `sync_wait` 和其他标准 sender 互操作。
- 你能解释为什么 adaptor 的 `completion_signatures` 依赖于 inner sender 的 signatures。
- 你能指出 adaptor 中 sender、receiver、operation_state 的嵌套拥有关系。

### 观察点

- sender adaptor 的本质是 sender-in-sender 嵌套 + receiver-wrapping。
- 每增加一层 adaptor，connect 时就多产生一层嵌套的 receiver 和 operation_state。
- 这种层层包装的模式就是 `then`、`let_value`、`upon_error` 等标准算法的内部结构。
- 理解这个模式后，你就能预测任何 sender 图的 connect 行为。

### 常见坑

- 在 `tap_receiver::set_value` 里忘了转发给下游 receiver，结果图断了。
- `set_error` 和 `set_stopped` 没有转发，导致 error/stopped 通道丢失。
- `get_env` 没有转发，导致下游 environment query 失效。
- 让 `tap_receiver` 持有下游 receiver 的引用而非值，导致生命周期问题。
- adaptor 的 `completion_signatures` 写错，与 inner sender 不一致。

### 提示

- 先让 tap 只处理 value channel，error 和 stopped 直接转发。
- 对照桥接段的骨架写出每个类型的结构。
- 画出两层嵌套的对象图（outer sender → inner sender, outer receiver → inner receiver）再写代码。
- 这道题是后面第二阶段模块 G（实现 my_then）的直接预热。

### 复盘问题

- sender adaptor 的核心设计模式可以用一句话总结吗？
- 为什么 adaptor 需要转发它不关心的 completion channel？
- 如果你要实现一个只关心 error channel 的 adaptor（类似 `upon_error`），结构会有什么变化？
- 理解了 sender adaptor 模式后，你现在能否预测 `then(just(42), f)` 在 connect 时生成了什么样的对象树？

### 对应官方参考

- `stdexec` 中 `then` 的实现思路
- P3090R0 中对 sender adaptor 组合的说明

---

## 练习 14：协程桥接

### 目标

把你已经写过的一段 sender 图改写成 coroutine 风格，并对照两种表达方式的优缺点，理解"协程是表达层，不是调度语义替代品"。

### 前置理解

- 你已经理解 sender 图如何描述工作。
- 你知道 `stdexec` 示例里存在 `task` 与 `co_await` 组合。
- 你接受本题的重点是表达差异与模型不变，而不是写出一整套协程框架。

### 必做任务

1. 从前面模块里选一道你已经做过的题，优先推荐模块 B 的两阶段流程或模块 C1 的统一结果流程。
2. 保留原 sender 图版本，作为对照组。
3. 再写一个 coroutine 版本，例如返回 `task<Result>`，在协程体内逐步 `co_await` 一到两段 sender。
4. 保证 coroutine 版本与原 sender 图版本产出同样的最终结果。
5. 对照记录两种版本：
   - 哪些顺序关系在协程里更线性
   - 哪些调度边界仍然需要显式处理
   - 哪些 completion 语义并没有因为 `co_await` 就消失
6. 用一页笔记比较"表达简洁性"和"图结构可见性"。

### 进阶任务

- 尝试把停止路径转成更容易消费的结果形式，例如可选值或状态对象。
- 再做一个版本：在 coroutine 内部显式切换 scheduler，观察它并不会自动替你决定执行资源。
- 如果你已经完成练习 12-13，可思考"自定义 sender 想要更自然地被协程消费，需要补什么接口或语义"。

### 验收点

- 两个版本结果一致。
- 你能说明协程改变的是表达方式，而不是 sender-receiver 的底层对象关系。
- 你能指出 coroutine 版本里哪些地方仍然依赖 sender 或 scheduler 语义。
- 你能说明什么时候 sender 图更容易看出整体结构，什么时候 coroutine 更适合写局部顺序逻辑。

### 观察点

- 协程常常让"顺序读取多个异步结果"的代码更线性。
- 但 sender 图在表达并列组合、显式调度和 completion 路径方面，往往更结构化。
- 这两者不是非此即彼，而是同一模型在不同表达层次上的配合。

### 常见坑

- 误以为 `co_await` 会自动解决调度器选择问题。
- 协程体写得很顺，但已经看不清哪一段在哪个 scheduler 上运行。
- 只比较语法长短，而不比较对象关系和 completion 语义。
- 为了迁就协程版本，反而删掉了原 sender 图里很关键的结构边界。

### 提示

- 先选你最熟悉的一题改写，不要一上来选最复杂的生命周期题。
- 先保证结果一致，再写比较笔记。
- 重点不是"协程更优雅"，而是"哪些结构被隐藏了，哪些语义仍然存在"。
- 参考 `hello_coro` 的最小形状，但不要把这题做成抄示例。

### 复盘问题

- 协程帮你隐藏了哪些样板，哪些东西其实没被消掉？
- 如果要表达三条并列分支的合流，sender 图和 coroutine 哪个更直观？为什么？
- 如果要表达显式的 scheduler 切换，协程会不会让它更容易被忽略？
- 为什么说协程是表达层桥接，而不是执行模型替换？

### 对应官方参考

- `stdexec/examples/hello_coro.cpp`
- P3090R0 与 P3143R0 中对 sender-receiver 对象关系的说明

---

## 做完模块 D 之后，你现在应该能说清楚什么

至少把下面几句话说顺：

- sender 是蓝图，operation_state 是一次具体执行。
- `connect` 与 `start` 不是细枝末节，而是模型骨架。
- `completion_signatures` 是 sender 的类型级合同，没有它就无法与通用生态互操作。
- receiver 让 completion channel 真正有落点。
- sender adaptor 的核心模式是 inner receiver 包装 + channel 拦截。
- coroutine 可以改善局部表达，但不会替你抹掉调度、completion、生命周期这些底层问题。
- 看懂官方示例时，应该先看对象关系，再看语法糖。
