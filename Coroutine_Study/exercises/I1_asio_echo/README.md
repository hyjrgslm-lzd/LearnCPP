# I1 Asio awaitable echo

Starter: `main.cpp` only states the task and compiles. Fill it with an Asio echo server.

Reference: `solution.cpp` is an automated loopback check. It binds port `0`, starts `listener()` with `co_spawn`, runs a client coroutine, verifies the echoed bytes, then emits `cancellation_signal` and cancels the pending `async_accept`.

Build:

```powershell
cmake -S Coroutine_Study/exercises -B build/coroutine-i1 -DCOROUTINE_STUDY_ENABLE_ASIO=ON -DCOROUTINE_STUDY_FETCH_DEPS=ON -DCOROUTINE_STUDY_BUILD_REFERENCE=ON
cmake --build build/coroutine-i1 --target I1_asio_echo_reference
ctest --test-dir build/coroutine-i1 -R I1_asio_echo_reference --output-on-failure
```

Observe:

- `co_spawn` binds coroutine resume to `io_context`.
- `as_tuple(use_awaitable)` turns EOF/cancel into `(error_code, value)` instead of exceptions.
- `bind_cancellation_slot(stop.slot(), token)` wires external cancellation into one async operation.
