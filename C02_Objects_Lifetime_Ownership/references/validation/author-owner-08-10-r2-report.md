# author-owner 08-10 r2 返修报告

状态：根据 `reviewer-owner-08-10-r1.md` 与 root 后续 L10 ASan 复现要求完成返修。07/L07 未修改。08/09/10 当前冻结，等待非作者复验。未删除/覆盖任何 r1/r2/r2b JSON；新增失败记录也保留。

## 本轮修复

### L08_unique

修复点：README Part 5 的 `incomplete_owner` 接口与真实 Reference/checker 不一致。

当前文档同步到真实 API：

- `explicit incomplete_owner(int value)` 创建并拥有 `IncompleteState`。
- `value()` 返回状态值；移动后空状态返回 `-1`。
- 不再写默认构造、`has_state()`、`state_value()`。

代码未变，仅文档/解析对齐现有真实接口。

### L09_shared

修复点：旧 `CycleResult` / `EnableResult` 让 Student 可硬编码自报结果，非作者 fake Part4/5 已实测通过。

当前改为 checker 持有事实：

- 删除 `checks/support/shared_support.hpp` 中的 `CycleResult` / `EnableResult`。
- 删除 `build_parent_child_without_cycle()` / `shared_from_this_result()` 自报接口。
- 新接口 `connect_parent_child(parent, child)` 只连真实节点；checker 直接检查 `parent->next == child`、`child->parent.lock() == parent`、外部 weak 过期和 fixture alive 计数。
- 新接口 `shared_from_existing(node&)` 必须从 checker 创建且已由 `shared_ptr` 管理的真实对象调用 `shared_from_this()`；checker 直接比较地址和 `owner.use_count()`。
- 新增 public bad `validation/bad_hardcoded_noop`：Part1-3 真实通过，Part4 不连边，必须被真实节点成员检查拒绝。

### L10_control_block

修复点：独立 ASan 已证实 `p = p->child` 与 `p = std::move(p->child)` 在旧 copy/move assignment 中 heap-use-after-free，原因是先 `release()` 销毁了包含 RHS 成员的旧 pointee，再读取 RHS。

当前修复：

- copy assignment：先保存 RHS control block 并递增 strong，再 release 旧 LHS，最后安装 RHS。
- move assignment：先 `std::exchange(other.block_, nullptr)` 到局部，再 release 旧 LHS，最后安装 RHS。
- checker 新增两个合法场景：`root = root->child` 和 `root = std::move(root->child)`，验证新 owner 指向 child、旧 parent 析构、child control block 保留、最终无泄漏。
- Reference `control_block_base*, bool` adopt 构造器已收回 private，经 `detail::rc_access` 授权 `make_rc`；`weak_rc::lock` 仍通过 friend 访问。
- 两个 public bad 变体的内部构造器也收进 private access，避免以公开 raw adopt 绕过接线问题。

## Fresh 验证证据

### L08 r2

- `author-owner-l08-r2-configure-default.json`：PASS，configure default。
- `author-owner-l08-r2-build-debug.json`：PASS，Debug build。
- `author-owner-l08-r2-ctest-debug.json`：PASS，CTest `100% tests passed`。
- `author-owner-l08-r2-configure-student-ref-off.json`：PASS，Reference OFF + Student ON configure。
- `author-owner-l08-r2-build-student-ref-off.json`：PASS，Student target build。
- `author-owner-l08-r2-student-placeholder.json`：PASS，Student 占位 expected exit 1，含 `check failed`。

### L09 r2

