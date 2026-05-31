# 结课项目 5：mini 协程库实现

对应文档：`14-第三阶段结课-mini协程库实现.md`

## 项目目标

从零实现一个最小但功能完整的协程库。**禁第三方依赖**（仅 C++23 标准库 +
stdexec / asio 用于 `as_awaitable` 桥接验证）。本项目是模块 D~G 的综合考试，
也是后续阅读 cppcoro / folly coro / stdexec 协程源码时最可靠的"对照坐标"。

## 必须包含的 10 个组件

| # | 组件 | 头文件 | 说明 |
|---|---|---|---|
| 1 | `mini::task<T>`              | `task.hpp`                    | lazy 启动的协程任务 |
| 2 | `mini::generator<T>`         | `generator.hpp`               | 同步惰性序列 |
| 3 | `mini::shared_task<T>`       | `shared_task.hpp`             | 多 awaiter 共享结果 |
| 4 | `mini::when_all`             | `when_all.hpp`                | 并发等待 |
| 5 | `mini::when_any`             | `when_any.hpp`                | 竞速等待 |
| 6 | `mini::sync_wait`            | `sync_wait.hpp`               | 同步阻塞等待 |
| 7 | `mini::async_scope`          | `async_scope.hpp`             | 结构化并发 |
| 8 | `mini::stop_token`           | `stop_token.hpp`              | 协作式取消 |
| 9 | `mini::single_thread_executor` | `single_thread_executor.hpp` | 单线程调度器 |
| 10 | `mini::as_awaitable(sender)` | `as_awaitable.hpp`            | sender -> awaitable 桥接 |

## 三个设计约束（**必须满足**）

1. **operation_state 等价物 non-movable**
   `task` 内部 promise 与 frame 在 `start()` 后地址不能变。**最简单方案**：
   `task` 禁 copy/move，或仅允许 move 构造但禁 move 赋值，且 `start()`
   后任何移动都视为 use-after-move。

2. **completion_signatures 编译期可查询**
   每个 sender-like 组件（task / generator / when_all 等）能通过元编程在
   编译期回答"我会产生什么类型的结果"。用 `static_assert` 验证至少 task 与
   when_all。

3. **HALO 在 sync_wait 入口可触发**
   用 Clang `-Rpass=coroutine-elide` 编译，简单路径
   `sync_wait(just(42))` 应见 "coroutine frame elided"。CMakeLists 已为
   主 demo 注入该 flag。

## 项目骨架

```
Capstone5_mini_corolib/
  include/mini/
    task.hpp                    # task<T>（决策 1B / 决策 2B / 决策 4A）
    generator.hpp               # generator<T>（按值存，免疫 J-1 trap 8）
    shared_task.hpp             # shared_task<T>（最复杂，建议留到进阶）
    when_all.hpp                # when_all（决策 3A：fail-fast）
    when_any.hpp                # when_any（atomic<bool> winner）
    sync_wait.hpp               # 返回 std::optional<std::tuple<Ts...>>
    async_scope.hpp             # spawn / on_empty / 析构等待
    stop_token.hpp              # std::stop_token + in_place_stop_source
    single_thread_executor.hpp  # std::queue + condvar
    as_awaitable.hpp            # bridge_receiver + sender 桥接
  src/
    main.cpp                    # demo driver: 4 个验证场景
  tests/
    task_test.cpp
    generator_test.cpp
    when_all_test.cpp
    sync_wait_test.cpp
    scope_test.cpp
    as_awaitable_test.cpp
  CMakeLists.txt
  README.md
```

## 必做任务

按从底层到上层的顺序实现（顺序非常重要）：

1. **CPO 与 concept 基础设施**（约 60-90 行） —— 在 task/sender concept 框架；
2. **just / 基础 sender**（约 50-80 行） —— `mini::just(values...)`；
3. **promise_type 与 task<T>**（约 120-180 行） —— 8 个 hook + symmetric transfer；
4. **generator<T>**（约 60-100 行） —— iterator 接口；
5. **when_all / when_any**（约 100-150 行） —— atomic count + tuple 收束；
6. **sync_wait**（约 50-80 行） —— condvar + optional<tuple>；
7. **single_thread_executor**（约 50-80 行） —— schedule sender；
8. **async_scope + stop_token**（约 80-120 行） —— spawn / on_empty；
9. **as_awaitable 桥接**（约 60-100 行） —— bridge receiver。

