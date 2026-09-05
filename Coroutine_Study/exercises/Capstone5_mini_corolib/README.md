# 结课项目 5：mini 协程库实现

对应文档：`14-第三阶段结课-mini协程库实现.md`

## 项目目标

从零实现一个最小但功能完整的协程库。核心库 **禁第三方依赖**（仅 C++23 标准库）；
stdexec 仅用于可选 sender -> awaitable 桥接验证。本项目是模块 D~G 的综合考试，
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
   coroutine state / frame 不会因为外层 `task` wrapper move 而移动；move
   移动的是 owning handle。真正的风险是 `start()` 之后 operation_state、
   receiver、等待者链和唯一所有权已经建立，再移动 wrapper 会让生命周期难以证明。
   **最简单方案**：禁 copy，允许未启动 move；`start()` / `co_await` / `connect`
   后禁止再移动。

2. **completion_signatures 编译期可查询**
   每个 sender-like 组件（task / generator / when_all 等）能通过元编程在
   编译期回答"我会产生什么类型的结果"。用 `static_assert` 验证至少 task 与
   when_all。

3. **HALO 在 sync_wait 入口可触发**
   用 Clang `-Rpass=coroutine-elide` 编译，简单路径
   `sync_wait(just(42))` 有机会出现 "coroutine frame elided"。HALO 是实现观察项，
   不是标准保证；记录触发/未触发的工具链和原因即可。

## 项目骨架

