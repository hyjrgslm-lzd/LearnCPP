# F01_native: standard `std::execution` facility probes

This unit keeps the standard path separate from `stdexec`. The probe sources in
`probes/` use `std::execution` and `std::this_thread::sync_wait` directly. They
do not alias to `stdexec`.

Each facility has two stages:

1. a compile probe in `probes/`;
2. a separate observation body in `body/` that runs only that facility.

Probe failure skips only that facility. A baseline failure is a configure
failure because no standard-mode result would be trustworthy.

- `baseline`: the compiler accepts the requested mode for a trivial program.
- `core`: `just`, `then`, `when_all`, and `sync_wait`.
- `scheduler`: `scheduler`, `inline_scheduler`, `schedule`, and `sync_wait`.
- `task`: `std::execution::task` and `co_return`.
- `scope`: `counting_scope`, `spawn`, and `join`.
- `bulk`: `bulk` with an execution policy.

Raw compile output is written under the build directory at
`F01_native/probe-output/*.log`.
## IDE 工程入口

启动目标是 F01_native_core/scheduler/task/scope/bulk 各观察目标；probes 只做能力探针。 本目录的 README/CMakeLists 和脚本显示在主项目中；观察程序通过只证明对应运行检查，不代替书面预测、源码阅读或性能归因。单题可用 `cmake -S <本目录> -B <build>` 独立生成。
