# C06 advanced slice r2 independent review

日期：2026-09-10  
审查范围：`C06_Ranges/11-模块H-高级实现模式.md`，以及 `H1_non_propagating_cache`、`H2_common_iter_proxy`、`H3_generator_const_iter`、`CAPSTONE3_impl_source_reading`、`CAPSTONE4_mini_ranges` 的 source / checker / validation / student 路径。  
源码快照：`advanced-review-r2-sourcehash-before-pwsh.json` 记录 `git_head=f261bea559d6722c31135fb2d3589be52fe958ed`，1644 个文件；`advanced-review-r2-sourcehash-after-pwsh.json` 同样 1644 个文件，本轮手工比对 hash 列表一致。

## Verdict

**BLOCK / REQUEST CHANGES**

r2 的实现和 checker 证据已覆盖上一轮多数问题：H3 reference/validation/student 的 `iter_const_reference_t`、`vector<bool>`、`move_iterator<const&&>` 和 generator 句柄生命周期均通过 hostile probe、Release/Debug/ASan/Student 负例验证；H1/H2/CAP3/CAP4 的受影响点也有源码读取和测试证据。

仍有 1 个阻断问题：H3 正文仍保留上一轮的简化 `conditional_t` alias 和“prvalue/proxy 按值返回”讲法。该正文与当前 H3 reference 代码、checker 断言和任务要求不一致，会继续把本机简化 Ref/Val 误讲成标准 `basic_const_iterator` 语义。

## Issues

### [HIGH] H3 正文仍保留旧版 `iter_const_reference_t` 简化推导，和 r2 代码修复自相矛盾

文件：`C06_Ranges/11-模块H-高级实现模式.md:849`

证据：

- `C06_Ranges/11-模块H-高级实现模式.md:849-854` 仍写：
  - `prvalue/proxy 按值返回`
  - `std::conditional_t<std::is_reference_v<std::iter_reference_t<I>>, std::common_reference_t<const value_type&&, std::iter_reference_t<I>>, std::iter_reference_t<I>>`
- 当前代码已改成标准形态：`C06_Ranges/exercises/H3_generator_const_iter/src/reference/generator_const_iter.hpp:15-17` 使用 `std::common_reference_t<const std::iter_value_t<I>&&, std::iter_reference_t<I>>`，不再对非 reference 的 `iter_reference_t` 直接原样返回。
- 当前 checker 已明确要求：
  - `vector<bool>::iterator` 包装后 `decltype(*wrapped) == bool`，且 `!std::indirectly_writable<wrapped, bool>`：`C06_Ranges/exercises/H3_generator_const_iter/main.cpp:104-111`
  - `std::move_iterator<std::vector<int>::iterator>` 包装后 `decltype(*wrapped) == const int&&`：`C06_Ranges/exercises/H3_generator_const_iter/main.cpp:114-121`
  - proxy/prvalue 按标准 alias 验证：`C06_Ranges/exercises/H3_generator_const_iter/main.cpp:124-136`

影响：

这是正文原位残留，不是测试缺口。学生阅读正文会得到和 reference/checker 相反的规则：对 `vector<bool>::iterator` 这类 prvalue proxy，旧正文的 `conditional_t` 分支会保留可写 proxy，而当前 checker 要求转换为不可写 `bool`。这正是上一轮冻结重点要求避免的问题。

最小修复：

把 `C06_Ranges/11-模块H-高级实现模式.md:849-854` 改为和 reference 一致的标准 alias，并同步修改注释。例如：

```cpp
template<class I>
using iter_const_reference_t =
    std::common_reference_t<const std::iter_value_t<I>&&, std::iter_reference_t<I>>;

using reference = iter_const_reference_t<I>;
```

旁边正文应点名三个边界：`vector<int>::iterator -> const int&`，`vector<bool>::iterator -> bool` 且不可写，`std::move_iterator<vector<int>::iterator> -> const int&&`。若继续保留“最小骨架”限制，应明确它只覆盖课程测试的表达式和运算，不宣称完整替代标准 `std::basic_const_iterator`。

## Old blocker re-check

