# E1 CPO 与 niebloid 边界

实现 `describe` 和 `name_of` 两个 CPO，观察裸 ADL 的劫持风险，并分清 CPO 与 Ranges 语境里的 niebloid。

Part:

1. 保留裸 `greet(Cat)` 与 `greet(Widget)`，说明 ADL 会找到关联命名空间里的函数。
2. `describe` 按 member -> `greet_impl` ADL hook -> default field 的顺序分派。
3. `name_of` 返回字符串值，验证 CPO 不只处理 `void` 操作。
4. bad 版本先走裸 `greet`，会破坏 member-first 优先级。

学生只改 `src/student/solution.hpp`。初态运行时报 `UNFINISHED`，不会调用 Reference。
