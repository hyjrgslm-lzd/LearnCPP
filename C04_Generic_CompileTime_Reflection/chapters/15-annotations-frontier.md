# 15. Annotations与前沿分层

C++26 annotations来自[P3394R4](https://wg21.link/p3394r4)。语法形态像attribute：

```cpp
struct skip_format {
    bool value;
};

struct User {
    int id;
    [[= skip_format{true}]] std::string password_hash;
};
```

但annotation不是传统attribute。`[[nodiscard]]`、`[[deprecated]]`这类attribute影响诊断、优化、ABI或实现约定；annotation保存一个常量表达式，供反射查询读取。`annotations_of(^^User::password_hash)`得到annotation元信息，`extract<skip_format>(...)`取回强类型值。`annotations_of_with_type(item, ^^skip_format)`按annotation的类型过滤，并保持顺序。

## annotation不是schema魔法

annotation适合表达“这个字段在某个投影里如何处理”。例如格式化时跳过密码哈希：

```cpp
template<class T>
void debug_print(T const& object) {
    template for (constexpr auto m :
        std::define_static_array(
            std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current()))) {
        if constexpr (std::meta::annotations_of_with_type(m, ^^skip_format).empty()) {
            std::println("{}={}", std::meta::identifier_of(m), object.[:m:]);
        }
    }
}
```

这不等于序列化也跳过该字段。格式化是展示投影；codec是数据契约。课程P1明确规定codec全部字段往返，annotations只进入格式化示例。

## C++26、C++29和未采纳提案分开

本课前沿材料按三层写，不混用年份：

| 层级 | 设施 | 本课处理 |
|---|---|---|
| C++26草案/N5050 | 反射、splicing、annotations、展开语句、pack indexing、fold expanded constraints、constexpr exceptions | 有规范讲解和独立probe；本机能力不足时记录SKIP |
| C++26草案/N5050库与常量求值 | P2747R2 constexpr placement new | 单独probe；`std::construct_at`对照，不把数组/错类型负例吞成SKIP |
| C++26 DR / N5055记录 | P4101R1 consteval-only values | 单独probe与说明，不当作新语言功能 |
| C++29方向 | P3670R4模板名pack indexing、P3822R2 requires复合要求的conditional `noexcept`；P3385R8属性反射是未采纳提案 | 已入草案/已投方向和未采纳提案分开；只按真实语法实验报告 |

Pack indexing来自[P2662R3](https://wg21.link/p2662r3)，语法是`Ts...[I]`或`args...[I]`，解决“取参数包第I项”。C++29的[P3670R4](https://wg21.link/p3670r4)补的是模板名包，例如`TT...[0]<T>`。这两者不是同一个能力。

Fold expanded constraints来自[P2963R3](https://wg21.link/p2963r3)。它改变约束归一化和包含关系，让`(C<Ts> && ...)`这类约束能参与更准确的subsumption。实验要检查重载选择，而不只是宏存在。

Constexpr exceptions来自[P3068R6](https://wg21.link/p3068r6)，N5050表22对应feature-test macro为`__cpp_constexpr_exceptions 202411L`。目标是允许常量求值中抛出并捕获异常，未捕获异常仍使常量表达式失败。反射查询错误能因此用更自然的错误通道表达；它不把UB变成可catch异常。

Constexpr placement new来自[P2747R2](https://wg21.link/p2747r2)，N5050/N5054宏表中的语言宏线索是`__cpp_constexpr 202406L`，库宏是`__cpp_lib_constexpr_new 202406L`。它补的是常量求值中直接写`::new (p) T(...)`，让默认初始化、列表初始化、指定初始化和数组形式不必绕成`std::construct_at`。边界仍然严格：placement实参要指向本次常量求值中开始的存储，且要和目标对象类型相容；`new (p + 1) int[]{...}`这类数组错位/尺寸不符负例仍应拒绝。本机若连正例都未通过，报告只能写“负例入口保留，未运行”，不能把未测负例写成PASS。

C++29的[P3822R2](https://wg21.link/p3822r2)把requires复合要求里的`noexcept`从“只要出现就是要求不抛”扩展为`noexcept(constant-expression)`。当常量表达式为`true`时，表达式必须是non-throwing；为`false`时，不抛要求关闭；条件不能转换为`bool`时，该要求不满足。N5054把`__cpp_concepts`提升到`202606L`。本课只用小concept检查语法和语义，不把函数声明上的`noexcept(expr)`误当成复合要求支持。

P4101R1把C++26反射中的“consteval-only type”问题推向“consteval-only value”模型。对课程读者的实际影响是：不要尝试把非空`std::meta::info`、指向immediate对象或immediate函数的值逃逸到运行时。`std::meta::info{}`空值和包含`std::meta::info`类型的普通对象在DR后语义会更细，实验报告必须写清楚用的是哪个草案/实现。

P4101R1的feature-test macro文字仍写作把`__cpp_consteval`从`202406L`提升到`20XXXXL`，没有给最终数值。本课probe把`__cpp_consteval >= 202606L`作为当前实现观察入口；这不是paper固定常量。真正的DR主体检查两件事：空`std::meta::info{}`可作为普通运行时值，非空`^^int`只能留在`constexpr`等允许的immediate对象中。源码还保留`C04_P4101_NEGATIVE_ESCAPE`入口，用来确认`auto escaped = ^^int;`这类非空反射逃逸应被拒绝。

P3385R8属性反射和P3394 annotations不同。P3385想让程序查询现有attribute；P3394则是新增可提取的annotation值。一个是观察attribute，另一个是给反射附加用户数据。本课不能用`[[deprecated]]`查询伪装成annotation练习，也不能说P3385已经是C++29标准。P3385R8当前API线索是`std::meta::attributes_of`、`std::meta::has_attribute`、`std::meta::is_attribute`以及反射attribute的`^^[[deprecated]]`语法，宏名是`__cpp_impl_reflection_attributes`，提案写的是占位值`2026XXL`。

## F01实验的停止条件

`F01_frontier`不是“学生把反射写完”的作业，而是一组能力观察：

- `reflection_query_probe.cpp`：`<meta>`、`^^`、字段/枚举查询、`access_context`；
- `annotations_probe.cpp`：`[[= ...]]`、`annotations_of`、`annotations_of_with_type`、`extract<T>`；
- `expansion_statement_probe.cpp`：`template for`独立展开；
- `define_static_probe.cpp`：`std::define_static_string/array/object`；
- `define_aggregate_probe.cpp`：`data_member_spec`和`define_aggregate`；
- `pack_indexing_probe.cpp`：C++26类型和值pack indexing；
- `fold_constraints_probe.cpp`：fold expanded constraints的重载排序；
- `constexpr_placement_new_probe.cpp`：P2747R2常量求值中直接placement new；
- `constexpr_exceptions_probe.cpp`：常量求值throw/catch；
- `c29_conditional_noexcept_requirement_probe.cpp`：P3822R2复合要求里的conditional `noexcept`；
- `c29_template_name_pack_indexing_probe.cpp`：C++29模板名包索引；
- `consteval_only_values_probe.cpp`：P4101R1 DR语义；
- `attributes_reflection_proposal_probe.cpp`：P3385R8未采纳提案的实验规格。

每个源文件独立编译、独立运行、独立记录。OFF表示`GENERIC_STUDY_ENABLE_FRONTIER=OFF`没有注册；SKIP表示本项能力缺失或没有明确宏；PASS表示真实源码编译、链接、运行成功；FAIL表示能力宣称存在但源码失败。这个分类比“这台机器没有反射”更麻烦，但能防止错误结论传给后续章节。
