# F2 completion signatures 解析与变换

实现简化版 `completion_signatures` 解析，手动推导 `then(f)` 的输出签名。

Part:

1. `set_value_t(Args...)`、`set_error_t(Error)`、`set_stopped_t()` 是函数类型签名；tag 本身不是类模板。
2. `value_signatures_t<Sigs>` 提取 value channel。
3. `error_signatures_t<Sigs>` 提取 error channel。
4. `sends_stopped_v<Sigs>` 检测 stopped channel。
5. `then_completion_signatures_t<Sigs, Fn>` 改写 value，保留 error/stopped。
6. `Fn::result<Args...> == void` 时必须生成 `set_value_t()`，不能发送 `void` 值。

bad 版本永远把 value 改成 `std::string`，能骗过非 void 用例，但会被 void 用例拒绝。

Student 初态以 `value channel signatures extracted` 作为首个实际类型检查失败，预期 exit 1；这表示类型结果错误，不使用完成 flag。
