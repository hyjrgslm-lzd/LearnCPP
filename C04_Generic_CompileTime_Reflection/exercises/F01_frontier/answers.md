# F01答案与判读

F01没有学生实现答案；答案是每个probe对当前工具链的能力判读。不要把SKIP改成PASS，也不要为了通过而把反射样例改成手写traits。

| probe | PASS说明 | SKIP说明 | FAIL说明 |
|---|---|---|---|
| `F01_reflection_query` | `<meta>`、`^^`、字段/枚举查询、`access_context`全部可用；查询vector只在`consteval`局部消费 | 缺`<meta>`或未显式开启反射语法 | 声称支持后源码编译、链接或运行失败 |
| `F01_splicing_generation` | `object.[:member:]`保留真实成员访问并可写回 | 缺`<meta>`或未显式开启splicing语法 | splicing表达式编译失败或写回失败 |
| `F01_annotations` | `[[= ...]]`、`annotations_of`、`annotations_of_with_type`、`is_annotation`、`extract<T>`可用；annotation vector只在`consteval`局部消费 | 缺`<meta>`或最小annotation语法正例 | annotation查询或类型提取失败 |
| `F01_expansion_statement` | `__cpp_expansion_statements >= 202506L`且`template for`可展开固定`std::array`并运行得到6 | N5050宏值不足 | 展开语句编译或运行错误 |
| `F01_define_static` | `std::define_static_string/object/array`均可用 | 缺`<meta>`或`__cpp_lib_define_static` | 静态物化后值不稳定或源码失败 |
| `F01_define_aggregate` | `data_member_spec`和`define_aggregate`生成可用聚合 | 缺`<meta>`或未显式开启生成实验 | 生成类型布局或指定成员名失败 |
| `F01_pack_indexing` | `__cpp_pack_indexing >= 202311L`且`Values...[1]`和`Ts...[1]`均可用 | N5050的C++26类型/值包宏值不足 | 类型或值pack索引失败 |
| `F01_fold_constraints` | CMake行为探测通过且重载选择符合P2963R3 | `__cpp_fold_expressions`仍为201603L且行为探测未通过 | 约束排序结果不符合预期 |
| `F01_constexpr_placement_new` | 行为探测或`__cpp_lib_constexpr_new >= 202406L`证明P2747R2，常量求值中直接placement new构造标量和aggregate；`C04_P2747_NEGATIVE_WRONG_STORAGE`应拒绝错类型存储 | 行为探测未通过且无库宏；本机正例未过时不运行标准负例 | 宏/探测宣称可用但placement new不能作为常量表达式，或负例被接受 |
| `F01_constexpr_exceptions` | `__cpp_constexpr_exceptions >= 202411L`且常量求值中throw/catch返回`std::nullopt` | N5050宏值不足 | 可用宏存在但throw/catch不能常量求值 |
| `F01_c29_conditional_noexcept_requirement` | C++29行为探测或`__cpp_concepts >= 202606L`证明`{ f() } noexcept(flag)`可按flag开关不抛要求；`C04_P3822_NEGATIVE_NON_BOOL_CONDITION`应让要求不满足 | C++29行为探测未通过 | 语法可用但throwing/non-throwing四格判断错误，或非bool条件负例被接受 |
| `F01_c29_template_name_pack_indexing` | `TT...[0]<T>`和`TT...[1]<T>`可用 | 无显式实现标记 | 模板名pack indexing失败 |
| `F01_consteval_only_values` | `__cpp_consteval >= 202606L`观察入口存在，空`std::meta::info{}`可作运行时值，非空反射留在`constexpr`对象；负例入口`C04_P4101_NEGATIVE_ESCAPE`应编译拒绝 | 缺`<meta>`或无DR实现观察标记 | consteval-only值语义实验失败 |
| `F01_attributes_reflection_proposal` | `__cpp_impl_reflection_attributes`存在，`attributes_of`、`has_attribute`、`is_attribute`和`^^[[deprecated]]`可用 | 未采纳提案且无实现标记 | 实验入口声称存在但查询失败 |
| `F01_declared_failure_control` | 不作为能力PASS使用；只在`GENERIC_STUDY_FRONTIER_ENABLE_FAILURE_CONTROL=ON`时注册 | 默认不注册 | 打印`macro=1 body=1`后返回1，验证已进入主体后的失败会报告FAIL而非SKIP |

本机MSVC 19.51没有`<meta>`时，反射、annotations、splicing、`define_static`、`define_aggregate`、consteval-only相关probe应SKIP。是否支持pack indexing、fold constraints、constexpr placement new、constexpr exceptions和C++29 requires改动必须看各自宏、CMake行为探测和真实源码，不能由`<meta>`缺失推断。
