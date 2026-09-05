# 练习 E-1：co_await 三步查找

## 目标

通过三种 awaitable 各自触发 `co_await` 查找的三条路径，
亲手验证 `await_transform` / 成员 `operator co_await` / ADL `operator co_await`
的优先级，理解"`await_transform` 是 promise 唯一的全局拦截钩子"。

## 必做任务

1. 准备三种 awaitable：A（被拦截）、B（含成员 operator co_await）、C（靠 ADL）。
2. 定义带 `await_transform` 的 `transform_task`：
   - 拦截 A，返回的 awaitable `await_resume()==100`。
   - 拦截 B，返回 `await_resume()==200`（验证它覆盖成员 operator co_await）。
   - 不拦截 C，让它走 ADL。
3. 写不含 `await_transform` 的 `plain_task` 做对照：B 走成员，C 走 ADL。
4. 在笔记中画完整的三步查找决策树，标出每一步编译器的判断。

## 验收点

- 通过日志输出确认 `await_transform` 优先于成员、成员优先于 ADL。
- 在对照协程中确认成员 / ADL 路径在无拦截时能正常工作。
- 你能解释为什么 promise 中泛型 `await_transform(T&&)` 会"贪婪"地拦截所有 co_await。

## 提示

- 用 `std::println` 在每个 `await_ready/await_suspend/await_resume` 中打印来源信息。
- 对照实验最有说服力：先看到 `await_transform` 拦截，再看到不拦截时的差异。
- ADL 的 `operator co_await` 必须放进 awaitable 类型所在的命名空间。

## 本轮练习契约

Starter 要求追踪 co_await 转换。Reference 同时覆盖 promise.await_transform、member operator co_await、free operator co_await。歧义和重载选择按普通 overload resolution；await_transform 只在当前协程 promise 存在对应成员时先参与。

命令：``cmake -S . -B build/dg-lane -DCOROUTINE_STUDY_BUILD_REFERENCE=ON``，然后构建 ``E1_co_await_lookup`` 与 ``E1_co_await_lookup_reference``，再用 ``ctest -R E1_co_await_lookup_reference`` 跑稳定验收。
