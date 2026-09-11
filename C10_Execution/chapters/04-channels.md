# 04 完成通道：value、error、stopped

sender/receiver 不把失败和取消塞进返回值旁边的小标志里，而是把一次执行的结局分成三条 completion channel：`set_value`、`set_error`、`set_stopped`。这三条都是终结信号。一个 operation state 被 `start` 后，最多只能发出其中一条；信号发出后，下游 receiver 可以销毁整条 operation state，所以发送方不能再读写自身状态。

## 背景

同步代码常写成：

```cpp
try {
  auto value = parse(input);
  use(value);
} catch (...) {
  recover();
}
```

sender 图中，`parse` 不一定在当前线程执行，也不一定马上执行。异常不能靠最外层到处包 `try/catch` 来表达，因为那会把图内部的恢复路径挪到图外，组合器也无法知道失败后还能不能继续。正确模型是：某个阶段失败时发 `set_error(e)`，恢复 adaptor 再把 error channel 转回 value channel，或者返回一条新的 sender 继续恢复。

`stopped` 又是另一件事。停止请求只表示“请尽量别继续做无意义的工作”，不是已经完成，也不是出错。只有某个 operation 明确发出 `set_stopped()`，这次执行才走 stopped completion。请求、观察请求、最终完成三件事必须分开讲。

## 错误通道

`upon_error` 是值级恢复：收到 error 后运行一个函数，并把函数返回值作为新的 value 发给下游。它适合把异常转成统一结果，例如 `parse_result{ok=false}`。

`let_error` 是 sender 级恢复：收到 error 后运行一个函数，这个函数返回新的 sender。它适合需要继续异步工作的恢复路径，例如记录日志、重新读缓存、重试一次，然后再发结果。

反例是把 error 当 value 伪造成功：

```cpp
void set_error(std::exception_ptr) && noexcept {
  set_value(std::move(downstream), fallback_success);
}
```

如果语义明确是恢复，这样可以；如果 adaptor 的职责只是转发或观察，它就破坏了协议。D13 的 `tap` 因此必须原样转发 error。C1_7 的恢复题则故意要求把 error 转成统一失败结果，并再验证“恢复后的下一段仍然可能失败”。

## 停止通道

`stop_token` 只表达请求。一个循环可以每步检查 token，看到请求后发 `set_stopped()`；也可以忽略请求继续完成，这在协议上不是 data race，也不是自动异常。取消是合作式的。

C1_8 固定三个窗口：

- cancel before start：`start` 一进入就看到请求，发 `set_stopped()`。
- cancel during work：受控步骤 2 请求停止，下一次检查发 `set_stopped()`。
- cancel after completion：先发 `set_value(3)`，外层再请求停止。已完成的 operation 不会被改写成 stopped。

最后一点很重要：完成信号之后不得再触碰可能已销毁的 operation state。本课检查器把 after-completion 请求放在 `start` 返回后做，避免用真实 UB 当教学实验。

## Part 对应

C1_7 练 value/error 合流：正常输入保持 value；非法输入经恢复得到统一失败结果；恢复后的阶段再抛异常时仍按新的 error path 处理。

C1_8 练 stopped：停止请求、合作检查、最终 stopped completion 分开验证。bad 版本会漏掉 during-work stopped，检查器必须拒绝。

D11、D12、D13 回到协议层：receiver 要接三条 channel；sender/opstate 要只发一次终结；adaptor 只改变它声明负责的 channel，其余 channel 必须透传。

## 答案解释

C1_7通过真实stdexec组合器完成恢复；C1_8与D11–D13用手动receiver记录事件、自写sender在start内发送完成信号，分别承担使用与协议实现的教学责任。检查器核对实际状态和值，不靠成功文案或sleep猜调度。bad版本均可编译，分别破坏恢复结果、合作停止或完成通道。

这个章节的最小判断是：如果你遮住答案，仍能说出“谁发起请求、谁观察请求、谁发 completion、completion 后谁还可以访问哪些对象”，就掌握了完成通道的核心。

## C1_7的实际组合与完整消费

本课实现使用固定库的真实adaptor，不在使用阶段手写另一套同步upon_error/let_error。这样读者先学清接口语义，G1再进入adaptor内部结构。解析函数本身可以抛普通异常；`then`是把它接入完成协议的边界。

```cpp
auto value_recovery = parse_graph(text)
  | stdexec::upon_error([](std::exception_ptr) {
      return parse_result{false, "BadRequest", -1};
    });
auto sender_recovery = parse_graph(text)
  | stdexec::let_error([](std::exception_ptr) {
      return stdexec::just(parse_result{false, "RecoveredBySender", -2});
    });
```

第一个回调产出值，第二个产出新sender。若把返回sender的回调用在upon_error，得到的是携带sender对象的value，而不是自动启动该工作；这正是let_error存在的原因。新的operation state必须活到恢复工作完成，不能在回调里创建局部op、start后立刻销毁。

错误只沿图向下游传递。上游upon_error已完成后，后续then再次抛异常，不会返回到过去的恢复节点。检查器明确覆盖这个顺序。所有字段完整消费后才算解析成功，避免`stoi("12garbage")`只消费前缀造成假成功。独立good用经典locale的stream检查，Reference用from_chars；两者验证同一明示语法，且不用同一完整解析实现。