- `author-owner-l09-r2-configure-debug.json`：PASS，Debug + validation variants configure。
- `author-owner-l09-r2-build-debug.json`：PASS，Debug build。
- `author-owner-l09-r2-ctest-debug.json`：PASS，observation/reference CTest `100% tests passed`。
- `author-owner-l09-r2-good-debug.json`：PASS，public good 输出 `L09_shared_validation_contract OK`。
- `author-owner-l09-r2-bad-hardcoded-noop-debug.json`：PASS，expected exit 1，含 `parent owns child through shared next`。
- `author-owner-l09-r2-bad-cycle-debug.json`：PASS，expected exit 1，含 `parent weak expires after graph scope`。
- `author-owner-l09-r2-bad-weak-debug.json`：PASS，expected exit 1，含 `lock fails after destruction`。
- `author-owner-l09-r2-configure-release-default.json` / `build-release-default.json` / `ctest-release-default.json`：PASS，Release default。
- `author-owner-l09-r2-configure-student-ref-off.json` / `build-student-ref-off.json` / `student-placeholder.json`：PASS，Reference OFF + Student build，Student 占位 expected exit 1。

### L10 r2 / r2b

- `author-owner-l10-r2b-configure-debug.json`：PASS，Debug + validation variants configure。
- `author-owner-l10-r2b-build-debug.json`：PASS，Debug build。
- `author-owner-l10-r2b-ctest-debug.json`：PASS，observation/reference CTest `100% tests passed`。
- `author-owner-l10-r2b-good-debug.json`：PASS，public good 输出 `L10_control_block_validation_contract OK`。
- `author-owner-l10-r2b-bad-leak-control-block-debug.json`：PASS，expected exit 1，含 `control block still alive`。
- `author-owner-l10-r2b-bad-weak-resurrect-debug.json`：PASS，expected exit 1，含 `object destroyed after last strong`。
- `author-owner-l10-r2b-configure-release-default.json` / `build-release-default.json` / `ctest-release-default.json`：PASS，Release default。
- `author-owner-l10-r2b-configure-student-ref-off.json` / `build-student-ref-off.json` / `student-placeholder.json`：PASS，Reference OFF + Student build，Student 占位 expected exit 1。

原 ASan repro 修复后复验：

- `author-owner-l10-r2-asan/build-alias-asan-author-md-no-contains.json`：PASS，使用 reviewer 原 `alias-copy-uaf.cpp` / `alias-move-uaf.cpp` 编译到作者 build 目录。
- `author-owner-l10-r2-asan/run-copy-alias-asan-author-md-with-path.json`：PASS，输出 `copy alias assignment OK`。
- `author-owner-l10-r2-asan/run-move-alias-asan-author-md-with-path.json`：PASS，输出 `move alias assignment OK`。

保留的 ASan 中间 FAIL：

- `author-owner-l10-r2-asan/build-alias-asan-author.json`：真实工具链失败，`/MDd` 不允许与 ASan 同用。
- `author-owner-l10-r2-asan/run-copy-alias-asan-author.json` / `run-move-alias-asan-author.json`：前一步未生成 exe，exit None。
- `author-owner-l10-r2-asan/build-alias-asan-author-md.json`：命令 exit 0，但 required stdout 误配；`clang-cl /nologo` 没有输出 exe 名。
- `author-owner-l10-r2-asan/run-copy-alias-asan-author-md.json` / `run-move-alias-asan-author-md.json`：ASan runtime DLL 不在该子进程 PATH，exit `3221225781`；后续 `*-with-path.json` 补足。

## 扫描

- `rg "CycleResult|EnableResult|build_parent_child_without_cycle|shared_from_this_result|has_state|state_value|默认构造拥有|student_ready|student_placeholder_state|placeholder_state" ...`：无命中。
- `rg "control_block_base\* block, bool add_ref" Core_Study/exercises/L10_control_block -n -C 2`：只命中 private 构造器与 `detail::rc_access` adopt，未公开为 public API。

## Known gaps

- 不自审、不放行；等待原 reviewer/verifier 复验。
- L10 仍是单线程教学模型，不扩展真实 STL 的并发、aliasing constructor、deleter/allocator、`enable_shared_from_this`。
- L08/L09/L10 public bad 覆盖本章核心代表错误，不宣称穷尽所有错误实现。
