# 练习 J-1：八大经典陷阱重现

知识讲解：[J1 对应章节](../../12-模块J-陷阱诊断与跨编译器.md#j1)。

对应主讲义：`12-模块J-陷阱诊断与跨编译器.md` 的 J-1。

这题练的是协程生命周期诊断。默认 starter/reference 只跑安全路径；危险路径以注释或 `COROUTINE_STUDY_ENABLE_UNSAFE_DEMOS` 保护。先读机制，再跑 reference，再挑 1-2 个陷阱单独打开 unsafe 观察。

八类陷阱按排错关键词记：

- 引用捕获：外部栈对象没有进入协程帧。
  **答案解析：** lambda 或函数返回 task 后，按引用捕获的对象仍由外部栈帧拥有。协程稍后恢复时外部栈帧可能已经结束，安全修复是按值捕获或把共享状态移动进协程帧。
- 临时派生指针：awaiter 活着，指针指向的对象可能死了。
  **答案解析：** `c_str()`、`data()`、`string_view`、span 这类对象经常只借用底层存储。awaiter 跨挂起存活只能保证 awaiter 自己没死，不能保证它保存的裸指针仍有效；安全修复是按值保存需要的数据，或让被借用对象成为跨挂起存活的协程局部变量。
- 锁跨挂起：恢复线程可能变，锁也会阻塞其他协程。
  **答案解析：** `std::mutex` 的加锁/解锁绑定线程，协程在 `co_await` 后可能从另一个线程恢复，析构 `lock_guard` 时就可能跨线程解锁。即使恢复线程没变，挂起期间持锁也会挡住其他 completion 和取消路径；安全修复是锁内复制数据，挂起前释放锁。
- `initial_suspend` 抛异常：清理路径受实现影响，不适合作库契约。
  **答案解析：** 协程创建时已经分配 frame、构造 promise 并拿到返回对象，`initial_suspend` 抛异常会进入很早的清理窗口。不同编译器对诊断和返回对象状态的表现可能不同；安全修复是让 `initial_suspend/final_suspend` 都保持 `noexcept`，把可能失败的工作移到协程体或启动前。
- detached：后台协程没有完成所有者。
  **答案解析：** detached 启动后调用方没有 future、scope 或 in-flight 计数来等待完成。异常可能丢失，shutdown 后 callback 还可能访问已销毁对象；安全修复是用 `use_future`、scope、completion handler 或显式 drain 证明后台协程都结束。
- yield 指针：当前 yield 窗口内可借用，推进后可能悬空。
  **答案解析：** generator 的当前 yield 值只在本轮暂停窗口内稳定。消费者把指针或引用保存起来，再执行下一次 `++it` 后，上一轮局部对象或 promise 中的当前槽可能已经被覆盖；需要跨窗口保存时复制值。
- promise 析构 throw：直接 `std::terminate`。
  **答案解析：** `coroutine_handle::destroy()` 会销毁 frame 内 promise，析构函数抛异常没有普通恢复路径。安全修复是析构保持 `noexcept`，清理失败提前显式处理或只记录状态。
- `string_view` generator：view 不拥有字符缓冲。
  **答案解析：** 如果 `yield_value` 只保存 `string_view`，而 yield 表达式来自临时 string，view 指向的 buffer 会在生命周期结束后失效。安全修复是 promise 按值保存 `std::string`，或者 API 明确 view 只能在当前 yield 窗口内使用。

运行：

```powershell
cmake -S C09_Coroutines/exercises -B build/coroutine-j1 -DCOROUTINE_STUDY_BUILD_REFERENCE=ON
cmake --build build/coroutine-j1 --target J1_eight_pitfalls_reference
ctest --test-dir build/coroutine-j1 -R J1_eight_pitfalls_reference --output-on-failure
```

预期：reference 打印八条分类，然后所有安全修复路径通过，末尾输出 `J1 reference: all safe invariants passed`。

**答案解析：** reference 默认跑安全路径，八条分类用于对照机制和排错关键词。安全断言覆盖按值捕获、临时对象生命周期、锁边界和 frame 分配释放等不变量；unsafe demo 需要单独开启，避免一次运行混入多个 UB 症状。

回读路径：先看 `traps` 表里的分类和 safe_rule；再看 `good_lambda_capture`、`good_temporary_lifetime`、`good_lock_boundary` 三个安全例子；最后看 frame 分配/释放计数，确认安全路径没有泄漏。

**答案解析：** `traps` 表回答“这类 bug 是什么、症状是什么、怎么修”。三个 good 示例分别把最常见的引用、临时对象和同步锁问题改成可维护生命周期；分配/释放计数则证明安全路径没有把协程帧遗留到 owner 之外。

观察 UB 时一次只打开一个陷阱，配 ASan/TSan 或 MSVC 运行时检查记录症状。陷阱 3/5 常常不会被 ASan 抓到，需要线程调度或静态分析证据。
