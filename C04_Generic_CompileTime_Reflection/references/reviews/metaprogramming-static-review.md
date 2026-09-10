# C04 07-10 元编程批次静态审查

结论：ITERATE。仅为静态阶段结论；本轮按正式编译成本采样窗口要求，没有运行 build/test/ctest，也没有把既有作者日志当作通过证据。

审查范围绑定到当前工作区文件，重点阅读：

- `LEARNCPP_GLOBAL_PLAN.md` 4.2，尤其模板/编译期系列必须能支撑 type_list、tuple/index_sequence、完成签名、常量求值、反射与下游应用。
- `CONTENT_REFACTORING_GUIDE.md` 4-8、13，尤其下游反推深度、正文/练习闭环、证据边界、独立审查闭环。
- `C04_Generic_CompileTime_Reflection/references/implementation-spec.md`。
- 被审正文：`chapters/07-packs-nttp.md`、`08-type-lists.md`、`09-tuple-traversal.md`、`10-constant-evaluation.md`。
- 被审练习：`exercises/L07_packs`、`L08_type_lists`、`L09_tuple`、`L10_constexpr`。

静态文件指纹：

| 文件 | SHA256 |
|---|---|
| `chapters/07-packs-nttp.md` | `CCBA5E27AFCFA0E786F548B4E8362CB18392928DF2994DACAA2ADD9C54D2BBC6` |
| `chapters/08-type-lists.md` | `B7DB9C0CE6014A557C54C9B63F9D07316C0225C03BF47FFE62F68F8D6F8A958A` |
| `chapters/09-tuple-traversal.md` | `F0244F23832E6D42F674B3F76172269548DA8ABFEA979B75DCEAB122890A61A7` |
| `chapters/10-constant-evaluation.md` | `A4353E6EDF152374EE7D44B9E3274007D01537AA3B1B76522FA3074EFFE5D34D` |
| `exercises/CMakeLists.txt` | `709DBC820A5180476DF7DB058C4ED5AFBBE20680C3808EA59952DC3265C0F916` |

## 阻断项

### [HIGH] L07-L10 未接入根练习构建清单，整课验证不会覆盖本批

位置：`C04_Generic_CompileTime_Reflection/exercises/CMakeLists.txt:4-8`、`C04_Generic_CompileTime_Reflection/references/implementation-spec.md:48-52`

根因：根 CMake 清单写明“authored unit is ready to build”，但 `set(lessons L11_customization)` 只注册 L11。L07-L10 各自目录有 `c04_add_exercise` 和 observation，但根 `verify-core/verify-debug` 入口不会进入这些叶级目录。规格要求核心 Debug/Release、所有叶级、Student-only 接线、good/bad 独立验证；当前静态接线已经不满足。

影响：作者即使完成 L07-L10 文件，根级验证仍可能全部绕过该批；最终报告若只看根构建会把“未注册”误报成“无失败”。

最小修复目标：在根 `exercises/CMakeLists.txt` 中把 L07-L10 纳入 lessons，或明确把它们保持为未 ready 状态并同步实施规格/交接。修复后正式验证另行审批运行。

### [HIGH] L08 正文与 README 没有把 map/filter/unique 推导到读者可实现的程度

位置：`chapters/08-type-lists.md:49-58`、`exercises/L08_type_lists/README.md:7-21`、`exercises/L08_type_lists/src/reference/type_list_tools.hpp:11-86`

根因：正文只列出 `map`、`filter`、`concat`、`unique` 的输入输出，并一句话说它们验证包展开、偏特化和递归；README 也只给“map 用模板模板参数，filter 用惰性分支，unique 保留第一次出现”。但 Reference 实际需要 forward declaration、`type_list<Ts...>` 偏特化、`push_front`、空列表基例、递归 tail、`contains`、`unique_impl<Seen, Rest>` 等步骤。按全局 4.2 和指引第 4/7/13 节，读者遮住答案后不应靠偷看 Reference 才知道算法形状。

影响：下游 completion signatures、sender adaptor、反射字段列表都会依赖这种“类型集合变换”能力。当前材料能让读者知道算法名字，不能稳定推出实现。

最小修复目标：补一段从 `type_list<int,double,int,char>` 手推 `map`、`filter`、`unique` 的完整展开；给出 `filter` 的空列表基例、递归步、保留/丢弃分支为什么只实例化所需结果；给出 `unique` 的 `Seen/Rest` 不变量。README 的 Part 1 至少指向这些步骤，而不是只写结论。

### [HIGH] L08 缺少惰性实例化的可跑正反例，无法证明诊断边界

位置：`chapters/08-type-lists.md:36-47`、`chapters/08-type-lists.md:80-84`、`exercises/L08_type_lists/checks/type_list_checks.cpp:24-53`、`exercises/L08_type_lists/validation/bad/type_list_tools.hpp:19-25`

根因：正文说“不可调用的签名变换应在 concept 上显示为不满足”，checker 也只用 `transformable<IntOnly, type_list<value_sig<string>>>` 覆盖完成签名别名在 requires 中失败。它没有提供一个实际 `filter`/`conditional_t` 相关的惰性反例：例如未选分支含不存在成员时应不实例化，错误分支被选中时才有限失败。bad 变体也只是硬编码丢失 error/stopped，不能证明 eager 实例化会被拒绝。

影响：用户指定的“惰性实例化反例、void 和错误/停止通道”里，void/error/stopped 有基础覆盖，但惰性实例化只停在文字和概念示例。读者仍无法判断“requires 失败”“别名请求失败”“深层硬错误”分别发生在哪个边界。

最小修复目标：增加一个独立 observation 或 checker 场景：构造 `Bad::missing_type` 一类危险表达式，证明未选分支不实例化；再构造被选中或直接请求 `::type` 的负例，诊断落在预期边界。bad 变体至少覆盖一种 eager 过滤/变换错误。

