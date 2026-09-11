# G3 retry：有界重试 sender_factory

本题实现 `c10_g3::retry(factory, max_attempts)`。`factory` 每次尝试创建一个新 sender；`max_attempts` 是总尝试次数，不是额外重试次数。

教学版类型域固定为：`set_value(int)`、`set_error(std::exception_ptr)`、`set_stopped()`。重点是状态归属和启动方式：每次 `connect` 都有自己的计数，`start()` 用一个受控 driver 推进下一次尝试，不能靠递归 `start` 在大量同步失败时爆栈。

Part：

1. `max_attempts <= 0` 在创建 sender 时明确拒绝。
2. 每个 attempt 的 sender 由 `factory()` 新建。
3. value 成功后停止重试并向下游发 value。
4. error 记录最后错误；未耗尽则继续，耗尽后发最后错误。
5. stopped 不重试，直接透传。
6. attempt op-state 在 pending 时必须保持存活；完成后才能释放，且两次 connect 不能共享尝试计数。
7. `get_env` 必须把下游环境转给 inner receiver，使 stop token 能穿过 retry。
8. 下游 terminal receiver 可以在回调里销毁外层 op；发出 terminal 后实现不能再访问外层 op 成员。

checker 覆盖同步失败、手动 pending、异步 error 后重试、跨线程 error、stop-before-start、pending cancel、terminal 回调销毁外层 op、receiver 只可 move-construct 不可 move-assign、factory/connect 抛异常、100 和 10000 次同步失败栈深度不增长。bad 版本故意在 `set_error` 中递归重试，并用 64 层预算受控失败；当前 checker 以 `recursive retry budget exceeded` 拒绝它。
