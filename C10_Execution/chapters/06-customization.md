# 06 customization：Execution 定制点从 ADL 到 member dispatch

C04 已经主讲名字查找、ADL、hidden friend、requires 和 CPO 的通用语言规则。本章只讲 C10 需要的执行协议：`connect`、`start`、`set_value`、`set_error`、`set_stopped`、`get_env` 这些名字为什么必须是受控定制点，以及当前课程代码为什么采用 member dispatch。

裸 ADL 的问题不是“它总是错”，而是候选集合太大。`greet(x)` 这种未限定调用会把实参类型相关命名空间里的函数都拉进来；如果第三方类型又继承或包装了你的类型，同名函数可能先被选中。Execution 的完成信号是协议边界，不能让无关命名空间里的同名函数决定一个 operation state 怎样完成。

CPO 把调用入口变成对象，例如 `connect(sender, receiver)` 实际是调用 `connect_t::operator()`。对象名本身不参与 ADL；实现者可以在对象内部明确写优先级：先看成员函数，再看旧式 `tag_invoke`，最后给出清晰约束失败。Ranges 里的 niebloid 是 CPO 家族的一种称呼，但不要把所有 CPO 都叫 niebloid。Execution 的 `connect_t`、`start_t`、`set_value_t` 是 CPO；它们和 Ranges niebloid共享“对象入口、抑制非预期 ADL”的设计动机，不共享全部库语义。

`tag_invoke` 是旧一代通用扩展实验：所有定制都走一个 ADL 名字，第一参数是 tag。它比每个协议一个 ADL 函数名更集中，也便于 hidden friend 定制；缺点是样板多，诊断容易绕，读代码时先看到的是 `friend tag_invoke(connect_t, ...)`，不是对象本身的协议成员。E2 保留这个历史实验，帮助读者看懂旧 stdexec/libunifex 风格代码，但它不是本课后续实现的新要求。

当前 stdexec `nvhpc-26.05` 已经支持更接近标准方向的 member dispatch。本课新代码采用这条线：sender 写 `connect(receiver)`，operation state 写 `start()`，receiver 写 `set_value(...)` / `set_error(...)` / `set_stopped()` / `get_env()`。E3 对比 member-first 与 legacy fallback：拥有类型源码时成员函数最直接；无法修改第三方类型时才需要外部适配层。G1 `my_then` 正是用这个思路解释真实 adaptor：外层 sender 的 `connect` 生成包装 receiver，包装 receiver 的成员完成函数改写或透传 completion channel。

定制点的约束不只检查“有这个名字”。`connect` 的结果必须是 operation state；`start` 必须能对左值 operation state 调用，并且后续章节要求 `noexcept`；receiver 的完成函数必须能接住 sender 声明的每一种 completion signature。语法满足只是入口，语义还包括惰性、每次 connect 独立、start 一次、接受工作恰好一次终结、完成后不得访问可能已销毁的 operation state。

练习映射：

- E1：从裸 ADL 到 CPO，分清 CPO 与 niebloid 的边界。
- E2：实现历史 `tag_invoke` 协议，理解旧代码和 hidden friend 查找。
- E3：固定 member-first 与 legacy fallback 对照，解释当前课程主协议。
