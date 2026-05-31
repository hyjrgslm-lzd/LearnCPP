# 练习 A-2：co_return 与 lazy task

> 详尽版本见 `../../02-模块A-三关键字与最小协程.md` 的 `练习 A-2` 章节。
> 本 README 仅摘抄"目标 / 必做任务 / 验收点"。
>
> 注：本题用到的 `lazy_task<T>` 头文件位于 `../include/coroutine_study/lazy_task.hpp`，
> 已统一放到题目项目的 include 路径中。文档原文随附了一个 30 行版本，本仓库提供的实现
> 在功能上一致，并额外支持了 `operator co_await`（B-2 顺序组合需要）。

## 目标

使用随附的 minimal `lazy_task<T>` 头文件，写一段三步值变换的协程，用 `co_return` 产出
最终结果，并在 main 中通过 sync 方式取走值。体会 lazy task 的"创建时不执行，被 await 或
sync_wait 时才执行"与普通函数的区别。

## 必做任务

1. 写一个协程函数 `compute_async(int x)`，返回 `lazy_task<int>`。
2. 协程体内做三步值变换：`step1 = x + 10; step2 = step1 * 2; step3 = step2 - 5;` 然后 `co_return step3;`。
3. 在每一步前后都加日志（步骤名 + 当前值）。
4. 在 main 中先 `auto task = compute_async(5);` 加日志，确认协程体此时未执行。
5. 然后调用 `task.sync_wait()` 取走结果，打印最终值。
6. 写一个普通函数 `int compute_sync(int x)` 做完全相同的三步计算，对比日志顺序。
7. 画一张协程帧状态图：initial_suspend、协程体三步、final_suspend、sync_wait 的 resume 入口。

## 进阶任务

- 在三步之间插入一个 `co_await std::suspend_always{}`，观察 `sync_wait` 循环 resume 几次。
- 把输入/输出换成结构体 `Request{id, payload}` / `Response{id, result}`，验证非平凡类型支持。

## 验收点

- 你能用日志证明：`auto task = compute_async(5)` 这一行不触发协程体的执行。
- 你能解释 `initial_suspend` 返回 `suspend_always` 是惰性启动的根源。
- 你能说明 `co_return step3` 是怎么经过 `promise.return_value` 到达 `sync_wait` 返回值的。
- 你能解释为什么 `lazy_task` 必须禁止拷贝（协程帧所有权唯一性）。
