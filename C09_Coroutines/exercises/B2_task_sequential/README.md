# 练习 B-2：`task<T>` 顺序异步组合

先读 [模块 B 的 task 顺序链章节](../../03-模块B-generator与task的使用.md#b2)。本题用小型模拟流程观察父协程 `co_await` 子 task 时，值和异常怎样沿链返回。

## Part 1：三段 child task

打开 [main.cpp](main.cpp)，实现三个独立 task：

- `fetch_user(id) -> lazy_task<User>`
- `parse_profile(User) -> lazy_task<Profile>`
- `validate_profile(Profile) -> lazy_task<ValidatedProfile>`

每个 task 入口和 `co_return` 前打印日志。它们可以只是纯计算或短暂 sleep；重点是每一步都有独立协程状态和完成值。

## Part 2：父 task 串联

实现 `process_user(id)`：

```cpp
auto user = co_await fetch_user(id);
auto profile = co_await parse_profile(std::move(user));
auto result = co_await validate_profile(std::move(profile));
co_return std::format("User {} ({}) validated, score={}",
                      result.user_id, result.display_name, result.score);
```

运行时，父 task 在每个 `co_await` 点暂停；子 task 完成后，父 task 从同一行拿到返回值继续执行。日志应呈现 `fetch -> parse -> validate`。

## Part 3：异常回传

让 `fetch_user(-1)` 抛 `std::runtime_error`。参考实现 [solution.cpp](solution.cpp) 断言异常会通过第一个 `co_await` 回到父 task，后续 parse/validate 不执行，最后由 `sync_wait` 抛给 main。

画出异常路径：

```text
fetch_user throws
  -> child promise.unhandled_exception
  -> parent await_resume rethrows at co_await fetch_user
  -> parent promise.unhandled_exception
  -> sync_wait rethrows to main
```

**答案解析：** 这条链的关键点是异常回到父协程的位置：父协程暂停在 `co_await fetch_user(id)`，子 task 失败后，父 awaiter 的 `await_resume()` 在同一行重新抛出。父协程没有拿到 `User`，所以不会执行 `parse_profile` 和 `validate_profile`。如果父协程也不捕获，异常继续进入父 promise，最后由最外层 `sync_wait` 抛给 main。

## Part 4：回调对照

写一个最小嵌套 lambda 对照版本即可。对照点是：回调版本把后续逻辑拆进闭包；协程版本把后续逻辑留在线性代码里，由协程帧保存状态。

## 验收

- 正常路径文本和 reference 一致。

  **答案解析：** 正常路径应完整经过 `fetch_user -> parse_profile -> validate_profile`，最后格式化出 reference 约定的用户 id、显示名和分数文本。这个文本一致，说明每个 child task 的返回值都被父 task 正确接住并传给下一步。

- 日志顺序为 `fetch;parse;validate;`。

  **答案解析：** `process_user` 是顺序链，第一个 `co_await` 完成后才创建并等待第二段，第二段完成后才进入第三段。即便底层 task 是协程，父协程的代码顺序仍约束了启动顺序。日志若出现 `parse` 早于 `fetch` 完成，就说明组合写成了并发或提前启动。

- `fetch_user` 抛异常时，后续 task 不执行。

  **答案解析：** `fetch_user(-1)` 的异常在父协程第一个 `co_await` 的 `await_resume()` 处重新抛出。父协程没有得到 `User`，自然无法调用 `parse_profile`；异常进入父 promise 后等待外层消费。检查 parse/validate 日志缺失，比只看 main 捕获异常更能证明短路位置正确。

- 你能说明每个 `co_await` 是父协程的暂停点，也是值/异常回到父协程的位置。

  **答案解析：** 父协程执行到 `co_await child_task` 时保存当前局部状态并挂起；child 完成后恢复父协程。正常时 `await_resume()` 返回 child 的值，异常时它重新抛出 child 保存的异常。于是同一行代码既是离开父协程的位置，也是父协程继续或失败的位置。
