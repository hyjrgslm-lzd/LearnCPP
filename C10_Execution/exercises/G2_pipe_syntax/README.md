# G2 pipe syntax：自己实现 adaptor closure 组合

本题只研究管道语法，不把重点放在 `then` 的内部实现。你需要实现 `c10_g2::then(f)` 返回的 pipe closure，并支持：

1. `sender | adaptor`：把 sender 交给 adaptor。
2. `adaptor | adaptor`：生成新的 closure，后续收到 sender 时按左到右执行。
3. 约束：非 sender 不能走 `sender | adaptor`，两个 closure 才能相互组合。
4. 值类别：closure 可以 move 后使用，sender 的 move-only value 继续传给下游。

编辑 `src/student/solution.hpp`。Reference 在 `src/reference/solution.hpp`，bad 版本故意把组合顺序反过来，应被诊断 `adaptor composition preserves left-to-right order` 拒绝。

```powershell
$env:STDEXEC_ROOT = '<固定checkout>'
cmake -S C10_Execution\exercises\G2_pipe_syntax -B build\c10-adaptor-author-g2 -DFETCHCONTENT_SOURCE_DIR_STDEXEC=$env:STDEXEC_ROOT
cmake --build build\c10-adaptor-author-g2 --config Debug --target G2_pipe_syntax_reference G2_pipe_syntax_validation_good G2_pipe_syntax_validation_bad G2_pipe_syntax_student
ctest --test-dir build\c10-adaptor-author-g2 -C Debug --output-on-failure
```

完整解析见 `../../chapters/08-adaptor.md` 的 G2 小节。
## IDE 工程入口

VS solution 中本题主入口是 `G2_pipe_syntax_student`。学生只编辑 `src/student/solution.hpp`；`main.cpp` 是共同检查器，Reference 在 `src/reference/solution.hpp`，good/bad 控制在 `validation` 下。`G2_pipe_syntax` 聚合目标只负责显式构建学生目标，收在 Support；Reference 与控制目标保留为独立项目，用来区分答案、正确对照和错误拒绝。单题可用 `cmake -S <本目录> -B <build>` 独立生成。
