# 设计文档：mini-ranges 子集实现

mini-ranges 是一个受限教学实现，目标是让学生从零写出可运行的惰性管道：

```cpp
auto v = my::views::iota(1, 11)
       | my::views::transform([](int x) { return x * x; })
       | my::views::take(5)
       | my::ranges::to<std::vector<int>>();
```

它不包装 `std::views` 冒充自写实现。标准库只可作为 good/checker 的行为参照。

## 设计选择

### CPO 层

`begin/end/size/iter_move` 做成函数对象，入口固定在 `my::ranges::*`。函数对象本身不参与 ADL，CPO 内部再按规则调用成员或 ADL fallback。这个结构把“用户调用哪个入口”和“如何发现定制点”拆开，正是 ranges CPO 的核心形态。

### concept 层

`range` 是结构检测；`view` 是显式 opt-in。`std::vector<int>` 满足 range，但不满足 view，因为 view 还包含轻量、可移动、O(1) 值语义承诺。`enable_view<T>` 是这个承诺的开关。

### CRTP 基础设施

`view_interface<D>` 与 `range_adaptor_closure<D>` 都用 CRTP。它们在编译期把派生类作为真实类型使用，避免虚函数和类型擦除。`range | closure` 的语法由 closure 基类注入，具体 closure 只保存参数并实现调用。

### 工厂 view

`iota_view` 是无存储容器的数值序列，iterator 只保存当前整数并支持随机访问。`single_view` 保存一个对象，`begin/end` 返回指针，用最简单方式演示“一个对象也能是 view”。

### adaptor view

`transform_view` 保存底层 view 和函数，解引用时计算结果。它保留底层随机访问移动能力，但不能声明 contiguous，因为 transform 后的值不一定来自连续内存。

`take_view` 使用 wrapper iterator + sentinel。iterator 保存底层当前位置和剩余数量；sentinel 保存底层 end。停止条件是“已取够”或“底层结束”。这比只用 `begin + n` 更通用，也能覆盖非随机访问底层。

### consumer

`to<C>()` 负责把惰性 range 物化到容器。它只要求 `push_back`，不做 reserve、insert-only 容器、关联容器、嵌套转换和 allocator 细节。

## 支持边界

| 项目 | 本题支持 | 本题不支持 |
| --- | --- | --- |
| CPO | array/member/ADL `begin/end`，fallback `iter_move` | 完整标准约束与所有 noexcept 细节 |
| concept | `range/view/input_range/forward_range/borrowed_range` | 完整 `viewable_range`、`sized_range` 组合 |
| view | `iota_view`、`single_view` | `empty_view`、`owning_view` 等完整族 |
| adaptor | `transform_view`、`take_view` | `filter/join/zip/common/as_const` |
| 管道 | `range | transform | take | to` | `closure | closure` 组合存储 |
| consumer | `push_back` 容器 | map/set、reserve 优化、嵌套 `to` |

## 最关键的学习点

真正复杂的不是 `operator|` 语法，而是能力声明必须和实际操作匹配：声明 random access 就要给出完整随机访问运算；声明 view 就要满足轻量值语义；返回 sentinel 就要让 consumer 正确比较 iterator/sentinel。CAPSTONE4 用小代码把这些约束连成一条完整链路。
