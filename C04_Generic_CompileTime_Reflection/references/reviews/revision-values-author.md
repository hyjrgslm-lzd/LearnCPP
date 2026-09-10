# C04 A01/A03 values author review

日期：2026-09-10。

范围：

- `chapters/18-compiletime-values.md`
- `chapters/20-explicit-object-forwarding.md`
- `exercises/A01_compiletime_values/`
- `exercises/A03_explicit_object/`
- `references/validation/revision-20260910/values-*`

结论：PASS，待非作者 review。root CMake、导航、正式 benchmark 均不在本 lane。

后置修复：独立 review `revision-values-review.md` 指出 A01 `make_row` 与文档承诺不一致：文档写“最多 15 个 ASCII 字符”，实现实际接受 16 字符且不拒绝非 ASCII。已保持原承诺修复，原盲写冻结稿和早期验证记录不覆盖。

## 内容契约

| 项 | 结果 | 证据 |
|---|---|---|
| A01 值级 constexpr 表 | PASS | 正文连续推导字面量 key、稳定排序、保留首个重复值、`table<N>` 形状推导、空输入、二分查找 |
| A01 consteval 边界 | PASS | 正文区分普通 consteval 参数、模板 NTTP、constexpr 局部和 scratch 存储；负编译拒绝 scratch pointer 逃逸 |
| A01 key 输入边界 | PASS | `key_width == 16` 明确为含终止 NUL；15 字符正例、空 key 正例、16 字符/非 ASCII/embedded NUL 负编译均覆盖 |
| A01 练习闭环 | PASS | Student 可构建但运行失败；Reference/good 通过；bad 保留最后重复值并被 checker 拒绝 |
| A03 显式对象参数 | PASS | 正文从四重载基线推到 `this auto&& self`；覆盖四类 cvref、`forward_like`、返回类型、地址、`noexcept` |
| A03 move-only / const 限制 | PASS | `take(this slot&&)` 只允许 mutable rvalue；负编译拒绝 `const&& take()` |
| A03 递归 lambda / 借用 | PASS | 正文和 checker 覆盖 explicit-object recursive lambda；`name_holder::name()` 保留外部借用且不延寿 |
| good / bad 独立性 | PASS | `values-static-good-isolation-r2.json` 未发现 good/bad include Reference、TODO、FIXME；A03 bad 已改为独立实现 |
| 记录完整性 | PASS | 每条 configure/build/ctest/static/hash 命令均经 C02 `record_process.py` 记录到 `values-*` JSON |

## 当前绑定验证

正式判定绑定 Visual Studio 18 2026 / MSVC 19.51 证据：

| 证据 | 命令 | 结果 |
|---|---|---|
| `values-a01-configure-vs-r3.json` | configure A01 author VS | PASS |
| `values-a01-build-debug-vs-r4.json` | build A01 author Debug | PASS |
| `values-a01-ctest-debug-vs-r4.json` | ctest A01 author Debug | PASS，4/4 |
| `values-a01-build-release-vs.json` | build A01 author Release | PASS |
| `values-a01-ctest-release-vs.json` | ctest A01 author Release | PASS，4/4 |
| `values-a03-configure-vs-r2.json` | configure A03 author VS | PASS |
| `values-a03-build-debug-vs-r4.json` | build A03 author Debug | PASS |
| `values-a03-ctest-debug-vs-r4.json` | ctest A03 author Debug | PASS，4/4 |
| `values-a03-build-release-vs-r2.json` | build A03 author Release | PASS |
| `values-a03-ctest-release-vs-r2.json` | ctest A03 author Release | PASS，4/4 |
| `values-a01-student-configure.json` | configure A01 Student | PASS |
| `values-a01-student-build-debug.json` | build A01 Student Debug | PASS |
| `values-a01-student-ctest-debug-expected-fail.json` | ctest A01 Student Debug | PASS verdict，exit 8，命中 `check failed:` |
| `values-a03-student-configure.json` | configure A03 Student | PASS |
| `values-a03-student-build-debug.json` | build A03 Student Debug | PASS |
| `values-a03-student-ctest-debug-expected-fail.json` | ctest A03 Student Debug | PASS verdict，exit 8，命中 `check failed:` |
| `values-static-good-isolation-r2.json` | good/bad 静态隔离检查 | PASS |
| `values-source-hashes-r2.json` | owned source SHA256 | PASS |

后置 A01 边界修复绑定证据：

| 证据 | 命令 | 结果 |
|---|---|---|
| `values-a01-boundary-configure-vs.json` | configure A01 author VS | PASS |
| `values-a01-boundary-build-debug-vs.json` | build A01 author Debug | PASS |
| `values-a01-boundary-ctest-debug-vs.json` | ctest A01 author Debug | PASS，7/7 |
| `values-a01-boundary-build-release-vs.json` | build A01 author Release | PASS |
| `values-a01-boundary-ctest-release-vs.json` | ctest A01 author Release | PASS，7/7 |
| `values-a01-boundary-student-configure-vs.json` | configure A01 Student | PASS |
| `values-a01-boundary-student-build-debug-vs.json` | build A01 Student Debug | PASS |
| `values-a01-boundary-student-ctest-debug-expected-fail.json` | ctest A01 Student Debug | PASS verdict，exit 8，Student 运行初态失败，4 个 diagnostic 通过 |
| `values-a01-boundary-static-isolation.json` | A01 静态隔离检查 | PASS |
| `values-a01-boundary-source-hashes.json` | A01 修复后源码 SHA256 | PASS |

关键 CTest 输出：

```text
values-a01-ctest-debug-vs-r4.json: 100% tests passed, 0 tests failed out of 4
values-a01-ctest-release-vs.json: 100% tests passed, 0 tests failed out of 4
values-a03-ctest-debug-vs-r4.json: 100% tests passed, 0 tests failed out of 4
values-a03-ctest-release-vs-r2.json: 100% tests passed, 0 tests failed out of 4
values-a01-student-ctest-debug-expected-fail.json: exit 8, check failed: sort orders literal keys
values-a03-student-ctest-debug-expected-fail.json: exit 8, check failed: value preserves object identity and cvref category
values-a01-boundary-ctest-debug-vs.json: 100% tests passed, 0 tests failed out of 7
values-a01-boundary-ctest-release-vs.json: 100% tests passed, 0 tests failed out of 7
values-a01-boundary-student-ctest-debug-expected-fail.json: exit 8, check failed: sort orders literal keys; 4 diagnostic tests passed
```

## 保留的失败记录

`values-a03-configure-release.json` / `values-a03-build-release.json` / `values-a03-ctest-release.json` 是一次工具链口径错误的原始记录：新 Ninja Release 目录选择了 GNU 13.2.0，不能解析 C++23 显式对象参数。该失败不绑定最终判定；最终判定使用同一个 VS/MSVC author build 目录的 Debug/Release 证据。

`values-a03-build-debug.json` 是早期实现阶段 `forward_like` ADL 歧义失败记录；后续已用全限定 `::c04_explicit::forward_like` 修正，并由 `values-a03-build-debug-vs-r4.json`、`values-a03-ctest-debug-vs-r4.json` 覆盖。

`values-a01-ctest-debug.json` / `values-a01-ctest-debug-r2.json` 是早期 checker/中文诊断匹配修正前记录；最终绑定 `values-a01-ctest-debug-vs-r4.json` 和 Release 证据。

## 未覆盖边界

- 未注册 root CMake 与导航；该范围由 root lane 处理。
- 未跑正式 benchmark；本 lane 不拥有 cost 文件。
- 未做非作者 review；本文件是作者自检记录。
