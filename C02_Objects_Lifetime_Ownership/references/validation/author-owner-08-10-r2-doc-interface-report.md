# author-owner 08-10 r2 文档接口补丁

状态：按 root 最新指令只修文档接口漂移，已通过静态核对；已通过代码保持冻结，未重跑无关矩阵，未删除或覆盖任何旧 JSON。

## 修复内容

### L08 README Part 4

旧文档写 `make_array(int n)` 创建 `1..n`，与真实 Reference/checker/Student 不一致。

当前同步为真实 API：

- `make_array(int size, int first_value)` 创建长度为 `size` 的数组，并从 `first_value` 开始递增写入。
- `sum_array(const int_array& values, int size)` 按传入长度求和。
- 解析明确 checker 调用 `make_array(4, 10)`，期望内容 `10, 11, 12, 13`，总和 `46`。

### L09 README Part 3

旧文档写 `lock_value` 返回 `{locked, value}`，与真实 Reference/checker/Student 不一致。

当前同步为真实 API：

- `observe(const std::shared_ptr<node>& owner)` 返回 `std::weak_ptr<node>`。
- `lock_value(const std::weak_ptr<node>& weak, int& out)` 成功时写出 `out` 并返回 `true`，失败时返回 `false`。
- 解析明确 checker 在 strong 存活时验证返回值和真实 value，在 scope 结束后验证返回 `false`。

### L09 public validation 命令

README 的 public validation 命令补上新增目标：

- `./build/c02-owner-l09/Debug/L09_shared_validation_bad_hardcoded_noop.exe`

## 核对证据

新增记录：

- `author-owner-08-10-r2-doc-interface-scan.json`：PASS。

该扫描覆盖：

- L08/L09 README。
- L08/L09 checker。
- L08/L09 Student/Reference。
- L09 CMake validation target。

扫描结果显示：

- L08 README 已出现 `make_array(int size, int first_value)` 和 `sum_array(const int_array& values, int size)`。
- L09 README 已出现 `lock_value(const std::weak_ptr<node>& weak, int& out)`。
- L09 README validation 命令已出现 `L09_shared_validation_bad_hardcoded_noop.exe`。
- 旧错误文本 `1..n`、`{locked, value}`、`has_state`、`state_value`、`build_parent_child_without_cycle`、`shared_from_this_result` 未命中。

## Known gaps

- 本次按指令不重跑构建矩阵；上一轮 L08 r2、L09 r2、L10 r2b 运行证据仍保留。
- 当前停止于文档接口修复冻结态，等待非作者快速关闭。
