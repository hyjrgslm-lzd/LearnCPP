# 练习 H-2：自定义 Query + Environment 组合器

## 目标

定义自己的 query CPO，实现一个 environment 组合器 `override_env`，并用它在 sender adaptor 中覆盖特定查询同时透传其余查询。通过这道题把 environment 查询链的"作用域链"本质刻进直觉。

## 前置理解

- 你已经完成模块 E，理解 `tag_invoke` 和 CPO 的定制点机制。
- 你已经完成模块 C2，知道 `get_scheduler()`、`get_stop_token()` 等标准 query 的用法。
- 你理解 environment 是 receiver 一侧向下游提供的上下文查询接口。
- 你接受这题先从两个简单 query 入手，不追求覆盖所有标准 query。

## 必做任务

1. 定义一个自定义 query CPO：`get_trace_id_t`。
   - 它是一个空结构体，同时也是一个可调用对象（参考模块 E 中 CPO 的写法）。
   - 当对某个 environment 调用 `get_trace_id(env)` 时，它通过 `tag_invoke(get_trace_id_t{}, env)` 分发到具体实现。
   - 定义一个全局常量 `inline constexpr get_trace_id_t get_trace_id{}`。
2. 实现一个最小 environment 类型 `trace_env`：
   - 内部持有一个 `std::string trace_id_`。
   - 对 `get_trace_id_t` 查询响应，返回 `trace_id_`。
   - 对其它查询不响应（编译期不匹配即可）。
3. 实现 `override_env<Base, Override>` 组合器：
   - 模板参数 `Base` 是基础 environment，`Override` 是覆盖 environment。
   - 对任意 query `Q`：先尝试在 `Override` 上查询，如果 `Override` 能响应则返回 `Override` 的结果；否则 fallback 到 `Base`。
   - 实现方式：用 `tag_invoke` 重载或 `if constexpr` + SFINAE 判断 `Override` 是否支持某个 query。
   - 提供便捷构造函数 `make_override_env(base, override_part)`。
4. 用 `override_env` 构造一个 adaptor receiver，验证 query 覆盖行为：
   - 从一个线程池拿到 scheduler，让 receiver 的 base environment 包含该 scheduler。
   - 用 `override_env` 在这个 base environment 上叠加 `trace_env{"req-42"}`。
   - 下游 sender（通过 `let_value` 或自定义 sender）查询 `get_scheduler` 时，应该 fallback 到 base，拿到线程池 scheduler。
   - 下游 sender 查询 `get_trace_id` 时，应该命中 override，拿到 `"req-42"`。
5. 验证端到端行为：
   - 构造一段 sender 图，在其中通过 environment 查询同时读取 scheduler 和 trace_id。
   - 打印查询结果，确认 scheduler 来自 base environment，trace_id 来自 override environment。
   - 在笔记中画出 environment 查询链：`query → Override → (miss) → Base → (hit) → result`。

## 进阶任务

- 再叠加一层 override：在已有的 `override_env<base, trace_env>` 之上再覆盖一个 `deadline_env`，形成三层查询链。验证查询依次穿透各层。
- 把 `override_env` 用在一个自定义 sender adaptor 的 receiver 中：这个 adaptor 接受一个 inner sender 和一个 scheduler，在 connect 时构造一个 adaptor receiver，其 `get_env()` 返回 `override_env<downstream_env, scheduler_env>`。这样 inner sender 看到的 scheduler 是被覆盖的，而其它 query 仍然透传。
- 实现一个 `env_chain(env1, env2, env3, ...)` 变参模板，支持任意多层 environment 组合。

## 验收点

- 你能定义新的 query CPO 并让自定义 environment 响应它。
- 你的 `override_env` 能正确实现"先 Override 后 Base"的 fallback 逻辑。
- 下游 sender 能同时查询到覆盖的 query 和 fallback 的 query。
- 你能画出 environment 查询链的完整流向，并解释它为什么像词法作用域链。
- 你没有用全局变量或手动参数传递来代替 environment 查询。

## 观察点

- environment 查询链的设计和编程语言里的词法作用域链（lexical scope chain）有精确的对应：内层作用域可以遮蔽外层同名变量，找不到的变量就往外层查。
- `override_env` 是框架可扩展性的基础设施。标准里的 `write_env` sender adaptor 本质上做的就是这件事。
- 这种基于类型的查询分发让 environment 的组合不需要运行时字典查找，完全在编译期决议。
- 理解了 environment 组合器，你就能理解为什么 sender adaptor 可以"注入"或"覆盖"执行上下文，而不需要修改 inner sender 的代码。

## 常见坑

- `override_env` 的 fallback 逻辑写反了：先查 Base 后查 Override，导致覆盖永远不生效。
- SFINAE 判断"Override 是否支持某个 query"时条件写错，导致所有 query 都 fallback 或所有 query 都命中 Override。
- 忘记在 `override_env` 中转发它不关心的 query，导致 base environment 的标准 query（如 `get_stop_token`）丢失。
- 把 environment 和 receiver 混为一谈：environment 是 receiver 的一个属性（通过 `get_env()` 返回），不是 receiver 本身。
- 自定义 query CPO 没有正确走 `tag_invoke` 分发，导致查询结果始终是默认值或编译失败。

## 提示

- `override_env` 的核心实现大约 30-50 行，不要过度设计。
- 先让两层查询（Base + Override）跑通，再考虑多层组合。
- 判断"Override 是否支持 query Q"的一种实用手法：用 `requires` 表达式或 `std::is_invocable` 检测 `tag_invoke(Q{}, override)` 是否合法。
- 画图时用箭头标注查询方向，它始终是从内层向外层单向穿透。
- 这道题的代码量大约在 80-120 行。

## 复盘问题

- 为什么 environment 查询链比"手动传一个大 context 结构体"更适合异步框架？
- `override_env` 的 fallback 逻辑和 JavaScript 的原型链（prototype chain）有什么结构相似性？
- 如果你要在 sender adaptor 中"注入"一个新的 stop_token，你会用 `override_env` 的什么模式？
- 为什么 environment 查询是编译期分发，而不是运行时 `map.find(key)`？这对性能和类型安全分别有什么影响？
- 标准里的 `write_env` 和你实现的 `override_env` 在设计意图上是否一致？

## 对应官方参考

- P2300R10 中 `execution::get_env` 和 environment query 的规范
- P2300R10 中 `execution::write_env` sender adaptor 的定义
- `stdexec` 中 `__env::__join` 或类似 environment 组合实现
- 模块 E 中 tag_invoke / CPO 的基础
