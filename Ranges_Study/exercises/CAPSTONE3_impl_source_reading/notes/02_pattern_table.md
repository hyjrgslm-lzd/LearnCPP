# 笔记 02：技术模式对照表

> 对应任务 3 / 验收点：四行全部填完，无空格；iterator_concept 推导列有具体 min 表达式；
> "你之前的误解"列有实质内容。

---

## 对照表

| 库视图 | 核心模式 | iterator_concept 推导 | 缓存 | 你实现的对应练习 | 你的最大收获 | 你之前的误解 |
|--------|----------|-----------------------|------|------------------|--------------|--------------|
| `single_view` | `view_interface` CRTP + 内部存值（movable-box），begin/end 返回裸指针 | `contiguous`（直接 pointer，自动推导） | 无 | 模块 G：实现最小 view | <!-- 填写 --> | <!-- 填写 --> |
| `transform_view` | inner iterator 持有底层迭代器（_M_current）+ parent 裸指针（_M_parent），`operator*` 调用 `std::invoke(*_M_parent->_M_fun, *_M_current)` | `min(底层 concept, random_access_iterator_tag)`；contiguous 底层降为 random_access（operator* 返回 prvalue，不是真实内存引用） | 无 | 模块 G：实现 transform adaptor | <!-- 填写 --> | <!-- 填写 --> |
| `filter_view` | inner iterator + `__non_propagating_cache<optional<iterator_t<V>>>` 缓存 begin 位置；begin() 为非 const 成员函数 | `min(底层 concept, bidirectional_iterator_tag)`；random_access 底层降为 bidirectional（operator- 无法 O(1)） | 有（begin cache，拷贝时重置 reset()） | 模块 H：带缓存的 view | <!-- 填写 --> | <!-- 填写 --> |
| `join_view` | 双层迭代器状态（外层 _M_outer + 内层 _M_inner）；xvalue 内层子范围用 `__non_propagating_cache<inner_range>` 稳定本体；operator++ 先推进内层，到达 end 后推进外层并重置内层 | `min(外层 concept, 内层 concept, bidirectional_iterator_tag)` | 有（xvalue 内层时缓存 inner_range 本体） | 07 文件内部"结课项目 2：源码对照"一节的观察 | <!-- 填写 --> | <!-- 填写 --> |

---

## 补充说明区（阅读笔记）

### single_view 观察
<!-- 在此写阅读 single_view 源码的观察 -->

### transform_view 观察
<!-- 在此写阅读 transform_view 源码的观察 -->

### filter_view 观察
<!-- 在此写阅读 filter_view 源码的观察，重点说明 __non_propagating_cache 的实现细节 -->

### join_view 观察
<!-- 在此写阅读 join_view 源码的观察，重点说明 xvalue 内层缓存的触发条件 -->

---

## 五个问题答案（对应任务 2）

对 single_view / transform_view / filter_view / join_view 各回答一次：

| 问题 | single_view | transform_view | filter_view | join_view |
|------|-------------|----------------|-------------|-----------|
| CPO 还是普通 dispatch？ | <!-- 填写 --> | <!-- 填写 --> | <!-- 填写 --> | <!-- 填写 --> |
| iterator_concept 如何推导？ | contiguous（裸指针） | min(底层, random_access) | min(底层, bidirectional) | min(外层, 内层, bidirectional) |
| 是否有 cache？什么类型？ | 无 | 无 | 有，`__non_propagating_cache<optional<It>>` | xvalue 内层时有，`__non_propagating_cache<inner_range>` |
| operation state 栈对象还是堆对象？ | <!-- 填写 --> | <!-- 填写 --> | <!-- 填写 --> | <!-- 填写 --> |
| 是否用 variant/tuple？ | <!-- 填写 --> | <!-- 填写 --> | <!-- 填写 --> | <!-- 填写 --> |

---

*填写完成后删除此行提示，保留所有表格内容。*