## 最终验证（14-mini §"最终验证"）

**验证 1：task + sync_wait（含 as_awaitable 桥接 stdexec::just）**
```cpp
mini::task<int> compute() {
    int x = co_await mini::as_awaitable(std::execution::just(21));
    co_return x * 2;
}
auto opt = mini::sync_wait(compute());
auto [v] = *opt;   // v == 42
```

**验证 2：generator + ranges**
```cpp
for (int v : fib() | std::views::take(10)) values.push_back(v);
// values == {0,1,1,2,3,5,8,13,21,34}
```

**验证 3：when_all + 类型安全**
```cpp
auto [sum, prod] = std::get<0>(*mini::sync_wait(
    mini::when_all(make_task(2+3), make_task(2*3))
));
// sum == 5, prod == 6
```

**验证 4：async_scope + stop_token**
```cpp
mini::async_scope scope;
for (int i = 0; i < 10; ++i) scope.spawn(work_i());
// scope 析构时等待所有任务完成
```

**验证 5：HALO**
```bash
clang++ -std=c++23 -O2 -Rpass=coroutine-elide main.cpp
# 看 stderr 里是否有 "remark: coroutine frame elided"
```

**验证 6：valgrind / ASan 无 leak**
```bash
g++ -std=c++23 -O2 -fsanitize=address mini_task_test.cpp -o test
./test    # 0 errors
```

## 验收点

- 10 个组件全部实现，至少 4 条最终验证通过；
- operation_state non-movable 约束满足（编译期或运行时检查均可）；
- `completion_signatures` 编译期可查（用 `static_assert` 验证 task 与 when_all）；
- HALO 在 sync_wait 入口的简单路径上触发（有 `-Rpass=coroutine-elide` 输出为证）；
- valgrind / ASan 0 errors；
- 能画出 task / when_all / sync_wait / as_awaitable 的完整对象关系图；
- 能解释每一层的设计选择（lazy vs eager、symmetric transfer、operation_state non-movable 的根因）。

## 关键设计决策（14-mini §"关键设计决策参考"）

| 决策 | 方案 | 本骨架默认 | 理由 |
|---|---|---|---|
| task 移动语义                  | A 运行时检查 / **B 禁 copy/move** | B | 零开销，编译期杜绝 use-after-move |
| final_suspend symmetric trans  | A 普通 / **B 对称转移**          | B | 栈安全，深嵌套不溢出 |
| when_all 错误策略              | **A fail-fast** / B 收集所有     | A | 资源释放快，与 stdexec 一致 |
| operation_state 存储           | **A 内联** / B unique_ptr        | A | 不分配，HALO 友好 |

## 进阶任务（可选）

- 实现 `mini::on(scheduler, sender)`；
- 实现 `mini::let_value(sender, factory)`；
- 给 task 加 allocator 感知（P0912 风格）；
- generator symmetric transfer 优化；
- `mini::task` vs cppcoro `task` 的 `when_all` 性能对比（1000 次）；
- 实现 `mini::split(sender)` —— 多 consumer 共享 sender 结果。

## 提示

- 先死 `T = int` 跑通所有组件，再泛化模板；
- 每个组件一个独立 `_test.cpp`，改一个组件只重编一个 TU；
- `completion_signatures` 推导可先用 `static_assert + 手写期望类型`；
- `when_all` 取消逻辑复杂，先做"不取消，全完成"简化版；
- `shared_task` 是最复杂的组件，建议留到进阶；
- 不要在 tracing wrapper 的内存分配路径上再打 trace（无限递归）。

## 项目复盘问题（14-mini §"项目复盘问题"）

- 哪一层最出乎你意料地复杂？
- `when_all` 的 `std::tuple` 结果推导造成什么困难？
- operation_state non-movable 对 `async_scope` 实现的影响？
- 你的 final_suspend symmetric transfer 在什么条件下会栈溢出？
- 把 `mini::task<T>` 扩展为 `mini::eager_task<T>` 需要改哪些地方？
- 完成本项目后，对 P3552（std::execution::task）的设计选择有了什么新理解？
- 与 cppcoro 相比的最大简化在哪里？这些简化在什么场景下出问题？
- 给本库加 "coroutine frame pool allocator"，最自然的插入点在哪？
