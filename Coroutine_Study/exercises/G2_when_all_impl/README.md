# G-2 实现 `when_all`

对应正文：[09 模块 G](../../09-模块G-symmetric_transfer与高级task.md#g2)。

`when_all` 是 barrier。两个子 task 都要先启动；最后一个完成时恢复父协程；若有异常，等两个分支都收束后传播首个异常。

## Part 1：从顺序 drain 改成 fan-out

目标流程：

```text
parent co_await when_all_2(a, b)
  -> await_suspend 保存 parent
  -> 创建 left/right runner
  -> start left
  -> start right
  -> parent 挂起
```

如果代码先完整等待 a，再启动 b，它只能证明顺序组合，不能证明 `when_all`。

## Part 2：runner 保存结果

每个 runner 只负责自己的子 task：

```text
try co_await child
  -> 写入对应 optional<T>
catch
  -> 若 first_error 为空，保存 current_exception
final_suspend callback
  -> remaining--
```

结果槽、错误和 remaining 要在同一个 operation 的同步保护下更新。reference 使用 mutex。

## Part 3：最后一个完成恢复 parent

`remaining` 从 2 减到 0 时，completion callback 返回 parent handle。父协程随后进入 `await_resume()`，有 error 就 rethrow，否则返回 tuple。

Reference 用 latch/barrier 证明两个分支先全部启动，释放前都没完成，最后一个分支完成后父协程只恢复一次。

## 验收

- `when_all_2(fetch_int(), fetch_string())` 返回正确 tuple。

  **答案解析：** 每个 runner 等待自己的 child task，并把结果写进 operation 内的对应 `optional` 槽。两个 runner 都完成后，父协程进入 `await_resume()`，把两个槽组合成 tuple 返回。reference 的二元实现固定返回 `(left_value, right_value)`，顺序来自传入参数顺序。
- 任一分支失败时，另一个分支仍收束，最后传播首个异常。

  **答案解析：** fail-delay 策略在 runner 捕获异常时只保存首个 `exception_ptr`，不立即恢复父协程。remaining 仍要等两个分支都完成，避免另一个分支还在访问 operation 时父协程返回并销毁它。G-2 的 error 场景用 `completed` 计数证明异常传播发生在两个分支收束之后。
- 同步完成的子 task 不会让父协程挂死。

  **答案解析：** 子 task 可能在 `await_suspend` 启动阶段同步完成，使 remaining 在 `await_suspend` 返回前降到 0。实现要在启动后检查 remaining，并在已经完成时返回 false，让父协程立即继续。reference 用 `parent_sync()` 验证两个同步分支不会留下无人恢复的父协程。
- 你能画出 `remaining: 2 -> 1 -> 0 -> resume parent`。

  **答案解析：** `remaining` 是 barrier 的核心状态。每个 runner 的 final completion 递减一次；第一个完成只记录状态，最后一个把 remaining 减到 0 并返回 parent handle。图里要标出 parent 只恢复一次，以及结果/错误在 `await_resume()` 统一消费。

## Reference

Reference 验证 fan-out/fan-in、同步完成、并发完成、异常在全部分支收束后传播。

```powershell
cmake -S . -B build/dg-lane -DCOROUTINE_STUDY_BUILD_REFERENCE=ON
cmake --build build/dg-lane --config Release --target G2_when_all_impl G2_when_all_impl_reference
ctest --test-dir build/dg-lane -C Release -R G2_when_all_impl_reference
```
