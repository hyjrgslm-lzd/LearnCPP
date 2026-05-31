# 笔记 03：CPO 使用清单

> 对应任务 4（间接）/ 验收点：至少列出 ranges::begin / end / iter_move / iter_swap /
> advance / distance / size / data 八项，每项标注定制意图。

---

## CPO 清单（8 项必填）

### 1. `ranges::begin`

**定制意图**：获取范围的起始迭代器。优先级链：成员 `begin()` → ADL `begin(r)` → 数组退化。
作为函数对象（不参与 ADL），阻止第三方 `begin` 函数意外劫持行为。

**libstdc++ 查找位置**：`<bits/ranges_base.h>`，`namespace __cust_access`

**你对 filter_view 的 dispatch 路径**：
<!-- 填写：ranges::begin(fv) → 调用 fv.begin()（成员函数）→ 触发缓存写入 _M_begin -->

---

### 2. `ranges::end`

**定制意图**：获取范围的哨兵（sentinel），类型可与 begin 不同（C++20 iterator/sentinel 分离）。

**libstdc++ 查找位置**：同 begin，`namespace __cust_access`

**你的观察**：
<!-- 填写：take_view 的 end() 返回异型 sentinel，ranges::end 通过成员 end() 调用得到 -->

---

### 3. `ranges::iter_move`

**定制意图**：从迭代器位置移出值，支持 proxy reference 迭代器的正确语义。
区别：`std::move(*it)` 对 proxy reference 只移动 proxy 本身，不移动底层元素；
`ranges::iter_move` 通过 hidden-friend 定制可正确处理 proxy 迭代器（如 zip_view）。

**libstdc++ 查找位置**：`<bits/iterator_concepts.h>`

**transform_view 的定制方式**：
<!-- 填写：transform_view::iterator 通过 hidden-friend iter_move，把 ranges::iter_move 转发到底层迭代器并对结果应用变换 -->

---

### 4. `ranges::iter_swap`

**定制意图**：交换两个迭代器位置的值，支持 proxy reference 的正确 element-wise 交换。
`std::swap(*it1, *it2)` 对 proxy reference 语义错误；`ranges::iter_swap` 可定制。

**libstdc++ 查找位置**：`<bits/iterator_concepts.h>`

**你的观察**：
<!-- 填写：zip_view 的 iter_swap 逐元素调用 ranges::iter_swap，tuple-of-refs 正确交换 -->

---

### 5. `ranges::advance`

**定制意图**：移动迭代器 n 步，自动根据迭代器 concept 选择最优路径
（random_access → `it += n`；其他 → 逐步 ++/--）。

**libstdc++ 查找位置**：`<bits/ranges_base.h>`

**你的观察**：
<!-- 填写 -->

---

### 6. `ranges::distance`

**定制意图**：计算两迭代器间距离，或范围长度。
对 sized_range 直接调用 `ranges::size(r)`；否则逐步迭代计数。

**libstdc++ 查找位置**：`<bits/ranges_base.h>`

**filter_view 的行为**：
<!-- 填写：filter_view 不是 sized_range，ranges::distance(fv) 必须遍历全部元素，O(N) -->

---

### 7. `ranges::size`

**定制意图**：获取范围元素数，仅对 sized_range 有效。
view_interface 注入的 `size()` 依赖底层满足 `forward_range && sized_sentinel_for`。

**libstdc++ 查找位置**：`<bits/ranges_base.h>`

**filter_view 的静默失效**：
<!-- 填写：filter_view 底层时 view_interface::size() 注入失效——因为 filter_view 的 sentinel 不满足 sized_sentinel_for<iterator>（无法 O(1) 计算距离）-->

---

### 8. `ranges::data`

**定制意图**：获取连续范围的底层指针，仅对 contiguous_range 有效。
优先级链：成员 `data()` → `to_address(ranges::begin(r))`。

**libstdc++ 查找位置**：`<bits/ranges_base.h>`

**你的观察**：
<!-- 填写：single_view 满足 contiguous_range，ranges::data(sv) 直接返回 &_v -->

---

## ranges CPO vs stdexec tag_invoke 对比（任务 4）

| 维度 | ranges CPO（`ranges::begin` 等） | stdexec tag_invoke |
|------|----------------------------------|--------------------|
| **定制入口形式** | `inline constexpr` 函数对象，"成员优先 → ADL 次之 → 回退"优先级链 | `tag_invoke(tag_t, ...)` 自由函数，类型 tag 作为第一参数携带语义 |
| **ADL 隔离方式** | CPO 本身是函数对象，不参与 ADL；内部再决定调成员还是走 ADL | tag_invoke 仍依赖 ADL，但所有定制集中在同名函数，避免名字冲突 |
| **多态点结构** | 每个操作有专属 CPO，多态点分散但命名清晰 | 所有 CPO 共用 `tag_invoke`，由第一参数 tag 类型区分，更集中但更难阅读 |
| **扩展对称性** | 不支持用户定义全新 CPO 与现有框架平等（`range_adaptor_closure` 是例外，P2387R3） | 天然允许用户定义新 tag，扩展点对称 |
| **编译期查询** | 直接用于 concept 约束：`requires { ranges::begin(r) }` | 依赖 `tag_invocable<tag_t, Args...>` concept 检测 |
| **你的补充维度** | <!-- 填写 --> | <!-- 填写 --> |

---

*填写完成后删除此行提示，保留所有清单和对比表。*
