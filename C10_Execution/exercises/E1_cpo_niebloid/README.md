# E1 CPO 与 niebloid 边界

实现 `describe` 和 `name_of` 两个 CPO，观察裸 ADL 的劫持风险，并分清 CPO 与 Ranges 语境里的 niebloid。

Part:

1. 保留裸 `greet(Cat)` 与 `greet(Widget)`，说明 ADL 会找到关联命名空间里的函数。
2. `describe` 按 member -> `greet_impl` ADL hook -> default field 的顺序分派。
3. `name_of` 返回字符串值，验证 CPO 不只处理 `void` 操作。
4. bad 版本先走裸 `greet`，会破坏 member-first 优先级。

学生只改 `src/student/solution.hpp`。初态运行时报 `UNFINISHED`，不会调用 Reference。
## IDE 工程入口

VS solution 中本题主入口是 `E1_cpo_niebloid_student`。学生只编辑 `src/student/solution.hpp`；`main.cpp` 是共同检查器，Reference 在 `src/reference/solution.hpp`，good/bad 控制在 `validation` 下。`E1_cpo_niebloid` 聚合目标只负责显式构建学生目标，收在 Support；Reference 与控制目标保留为独立项目，用来区分答案、正确对照和错误拒绝。单题可用 `cmake -S <本目录> -B <build>` 独立生成。