```
Capstone5_mini_corolib/
  include/mini/
    task.hpp                    # task<T>（决策 1B / 决策 2B / 决策 4A）
    generator.hpp               # generator<T>（按值存，免疫 J-1 trap 8）
    shared_task.hpp             # shared_task<T>（最复杂，建议留到进阶）
    when_all.hpp                # when_all（全分支收束后返回结果/首异常）
    when_any.hpp                # when_any（首个成功 value 获胜，全部收束后返回）
    sync_wait.hpp               # 返回 std::optional<std::tuple<Ts...>>
    async_scope.hpp             # spawn / on_empty / 析构等待
    stop_token.hpp              # std::stop_token/std::stop_source 包装 + mini::in_place_stop_source 进阶
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

`include/` 与 `src/` 是学生 starter；`reference/` 是可运行标准答案。核心 reference 不依赖第三方；stdexec adapter 是可选桥接验证，只在 `stdexec::stdexec` 可用时单独构建。

```powershell
cmake -S Coroutine_Study/exercises --preset verify-core
cmake --build Coroutine_Study/exercises/build/verify-core --config Release --target mini_reference_task mini_reference_generator mini_reference_sync_wait mini_reference_when_all mini_reference_when_any mini_reference_run_loop mini_reference_task_scope mini_reference_stop mini_reference_shared_task
ctest --test-dir Coroutine_Study/exercises/build/verify-core -C Release -R "mini_reference_"
```

Reference 教学契约：

- `task` 由单一 owner 驱动；`start()` / `co_await` 后不得二次启动，完成后的重复 `co_await` / `await_resume` 也应拒绝。
- `when_all` 先启动全部分支，等全部分支完结后返回 tuple；若有异常，传播首个异常。
- `when_any` 首个成功 value 获胜并请求 stop，但仍等待全部分支收束后返回；若没有成功 value，传播首个异常。
- 不同线程完成由每个 operation 自己的 `mutex` 或等价同步保护；runner 在 `final_suspend` 路径计入完成。
- `sync_wait` 只 start root task 一次，完成通知来自 runner 的 `final_suspend` 后续路径，而不是 blind-resume 循环。

## 必做任务

按从底层到上层的顺序实现（顺序非常重要）：

1. **CPO 与 concept 基础设施**（约 60-90 行） —— 在 task/sender concept 框架；
2. **just / 基础 sender**（约 50-80 行） —— `mini::just(values...)`；
3. **promise_type 与 task<T>**（约 120-180 行） —— 8 个 hook + symmetric transfer；
4. **generator<T>**（约 60-100 行） —— iterator 接口；
5. **when_all / when_any**（约 100-150 行） —— 二元 task 版，operation 内部 mutex 保护 remaining/winner；
6. **sync_wait**（约 50-80 行） —— condvar + optional<tuple>；
7. **single_thread_executor**（约 50-80 行） —— schedule sender；
8. **async_scope + stop_token**（约 80-120 行） —— spawn / on_empty；
9. **as_awaitable 桥接**（约 60-100 行） —— bridge receiver。

## 最终验证（14-mini §"最终验证"）

**验证 1：task + sync_wait（可选：as_awaitable 桥接 stdexec::just）**
```cpp
mini::task<int> compute() {
    int x = co_await mini::as_awaitable(stdexec::just(21));
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
auto opt = mini_ref::sync_wait(
    mini_ref::when_all(make_task(2+3), make_task(2*3))
);
// opt 的类型是 std::optional<std::tuple<std::tuple<int, int>>>
auto [sum, prod] = std::get<0>(*opt);
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
# 记录 stderr 里是否有 "remark: coroutine frame elided"
```

**验证 6：valgrind / ASan 无 leak**
```bash
g++ -std=c++23 -O2 -fsanitize=address mini_task_test.cpp -o test
./test    # 通过时记录 ASan 0 errors；失败时先修生命周期问题
```

## 验收点

- 10 个组件全部实现，至少 4 条最终验证通过；
- operation_state non-movable 约束满足（编译期或运行时检查均可）；
- `completion_signatures` 编译期可查（用 `static_assert` 验证 task 与 when_all）；
- HALO 观察有记录：触发或未触发都要说明编译器、优化级别和原因分析；
- valgrind / ASan 验证有记录；没有这些工具时明确写下未验证原因；
- 能画出 task / when_all / sync_wait / as_awaitable 的完整对象关系图；
- 能解释每一层的设计选择（lazy vs eager、symmetric transfer、operation_state non-movable 的根因）。

## 关键设计决策（14-mini §"关键设计决策参考"）

| 决策 | 方案 | 本骨架默认 | 理由 |
|---|---|---|---|
| task 移动语义                  | A 运行时检查 / **B 禁 copy，允许未启动 move** | B | 保留返回值移动，同时禁止已启动对象所有权漂移 |
| final_suspend symmetric trans  | A 普通 / **B 对称转移**          | B | 标准层面转交 continuation；机器栈表现需实测 |
| when_all 错误策略              | **A 全分支收束后首异常** / B 收集所有 | A | 结果/错误生命周期清楚，和 reference 一致 |
| operation_state 存储           | **A 内联** / B unique_ptr        | A | 不分配，HALO 友好 |

## 进阶任务（可选）

- 实现 `mini::on(scheduler, sender)`；
- 实现 `mini::let_value(sender, factory)`；
- 把二元 `when_all` / `when_any` 泛化为三元或 variadic 版本；
- 给 task 加 allocator 感知（P0912 风格）；
- generator symmetric transfer 优化；
- `mini::task` vs cppcoro `task` 的 `when_all` 性能对比（1000 次）；
- 实现 `mini::split(sender)` —— 多 consumer 共享 sender 结果。

## 提示

- 先死 `T = int` 跑通所有组件，再泛化模板；
- 每个组件一个独立 `_test.cpp`，改一个组件只重编一个 TU；
- `completion_signatures` 推导可先用 `static_assert + 手写期望类型`；
- `when_all` 取消传播复杂，reference 先做"全部分支收束后返回结果/首异常"；
- `shared_task` 是最复杂的组件，建议留到进阶；
- 不要在 tracing wrapper 的内存分配路径上再打 trace（无限递归）。

## 项目复盘问题（14-mini §"项目复盘问题"）

- 哪一层最出乎你意料地复杂？
- `when_all` 的 `std::tuple` 结果推导造成什么困难？
- operation_state non-movable 对 `async_scope` 实现的影响？
- 你的 final_suspend symmetric transfer 在目标编译器上呈现怎样的机器栈表现？哪些不是标准保证？
- 把 `mini::task<T>` 扩展为 `mini::eager_task<T>` 需要改哪些地方？
- 完成本项目后，对 P3552（std::execution::task）的设计选择有了什么新理解？
- 与 cppcoro 相比的最大简化在哪里？这些简化在什么场景下出问题？
- 给本库加 "coroutine frame pool allocator"，最自然的插入点在哪？
