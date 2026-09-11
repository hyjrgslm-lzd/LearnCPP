# 08 adaptor：从 my_then 推导 sender adaptor

`then` 看起来只是“等上游有值后调用一个函数”。在 sender/receiver 模型里，这个小动作会碰到五个对象：外层 sender、下游 receiver、包装 receiver、operation state、变换函数。G1 的 `my_then` 只做 value channel 上的函数调用，但它足够代表多数 adaptor 的骨架。

本章固定对照 stdexec `nvhpc-26.05`，revision `6d7ad689f4d4831c5136e4abe1c601f9a3b64e43`。读者把 `$env:STDEXEC_ROOT` 指向这个固定 checkout 后，可查看相对文件：

```text
examples/algorithms/then.hpp
include/stdexec/__detail/__then.hpp
```

对应固定 commit 链接：

- <https://github.com/NVIDIA/stdexec/blob/6d7ad689f4d4831c5136e4abe1c601f9a3b64e43/examples/algorithms/then.hpp>
- <https://github.com/NVIDIA/stdexec/blob/6d7ad689f4d4831c5136e4abe1c601f9a3b64e43/include/stdexec/__detail/__then.hpp>

G1 主线使用这个固定版本支持的成员函数 dispatch：receiver 写 `set_value/error/stopped/get_env` 成员，operation state 写 `start` 成员，sender 写 `connect` 成员。老式 `tag_invoke` 会留到后续历史对照题，不作为新样章主协议。当前库实现 `stdexec::then` 已经转成 sender expression，但核心动作一样：改写 `set_value`，保留 error/stopped，把异常转成 error。

## 背景

先看不用 sender 的普通代码：

```cpp
auto y = f(x);
```

同步代码里，`f` 的参数和返回值都在这一行出现。sender 里，`x` 不在 `my_then` 调用点出现。`my_then(ex::just(1), f)` 只创建一个描述；真正的 `1` 在之后 `start(op)` 时经由上游 `set_value(1)` 送到 receiver。

所以 adaptor 的工作不是马上调用 `f`，而是制造一个中间 receiver。这个 receiver 接住上游完成信号：

```text
downstream receiver
        ^
        | set_value(f(args...)) / set_error(e) / set_stopped()
my_then_receiver
        ^
        | set_value(args...) / set_error(e) / set_stopped()
inner sender
```

这就是“惰性执行”的直接后果。`my_then` 只组装对象；`connect` 才建立一次运行；`start` 才触发运行。

## 五个对象与调用时序

G1 Reference 的五个对象：

```cpp
my_then_sender<InnerSender, F>
my_then_receiver<DownstreamReceiver, F>
my_then_operation_state<InnerSender, DownstreamReceiver, F>
InnerSender
DownstreamReceiver
```

调用时序是：

```text
auto s = my_then(inner, f)
auto op = connect(std::move(s), downstream)
  -> connect(inner, my_then_receiver{downstream, f})
start(op)
  -> start(inner_op)
  -> inner calls set_value / set_error / set_stopped on my_then_receiver
  -> my_then_receiver completes downstream exactly once
```

两个边界要分清：

- `connect` 是构造阶段。它可以分配、移动对象、连接上游，也可以抛异常。G1 的 `throw_connect_sender` 验证这个异常不能被吞掉。
- `start` 是启动阶段。operation state 已经存在，`start` 必须 `noexcept`。G1 只在 `start` 里委托 `ex::start(inner_op_)`。

operation state 删除 copy/move。原因很简单：上游可能在内部保存它的地址，移动后地址变化会破坏协议。教学版不假设所有上游都能承受移动。

## 最小错误复现

最常见的错误是只写 value path：

```cpp
template <class Error>
void set_error(Error&&) && noexcept {
  ex::set_value(std::move(downstream_), -404); // 错
}
```

这会把上游失败伪造成成功值。`validation/bad/solution.hpp` 故意这样写。checker 用一个既可能 `set_value(int)` 又实际 `set_error(exception_ptr)` 的 sender 验证它：

```text
error_sender -> my_then(identity) -> sync_wait
```

正确实现会让 `sync_wait` 重新抛出 `"boom"`。bad 实现没有抛错，于是触发诊断：

```text
error channel forwarded as set_error
```

这个反例说明 adaptor 不能只看“我关心 value”。error/stopped 是完成协议的一部分，不是异常分支上的可选装饰。

## 签名集合变换

sender 的 completion signatures 是它承诺可能发出的完成信号集合。`my_then` 改写 value signatures：

