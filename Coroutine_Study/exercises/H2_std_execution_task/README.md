# H-2 std::execution::task / stdexec task

当前工作草案的 `[exec.task]` 描述的是标准库未来的 coroutine task 类型。
本练习用 pinned NVIDIA/stdexec `nvhpc-26.05` 的 `stdexec::task` 观察同类契约；
它不是 `std::execution::task`，也不应写成标准库类型。

## 验收点

- `H2_std_task_probe` 由 CMake `check_cxx_source_compiles` 真实编译
  `std::execution::task<void>` 后生成宏，不只看 feature-test macro。
- reference 只在支持工具链构建：MSVC，或 Linux + GNU。其他工具链不创建
  `H2_std_execution_task_reference` target，也不注册通过状态的 CTest。
- reference 运行时检查：
  - `stdexec::task<int>` 能 `co_await` sender，并作为 sender 被 `sync_wait` 消费。
  - `exec::static_thread_pool` + `starts_on` 产生可观察线程切换。
  - `when_all` 汇合两个调度到 worker 的 sender。
  - `just_stopped` 通过 `stopped_as_optional` 变成 `std::optional` 空值。
  - `get_stop_token` 能从 task environment 读到可用 stop token。

## 命令

```powershell
cmake -S Coroutine_Study/exercises -B Coroutine_Study/exercises/build-h-release -G "Visual Studio 18 2026" -DCOROUTINE_STUDY_ENABLE_STDEXEC=ON -DCOROUTINE_STUDY_FETCH_DEPS=ON -DCOROUTINE_STUDY_BUILD_REFERENCE=ON
cmake --build Coroutine_Study/exercises/build-h-release --config Release --target H2_std_task_probe H2_std_execution_task_reference
ctest --test-dir Coroutine_Study/exercises/build-h-release -C Release -R H2_std_execution_task_reference --output-on-failure
```

如果使用 MinGW/GCC + Windows，CMake 会构建 starter 和 probe，但不会创建 reference
target；这是工具链支持声明，不是运行时成功。
