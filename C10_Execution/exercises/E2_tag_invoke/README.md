# E2 tag_invoke 历史实验

实现旧式 `tag_invoke` 基础设施，理解“单一 ADL 入口 + tag 类型”的历史方案。本题是历史对照，不代表后续 C10 当前主协议。

Part:

1. 提供 poison-pill 风格的 `tag_invoke` 查找入口。
2. 实现 `tag_invocable<Tag, Args...>`。
3. `connect_t`、`start_t`、`get_scheduler_t` 都通过 `tag_invoke` 分派。
4. `sender` 和 `env` 用 hidden friend / ADL 提供定制。
5. bad 版本提供 fallback，看似能调用，实际没有走 tag_invoke。

学生只改 `src/student/solution.hpp`。初态 concept 合法，但运行时抛 `c10::unfinished`。
## IDE 工程入口

VS solution 中本题主入口是 `E2_tag_invoke_student`。学生只编辑 `src/student/solution.hpp`；`main.cpp` 是共同检查器，Reference 在 `src/reference/solution.hpp`，good/bad 控制在 `validation` 下。`E2_tag_invoke` 聚合目标只负责显式构建学生目标，收在 Support；Reference 与控制目标保留为独立项目，用来区分答案、正确对照和错误拒绝。单题可用 `cmake -S <本目录> -B <build>` 独立生成。
