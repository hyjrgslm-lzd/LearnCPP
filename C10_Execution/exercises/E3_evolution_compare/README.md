# E3 member-first 与 legacy 对照

实现当前课程采用的 member-first dispatch，并保留 legacy `tag_invoke` fallback 作对照。

Part:

1. `member_sender::connect(receiver)` 是拥有类型源码时的首选路径。
2. `legacy_sender` 只能通过 `tag_invoke(connect_t, ...)` 接入。
3. `connect_t` 必须先尝试 member，再 fallback 到 legacy。
4. `lookup_report()` 明确说明当前协议不走裸 ADL。
5. bad 版本先走 legacy，导致同时具备 member 与 legacy 的 sender 选错路径。

学生只改 `src/student/solution.hpp`。初态满足 checker 的语法探测，运行时拒绝。
## IDE 工程入口

VS solution 中本题主入口是 `E3_evolution_compare_student`。学生只编辑 `src/student/solution.hpp`；`main.cpp` 是共同检查器，Reference 在 `src/reference/solution.hpp`，good/bad 控制在 `validation` 下。`E3_evolution_compare` 聚合目标只负责显式构建学生目标，收在 Support；Reference 与控制目标保留为独立项目，用来区分答案、正确对照和错误拒绝。单题可用 `cmake -S <本目录> -B <build>` 独立生成。