```text
set_value_t(Args...) -> set_value_t(invoke_result_t<F, Args...>)
set_value_t(Args...) -> set_value_t()                    // F 返回 void
set_error_t(E)       -> set_error_t(E)
set_stopped_t()      -> set_stopped_t()
```

因为 `F` 可能抛异常，教学版额外加入：

```cpp
set_error_t(std::exception_ptr)
```

G1 Reference 没有直接调用 `stdexec::then`，也没有依赖上游 `__then` 的 sender expression 框架。它写了一个很小的 meta 变换：拆开 `stdexec::completion_signatures<...>`，逐个处理 `set_value_t(Args...)`、`set_error_t(E)` 和 `set_stopped_t()`，最后拼回新的 `completion_signatures`。

```cpp
template <class Result>
struct my_then_result_completion {
  using type = ex::completion_signatures<ex::set_value_t(Result)>;
};

template <>
struct my_then_result_completion<void> {
  using type = ex::completion_signatures<ex::set_value_t()>;
};

template <class F, class... Args>
struct my_then_value_completion
    : my_then_result_completion<std::invoke_result_t<F, Args...>> {};

template <class Self, class... Env>
static consteval auto get_completion_signatures()
  -> typename my_then_completions<
       F,
       ex::completion_signatures_of_t<InnerSender, Env...>>::type;
```


这里不要写成 `std::conditional_t<std::is_void_v<R>, set_value_t(), set_value_t(R)>`。两个模板实参都必须先能形成类型；把依赖参数 `R` 替换成 `void` 时，另一个分支的函数参数类型就不合法。MSVC 在本轮接受了这个写法，GCC13 拒绝；独立的 `void` 特化避开了无效类型的形成。注意，直接写非依赖的 `set_value_t(void)` 是无参函数类型的合法拼写，不能把两种情况混为一谈。依据为 N5050 [dcl.fct]/2（PDF 第245页），其中空参数的特例要求该 `void` 类型非依赖。

这里的 `Env...` 不能删。上游 sender 的签名可能依赖环境，例如 scheduler、stop token、allocator 或自定义 query。`my_then` 是 adaptor，不拥有这些环境事实；它要把环境参数交给 child sender，再在 child 的结果上改写 value signatures。

G1 的 checker 有一个专门例子：`env_signature_sender` 在 `int_signature_env` 下声明 `set_value_t(int)`，在 `string_signature_env` 下声明 `set_value_t(std::string)`。同一个 `env_signature_fn` 对 `int` 返回 `long`，对 `std::string` 返回 `std::size_t`。因此 `my_then` 必须分别能在两个环境下实例化，并在运行时分别得到 `22` 和 `4`。这证明它不是只把 `stdexec::env<>` 下的签名写死。

## 值类别、异常规格、环境

`set_value` 中要转发上游参数：

```cpp
std::invoke(std::move(f_), std::forward<Values>(values)...)
```

这样 move-only 值可以通过。checker 使用 `std::unique_ptr<int>` 验证：如果实现把参数按值复制，编译或运行会失败。

`F` 返回 `void` 时，下游收到的是无参 `set_value()`。不能把 `void` 存进变量，也不能发一个假值。

异常处理只包住 `F` 的调用和向下游发送 value 的动作：

```cpp
try {
  ...
} catch (...) {
  ex::set_error(std::move(downstream_), std::current_exception());
}
```

教学版使用 `std::exception_ptr`，因为它是最小可移交的异常载体。为保持代码短，它保守地把 `set_error_t(std::exception_ptr)` 加入结果签名，即使某个具体 `F` 是 `noexcept`。生产实现会把 `F` 的值类别、`noexcept`、不可调用诊断、domain customization 和 completion scheduler 处理得更精细；G1 只保留足以讲清协议的部分。

环境转发发生在包装 receiver 上：

```cpp
auto get_env() const noexcept -> decltype(ex::get_env(downstream_)) {
  return ex::get_env(downstream_);
}
```

上游在连接或启动期间查询 receiver 环境时，看到的仍是下游环境。G1 的 `env_read_sender` 会从 receiver 环境读出 `41`，经 `my_then` 加一后下游得到 `42`。

## 生命周期

`my_then_sender` 保存 `inner_` 和 `f_`。`connect(std::move(sender), receiver)` 把它们移动进 `my_then_operation_state` 内部的 `inner_op_`：

```cpp
inner_op_(ex::connect(std::forward<InnerSender>(sender),
                      inner_receiver_t{std::forward<DownstreamReceiver>(receiver),
                                       std::forward<F>(f)}))
```

