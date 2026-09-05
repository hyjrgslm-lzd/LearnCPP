# I5 cppcoro patterns

Starter: `main.cpp` states the porting task.

Reference: `solution.cpp` maps this repository's hand-written runtime ideas to maintained cppcoro primitives:

- `task<T>`: lazy single-consumer result.
- `shared_task<T>`: multi-consumer cached result.
- `generator<T>`: synchronous pull sequence.
- `sync_wait`: top-level bridge from `main`.
- `when_all`: structured fan-out/fan-in.
- `static_thread_pool::schedule`: explicit scheduler handoff.
- `cancellation_source` / `cancellation_token` / `cancellation_registration`: cooperative cancellation and callback delivery.

Build:

```powershell
cmake -S Coroutine_Study/exercises -B build/coroutine-i5 -DCOROUTINE_STUDY_ENABLE_CPPCORO=ON -DCOROUTINE_STUDY_FETCH_DEPS=ON -DCOROUTINE_STUDY_BUILD_REFERENCE=ON
cmake --build build/coroutine-i5 --target I5_cppcoro_patterns_reference
ctest --test-dir build/coroutine-i5 -R I5_cppcoro_patterns_reference --output-on-failure
```

Observe: cppcoro packages the same mechanisms you built earlier. `task` owns a coroutine frame, `sync_wait` starts the top-level coroutine, `when_all` starts siblings, `schedule()` resumes the awaiting coroutine from the pool, and cancellation is an explicit token channel rather than magic global state.
