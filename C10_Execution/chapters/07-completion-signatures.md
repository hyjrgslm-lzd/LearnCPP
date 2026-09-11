# 07 completion signatures：完成通道的类型级合同

sender 不直接返回值。它声明自己将来可能调用哪些完成函数：`set_value_t(Args...)`、`set_error_t(Error)`、`set_stopped_t()`。这些函数类型组成 `completion_signatures<...>`，形成 sender 和 receiver 之间的类型级合同。

`set_value_t(int, std::string)` 表示 value channel 会带两个值；`set_value_t()` 表示成功但没有值；`set_error_t(std::exception_ptr)` 表示错误通道；`set_stopped_t()` 表示停止通道。这里的 `set_value_t(Args...)` 是函数类型写法，`set_value_t` 本身是 tag 类型，不是 `set_value_t<Args...>` 类模板。这个细节很小，但会直接影响 `void`：非依赖的 `set_value_t(void)` 可以表示无参数函数，依赖参数替换成 `void` 时却容易形成非法参数类型或跨编译器差异。F2 和 G1 都用独立 `void` 特化生成 `set_value_t()`，不把 `void` 当成一个值发下去。

adaptor 的核心工作是签名变换。`then(f)` 只改 value channel：

```text
set_value_t(Args...) -> set_value_t(invoke_result_t<F, Args...>)
set_value_t(Args...) -> set_value_t()       // F 返回 void
set_error_t(E)       -> set_error_t(E)
set_stopped_t()      -> set_stopped_t()
```

教学版还保守加入 `set_error_t(std::exception_ptr)`，因为 `f` 可能抛异常。生产实现会继续收窄 `noexcept`、domain、completion scheduler 和函数对象值类别；本课先让读者能手动推导每个通道。

环境参数不能丢。`completion_signatures_of_t<S, Env>` 里的 `Env` 可能改变上游 sender 的签名：同一个 sender 在不同 scheduler、stop token 或自定义 query 下能声明不同 value 类型。G1 的 `env_signature_sender` 专门验证这一点；F2 的简化模型先不接环境，正文必须说明这是教学裁剪，不是标准协议完整形态。

Concept 检查把签名合同接到 receiver 能力上。`receiver_of<R, Sigs>` 不是“R 有 receiver_tag”这么简单；它要能接住 `Sigs` 里的每个 `set_value_t`、`set_error_t` 和 `set_stopped_t`。F3 的 bad 版本故意只检查语法 tag，不检查 value 参数，所以会把只能接 `std::string` 的 receiver 错接到声明 `set_value_t(int)` 的 sender 上。

练习映射：

- F1：实现最小 `type_list`，连接、去重、映射、过滤 completion 类型集合。
- F2：解析和变换 completion signatures，独立处理 `void` value。
- F3：区分 concept 的语法满足和语义约束，完成 sender/receiver/receiver_of/sender_to 的最小检查。
