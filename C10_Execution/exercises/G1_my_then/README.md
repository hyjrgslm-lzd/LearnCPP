# G1 my_then：教学版 sender adaptor

本题实现一个教学版 `my_then(sender, f)`。它拦截上游 `set_value(args...)`，调用 `f(args...)`，再把返回值发给下游；`set_error` 和 `set_stopped` 必须原样透传。完整讲解见 [08-adaptor.md](../../chapters/08-adaptor.md)。

## 编辑位置

- 学生只改 `src/student/solution.hpp`。
- `main.cpp` 是共同 checker，不为 Student/Reference 改条件。
- `src/reference/solution.hpp` 是完整参考实现。
- `validation/good/solution.hpp` 是独立 oracle，使用 `stdexec::then`。
- `validation/bad/solution.hpp` 是可编译的错误实现：它把上游 error 错发成 value，必须被 checker 拒绝。

## Part

1. 定义 `my_then_sender<InnerSender, F>`：保存上游 sender 和函数对象，声明 `sender_concept`，按环境推导 completion signatures。
2. 定义 `my_then_receiver<DownstreamReceiver, F>`：在 `set_value` 中调用 `F`，处理非 `void`/`void` 结果，并把异常转成 `set_error(std::exception_ptr)`。
3. 透传 `set_error`、`set_stopped` 和 `get_env`。
4. 定义不可移动的 operation state，构造时 `connect` 上游，`start()` 必须 `noexcept` 并只委托内层 op-state。
5. 保持 `connect` 构造失败为普通异常传播；不要在 `start()` 里重新连接。

## 构建与运行

使用固定 revision `6d7ad689f4d4831c5136e4abe1c601f9a3b64e43` 的 stdexec checkout。把 `$env:STDEXEC_ROOT` 指向这个 checkout，不要求使用作者机器上的 C09 build 缓存。

```powershell
$env:STDEXEC_ROOT = '<固定checkout>'
cmake -S C10_Execution\exercises\G1_my_then -B build\c10-sample-author -DFETCHCONTENT_SOURCE_DIR_STDEXEC=$env:STDEXEC_ROOT
cmake --build build\c10-sample-author --config Debug --target G1_my_then_reference G1_my_then_validation_good G1_my_then_validation_bad G1_my_then_student
ctest --test-dir build\c10-sample-author -C Debug --output-on-failure
```

期望：

- `G1_my_then_reference` PASS。
- `G1_my_then_validation_good` PASS。
- `G1_my_then_validation_bad_rejected` PASS，含诊断 `error channel forwarded as set_error`。
- `G1_my_then_student` 初始实现可编译，运行输出 `UNFINISHED: G1 my_then: implement sender adaptor`，进程码为 `2`。

## Checker 覆盖

`main.cpp` 覆盖：

- 零输入值、单值变换、类型变化、多输入值。
- `void` 返回。
- move-only 值。
- transform 抛异常转 `set_error`。
- 上游 `set_error` 透传。
- 上游 `set_stopped` 透传。
- receiver 环境通过 inner receiver 转发。
- completion signatures 随环境变化：同一个上游 sender 在 `int_signature_env` 下发 `int`，在 `string_signature_env` 下发 `std::string`，`my_then` 必须分别推导成 `long` 和 `std::size_t` 路径。
- operation state 不可移动、`start` 为 `noexcept`。
- `connect` 阶段构造失败不被吞掉。
- completion signatures 能在 `stdexec::env<>` 下实例化。

## 上游对照

固定上游源码：

```text
revision: 6d7ad689f4d4831c5136e4abe1c601f9a3b64e43
example: https://github.com/NVIDIA/stdexec/blob/6d7ad689f4d4831c5136e4abe1c601f9a3b64e43/examples/algorithms/then.hpp
implementation: https://github.com/NVIDIA/stdexec/blob/6d7ad689f4d4831c5136e4abe1c601f9a3b64e43/include/stdexec/__detail/__then.hpp
local relative files: examples/algorithms/then.hpp, include/stdexec/__detail/__then.hpp
```

本题 Reference 没有调用 `stdexec::then`。它使用固定版本支持的成员函数 dispatch（`receiver.set_value/error/stopped/get_env`、`op.start`、`sender.connect`）和三对象教学骨架，并用小型自写 meta 函数变换 `stdexec::completion_signatures<...>`，补上 `void`、多值、异常和环境相关签名路径。