### [HIGH] L10 常量求值主讲链过短，缺失 immediate invocation、`is_constant_evaluated`、`string`/非瞬态存储反例和诊断闭环

位置：`chapters/10-constant-evaluation.md:7-43`、`chapters/10-constant-evaluation.md:45-49`、`exercises/L10_constexpr/README.md:5-21`、`exercises/L10_constexpr/checks/constexpr_tools_checks.cpp:7-18`

根因：本章目标声称区分 `constexpr`、`consteval`、`if consteval` 和 `std::is_constant_evaluated()`，并讲对象/存储限制。实际正文只有三段核心规则：`constexpr` 可运行时调用、`consteval` 运行时值不良构、`if consteval` 与 `is_constant_evaluated` 的一句对比。练习只覆盖 `decimal_value`、临时 `vector` 求和、`phase_value` 的运行时/常量分支。没有 immediate invocation/immediate function context 的传播边界，没有 `std::is_constant_evaluated()` 的试探性常量求值正反例，没有 `std::string` 或非瞬态存储失败的可跑/可隔离反例，也没有 invalid digit 的诊断检查。

影响：这达不到全局 4.2 对 constexpr/consteval、对象与存储限制、编译期算法/容器、诊断的下限。后续反射章节已经会遇到常量求值范围、`define_static_*`、consteval-only 值和 `vector<info>` 暂存边界；L10 目前不能作为这些内容的可靠先修。

最小修复目标：把 L10 拆成最小但完整的推导链：manifestly constant-evaluated context、immediate invocation 与 immediate function context、`constexpr` 函数运行时调用边界、`if consteval` 与 `std::is_constant_evaluated()` 的差别、临时 `vector`/`string` 成功例、逃逸指针或持久化分配的隔离负例、`consteval` 解析失败的诊断位置。checker/diagnostic case 至少覆盖 invalid digit 和非瞬态存储中的一个。

### [MEDIUM] L10 对 `consteval` 的表述容易被读成“任意 consteval 调用都只看实参是否常量”，版本/传播边界未写清

位置：`chapters/10-constant-evaluation.md:18-20`、`references/standards-and-implementations.md:7-9`、`chapters/15-annotations-frontier.md:51-53`

根因：正文说“`consteval` 函数必须立即常量求值。传入运行时值不良构”，但没有把调用点是否是 immediate invocation、调用是否位于 immediate function context、以及 C++26/DR 对 consteval-only 值的影响分开。规范索引已经固定 N4950/N5050/N5054/N5055，并把 P4101R1 作为 C++26 DR 记录；前沿章节也会讲 consteval-only values。L10 作为先修应先给出精确边界，避免后文读者把所有场景压成“参数常量/非常量”二分。

影响：反射和生成章节中的 `consteval auto fields_of(info type)`、`std::meta::info` 暂存、`define_static_array` 等都会依赖这条边界。表述不精确会直接污染后续标准版本归属和诊断解释。

最小修复目标：在 L10 加一个短小但准确的版本框：C++23 核心规则、C++26/DR 另在前沿章节跟踪；用两三个调用位置说明 immediate invocation 与 immediate function context，而不是只按实参来源判断。

### [MEDIUM] L07 章节承诺与练习/checker 不一致：包计数没有实现入口

位置：`chapters/07-packs-nttp.md:11-19`、`chapters/07-packs-nttp.md:94-98`、`exercises/L07_packs/README.md:5-27`、`exercises/L07_packs/checks/pack_tools_checks.cpp:17-38`

根因：正文先讲 `count_types`/`count_values`，末尾又说 L07 要实现“包计数、空包 identity、四种 fold ...”。但 README 和 checker 只要求 `all_true`、`any_true`、`sum_values`、`left_subtract`、`right_subtract`、`call_in_order`、NTTP/fixed_string/template-template；Student/Reference 也没有 `count_types` 或 `count_values`。

影响：这是承诺/练习/检查器不一致。它不如 L08/L10 的深度缺口严重，但会破坏“逐 Part 的任务、Reference、有效检查和解析相互对应”的验收口径。

最小修复目标：二选一：补 `count_types`/`count_values` 到 README、Student、Reference、good/bad 或 checker；或从章节练习承诺中删去“包计数”，保留为正文观察点。

## 静态通过项

- L09 的教学链相对闭合：正文说明 `std::get` 保留 cv/ref、`index_sequence` 两层展开、`F&` callback 复用、`std::forward<Tuple>`、逗号 fold 左到右、异常停止但不回滚副作用；checker 覆盖空 tuple、顺序、同一 callback、引用写回、const 引用、move-only 右值、异常传播和副作用边界。静态阶段未发现 L09 阻断。
- L07 的 fold identity、fold 方向、逗号顺序、NTTP/fixed_string/template-template 主线基本可读；阻断仅为“包计数”承诺不匹配及根级接线。
- Student/Reference/good/bad 的叶级结构使用 `c04_add_exercise`，静态看能独立选择实现目录；但根级未注册导致整课入口绕过本批。

## 未执行验证

- 未运行 configure/build/ctest/Student-only/diagnostic cases：当前处于正式编译成本采样窗口，任务明确禁止任何 build/test。
- 未运行 lsp diagnostics：本任务是非 coding 教学/技术静态审查，且当前工具面没有适用于 Markdown/CMake/C++ 叶级的 lsp_diagnostics；本报告只做源码/规范静态判断。

## Recommendation

ITERATE。先修复根级接线、L08 算法与惰性反例、L10 常量求值主讲与反例，再进入作者自检和后续非作者复验。L09 可作为本批中当前较接近放行的单元，但不能因为 L09 闭合而批准 07-10 整批。
