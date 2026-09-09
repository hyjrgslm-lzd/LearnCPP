# 练习 E-1：`co_await` 三层查找

对应正文：[07 模块 E](../../07-模块E-awaitable三层与co_await变换.md#e1)。

本练习用日志证明 `co_await expr` 怎样从原始表达式变成最终 awaiter。不要只记“优先级”；要看每个分支的 `await_suspend` 和 `await_resume` 是否真的被调用。

## 做题前先画决策树

普通 `co_await expr` 的查找顺序：

```text
promise scope 找 await_transform
  -> 找到：调用 promise.await_transform(expr)，不可调用则 ill-formed
  -> 没找到：expr 自身进入下一层

operator co_await 重载决议
  -> 成员候选与 ADL 自由函数候选一起决议
  -> 无可行候选时，对象本身必须是 awaiter
```

重点：`await_transform` 找到但不可调用时不会静默回退。

## Part 1：A/B/C 三种源对象

- A 本身有 awaiter 三方法，但在 `transform_task` 中会被 `await_transform` 改写。
- B 有成员 `operator co_await()`，在没有 promise 拦截时返回 wrapper。
- C 位于自定义命名空间，通过 ADL 自由 `operator co_await(C)` 返回 wrapper。

每个 wrapper 都要打印自己的来源，并返回不同数值。数值比文字更可靠：A 返回 100 表示 promise 拦截，B 返回 20 表示成员路径，C 返回 30 表示 ADL 路径。

## Part 2：带 `await_transform` 的协程

`transform_task::promise_type` 对 A 和 B 定义 `await_transform`。运行后应看到：

```text
A -> await_transform -> 100
B -> await_transform -> 200
C -> ADL operator co_await -> 30
```

B 的成员 `operator co_await` 没有机会生效，因为 promise 已经先把 B 改写成另一个 awaitable。

## Part 3：无 `await_transform` 的对照

`plain_task` 没有 promise 拦截。运行后应看到：

```text
B -> member operator co_await -> 20
C -> ADL operator co_await -> 30
```

这证明成员/自由 operator 属于第二层查找，发生在 promise 拦截之后。

## 复盘

泛型 `await_transform(T&&)` 会拦截几乎所有 await expression。它可以作为日志、调度、sender 桥接入口，也可能意外吞掉第三方类型的 ADL 扩展。写协程框架时要把这个全局影响写清楚。

**答案解析：** 普通 `co_await` 先查 promise scope 的 `await_transform` 名字；一旦找到就先尝试 `promise.await_transform(expr)`，不可调用时程序不良构。只有没有 promise 拦截时，表达式才进入成员/ADL `operator co_await` 的重载决议。E-1 的 reference 用 transform、member、free 三条日志证明这个顺序。

## Reference

Reference 同时覆盖 promise `await_transform`、member `operator co_await`、free `operator co_await`。

```powershell
cmake -S . -B build/dg-lane -DCOROUTINE_STUDY_BUILD_REFERENCE=ON
cmake --build build/dg-lane --config Release --target E1_co_await_lookup E1_co_await_lookup_reference
ctest --test-dir build/dg-lane -C Release -R E1_co_await_lookup_reference
```
