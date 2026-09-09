# 作者 C：12—15 验证记录，2026-09-09

范围：`chapters/12-*` 到 `chapters/15-*`，`exercises/L12_callables`、`L13_indirect_values`、`L14_contracts`、`L15_evolution`，以及 `references/validation/author-c-*`。

未修改：公共 CMake、根 README、F01 前沿探测、标准索引冻结内容之外的其他作者文件。

## 验证矩阵

每个叶级目录均使用 MSVC `Visual Studio 18 2026` x64 生成器，Debug 与 Release 分离构建。命令由 C02 `record_process.py` 保存原始 JSON。

| 叶级目录 | 配置 | Configure | Build | CTest | 结果含义 |
|---|---|---|---|---|---|
| `L12_callables` | Debug | `author-c-l12-r2-debug-20260909-configure.json` | `author-c-l12-r2-debug-20260909-build.json` | `author-c-l12-r2-debug-20260909-ctest.json` | PASS |
| `L12_callables` | Release | `author-c-l12-r2-release-20260909-configure.json` | `author-c-l12-r2-release-20260909-build.json` | `author-c-l12-r2-release-20260909-ctest.json` | PASS |
| `L13_indirect_values` | Debug | `author-c-l13-debug-20260909-configure.json` | `author-c-l13-debug-20260909-build.json` | `author-c-l13-debug-20260909-ctest.json` | PASS |
| `L13_indirect_values` | Release | `author-c-l13-release-20260909-configure.json` | `author-c-l13-release-20260909-build.json` | `author-c-l13-release-20260909-ctest.json` | PASS |
| `L14_contracts` | Debug | `author-c-l14-debug-20260909-configure.json` | `author-c-l14-debug-20260909-build.json` | `author-c-l14-debug-20260909-ctest.json` | PASS |
| `L14_contracts` | Release | `author-c-l14-release-20260909-configure.json` | `author-c-l14-release-20260909-build.json` | `author-c-l14-release-20260909-ctest.json` | PASS |
| `L15_evolution` | Debug | `author-c-l15-r3-debug-20260909-configure.json` | `author-c-l15-r3-debug-20260909-build.json` | `author-c-l15-r3-debug-20260909-ctest.json` | PASS |
| `L15_evolution` | Release | `author-c-l15-r3-release-20260909-configure.json` | `author-c-l15-r3-release-20260909-build.json` | `author-c-l15-r3-release-20260909-ctest.json` | PASS |

当前冻结证据集共 24 个原始 JSON：L12 使用 r2 记录，L13—L14 使用首轮记录，L15 使用 r3 记录。全部 verdict 为 `PASS`，exit 为 `0`。目录中保留的旧 L12/L15 JSON 是返修前历史原始证据，不作为冻结判定。

## 测试含义

- L12 是观察型：验证 `std::invoke`、`std::reference_wrapper`、`std::function` 空调用异常和 const 历史缺陷、`std::move_only_function` move-only 捕获；r2 增加 `int() &` 左值限定与 `int() const noexcept` nothrow/const 调用检查。未触发空 `move_only_function` 调用。
- L13 是观察型：验证教学 `clone_value` 的深复制、多态值、const 传播、clone 失败强保证和移动后空状态。它不是 `std::indirect/std::polymorphic`。
- L14 是观察型：验证 C++23 安全模型中的输入校验、错误通道和宏观察。它不声明本机 C++26 contracts 支持，不触发 library hardening violation。
- L15 是实现型：Reference 与 validation_good 通过；validation_bad 被 `v1 default timeout remains 1000` 精确拒绝，覆盖默认实参绑定导致行为兼容破坏的单一触发面。r3 还运行 ABI 风险模型输出 `public data layout change is an ABI risk model`。

## 边界

C++26/29 前沿设施仍由 `F01_frontier` 负责能力探测。本批不重复造 probe 框架，不把教学替代类型冒充标准设施。ABI 部分只讲模型和 v1/v2 源级行为实验；未做跨编译器或旧二进制加载实验。
