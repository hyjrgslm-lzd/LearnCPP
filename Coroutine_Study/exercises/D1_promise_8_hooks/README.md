# 练习 D-1：从零写 lazy_task<T>

## 目标

完整实现 `lazy_task<T>` 的 promise_type 可定制点：
get_return_object / initial_suspend / final_suspend / return_value /
unhandled_exception / operator new+delete /
get_return_object_on_allocation_failure。
验证 `co_return` 值正确取出、协程内异常被外部 `get()` 重新抛出、
嵌套 `co_await` 正确生效。

## 注意

D-1 起不再使用 `include/coroutine_study/lazy_task.hpp`。
本目录的 main.cpp 必须包含完整 `lazy_task<T>` 实现，所有 TODO
位于每个 hook 函数体内（类型层骨架已给出）。

## 必做任务

1. 实现 8 个 hook（含 yield_value 显式省略）。
2. 写测试协程验证正常 return（`co_return x+y`）。
3. 写测试协程验证异常路径（throw -> get() rethrow）。
4. 写测试协程验证嵌套 `co_await`。
5. 在笔记中画出协程帧分配 / get_return_object / initial_suspend /
   final_suspend / continuation 的时序图。

## 验收点

- `lazy_task<T>` 能正确返回 `co_return` 的值。
- 能重新抛出协程体内的异常到 `get()` 调用者。
- 支持嵌套 `co_await`。
- 能口头说出每个 hook 在生命周期中的位置和职责。
- 没有在 `unhandled_exception` 中再次 `throw`。

## 提示

- 先让 normal/异常路径跑通再补 hook。
- final_suspend 必须挂起；否则在 get() 读取 result 前帧已销毁 -> UB。
- get_return_object 中可以安全地 `from_promise(*this)`，
  因为 promise 已构造完成。

## 本轮练习契约

Starter 只保留入口提示；在本目录从零实现 task/promise。Reference 验证 value、exception_ptr、nested co_await 和 final_suspend continuation。注意：所谓 hook 数量是教学分组，不是标准固定八个；allocation failure 只有在 promise 提供 non-throwing operator new 时才走 get_return_object_on_allocation_failure。标准来源：coroutine promise lookup、initial/final suspend、return_value、unhandled_exception、allocation failure rules。

命令：``cmake -S . -B build/dg-lane -DCOROUTINE_STUDY_BUILD_REFERENCE=ON``，然后构建 ``D1_promise_8_hooks`` 与 ``D1_promise_8_hooks_reference``，再用 ``ctest -R D1_promise_8_hooks_reference`` 跑稳定验收。
