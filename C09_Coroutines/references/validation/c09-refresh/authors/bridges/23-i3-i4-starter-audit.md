I3/I4 starter audit after H checker blocker.

Scope:
- I3_folly_safe_task/main.cpp now uses varied inputs 17/25/5 and i3_trace counters. Expected starter completion requires values 17/25/7 and trace 3/2/1.
- I4_cobalt_channel/main.cpp now uses channel_trace with zero-buffer channel fixture. Expected starter completion requires writes=3, reads=3, sum=63.

Verification boundary:
- Local Windows light configure does not provide Folly or Boost.Cobalt heavy targets; no local compile/run claim is made for I3/I4.
- API shapes were kept to existing file/solution patterns already present in this repository: folly::coro::{value_task, now_task, co_cleanup_safe_task, Task, blocking_wait}; boost::cobalt::{channel, promise, gather, main} with ch.write/ch.read TODO shape.
- This note records source-level hardening only; heavy Linux owners should verify with heavy-folly-linux and heavy-cobalt-linux presets.
