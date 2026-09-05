# I3 Folly coro safe task

Starter: `main.cpp` includes the real Folly headers and asks you to replace the earlier simulation.

Reference: `solution.cpp` uses `folly/coro/Task.h`, `folly/coro/BlockingWait.h`, `folly/coro/safe/SafeTask.h`, `NowTask.h`, and `AsyncClosure.h`. The runnable subset covers `value_task<int>`, `now_task<int>`, an executor-bound `Task`, `async_closure`, and `co_cleanup_safe_task`.

Build on a host with Folly v2026.08.31.00-compatible headers/libraries:

```bash
cmake -S Coroutine_Study/exercises -B build/coroutine-i3 -DCOROUTINE_STUDY_ENABLE_FOLLY=ON -DCOROUTINE_STUDY_BUILD_REFERENCE=ON
cmake --build build/coroutine-i3 --target I3_folly_safe_task_reference
ctest --test-dir build/coroutine-i3 -R I3_folly_safe_task_reference --output-on-failure
```

Observe:

- `value_task<T>` is the movable safe-task alias for value-semantic arguments and return values.
- `now_task<T>` is immovable and meant to be awaited in the full expression that created it.
- `async_closure(bind::args{...}, fn)` is the safe wrapper for less-structured capture/cleanup cases.
- `co_cleanup_safe_task<T>` is the safe-task alias designed for work that may run during closure/scope cleanup.
- Folly's safe task layer rejects unsafe argument/return aliasing at compile time; it is not a runtime convention.
- `blocking_wait` is only a top-level/test bridge. Do not call it from a real executor thread.
