# H2：自定义 Query 与通用 Environment 组合

本题把请求 trace、配额与真实 scheduler 放进同一条 receiver 查询链。正文见[运行时与环境组合](../../chapters/09-runtime.md)。编辑 `src/student/solution.hpp`；初态在 `make_override_env` 抛出 UNFINISHED，exit 2。

## Part 1：先定义查询契约

`get_trace_id` 和 `get_quota` 是空 CPO 对象，通过 `env.query(tag)` 查询。它们声明 `query(stdexec::forwarding_query_t)` 为 true，允许信息穿过保留 forwarding query 的 adaptor 环境。旧 `tag_invoke` 机制在 E2 专门比较，本题使用固定 stdexec 的当前成员定制。

`trace_env` 只响应 trace 查询，不能为所有未知 query 提供默认值。否则 generic composition 无法分辨“本层有答案”和“应该继续向父层查”。返回 `const std::string&`，并传播实际查询的 `noexcept`；复制成临时字符串会改变借用契约。

## Part 2：实现覆盖优先的组合

实现 `make_override_env(base, override_part)`。产生的类型对任意 query Q：override 支持则命中 override，否则交给 base；两者均不支持时，该 query 应在 requires 表达式中不可调用。不能只复制几个已知字段，也不能为 trace/配额写两个特例。

Reference 用受约束的两个 `query(Q)` 重载区分命中和回退；Good 用固定库 `stdexec::env{override, base}` 的首个可查询环境规则形成独立对照。Bad 故意把 base 放在前面，被 `override takes priority over parent` 拒绝。

## Part 3：接入真实 sender 图

main.cpp 创建 `stdexec::run_loop::scheduler`，将 scheduler、父 trace 和配额放入 base。四组配额/不同 trace 验证父对象不变、覆盖优先、未知 query SFINAE、返回引用类别与 noexcept。再次覆盖配额形成两层组合，然后通过 `write_env`、`when_all`、`read_env` 在真实 receiver 环境里读取信息。

画出两条查找路径：trace 命中第二层父环境中的 trace override；配额直接命中最外层 override。scheduler 回退应返回原 scheduler 对象，而不是用于展示的线程名字。环境暴露能力不等于调度实际发生；执行位置的实验在 B5/H1。

## 实现解析与边界

组合对象按值拥有 Base/Override，生命周期清楚；若 query 返回引用，它的有效期仍受对应环境对象约束。checker 在 `then` 内把 trace 复制成拥有值后再离开图，避免 `sync_wait` 返回悬空借用。`when_all` 会叠加 stop token 并过滤非 forwarding query；删除 forwarding 声明应触发编译错误，而不是猜测运行结果。

扩展时可添加 allocator 或优先级 query，无需修改组合器。若要借用外部环境，先说明谁保证其寿命，再选择 reference wrapper；本题不预埋全局状态或动态类型擦除。

构建/运行沿[公共指南](../BUILD_GUIDE.md)，单题目录 `H2_custom_query`；检查为 `H2_custom_query_reference`、`H2_custom_query_validation_good`、`H2_custom_query_validation_bad_rejected`。Reference OFF 时仍可构建同一个 Student 接口。
