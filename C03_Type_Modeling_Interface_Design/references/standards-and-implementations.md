# C03 规范与实现状态

核对日期：2026-09-09。C03 固定三条版本轴：C++23 使用 [N4950](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/n4950.pdf)；C++26 使用 [N5050](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5050.pdf)，由 [N5051](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5051.html) 说明其作为 C++26 DIS 基础；C++29 工作草案使用 [N5054](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5054.pdf)，由 [N5055](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5055.html) 记录 Brno 2026 后纳入项。

本页区分“规范归属”和“本机实现支持”。宏存在、头文件存在、样例能编译、样例能链接运行分别是不同证据；缺实现不删除课程正文，只进入 `F01_frontier` 能力探测和 SKIP 记录。feature-test 宏值以 [SD-6 standing document](https://isocpp.org/std/standing-documents/sd-6-sg10-feature-test-recommendations) 为当前发布来源；提案 wording 中仍保留 `20XXXXL` / `2025026XXL` 占位时，不把占位当固定宏值。

## 本机标准库输入

本机只读核对到的 MSVC STL 输入为：

- MSVC tools: `D:\VisualStudio2026\Installed\VC\Tools\MSVC\14.51.36231`
- Include root: `D:\VisualStudio2026\Installed\VC\Tools\MSVC\14.51.36231\include`
- `_MSVC_STL_VERSION=145`
- `_MSVC_STL_UPDATE=202604L`

本机头文件指纹只绑定当前安装内容，不等同于某个 GitHub 提交。若正文做源码导读，应另行固定上游仓库链接或发行版本，并说明与本机头文件的关系。

## 设施状态表

| 设施 | 规范归属与来源 | feature-test 宏 | 本机状态 | C03 使用边界 |
|---|---|---:|---|---|
| `std::optional` monadic | C++23，[P0798R8](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p0798r8.html) | `__cpp_lib_optional=202110L` | 本机 `yvals_core.h` 提供 | 主线可运行；讲 `and_then` 返回 optional、`transform` 包装值、`or_else` 只处理空状态。 |
| `std::expected` | C++23，[P0323R12](https://www.open-std.org/JTC1/SC22/WG21/docs/papers/2022/p0323r12.html) | `__cpp_lib_expected=202211L` | 本机提供 | 主线可运行；讲 value/error 双通道和 `unexpected<E>`。 |
| `std::expected` monadic | C++23，[P2505R5](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2505r5.html) | `__cpp_lib_expected=202211L` | 本机提供 | 主线可运行；讲 `and_then/transform/or_else/transform_error`，不得把错误映射成空 optional。 |
| `std::move_only_function` | C++23，[P0288R9](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p0288r9.html) | `__cpp_lib_move_only_function=202110L` | 本机提供 | 主线可运行；讲 move-only、cv/ref/noexcept 签名、空调用前置条件。 |
| `std::optional<T&>` | C++26，[P2988R12](https://wg21.link/p2988r12) | `__cpp_lib_optional=202506L` | 本机仅 `202110L`，未提供 | 前沿正文与能力探测；重点讲非拥有引用、assignment rebind、禁止临时悬垂。 |
| optional range support | C++26，[P3168R2](https://wg21.link/p3168r2) | `__cpp_lib_optional_range_support=202406L` | 本机未找到 | C03 讲 0/1 值类型接口；C06 桥接 view/range 组合。 |
| `std::variant::visit` member | C++26，[P2637R3](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2637r3.html) | `__cpp_lib_variant=202306L` | 本机 `202106L`，未提供 | C03 variant 章节纳入；它是成员形式转发到非成员 `std::visit` 的易用性增量。 |
| `std::copyable_function` | C++26，[P2548R6](https://wg21.link/p2548r6) | `__cpp_lib_copyable_function=202306L` | 本机未找到 | 前沿正文与能力探测；讲 copyable wrapper、目标 copy-constructible、修正 `std::function` const bug。 |
| `std::function_ref` | C++26，[P0792R14](https://wg21.link/p0792r14)；MSVC STL [#3788](https://github.com/microsoft/STL/issues/3788) 仍为 Investigating | `__cpp_lib_function_ref=202604L` | 本机未找到 | 前沿正文与能力探测；非拥有 callable reference，不能延长 callable 生命周期。 |
| `std::indirect` | C++26，[P3019R14](https://wg21.link/p3019r14) | `__cpp_lib_indirect=202502L` | 本机未找到 | 前沿正文与能力探测；动态分配对象的值语义，不代表 nullable。 |
| `std::polymorphic` | C++26，[P3019R14](https://wg21.link/p3019r14) | `__cpp_lib_polymorphic=202502L` | 本机未找到 | 前沿正文与能力探测；开放派生集合的值语义，复制保留动态类型。 |
| C++26 Contracts | C++26，[P2900R14](https://wg21.link/p2900r14)；P2900R14 wording 写 `20XXXXL` 占位，SD-6 给当前值 | `__cpp_contracts=202502L`, `__cpp_lib_contracts=202502L` | 本机未找到 | 讲接口前置/后置/断言；语言语法与 `<contracts>` 库 API 分开，不替代外部输入验证。 |
| STL hardening | C++26，[P3471R4](https://wg21.link/p3471r4)；MSVC STL changelog 说明默认关闭 | 无统一语言宏；MSVC 用 `_MSVC_STL_HARDENING` 控制 | 本机头文件具备版本输入；本支线只记录默认宏状态，不触发 violation | 独立于语言 contracts；库前置条件检查不是接口契约语法。 |
| C++29 virtual contracts | C++29 工作草案，[N5055](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5055.html) CWG 第 16 项投票纳入 [P3097R3](https://wg21.link/p3097r3)；P3097R3 要求 bump 但正文写 `2025026XXL` 占位/笔误，SD-6 当前列为 `202606` | `__cpp_contracts=202606L` | 本机未找到 | C03 动态多态/契约桥接；工作草案增量，不写成 C++26 主线。 |
| pattern matching | 提案跟踪，[P2688R5](https://wg21.link/p2688r5) | 无标准库支持宏 | 本机不声明支持 | 只做 variant/接口演进对照，不造 `std` 支持。 |
| quantities and units | 提案跟踪，[P2980R1](https://www.open-std.org/JTC1/SC22/WG21/docs/papers/2023/p2980r1.html)、[P3045R9](https://www.open-std.org/JTC1/SC22/WG21/docs/papers/2026/p3045r9.html)、[P4185R1](https://www9.open-std.org/JTC1/SC22/WG21/docs/papers/2026/p4185r1.html) | 无标准库支持宏 | 本机不声明支持 | C03 讲强类型/量纲建模接口；C13 讲数值与单位成本。 |

## 上游实现入口

- MSVC STL 当前公开状态以 [Microsoft C/C++ language conformance](https://learn.microsoft.com/en-us/cpp/overview/visual-cpp-language-conformance?view=msvc-170)、[microsoft/STL Changelog](https://github.com/microsoft/STL/wiki/Changelog) 和具体 issue 为准；本机头文件 SHA 不能替代这些页面。
- libstdc++ 已有 `copyable_function` 邮件记录：`r16-618-g0e93f7cd4ed0cf6bcfda90ed4dcad51a1f65b4b6`，见 GCC bug mail for PR119125。
- libstdc++ 已有 `indirect` 邮件记录：`r16-878-gcaf804b1795575d7714c62dd45b649831598055e`，见 GCC bug mail for PR119152。
- `optional<T&>` 与 optional range 的实验实现入口可看 [bemanproject/optional](https://github.com/bemanproject/optional)，但 C03 不把 Beman API 当作 `std::optional` 支持。

## F01 能力探测规则

[F01_frontier](../exercises/F01_frontier/README.md) 长期保存每个设施的独立源码。开启 `TYPE_STUDY_ENABLE_FRONTIER=ON` 后，每个测试先打印头文件/宏状态；缺宏或缺版本时返回 77，且只由该 capability 测试设置 `SKIP_RETURN_CODE 77`。一旦实现提供宏，源码会实例化真实标准接口、链接并运行；编译或运行失败即为 FAIL，不转成 SKIP。Contracts 当前只验证合法调用路径的语法/链接/运行入口，不触发 violation，不声称检查模式已验证。