连接完成后，运行所需状态属于 operation state。`start` 不能访问已经被移动走的 sender，也不能重新创建 receiver。完成信号发出后，下游 receiver 被 move 掉，不能再访问。这个“接受的 op exactly one terminal signal，然后可能销毁自身”的规则，是后续 retry、scope、run_loop 的基础。

## Part 解析

Part 1：`my_then_sender` 只保存描述，不执行。它必须提供 sender 标记和 completion signatures。签名推导按 `Env...` 传给上游，然后把每个 value completion 改成 `F` 的返回类型。

Part 2：`my_then_receiver::set_value` 是唯一改写通道。非 `void` 返回就把结果作为一个 value 发下去；`void` 返回就发无参 value；抛异常就发 `std::exception_ptr`。

Part 3：`set_error`、`set_stopped`、`get_env` 透传。不要记录、吞掉、转换这些信号，除非 adaptor 的语义明确要求。

Part 4：operation state 保存 `connect(inner, wrapper_receiver)` 的结果，删除 copy/move。`start` 是 `noexcept` 委托。

Part 5：工厂函数按值保存 decay 后的 sender 和函数对象。教学版只覆盖 rvalue `connect`，这足够本题使用；完整库会继续处理 const/lvalue sender、pipe closure、domain customization 和更强诊断。

## 边界

G1 是教学实现，不是替代 `stdexec::then` 的生产实现。已覆盖：

- 零/多输入值：checker 同时覆盖 `just()` 和 `just(1, 2)`。
- 返回 `void` 和类型变化。
- move-only 参数。
- transform 抛异常。
- 上游 error/stopped 透传。
- receiver 环境转发。
- 环境改变上游 completion signatures 时，`my_then` 继续按该环境推导 value 类型。
- operation state 不可移动、`start` noexcept、`connect` 抛异常边界。

未覆盖或只浅覆盖：

- pipe closure 写法。
- lvalue/const sender 全组合。
- domain customization。
- 标准最终 `std::execution` 命名空间。
- 完整 `stdexec::then` 对函数对象值类别和 `noexcept` 的精细签名收窄。

这些留给后续 G2/G3 和源码阅读。G1 的目标是让读者能亲手推导 adaptor 的骨架，而不是重写整个 stdexec 内部框架。

## G2：pipe closure 不是语法糖，是真对象组合

G1 已经说明 adaptor 的主体是 sender、receiver 和 operation state。G2 换一个角度：为什么 `sender | then(f)` 可以成立？这不是 C++ 对 sender 的内建语法，而是普通 `operator|` 重载。

教学版把 adaptor 分成两类对象：

```cpp
template <class F>
struct then_closure;

template <class Left, class Right>
struct composed_closure;
```

`then(f)` 不马上拿到 sender，因此它只能保存 `f`，返回一个 closure。等左侧真的出现 sender，`operator|(sender, closure)` 才调用 closure：

```cpp
template <stdexec::sender Sender, pipe_closure Closure>
auto operator|(Sender&& sender, Closure&& closure) {
  return std::forward<Closure>(closure)(std::forward<Sender>(sender));
}
```

这一步才把 `sender` 交给 `stdexec::then` 或你自己的 adaptor 实现。换句话说：

```cpp
ex::just(1) | c10_g2::then(f)
```

等价于：

```cpp
c10_g2::then(f)(ex::just(1))
```

第二类是 `adaptor | adaptor`。这时两边都没有 sender，所以不能执行，只能生成一个新的 closure：

```cpp
auto p = then(f) | then(g);
```

`p` 收到 sender 后，必须按左到右执行：先 `f`，再 `g`。

```cpp
return std::move(right_)(std::move(left_)(std::forward<Sender>(sender)));
```

G2 的 bad 版本故意只保留右侧 closure。`then(+1) | then(*3) | then(to_string)` 应该让 `4` 变成 `"15"`；bad 只执行最后一步，得到 `"4"`，被 checker 以 `adaptor composition preserves left-to-right order` 拒绝。

这里的约束也很关键。`int | then(f)` 不应该进入 sender 管道重载；只有满足 `stdexec::sender` 的左操作数才能匹配。两个 closure 相互组合时也只接受 closure，不接受任意对象。checker 用 `requires` 表达式做编译期检查，避免把“某个错误碰巧编译失败”当作设计。

值类别边界是 G2 的另一个重点。closure 保存函数对象，组合后经常只使用一次，因此实现允许 move closure。sender 中的 value 也要继续按上游语义移动；checker 使用 `std::unique_ptr<int>` 证明 pipe 语法没有偷偷复制 value。

