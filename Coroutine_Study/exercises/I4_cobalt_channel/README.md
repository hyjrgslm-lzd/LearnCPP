# I4 Boost.Cobalt channel

Starter: `main.cpp` includes real Boost.Cobalt headers and states the exercise.

Reference: `solution.cpp` uses `cobalt::main co_main(int,char**)`, `cobalt::channel<int>`, `cobalt::gather`, and `cobalt::race`. It keeps reads count-based instead of relying on guessed close/optional behavior.

Build on a host with Boost 1.92 Cobalt:

```bash
cmake -S Coroutine_Study/exercises -B build/coroutine-i4 -DCOROUTINE_STUDY_ENABLE_COBALT=ON -DCOROUTINE_STUDY_BUILD_REFERENCE=ON
cmake --build build/coroutine-i4 --target I4_cobalt_channel_reference
ctest --test-dir build/coroutine-i4 -R I4_cobalt_channel_reference --output-on-failure
```

Observe:

- A zero-buffer channel exposes symmetric transfer clearly: writer and reader hand control to each other instead of routing through the executor queue.
- `race` reports the winning branch index in Boost.Cobalt examples; it does not mean the losing operation was automatically cancelled in every version.
- Backpressure is a scheduling contract. It is safe only while producers suspend instead of blocking the event-loop thread.
