# I2 io_uring / IOCP awaiter

Starter: platform-specific `main_*` prints the task and compiles.

Reference:

- Windows: `windows/solution_iocp.cpp` creates a loopback TCP pair, embeds `OVERLAPPED` as the first awaiter field, waits with `GetQueuedCompletionStatus(..., 5000)`, then resumes the coroutine.
- Linux: `linux/solution_io_uring.cpp` writes a temp file, submits `io_uring_prep_read`, stores the awaiter in SQE `user_data`, calls `io_uring_cqe_seen()` before resume, and uses `io_uring_wait_cqe_timeout`.

Build one platform:

```powershell
cmake -S Coroutine_Study/exercises -B build/coroutine-i2 -DCOROUTINE_STUDY_ENABLE_IOCP=ON -DCOROUTINE_STUDY_BUILD_REFERENCE=ON
cmake --build build/coroutine-i2 --target I2_io_uring_iocp_reference
ctest --test-dir build/coroutine-i2 -R I2_io_uring_iocp_reference --output-on-failure
```

```bash
cmake -S Coroutine_Study/exercises -B build/coroutine-i2 -DCOROUTINE_STUDY_ENABLE_IO_URING=ON -DCOROUTINE_STUDY_BUILD_REFERENCE=ON
cmake --build build/coroutine-i2 --target I2_io_uring_iocp_reference
ctest --test-dir build/coroutine-i2 -R I2_io_uring_iocp_reference --output-on-failure
```

Observe: both OS APIs use the same shape: `await_suspend` submits work, the kernel later returns an identity token, the loop restores the awaiter, stores result state, and resumes exactly once.
