# 练习 L04：名字查找、依赖名与 ADL

先阅读 [04. 名字查找、依赖名与 ADL](../../chapters/04-lookup-adl.md)。本题只编辑 `src/student/lookup_probe.hpp`。Reference 在 `src/reference/lookup_probe.hpp`，good/bad 控制实现分别在 `validation/good/lookup_probe.hpp` 和 `validation/bad/lookup_probe.hpp`。

## 文件位置

- 学生编辑：`src/student/lookup_probe.hpp`
- Reference：`src/reference/lookup_probe.hpp`
- 正确控制：`validation/good/lookup_probe.hpp`
- 错误控制：`validation/bad/lookup_probe.hpp`
- checker：`checks/lookup_checks.cpp`
- 观察程序：`observations/lookup_observation.cpp`
- 编译诊断：`validation/diagnostics/missing_typename*.cpp`

## Part 1：成员查找

实现 `c04_lookup::inspect(object)`。若当前 cv/ref 条件下 `object.inspect()` 可调用，返回成员调用结果。返回类型和值类别必须保留，`noexcept` 必须来自真实成员表达式。

已提供 `MemberOnly` 正例，checker 检查 `int&`、`const int&`、`int&&` 三种结果。学生任务是让同一个 `operator()` 按实参 cv/ref 形成真实成员调用，不把右值变成左值，也不把引用返回成值。

## Part 2：ADL 查找

成员不可调用时，尝试未限定调用 `inspect(object)`。ADL 必须能找到对象关联 namespace 里的自由函数，也必须能找到 hidden friend。内部用 detail namespace 和不可匹配的 poison pill 隔离，避免查找到 `c04_lookup::inspect` 这个 CPO 对象。

已提供 `AdlOnly`、`HiddenFriend` 和 `WrongMemberHasAdl`。checker 实际调用 ADL 返回的引用并写回原对象。学生任务是把 ADL 探测写在可替换的依赖表达式里；不能用限定名，也不能在看见同名但不可调用成员后直接失败。

## Part 3：成员优先与拒绝

成员和 ADL 同时存在时，成员胜出。没有成员也没有 ADL 时，`requires { c04_lookup::inspect(object); }` 为 `false`。成员名存在但签名不匹配时，不算合法成员路径，应继续尝试 ADL。

已提供 `BothRoutes` 和 `NoRoute`。checker 检查返回地址来自 member 字段，并用 `c04_inspectable` 拒绝 no-route 类型。bad 控制实现多了兜底 overload，因此会被 `no-route object must not satisfy c04_inspectable` 拒绝。

## Part 4：观察与诊断

`lookup_observation.cpp` 展示 `this->`、`typename`、`template` 的正向用法，并用递归深度哨兵展示未隔离 CPO 名字会接受 no-route 对象。`missing_typename_control.cpp` 是正例；`missing_typename.cpp` 是缺 `typename` 的语义负例，预期匹配 MSVC `error C7510`。

## 验证入口

本题注册 Reference、good、bad、observation 和一个编译诊断 case。Student 初始实现能编译，但应被 checker 拒绝。正式构建窗口开启前不要运行本题；作者证据写入 `references/validation/lookup-author/`。
