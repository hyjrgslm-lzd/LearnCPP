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
