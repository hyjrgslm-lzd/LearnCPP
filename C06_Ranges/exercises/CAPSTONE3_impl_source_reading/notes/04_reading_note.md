# 笔记 04：我现在如何向别人解释 stdlib ranges 的实现架构

我会先从 CPO 讲起。`std::ranges::begin`、`end`、`size`、`iter_move` 和 `iter_swap`
把公开操作变成一组函数对象，算法只调用这些入口，不把成员查找、ADL、数组退化和 proxy 处理散落在每个算法里。
这也是 ranges 比旧算法更能处理 sentinel 和 proxy iterator 的原因。

第二层是 `view_interface` 和 `range_adaptor_closure`。`view_interface` 用 CRTP 给派生 view
补 `empty`、`front`、`back`、`operator[]` 这类派生操作，前提由 concept 控制。`range_adaptor_closure`
让 `range | views::transform(f) | views::filter(p)` 这种组合只构造轻量对象，真正遍历留给 begin 和 `operator++`。

第三层是每个 view 的状态。`transform_view` 的 iterator 持有当前位置和 parent 指针，因为函数对象在 parent view 里。
`filter_view` 多了 `__non_propagating_cache`，缓存第一次 begin 扫描到的位置，但拷贝和移动 view 时重置缓存，避免新 view
继承旧 view 的迭代状态。`join_view` 也可能用 cache，不过它缓存的是 xvalue 内层子范围本体，目的是保护生命期，不是 begin
摊还复杂度。

第四层是 iterator 能力声明。`iterator_concept` 给 ranges 新算法看，`iterator_category` 给旧算法看。proxy reference 或
prvalue 解引用会让旧 category 降级，即使新 concept 还能保持 random access。`borrowed_range` 也要分开讲：它只说明迭代器
是否能脱离 range 对象使用，不会延长容器或临时子范围的生命期。

源码阅读的结论必须绑定版本。本次观察绑定 MSVC STL 145、`_MSVC_STL_UPDATE=202604L` 和本机 include 目录；这些私有名字和行号是实现证据，
不是标准要求。标准层面的新变化要回到固定提案和编辑报告，例如 N5047/P3725R3 给 `filter_view` 增加受限 const 支线，
但普通 forward/vector 缓存路径仍按非 const begin 理解。
