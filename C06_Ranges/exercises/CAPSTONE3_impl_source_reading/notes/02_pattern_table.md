# 笔记 02：技术模式对照表

固定输入同 `references/source-reading.md`：MSVC STL 145 / `_MSVC_STL_UPDATE=202604L`，源码入口为本机 `<ranges>`。
源码阅读结论不等于标准强制实现布局。

| 实现 | 核心模式 | iterator_concept 推导 | cache | 对应课程主题 | 之前的误解 | 修正后的判断 |
|---|---|---|---|---|---|---|
| `single_view` / `empty_view` / `iota_view` | 完成型 view；对象自己提供 begin/end，不需要 parent 指针 | `single_view` 用指针得到 contiguous；整数 `iota_view` 可到 random_access；`empty_view` 无状态且 sized | 无 | 模块 F：`view_interface` 和完成型 view | 以为所有 view 都包装一个底层 range | 完成型 view 可以直接拥有元素或只表达一个生成规则 |
| `transform_view` | inner iterator = 当前底层 iterator + parent 指针；解引用时调用函数对象 | `min(底层, random_access)`；contiguous 会降为 random_access，因为解引用可能产出 prvalue | 无 | 模块 G：inner iterator 与 projection | 以为底层 contiguous 会继续 contiguous | 连续存储性质要求引用真实相邻对象，transform 的 prvalue 不满足 |
| `filter_view` | iterator 跳过不满足谓词的元素；parent 提供谓词和 base | `min(底层, bidirectional)`；random_access 降级，因为无法 O(1) 跳过任意数量已拒绝元素 | forward 底层有 begin cache，拷贝/移动时 reset | 模块 H：`__non_propagating_cache` | 以为 filter 永远没有 const begin | N5047/P3725R3 只给 input-only const 支线；普通 forward/vector 缓存路径仍是非 const begin |
| `join_view` | 双层状态：外层子范围位置 + 内层元素位置；空子范围循环跳过 | `min(外层, 内层, bidirectional)`，且受 common/reference 类别约束 | xvalue 内层子范围需要 cache 稳定本体 | 模块 H：双层迭代与临时子范围 | 以为 join 只是两层 for 循环语法糖 | 实现必须管理内层对象生命期和空子范围推进，缓存动机不同于 filter |

五个问题答案：

1. CPO 先把公开操作压成一个稳定入口，再由成员、ADL 或回退分支决定实际调用；实现类不需要暴露私有 helper。
2. inner iterator 指向 parent，是因为函数对象、谓词和 inner cache 属于 view 对象，不属于每个迭代器副本。
3. cache 类型不是性能装饰。`filter_view` 缓存 begin 扫描结果；`join_view` 缓存 xvalue 子范围本体。一个解决摊还复杂度，一个解决对象生命期。
4. `iterator_concept` 描述 ranges 新算法可用能力；`iterator_category` 面向旧算法，遇到 proxy/prvalue 时常需要降级。
5. `borrowed_range` 只说明迭代器离开 range 对象后是否仍可用，不延长被借用容器、临时子范围或 parent view 的生命期。
