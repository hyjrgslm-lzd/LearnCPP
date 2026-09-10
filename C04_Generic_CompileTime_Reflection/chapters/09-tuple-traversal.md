# 09. `tuple`、`index_sequence` 与异构遍历

`std::tuple` 是一组位置固定、类型可不同的子对象。它常被当成“编译期数组”讲，但更准确的说法是：tuple 把每个元素的类型留在类型系统里，访问时用编译期索引选择具体元素。

本章目标是写一个很小的 `for_each_tuple(tuple, callback)`。它必须按位置顺序访问，保留元素引用和值类别，把 callback 当左值多次调用，空 tuple 零调用，异常停止后续元素，但不回滚 callback 已经做过的外部副作用。

## `get`、`apply` 与结构化绑定

tuple 的基本访问是 `std::get<I>(tuple)`。`I` 是模板实参，必须编译期已知。返回类型保留 tuple 的 cv/ref：对 `tuple<int>&` 得到 `int&`，对 `const tuple<int>&` 得到 `const int&`，对 `tuple<std::unique_ptr<int>>&&` 得到 `std::unique_ptr<int>&&`。

`std::apply(f, tuple)` 把 tuple 元素展开为一次函数调用。它适合“所有元素组成一个调用表达式”的场景：

```cpp
std::apply([](auto&&... xs) { return (xs + ...); }, tuple);
```

结构化绑定适合少量已知字段：

```cpp
auto& [name, score] = record;
```

它不是遍历工具。元素数量未知时，仍需要 index sequence。

## 用 `index_sequence` 生成索引

`std::make_index_sequence<N>` 生成 `0..N-1` 的索引类型。实现常拆成两层：外层推导 tuple 类型和长度，内层拿到索引包。

```cpp
template<class Tuple, class F, std::size_t... Is>
constexpr void for_each_impl(Tuple&& tuple, F& f, std::index_sequence<Is...>) {
    (static_cast<void>(f(std::get<Is>(std::forward<Tuple>(tuple)))), ...);
}
```

这里有三个细节。

第一，`F& f`。callback 是同一个对象，多次调用应累积状态。若每次复制 callback，计数器、输出迭代器和捕获状态都会错。

第二，`std::forward<Tuple>(tuple)` 作用在 `get` 的实参上，才能保留 tuple 元素的值类别。对右值 tuple，move-only 元素能以右值引用交给 callback。

第三，用逗号 fold 保证左到右。用 `+`、`|` 或其他运算符拼调用，不会自动给副作用排序。

## 异常与副作用

如果 callback 在第 2 个元素抛异常，遍历必须停止第 3 个元素。这是普通表达式求值的结果，不需要特殊框架。但已经发生的第 1 个元素副作用不会回滚。C++ 异常只展开栈上对象，不知道你的 callback 给外部 vector push 了什么。

所以文档和检查都要说清楚：异常停止后续访问；外部副作用由调用者自己设计事务或补偿。

## 练习与反例

L09 的 Student 只编辑 `src/student/tuple_for_each.hpp`。checker 覆盖空 tuple、左到右顺序、callback 左值复用、引用写回、const 引用、move-only 元素右值访问，以及异常停止不回滚已发生副作用。

bad 控制实现反向访问二元 tuple，checker 会以 `tuple traversal must preserve left-to-right order` 拒绝。观察程序对照 `std::apply` 的一次调用和 index sequence 的逐元素调用，说明两者适用场景不同。
