# C03 states independent r2 approval: L03/L05 affected recheck

日期：2026-09-09。角色：原非作者审查者。范围只复验返修影响面：

- `chapters/03-optional-and-empty-state.md`
- `chapters/05-expected-and-error-channels.md`
- `exercises/L05_expected/README.md`
- `exercises/L05_expected/observation/expected_observation.cpp`
- `references/validation/author-states-report.md`
- 复用 r1 负例证据 `references/validation/reviews/states-independent-evidence/expected-ref-negative-build-r2.json`

结论：APPROVE，原 HIGH/MEDIUM 阻断均已关闭。保留 1 个 LOW 级报告路径问题，不阻断 03/05/L05 技术与教学修复批准。

## 已关闭项

### r1 HIGH: C++23 `std::expected<T&, E>` 被写成可用类型

Status: CLOSED.

Evidence:

- `chapters/05-expected-and-error-channels.md:98` 现在明确写 C++23 `std::expected<T, E>` 禁止 `T` 是引用类型，并引用本机负例。
- `chapters/05-expected-and-error-channels.md:100` 改为 C++23 主线用 `std::expected<std::reference_wrapper<T>, E>`、指针、迭代器或回调访问表达借用。
- `chapters/05-expected-and-error-channels.md:102` 补充 `const std::reference_wrapper<T>` 与 `std::reference_wrapper<const T>` 的差异。
- `exercises/L05_expected/README.md:25` 明确 `expected<T&, E>` 不合法。
- `exercises/L05_expected/observation/expected_observation.cpp:44` 和 `:52` 使用 `expected<reference_wrapper<int>, AppError>` 与 `expected<reference_wrapper<const int>, AppError>` 正例。
- `exercises/L05_expected/observation/expected_observation.cpp:183`-`:199` 验证 const wrapper 不等于 const pointee、可写借用、只读借用和缺失错误状态。
- 复用 r1 编译负例：`states-independent-evidence/expected-ref-negative-build-r2.json`，MSVC 报 `T must not be a reference type. (N4950 [expected.object.general]/2)`，实例化点为 `expected_ref_negative.cpp:10`。

Official check: P0323R12 wording states `expected<T, E>` with reference `T` is ill-formed.

### r1 MEDIUM: `value()` 被归入访问前提

Status: CLOSED.

Evidence:

- `chapters/03-optional-and-empty-state.md:43` 已改成 `unchecked` 与 `checked observer` 分类。
- `chapters/03-optional-and-empty-state.md:53` 把 `*opt` / `->` 标成 unchecked observer 和 `has_value()` 前提。
- `chapters/03-optional-and-empty-state.md:55` 把 `value()` 标成 checked observer，空状态按定义抛 `bad_optional_access`。
- `chapters/05-expected-and-error-channels.md:37` 已改成“前提与定义好的抛异常路径”。
- `chapters/05-expected-and-error-channels.md:51`-`:54` 清楚区分 `*`/`->` 前提、`error()` 前提、`value()` 错误状态抛 `bad_expected_access<E>`。
- `exercises/L05_expected/README.md:17` 同步改为 observer 分类。

Official check: P0323R12 observer wording gives `operator->`/`operator*` preconditions, `value()` throw behavior, and `error()` precondition separately.

## 新发现

### [LOW] 作者报告中的 r1 负例日志路径多了一层目录

File: `C03_Type_Modeling_Interface_Design/references/validation/author-states-report.md:20`

Issue: 作者报告写 `references/validation/reviews/states-independent-evidence/expected-ref-negative/expected-ref-negative-build-r2.json`，但该路径不存在。真实日志是 `references/validation/reviews/states-independent-evidence/expected-ref-negative-build-r2.json`。`Test-Path` 已验证前者 `False`、后者 `True`。

Impact: 只影响读者从作者报告跳转复用负例证据；不影响正文技术修复、L05 正例运行或 r1 负例本身有效性。

Fix: 把作者报告里的路径改成 `references/validation/reviews/states-independent-evidence/expected-ref-negative-build-r2.json`。

## 独立复验命令与结果

工作目录：`F:\CPPTrain\LearnCPP`。

| Check | Result | Evidence |
|---|---|---|
| L05 configure | PASS | `states-independent-evidence/r2-l05-configure.json` |
| L05 Debug build | PASS | `states-independent-evidence/r2-l05-build-debug.json` |
| L05 Release build | PASS | `states-independent-evidence/r2-l05-build-release.json` |
| L05 Debug ctest | PASS, `L05_expected_observation`, `100% tests passed` | `states-independent-evidence/r2-l05-ctest-debug.json` |
| L05 Release ctest | PASS, `L05_expected_observation`, `100% tests passed` | `states-independent-evidence/r2-l05-ctest-release.json` |
| `expected<T&>` remaining scan | PASS | Only appears in illegal/negative/old-review contexts |
| workaround/security pattern scan | PASS | No new masking fallback, empty catch, hardcoded secret, or swallowed-error branch in affected files |

L04 was unchanged by this返修 and was not rerun.

## 版本绑定

Repo HEAD: `8e0f407afd155ede81f0b06a51553dadef206a23`

| File | SHA256 |
|---|---|
| `chapters/03-optional-and-empty-state.md` | `EA34F5229609C7E43F1A9575970B70BBD24F16A392E5C4C136C9A0EF0F21EED5` |
| `chapters/05-expected-and-error-channels.md` | `94A900A718E2379EC2DB8786BB6CE996A2855E4484395E9736DB18625F62BD71` |
| `exercises/L05_expected/README.md` | `572AE2521985FCFC68812EEC41A9D2768AC9F957E2050F43A511E02BE3662142` |
| `exercises/L05_expected/observation/expected_observation.cpp` | `F7011C81E6AFFD3C28B319754CD20A6CBDB3E2E16064367D0CFE4287CFCCF8E0` |
| `references/validation/author-states-report.md` | `2CDB2163CB2C2CAEAE07EE03ACDA1A55392D1B257FBC5F746D6B0AA2614E246B` |

## 验证缺口

- 未找到可调用的 `lsp_diagnostics` 或 `ast_grep_search` 工具；本轮沿用 MSVC `/W4 /permissive-` 构建诊断和 `rg` 模式扫描。
- 本复验不覆盖整课矩阵、ASan、frontier 全量复跑或未受影响的 L04。
