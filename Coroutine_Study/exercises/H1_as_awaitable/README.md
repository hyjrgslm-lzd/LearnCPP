# H-1 as_awaitable 桥

目标：把 stdexec sender 适配成 awaitable。参考答案覆盖三条 completion channel：
`set_value` 返回值，`set_error` 重新抛异常，`set_stopped` 抛 `stopped_error`。

关键验收：

- `operation_state` 是 awaitable 成员，活到 `await_resume`。
- sender 在 `start()` 内同步完成也安全：receiver 只记录完成；`await_suspend`
  用原子握手决定返回 `false` 继续当前协程，避免 reentrant resume。
- receiver 的 `get_env` 当前返回 `empty_env`；进阶版再转发 scheduler/stop_token。

命令：

```powershell
cmake -S Coroutine_Study/exercises -B Coroutine_Study/exercises/build-h -G Ninja -DCOROUTINE_STUDY_ENABLE_STDEXEC=ON -DCOROUTINE_STUDY_FETCH_DEPS=ON -DCOROUTINE_STUDY_BUILD_REFERENCE=ON
cmake --build Coroutine_Study/exercises/build-h --target H1_as_awaitable_reference
ctest --test-dir Coroutine_Study/exercises/build-h -R H1_as_awaitable_reference --output-on-failure
```
