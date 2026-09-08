# Coroutine_Study 质量报告

本批定点补修已获[非作者独立 APPROVE](validation/c01-independent-review.md)。审查先关闭默认测试混入Student、Release检查失效、假stopped类型、桥接完成语义冲突，再从随仓库交付的变体源重建复验。核心43项、RPC/bridge两项及Student正反验证的范围如下；没有将有限检查扩大为所有生命周期或平台的正确性证明。本段为审查后的状态回填，审查记录保留其被审原快照指纹。

核对日期：2026-09-08。范围：本轮只补 `Coroutine_Study` 内 Capstone4/Capstone5 starter 检查、CTest 接线、覆盖登记与说明文档；未修改根目录、`.omx/` 或用户本地已改的 `exercises/P2_generator_basics/main.cpp`。

## 本轮修复

| 问题 | 修复 | 证据边界 |
| --- | --- | --- |
| `mini::sync_wait` 返回 `nullopt`，把未完成伪装成 stopped | `sync_wait(task<T>)`、`sync_wait(task<void>)` 和显式 sender 占位改为抛 `logic_error` | starter 未完成时明确失败；完整语义仍由学生实现和 Reference 验证 |
| `mini::async_scope::spawn` 先递增 `in_flight_`，析构可能永久等待 | 未完成时在修改计数前抛 `logic_error` | 避免危险挂起；不代写 spawn 答案 |
| `when_all` 空 tuple 占位可能绕过学生实现 | 保留二元练习形状，运行时明确抛未实现 | 只保证检查能触达接口 |
| `as_awaitable` 占位挂起后无人恢复 | `await_suspend` 改为抛 `logic_error` | 避免测试卡死；完整 phase 语义仍看 Reference |
| Capstone5 学生测试打印 skip 后返回 0 | 测试改为实际调用 value、void、error、when_all 和非空 scope 路径；可选 stdexec bridge stopped 按 Reference 检查异常；检查使用 Release 有效的 `coroutine_study::check` | 未完成 starter 失败不标 `WILL_FAIL` |
| `sync_wait` starter 用空 `stopped_sender` 假覆盖 | 删除无协议 dummy sender；core `task` 只测 value/void/error | 直接 sender stopped 需以后定义真实 sender 协议后再测 |
| `as_awaitable` stopped 期望与 Reference 冲突 | `just_stopped()` 改为期望 `runtime_error`，与 `mini_ref/stdexec_awaitable.hpp` 和 Reference 测试一致 | 不改 Reference 契约 |
| Capstone4 starter driver 返回 0 | 改为返回 2 并打印未完成说明 | 该 target 仍可编译；行为验收看 Reference |
| CTest 缺少外部超时和标签 | 普通 Reference/runtime/Capstone5 默认 30 秒，RPC 60 秒，G2 既有 5 秒保留 | 超时是进程外保护，不替代协议正确性 |
| 默认 `verify-core` 被未完成 starter 污染 | 新增 `COROUTINE_STUDY_TEST_STARTERS`，默认 OFF，`student` preset 显式 ON | 默认 core 只跑 Reference/runtime；学生检查单独打开 |

## 构建与测试口径

- `reference` 标签：完整答案或课程运行时检查，应通过。
- `starter` 标签：学生起点检查；未完成时失败是有效信号，不作为课程通过证据；只有 `COROUTINE_STUDY_TEST_STARTERS=ON` 时注册。
- `runtime`、`rpc`、`stdexec` 标签：说明依赖或覆盖方向。
- 核心 Reference 不依赖 stdexec；stdexec bridge 只在 `stdexec::stdexec` target 存在时构建。

## 可复验命令

本轮实际运行：

```powershell
D:\cmake\install\bin\cmake.exe -S Coroutine_Study/exercises -B Coroutine_Study/exercises/build/c01-supplement -DCOROUTINE_STUDY_BUILD_REFERENCE=ON -DCOROUTINE_STUDY_FETCH_DEPS=OFF -DCOROUTINE_STUDY_TEST_STARTERS=OFF
D:\cmake\install\bin\cmake.exe --build Coroutine_Study/exercises/build/c01-supplement --config Release
D:\cmake\install\bin\ctest.exe --test-dir Coroutine_Study/exercises/build/c01-supplement -C Release --output-on-failure
```

结果：

- configure：通过，VS 2026 / MSVC 19.51.36256.0。
- build：默认 core 完整构建通过。
- `ctest --test-dir Coroutine_Study/exercises/build/c01-supplement -C Release --output-on-failure`：43/43 passed；标签汇总只有 `reference` 和 `runtime`，没有 Capstone5 starter。
- 直接运行 `Capstone4_rpc_framework.exe`：打印 compile-only/starter 说明并返回非零。

显式 starter 检查：

