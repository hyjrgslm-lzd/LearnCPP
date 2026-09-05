# H-3 双向桥接

目标：`my_task<T>` 同时是 sender 和 awaitable。

三个必须跑通的路径：

- `stdexec::connect(my_task<int>, receiver)` + `stdexec::start(op)`：走 sender 协议，`connect` 生成 op_state，
  `start` 恢复协程，`final_suspend` 通知外部 receiver。
- `co_await my_task<int>`：走 awaitable 协议，`await_suspend` 保存 caller，
  然后 symmetric transfer 到被等待 task。
- `my_task` 协程体内 `co_await stdexec::just(42)`：走 `await_transform`，
  复用 H-1 sender -> awaitable 桥。

关键验收：

- `final_suspend` 使用精确的 `std::coroutine_handle<promise_type>`，不要用
  `from_address` 把派生 promise 伪装成基类 promise。
- `external_receiver` 只服务 sender 路径；`continuation` 只服务 awaitable 路径。
- `completion_signatures` 与真实 completion 一致：value/error/stopped 都声明。
- pinned stdexec 的 sender opt-in 写 `using sender_concept = stdexec::sender_tag`。
- `my_task` 不直接暴露通用 awaiter 三件套；只在自己的 `promise_type::await_transform`
  中返回专用 awaiter，避免 stdexec 把它误判为普通 awaitable sender。

命令：

```powershell
cmake -S Coroutine_Study/exercises -B Coroutine_Study/exercises/build-h -G Ninja -DCOROUTINE_STUDY_ENABLE_STDEXEC=ON -DCOROUTINE_STUDY_FETCH_DEPS=ON -DCOROUTINE_STUDY_BUILD_REFERENCE=ON
cmake --build Coroutine_Study/exercises/build-h --target H3_bidirectional_bridge_reference
ctest --test-dir Coroutine_Study/exercises/build-h -R H3_bidirectional_bridge_reference --output-on-failure
```
