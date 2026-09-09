# C03 states independent review: L03-L05

日期：2026-09-09。角色：非作者技术/教学/实验审查。范围：

- `chapters/03-optional-and-empty-state.md`
- `chapters/04-variant-and-state-space.md`
- `chapters/05-expected-and-error-channels.md`
- `exercises/L03_optional`
- `exercises/L04_variant`
- `exercises/L05_expected`
- 作者证据 `references/validation/author-states-*`
- 前沿能力证据 `references/validation/capabilities/*`

结论：ITERATE。

## 阻断问题

### [HIGH] C++23 `std::expected<T&, E>` 被写成可用类型

File: `C03_Type_Modeling_Interface_Design/chapters/05-expected-and-error-channels.md:98`

Issue: 正文写道 C++23 `expected<T&, E>` 可用于成功时借用对象。但 C++23 `std::expected` 禁止 `T` 为引用类型。P0323R12 的 wording 明确说实例化 `expected<T, E>` 时 `T` 为 reference type 是 ill-formed；本机 MSVC 也在负例中报 `T must not be a reference type. (N4950 [expected.object.general]/2)`。

Impact: 读者会把不存在的标准接口用于借用返回，后续接口设计会编译失败。该错误正好落在 L05 的核心主题：错误通道与所有权/借用边界。

Fix: 删除 `expected<T&, E>` 可用说法，改为 C++23 主线使用 `std::expected<std::reference_wrapper<T>, E>`、指针、迭代器或回调访问来表达借用；同时说明这些形式不延长被借用对象寿命。建议保留一个 compile-fail 负例或 F01 能力项，证明 `expected<int&, E>` 不是当前标准能力。

Evidence:

- Official: P0323R12 `expected` wording: reference `T` instantiation is ill-formed.
- Local compile negative: `references/validation/reviews/states-independent-evidence/expected-ref-negative/expected_ref_negative.cpp:10`
- Local log: `references/validation/reviews/states-independent-evidence/expected-ref-negative-build-r2.json`

### [MEDIUM] `value()` 被归入访问前提，容易把定义行为讲成前提违约

File: `C03_Type_Modeling_Interface_Design/chapters/05-expected-and-error-channels.md:49`

Issue: 本节用“前提要分清”统领 `*e`、`e->`、`e.value()`、`e.error()`，并在 `05-expected-and-error-channels.md:52` 写 `e.value()` 要求成功。标准里 `operator*`/`operator->` 有 `has_value()` 前提；`value()` 在错误状态下是定义行为，会抛 `bad_expected_access<E>`；`error()` 在成功状态下才是前提违约。练习 README 的 Part 3 也用“访问前提”标题描述 `value()` 抛异常。

Impact: 读者可能把 `value()` 的可捕获错误路径和 `operator*` 的前提违约混为一类，削弱本章承诺的访问语义区分。

Fix: 拆成两组：`*`/`->`/`error()` 是有前提的 observer；`value()` 是 checked observer，错误状态抛 `bad_expected_access<E>`；`value_or()`/`error_or()` 没有访问前提问题，但有默认实参先求值和值类别/转换要求。同步更新 L05 练习 README 的标题或说明。

Evidence:

- Official: P0323R12 observer wording: `operator->`/`operator*` 有 `has_value()` 前提；`value()` 在无值时抛 `bad_expected_access`；`error()` 有 `!has_value()` 前提。
- Current text: `chapters/05-expected-and-error-channels.md:49`, `:52`, `:117`; `exercises/L05_expected/README.md:17`
- Local positive check: `expected_observation.cpp:85`-`:89` 确实验证了 `value()` 错误状态抛异常，代码行为正确，正文分类需要修。

## 已验证通过项

