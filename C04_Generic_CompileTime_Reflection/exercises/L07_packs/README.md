# 练习 L07：参数包、fold 和 NTTP

先读 `../../chapters/07-packs-nttp.md`。只编辑 `src/student/pack_tools.hpp`。

## Part 1：包计数

实现 `count_types<Ts...>` 和 `count_values<Vs...>`，分别返回类型包和值包元素个数。

解析：`sizeof...(pack)` 是读取包大小，不展开包。

## Part 2：fold identity

实现 `all_true<Vs...>`、`any_true<Vs...>`、`sum_values<Vs...>`。空 `all_true<>` 必须为 `true`，空 `any_true<>` 为 `false`，空求和为 `0`。

解析：`&&` 和 `||` 的一元 fold 有语言 identity；求和要写二元 fold。

## Part 3：四种 fold 可区分

实现 `left_subtract<Vs...>` 和 `right_subtract<Vs...>`，用二元 fold 明确空包初值。`1,2,3` 的左折叠和右折叠应给出不同结果。

解析：不要把所有运算都当加法。非结合运算能看出 fold 方向。

## Part 4：求值顺序

实现 `call_in_order(fs...)`，按传入顺序调用所有可调用对象。

解析：用逗号 fold 或初始化列表。不要用 `+` 等未承诺操作数顺序的运算符承载副作用。

## Part 5：NTTP 和模板模板参数

实现 `constant<V>`、`fixed_string`、`named_value<Name, V>` 和 `apply_unary_template<F, T>`。

解析：`auto` NTTP 让值类型从实参推导；`fixed_string` 让字符串字面量进入类型身份；模板模板参数接收“从类型到类型”的模板。
