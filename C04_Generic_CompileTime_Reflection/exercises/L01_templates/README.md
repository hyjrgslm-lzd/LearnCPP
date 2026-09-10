# 练习 L01：模板声明、实例化与特化

先读 [01. 模板模型](../../chapters/01-template-model.md)。本题只编辑 `src/student/quantity_templates.hpp`。

实现 `l01::quantity<T, Unit>`、`unit_scale_v<Unit>`、`quantity_value_t<Q>`、`same_unit_v<A, B>`、`to_base(q)` 和 `add_same_unit(a, b)`。本题用厘米作整数 base：`centimeters` 倍率为 1，`meters` 为 100，`kilometers` 为 100000。默认主模板倍率为 0，这样 checker 能区分“显式特化”与“碰巧落回默认值”。

Part 1：类模板保存 `value_type`、`unit_type` 和 `value`。构造要能从一个值初始化，不做隐式单位转换。

Part 2：变量模板提供默认倍率，并用显式特化覆盖 `kilometers`。checker 会直接读取变量模板实例，不接受在 `to_base` 里硬编码。

Part 3：别名模板 `quantity_value_t<Q>` 提取 `Q::value_type`。这验证依赖类型名的位置。

Part 4：用偏特化判断同单位 quantity。`same_unit_v<quantity<int, meters>, quantity<long long, meters>>` 为 true；不同单位为 false。

Part 5：`add_same_unit` 只接受同单位，返回同单位 quantity，值为相加结果。不同单位应在 requires 检查中不可调用，不靠运行时报错。

`validation/bad` 会把不同单位也当成可加，必须被 checker 拒绝。`observations/instantiation.cpp` 展示按需实例化：模板里有只对指针合法的成员，不调用它时 `lazy_probe<int>` 仍可使用。

`L01_templates_extern_template` 是多翻译单元 observation：`twice.hpp` 只声明函数模板和 `extern template int twice<int>(int)`，`twice_instantiation.cpp` 提供模板定义和显式实例化，两个 use TU 只调用 `twice<int>`。`L01_templates_missing_extern_provider` 是隔离链接负例，证明漏掉显式实例化提供者时不是语法错误，而是链接边界缺失。ODR 不一致的跨 TU 坏例不作为必然诊断检查，因为这类程序可能属于 ill-formed, no diagnostic required。