```powershell
D:\cmake\install\bin\cmake.exe -S Coroutine_Study/exercises -B Coroutine_Study/exercises/build/c01-supplement-starters -G "Visual Studio 18 2026" -DCOROUTINE_STUDY_BUILD_REFERENCE=OFF -DCOROUTINE_STUDY_FETCH_DEPS=OFF -DCOROUTINE_STUDY_TEST_STARTERS=ON
D:\cmake\install\bin\cmake.exe --build Coroutine_Study/exercises/build/c01-supplement-starters --config Release --target mini_task_test mini_generator_test mini_sync_wait_test mini_when_all_test mini_scope_test J1_eight_pitfalls
D:\cmake\install\bin\ctest.exe --test-dir Coroutine_Study/exercises/build/c01-supplement-starters -C Release -L starter --output-on-failure
```

结果：配置和编译通过；`ctest -L starter` 返回 8，6 项中 `J1_eight_pitfalls`、`mini_task_test`、`mini_generator_test` 通过，`mini_when_all_test`、`mini_sync_wait_test`、`mini_scope_test` 清晰打印 `starter check failed: TODO...` 后失败。

可选 Windows 全量轻依赖入口：

```powershell
D:\cmake\install\bin\cmake.exe -S Coroutine_Study/exercises -B Coroutine_Study/exercises/build/full-windows -DCOROUTINE_STUDY_TEST_STARTERS=ON
D:\cmake\install\bin\cmake.exe --build Coroutine_Study/exercises/build/full-windows --config Release --target Capstone4_rpc_framework Capstone4_rpc_framework_reference mini_reference_as_awaitable mini_as_awaitable_test
D:\cmake\install\bin\ctest.exe --test-dir Coroutine_Study/exercises/build/full-windows -C Release -R "Capstone4_rpc_framework_reference|mini_reference_as_awaitable" --output-on-failure
D:\cmake\install\bin\ctest.exe --test-dir Coroutine_Study/exercises/build/full-windows -C Release -R "mini_as_awaitable_test" --output-on-failure
```

结果：

- build：`Capstone4_rpc_framework`、`Capstone4_rpc_framework_reference`、`mini_reference_as_awaitable`、`mini_as_awaitable_test` 通过。CMake 重新生成时仅出现 stdexec 依赖内部 FetchContent deprecation dev warning。
- `ctest -R "Capstone4_rpc_framework_reference|mini_reference_as_awaitable"`：2/2 passed。
- `ctest -R "mini_as_awaitable_test"`：1/1 failed，`starter` 标签，约 0.29 秒内正常非零返回，清晰打印 `starter check failed: TODO: implement mini::sync_wait(task<T>)`；不把崩溃或超时当成这种教学失败信号。
- CTestfile 回读：G2 Reference `TIMEOUT "5"`；Capstone5 starter/reference `TIMEOUT "30"`；RPC Reference `LABELS "reference;rpc"`、`TIMEOUT "60"`。

临时检查器自证源码随 repo 放在 `Coroutine_Study/references/validation/c01-supplement/validation_variants`，只使用 shadow `mini` 头，不调用 `mini_ref` 或实际学生 TODO。CMake 通过相对路径默认定位课程根；如在非标准位置复验，也可显式传入 `-DCOROUTINE_STUDY_ROOT=<path-to-Coroutine_Study>`。构建产物仍放在 `Coroutine_Study/exercises/build/c01-supplement-validation-variants-replay`。

```powershell
D:\cmake\install\bin\cmake.exe -S Coroutine_Study/references/validation/c01-supplement/validation_variants -B Coroutine_Study/exercises/build/c01-supplement-validation-variants-replay -G "Visual Studio 18 2026"
D:\cmake\install\bin\cmake.exe --build Coroutine_Study/exercises/build/c01-supplement-validation-variants-replay --config Release
D:\cmake\install\bin\ctest.exe --test-dir Coroutine_Study/exercises/build/c01-supplement-validation-variants-replay -C Release -R "^good_" --output-on-failure
D:\cmake\install\bin\ctest.exe --test-dir Coroutine_Study/exercises/build/c01-supplement-validation-variants-replay -C Release -R "bad|nullopt|noop|error_channel|stopped_value" --output-on-failure
```

追加复验结果：`good_sync_wait`、`good_when_all`、`good_scope`、`good_bridge` 4/4 passed；`sync_wait_bad_nullopt`、`sync_wait_bad_error_channel`、`scope_bad_noop`、`when_all_bad_error`、`bridge_bad_stopped_value` 5/5 failed，分别拒绝恒定 nullopt、错误通道吞掉、noop spawn、when_all 吞错误、bridge stopped 返回值而非抛 `runtime_error`。

原始证据保存位置：`references/validation/c01-supplement/*.command.txt`、`*.stdout.txt`、`*.stderr.txt`、`*.result.txt`、`*.summary.txt`。

## 未验证边界

- 本轮不声明学生 TODO 已完成；starter 失败是预期的未完成信号。
- 本轮不新增依赖、不下载第三方、不提交、不推送。
- C01工程内容与本课协程协议分别主讲，README与覆盖表提供按需入口；全局课程建设状态另看根计划。
- 本批独立审查已通过，见文首记录；后续跨课导航和汇总报告仍纳入最终集成复验。
- 本轮运行了既有生命周期runtime/Reference检查，未运行ASan；有限测试不等于所有生命周期或并发交错的证明。Linux/io_uring/Folly/Cobalt依赖路径未验证。
