# 笔记：mini-ranges 六层架构图

> 对应任务 1 / 验收点 6：能拿着六层架构图解释整个系统，指出每层的职责和层间依赖方向。

---

## 占位骨架（任务 1：实现前先画架构图）

```
【实现前填写】

层间依赖方向：上层 include 下层，依赖方向从上到下。

┌─────────────────────────────────────────────────────────────────┐
│  层 6 — 消费层 (06_consumers.hpp)                               │
│  ranges::to<C>                                                  │
│  职责：将 range 元素收集到容器；管道最终出口                      │
│  继承：range_adaptor_closure（可出现在管道右侧）                  │
├─────────────────────────────────────────────────────────────────┤
│  层 5 — closure 层 (05_adaptors.hpp)                            │
│  _transform_closure / _take_closure                             │
│  职责：持有参数（函数/数量），等待 range 输入；注入 operator|     │
│  继承：range_adaptor_closure                                     │
├─────────────────────────────────────────────────────────────────┤
│  层 4/5 — view 层 (04_factories.hpp, 05_adaptors.hpp)           │
│  iota_view / single_view / transform_view / take_view           │
│  职责：持有数据或底层 view，提供 begin/end；惰性求值              │
│  继承：view_interface（注入 empty/front 等）                     │
├─────────────────────────────────────────────────────────────────┤
│  层 3 — 基础设施层 (03_interface.hpp)                            │
│  view_interface<D> + range_adaptor_closure<D>                   │
│  职责：CRTP 基类，零开销注入接口；无虚函数                        │
├─────────────────────────────────────────────────────────────────┤
│  层 2 — 概念层 (02_concepts.hpp)                                 │
│  range / view / input_range / forward_range                     │
│  职责：编译期约束，描述类型能力；不产生运行时代码                  │
├─────────────────────────────────────────────────────────────────┤
│  层 1 — CPO 层 (01_cpo.hpp)                                     │
│  begin / end / iter_move / size                                 │
│  职责：函数对象，ADL 隔离；统一接口入口；enable_view 变量模板     │
└─────────────────────────────────────────────────────────────────┘
```

---

## 详细层说明（实现后补全）

### 层 1 — CPO 层

**文件**：`my_ranges/01_cpo.hpp`

**核心内容**：
- `my::ranges::begin` / `end`：`inline constexpr` 函数对象，成员优先
- `my::ranges::iter_move`：hidden-friend 优先，回退 `std::move(*it)`
- `enable_view<T>` / `enable_borrowed_range<T>`：变量模板，默认 false

**为什么用函数对象而非函数模板**：
<!-- 填写：ADL 隔离——函数对象不参与 ADL，阻止第三方同名函数劫持 -->

---

### 层 2 — 概念层

**文件**：`my_ranges/02_concepts.hpp`

**核心内容**：
- `range`：能调用 begin/end
- `view`：range + movable + enable_view = true
- `input_range` / `forward_range`：约束链递增

**约束链图**：
```
range
  └─ input_range (+ input_iterator)
       └─ forward_range (+ forward_iterator)
            └─ [进阶] bidirectional_range
                 └─ [进阶] random_access_range
                      └─ [进阶] contiguous_range
```

---

### 层 3 — 基础设施层

**文件**：`my_ranges/03_interface.hpp`

**为什么用 CRTP 而非虚函数**：
<!-- 填写：零开销，与 O(1) 公理兼容；虚函数有 vtable 开销，且阻止某些优化 -->

**operator| 的两条路径**：
<!-- 填写：1) range | closure → d(forward(r))；2) [进阶] closure | closure → _Pipe{c1,c2} -->

---

### 层 4 — 工厂 view

**文件**：`my_ranges/04_factories.hpp`

**iota_view 特性**：iterator_concept = random_access；enable_borrowed_range = true（迭代器只持有当前整数，不依赖 view 对象）

**single_view 特性**：begin/end = 裸指针（iterator_concept 自动为 contiguous）；无底层 view 依赖

---

### 层 5 — adaptor view + closure

**文件**：`my_ranges/05_adaptors.hpp`

**transform_view 关键点**：
<!-- 填写：inner iterator 持有 _cur + _fp；contiguous 底层降为 random_access -->

**take_view sentinel 异型**：
<!-- 填写：end() 返回 sentinel 类型 ≠ begin() 返回 iterator_t<V>；to<Container> 消费时需要 sentinel 可比较 -->

---

### 层 6 — 消费端

**文件**：`my_ranges/06_consumers.hpp`

**简化点**：
<!-- 填写：不做 reserve；不支持 insert-based 容器；不支持嵌套 ranges::to -->

---

## 完成验收样例

实现后补充运行结果：

```
输入管道：iota(1, 11) | transform(x*x) | take(5) | to<vector<int>>()
期望输出：{1, 4, 9, 16, 25}
实际输出：<!-- 填写 -->
```

---

*填写完成后删除此行提示，保留图表与说明。*