G2 不是要重写 `stdexec` 的整套 pipe 系统。它只让读者亲手实现两个必要规则：`sender | adaptor` 执行一次 adaptor，`adaptor | adaptor` 延迟组合。后续读标准库或 stdexec 源码时，复杂的 domain customization、pipeable sender expression 和诊断框架都可以落回这个骨架理解。

## G3：retry 是状态机，不是递归调用 then

`retry(factory, max_attempts)` 的输入不是一个已经连接好的 operation state，而是一个 sender factory。每次尝试都要重新调用 factory，拿到新的 sender，再 connect 出新的 operation state。这样做有两个原因：

1. operation state 是一次运行的状态，完成后不能拿来再 start。
2. 上一次失败可能已经消耗 sender 内部资源，下一次需要干净状态。

G3 教学版限定 completion domain：

```text
set_value(int)
set_error(std::exception_ptr)
set_stopped()
```

这个限制让代码集中讲状态机，而不被泛型签名变换淹没。完整库会把 value 类型、error 类型和环境都泛化；G3 先把生命周期讲清楚。

operation state 内部保存：

```text
shared state(factory, receiver)
max_attempts
attempts
last_error
current inner op slot
driver flags
```

第一版常见错误是把 retry 写成普通同步循环：

```text
while not done and attempts < max_attempts:
  ++attempts
  sender = factory()
  inner_op = connect(sender, attempt_receiver{this})
  start(inner_op)
  destroy inner_op
if not done:
  set_error(last_error)
```

这段只对同步 sender 成立。真实 sender 可以在 `start(inner_op)` 返回后仍然 pending；如果此时销毁 `inner_op`，异步完成稍后会打到已经销毁的 receiver。G3 checker 用手动 pending sender 先确认 `start` 返回后 `live_ops == 1`，再手动发 error/value，避免默认触发真实 UAF。

正确骨架是一个小 driver：

```text
start:
  keep shared state alive
  drive()

set_error:
  record last_error
  ask driver to continue

drive:
  if another drive is active: mark drive_again and return
  while more work is needed:
    release completed attempt
    if stop requested: set_stopped downstream and return
    if attempts exhausted: set_error(last_error) downstream and return
    construct next attempt op
    start attempt op
    if attempt is pending: return
```

这里不能写成“失败后在 `set_error` 里直接递归调用 `start()`”。如果上游同步失败 10000 次，递归写法可能叠 10000 层调用栈。G3 checker 记录 `current_depth/max_depth`，分别运行 100 次和 10000 次同步失败，要求深度不随尝试次数增长，并且落在固定预算内。Reference 的 driver 深度是 1；独立 good 用 `exec::repeat_until` 的 trampoline 组合，深度是 2；两者都满足“有界栈”。

`set_error` 不直接通知下游；它只记录最后错误，让 driver 决定是否继续。`set_value` 和 `set_stopped` 是终态：value 成功后不再重试；stopped 表示取消或停止，也不重试。耗尽后发送最后一次错误。`max_attempts` 是总尝试次数，`3` 表示最多创建并启动三次 sender；`<= 0` 在创建 retry sender 时直接拒绝。

G3 还检查几类容易漏的资源边界。第一，pending attempt op-state 必须活到对应完成，完成后可以立刻释放，也可以随外层 op 析构释放；checker 用 `live_ops` 计数确认没有提前销毁和最终泄漏。第二，每次 connect 都必须拥有独立计数；把 attempt 计数放进共享 sender 或静态变量会污染下一次运行。第三，inner receiver 的 `get_env` 要转发下游环境，否则 stop token 传不到 pending sender。第四，terminal receiver 可以在回调里销毁外层 op；实现必须把下游 receiver 移到局部后发送 terminal，之后不再访问外层 op 成员。第五，标准 receiver 只要求可 move-construct，不要求可 move-assign；把 receiver 从共享状态取出时要用 move construction（例如 `optional::emplace`），不能写出 `optional<Receiver>` 的 move-assignment 隐含约束。bad 版本故意在 `set_error` 中递归重试，并用受控预算失败，避免真的靠爆栈暴露问题。

上游 `examples/algorithms/retry.hpp` 展示了受控重入的思路，但本题 Reference 没有直接转调 `exec::retry`。它用一个最小 `operation_slot` 存放不可移动 inner op，并用 placement-new 从 `connect(...)` 的 prvalue 直接构造，避免把不可移动 op 再移动一次。外层 op 只持有 `shared_state`；inner receiver 持有 `weak_ptr`，所以 pending 时不会形成自引用泄漏，terminal 回调销毁外层 op 时也不会留下悬空 owner 指针。这个细节也解释了为什么 sender/receiver 代码经常看起来比普通同步循环复杂：对象地址和完成时序本身就是协议的一部分。
