# 标准、缺陷修正与实现状态

核对日期：2026-09-10。**默认构建C++26不表示标准库实现了全部C++26，更不能用编译选项判断某功能首次所属标准。** 正文分别标出标准语义、追溯缺陷修正、特定实现观察和未验证实验。精确前沿能力以F01实际探测及运行记录为准。

## 已核对的历史边界

| 主题 | 规范边界与一手来源 | 对本课的影响 |
|---|---|---|
| split/lazy_split | [N4893编辑报告，LWG poll 20](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/n4893.html)将[P2210R2](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p2210r2.html)作为C++20缺陷修正采纳 | 旧split设计和修订后split是历史实现差异，不能假定切换-std=c++20就恢复旧行为。修订后split对连续底层产生连续subrange；lazy_split保留不同设计目标。 |
| enumerate | [P2164R9](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2164r9.pdf)、[GCC实际合入记录](https://gcc.gnu.org/pipermail/gcc-cvs/2023-April/380742.html) | enumerate属于C++23；feature宏为202302L。后续reserve_hint变化不能倒推整个view属于C++26。 |
| 算法函数对象 | [P3136R1 Retiring niebloids](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p3136r1.html)、[libc++ C++26状态](https://libcxx.llvm.org/Status/Cxx26.html) | C++20/23的特殊查找规则、库的函数对象实现和C++26明确算法函数对象要求分别解释；算法对象与支持用户定制的访问CPO不能混为同一契约。 |
| const iterator | [const.iterators.alias](https://eel.is/c++draft/const.iterators.alias) | iter_const_reference_t使用common_reference_t，不能对任意解引用结果强制返回const value_type&。按值/proxy结果必须独立检查。 |
| generator句柄 | [coroutine.handle.con](https://eel.is/c++draft/coroutine.handle.con)、[generator](https://eel.is/c++draft/coro.generator) | coroutine_handle是非拥有句柄；默认移动不会替拥有帧的包装器清空源对象。generator首次begin启动生产，++推进下一值。 |
| view对象与元素 | [range.view](https://eel.is/c++draft/range.view)、[range.owning](https://eel.is/c++draft/range.owning) | view可以拥有元素。borrowed只讨论迭代器与range对象之间的依赖，不延长被借用owner的寿命。 |
| as_input命名 | [N5047 LWG Poll14](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5047.html)、[P3828R1](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/p3828r1.pdf) | 2026-03采纳将to_input更名为as_input；旧名只作历史路径，当前主体与宏使用as_input，不静默别名代替。 |
| filter的新const支线 | [N5047 LWG Poll13](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5047.html)、[P3725R3](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/p3725r3.pdf) | 作为DR加入受约束的const begin/end：const底层可input但不可forward，且const谓词可用。普通vector底层filter仍不满足这条支线。缺少实现与规范允许必须分开。 |
| inplace_vector尝试插入返回值 | [N5047 LWG Poll19](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5047.html)、[P3981R2](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/p3981r2.html) | 2026-03采纳把try_push_back/try_emplace_back从T*改为optional<T&>；完整主体针对新签名，旧指针版另标历史。不得从P0843早期示例直接复制nullptr契约。 |

上表的eel.is链接是滚动草案：用于条款定位，历史年代须看固定提案及编辑报告。C++26/C++29内容分别以[全局计划](../../LEARNCPP_GLOBAL_PLAN.md)列明的固定草案及本次能力记录为准；未经核对不得把提案写成已采纳。

## 本机观察

本批基线MSVC前端19.51.36256.0，安装工具目录版本14.51.36231，STL宏`_MSVC_STL_VERSION=145`、`_MSVC_STL_UPDATE=202604L`，CMake4.2.3。前端版本、工具目录版本和库版本是三个字段，不相互替代。

头文件中可见enumerate、fold、zip、chunk、ranges::to等特性宏，只能证明实现声明；必须继续实例化、链接与运行。本批不承诺GCC/Clang完整C++26构建；GCC13.2仅用于冻结的C++20编译反例。

## 前沿登记规则

### C++29已入稿与未采纳方向

[N5055编辑报告](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5055.html)明确N5054是C++29工作草案，LWG Poll4纳入[P3052R2 view_interface::at](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/p3052r2.html)，Poll16纳入[P3091R6关联容器lookup](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/p3091r6.html)。F01保留这两个真实主体与能力检查；旧提案的get名称不作为当前接口。

[P3933R1 constexpr hive](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/p3933r1.html)面向C++29，当前所核对的采纳报告未将其列为已入稿。本课只登记提案与原型线索，不能把hive的运行期支持写成constexpr支持。模板后置++默认生成等语言增量由C04主讲；本课既有iterator仍手写所需操作，不用未支持语法掩盖概念要求。

flat_map/flat_set作为已有标准容器融入主线；inplace_vector/hive、concat/cache_latest/as_input（旧名to_input）/approximately_sized_range与reserve_hint提供独立真实主体。头文件缺失、特性宏不足、实例化失败、链接失败和运行失败分别记录。只有缺能力才允许SKIP；启用后主体报错不能转成SKIP。C++29只登记明确已入稿增量；Unicode转码等其他课程主讲主题采用桥接，不根据目标年份虚构views::as_utf8/16/32标准接口。
