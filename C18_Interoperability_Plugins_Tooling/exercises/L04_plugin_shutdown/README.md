# L04 plugin shutdown

实现一个最小插件 session：

- 通过 `c18_get_api` 取得 C11 ABI 函数表。
- `process` 把 ASCII `a-z` 转成 `A-Z`，其他字节保持原样。
- small buffer 必须返回所需长度，不写业务输出，不推进关闭状态。
- callback 期间只能 request stop；session 不得持锁进入任何插件 callout。
- `close` 进入 closing 后拒绝新调用，等 active call/callback/borrow 归零，再 destroy 并释放本次 loader handle。
- `destroy` 或 loader release 失败时必须保留可重试所有权；若 context 已 destroy 但 release 失败，重试时不能再次 destroy 旧 context。

Checker 覆盖四段：

1. 缺入口、版本拒绝、入口错误、转换、空输入、small buffer。
2. 同步 callback 内调用 `request_stop`，证明 host 未持 session 锁进入插件。
3. 受控并发：`process` 进入 callback 栅栏，主线程 `request_stop`，再启动 `close`；closing 必须拒绝新 `process`，`close` 必须等 callback 放行后才返回并 destroy。
4. `destroy` 返回 `C18_STATUS_BUSY` 和模拟 loader release 失败都必须可 retry。

`student/solution.hpp` 只给安全骨架，应该能编译并真实失败。`reference` 是完整答案；`validation/good` 是独立好实现；`validation/bad` 是真实坏实现，能正常加载和处理，但 `close` 不等待 callback drain，同一 checker 必须用 `check failed: close returned before callback drained` 拒绝它。崩溃、timeout 或其他输出都不算正确拒绝。
