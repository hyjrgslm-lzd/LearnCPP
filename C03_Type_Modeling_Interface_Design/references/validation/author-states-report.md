# C03 states author report: L03-L05

日期：2026-09-09。范围只包含：

- `chapters/03-optional-and-empty-state.md`
- `chapters/04-variant-and-state-space.md`
- `chapters/05-expected-and-error-channels.md`
- `exercises/L03_optional`
- `exercises/L04_variant`
- `exercises/L05_expected`

未修改公共构建函数、根导航、其他章节、F01 或共享工具。

## 覆盖

L03 覆盖 `std::optional` 的空/有状态、内部对象生命期、`reset`、`emplace` 构造失败后空状态、`value` 与 `*`/`->` 的访问前提、`value_or` 默认实参求值和值类别、C++23 `and_then`/`transform`/`or_else` 返回要求、短路和异常传播。C++26 `optional<T&>` 明确限定为 C++26，讲非拥有、重绑定、拒绝临时悬垂；optional 0/1 range 讲访问形式、所有权和 borrowed 边界，并指向 F01 真实探测。

L04 覆盖 `std::variant` 的封闭状态空间、默认构造和 `monostate`、构造选择、重复类型按索引访问、`get`/`get_if`/`holds_alternative` 前提、单/多 variant `visit` 穷尽分发与返回类型、`emplace` 抛异常和 `valueless_by_exception` 的实现允许差异、copy/move 能力来源、C++26 member visit 边界。

L05 覆盖 `std::expected<T,E>` 与 `expected<void,E>`、`unexpected` 构造、`*`/`->`/`error` 前提、`value` 的定义好抛异常路径、`value_or`/`error_or` 默认实参求值、错误码和异常的分层边界、工厂函数与错误映射、`exception_ptr` 到 C09/C10 的桥接、C++23 `and_then`/`transform`/`or_else`/`transform_error` 的短路、返回要求、异常传播、move-only 成功值和 `reference_wrapper` 借用限制。C++23 `expected<T&, E>` 错误说法已删除，复用非作者负例证据 `references/validation/reviews/states-independent-evidence/expected-ref-negative-build-r2.json`。

三套练习均为观察型，不设置学生占位文件。目标名为 `L03_optional_observation`、`L04_variant_observation`、`L05_expected_observation`，使用本课现有 `c03_add_observation`，通过 `check.hpp` 做进程级断言。

## 验证

所有命令通过 C02 `record_process.py` 记录，工作目录为 `F:\CPPTrain\LearnCPP`。

| 单元 | 配置 | configure | build | ctest |
|---|---|---|---|---|
| L03 | Debug | `author-states-l03-configure-debug.json` PASS | `author-states-l03-build-debug.json` PASS | `author-states-l03-ctest-debug.json` PASS |
| L03 | Release | `author-states-l03-configure-release.json` PASS | `author-states-l03-build-release.json` PASS | `author-states-l03-ctest-release.json` PASS |
| L04 | Debug | `author-states-l04-configure-debug.json` PASS | `author-states-l04-build-debug-r2.json` PASS | `author-states-l04-ctest-debug.json` PASS |
| L04 | Release | `author-states-l04-configure-release.json` PASS | `author-states-l04-build-release.json` PASS | `author-states-l04-ctest-release.json` PASS |
| L05 | Debug | `author-states-l05-configure-debug.json` PASS | `author-states-l05-build-debug.json` PASS | `author-states-l05-ctest-debug.json` PASS |
| L05 | Release | `author-states-l05-configure-release.json` PASS | `author-states-l05-build-release.json` PASS | `author-states-l05-ctest-release.json` PASS |

已关闭问题：`author-states-l04-build-debug.json` 首轮 FAIL，原因是观察程序用 `variant<Rectangle, Circle>` 断言不可默认构造，但 `Rectangle` 有默认成员初始化。修复为专用 `NoDefault` 替代项后，Debug build r2 与后续测试通过。失败证据保留，不作为通过记录。

## 非作者 ITERATE 修复

审查记录：`references/validation/reviews/states-independent-review.md`。

修复项：

- 删除 C++23 `expected<T&, E>` 可用的错误表述，改为 `expected<reference_wrapper<T>, E>`、指针、迭代器或回调访问；补充不延长对象生命期、`const reference_wrapper<T>` 不等于 `reference_wrapper<const T>`。
- 把 `value()` 从“访问前提”组拆出：`expected::value()` 错误状态抛 `bad_expected_access<E>` 是定义行为；`operator*`、`operator->` 和 `error()` 才有对应状态前提。同步检查并修正 L03 optional 的同类标题表述。
- L05 观察程序新增 `reference_wrapper` 借用正例；不重造 `expected<T&, E>` 负例，复用审查者已有编译负例。

复验只跑 L05 Debug/Release：

| 单元 | 配置 | configure | build | ctest |
|---|---|---|---|---|
| L05 | Debug | `author-states-l05-configure-debug-r2.json` PASS | `author-states-l05-build-debug-r3.json` PASS | `author-states-l05-ctest-debug-r3.json` PASS |
| L05 | Release | `author-states-l05-configure-release-r2.json` PASS | `author-states-l05-build-release-r2.json` PASS | `author-states-l05-ctest-release-r2.json` PASS |

已关闭返修自检问题：`author-states-l05-build-debug-r2.json` 首轮 FAIL，原因是在正例观察程序中写了 `std::is_constructible_v<std::expected<int&, AppError>, int&>`，该表达式本身会实例化被标准禁止的 `expected<T&, E>`。已删除这条正例内编译探测，继续复用非作者负例证据验证该非法接口。

修复后 SHA256：

| File | SHA256 |
|---|---|
| `chapters/03-optional-and-empty-state.md` | `EA34F5229609C7E43F1A9575970B70BBD24F16A392E5C4C136C9A0EF0F21EED5` |
| `chapters/05-expected-and-error-channels.md` | `94A900A718E2379EC2DB8786BB6CE996A2855E4484395E9736DB18625F62BD71` |
| `exercises/L05_expected/README.md` | `572AE2521985FCFC68812EEC41A9D2768AC9F957E2050F43A511E02BE3662142` |
| `exercises/L05_expected/observation/expected_observation.cpp` | `F7011C81E6AFFD3C28B319754CD20A6CBDB3E2E16064367D0CFE4287CFCCF8E0` |

## 边界

本批只验证 L03-L05 叶级 MSVC Debug/Release。未跑整课矩阵、ASan 和 frontier；C++26/C++29 能力仍由 F01 单独探测。`valueless_by_exception` 观察绑定当前 MSVC STL 和当前替代项组合，不推广为所有实现和所有操作。
