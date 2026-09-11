# C09 H/I author slice

Owner: bridges slice, 2026-09-11.

## Scope

- H docs: `10-模块H-协程与sender_receiver桥接.md`.
- I docs: `11-模块I-真实异步IO与并发框架.md`.
- Source route: `15-源码阅读路线.md`.
- Standard index: `references/标准条款与版本状态.md`.
- Exercises: H1/H2/H3/I1/I2/I3/I4/I5 local CMake and starter checks.

No shared include, top-level CMake, AddExercise, ThirdPartySetup, RPC, Capstone4/5, commit, push, install, or dependency upgrade in this slice.

## Unit Audit

| Unit | Change | Check meaning |
| --- | --- | --- |
| H1 | Starter now runs a coroutine that awaits two stdexec senders and expects 52. | TODO bridge returns `-999`, completed bridge naturally passes. |
| H2 | Starter calls `run_task_observations(pool.get_scheduler())` and checks all five observations. H2 std probe now compiles a C++26 preview `std::execution::task<int>` coroutine body and registers as probe/SKIP77. | Empty observation struct fails; stdexec reference passes independently. Reference target/test are guarded by `COROUTINE_STUDY_BUILD_REFERENCE`. |
| H3 | Starter now has a compilable failing `my_task` stub and checks sender, nested task await, and sender-to-awaitable bridge. | It consumes student `my_task` through `connect/start`, `outer()`, and `bridge_side()`. |
| I1 | Starter performs a 250ms bounded loopback echo. | No accept/echo implementation fails without hanging. |
| I2 | Starter rejects ready-fast-path awaiters; docs define read/complete/resume/close phases. | Windows starter fails structurally; Windows reference passes real IOCP; Linux reference passes real io_uring. |
| I3 | Starter calls Folly TODO functions when target is enabled. | Current Windows light lane does not enable Folly; docs mark missing dependency as unverified, not pass. |
| I4 | Starter enters Cobalt gather when target is enabled and fails without channel work. | Current Windows light lane does not enable Cobalt; docs mark missing dependency as unverified, not pass. |
| I5 | Starter calls cppcoro generator/task/shared_task/cancel/scheduler TODO functions. | Empty implementations fail; reference passes cppcoro task/shared_task/generator/when_all/scheduler. |

## Version Notes

- WG21 checked from primary sources on 2026-09-11: N5050 is the C++26 DIS basis via N5051; N5054 is the C++29 working draft via N5055.
- H docs and standard index keep `stdexec::`/`exec::` separate from future `std::execution`.
- Local stdexec cache needed `execution.bs` copied into author build directories because the disconnected configure created a zero-byte file before failing. This is a public configure helper gap; this slice did not edit the helper.

## Evidence

- `01-reference-ctest-release.json`: Windows Release reference CTest, H1/H2/H3/I1/I2/I5, PASS.
- `03-starter-ctest-expected-fail.json`: Windows Release starter CTest, H1/H2/H3/I1/I2/I5, expected CTest exit 8 with six behavior failures, PASS.
- `05-h2-std-task-probe.json`: Windows `std::execution::task<void>` probe reports unavailable, PASS.
- `07-linux-i2-ctest-reference-and-starter.json`: WSL I2, starter expected failure plus real io_uring reference PASS.
- `09-h2-cxx26-probe-ctest-skip.json`: Windows C++26 preview std task coroutine probe registers as `probe` and SKIPs through return 77 because `<execution>` lacks task body support.
- `11-h2-referenceoff-showonly-after-probe.json`: ReferenceOFF fresh student testlist captured after H2 guard/probe fix.

PowerShell parse of that testlist for this slice:

```text
H/I+probe tests=7 refs=0 probe_labels=probe
```

Full top-level ReferenceOFF testlist still contains eight `runtime_*_reference_test` entries from shared `runtime_tests`, outside this slice and not edited here.

Mis-recorded expected exit files are retained:

- `02-starter-ctest-expected-fail.json`: expected exit 1 but CTest returned 8.
- `04-h2-std-task-probe.json`: probe exe not built yet.
- `06-linux-i2-ctest-reference-and-starter.json`: expected exit 1 but CTest returned 8.
- `08-h2-referenceoff-showonly.json`: old show-only before C++26 probe registration change.
- `10-h2-referenceoff-showonly-after-probe.json`: recorder command malformed by quoted `--contains`.