- L03 正文覆盖空/有状态、对象生命期、`reset`、`emplace` 失败后空状态、`value` 与 `*`/`->`、`value_or` 默认实参求值和值类别、C++23 monadic 短路与异常传播、C++26 optional reference/range 边界。
- L04 正文覆盖封闭状态空间、默认构造与 `monostate`、构造选择、重复类型索引访问、`get`/`get_if`/`holds_alternative`、单/多 `visit`、返回类型、`valueless_by_exception` 的实现边界、copy/move、C++26 member visit。
- L05 除上述两处外，正文覆盖 `expected<T,E>`/`expected<void,E>`、`unexpected`、never-empty 不等于不抛、错误分类、异常/错误映射、`exception_ptr`、monadic 短路与异常传播、move-only 成功值和 const 访问限制。
- 三个观察程序没有发现空 catch、硬编码 secret、静默 fallback、吞错或绕过主路径的 masking workaround。

## 独立运行证据

工作目录：`F:\CPPTrain\LearnCPP`。工具链来自 CMake configure：Visual Studio 18 2026 / MSVC，C++23。

| 单元 | Configure | Debug build | Debug ctest | Release build | Release ctest |
|---|---|---|---|---|---|
| L03_optional | PASS `states-independent-evidence/l03-configure.json` | PASS `l03-build-debug.json` | PASS `l03-ctest-debug.json` | PASS `l03-build-release.json` | PASS `l03-ctest-release.json` |
| L04_variant | PASS `states-independent-evidence/l04-configure.json` | PASS `l04-build-debug.json` | PASS `l04-ctest-debug.json` | PASS `l04-build-release.json` | PASS `l04-ctest-release.json` |
| L05_expected | PASS `states-independent-evidence/l05-configure.json` | PASS `l05-build-debug.json` | PASS `l05-ctest-debug.json` | PASS `l05-build-release.json` | PASS `l05-ctest-release.json` |

负例：

- `expected-ref-negative-configure.json`: PASS。
- `expected-ref-negative-build-r2.json`: PASS as expected failure, exit 1, required text `T must not be a reference type` and `std::expected<int &,Error>` present.
- `expected-ref-negative-build.json`: retained first reviewer recording attempt; command failed as expected but the expected text was too broad, so the recorder verdict is FAIL. Superseded by r2.

## 版本绑定

Repo HEAD: `8e0f407afd155ede81f0b06a51553dadef206a23`

| File | SHA256 |
|---|---|
| `chapters/03-optional-and-empty-state.md` | `152F871CFA528F4A436C954FF5F8737A987867FF884F253BC1AD047554528F3B` |
| `chapters/04-variant-and-state-space.md` | `9B413003538A5F460CB3D282D791C6F82110BB0303174F4082907B9BD0E7663E` |
| `chapters/05-expected-and-error-channels.md` | `BBED646DE414A25E8CBF60D0DCA5117B82C8BB49EB5EDDB8DB999CB07FA19A5D` |
| `exercises/L03_optional/observation/optional_observation.cpp` | `DDC7F66A9B111FE481F44AA2E3F66668095E20A390ACB02D632413BE6F8BDBD0` |
| `exercises/L04_variant/observation/variant_observation.cpp` | `8728EC496667CF12C891D13043FAEFFD7B0CB70C3D61C30677EF68F96653A0C3` |
| `exercises/L05_expected/observation/expected_observation.cpp` | `D48AF56A3344CA02FE7DFB06AEEA4BCD5A1CC8FF50B8E46CBCA62BAAB2FF9E7F` |
| `reviews/states-independent-evidence/expected-ref-negative/expected_ref_negative.cpp` | `518EA4741C68679D1658523F76A6E45E2A50E6EB18E27E592ACEF648FCE84715` |

## 验证缺口

- 未找到可调用的 `lsp_diagnostics` 或 `ast_grep_search` 工具；已用 MSVC `/W4 /permissive-` 构建诊断和 `rg` 模式扫描替代。
- 本审查不覆盖整课矩阵、ASan、frontier 全量复跑或 00-02/06-17；这些仍按总质量报告门槛处理。