- H3 code alias：已修复。Reference 使用标准 `common_reference_t<const value_type&&, reference>` 形态；新增 `advanced-review-r2-h3-hostile-probe.cpp` 对 reference/good/bad/student 四实现编译通过，覆盖 `vector<int>`、`vector<bool>`、`move_iterator`、transform prvalue、custom prvalue proxy。
- H3 generator 生命周期：代码层面通过。Reference move ctor / move assignment 用 `std::exchange` 转移句柄，析构 `reset()`，`begin()` 首次 `resume()` 并传播异常；checker 覆盖 move、self-move、析构计数、空 generator、异常路径和 first-yield。
- H3 入口链接：已修复。`H3_generator_const_iter/CMakeLists.txt` 使用 `CHECK main.cpp`；同类 H1/H2/CAP4 也只看到 `main.cpp`，未发现旧 `checks/*.cpp` 残留参与。
- CAPSTONE3 注释：已修复。`main.cpp:29-31` 正确声明 `empty_view` 是 `sized_range` 和 `borrowed_range`，运行检查 `distance == 0`。
- CAPSTONE4 good 独立性：代码读取通过。Reference/Student 是六层自写实现；Good 不复用 Reference，使用 `std::ranges` / `std::views` 作独立行为 oracle；Bad 的缺陷集中在 `take_view` sentinel `remaining < 0`，会多产一个元素但仍安全终止。六个 good/reference header hash 均不同，这一点只作为辅助证据，结论主要来自语义读取。
- H1 cache owner 依赖：代码与正文均改为 copy/move reset cache；checker 覆盖 copy 后重新扫描。
- H2 proxy / `iter_move` / `iter_swap`：代码与 checker 覆盖 tuple-of-refs proxy、rvalue tuple 和底层元素交换。

## Validation evidence

Recorder：`C02_Objects_Lifetime_Ownership/exercises/tools/record_process.py`

PASS artifacts：

- `advanced-review-r2-sourcehash-before-pwsh.json`：PowerShell 7 生成源码 hash，1644 files。
- `advanced-review-r2-h3-probe-configure.json`：H3 hostile probe 工程配置通过。
- `advanced-review-r2-h3-probe-build-four-variants.json`：H3 reference/good/bad/student 四实现 hostile probe 编译通过。
- `advanced-review-r2-configure.json`：Release/Debug 窄验证工程配置通过。
- `advanced-review-r2-build-targets-release.json`：Release 13 个目标构建通过。
- `advanced-review-r2-ctest-release.json`：Release 13/13 tests passed。
- `advanced-review-r2-build-targets-debug.json`：Debug 13 个目标构建通过。
- `advanced-review-r2-ctest-debug.json`：Debug 13/13 tests passed。
- `advanced-review-r2-student-configure.json`：Student 工程配置通过。
- `advanced-review-r2-student-build-targets.json`：H1/H2/H3/CAP4 student 四目标构建通过。
- `advanced-review-r2-student-ctest-expected-fail.json`：Student 四测试真实失败，CTest exit 8，recorder 按预期判 PASS。
- `advanced-review-r2-asan-configure.json`：ASan 工程配置通过。
- `advanced-review-r2-asan-build-targets.json`：ASan 9 个安全目标构建通过。
- `advanced-review-r2-asan-ctest-pass.json`：ASan 9/9 tests passed。
- `advanced-review-r2-sourcehash-after-pwsh.json`：测试后源码 hash，1644 files，与 before 手工比对一致。
- `advanced-review-r2-static-secret-emptycatch-scan.json`：无空 catch、硬编码 secret/token/password、`best effort` 或“绕过”命中。
- `advanced-review-r2-cap4-good-hash-compare-pass.json`：CAP4 good/reference 六个 header hash 均不同。

Non-product recorder failures retained for audit：

- `advanced-review-r2-sourcehash-before.json`：Windows PowerShell 5 按非 UTF-8 读取中文文件名失败；已用 PowerShell 7 复跑通过。
- `advanced-review-r2-asan-ctest.json`：CTest 本身 exit 0 且输出 9/9 pass，但 recorder 的 `--contains "9 tests passed"` 条件写错；已用 `advanced-review-r2-asan-ctest-pass.json` 复跑通过。
- `advanced-review-r2-cap4-good-hash-compare.json`、`advanced-review-r2-cap4-good-hash-compare-pwsh.json`：命令行 quoting/参数转义错误，命令未有效启动；已改脚本并用 `advanced-review-r2-cap4-good-hash-compare-pass.json` 复跑通过。

## Review limits

- 未运行正式性能基准；本轮只验证 advanced slice 的功能、负例和 ASan 安全集。
- 当前工具表未暴露 `lsp_diagnostics` 或 `ast_grep_search`；本轮用 MSVC/CMake/CTest 编译诊断、ASan、hostile probe 和 `rg` 静态扫描替代。由于结论为 BLOCK，不存在“未跑 LSP 仍批准”的问题。
- 未冒称专用 architect lane；本报告只给代码/规格/安全审查 verdict。
