# 06 算法契约：前提、结果边界与投影

标准算法不是“循环短写法”。每个算法都有前提、结果形状、复杂度和失效边界。Ranges 算法还把 projection 放进接口，让“取哪个字段”和“怎样比较/判断”分开。写算法前先问三件事：输入满足什么形状，算法会不会重排或写入，返回值表示哪个边界。

本章仍沿用四字段日志记录，不增加新业务字段。算法要服务主案例：过滤非法行后的记录序列、按 timestamp 排序、按 level 查找、按 user_id 统计、取前 N 条高优先级记录。

## 遍历、查找和修改

`for_each` 适合明确副作用；`find`、`find_if`、`count_if` 适合表达查询。Ranges 版本常返回迭代器或富返回类型，右值非 borrowed range 可能返回 `std::ranges::dangling`。这不是麻烦，是防止迭代器离开已销毁对象。

会修改元素的算法必须有可写迭代器。`std::ranges::transform` 写入输出范围时，输入和输出的重叠关系要由算法契约允许；不确定时先写到新容器。对日志记录这种拥有型对象，先收束成 `vector` 再排序/统计，比把所有事情塞进一个惰性管道更容易验证。

## partition 与 sort 的前提不同

`partition` 只保证谓词为 true 的元素在前，两个分区内部顺序不保证；`stable_partition` 保留分区内相对顺序，但成本可能更高。`sort` 要求随机访问范围，比较器必须形成严格弱序。`stable_sort` 还保证等价元素的相对顺序。

排序后的范围才能使用二分算法。`lower_bound`、`binary_search`、`equal_range` 的前提不是“看起来大致有序”，而是按同一个比较器和 projection 有序。用另一套 key 排序，再用当前 key 二分，结果没有契约支撑。

## merge、集合算法与 heap

`merge` 和 `set_union`、`set_intersection`、`set_difference` 都要求两个输入分别有序，并且比较器一致。它们不替你排序；输入错了，输出也没有可解释语义。

堆算法维护的是“父节点不小于子节点”的局部不变量，不是全序数组。`make_heap` 后 `front()` 是最大值；如果要依次拿到降序序列，必须反复 `pop_heap` 再从末尾取出。`priority_queue` 是堆算法的容器适配器：它暴露 `push/top/pop`，故意不暴露迭代器，因为调用方不应该依赖内部数组形状。

本课 L06 会手写有限二叉最大堆，只支持 `int`、固定容量、`push`、`peek_max`、`pop_max`。目标是看清上浮、下沉和容量边界，不是重做 `std::priority_queue`。

## erase-remove、fold 与 projection

顺序容器删除满足条件的元素通常分两步：

```cpp
auto [first, last] = std::ranges::remove_if(records, is_debug);
records.erase(first, last);
```

`remove_if` 不缩短容器，它只把保留元素移动到前面并返回新逻辑尾。真正改变 `vector` 大小的是 `erase`。C++20 有 `std::erase_if`，能直接表达删除；教学里仍保留 erase-remove，是为了看清算法和容器成员的分工。

fold 是“把序列收束成一个值”。如果标准库支持 `std::ranges::fold_left`，它能直接表达累加；没有时用显式循环或 `std::accumulate`。不要把 fold 写成排序分组，除非问题真的需要相邻分组边界。

Projection 在算法内部先于谓词或比较器执行：

```cpp
std::ranges::sort(records, std::less{}, &LogRecord::timestamp);
std::ranges::count_if(records, is_error, &LogRecord::level);
```

成员指针 projection 不只是省 lambda，它把“取字段”从“判断规则”里拿出来。checker 应该检查结果，不检查你是否为了好看多写一层 lambda；但正文要求读者能解释 projection 的位置和适用前提。

## L04 观察题

`L04_algorithms` 用一个完整程序覆盖算法前提：find/count/transform、partition/stable_partition、sort/stable_sort、二分、merge/集合、heap、erase-remove、fold 和 projection。通过只证明这些场景实际运行；它不证明所有比较器、所有容器或所有标准库实现。

解析重点：

1. 二分、merge、集合算法必须吃同口径有序输入。
2. heap 只保证顶端最大，不保证全体排序。
3. `remove_if` 返回逻辑尾，容器大小由后续 `erase` 改变。
4. projection 被算法调用，不是 view 管道层。
5. 稳定算法保留等价元素相对顺序；非稳定算法不承诺。
